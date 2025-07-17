#ifndef _LUA_RTOS_SYS_SCHED_H_
#define _LUA_RTOS_SYS_SCHED_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/cpu_set.h>

struct sched_param {
  int sched_priority;     /* Process execution scheduling priority */

  #if LUA_RTOS_INCLUDE_IDF_REPLACEMENTS
  cpu_set_t affinityset;
  int initial_state;
  #endif
};

#include_next <sched.h>

#ifdef __cplusplus
}
#endif

#endif
