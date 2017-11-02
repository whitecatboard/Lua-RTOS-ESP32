/*
 * Lua RTOS, RMII Ethernet driver
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

#if CONFIG_LUA_RTOS_LUA_USE_NET && CONFIG_LUA_RTOS_ETH_HW_TYPE_RMII

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_system.h"
#include "tcpip_adapter.h"
#include "esp_eth.h"

#include "eth_phy/phy_lan8720.h"

#include "lwip/dns.h"
#include "lwip/ip_addr.h"

#include <string.h>

#include <sys/status.h>

#include <drivers/eth.h>
#include <drivers/net.h>
#include <drivers/gpio.h>

#ifdef CONFIG_PHY_LAN8720
#include "eth_phy/phy_lan8720.h"
#define DEFAULT_ETHERNET_PHY_CONFIG phy_lan8720_default_ethernet_config
#endif
#ifdef CONFIG_PHY_TLK110
#include "eth_phy/phy_tlk110.h"
#define DEFAULT_ETHERNET_PHY_CONFIG phy_tlk110_default_ethernet_config
#endif

// Register drivers and errors
DRIVER_REGISTER_BEGIN(ETH,eth,NULL,NULL,NULL);
	DRIVER_REGISTER_ERROR(ETH, eth, CannotSetup, "can't setup", ETH_ERR_CANT_INIT);
	DRIVER_REGISTER_ERROR(ETH, eth, NotSetup, "ethernet is not setup", ETH_ERR_NOT_INIT);
	DRIVER_REGISTER_ERROR(ETH, eth, NotStarted, "ethernet is not started", ETH_ERR_NOT_START);
	DRIVER_REGISTER_ERROR(ETH, eth, CannotConnect, "can't connect check cable", ETH_ERR_CANT_CONNECT);
DRIVER_REGISTER_END(ETH,eth,NULL,NULL,NULL);

extern EventGroupHandle_t netEvent;

static void eth_gpio_config_rmii(void) {
    // RMII data pins are fixed:
    // TXD0 = GPIO19
    // TXD1 = GPIO22
    // TX_EN = GPIO21
    // RXD0 = GPIO25
    // RXD1 = GPIO26
    // CLK == GPIO0
    phy_rmii_configure_data_interface_pins();

    // MDC is GPIO 23, MDIO is GPIO 18
    phy_rmii_smi_configure_pins(CONFIG_PHY_SMI_MDC_PIN, CONFIG_PHY_SMI_MDIO_PIN);
}

static void phy_device_power_enable_via_gpio(bool enable)
{
    if (!enable) {
        /* Do the PHY-specific power_enable(false) function before powering down */
    	DEFAULT_ETHERNET_PHY_CONFIG.phy_power_enable(false);
    }

    gpio_pin_output(CONFIG_PHY_POWER_PIN);
    if(enable) {
    	gpio_pin_set(CONFIG_PHY_POWER_PIN);
    } else {
    	gpio_pin_clr(CONFIG_PHY_POWER_PIN);
    }

    // Allow the power up/down to take effect, min 300us
    vTaskDelay(1);

    if (enable) {
        /* Run the PHY-specific power on operations now the PHY has power */
    	DEFAULT_ETHERNET_PHY_CONFIG.phy_power_enable(true);
    }
}

/*
 * Operation functions
 */
driver_error_t *eth_setup(uint32_t ip, uint32_t mask, uint32_t gw, uint32_t dns1, uint32_t dns2) {
	driver_unit_lock_error_t *lock_error = NULL;
	driver_error_t *error;
	tcpip_adapter_ip_info_t ip_info;
	ip_addr_t dns;
	ip_addr_t *dns_p = &dns;

	// Init network, if needed
	if (!status_get(STATUS_ETH_SETUP)) {
		if ((error = net_init())) {
			return error;
		}

		status_set(STATUS_ETH_SETUP);
	} else {
		return NULL;
	}

	// Lock resources
    if ((lock_error = driver_lock(ETH_DRIVER, 0, GPIO_DRIVER, 19, DRIVER_ALL_FLAGS, "TXD0"))) {
    	return driver_lock_error(ETH_DRIVER, lock_error);
    }

    if ((lock_error = driver_lock(ETH_DRIVER, 0, GPIO_DRIVER, 22, DRIVER_ALL_FLAGS, "TXD1"))) {
    	return driver_lock_error(ETH_DRIVER, lock_error);
    }

    if ((lock_error = driver_lock(ETH_DRIVER, 0, GPIO_DRIVER, 21, DRIVER_ALL_FLAGS, "TX_EN"))) {
    	return driver_lock_error(ETH_DRIVER, lock_error);
    }

    if ((lock_error = driver_lock(ETH_DRIVER, 0, GPIO_DRIVER, 25, DRIVER_ALL_FLAGS, "RXD0"))) {
    	return driver_lock_error(ETH_DRIVER, lock_error);
    }

    if ((lock_error = driver_lock(ETH_DRIVER, 0, GPIO_DRIVER, 26, DRIVER_ALL_FLAGS, "RXD1"))) {
    	return driver_lock_error(ETH_DRIVER, lock_error);
    }

    if ((lock_error = driver_lock(ETH_DRIVER, 0, GPIO_DRIVER, 0, DRIVER_ALL_FLAGS, "CLK"))) {
    	return driver_lock_error(ETH_DRIVER, lock_error);
    }

    if ((lock_error = driver_lock(ETH_DRIVER, 0, GPIO_DRIVER, CONFIG_PHY_SMI_MDC_PIN, DRIVER_ALL_FLAGS, "MDC"))) {
    	return driver_lock_error(ETH_DRIVER, lock_error);
    }

    if ((lock_error = driver_lock(ETH_DRIVER, 0, GPIO_DRIVER, CONFIG_PHY_SMI_MDIO_PIN, DRIVER_ALL_FLAGS, "MDIO"))) {
    	return driver_lock_error(ETH_DRIVER, lock_error);
    }

    if ((lock_error = driver_lock(ETH_DRIVER, 0, GPIO_DRIVER, CONFIG_PHY_POWER_PIN, DRIVER_ALL_FLAGS, "POWER"))) {
    	return driver_lock_error(ETH_DRIVER, lock_error);
    }

    esp_err_t ret = ESP_OK;

	eth_config_t config = DEFAULT_ETHERNET_PHY_CONFIG;

    // Set the PHY address in the example configuration */
    config.phy_addr = CONFIG_PHY_ADDRESS;
    config.gpio_config = eth_gpio_config_rmii;
    config.tcpip_input = tcpip_adapter_eth_input;

    config.phy_power_enable = phy_device_power_enable_via_gpio;

    ret = esp_eth_init(&config);
    if (ret != ESP_OK) {
    	return NULL;
    }

	// Set ip / mask / gw, if present
	if (ip && mask && gw) {
		ip_info.ip.addr = ip;
		ip_info.netmask.addr = mask;
		ip_info.gw.addr = gw;

		tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_ETH);
		tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_ETH, &ip_info);

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

driver_error_t *eth_start(uint8_t silent) {
	if (!status_get(STATUS_ETH_SETUP)) {
		return driver_error(ETH_DRIVER, ETH_ERR_NOT_INIT, NULL);
	}

	if (!status_get(STATUS_ETH_STARTED)) {
		esp_eth_enable();

		if (!silent) {
		    // Wait for connect
		    EventBits_t uxBits = xEventGroupWaitBits(netEvent, evETH_CONNECTED | evETH_CANT_CONNECT, pdTRUE, pdFALSE, 10000 / portTICK_PERIOD_MS);
		    if (uxBits & (evETH_CONNECTED)) {
		    } else if (uxBits & (evETH_CANT_CONNECT)) {
		    	return driver_error(ETH_DRIVER, ETH_ERR_CANT_CONNECT, NULL);
		    } else {
		    	return driver_error(ETH_DRIVER, ETH_ERR_CANT_CONNECT, NULL);
		    }
		}
	}

	return NULL;
}

driver_error_t *eth_stop() {
	if (!status_get(STATUS_ETH_SETUP)) {
		return driver_error(ETH_DRIVER, ETH_ERR_NOT_INIT, NULL);
	}

	if (status_get(STATUS_ETH_STARTED)) {
    	status_clear(STATUS_ETH_STARTED);
	}

	esp_eth_disable();

	return NULL;
}

driver_error_t *eth_stat(ifconfig_t *info) {
	tcpip_adapter_ip_info_t esp_info;
	uint8_t mac[6] = {0,0,0,0,0,0};

	// Get IP info
	tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_ETH, &esp_info);

	// Get MAC info
	tcpip_adapter_get_mac(TCPIP_ADAPTER_IF_ETH, mac);

	// Copy info
	info->gw = esp_info.gw;
	info->ip = esp_info.ip;
	info->netmask = esp_info.netmask;

	memcpy(info->mac, mac, sizeof(mac));

	return NULL;
}

#endif
