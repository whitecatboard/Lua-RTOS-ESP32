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
 * Lua RTOS, HCI API
 *
 */

#include "sdkconfig.h"

#if CONFIG_BT_ENABLED

#ifndef BT_HCI_H_
#define BT_HCI_H_

#include "bluetooth.h"

#include <stdint.h>

#include <sys/driver.h>

// Advertisement types
typedef enum {
	ADV_IND = 0, 			  ///< Connectable and scannable undirected advertising
	ADV_DIRECT_IND_HIGH = 1,  ///< Connectable high duty cycle directed advertising
	ADV_SCAN_IND = 2,		  ///< Scannable undirected advertising
	ADV_NONCONN_IND = 3,	  ///< Non connectable undirected advertising
	ADV_DIRECT_IND_LOW = 4,   ///< Connectable low duty cycle directed advertising
} bte_adv_type_t;

// Own address types
typedef enum {
	OwnPublic = 0,			///< Public Device Address
	OwnRandom = 1,			///< Random Device Address

	OwnPrivatePublic = 2,   ///< Controller generates Resolvable Private Address based
							///< on the local IRK from the resolving list. If the resolving
							///< list contains no matching entry, use the public address.

	OwnPrivateRandom = 3    ///< Controller generates Resolvable Private Address based on the
							///< local IRK from the resolving list. If the resolving list contains
							///< no matching entry, use the random address from LE_Set_Random_Addres
} bte_own_addr_type_t;

// Peer address types
typedef enum {
	PeerPublic = 0,			///< Public Device Address or Public Identity Address
	PeerRandom = 1,			///< Random Device Address or Random (static) Identity Address
} bte_peer_addr_type_t;

// Channel map types
typedef enum {
	Chann37  = 0b001,		///< Channel 37 shall be used
	Chann38  = 0b010,		///< Channel 38 shall be used
	Chann39  = 0b100,		///< Channel 39 shall be used
	AllChann = 0b111		///< All channels enabled
} bte_adv_channel_map_t;

typedef enum {
	ConnAllScanAll = 0,		///< Process scan and connection requests from all devices

	ConnAllScanWhite = 1,	///< Process connection requests from all devices and only scan requests
							///< from devices that are in the White List.

	ConnWhiteScanAll = 2,	///< Process scan requests from all devices and only connection requests
							///< from devices that are in the White List.

	ConnWhiteScanWhite = 3,	///< Process scan and connection requests only from devices in the White List.
} bte_adv_filter_policy_t;

typedef struct {
	uint16_t interval_min; ///< Minimum advertising interval for undirected and low duty cycle
						   ///< directed advertising. Expressed in 0.625 ms units.

	uint16_t interval_max; ///< Maximum advertising interval for undirected and low duty cycle
						   ///< directed advertising. Expressed in 0.625 ms units.

	bte_adv_type_t type;
	bte_own_addr_type_t own_address_type;
	bte_peer_addr_type_t peer_address_type;

	uint8_t peer_address[6]; ///< Public Device Address, Random Device Address, Public Identity Address,
						     ///< or Random (static) Identity Address of the device to be connected.

	bte_adv_channel_map_t chann_map;
	bte_adv_filter_policy_t filter_policy;
} bte_advertise_params_t;

enum {
	H4_TYPE_COMMAND = 1,
	H4_TYPE_ACL     = 2,
	H4_TYPE_SCO     = 3,
	H4_TYPE_EVENT   = 4
};

#define HCI_H4_CMD_PREAMBLE_SIZE           			(4)

#define HCI_GRP_HOST_CONT_BASEBAND_CMDS    			(0x03 << 10)
#define HCI_GRP_BLE_CMDS                   			(0x08 << 10)

#define HCI_RESET                          			(0x0003 | HCI_GRP_HOST_CONT_BASEBAND_CMDS)
#define HCI_SET_EVENT_MASK          				(0x0001 | HCI_GRP_HOST_CONT_BASEBAND_CMDS)

#define HCI_BLE_SET_EVENT_MASK          			(0x0001 | HCI_GRP_BLE_CMDS)
#define HCI_BLE_SET_ADV_ENABLE           			(0x000A | HCI_GRP_BLE_CMDS)
#define HCI_BLE_SET_ADV_PARAMS           			(0x0006 | HCI_GRP_BLE_CMDS)
#define HCI_BLE_SET_ADV_DATA             			(0x0008 | HCI_GRP_BLE_CMDS)

#define HCI_SET_EVENT_MASK_SIZE						(8)

#define HCI_BLE_SET_EVENT_MASK_SIZE					(8)
#define HCI_BLE_SET_ADV_ENABLE_SIZE		        	(1)
#define HCI_BLE_SET_ADV_PARAMS_SIZE			    	(15)
#define HCI_BLE_SET_ADV_DATA_SIZE			      	(31)

#define BD_ADDR_LEN     (6)                     /* Device address length */
typedef uint8_t bd_addr_t[BD_ADDR_LEN];         /* Device address */

#define UINT16_TO_STREAM(p, u16) 					{*(p)++ = (uint8_t)(u16); *(p)++ = (uint8_t)((u16) >> 8);}
#define UINT8_TO_STREAM(p, u8)   					{*(p)++ = (uint8_t)(u8);}
#define BDADDR_TO_STREAM(p, a)   					{int ijk; for (ijk = 0; ijk < BD_ADDR_LEN;  ijk++) *(p)++ = (uint8_t) a[BD_ADDR_LEN - 1 - ijk];}
#define ARRAY_TO_STREAM(p, a, len) 					{int ijk; for (ijk = 0; ijk < len; ijk++) *(p)++ = (uint8_t) a[ijk];}

driver_error_t *HCI_Reset();
driver_error_t *HCI_Set_Event_Mask(uint8_t *mask);
driver_error_t *HCI_LE_Set_Event_Mask(uint8_t *mask);
driver_error_t *HCI_LE_Set_Advertising_Parameters(bte_advertise_params_t params);
driver_error_t *HCI_LE_Set_Advertising_Data(uint8_t *adv_data, uint16_t adv_data_len);
driver_error_t *HCI_LE_Set_Advertise_Enable(uint8_t enable);

#endif /* BT_HCI_H_ */

#endif
