/*
 * Whitecat, main program
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
#include "task.h"
#include "portable.h"
#include "timers.h"
//#include "lua.h"
  
//#include <machine/pic32mz.h>
//#include <machine/machConst.h>
#include <sys/drivers/gpio.h>
#include <sys/drivers/uart.h>
#include <sys/syslog.h>
#include <sys/panic.h>
//#include <drivers/network/network.h>
//#include <drivers/lora/lora.h>
//#include <utils/delay.h>

#include <stdio.h>
#include <fcntl.h>

//#include <stdio.h>
//#include <sys/param.h>
//#include <sys/stat.h>
//#include <unistd.h>
//#include <errno.h>
//#include <stdlib.h>
//#include <string.h>
//#include <syslog.h>
//#include <stdarg.h>
//#include <dirent.h>
//#include <time.h>
#include <setjmp.h>

#include "pthread.h"
#include <stdlib.h>
#include <sys/delay.h>
#include <sys/debug.h>

extern const char *__progname;

// #include "math.h"

int luaos_main (void);

void *lua_start(void *arg) {
	for(;;) {
		luaos_main();
    }

    return NULL;
}

//extern char *normalize_path(const char *path);
//extern double nmea_geoloc_to_decimal(char *token);
   

void user_init(void) {	
	#if LED_ACT
    	// Init leds
    	gpio_pin_output(LED_ACT);
    	gpio_pin_clr(LED_ACT);
	#endif

    pthread_attr_t attr;
    pthread_t thread;
    int res;

	debug_free_mem_begin(lua_main_thread);
    
	pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, luaTaskStack);

    res = pthread_create(&thread, &attr, lua_start, NULL);
    if (res) {
		panic("Cannot start lua");
	}
	
	debug_free_mem_end(lua_main_thread, NULL);
}

