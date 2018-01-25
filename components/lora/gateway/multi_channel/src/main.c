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
 * Lua RTOS, LoRa WAN multichannel main program
 *
 */

#include <string.h>
#include <pthread.h>

#include <drivers/net.h>
#include <drivers/eth.h>
#include <drivers/spi_eth.h>
#include <drivers/wifi.h>

#include <sys/syslog.h>
#include <sys/driver.h>
#include <sys/delay.h>
#include <sys/path.h>
#include <sys/param.h>

int lora_pkt_fwd(void);

static int gen_local_conf() {
	uint8_t mac[6] = {0,0,0,0,0,0};
	uint8_t have_mac = 0;
	ifconfig_t info;

	// Get the MAC address to build the gateway EUI
#if CONFIG_LUA_RTOS_LUA_USE_NET && CONFIG_LUA_RTOS_ETH_HW_TYPE_RMII
	eth_stat(&info);
	if (memcmp(info.mac, mac, 6) != 0) {
		memcpy(mac, info.mac, 6);
		have_mac = 1;
	}
#else
#if CONFIG_LUA_RTOS_LUA_USE_NET && CONFIG_LUA_RTOS_ETH_HW_TYPE_SPI
	spi_eth_stat(&info);
	if (memcmp(info.mac, mac, 6) != 0) {
		memcpy(mac, info.mac, 6);
		have_mac = 1;
	}
#endif
#endif

	if (!have_mac) {
		wifi_stat(&info);
		if (memcmp(info.mac, mac, 6) != 0) {
			memcpy(mac, info.mac, 6);
			have_mac = 1;
		}
	}

	if (!have_mac) {
		syslog(LOG_INFO, "INFO: can't get gateway ID. Check that you have configured a network connection.");

		return -1;
	}

	FILE *conf = NULL;

	conf = fopen("/etc/lora/local_conf.json", "a+");
	if (conf) {
		fprintf(conf, "{\r\n");
		fprintf(conf, "\t\"gateway_conf\": {\r\n");
		fprintf(conf, "\t\t\"gateway_ID\": \"%02X%02X%02XFFFE%02X%02X%02X\", \r\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
		fprintf(conf, "\t\t\"ref_latitude\": \"41.359875\", \r\n");
		fprintf(conf, "\t\t\"ref_longitude\": \"2.061760\", \r\n");
		fprintf(conf, "\t\t\"ref_altitude\": \"40\", \r\n");
		fprintf(conf, "\t\t\"contact_email\": \"your@mail\", \r\n");
		fprintf(conf, "\t\t\"description\": \"WHITECAT ESP32 LORA GW\" \r\n");
		fprintf(conf, "\t}\r\n");
		fprintf(conf, "}\r\n");

		fclose(conf);
	}

	return 0;
}

static void *lora_gw(void *arg) {
	driver_error_t *error;

	// Create lora directory structure if not exist
	mkpath("/etc/lora");

	// Wait for network
	while ((error = net_check_connectivity())) {
		free(error);
		delay(100);
	}

	// Create local configuration if not exist
	if (mkfile("/etc/lora/local_conf.json") == 0) {
		if (gen_local_conf() < 0) {
			return NULL;
		}
	}

	lora_pkt_fwd();

	return NULL;
}

void lora_gw_start() {
	pthread_t thread;
	pthread_attr_t attr;

	pthread_attr_init(&attr);

	// Set stack size
	pthread_attr_setstacksize(&attr, CONFIG_LUA_RTOS_LUA_STACK_SIZE);

	if (!pthread_create(&thread, &attr, lora_gw, NULL)) {
		return;
	}

	pthread_setname_np(thread, "lora");
}
