#include "FreeRTOS.h"
#include "task.h"

#include "syscalls.h"

pid_t getpid(void) {
    return (pid_t)uxTaskGetTaskNumber(NULL);
}

/*
 * Allocate a zeroed cred structure.
 */
struct ucred *
crget()
{
    register struct ucred *cr;

    MALLOC(cr, struct ucred *, sizeof(*cr), M_CRED, M_WAITOK);
    bzero((caddr_t)cr, sizeof(*cr));
    cr->cr_ref = 1;
    return (cr);
}

/*
 * Free a cred structure.
 * Throws away space when ref count gets to 0.
 */
void
crfree(cr)
    struct ucred *cr;
{
	portDISABLE_INTERRUPTS();
    if (--cr->cr_ref == 0)
        FREE((caddr_t)cr, M_CRED);
	portENABLE_INTERRUPTS();
}

/*
 * Copy cred structure to a new one and free the old one.
 */
struct ucred *
crcopy(cr)
    struct ucred *cr;
{
    struct ucred *newcr;

    if (cr->cr_ref == 1)
        return (cr);
    newcr = crget();
    *newcr = *cr;
    crfree(cr);
    newcr->cr_ref = 1;
    return (newcr);
}

/*
 * Dup cred struct to a new held one.
 */
struct ucred *
crdup(cr)
    struct ucred *cr;
{
    struct ucred *newcr;

    newcr = crget();
    *newcr = *cr;
    newcr->cr_ref = 1;
    return (newcr);
}
