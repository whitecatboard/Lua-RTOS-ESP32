/*
 * Copyright (C) 2015 - 2020, IBEROXARXA SERVICIOS INTEGRALES, S.L.
 * Copyright (C) 2015 - 2020, Jaume Olivé Petrus (jolive@whitecatboard.org)
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
 * Lua RTOS, RMII ethernet driver
 *
 */

#include "sdkconfig.h"

#if CONFIG_LUA_RTOS_LUA_USE_NET && CONFIG_LUA_RTOS_ETH_HW_TYPE_RMII

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include "esp_event.h"
#include "esp_system.h"
#include "esp_netif.h"
#include "esp_eth.h"

#include "lwip/dns.h"
#include "lwip/ip_addr.h"

#include <string.h>

#include <sys/status.h>

#include <drivers/eth.h>
#include <drivers/net.h>
#include <drivers/gpio.h>
#include <sys/panic.h>

// Register drivers and errors
DRIVER_REGISTER_BEGIN(ETH,eth,0,NULL,NULL);
    DRIVER_REGISTER_ERROR(ETH, eth, CannotSetup, "can't setup", ETH_ERR_CANT_INIT);
    DRIVER_REGISTER_ERROR(ETH, eth, NotSetup, "ethernet is not setup", ETH_ERR_NOT_INIT);
    DRIVER_REGISTER_ERROR(ETH, eth, NotStarted, "ethernet is not started", ETH_ERR_NOT_START);
    DRIVER_REGISTER_ERROR(ETH, eth, CannotConnect, "can't connect check cable", ETH_ERR_CANT_CONNECT);
    DRIVER_REGISTER_ERROR(ETH, eth, InvalidArg, "invalid argument", ETH_ERR_INVALID_ARGUMENT);
    DRIVER_REGISTER_ERROR(ETH, eth, NotEnoughtMemory, "not enough memory", ETH_ERR_ETH_NO_MEM);
DRIVER_REGISTER_END(ETH,eth,0,NULL,NULL);

extern EventGroupHandle_t netEvent;
extern net_event_register_callback_t net_event_callback[MAX_NET_EVENT_CALLBACKS];

static esp_eth_handle_t eth_handle = NULL;
static esp_netif_t *eth_netif = NULL;

driver_error_t *net_eth_check_error(esp_err_t error) {
    if (error == ESP_OK) return NULL;

    switch (error) {
        case ESP_FAIL:                 return driver_error(ETH_DRIVER, ETH_ERR_INVALID_ARGUMENT, NULL);
        case ESP_ERR_NO_MEM:           return driver_error(ETH_DRIVER, ETH_ERR_ETH_NO_MEM, NULL);
        case ESP_ERR_INVALID_ARG:      return driver_error(ETH_DRIVER, ETH_ERR_INVALID_ARGUMENT, NULL);

        case ESP_ERR_ESP_NETIF_DRIVER_ATTACH_FAILED: return driver_error(ETH_DRIVER, ETH_ERR_CANT_INIT, NULL);

        default: {
            char *buffer;

            buffer = malloc(40);
            if (!buffer) {
                panic("not enough memory");
            }

            snprintf(buffer, 40, "missing wifi error case %d", error);

            return driver_error(ETH_DRIVER, ETH_ERR_CANT_INIT, buffer);
        }
    }

    return NULL;
}

static void net_eth_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data) {
    EventBits_t bits = 0;

    // Exit if not for us4
    if (((esp_event_base_t)arg) != ETH_EVENT) {
		return;
	}

    switch (id) {
		case ETHERNET_EVENT_START:
			status_set(STATUS_ETH_STARTED, STATUS_ETH_CONNECTED | STATUS_ETH_HAS_IP);
			break;
		case ETHERNET_EVENT_STOP:
			status_set(0x00000000, STATUS_ETH_STARTED | STATUS_ETH_CONNECTED | STATUS_ETH_HAS_IP);
			break;
		case ETHERNET_EVENT_CONNECTED:
			status_set(STATUS_ETH_CONNECTED, STATUS_ETH_HAS_IP);
			// TO DO
			#if 0
			tcpip_adapter_create_ip6_linklocal(TCPIP_ADAPTER_IF_ETH);
			#endif
			break;
		case ETHERNET_EVENT_DISCONNECTED:
			status_set(0x00000000, STATUS_ETH_CONNECTED | STATUS_ETH_HAS_IP);
			bits |= evETH_CANT_CONNECT;
			break;
		default:
			break;
    }

    // Call to the registered callbacks
    for(int i=0; i < MAX_NET_EVENT_CALLBACKS; i++) {
        if (net_event_callback[i]) {
			net_event_callback[i](NetEventTypeEth, id);
        }
    }

    if (bits) {
        xEventGroupSetBits(netEvent, bits);
    }
}

static void net_eth_ip_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data) {
    EventBits_t bits = 0;

    // Exit if not for us4
    if (((esp_event_base_t)arg) != IP_EVENT) {
		return;
	}

    switch (id) {
    	case IP_EVENT_ETH_GOT_IP:
			status_set(STATUS_ETH_HAS_IP, 0x00000000);
			bits |= evETH_CONNECTED;
			break;

    	case IP_EVENT_ETH_LOST_IP:
			break;

    	case IP_EVENT_GOT_IP6:
			break;
    }

    // Call to the registered callbacks
    for(int i=0; i < MAX_NET_EVENT_CALLBACKS; i++) {
        if (net_event_callback[i]) {
			net_event_callback[i](NetEventTypeEthIp, id);
        }
    }

    if (bits) {
        xEventGroupSetBits(netEvent, bits);
    }
}

#if CONFIG_PHY_POWER_PIN >= 0
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
#endif

/*
 * Operation functions
 */
driver_error_t *eth_setup(uint32_t ip, uint32_t mask, uint32_t gw, uint32_t dns1, uint32_t dns2) {
#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
    driver_unit_lock_error_t *lock_error = NULL;
#endif
    driver_error_t *error;

    // Init network, if needed
    if (!status_get(STATUS_ETH_SETUP)) {
        if ((error = net_init())) {
            return error;
        }

        status_set(STATUS_ETH_SETUP, 0x00000000);
    } else {
        return NULL;
    }

    esp_event_handler_instance_register(ETH_EVENT, ESP_EVENT_ANY_ID, &net_eth_event_handler, (void *)ETH_EVENT, NULL);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &net_eth_ip_event_handler, (void *)IP_EVENT, NULL);

#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
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

#if CONFIG_PHY_POWER_PIN >= 0
    if ((lock_error = driver_lock(ETH_DRIVER, 0, GPIO_DRIVER, CONFIG_PHY_POWER_PIN, DRIVER_ALL_FLAGS, "POWER"))) {
        return driver_lock_error(ETH_DRIVER, lock_error);
    }
#endif
#endif

    // PHY configuration
    eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();

    phy_config.phy_addr = CONFIG_PHY_ADDRESS;
    phy_config.reset_gpio_num = CONFIG_PHY_POWER_PIN;

    // MAC configuration
    eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();

    // Specific MAC configuration
    eth_esp32_emac_config_t esp32_emac_config = ETH_ESP32_EMAC_DEFAULT_CONFIG();

    esp32_emac_config.smi_mdc_gpio_num = CONFIG_PHY_SMI_MDC_PIN;
    esp32_emac_config.smi_mdio_gpio_num = CONFIG_PHY_SMI_MDIO_PIN ;

    // Create MAC instance
    esp_eth_mac_t *mac = esp_eth_mac_new_esp32(&esp32_emac_config, &mac_config);
    if (!mac) {
    	return driver_error(ETH_DRIVER, ETH_ERR_ETH_NO_MEM ,NULL);
    }

    // Create PHY instance
#if CONFIG_IP101
    esp_eth_phy_t *phy = esp_eth_phy_new_ip101(&phy_config);
#elif CONFIG_PHY_RTL8201
    esp_eth_phy_t *phy = esp_eth_phy_new_rtl8201(&phy_config);
#elif CONFIG_PHY_LAN8720
    esp_eth_phy_t *phy = esp_eth_phy_new_lan87xx(&phy_config);
#elif CONFIG_PHY_DP83848
    esp_eth_phy_t *phy = esp_eth_phy_new_dp83848(&phy_config);
#elif CONFIG_PHY_KSZ80XX
    esp_eth_phy_t *phy = esp_eth_phy_new_ksz80xx(&phy_config);
#endif

    if (!phy) {
    	return driver_error(ETH_DRIVER, ETH_ERR_ETH_NO_MEM ,NULL);
    }

    // Init Ethernet driver to default and install it
    esp_eth_config_t config = ETH_DEFAULT_CONFIG(mac, phy);

    // Install driver
    if ((error = net_eth_check_error(esp_eth_driver_install(&config, &eth_handle)))) return error;

    // Create network interface
    esp_netif_config_t cfg = ESP_NETIF_DEFAULT_ETH();
    eth_netif = esp_netif_new(&cfg);
    if (!eth_netif) {
    	return driver_error(ETH_DRIVER, ETH_ERR_ETH_NO_MEM ,NULL);
    }

    // Attach network interface to TCP/IP stack
    if ((error = net_eth_check_error(esp_netif_attach(eth_netif, esp_eth_new_netif_glue(eth_handle))))) return error;

    // Set ip / mask / gw, if present
    if (ip && mask && gw) {
    	esp_netif_ip_info_t ip_info;
    	esp_netif_dns_info_t dns_info;

        ip_info.ip.addr = ip;
        ip_info.netmask.addr = mask;
        ip_info.gw.addr = gw;

        esp_netif_dhcpc_stop(eth_netif);
        esp_netif_set_ip_info(eth_netif, &ip_info);

        // If present, set dns1, else set to 8.8.8.8
        if (!dns1) dns1 = 134744072;

        dns_info.ip.u_addr.ip4.addr = dns1;
        dns_info.ip.type = IPADDR_TYPE_V4;

        esp_netif_set_dns_info(eth_netif, ESP_NETIF_DNS_MAIN, &dns_info);

        // If present, set dns2, else set to 8.8.4.4
        if (!dns2) dns2 = 67373064;

        dns_info.ip.u_addr.ip4.addr = dns2;
        dns_info.ip.type = IPADDR_TYPE_V4;

        esp_netif_set_dns_info(eth_netif, ESP_NETIF_DNS_BACKUP, &dns_info);
    }

    return NULL;
}

driver_error_t *eth_start(uint8_t async) {
    if (!async) {
        status_set(STATUS_ETH_SYNC, 0x00000000);
    } else {
        status_set(0x00000000, STATUS_ETH_SYNC);
    }

    if (!status_get(STATUS_ETH_SETUP)) {
        return driver_error(ETH_DRIVER, ETH_ERR_NOT_INIT, NULL);
    }

    if (!status_get(STATUS_ETH_STARTED)) {
    	esp_eth_start(eth_handle);

        if (!async) {
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
        status_set(0x00000000, STATUS_ETH_STARTED);
    }

    esp_eth_stop(eth_netif);

    return NULL;
}

driver_error_t *eth_stat(ifconfig_t *info) {
	esp_netif_ip_info_t ip_info = {0};
    ip6_addr_t adr = {0};
    uint8_t mac[6] = {0,0,0,0,0,0};

    // Get netif info
    if (status_get(STATUS_ETH_STARTED)) {
    	esp_netif_get_ip_info(eth_netif, &ip_info);
    }

    // Get netif MAC
    if (status_get(STATUS_ETH_STARTED)) {
    	esp_netif_get_mac(eth_netif, mac);
    }

    // Copy info
    info->gw.addr = ip_info.gw.addr;
    info->ip.addr = ip_info.ip.addr;
    info->netmask.addr = ip_info.netmask.addr;
    info->ip6 = adr;

    memcpy(info->mac, mac, sizeof(mac));

    return NULL;
}

#endif
