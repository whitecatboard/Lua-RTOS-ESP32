/*
 * Lua RTOS, system init
 *
 * Copyright (C) 2015 - 2016
 * IBEROXARXA SERVICIOS INTEGRALES, S.L. & CSS IBÉRICA, S.L.
 * 
 * Author: Jaume Olivé (jolive@iberoxarxa.com / jolive@whitecatboard.org)
 * 
 * All rights reserved.  
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appear in all
 * copies and that both that the copyright notice and this
 * permission notice and warranty disclaimer appear in supporting
 * documentation, and that the name of the author not be used in
 * advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * The author disclaim all warranties with regard to this
 * software, including all implied warranties of merchantability
 * and fitness.  In no event shall the author be liable for any
 * special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether
 * in an action of contract, negligence or other tortious action,
 * arising out of or in connection with the use or performance of
 * this software.
 */

#include "luartos.h"

#include "lua.h"

#include <string.h>
#include <stdio.h>

#include <sys/reent.h>
#include <sys/syslog.h>
#include <sys/console.h>
#include <sys/mount.h>
#include <sys/time.h>

#include <drivers/cpu.h>
#include <drivers/uart.h>

extern void _syscalls_init();
extern void _pthread_init();
extern void _signal_init();
extern void _mtx_init();
extern void _resource_init();
extern void _cpu_init();
extern void _clock_init();

#if LUA_USE_LORA
extern void _lora_init();
#endif

extern const char *__progname;

#if USE_SPIFFS
extern int spiffs_init();
#endif

void vfs_tty_register();
void vfs_spiffs_register();
void vfs_fat_register();

void _sys_init() {
	// TO DO: do this only if RTC is not set
	struct timeval tv;

	tv.tv_sec = BUILD_TIME;
	tv.tv_usec = 0;

	settimeofday(&tv, NULL);

	// Init important things for Lua RTOS
	_clock_init();
	_cpu_init();
	_resource_init();
    _mtx_init();
    _pthread_init();
    _syscalls_init();
    _signal_init();

#if LUA_USE_LORA
    _lora_init();
#endif
	console_clear();

	vfs_tty_register();

	printf("  /\\       /\\\r\n");
    printf(" /  \\_____/  \\\r\n");
    printf("/_____________\\\r\n");
    printf("W H I T E C A T\r\n\r\n");

    printf("Lua RTOS %s build %d Copyright (C) 2015 - 2016 whitecatboard.org\r\n", LUA_OS_VER, BUILD_TIME);

    openlog(__progname, LOG_CONS | LOG_NDELAY, LOG_LOCAL1);

    cpu_show_info();

    //Init filesystem  
    #if USE_SPIFFS
    	vfs_spiffs_register();
    #endif

	#if USE_FAT
    	vfs_fat_register();

    	if (mount_is_mounted("fat")) {
            // Redirect console messages to /log/messages.log ...
            closelog();
            syslog(LOG_INFO, "redirecting console messages to file system ...");
            openlog(__progname, LOG_NDELAY , LOG_LOCAL1);
        } else {
        	syslog(LOG_ERR, "can't redirect console messages to file system, an SDCARD is needed");
        }   
    #endif
        
    // Continue init ...
    printf("\n");
}
