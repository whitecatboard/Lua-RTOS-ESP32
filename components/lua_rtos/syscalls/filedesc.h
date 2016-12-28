#include <sys/types.h>

#define NDFILE 10

/*
 * File system types
 */
#define FS_TTY       1
#define FS_SPIFFS    2
#define FS_FAT       3
#define FS_SOCKET    4

struct filedesc {
    struct  file **fd_ofiles;   /* file structures for open files */
    char    *fd_ofileflags;     /* per-process open file flags */
    int     fd_nfiles;          /* number of open files allocated */
    u_short fd_lastfile;        /* high-water mark of fd_ofiles */
    u_short fd_freefile;        /* approx. next free file */
    u_short fd_cmask;           /* mask for file creation */
    u_short fd_refcnt;          /* reference count */
};

/*
 * Per-process open flags.
 */
#define UF_EXCLOSE  0x01        /* auto-close on exec */
#define UF_MAPPED   0x02        /* mapped from device */

int     falloc (struct file **resultfp, int *resultfd);
struct  filedesc *fdcopy();
void    fdfree();
