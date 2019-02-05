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
 * Lua RTOS, Lua OpenVPN net module
 *
 */

#include "sdkconfig.h"

#if CONFIG_LUA_RTOS_USE_OPENVPN

#include "net.h"

#include <unistd.h>
#include <pthread.h>

#include <sys/driver.h>
#include <sys/delay.h>

#include <drivers/net.h>

int openvpn(int argc, char *argv[]);
static u8_t volatile _openvpn_running = 0;

static void *openvpn_thread(void *arg) {
	// Wait for network
    if (!wait_for_network(20000)) {
        luaL_exception(L, NET_ERR_NOT_AVAILABLE);
    }

	delay(1000);

	char* argv[] = {
			"openvpn",
			"--config", CONFIG_LUA_RTOS_OPENVPN_CONFIG_FILE,
			"--ifconfig-nowarn",
			"--single-session"
	};

	_openvpn_running = 1;
	openvpn(5, argv);
	_openvpn_running = 0;

	return NULL;
}

static int lopenvpn_service_start(lua_State* L) {
	pthread_t thread;
	pthread_attr_t attr;

	pthread_attr_init(&attr);

    pthread_attr_setstacksize(&attr, 8192);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    if (pthread_create(&thread, &attr, openvpn_thread, NULL)) {
    	return 0;
	}

    pthread_setname_np(thread, "openvpn");

	return 0;
}

static int lopenvpn_service_stop(lua_State* L) {
	return 0;
}

static int lopenvpn_service_running( lua_State* L ) {
	lua_pushboolean(L, _openvpn_running);
	return 1;
}

static const LUA_REG_TYPE openvpn_map[] = {
	{ LSTRKEY("start"  ), LFUNCVAL( lopenvpn_service_start   ) },
	{ LSTRKEY("stop"   ), LFUNCVAL( lopenvpn_service_stop    ) },
	{ LSTRKEY("running"), LFUNCVAL( lopenvpn_service_running ) },
	{ LNILKEY,LNILVAL }
};

#endif
