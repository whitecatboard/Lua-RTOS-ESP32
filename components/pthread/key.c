/*
 * Copyright (C) 2015 - 2018, IBEROXARXA SERVICIOS INTEGRALES, S.L.
 * Copyright (C) 2015 - 2018, Jaume Olivé Petrus (jolive@whitecatboard.org)
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the <organization> nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *     * The WHITECAT logotype cannot be changed, you can remove it, but you
 *       cannot change it in any way. The WHITECAT logotype is:
 *
 *          /\       /\
 *         /  \_____/  \
 *        /_____________\
 *        W H I T E C A T
 *
 *     * Redistributions in binary form must retain all copyright notices printed
 *       to any local or remote output device. This include any reference to
 *       Lua RTOS, whitecatboard.org, Lua, and other copyright notices that may
 *       appear in the future.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Lua RTOS pthread implementation for FreeRTOS
 *
 */

#include "_pthread.h"

#include <stdlib.h>

extern struct list key_list;

int pthread_key_create(pthread_key_t *k, void (*destructor)(void*)) {
    struct pthread_key *key;
    int res;

    // Allocate space for the key
    key = (struct pthread_key *)malloc(sizeof(struct pthread_key));
    if (!key) {
        return ENOMEM;
    }
    
    // Init key
    key->destructor = destructor;
    
    lstinit(&key->specific, 1, LIST_DEFAULT);
    
    // Add key to key list
    res = lstadd(&key_list, (void *)key, (int*)k);
    if (res) {
        free(key);
        return res;
    }

    return 0;
}

int pthread_setspecific(pthread_key_t k, const void *value) {
    struct pthread_key_specific *specific;
    struct pthread_key *key;
    pthread_t thread;
    int res;
	int index;

    // Get key
    res = lstget(&key_list, k, (void **)&key);
    if (res) {
        return res;
    }

    if (value) {
        // Allocate space for specific
        specific = (struct pthread_key_specific *)malloc(sizeof(struct pthread_key_specific));
        if (!specific) {
            return ENOMEM;
        }

        specific->thread = pthread_self();
        specific->value = value;

        lstadd(&key->specific, (void **)specific, &index);
    } else {
        thread = pthread_self();

        index = lstfirst(&key->specific);
        while (index >= 0) {
            lstget(&key->specific, index, (void **)&specific);

            if (specific->thread == thread) {
            	lstremove(&key->specific, k, 1);
            	break;
            }

            index = lstnext(&key->specific, index);
        }
    }

    return 0;
}

void *pthread_getspecific(pthread_key_t k) {
    struct pthread_key_specific *specific;
    struct pthread_key *key;
    pthread_t thread;
    int res;
    int index;

    // Get key
    res = lstget(&key_list, k, (void **)&key);
    if (res) {
        return NULL;
    }
    
    // Get specific value
    thread = pthread_self();

    index = lstfirst(&key->specific);
    while (index >= 0) {
        lstget(&key->specific, index, (void **)&specific);
                
        if (specific->thread == thread) {
            return (void *)specific->value;
        }
        
        index = lstnext(&key->specific, index);
    }
    
    return NULL;
}

int pthread_key_delete(pthread_key_t k) {
    struct pthread_key *key;
    int res;

    // Get key
    res = lstget(&key_list, k, (void **)&key);
    if (res) {
        return res;
    }
    
    lstremove(&key_list, k, 1);

    return 0;
}
