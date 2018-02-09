/*
 * Copyright (C) 2015 - 2018, IBEROXARXA SERVICIOS INTEGRALES, S.L.
 * Copyright (C) 2015 - 2018, Jaume Olivé Petrus (jolive@whitecatboard.org)
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the <organization> nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *     * The WHITECAT logotype cannot be changed, you can remove it, but you
 *       cannot change it in any way. The WHITECAT logotype is:
 *
 *          /\       /\
 *         /  \_____/  \
 *        /_____________\
 *        W H I T E C A T
 *
 *     * Redistributions in binary form must retain all copyright notices printed
 *       to any local or remote output device. This include any reference to
 *       Lua RTOS, whitecatboard.org, Lua, and other copyright notices that may
 *       appear in the future.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Lua RTOS, spi ethernet driver
 *
 */

#include "sdkconfig.h"

#if CONFIG_LUA_RTOS_LUA_USE_NET && CONFIG_LUA_RTOS_ETH_HW_TYPE_SPI

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

// Register drivers and errors
DRIVER_REGISTER_BEGIN(SPI_ETH,spi_eth,NULL,NULL,NULL);
	DRIVER_REGISTER_ERROR(SPI_ETH, spi_eth, CannotSetup, "can't setup", SPI_ETH_ERR_CANT_INIT);
	DRIVER_REGISTER_ERROR(SPI_ETH, spi_eth, NotSetup, "ethernet is not setup", SPI_ETH_ERR_NOT_INIT);
	DRIVER_REGISTER_ERROR(SPI_ETH, spi_eth, NotStarted, "ethernet is not started", SPI_ETH_ERR_NOT_START);
	DRIVER_REGISTER_ERROR(SPI_ETH, spi_eth, CannotConnect, "can't connect check cable", SPI_ETH_ERR_CANT_CONNECT);
DRIVER_REGISTER_END(SPI_ETH,spi_eth,NULL,NULL,NULL);

extern EventGroupHandle_t netEvent;

/*
 * Operation functions
 */
driver_error_t *spi_eth_setup(uint32_t ip, uint32_t mask, uint32_t gw, uint32_t dns1, uint32_t dns2) {
	driver_error_t *error;
	tcpip_adapter_ip_info_t ip_info;
	ip_addr_t dns;
	ip_addr_t *dns_p = &dns;

	esp_event_set_default_spi_eth_handlers();

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

		tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_SPI_ETH);
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
		return driver_error(SPI_ETH_DRIVER, SPI_ETH_ERR_NOT_INIT, NULL);
	}

	if (!status_get(STATUS_SPI_ETH_STARTED)) {
		// Start SPI ethetnet
		system_event_t evt;
	    evt.event_id = SYSTEM_EVENT_SPI_ETH_START;
	    esp_event_send(&evt);
	}

	return NULL;
}

driver_error_t *spi_eth_stop() {
	if (!status_get(STATUS_SPI_ETH_SETUP)) {
		return driver_error(SPI_ETH_DRIVER, SPI_ETH_ERR_NOT_INIT, NULL);
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

#endif
