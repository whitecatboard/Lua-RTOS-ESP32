/*
 * Whitecat, platform functions for lua net module
 *
 * Copyright (C) 2015 - 2016
 * IBEROXARXA SERVICIOS INTEGRALES, S.L. & CSS IBÉRICA, S.L.
 * 
 * Author: Jaume Olivé (jolive@iberoxarxa.com / jolive@whitecatboard.org)
 * 
 * All rights reserved.  
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appear in all
 * copies and that both that the copyright notice and this
 * permission notice and warranty disclaimer appear in supporting
 * documentation, and that the name of the author not be used in
 * advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * The author disclaim all warranties with regard to this
 * software, including all implied warranties of merchantability
 * and fitness.  In no event shall the author be liable for any
 * special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, ƒintedata or profits, whether
 * in an action of contract, negligence or other tortious action,
 * arising out of or in connection with the use or performance of
 * this software.
 */
#include "whitecat.h"

#if LUA_USE_NET

#include <string.h>

#include "lwip/inet.h"
#include "lwip/tcp.h"
#include "lwip/ip_frag.h"
#include "lwip/netif.h"
#include "lwip/init.h"
#include "lwip/stats.h"
#include "netif/ethernetif.h"
#include "netif/etharp.h"
#include "lwip/ip_addr.h"
#include "netif/ppp/ppp.h"
#include "lwip/tcpip.h"

#include <drivers/network/network.h>

extern struct netif eth_netif;
extern struct netif gprs_netif;
extern struct netif wifi_netif;
extern ip_addr_t  dns1;
extern ip_addr_t  dns2;

int platform_net_exists(const char *interface) {
    int exists;
    
    exists  = (strcmp(interface,"en") == 0);
    exists |= (strcmp(interface,"gprs") == 0);
    
    //exists |= strcmp(interface,"wf");
    
    return exists;
}

void platform_net_stat_iface(const char *interface) {
    struct netif *netif;
    
    if (!platform_net_exists(interface)) {
        return;
    }
    
    if (strcmp(interface,"en") == 0) {
        netif = &eth_netif;
    }

    if (strcmp(interface,"gprs") == 0) {
        netif = &gprs_netif;
    }

    if (strcmp(interface,"wf") == 0) {
        netif = &wifi_netif;
    }

    printf(
            "%s: mac address %02x:%02x:%02x:%02x:%02x:%02x\n",
            interface,
            netif->hwaddr[0],netif->hwaddr[1],
            netif->hwaddr[2],netif->hwaddr[3],
            netif->hwaddr[4],netif->hwaddr[5]
    );

    printf(
            "\tip address %"U16_F".%"U16_F".%"U16_F".%"U16_F" netmask %"U16_F".%"U16_F".%"U16_F".%"U16_F"\n",
            ip4_addr1_16(&netif->ip_addr),    
            ip4_addr2_16(&netif->ip_addr),
            ip4_addr3_16(&netif->ip_addr),
            ip4_addr4_16(&netif->ip_addr),
            ip4_addr1_16(&netif->netmask),    
            ip4_addr2_16(&netif->netmask),
            ip4_addr3_16(&netif->netmask),
            ip4_addr4_16(&netif->netmask)
    );

    printf(
            "\tgw address %"U16_F".%"U16_F".%"U16_F".%"U16_F"\n",
            ip4_addr1_16(&netif->gw),    
            ip4_addr2_16(&netif->gw),
            ip4_addr3_16(&netif->gw),
            ip4_addr4_16(&netif->gw)
    );

    printf(
            "\tdns1 address %"U16_F".%"U16_F".%"U16_F".%"U16_F"\n",
            ip4_addr1_16(&dns1),    
            ip4_addr2_16(&dns1),
            ip4_addr3_16(&dns1),
            ip4_addr4_16(&dns1)
    );

    printf(
            "\tdns2 address %"U16_F".%"U16_F".%"U16_F".%"U16_F"\n",
            ip4_addr1_16(&dns2),    
            ip4_addr2_16(&dns2),
            ip4_addr3_16(&dns2),
            ip4_addr4_16(&dns2)
    );
}

const char *platform_net_error(int code) {
    return netStrError(code);
}

int platform_net_start(const char *interface) {
    return netStart(interface);
}

int platform_net_stop(const char *interface) {
    return netStop(interface);
}

int platform_net_sntp_start() {
    return netSntpStart();
}

void platform_net_setup_gprs(const char *apn, const char *pin) {
    netSetupGprs(apn, pin);
}

#endif