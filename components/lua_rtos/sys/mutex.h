/*
 * Lua RTOS, mutex api implementation over FreeRTOS
 *
 * Copyright (C) 2015 - 2017
 * IBEROXARXA SERVICIOS INTEGRALES, S.L.
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

#ifndef MUTEX_H_H
#define	MUTEX_H_H

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#define MUTEX_INITIALIZER {.sem = 0}

struct mtx {
    SemaphoreHandle_t sem;
};

void mtx_init(struct mtx *mutex, const char *name, const char *type, int opts);
void mtx_lock(struct mtx *mutex);
int  mtx_trylock(struct	mtx *mutex);
void mtx_unlock(struct mtx *mutex);
void mtx_destroy(struct	mtx *mutex);

#endif	/* MUTEX_H_H */

