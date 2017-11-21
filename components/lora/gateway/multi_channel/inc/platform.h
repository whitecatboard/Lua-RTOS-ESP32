#ifndef _LIBLORAGW_PLATFORM_H_
#define _LIBLORAGW_PLATFORM_H_

#include <time.h>

#define CLOCK_MONOTONIC 1

int clock_gettime(clockid_t clk_id, struct timespec *tp);


#endif /* _LIBLORAGW_PLATFORM_H_ */
