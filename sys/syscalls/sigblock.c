#include "syscalls.h"

#include <signal.h>

int sigvec(int sig, struct sigvec *vec, struct sigvec *ovec) {
    return 0;
}

int sigblock(int mask) {
    return 0;
}

int sigsetmask(int mask) {
    return 0;
}

int siggetmask(void) {
    return 0;
}


