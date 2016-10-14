#include <sys/types.h>
#include <signal.h>

typedef struct {
        char    *ss_sp;
        size_t  ss_size;
        int     ss_flags;
} stack_t;

int
sigaltstack(const stack_t *ss, stack_t *oss) {
    return 0;
}
