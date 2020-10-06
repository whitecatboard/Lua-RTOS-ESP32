#include "platform.h"

#include <stddef.h>
#include <sys/time.h>

#ifndef CLOCK_MONOTONIC
#define CLOCK_MONOTONIC 1
#endif

int clock_gettime(clockid_t clk_id, struct timespec *tp) {
	struct timeval tv;

	switch (clk_id) {
		case CLOCK_MONOTONIC:
			gettimeofday(&tv, NULL);

			tp->tv_sec = tv.tv_sec;
			tp->tv_nsec = tv.tv_usec * 1000;
			break;
	}

	return 0;
}
