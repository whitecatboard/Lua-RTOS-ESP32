#ifndef _LUA_RTOS_SYS_CPU_SET_H_
#define _LUA_RTOS_SYS_CPU_SET_H_

#include <stdint.h>

#define CPU_INITIALIZER 0

typedef int32_t cpu_set_t;

// Add CPU cpu to set
#define CPU_SET(ncpu, cpuset) \
  *(cpuset) |= (1 << ncpu)

// Test to see if CPU cpu is a member of set.
#define CPU_ISSET(ncpu, cpuset) \
  (*(cpuset) & (1 << ncpu))

#endif
