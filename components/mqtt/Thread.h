/*******************************************************************************
 * Copyright (c) 2009, 2014 IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution. 
 *
 * The Eclipse Public License is available at 
 *    http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at 
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Ian Craggs - initial implementation
 *    Ian Craggs, Allan Stockdill-Mander - async client updates
 *    Ian Craggs - fix for bug #420851
 *******************************************************************************/

#ifndef THREAD_H
#define THREAD_H


#include "arch/sys_arch.h"
#include "arch/cc.h"

#include <pthread/pthread.h>

#undef AF_INET6

#define thread_type pthread_t
#define thread_return_type void*

typedef unsigned long thread_id_type;

typedef thread_return_type (*thread_fn)(void *pvParameters);

#define mutex_type pthread_mutex_t*

typedef struct { pthread_cond_t cond; pthread_mutex_t mutex; } cond_type_struct;
//typedef cond_type_struct *cond_type;

#define cond_type void*

typedef sys_sem_t *sem_type;

cond_type Thread_create_cond();
int Thread_signal_cond(cond_type);
int Thread_wait_cond(cond_type condvar, int timeout);
int Thread_destroy_cond(cond_type);

thread_type Thread_start(thread_fn, void*);

mutex_type Thread_create_mutex();
int Thread_lock_mutex(mutex_type);
int Thread_unlock_mutex(mutex_type);
void Thread_destroy_mutex(mutex_type);

thread_id_type Thread_getid();

sem_type Thread_create_sem();
int Thread_wait_sem(sem_type sem, int timeout);
int Thread_check_sem(sem_type sem);
int Thread_post_sem(sem_type sem);
int Thread_destroy_sem(sem_type sem);

#endif
