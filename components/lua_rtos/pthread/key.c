/*
 * Lua RTOS, pthread implementation over FreeRTOS
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

#include <errno.h>
#include <stdlib.h>
#include <pthread/pthread.h>

extern struct list key_list;

int pthread_key_create(pthread_key_t *k, void (*destructor)(void*)) {
    struct pthread_key *key;
    int res;
    
    // Allocate space for the key
    key = (struct pthread_key *)malloc(sizeof(struct pthread_key));
    if (!key) {
        errno = ENOMEM;
        return ENOMEM;
    }
    
    // Init key
    key->destructor = destructor;
    
    list_init(&key->specific,1);
    
    // Add key to key list
    res = list_add(&key_list, key, k);
    if (res) {
        free(key);
        errno = res;
        return res;
    }
    
    return 0;
}

int pthread_setspecific(pthread_key_t k, const void *value) {
    struct pthread_key_specific *specific;
    struct pthread_key *key;    
    int res;
    
    // Get key
    res = list_get(&key_list, k, (void **)&key);
    if (res) {
        return res;
    }

    // Allocate space for specific
    specific = (struct pthread_key_specific *)malloc(sizeof(struct pthread_key_specific));
    if (!specific) {
        errno = ENOMEM;
        return ENOMEM;
    }
    
    specific->thread = pthread_self();
    specific->value = value;
    
    return 0;
}

void *pthread_getspecific(pthread_key_t k) {
    struct pthread_key_specific *specific;
    struct pthread_key *key;
    pthread_t thread;
    int res;
    int index;
    
    // Get key
    res = list_get(&key_list, k, (void **)&key);
    if (res) {
        return NULL;
    }
    
    // Get specific value
    thread = pthread_self();

    index = list_first(&key->specific);
    while (index >= 0) {
        list_get(&key->specific, index, (void **)&specific);
                
        if (specific->thread == thread) {
            return (void *)specific->value;
        }
        
        index = list_next(&key->specific, index);
    }
    
    return NULL;
}

int pthread_key_delete(pthread_key_t k) {
    struct pthread_key *key;
    int res;

    // Get key
    res = list_get(&key_list, k, (void **)&key);
    if (res) {
        errno = res;
        return res;
    }
    
    list_remove(&key_list, k, 1);

    return 0;
}
