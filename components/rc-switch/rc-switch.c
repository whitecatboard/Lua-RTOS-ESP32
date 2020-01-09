/*
 * Copyright (C) 2015 - 2018, IBEROXARXA SERVICIOS INTEGRALES, S.L.
 * Copyright (C) 2015 - 2019, Thomas E. Horner (whitecatboard.org@horner.it)
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
 */

#include "sdkconfig.h"
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "error.h"

#if CONFIG_LUA_RTOS_LUA_USE_RCSWITCH

#include "arduino.h"
#include "RCSwitch.h"
#include "rc-switch.h"
#include <freertos/task.h>
#include <sys/driver.h>
#include "driver/timer.h"
#include <sys/stat.h>
#include <sys/status.h>
#include <sys/delay.h>
#include "sys.h"
#include <sys/syslog.h>

// Register drivers and errors
DRIVER_REGISTER_BEGIN(RCSWITCH,rcswitch,0,NULL,NULL);
	DRIVER_REGISTER_ERROR(RCSWITCH, rcswitch, CannotStartThread, "can't start rcswitch listen thread",  LUA_RCSWITCH_ERR_CANT_START_THREAD);
DRIVER_REGISTER_END(RCSWITCH,rcswitch,0,NULL,NULL);

static uint8_t rcswitch_initialized = 0;
static uint8_t volatile listen_shutdown = 0;
static lua_callback_t *receive_callback = NULL;

static void ensure_initialized() {
	if (rcswitch_initialized == 0) {
		RCSwitch_init();
		rcswitch_initialized = 1;
	}
}

int rcswitch_send(lua_State *L) {

  unsigned long code = luaL_checkinteger(L, 1);
  uint8_t pin = luaL_optinteger(L, 2, 12);
  int pulse = luaL_optinteger(L, 3, 330);
  uint8_t protocol = luaL_optinteger(L, 4, 1);
  uint8_t bitlength = luaL_optinteger(L, 5, 24);

  syslog(LOG_INFO, "rcswitch: sending code %lu on pin %u\n", code, pin);

  ensure_initialized();
  RCSwitch_enableTransmit(pin); // txPin
  RCSwitch_setProtocol(protocol);
  RCSwitch_setPulseLength(pulse); // = "receiving delay"
  RCSwitch_send(code, bitlength);

  return 0;
}

static void rcswitch_clear_callback(lua_State *L) {
  if (receive_callback != NULL) {
    luaS_callback_destroy(receive_callback);
    receive_callback = NULL;
  }
}

static void *rcswitch_listen_thread(void *arg) {
  int pin = (int)arg;

  ensure_initialized();
  RCSwitch_enableReceive(pin); // rxPin

  while (!listen_shutdown) {
    if (RCSwitch_available()) {

      syslog(LOG_INFO, "rcswitch: Received %lu / %u bit   Protocol: %u   Delay: %u\n", 
                       RCSwitch_getReceivedValue(), RCSwitch_getReceivedBitlength(), 
                       RCSwitch_getReceivedProtocol(), RCSwitch_getReceivedDelay() );

      if (receive_callback != NULL) {
        lua_State *state = luaS_callback_state(receive_callback);
        lua_pushinteger(state, RCSwitch_getReceivedValue());
        lua_pushinteger(state, RCSwitch_getReceivedBitlength());
        lua_pushinteger(state, RCSwitch_getReceivedProtocol());
        lua_pushinteger(state, RCSwitch_getReceivedDelay());
        luaS_callback_call(receive_callback, 4);
      }
      else {
        syslog(LOG_ERR, "rcswitch: cannot call lua, callback is empty");
      }

      RCSwitch_resetAvailable();
    }
  }

  //pthread_exit(NULL);
  return NULL;
}

int rcswitch_listen(lua_State *L) {

  int pin = luaL_optinteger(L, 1, 14);
  unsigned long millis = luaL_optinteger(L, 2, 500);

  if (millis == 0 && lua_gettop(L) > 2) {

    rcswitch_clear_callback(L);
    luaL_checktype(L, 3, LUA_TFUNCTION);
    receive_callback = luaS_callback_create(L, 3);

    pthread_t thread_listen;
    pthread_attr_t attr;
    struct sched_param sched;

    // Init thread attributes
    pthread_attr_init(&attr);

    // Set stack size
    pthread_attr_setstacksize(&attr, CONFIG_LUA_RTOS_RCSWITCH_SERVER_STACK_SIZE);

    // Set priority
    sched.sched_priority = CONFIG_LUA_RTOS_RCSWITCH_SERVER_TASK_PRIORITY;
    pthread_attr_setschedparam(&attr, &sched);

    // Set CPU
    cpu_set_t cpu_set = CPU_INITIALIZER;
    CPU_SET(CONFIG_LUA_RTOS_RCSWITCH_SERVER_TASK_CPU, &cpu_set);

    pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpu_set);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    listen_shutdown = 0; //reset the shutdown flag

    int res = pthread_create(&thread_listen, &attr, rcswitch_listen_thread, (void*)pin);
    if (res) {
      return luaL_driver_error(L, driver_error(RCSWITCH_DRIVER, LUA_RCSWITCH_ERR_CANT_START_THREAD, NULL));
    }

    pthread_setname_np(thread_listen, "rcswitch");
    pthread_attr_destroy(&attr);

  }
  else {

    listen_shutdown = 1;
    usleep(1000); // 1ms

    ensure_initialized();
    RCSwitch_enableReceive(pin); // rxPin

    //blocking call for a specified maximum time, e.g. 500ms
    //upto must be greater than the expected getReceivedDelay (!)
    //or the function must be called repeated and without delay
    const long upto = micros() + (millis * 1000);
    while(micros() < upto) {
      if (RCSwitch_available()) {
        syslog(LOG_INFO, "rcswitch: Received %lu / %u bit   Protocol: %u   Delay: %u\n", 
                         RCSwitch_getReceivedValue(), RCSwitch_getReceivedBitlength(), 
                         RCSwitch_getReceivedProtocol(), RCSwitch_getReceivedDelay() );

        lua_pushinteger(L, RCSwitch_getReceivedValue());
        lua_pushinteger(L, RCSwitch_getReceivedBitlength());
        lua_pushinteger(L, RCSwitch_getReceivedProtocol());
        lua_pushinteger(L, RCSwitch_getReceivedDelay());

        RCSwitch_resetAvailable();
        return 4;
      }
    }
  }

  return 0;
}

void rcswitch_destroy(lua_State *L) {
	rcswitch_clear_callback(L);
	if (rcswitch_initialized != 0) {
		listen_shutdown = 1;
		usleep(1000); // 1ms
		rcswitch_initialized = 0;
	}
}

#endif
