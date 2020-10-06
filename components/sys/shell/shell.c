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
 * Lua RTOS shell
 *
 */

#include "lua.h"

#if CONFIG_LUA_RTOS_USE_SSH_SERVER

#include "shell.h"
#include "esp_ota_ops.h"

#include <signal.h>
#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/delay.h>
#include <sys/console.h>
#include <sys/param.h>

#include <reent.h>

extern void doREPL(lua_State *L);
extern void firmware_copyright_notice();

static void *shell(void *arg) {
	shell_config_t *config = (shell_config_t *)arg;

	// Set standard i/o streams for the shell
	__getreent()->_stdin  = fdopen(config->fdin,"a+");
	__getreent()->_stdout = fdopen(config->fdout,"a+");
	__getreent()->_stderr = fdopen(config->fderr,"a+");

	// Work-around newlib is not compiled with HAVE_BLKSIZE flag
	setvbuf(__getreent()->_stdin , NULL, _IONBF, 0);
	setvbuf(__getreent()->_stdout, NULL, _IONBF, 0);
	setvbuf(__getreent()->_stderr, NULL, _IONBF, 0);

	firmware_copyright_notice();

	const esp_partition_t *running = esp_ota_get_running_partition();

	printf(
		"Lua RTOS %s. Copyright (C) 2015 - 2018 whitecatboard.org\r\n\r\nbuild %d\r\ncommit %s\r\nRunning from %s partition\r\n",
		LUA_OS_VER, BUILD_TIME, BUILD_COMMIT, running->label
	);

	printf("board type %s\r\n\r\n", CONFIG_LUA_RTOS_BOARD_TYPE);

	// Run lua in interpreter mode
	lua_State *L = pvGetLuaState();  /* get state */
	lua_State* TL = L ? lua_newthread(L) : NULL;
	doREPL(TL ? TL : L);

	printf("thread %d\r\n", config->parent_thread);

	// The lua interpreter exit when the user enters the Ctrl-C key
	// This situation must be interpreted as a SIGINT signal
	pthread_kill(config->parent_thread, SIGINT);

	return NULL;
}

int create_shell(shell_config_t *config) {
	pthread_attr_t attr;
	struct sched_param sched;
	pthread_t thread;
	int res;

	// Init thread attributes
	pthread_attr_init(&attr);

	// Set stack size
	pthread_attr_setstacksize(&attr, CONFIG_LUA_RTOS_SSH_SHELL_STACK_SIZE);

	// Set priority
	sched.sched_priority = CONFIG_LUA_RTOS_SSH_SHELL_TASK_PRIORITY;
	pthread_attr_setschedparam(&attr, &sched);

	// Set CPU
	cpu_set_t cpu_set = CPU_INITIALIZER;
	CPU_SET(CONFIG_LUA_RTOS_SSH_SHELL_TASK_CPU, &cpu_set);

	pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpu_set);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	// Create thread
	res = pthread_create(&thread, &attr, shell, (void *)config);
	if (res) {
		errno = ENOMEM;
		return -1;
	}

	pthread_setname_np(thread, "shell");
	delay(1000);

	return 0;
}

#endif

