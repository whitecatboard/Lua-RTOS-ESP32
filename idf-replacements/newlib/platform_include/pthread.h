/*
 * SPDX-FileCopyrightText: 2018-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __ESP_PLATFORM_PTHREAD_H__
#define __ESP_PLATFORM_PTHREAD_H__

#include <sys/types.h>
#include <sys/time.h>
#include <sys/features.h>

#include_next <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

int pthread_condattr_getclock(const pthread_condattr_t * attr, clockid_t * clock_id);
int pthread_condattr_setclock(pthread_condattr_t *attr, clockid_t clock_id);

// WHITECAT
int  pthread_attr_setaffinity_np(pthread_attr_t *attr, size_t cpusetsize, const cpu_set_t *cpuset);
int  pthread_attr_getaffinity_np(const pthread_attr_t *attr, size_t cpusetsize, cpu_set_t *cpuset);
int pthread_setname_np(pthread_t id, const char *name);
int pthread_getname_np(pthread_t id, char *name, size_t len);
int  pthread_attr_setinitialstate_np(pthread_attr_t *attr, int initial_state);
int pthread_attr_setinitfunc_np(pthread_attr_t *attr, void (*init_func)(void *));

#ifdef __cplusplus
}
#endif

#endif // __ESP_PLATFORM_PTHREAD_H__
