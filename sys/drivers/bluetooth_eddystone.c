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
 * Lua RTOS, EDDYSTONE BEACON SERVICE
 *
 */

#include "sdkconfig.h"

#if CONFIG_BT_ENABLED

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "bluetooth.h"

#include <stdint.h>
#include <string.h>
#include <math.h>

#include <sys/list.h>

// Is init?
static uint8_t is_init = 0;

// Beacon list
static struct list beacons;

/*
 * Helper functions
 */

static inline float distance(int rssi, int tx_power) {
	return (float)pow((double)10.0,  ((double)tx_power - (float)rssi) / (float)20.0) / (float)100.0;
}

static inline uint16_t little_endian_read_16(const uint8_t *buffer, uint8_t pos) {
    return ((uint16_t)buffer[pos]) | (((uint16_t)buffer[(pos)+1]) << 8);
}

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
	// Sanity checks
	if ((tx_power < -100) || (tx_power > 20)) {
		return driver_error(BT_DRIVER, BT_ERR_INVALID_TX_POWER, NULL);
	}

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

void bt_eddystone_decode(uint8_t *data, uint8_t datalen, bt_adv_frame_t *frame) {
	uint8_t pos;
	uint8_t ad_len;
	uint8_t ad_type;
	uint16_t ad_uuid = 0;
	uint16_t ad_sdt = 0;

	pos = 0;
	while (pos < datalen) {
		ad_len  = data[pos++];
		ad_type = data[pos++];

		if (ad_type == 0x01) {
			// Flags
			frame->flags = data[pos++];
		} else if (ad_type == 0x03) {
			// Complete List of 16-bit Service Class UUIDs
			ad_uuid = little_endian_read_16(data, pos);
			pos += 2;
		} else if (ad_type == 0x16) {
			// Service Data data type value
			ad_sdt = little_endian_read_16(data, pos);
			pos += 2;
			break;
		} else {
			pos += ad_len;
		}
	}

	if ((ad_uuid == 0xfeaa) && (ad_sdt == 0xfeaa)) {
		uint8_t type = data[pos++];
		int8_t  tx_power = data[pos++];

		if (type == 0x00) {
			//UID
			memcpy(frame->data.eddystone_uid.namespace, &data[pos], sizeof(frame->data.eddystone_uid.namespace));
			pos += sizeof(frame->data.eddystone_uid.namespace);

			memcpy(frame->data.eddystone_uid.instance, &data[pos], sizeof(frame->data.eddystone_uid.instance));
			pos += sizeof(frame->data.eddystone_uid.instance);

			if ((data[pos] != 0x00) || (data[pos +1 ] != 0x00)) {
				frame->frame_type = BTAdvUnknown;
				return;
			}

			frame->frame_type = BTAdvEddystoneUID;
			frame->data.eddystone_uid.tx_power = tx_power;
			frame->data.eddystone_uid.distance = distance(frame->rssi, tx_power);

			return;
		} else if (type == 0x10) {
			// URL
			// Decode URL
			int i;
			int len = ad_len - 6;
			char c[2] = {0x00, 0x00};

			memset(frame->data.eddystone_url.url, 0, sizeof(frame->data.eddystone_url.url));

			switch (data[pos]) {
				case HTTP_WWW:  strcat((char *)frame->data.eddystone_url.url, "http://www.");break;
				case HTTPS_WWW: strcat((char *)frame->data.eddystone_url.url, "https://www.");break;
				case HTTP:      strcat((char *)frame->data.eddystone_url.url, "http://");break;
				case HTTPS:     strcat((char *)frame->data.eddystone_url.url, "https://");break;
			}

			for(i=0;i<len;i++) {
				switch (data[++pos]) {
					case DOT_COM_P:  strcat((char *)frame->data.eddystone_url.url, ".com/") ;break;
					case DOT_ORG_P:  strcat((char *)frame->data.eddystone_url.url, ".org/") ;break;
					case DOT_EDU_P:  strcat((char *)frame->data.eddystone_url.url, ".edu/") ;break;
					case DOT_NET_P:  strcat((char *)frame->data.eddystone_url.url, ".net/") ;break;
					case DOT_INFO_P: strcat((char *)frame->data.eddystone_url.url, ".info/");break;
					case DOT_BIZ_P:  strcat((char *)frame->data.eddystone_url.url, ".biz/") ;break;
					case DOT_GOV_P:  strcat((char *)frame->data.eddystone_url.url, ".gov/") ;break;
					case DOT_COM_S:  strcat((char *)frame->data.eddystone_url.url, ".com")  ;break;
					case DOT_ORG_S:  strcat((char *)frame->data.eddystone_url.url, ".org")  ;break;
					case DOT_EDU_S:  strcat((char *)frame->data.eddystone_url.url, ".edu")  ;break;
					case DOT_NET_S:  strcat((char *)frame->data.eddystone_url.url, ".net")  ;break;
					case DOT_INFO_S: strcat((char *)frame->data.eddystone_url.url, ".info") ;break;
					case DOT_BIZ_S:  strcat((char *)frame->data.eddystone_url.url, ".biz")  ;break;
					case DOT_GOV_S:  strcat((char *)frame->data.eddystone_url.url, ".gov")  ;break;

					default:
						c[0] = data[pos];
						strcat((char *)frame->data.eddystone_url.url, c);
				}
			}

			frame->frame_type = BTAdvEddystoneURL;
			frame->data.eddystone_url.tx_power = tx_power;
			frame->data.eddystone_uid.distance = distance(frame->rssi, tx_power);

			return;
		}
	}

	frame->frame_type = BTAdvUnknown;
}

#endif
