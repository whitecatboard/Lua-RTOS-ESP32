/*
 * Lua RTOS, xc.h implementation for minimal change over FreeRTOS sources for
 * compilation with gcc
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

#ifndef XC_H
#define	XC_H

#include <machine/pic32mz.h>
#include <machine/machConst.h>

#define _CP0_GET_STATUS mfc0_Status
#define _CP0_SET_STATUS mtc0_Status
#define _clz __builtin_clz 
#define __builtin_disable_interrupts mips_di

#define _CP0_BIS_CAUSE(a) \
            int rup = mfc0_Cause(); \
            mtc0_Cause(rup | a);
            
            
#endif

