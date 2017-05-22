/*
 * Lua RTOS, SPI ethernet driver
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

#if CONFIG_SPI_ETHERNET

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_system.h"
#include "tcpip_adapter.h"

#include "lwip/dns.h"
#include "lwip/ip_addr.h"

#include <string.h>
#include <stdlib.h>

#include <sys/status.h>
#include <sys/panic.h>
#include <sys/syslog.h>

#include <drivers/spi_eth.h>

DRIVER_REGISTER_ERROR(SPI_ETH, spi_eth, CannotSetup, "can't setup", SPI_ETH_ERR_CANT_INIT);
DRIVER_REGISTER_ERROR(SPI_ETH, spi_eth, NotSetup, "ethernet is not setup", SPI_ETH_ERR_NOT_INIT);
DRIVER_REGISTER_ERROR(SPI_ETH, spi_eth, NotStarted, "ethernet is not started", SPI_ETH_ERR_NOT_START);
DRIVER_REGISTER_ERROR(SPI_ETH, spi_eth, CannotConnect, "can't connect, check cable", SPI_ETH_ERR_CANT_CONNECT);

extern EventGroupHandle_t netEvent;

/*
 * Operation functions
 */
driver_error_t *spi_eth_setup(uint32_t ip, uint32_t mask, uint32_t gw, uint32_t dns1, uint32_t dns2) {
	driver_error_t *error;
	tcpip_adapter_ip_info_t ip_info;
	ip_addr_t dns;
	ip_addr_t *dns_p = &dns;

	// Init network, if needed
	if (!status_get(STATUS_SPI_ETH_SETUP)) {
		if ((error = net_init())) {
			return error;
		}

		status_set(STATUS_SPI_ETH_SETUP);
	}

	// Set ip / mask / gw, if present
	if (ip && mask && gw) {
		ip_info.ip.addr = ip;
		ip_info.netmask.addr = mask;
		ip_info.gw.addr = gw;

		tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_SPI_ETH, &ip_info);

		// If present, set dns1, else set to 8.8.8.8
		if (!dns1) dns1 = 134744072;
		ip_addr_set_ip4_u32(dns_p, dns1);

		dns_setserver(0, (const ip_addr_t *)&dns);

		// If present, set dns2, else set to 8.8.4.4
		if (!dns2) dns2 = 67373064;
		ip_addr_set_ip4_u32(dns_p, dns2);

		dns_setserver(1, (const ip_addr_t *)&dns);
	}

	return NULL;
}

driver_error_t *spi_eth_start() {
	if (!status_get(STATUS_SPI_ETH_SETUP)) {
		return driver_operation_error(SPI_ETH_DRIVER, SPI_ETH_ERR_NOT_INIT, NULL);
	}

	if (!status_get(STATUS_SPI_ETH_STARTED)) {
		// Start SPI ethetnet
		system_event_t evt;
	    evt.event_id = SYSTEM_EVENT_SPI_ETH_START;
	    esp_event_send(&evt);

	    // Wait for connect
	    EventBits_t uxBits = xEventGroupWaitBits(netEvent, evSPI_ETH_CONNECTED | evSPI_ETH_CANT_CONNECT, pdTRUE, pdFALSE, 4000 / portTICK_PERIOD_MS);
	    if (uxBits & (evSPI_ETH_CONNECTED)) {
		    status_set(STATUS_SPI_ETH_STARTED);
	    } else if (uxBits & (evSPI_ETH_CANT_CONNECT)) {
	    	status_clear(STATUS_SPI_ETH_STARTED);
	    	return driver_operation_error(SPI_ETH_DRIVER, SPI_ETH_ERR_CANT_CONNECT, NULL);
	    } else {
	    	status_clear(STATUS_SPI_ETH_STARTED);
	    	return driver_operation_error(SPI_ETH_DRIVER, SPI_ETH_ERR_CANT_CONNECT, NULL);
	    }
	}

	return NULL;
}

driver_error_t *spi_eth_stop() {
	if (!status_get(STATUS_SPI_ETH_SETUP)) {
		return driver_operation_error(SPI_ETH_DRIVER, SPI_ETH_ERR_NOT_INIT, NULL);
	}

	if (status_get(STATUS_SPI_ETH_STARTED)) {
    	status_clear(STATUS_SPI_ETH_STARTED);
	}

	return NULL;
}

driver_error_t *spi_eth_stat(ifconfig_t *info) {
	tcpip_adapter_ip_info_t esp_info;
	uint8_t mac[6] = {0,0,0,0,0,0};

	// Get IP info
	tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_SPI_ETH, &esp_info);

	// Get MAC info
	tcpip_adapter_get_mac(TCPIP_ADAPTER_IF_SPI_ETH, mac);

	// Copy info
	info->gw = esp_info.gw;
	info->ip = esp_info.ip;
	info->netmask = esp_info.netmask;

	memcpy(info->mac, mac, sizeof(mac));

	return NULL;
}

DRIVER_REGISTER(SPI_ETH,spi_eth,NULL,NULL,NULL);

#endif
