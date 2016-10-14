/*
 * Whitecat, machine device init
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

#include "whitecat.h"
#include "FreeRTOS.h"
#include "build.h"

#include <sys/drivers/console.h>
#include <sys/drivers/cpu.h>

#include <stdio.h>
#include <fcntl.h>
		
#include <unistd.h>
#include <sys/syslog.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/syscalls/mount.h>
#include <unistd.h>
#include <errno.h>



//const char *__progname = "whitecat";
extern const char *__progname;

#include <sys/filedesc.h>
#include <sys/file.h>


#if USE_CFI
extern int spiffs_init();
#endif

void mach_dev() { 
	console_clear();

    printf("  /\\       /\\\n");
    printf(" /  \\_____/  \\\n");
    printf("/_____________\\\n");
    printf("W H I T E C A T\n\n");

    printf("LuaOS %s build %d Copyright (C) 2015 - 2016 whitecatboard.org\n\n", LUA_OS_VER, BUILD_TIME);
    
    openlog(__progname, LOG_CONS | LOG_NDELAY, LOG_LOCAL1);
	
    cpu_show_info();
    
    //Init filesystem  
    #if USE_CFI
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
