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

int pthread_attr_init(pthread_attr_t *attr) {
    attr->stack_size = PTHREAD_STACK_MIN;
    attr->initial_state = PTHREAD_INITIAL_STATE_RUN;
    
    return 0;
}

int pthread_attr_destroy(pthread_attr_t *attr) {
    return 0;
    
}

int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize) {
    if (stacksize < PTHREAD_STACK_MIN) {
        errno = EINVAL;
        return EINVAL;
    }
    
    attr->stack_size = stacksize;
    
    return 0;
}

int pthread_attr_setinitialstate(pthread_attr_t *attr, int initial_state) {
    if ((initial_state != PTHREAD_INITIAL_STATE_RUN) && (initial_state != PTHREAD_INITIAL_STATE_SUSPEND)) {
        errno = EINVAL;
        return EINVAL;
    }
    
    attr->initial_state = initial_state;
    
    return 0;
}

int pthread_attr_getstacksize(const pthread_attr_t *attr, size_t *stacksize) {
    *stacksize = attr->stack_size;
    
    return 0;
}

int pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate) {
    return 0;
}

int pthread_setcancelstate(int state, int *oldstate) {
    return 0;
}