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
 * Lua RTOS, EDDYSTONE BEACON SERVICE
 *
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "sdkconfig.h"
#include "bluetooth.h"

#include <stdint.h>
#include <string.h>

#include <sys/list.h>

// Is init?
static uint8_t is_init = 0;

// Beacon list
static struct list beacons;

/*
 * Helper functions
 */
static void eddystone_task(void *args) {
	bt_eddystone_t *beacon;
	int index;

	for (;;) {
		index = lstfirst(&beacons);
		if (index == -1) {
			bt_adv_stop();
		}

		while (index >= 0) {
			if (lstget(&beacons, index, (void **) &beacon) != 0) {
				vTaskDelay(200 / portTICK_PERIOD_MS);
				continue;
			}

			if (!beacon->started) {
				bt_adv_stop();
			} else if (beacon->type == EddystoneUID) {
				uint8_t adv_data[31] = {
					0x02, // Length	Flags. CSS v5, Part A, 1.3
					0x01, // Flags data type value
					0x06, // Flags data GENERAL_DISC_MODE 0x02 | BR_EDR_NOT_SUPPORTED 0x04
					0x03, // Length	Complete list of 16-bit Service UUIDs. Ibid.1.1
					0x03, // Complete list of 16-bit Service UUIDs data type value
					0xaa, // 16-bit Eddystone UUID 0xAA LSB
					0xfe, // 16-bit Eddystone UUID 0xFE MSB
					0x17, // Length	Service Data. Ibid.1.11 23 bytes from this point
					0x16, // Service Data data type value
					0xaa, // 16-bit Eddystone UUID 0xAA LSB
					0xfe, // 16-bit Eddystone UUID 0xFE MSB
					0x00, // Frame Type Eddystone UUID Value = 0x00
					beacon->data.uid.tx_power, // Ranging Data	Calibrated Tx power at 0 m  -60dBm meter + 41dBm = -19dBm
					beacon->data.uid.name_space[0], // 10-byte Namespace uuid generated
					beacon->data.uid.name_space[1], // 10-byte Namespace uuid generated
					beacon->data.uid.name_space[2], // 10-byte Namespace uuid generated
					beacon->data.uid.name_space[3], // 10-byte Namespace uuid generated
					beacon->data.uid.name_space[4], // 10-byte Namespace uuid generated
					beacon->data.uid.name_space[5], // 10-byte Namespace uuid generated
					beacon->data.uid.name_space[6], // 10-byte Namespace uuid generated
					beacon->data.uid.name_space[7], // 10-byte Namespace uuid generated
					beacon->data.uid.name_space[8], // 10-byte Namespace uuid generated
					beacon->data.uid.name_space[9], // 10-byte Namespace uuid generated
					beacon->data.uid.instance[0], // 6-byte Instance MAC address
					beacon->data.uid.instance[1], // 6-byte Instance MAC address
					beacon->data.uid.instance[2], // 6-byte Instance MAC address
					beacon->data.uid.instance[3], // 6-byte Instance MAC address
					beacon->data.uid.instance[4], // 6-byte Instance MAC address
					beacon->data.uid.instance[5], // 6-byte Instance MAC address
					0x00, // RFU Reserved for future use, must be0x00
					0x00  // RFU Reserved for future use, must be0x00
				};

				bt_adv_start(beacon->advParams, adv_data, sizeof(adv_data));
			} else if (beacon->type == EddystoneURL) {
				uint8_t adv_data[31] = {
					0x02, // Length	Flags. CSS v5, Part A, 1.3
					0x01, // Flags data type value
					0x06, // Flags data GENERAL_DISC_MODE 0x02 | BR_EDR_NOT_SUPPORTED 0x04
					0x03, // Length of service list
					0x03, // Service list
					0xaa, // 16-bit Eddystone UUID 0xAA LSB
					0xfe, // 16-bit Eddystone UUID 0xFE MSB
					beacon->data.url.encoded_url_len + 6, // Length of service data
					0x16, // Service data
					0xaa, // 16-bit Eddystone UUID 0xAA LSB
					0xfe, // 16-bit Eddystone UUID 0xFE MSB
					0x10, // frame type Eddyston-URL
					beacon->data.url.tx_power, // tx power
					beacon->data.url.prefix, // URL Scheme
					beacon->data.url.encoded_url[0],
					beacon->data.url.encoded_url[1],
					beacon->data.url.encoded_url[2],
					beacon->data.url.encoded_url[3],
					beacon->data.url.encoded_url[4],
					beacon->data.url.encoded_url[5],
					beacon->data.url.encoded_url[6],
					beacon->data.url.encoded_url[7],
					beacon->data.url.encoded_url[8],
					beacon->data.url.encoded_url[9],
					beacon->data.url.encoded_url[10],
					beacon->data.url.encoded_url[11],
					beacon->data.url.encoded_url[12],
					beacon->data.url.encoded_url[13],
					beacon->data.url.encoded_url[14],
					beacon->data.url.encoded_url[15],
					beacon->data.url.encoded_url[16]
				};

				bt_adv_start(beacon->advParams, adv_data, sizeof(adv_data));
			}

			index = lstnext(&beacons, index);

			vTaskDelay(200 / portTICK_PERIOD_MS);
		}

		vTaskDelay(200 / portTICK_PERIOD_MS);
	}
}

static void init() {
	if (!is_init) {
		is_init = 1;

		bt_setup(BLE);

		lstinit(&beacons, 1, LIST_DEFAULT);

		// Create task
		BaseType_t ret = xTaskCreatePinnedToCore(&eddystone_task, "eddystone", 2048, NULL, 5, NULL, 0);
		assert(ret == pdPASS);
	}
}

/*
 * Operation functions
 */
driver_error_t *bt_add_eddystone_uid(
	bt_adress_t address,
	bt_eddystone_tx_power_t tx_power,
	bt_eddystone_namespace_t namespace,
	bt_eddystone_instance_t instance,
	int *beacon_h
) {
	init();

	// Allocate space for the beacon
	bt_eddystone_t *beacon;

	beacon = calloc(1, sizeof(bt_eddystone_t));
	if (!beacon) {
		return driver_error(BT_DRIVER, BT_ERR_NOT_ENOUGH_MEMORY, NULL);
	}

	// Beacon is stopped
	beacon->started = 0;

	// Beacon type is EddystoneUID
	beacon->type = EddystoneUID;

	// Set advertise parameters
	beacon->advParams.type = ADV_NONCONN_IND;
	beacon->advParams.own_address_type = OwnPublic;
	beacon->advParams.peer_address_type = PeerPublic;
	memcpy(beacon->advParams.peer_address, address, sizeof(bt_adress_t));

	beacon->advParams.interval_min = 250; // 160 msecs
	beacon->advParams.interval_max = 320; // 200 msecs
	beacon->advParams.chann_map = AllChann;
	beacon->advParams.filter_policy = ConnAllScanAll;

	// Set advertise data
	beacon->data.uid.tx_power = tx_power;
	memcpy(beacon->data.uid.name_space, namespace, sizeof(bt_eddystone_namespace_t));
	memcpy(beacon->data.uid.instance, instance, sizeof(bt_eddystone_instance_t));

	// Add beacon to beacon list
    int res = lstadd(&beacons, (void *)beacon, (int *)beacon_h);
    if (res) {
    	free(beacon);

    	return driver_error(BT_DRIVER, BT_ERR_NOT_ENOUGH_MEMORY, NULL);
    }

	return NULL;
}

driver_error_t *bt_add_eddystone_url(
	bt_adress_t address,
	bt_eddystone_tx_power_t tx_power,
	const char *url,
	int *beacon_h) {

	init();

	// Allocate space for the beacon
	bt_eddystone_t *beacon;

	beacon = calloc(1, sizeof(bt_eddystone_t));
	if (!beacon) {
		return driver_error(BT_DRIVER, BT_ERR_NOT_ENOUGH_MEMORY, NULL);
	}

	// Beacon is stopped
	beacon->started = 0;

	// Beacon type is EddystoneURL
	beacon->type = EddystoneURL;

	// Set advertise parameters
	beacon->advParams.type = ADV_NONCONN_IND;
	beacon->advParams.own_address_type = OwnPublic;
	beacon->advParams.peer_address_type = PeerPublic;
	memcpy(beacon->advParams.peer_address, address, sizeof(bt_adress_t));

	beacon->advParams.interval_min = 250; // 160 msecs
	beacon->advParams.interval_max = 320; // 200 msecs
	beacon->advParams.chann_map = AllChann;
	beacon->advParams.filter_policy = ConnAllScanAll;

	// Set advertise data
	beacon->data.url.tx_power = tx_power;

	// Code URL
	char *curl = (char *)url;
	if (strncmp("http://www.", curl, 11) == 0) {
		curl = curl + 11;
		beacon->data.url.prefix = HTTP_WWW;
	} else if (strncmp("https://www.", curl, 12) == 0) {
		curl = curl + 12;
		beacon->data.url.prefix = HTTPS_WWW;
	} else if (strncmp("http://", curl, 7) == 0) {
		curl = curl + 7;
		beacon->data.url.prefix = HTTP;
	} else if (strncmp("https://", curl, 8) == 0) {
		curl = curl + 8;
		beacon->data.url.prefix = HTTPS;
	}

	beacon->data.url.encoded_url_len = 0;
	uint8_t *encoded = (uint8_t *)&beacon->data.url.encoded_url[0];
	while (*curl) {
		if (strncmp(".com/", curl, 5) == 0) {
			curl = curl + 5;
			*encoded = DOT_COM_P;
		} else if (strncmp(".org/", curl, 5) == 0) {
			curl = curl + 5;
			*encoded = DOT_ORG_P;
		} else if (strncmp(".edu/", curl, 5) == 0) {
			curl = curl + 5;
			*encoded = DOT_EDU_P;
		} else if (strncmp(".net/", curl, 5) == 0) {
			curl = curl + 5;
			*encoded = DOT_NET_P;
		} else if (strncmp(".info/", curl, 5) == 0) {
			curl = curl + 5;
			*encoded = DOT_INFO_P;
		} else if (strncmp(".biz/", curl, 5) == 0) {
			curl = curl + 5;
			*encoded = DOT_BIZ_P;
		} else if (strncmp(".gov/", curl, 5) == 0) {
			curl = curl + 5;
			*encoded = DOT_GOV_P;
		} else if (strncmp(".com", curl, 4) == 0) {
			curl = curl + 4;
			*encoded = DOT_COM_S;
		} else if (strncmp(".org", curl, 4) == 0) {
			curl = curl + 4;
			*encoded = DOT_ORG_S;
		} else if (strncmp(".edu", curl, 4) == 0) {
			curl = curl + 4;
			*encoded = DOT_EDU_S;
		} else if (strncmp(".net", curl, 4) == 0) {
			curl = curl + 4;
			*encoded = DOT_NET_S;
		} else if (strncmp(".info", curl, 4) == 0) {
			curl = curl + 4;
			*encoded = DOT_INFO_S;
		} else if (strncmp(".biz", curl, 4) == 0) {
			curl = curl + 4;
			*encoded = DOT_BIZ_S;
		} else if (strncmp(".gov", curl, 4) == 0) {
			curl = curl + 4;
			*encoded = DOT_GOV_S;
		} else {
			*encoded = *curl;
			curl++;
		}
		encoded++;

		if (beacon->data.url.encoded_url_len + 1 < 17) {
			beacon->data.url.encoded_url_len++;
		} else {
			return driver_error(BT_DRIVER, BT_ERR_INVALID_ARGUMENT, "not enough space to encode url");
		}
	}

	// Add beacon to beacon list
    int res = lstadd(&beacons, (void *)beacon, (int *)beacon_h);
    if (res) {
    	free(beacon);

    	return driver_error(BT_DRIVER, BT_ERR_NOT_ENOUGH_MEMORY, NULL);
    }
    return NULL;
}

driver_error_t *bt_eddystone_remove(int beacon_h) {
	lstremove(&beacons, beacon_h, 1);

    return NULL;
}

driver_error_t *bt_eddystone_start(int beacon_h) {
	bt_eddystone_t *beacon;

    int res = lstget(&beacons, beacon_h, (void **)&beacon);
    if (res) {
    	return driver_error(BT_DRIVER, BT_ERR_INVALID_BEACON, NULL);
    }

    beacon->started = 1;

    return NULL;
}

driver_error_t *bt_eddystone_stop(int beacon_h) {
	bt_eddystone_t *beacon;

    int res = lstget(&beacons, beacon_h, (void **)&beacon);
    if (res) {
    	return driver_error(BT_DRIVER, BT_ERR_INVALID_BEACON, NULL);
    }

    beacon->started = 0;

    return NULL;
}

void bt_eddystone_decode(uint8_t *data, uint8_t len, bt_adv_decode_t *decoded) {
	uint8_t is_eddystone = 1;

	is_eddystone &= (data[0]  == 0x02); // Length	Flags. CSS v5, Part A, 1.3
	is_eddystone &= (data[1]  == 0x01); // Flags data type value
	is_eddystone &= (data[2]  == 0x06); // Flags data GENERAL_DISC_MODE 0x02 | BR_EDR_NOT_SUPPORTED 0x04
	is_eddystone &= (data[3]  == 0x03); // Length	Complete list of 16-bit Service UUIDs. Ibid.1.1
	is_eddystone &= (data[4]  == 0x03); // Complete list of 16-bit Service UUIDs data type value
	is_eddystone &= (data[5]  == 0xaa); // 16-bit Eddystone UUID 0xAA LSB
	is_eddystone &= (data[6]  == 0xfe); // 16-bit Eddystone UUID 0xFE MSB
	is_eddystone &= (data[7]  <= 23  ); // Length	Service Data. Ibid.1.11 from this point
	is_eddystone &= (data[8]  == 0x16); // Service Data data type value
	is_eddystone &= (data[9]  == 0xaa); // 16-bit Eddystone UUID 0xAA LSB
	is_eddystone &= (data[10] == 0xfe); // 16-bit Eddystone UUID 0xFE MSB

	if (is_eddystone) {
		if (data[11] == 0x00) {
			//UID
			if ((data[29] == 0x00) && (data[30] == 0x00)) {
				decoded->frame_type = BTAdvEddystoneUID;
				memcpy(decoded->data.eddystone_uid.namespace, &data[13], sizeof(decoded->data.eddystone_uid.namespace));
				memcpy(decoded->data.eddystone_uid.instance, &data[23], sizeof(decoded->data.eddystone_uid.instance));
			} else {
				decoded->frame_type = BTAdvUnknown;
				memset(decoded->data.eddystone_url.url, 0, sizeof(decoded->data.eddystone_url.url));
			}
		} else if (data[11] == 0x10) {
			//URL
			decoded->frame_type = BTAdvEddystoneURL;
		} else {
			decoded->frame_type = BTAdvUnknown;
		}
	} else {
		decoded->frame_type = BTAdvUnknown;
	}
}
