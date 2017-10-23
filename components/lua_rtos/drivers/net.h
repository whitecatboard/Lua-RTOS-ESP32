/*
 * Lua RTOS, network manager
 *
 * Copyright (C) 2015 - 2017
 * IBEROXARXA SERVICIOS INTEGRALES, S.L.
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
 * whatsoever resulting from loss of use, data or profits, whether
 * in an action of contract, negligence or other tortious action,
 * arising out of or in connection with the use or performance of
 * this software.
 */

#include "sdkconfig.h"

#if CONFIG_LUA_RTOS_LUA_USE_NET

#ifndef NET_H_
#define NET_H_

#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "lwip/ip_addr.h"

#include <sys/driver.h>
#include <sys/status.h>

#define MAX_NET_EVENT_CALLBACKS 3

#define evWIFI_SCAN_END 	       	 ( 1 << 0 )
#define evWIFI_CONNECTED 	       	 ( 1 << 1 )
#define evWIFI_CANT_CONNECT          ( 1 << 2 )
#define evSPI_ETH_CONNECTED 	     ( 1 << 3 )
#define evSPI_ETH_CANT_CONNECT       ( 1 << 4 )

typedef struct {
    ip4_addr_t ip;
    ip4_addr_t netmask;
    ip4_addr_t gw;
    uint8_t    mac[6];
    ip6_addr_t ip6;
} ifconfig_t;

// This macro gets a reference for this driver into drivers array
#define NET_DRIVER driver_get_by_name("net")

// NET errors
#define NET_ERR_NOT_AVAILABLE              (DRIVER_EXCEPTION_BASE(NET_DRIVER_ID) |  0)
#define NET_ERR_INVALID_IP                 (DRIVER_EXCEPTION_BASE(NET_DRIVER_ID) |  1)
#define NET_ERR_NO_MORE_CALLBACKS          (DRIVER_EXCEPTION_BASE(NET_DRIVER_ID) |  2)
#define NET_ERR_NO_CALLBACK_NOT_FOUND      (DRIVER_EXCEPTION_BASE(NET_DRIVER_ID) |  3)
#define NET_ERR_NAME_CANNOT_BE_RESOLVED	   (DRIVER_EXCEPTION_BASE(NET_DRIVER_ID) |  4)
#define NET_ERR_NAME_CANNOT_CREATE_SOCKET  (DRIVER_EXCEPTION_BASE(NET_DRIVER_ID) |  5)
#define NET_ERR_NAME_CANNOT_SETUP_SOCKET   (DRIVER_EXCEPTION_BASE(NET_DRIVER_ID) |  6)

extern const int net_errors;
extern const int net_error_map;

typedef void (*net_event_register_callback_t)(system_event_t *event);

/**
 * @brief Init the network driver. It must be called before use any network function.
 *        The function inits the tcp/ip adapter, the main network event loop, and some
 *        internal structures required by Lua RTOS.
 *
 * @return
 *     - NULL success
 *     - Pointer to driver_error_t if some error occurs. Reserved for future use.
 */
driver_error_t *net_init();

/**
 * @brief Checks the network connectivity. We consider that network is available if
 *        one of the network interface are init and up.
 *
 * @return
 *     - NULL success, the network is available
 *     - Pointer to driver_error_t if some error occurs.
 *
 *     	 NET_ERR_NOT_AVAILABLE
 */
driver_error_t *net_check_connectivity();

/**
 * @brief Lookup for a hostname, and get the IP, doing a DNS search.
 *
 * @param name The hostname.
 * @param name The port number.
 * @param address A pointer to a sockaddr_in structure where put the IP address.
 *
 * @return
 *     - NULL success, the host name where find by the DNS
 *     - Pointer to driver_error_t if some error occurs.
 *
 *     	 NET_ERR_NOT_AVAILABLE
 */
driver_error_t *net_lookup(const char *name, int port, struct sockaddr_in *address);

/**
 * @brief Register a function callback in the network event loop. When an event is
 *        received in the event loop the net driver executes the default treatment
 *        for the event, and at the end call to the registered callbacks.
 *
 * @param name A pointer to the callback function.
 *
 * @return
 *     - NULL success
 *     - Pointer to driver_error_t if some error occurs.
 *
 *     	 NET_ERR_NO_MORE_CALLBACKS
 */
driver_error_t *net_event_register_callback(net_event_register_callback_t func);

/**
 * @brief Unregister a function callback previously registered with the
 *        net_event_register_callback function.
 *
 * @param name A pointer to the callback function.
 *
 * @return
 *     - NULL success
 *     - Pointer to driver_error_t if some error occurs.
 *
 *     	 NET_ERR_NO_MORE_CALLBACKS
 */
driver_error_t *net_event_unregister_callback(net_event_register_callback_t func);

driver_error_t *net_ping(const char *name, int count, int interval, int size, int timeout);

driver_error_t *net_reconnect();

#endif

#endif
