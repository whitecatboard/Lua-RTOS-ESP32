/*
 * Whitecat, pthread implementation ober FreeRTOS
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

#include "pthread.h"

#include <errno.h>

extern struct mtx once_mtx;

int pthread_once(pthread_once_t *once_control, void (*init_routine)(void)) {
    // Init once_control, if not
    mtx_lock(&once_mtx);
#if !MTX_USE_EVENTS
    if (once_control->mutex.sem == NULL) {
        mtx_init(&once_control->mutex, NULL, NULL, 0);
    }
#else
    if (once_control->mutex.mtxid == 0) {
        mtx_init(&once_control->mutex, NULL, NULL, 0);
    }
#endif    
    // Excec init_routine, if needed
    if (mtx_trylock(&once_control->mutex)) {
        init_routine();
    }

    mtx_unlock(&once_mtx);  
    
    return 0;
}
