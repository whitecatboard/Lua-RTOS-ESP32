#include "_pthread.h"

#include <errno.h>
 #include <signal.h>

int pthread_kill(pthread_t thread, int signal) {
    if (signal > PTHREAD_NSIG) {
    	return EINVAL;
    }

    _signal_queue(thread, signal);

	return 0;
}
