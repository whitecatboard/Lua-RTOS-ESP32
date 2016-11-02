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

#include "build.h"
#include "lua.h"

#include <string.h>
#include <stdio.h>

#include <sys/reent.h>
#include <sys/syslog.h>
#include <sys/drivers/console.h>
#include <sys/drivers/cpu.h>
#include <sys/syscalls/mount.h>

extern void _clock_init();
extern void _syscalls_init();
extern void _pthread_init();
extern void _console_init();
extern void _lora_init();
extern void _signal_init();
extern void _mtx_init();

extern void _cleanup_r(struct _reent* r);
extern const char *__progname;

#if USE_SPIFFS
extern int spiffs_init();
#endif

#if !PLATFORM_ESP32
void _reent_init(struct _reent* r) {
    memset(r, 0, sizeof(*r));
    r->_stdout = _GLOBAL_REENT->_stdout;
    r->_stderr = _GLOBAL_REENT->_stderr;
    r->_stdin  = _GLOBAL_REENT->_stdin;
    r->__cleanup = &_cleanup_r;
    r->__sdidinit = 1;
    r->__sglue._next = NULL;
    r->__sglue._niobs = 0;
    r->__sglue._iobs = NULL;
    r->_current_locale = "C";
}
#endif

void _sys_init() {  	
#if !PLATFORM_ESP32
	_reent_init(_GLOBAL_REENT);
#endif
	
    _mtx_init();
    _pthread_init();
    _clock_init();
    _syscalls_init();
	_console_init();
    _signal_init();

	console_clear();

    printf("  /\\       /\\\n");
    printf(" /  \\_____/  \\\n");
    printf("/_____________\\\n");
    printf("W H I T E C A T\n\n");
		
    printf("LuaOS %s build %d Copyright (C) 2015 - 2016 whitecatboard.org\n\n", LUA_OS_VER, BUILD_TIME);
    
    openlog(__progname, LOG_CONS | LOG_NDELAY, LOG_LOCAL1);
	
    cpu_show_info();
    
    //Init filesystem  
    #if USE_SPIFFS
		if (spiffs_init()) {
            mount_set_mounted("cfi", 1);
		}
    #endif

    #if USE_SD
        if (sd_init(0)) {
            if (fat_init()) {
                mount_set_mounted("sd", 1);
            }
        }
    #endif

    #if USE_RTC
        rtc_init(time(NULL));
    #endif
        
    #if (USE_SD)
        if (mount_is_mounted("sd")) {
            // Redirect console messages to /log/messages.log ...
            closelog();            
            syslog(LOG_INFO, "redirecting console messages to /sd/log/messages.log ...");
            openlog(__progname, LOG_NDELAY , LOG_LOCAL1);
        } else {
            syslog(LOG_ERR, "can't redirect console messages to /sd/log/messages.log, insert an SDCARD");
        }   
    #endif
        
    // Log only errors
	setlogmask(LOG_ERR);
        
    // Continue init ...
    printf("\n");
}