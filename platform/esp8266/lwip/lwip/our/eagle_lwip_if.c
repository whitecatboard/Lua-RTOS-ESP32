#include "lwip/netif.h"
#include "netif/etharp.h"
#include "lwip/app/dhcpserver.h"

#define QUEUE_LEN 10
#define TASK_IF0_PRIO 28
#define TASK_IF1_PRIO 29
#define DEFAULT_MTU 0x5dc

err_t ieee80211_output_pbuf(struct netif *netif, struct pbuf *p);
extern int *g_ic[];
extern uint16_t dhcps_flag;

struct netif *eagle_lwip_getif(int n);
struct myif_state {
    struct netif *myif;
    uint32_t padding[39];
    uint32_t dhcps_if;
};

static void
task_if0(struct ETSEventTag *e)
{
    struct netif *myif = eagle_lwip_getif(0);

    if (e->sig == 0) {
        myif->input((struct pbuf *)(e->par), myif);
    }
}

static void
task_if1(struct ETSEventTag *e)
{
    struct netif *myif = eagle_lwip_getif(1);

    if (e->sig == 0) {
        myif->input((struct pbuf *)(e->par), myif);
    }
}

static err_t
init_fn(struct netif *myif)
{
    myif->hwaddr_len = 6;
    myif->mtu = DEFAULT_MTU;
    myif->flags = NETIF_FLAG_IGMP | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;
    myif->flags |= NETIF_FLAG_BROADCAST;
    return 0;
}

struct netif *eagle_lwip_getif(int n)
{
    int *ret;

    if (n==0) {
        ret = g_ic[4];
    } else if (n == 1) {
        ret = g_ic[5];
    } else {
        return (struct netif *)n;
    }

    if (ret == NULL) {
        return 0;
    }

    return (struct netif *)*ret;
}


struct netif *
eagle_lwip_if_alloc(struct myif_state *state, u8_t hw[6], ip_addr_t *ips)
{
    struct ETSEventTag *queue;
    struct netif *myif;
    ip_addr_t ipaddr = ips[0];
    ip_addr_t netmask = ips[1];
    ip_addr_t gw = ips[2];

    if (state->myif == NULL) {
        myif = (void *)os_zalloc(sizeof(struct netif));
    }

    myif->state = state;
    myif->name[0] = 'e';
    myif->name[1] = 'w';
    myif->linkoutput = ieee80211_output_pbuf;
    myif->output = etharp_output;
    ets_memcpy(myif->hwaddr, hw, 6);

    queue = (void *) os_malloc(sizeof(struct ETSEventTag) * QUEUE_LEN);

    if (state->dhcps_if == 0) {
#if LWIP_NETIF_HOSTNAME
#ifdef LWIP_NETIF_HOSTNAME_PREFIX
        myif->hostname = os_malloc(sizeof(LWIP_NETIF_HOSTNAME_PREFIX)+10);
        os_sprintf(myif->hostname, "%s%02x%02x%02x", LWIP_NETIF_HOSTNAME_PREFIX,
                myif->hwaddr[3], myif->hwaddr[4], myif->hwaddr[5]);
#endif
#endif

        ets_task(task_if0, TASK_IF0_PRIO, queue, QUEUE_LEN);
    } else {
        netif_set_addr(myif, &ipaddr, &netmask, &gw);
        if (dhcps_flag != 0) {
            dhcps_start((struct ip_info *)ips);
            os_printf("dhcp server start:(ip:%08x,mask:%08x,gw:%08x)\n",
                    ipaddr.addr, netmask.addr, gw.addr);
        }

        ets_task(task_if1, TASK_IF1_PRIO, queue, QUEUE_LEN);
    }

    netif_add(myif, &ipaddr, &netmask, &gw, state, init_fn, ethernet_input);

    return myif;
}
