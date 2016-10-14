#ifndef __LWIPOPTS_H__
#define __LWIPOPTS_H__

#include "mem_manager.h"
#include "eagle_soc.h"

// Don't change:
#define NO_SYS                              1
#define NO_SYS_NO_TIMERS                    0
#define LWIP_NETIF_TX_SINGLE_PBUF           1
#define LWIP_ESP                            1
#define PBUF_RSV_FOR_WLAN                   1
#define ICACHE_FLASH                        1
#define EBUF_LWIP                           1

// Leave unchanged unless you really know what you're doing:
#define MEM_ALIGNMENT                       4
#define TCP_MSS                             1460
#define TCP_SND_BUF                         (2*TCP_MSS)
#define MEMP_MEM_MALLOC                     1
#define MEM_LIBC_MALLOC                     1

#define MEMP_NUM_TCP_PCB                    (*((volatile uint32*)0x600011FC))
#define LWIP_RAND()                         rand()

#if MEM_LIBC_MALLOC
#define mem_free                            vPortFree
#define mem_malloc                          pvPortMalloc
#define mem_calloc                          pvPortCalloc
#endif

#define MEMCPY(dst,src,len)                 os_memcpy(dst,src,len)
#define SMEMCPY(dst,src,len)                os_memcpy(dst,src,len)

static inline uint32_t sys_now(void)
{
    return NOW()/(TIMER_CLK_FREQ/1000);
}

// For espconn:
#define os_malloc(s)                        pvPortMalloc((s))
#define os_realloc(p, s)                    pvPortRealloc((p), (s))
#define os_zalloc(s)                        pvPortZalloc((s))
#define os_free(p)                          vPortFree((p))

// Required:
#define LWIP_DHCP                           1
#define LWIP_DNS                            1

// Optional:
#define LWIP_IGMP                           1
#define LWIP_NETIF_LOOPBACK                 0
#define LWIP_HAVE_LOOPIF                    0

// Tweakable:
#define ESP_TIMEWAIT_THRESHOLD              10000

#define TCP_TMR_INTERVAL                    125
#define TCP_KEEPIDLE_DEFAULT                3000
#define TCP_KEEPINTVL_DEFAULT               1000
#define TCP_KEEPCNT_DEFAULT                 3

#define LWIP_NETCONN                        0
#define LWIP_SOCKET                         0
#define MEMP_NUM_SYS_TIMEOUT                8

#define TCP_LOCAL_PORT_RANGE_START          0x1000
#define TCP_LOCAL_PORT_RANGE_END            0x7fff
#define UDP_LOCAL_PORT_RANGE_START          0x1000
#define UDP_LOCAL_PORT_RANGE_END            0x7fff

#define ARP_QUEUEING                        1
#define ETHARP_TRUST_IP_MAC                 1
#define IP_FRAG                             0
#define IP_REASSEMBLY                       0
#define IP_FRAG_USES_STATIC_BUF             1
#define TCP_QUEUE_OOSEQ                     0
#define LWIP_TCP_KEEPALIVE                  1
#define LWIP_STATS                          0

#ifdef LWIP_OUR_IF
  #define LWIP_NETIF_HOSTNAME               1 // our eagle_lwip_if.o required
  #define LWIP_NETIF_HOSTNAME_PREFIX        "esp8266-"
#endif

// #define LWIP_DEBUG
// #define IP_DEBUG                            LWIP_DBG_ON

#endif /* __LWIP_OPT_H__ */
