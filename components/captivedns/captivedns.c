/*******************************************************************************
 * Copyright (c) 2017
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 ******************************************************************************/
#include "sdkconfig.h"

#if CONFIG_LUA_RTOS_USE_HTTP_SERVER

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "lwip/ip_addr.h"
#include "lwip/udp.h"
#include "lwip/pbuf.h"
#include "lwip/tcpip.h"

#include "esp_wifi_types.h"
#include "tcpip_adapter.h"
#include <sys/driver.h>
#include <sys/panic.h>
#include <esp_wifi.h>

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/syslog.h>

#define PORT           53

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#define DNS_QR_QUERY 0
#define DNS_QR_RESPONSE 1
#define DNS_OPCODE_QUERY 0

struct DNSHeader
{
  uint16_t ID;               // identification number
  unsigned char RD : 1;      // recursion desired
  unsigned char TC : 1;      // truncated message
  unsigned char AA : 1;      // authoritive answer
  unsigned char OPCode : 4;  // message_type
  unsigned char QR : 1;      // query/response flag
  unsigned char RCode : 4;   // response code
  unsigned char Z : 3;       // its z! reserved
  unsigned char RA : 1;      // recursion available
  uint16_t QDCount;          // number of question entries
  uint16_t ANCount;          // number of answer entries
  uint16_t NSCount;          // number of authority entries
  uint16_t ARCount;          // number of resource entries
};

static tcpip_adapter_ip_info_t esp_info;
static struct udp_pcb *captivedns_pcb;
static u8_t captivedns_pcb_refcount;
static void captivedns_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port);
driver_error_t *wifi_check_error(esp_err_t error);

/** Ensure captivedns PCB is allocated and bound */
static err_t
captivedns_inc_pcb_refcount(void)
{
  if(captivedns_pcb_refcount == 0) {
    LWIP_ASSERT("captivedns_inc_pcb_refcount(): memory leak", captivedns_pcb == NULL);

    LOCK_TCPIP_CORE()

    /* allocate UDP PCB */
    captivedns_pcb = udp_new(); // NOTE regarding possible crashes with TCP: https://github.com/espressif/esp-idf/issues/2113#issuecomment-405777313
                                // not 100% sure about UDP - but to be safe let's use TCPIP_CORE locking

    if(captivedns_pcb == NULL) {
      UNLOCK_TCPIP_CORE()
      return ERR_MEM;
    }

    /* set up local port for the pcb -> listen on all interfaces on all src/dest IPs */
    if (udp_bind(captivedns_pcb, IP_ADDR_ANY, PORT) != 0) {
      udp_remove(captivedns_pcb);
      captivedns_pcb = NULL;
      UNLOCK_TCPIP_CORE()
      return ESP_ERR_INVALID_STATE;
    }

    udp_recv(captivedns_pcb, captivedns_recv, NULL);
    UNLOCK_TCPIP_CORE()

    //we only need one captivedns pcb
    captivedns_pcb_refcount++;
  }

  return ERR_OK;
}

/** Free captivedns PCB if the last netif stops using it */
static void
captivedns_dec_pcb_refcount(void)
{
  if(captivedns_pcb_refcount) {
    captivedns_pcb_refcount--;

    LOCK_TCPIP_CORE()
    udp_remove(captivedns_pcb);
    captivedns_pcb = NULL;
    UNLOCK_TCPIP_CORE()
  }
}

/**
 * In case of an incoming dns message, trigger the state machine
 */
static void
captivedns_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
  struct netif *netif = ip_current_input_netif();
  struct DNSHeader* dnsHeader = (struct DNSHeader*)p->payload;

  if (dnsHeader->QR == DNS_QR_QUERY && dnsHeader->OPCode == DNS_OPCODE_QUERY &&
      /* request only contains one question */
      ntohs(dnsHeader->QDCount) == 1 && dnsHeader->ANCount == 0 &&
      dnsHeader->NSCount == 0 && dnsHeader->ARCount == 0
      /* request only contains one question */
     ) {

      dnsHeader->QR = DNS_QR_RESPONSE;
      dnsHeader->ANCount = dnsHeader->QDCount;

      struct pbuf *p_out = pbuf_alloc(PBUF_TRANSPORT, p->len + 16, PBUF_RAM);
      if (NULL != p_out) {
        pbuf_take(p_out, p->payload, p->len); //cannot use tot_len here

        u16_t position = p->len - 1;
        pbuf_put_at(p_out, ++position, (uint8_t)192); // answer name is a pointer
        pbuf_put_at(p_out, ++position, (uint8_t)12);  // pointer to offset at 0x00c

        pbuf_put_at(p_out, ++position, (uint8_t)0);   // 0x0001  answer is type A query (host address)
        pbuf_put_at(p_out, ++position, (uint8_t)1);

        pbuf_put_at(p_out, ++position, (uint8_t)0);   // 0x0001 answer is class IN (internet address)
        pbuf_put_at(p_out, ++position, (uint8_t)1);

        pbuf_put_at(p_out, ++position, (uint8_t)0);   // fixed TTL of zero
        pbuf_put_at(p_out, ++position, (uint8_t)0);
        pbuf_put_at(p_out, ++position, (uint8_t)0);
        pbuf_put_at(p_out, ++position, (uint8_t)0);

        pbuf_put_at(p_out, ++position, (uint8_t)0);   // length of RData is 4 bytes (because, in this case, RData is IPv4)
        pbuf_put_at(p_out, ++position, (uint8_t)4);

        //esp32 ip addr
        pbuf_put_at(p_out, ++position, (uint8_t)ip4_addr1(&esp_info.ip));
        pbuf_put_at(p_out, ++position, (uint8_t)ip4_addr2(&esp_info.ip));
        pbuf_put_at(p_out, ++position, (uint8_t)ip4_addr3(&esp_info.ip));
        pbuf_put_at(p_out, ++position, (uint8_t)ip4_addr4(&esp_info.ip));

        LOCK_TCPIP_CORE()
        udp_sendto_if(captivedns_pcb, p_out, addr, port, netif);
        UNLOCK_TCPIP_CORE()

        pbuf_free(p_out);

      }
  }

  pbuf_free(p);
}

int captivedns_start(lua_State* L) {
  driver_error_t *error;

  wifi_mode_t mode;
  if ((error = wifi_check_error(esp_wifi_get_mode(&mode)))) {
    free(error);
    return luaL_error(L, "couldn't get wifi mode");
  }
  
  if (mode == WIFI_MODE_STA) {
    return luaL_error(L, "wrong wifi mode");
  }

  // get WIFI IF info
  if ((error = wifi_check_error(tcpip_adapter_get_ip_info(ESP_IF_WIFI_AP, &esp_info)))) {
    free(error);
    return luaL_error(L, "couldn't get interface information");
  }
  
  if(captivedns_inc_pcb_refcount()!=ERR_OK) {
    return luaL_error(L, "couldn't start captivedns service");
  }
  
  return 0;
}

void captivedns_stop() {
  captivedns_dec_pcb_refcount();
}

int captivedns_running() {
  return (captivedns_pcb_refcount>0 ? 1 : 0);
}

#endif
