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
 * Lua RTOS, Lua LORaWAN module
 *
 */

#include "freertos/projdefs.h"
#include "luartos.h"

#if CONFIG_LUA_RTOS_LORA_MODE_NODE || CONFIG_LUA_RTOS_LORA_MODE_SINGLE_CHANNEL_GATEWAY ||                              \
    CONFIG_LUA_RTOS_LORA_MODE_MULTICHANNEL_GATEWAY

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "error.h"
#include "hex.h"
#include "hex_string.h"
#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"
#include "modules.h"
#include "sys.h"

#include <stdlib.h>
#include <string.h>

#if CONFIG_LUA_RTOS_LORA_MODE_MULTICHANNEL_GATEWAY
void lora_gw_start();
#endif

#if CONFIG_LUA_RTOS_LORA_MODE_NODE || CONFIG_LUA_RTOS_LORA_MODE_SINGLE_CHANNEL_GATEWAY

#include "lora.h"

static lua_callback_t *callback = NULL;

static void _downlink_dispatcher(void *arg) {
  for (;;) {
    // Get rx queue handle
    QueueHandle_t q_h = lora_get_rx_queue_h();

    if (q_h != NULL) {
      // Wait for new downlink item
	  lora_downlink_t downlink;
	  
      if (xQueueReceive(q_h, &downlink, portMAX_DELAY) == pdPASS) {
        // Convert binary payload to hex string payload
        char hex_payload[LORA_MAC_MAX_PAYLOAD_SIZE + 1];

        val_to_hex_string((char *)hex_payload, (char *)downlink.payload, downlink.size, 0);
        hex_payload[downlink.size * 2] = 0x00;

        // Push argument for the callback's function
        lua_pushinteger(luaS_callback_state(callback), downlink.port);
        lua_pushlstring(luaS_callback_state(callback), hex_payload, strlen(hex_payload));

        // Call callback
        luaS_callback_call(callback, 2);
      }
    } else {
      vTaskDelay(pdMS_TO_TICKS(100));
    }
  }
}

#if CONFIG_LUA_RTOS_LORA_MODE_NODE
static const char *luaL_checkhexstring(lua_State *L, int arg, int len) {
  // Check string
  const char *str = luaL_checkstring(L, arg);

  // Check hex string
  if (!check_hex_str(str)) {
    luaL_exception_extended(L, LORA_ERR_INVALID_ARGUMENT, "invalid hex string");
  }

  // Check length
  if ((len > 0) && (strlen(str) != len)) {
    luaL_exception_extended(L, LORA_ERR_INVALID_ARGUMENT, "invalid hex string length");
  }

  return str;
}
#endif

#endif

static int llora_attach(lua_State *L) {
#if CONFIG_LUA_RTOS_LORA_MODE_NODE || CONFIG_LUA_RTOS_LORA_MODE_SINGLE_CHANNEL_GATEWAY
  driver_error_t *error;

#if CONFIG_LUA_RTOS_LORA_MODE_SINGLE_CHANNEL_GATEWAY
  int band = luaL_checkinteger(L, 1);
  const char *host = luaL_optstring(L, 2, "router.eu.thethings.network");
  int port = luaL_optinteger(L, 3, 1700);
  int freq = luaL_optinteger(L, 4, 868100000);
  int drate = luaL_optinteger(L, 5, 5);

  if ((error = lora_gw_setup(band, host, port, freq, drate))) {
    return luaL_driver_error(L, error);
  }
#endif

#if CONFIG_LUA_RTOS_LORA_MODE_NODE
  int total = lua_gettop(L); // Number of arguments
  int band = luaL_checkinteger(L, 1);

  if (total > 1) {
    if (lua_type(L, 2) == LUA_TFUNCTION) {
      callback = luaS_callback_create(L, 2);
      if (callback == NULL) {
        return luaL_exception_extended(L, LORA_ERR_NO_MEM, NULL);
      }
    }
  }

  // Setup
  error = lora_setup(band);
  if (error) {
    return luaL_driver_error(L, error);
  }

  // Create downlink dispatcher
  BaseType_t xReturned =
      xTaskCreatePinnedToCore(_downlink_dispatcher, "lora_stack", 10 * 1024, NULL, CONFIG_LUA_RTOS_LORA_TASK_PRIORITY,
                              NULL, CONFIG_LUA_RTOS_LORA_TASK_CPU);

  if (xReturned != pdPASS) {
    return luaL_exception_extended(L, LORA_ERR_NO_MEM, NULL);
  }
#endif
#endif
#if CONFIG_LUA_RTOS_LORA_MODE_MULTICHANNEL_GATEWAY
  lora_gw_start();
#endif
  return 0;
}

#if CONFIG_LUA_RTOS_LORA_MODE_NODE
static int llora_set_setDevAddr(lua_State *L) {
  // Sanity checks
  const char *hexArg;

  hexArg = luaL_checkhexstring(L, 1, LORA_MAC_DEVADDR_SIZE);

  // Convert hex string argument to bytes
  uint8_t bytesArg[LORA_MAC_DEVADDR_SIZE];

  hex_string_to_val((char *)hexArg, (char *)(&bytesArg), LORA_MAC_DEVADDR_SIZE / 2, 1);

  // Set
  driver_error_t *error = lora_mac_set(LORA_MAC_SET_DEVADDR, bytesArg);
  if (error) {
    return luaL_driver_error(L, error);
  }

  return 0;
}
#endif

#if CONFIG_LUA_RTOS_LORA_MODE_NODE
static int llora_set_DevEui(lua_State *L) {
  // Sanity checks
  const char *hexArg;

  hexArg = luaL_checkhexstring(L, 1, LORA_MAC_EUI_SIZE);

  // Convert hex string argument to bytes
  uint8_t bytesArg[LORA_MAC_EUI_SIZE];

  hex_string_to_val((char *)hexArg, (char *)(&bytesArg), LORA_MAC_EUI_SIZE / 2, 0);

  // Set
  driver_error_t *error = lora_mac_set(LORA_MAC_SET_DEVEUI, bytesArg);
  if (error) {
    return luaL_driver_error(L, error);
  }

  return 0;
}
#endif

#if CONFIG_LUA_RTOS_LORA_MODE_NODE
static int llora_set_joinEui(lua_State *L) {
  // Sanity checks
  const char *hexArg;

  hexArg = luaL_checkhexstring(L, 1, LORA_MAC_EUI_SIZE);

  // Convert hex string argument to bytes
  uint8_t bytesArg[LORA_MAC_EUI_SIZE];

  hex_string_to_val((char *)hexArg, (char *)(&bytesArg), LORA_MAC_EUI_SIZE / 2, 0);

  // Set
  driver_error_t *error = lora_mac_set(LORA_MAC_SET_JOINEUI, bytesArg);
  if (error) {
    return luaL_driver_error(L, error);
  }

  return 0;
}
#endif

#if CONFIG_LUA_RTOS_LORA_MODE_NODE
static int llora_set_AppKey(lua_State *L) {
  // Sanity checks
  const char *hexArg;

  hexArg = luaL_checkhexstring(L, 1, LORA_MAC_KEY_SIZE);

  // Convert hex string argument to bytes
  uint8_t bytesArg[LORA_MAC_KEY_SIZE];

  hex_string_to_val((char *)hexArg, (char *)(&bytesArg), LORA_MAC_KEY_SIZE / 2, 0);

  // Set
  driver_error_t *error = lora_mac_set(LORA_MAC_SET_APPKEY, bytesArg);
  if (error) {
    return luaL_driver_error(L, error);
  }

  return 0;
}
#endif

#if CONFIG_LUA_RTOS_LORA_MODE_NODE
static int llora_set_NwkKey(lua_State *L) {
  // Sanity checks
  const char *hexArg;

  hexArg = luaL_checkhexstring(L, 1, LORA_MAC_KEY_SIZE);

  // Convert hex string argument to bytes
  uint8_t bytesArg[LORA_MAC_KEY_SIZE];

  hex_string_to_val((char *)hexArg, (char *)(&bytesArg), LORA_MAC_KEY_SIZE / 2, 0);

  // Set
  driver_error_t *error = lora_mac_set(LORA_MAC_SET_NWKKEY, bytesArg);
  if (error) {
    return luaL_driver_error(L, error);
  }

  return 0;
}
#endif

#if CONFIG_LUA_RTOS_LORA_MODE_NODE
static int llora_set_Dr(lua_State *L) {
  int dr = luaL_checkinteger(L, 1);

  if ((dr < 0) || (dr > 7)) {
    return luaL_error(L, "%d:invalid data rate value (0 to 7)", LORA_ERR_INVALID_ARGUMENT);
  }

  // Set
  uint8_t bytesArg[1];
  bytesArg[0] = dr & 0xff;

  driver_error_t *error = lora_mac_set(LORA_MAC_SET_DR, bytesArg);
  if (error) {
    return luaL_driver_error(L, error);
  }

  return 0;
}
#endif

#if CONFIG_LUA_RTOS_LORA_MODE_NODE
static int llora_set_Adr(lua_State *L) {
  // Set
  uint8_t bytesArg[1];

  luaL_checktype(L, 1, LUA_TBOOLEAN);
  if (lua_toboolean(L, 1)) {
    bytesArg[0] = 1;
  } else {
    bytesArg[0] = 0;
  }

  driver_error_t *error = lora_mac_set(LORA_MAC_SET_ADR, bytesArg);
  if (error) {
    return luaL_driver_error(L, error);
  }

  return 0;
}
#endif

#if CONFIG_LUA_RTOS_LORA_MODE_NODE
static int llora_set_nb_trans(lua_State *L) {
  int rets = luaL_checkinteger(L, 1);

  if ((rets < 1) || (rets > 15)) {
    return luaL_error(L, "%d:invalid uplink retransmissions (0 to 8)", LORA_ERR_INVALID_ARGUMENT);
  }

  // Set
  uint8_t bytesArg[1];
  bytesArg[0] = rets & 0xff;

  driver_error_t *error = lora_mac_set(LORA_MAC_SET_NB_TRANS, bytesArg);
  if (error) {
    return luaL_driver_error(L, error);
  }

  return 0;
}
#endif

#if CONFIG_LUA_RTOS_LORA_MODE_NODE
static int llora_set_Class(lua_State *L) {
  int class = luaL_checkinteger(L, 1);

  if ((class < 0) || (class > 2)) {
    return luaL_error(L, "%d:invalid device class value (0 to 2)", LORA_ERR_INVALID_ARGUMENT);
  }

  // Set
  uint8_t bytesArg[1];
  bytesArg[0] = class;

  driver_error_t *error = lora_mac_set(LORA_MAC_SET_DEVICE_CLASS, bytesArg);
  if (error) {
    return luaL_driver_error(L, error);
  }

  return 0;
}
#endif

#if CONFIG_LUA_RTOS_LORA_MODE_NODE
static int llora_get_DevAddr(lua_State *L) {
  // Get
  uint8_t bytesArg[LORA_MAC_DEVADDR_SIZE / 2];
  char hexArg[LORA_MAC_DEVADDR_SIZE];

  driver_error_t *error = lora_mac_get(LORA_MAC_GET_DEVADDR, bytesArg);
  if (error) {
    return luaL_driver_error(L, error);
  }

  val_to_hex_string(hexArg, (char *)bytesArg, sizeof(bytesArg), 1);

  lua_pushlstring(L, hexArg, strlen(hexArg));

  return 1;
}
#endif

#if CONFIG_LUA_RTOS_LORA_MODE_NODE
static int llora_get_DevEui(lua_State *L) {
  // Get
  uint8_t bytesArg[LORA_MAC_EUI_SIZE / 2];
  char hexArg[LORA_MAC_EUI_SIZE];

  driver_error_t *error = lora_mac_get(LORA_MAC_GET_DEVEUI, bytesArg);
  if (error) {
    return luaL_driver_error(L, error);
  }

  val_to_hex_string(hexArg, (char *)bytesArg, sizeof(bytesArg), 0);

  lua_pushlstring(L, hexArg, strlen(hexArg));

  return 1;
}
#endif

#if CONFIG_LUA_RTOS_LORA_MODE_NODE
static int llora_get_joinEui(lua_State *L) {
  // Get
  uint8_t bytesArg[LORA_MAC_EUI_SIZE / 2];
  char hexArg[LORA_MAC_EUI_SIZE];

  driver_error_t *error = lora_mac_get(LORA_MAC_GET_JOINEUI, bytesArg);
  if (error) {
    return luaL_driver_error(L, error);
  }

  val_to_hex_string(hexArg, (char *)bytesArg, sizeof(bytesArg), 0);

  lua_pushlstring(L, hexArg, strlen(hexArg));

  return 1;
}
#endif

#if CONFIG_LUA_RTOS_LORA_MODE_NODE
static int llora_get_Dr(lua_State *L) {
  // Get
  uint8_t value;

  driver_error_t *error = lora_mac_get(LORA_MAC_GET_DR, &value);
  if (error) {
    return luaL_driver_error(L, error);
  }

  lua_pushinteger(L, value);

  return 1;
}
#endif

#if CONFIG_LUA_RTOS_LORA_MODE_NODE
static int llora_get_Adr(lua_State *L) {
  // Get
  uint8_t value;

  driver_error_t *error = lora_mac_get(LORA_MAC_GET_ADR, &value);
  if (error) {
    return luaL_driver_error(L, error);
  }

  lua_pushboolean(L, value);

  return 1;
}
#endif

#if CONFIG_LUA_RTOS_LORA_MODE_NODE
static int llora_get_nb_trans(lua_State *L) {
  // Get
  uint8_t value;

  driver_error_t *error = lora_mac_get(LORA_MAC_GET_NB_TRANS, &value);
  if (error) {
    return luaL_driver_error(L, error);
  }

  lua_pushinteger(L, value);

  return 1;
}
#endif

#if CONFIG_LUA_RTOS_LORA_MODE_NODE
static int llora_get_Class(lua_State *L) {
  // Get
  uint8_t value;

  driver_error_t *error = lora_mac_get(LORA_MAC_GET_DEVICE_CLASS, &value);
  if (error) {
    return luaL_driver_error(L, error);
  }

  lua_pushinteger(L, value);

  return 1;
}
#endif

#if CONFIG_LUA_RTOS_LORA_MODE_NODE
static int llora_join(lua_State *L) {
  driver_error_t *error = lora_join();
  if (error) {
    return luaL_driver_error(L, error);
  }

  return 0;
}
#endif

#if CONFIG_LUA_RTOS_LORA_MODE_NODE
static int llora_tx(lua_State *L) {
  luaL_checktype(L, 1, LUA_TBOOLEAN);
  int cnf = lua_toboolean(L, 1);
  int port = luaL_checkinteger(L, 2);
  const char *hexArg = luaL_checkhexstring(L, 3, 0);

  // Check if transmissions is for regular up-links (subject to application throttle) or urgent up-links
  int regular = 1;

  if (lua_gettop(L) == 4) {
    luaL_checktype(L, 4, LUA_TBOOLEAN);
    regular = lua_toboolean(L, 4);
  }

  if ((port < 1) || (port > 223)) {
    return luaL_error(L, "%d:invalid port number", LORA_ERR_INVALID_ARGUMENT);
  }

  // Convert payload (hex string) into bytes
  uint8_t bytesArg[LORA_MAC_MAX_PAYLOAD_SIZE / 2];

  hex_string_to_val((char *)hexArg, (char *)(&bytesArg), LORA_MAC_MAX_PAYLOAD_SIZE / 2, 0);

  // Tx
  lora_tx_ret_t tx_ret;

  driver_error_t *error = lora_tx(regular, cnf, port, bytesArg, strlen(hexArg) >> 1, &tx_ret);
  if (error) {
    return luaL_driver_error(L, error);
  }

  lua_pushinteger(L, tx_ret.frequency);
  lua_pushinteger(L, tx_ret.time_on_air);
  lua_pushinteger(L, tx_ret.time_off_air);
  lua_pushinteger(L, tx_ret.dr);

  return 4;
}
#endif

#if CONFIG_LUA_RTOS_LORA_MODE_NODE
static int llora_deviceTimeReq(lua_State *L) {
  driver_error_t *error = lora_device_time_req();
  if (error) {
    return luaL_driver_error(L, error);
  }

  return 0;
}
#endif

#if CONFIG_LUA_RTOS_LORA_MODE_NODE
static int llora_linkCheckReq(lua_State *L) {
  lora_link_check_req_ret_t req_ret;

  driver_error_t *error = lora_link_check_req(&req_ret);
  if (error) {
    return luaL_driver_error(L, error);
  }

  lua_pushinteger(L, req_ret.demod_margin);
  lua_pushinteger(L, req_ret.nb_gateways);

  return 2;
}
#endif

static const LUA_REG_TYPE lora_map[] = {{LSTRKEY("attach"), LFUNCVAL(llora_attach)},
#if CONFIG_LUA_RTOS_LORA_MODE_NODE
                                        {LSTRKEY("setDevAddr"), LFUNCVAL(llora_set_setDevAddr)},
                                        {LSTRKEY("setDevEui"), LFUNCVAL(llora_set_DevEui)},
                                        {LSTRKEY("setJoinEui"), LFUNCVAL(llora_set_joinEui)},
                                        {LSTRKEY("setAppKey"), LFUNCVAL(llora_set_AppKey)},
                                        {LSTRKEY("setNwkKey"), LFUNCVAL(llora_set_NwkKey)},
                                        {LSTRKEY("setDr"), LFUNCVAL(llora_set_Dr)},
                                        {LSTRKEY("setAdr"), LFUNCVAL(llora_set_Adr)},
                                        {LSTRKEY("setNbTrans"), LFUNCVAL(llora_set_nb_trans)},
                                        {LSTRKEY("setClass"), LFUNCVAL(llora_set_Class)},
                                        {LSTRKEY("getDevAddr"), LFUNCVAL(llora_get_DevAddr)},
                                        {LSTRKEY("getDevEui"), LFUNCVAL(llora_get_DevEui)},
                                        {LSTRKEY("getJoinEui"), LFUNCVAL(llora_get_joinEui)},
                                        {LSTRKEY("getDr"), LFUNCVAL(llora_get_Dr)},
                                        {LSTRKEY("getAdr"), LFUNCVAL(llora_get_Adr)},
                                        {LSTRKEY("getNbTrans"), LFUNCVAL(llora_get_nb_trans)},
                                        {LSTRKEY("getClass"), LFUNCVAL(llora_get_Class)},
                                        {LSTRKEY("join"), LFUNCVAL(llora_join)},
                                        {LSTRKEY("tx"), LFUNCVAL(llora_tx)},
                                        {LSTRKEY("deviceTimeReq"), LFUNCVAL(llora_deviceTimeReq)},
                                        {LSTRKEY("linkCheckReq"), LFUNCVAL(llora_linkCheckReq)},

                                        // Constant definitions
                                        {LSTRKEY("CLASS_A"), LINTVAL(0)},
                                        {LSTRKEY("CLASS_B"), LINTVAL(1)},
                                        {LSTRKEY("CLASS_C"), LINTVAL(2)},
#endif
#if CONFIG_LUA_RTOS_LORA_MODE_NODE || CONFIG_LUA_RTOS_LORA_MODE_SINGLE_CHANNEL_GATEWAY
                                        {LSTRKEY("BAND868"), LINTVAL(868)},
                                        {LSTRKEY("BAND433"), LINTVAL(433)},
                                        {LSTRKEY("BAND915"), LINTVAL(915)},

                                        DRIVER_REGISTER_LUA_ERRORS(lora)
#endif

                                            {LNILKEY, LNILVAL}};

int luaopen_lora(lua_State *L) { return 0; }

MODULE_REGISTER_ROM(LORA, lora, lora_map, luaopen_lora, 1);

#endif

/*
        Simple channel gateway example:

        net.wf.setup(net.wf.mode.STA,"CITILAB","wifi@citilab")
        net.wf.start()
        net.service.sntp.start()

        lora.attach(lora.BAND868, nil, nil, 868100000, 5)

 */
