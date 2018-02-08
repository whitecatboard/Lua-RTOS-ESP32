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
 * Lua RTOS, BT driver
 *
 */

#include "sdkconfig.h"

#include "bluetooth.h"
#include "esp_bt.h"

#include <stdint.h>
#include <string.h>

#include <sys/delay.h>
#include <sys/driver.h>
#include <sys/syslog.h>

DRIVER_REGISTER_BEGIN(BT,bt,NULL,NULL,NULL);
	DRIVER_REGISTER_ERROR(BT, bt, CannotSetup, "can't setup", BT_ERR_CANT_INIT);
	DRIVER_REGISTER_ERROR(BT, bt, InvalidMode, "invalid mode", BT_ERR_INVALID_MODE);
	DRIVER_REGISTER_ERROR(BT, bt, NotSetup, "is not setup", BT_ERR_IS_NOT_SETUP);
	DRIVER_REGISTER_ERROR(BT, bt, NotEnoughtMemory, "not enough memory", BT_ERR_NOT_ENOUGH_MEMORY);
	DRIVER_REGISTER_ERROR(BT, bt, InvalidArgument, "invalid argument", BT_ERR_INVALID_ARGUMENT);
DRIVER_REGISTER_END(BT,bt,NULL,NULL,NULL);

// Is BT setup?
static uint8_t setup = 0;

/*
 * Helper functions
 */

static void gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t* param) {
}

static void controller_rcv_pkt_ready(void) {
}

static int host_rcv_pkt(uint8_t *data, uint16_t len) {
    return 0;
}

/*
 * Operation functions
 */

driver_error_t *bt_setup(bt_mode_t mode) {
	esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();

	// Sanity checks
	if (setup) {
		return NULL;
	}

	if (mode > Dual) {
		return driver_error(BT_DRIVER, BT_ERR_INVALID_MODE, NULL);
	}

	// Initialize BT controller
	esp_bt_controller_init(&bt_cfg);

	// Enable BT controller
	if (esp_bt_controller_enable(mode) != ESP_OK) {
		return driver_error(BT_DRIVER, BT_ERR_CANT_INIT, NULL);
	}

	// Register vhci callbaks
	static esp_vhci_host_callback_t vhci_host_cb = {
	    controller_rcv_pkt_ready,
	    host_rcv_pkt
	};

	esp_vhci_host_register_callback(&vhci_host_cb);

	// Reset controller
	bt_reset();

    esp_bluedroid_init();
    esp_bluedroid_enable();
	esp_ble_gap_register_callback(gap_cb);

	setup = 1;

	return NULL;
}

driver_error_t *bt_reset() {
	driver_error_t *error;

	// Sanity checks
	if (!setup) {
		return driver_error(BT_DRIVER, BT_ERR_IS_NOT_SETUP, NULL);
	}

	if ((error = HCI_Reset())) {
		return error;
	}

	return NULL;
}

driver_error_t *bt_adv(bte_advertise_params_t adv_params, uint8_t *adv_data, uint16_t adv_data_len) {
	driver_error_t *error;

	// Sanity checks
	if (!setup) {
		return driver_error(BT_DRIVER, BT_ERR_IS_NOT_SETUP, NULL);
	}

	if ((error = HCI_LE_Set_Advertising_Parameters(adv_params))) return error;
	if ((error = HCI_LE_Set_Advertising_Data(adv_data, adv_data_len))) return error;
	if ((error = HCI_LE_Set_Advertise_Enable(1))) return error;

	return NULL;
}
