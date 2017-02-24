/*
 * Lua RTOS, some debug functions
 *
 * Copyright (C) 2015 - 2017
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

#ifndef _SYS_DEBUG_H_
#define _SYS_DEBUG_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


#if DEBUG_FREE_MEM
#define debug_used_stack() printf("remaining stak %d bytes\r\n", uxTaskGetStackHighWaterMark(NULL) * 4)

#define debug_free_mem_begin(var) \
int elapsed_begin_##var = xPortGetFreeHeapSize(); 

#define debug_free_mem_end(var,msg) \
int elapsed_end_##var = xPortGetFreeHeapSize(); \
const char *elapsed_end_##var_msg = msg; \
if (elapsed_end_##var_msg) { \
	printf("%s (%s) comsumption %d bytes (%d bytes free)\n", (char *)#var, elapsed_end_##var_msg, elapsed_begin_##var - elapsed_end_##var, xPortGetFreeHeapSize()); \
} else { \
	printf("%s comsumption %d bytes (%d bytes free)\n", (char *)#var, elapsed_begin_##var - elapsed_end_##var, xPortGetFreeHeapSize());	\
}
#else
#define debug_free_mem_begin(var)
#define debug_free_mem_end(var, msg)
#define debug_used_stack()
#endif

#endif /* !_SYS_DEBUG_H_ */



