/*
 * Copyright (C) 2018, IBEROXARXA SERVICIOS INTEGRALES, S.L.
 * Copyright (C) 2018, Jaume Olivé Petrus (jolive@whitecatboard.org)
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
 * Lua RTOS ssh Lua module
 *
 */

#include "sdkconfig.h"

#if CONFIG_LUA_RTOS_USE_SSH_SERVER

#include "net.h"

#include <unistd.h>
#include <pthread.h>

#include <sys/driver.h>
#include <sys/delay.h>

int dropbear_server_main(int argc, char ** argv);

static void *ssh_thread(void *arg) {
	driver_error_t *error;

	// Wait for network
	while ((error = net_check_connectivity())) {
		free(error);
		delay(100);
	}

	delay(1000);

	char* argv[] = {
			"dropbear"
	};

	dropbear_server_main(1, argv);

	return NULL;
}

static int lssh_service_start(lua_State* L) {
	pthread_t thread;
	pthread_attr_t attr;

	pthread_attr_init(&attr);

    pthread_attr_setstacksize(&attr, 12288);

    if (pthread_create(&thread, &attr, ssh_thread, NULL)) {
    	return 0;
	}

    pthread_setname_np(thread, "ssh");

	return 0;
}

static int lssh_service_stop(lua_State* L) {
	return 0;
}

static const LUA_REG_TYPE sshs_map[] = {
	{ LSTRKEY("start"), LFUNCVAL(lssh_service_start) },
	{ LSTRKEY("stop") , LFUNCVAL(lssh_service_stop ) },
	{ LNILKEY,LNILVAL }
};

#endif
