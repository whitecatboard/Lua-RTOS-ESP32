// From: https://github.com/goertzenator/lwip/blob/master/contrib-1.4.0/apps/ping/ping.c


#include "lwip/ip.h"
#include "lwip/ip4.h"
#include "lwip/err.h"
#include "lwip/icmp.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#ifndef PING_DEBUG
#define PING_DEBUG     LWIP_DBG_ON
#endif

/** ping target - should be a "ip_addr_t" */
#ifndef PING_TARGET
#define PING_TARGET   0
#endif

/** ping receive timeout - in milliseconds */
#ifndef PING_RCV_TIMEO
#define PING_RCV_TIMEO 1000
#endif

/** ping delay - in milliseconds */
#ifndef PING_DELAY
#define PING_DELAY     1000
#endif

/** ping identifier - must fit on a u16_t */
#ifndef PING_ID
#define PING_ID        0xAFAF
#endif

/** ping additional data size to include in the packet */
#ifndef PING_DATA_SIZE
#define PING_DATA_SIZE 32
#endif

/** ping result action - no default action */
#ifndef PING_RESULT
#define PING_RESULT(ping_ok)
#endif

static u16_t ping_seq_num;
static u32_t ping_time;

static void ping_prepare_echo( struct icmp_echo_hdr *iecho, u16_t len) {
  size_t i;
  size_t data_len = len - sizeof(struct icmp_echo_hdr);

  ICMPH_TYPE_SET(iecho, ICMP_ECHO);
  ICMPH_CODE_SET(iecho, 0);
  iecho->chksum = 0;
  iecho->id     = PING_ID;
  iecho->seqno  = htons(++ping_seq_num);

  /* fill the additional data buffer with some data */
  for(i = 0; i < data_len; i++) {
    ((char*)iecho)[sizeof(struct icmp_echo_hdr) + i] = (char)i;
  }

 // TO DO iecho->chksum = inet_chksum(iecho, len);
}

static err_t ping_send(int s, ip4_addr_t *addr) {
  int err;
  struct icmp_echo_hdr *iecho;
  struct sockaddr_in to;
  size_t ping_size = sizeof(struct icmp_echo_hdr) + PING_DATA_SIZE;
  LWIP_ASSERT("ping_size is too big", ping_size <= 0xffff);

  iecho = (struct icmp_echo_hdr *)mem_malloc((mem_size_t)ping_size);
  if (!iecho) {
    return ERR_MEM;
  }

  ping_prepare_echo(iecho, (u16_t)ping_size);

  to.sin_len = sizeof(to);
  to.sin_family = AF_INET;
  inet_addr_from_ipaddr(&to.sin_addr, addr);

  err = sendto(s, iecho, ping_size, 0, (struct sockaddr*)&to, sizeof(to));

  mem_free(iecho);

  return (err ? ERR_OK : ERR_VAL);
}

static void ping_recv(int s) {
  char buf[64];
  int fromlen, len;
  struct sockaddr_in from;
  struct ip_hdr *iphdr;
  struct icmp_echo_hdr *iecho;

  while((len = recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr*)&from, (socklen_t*)&fromlen)) > 0) {
    if (len >= (int)(sizeof(struct ip_hdr)+sizeof(struct icmp_echo_hdr))) {
      ip4_addr_t fromaddr;
      inet_addr_to_ipaddr(&fromaddr, &from.sin_addr);

      iphdr = (struct ip_hdr *)buf;
      iecho = (struct icmp_echo_hdr *)(buf + (IPH_HL(iphdr) * 4));
      if ((iecho->id == PING_ID) && (iecho->seqno == htons(ping_seq_num))) {
        /* do some ping result processing */
        return;
      } else {
      }
    }
  }

  if (len == 0) {
  }
}

static void ping_thread(void *arg) {
  int s;
  int timeout = PING_RCV_TIMEO;
  ip4_addr_t ping_target;

  LWIP_UNUSED_ARG(arg);

  if ((s = socket(AF_INET, SOCK_RAW, IP_PROTO_ICMP)) < 0) {
    return;
  }

  setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

  while (1) {
    ping_target.addr = 0;

    if (ping_send(s, &ping_target) == ERR_OK) {
    	 // TO DO
      ping_time = sys_now();
      ping_recv(s);
    } else {
    	 // TO DO
    }
 // TO DO
    sys_msleep(PING_DELAY);
  }
}
