#include "syscalls.h"

#include <sys/time.h>

extern struct filedesc *p_fd;

static int
selscan(ibits, obits, nfd, retval)
    fd_set *ibits, *obits;
    int nfd;
    register_t *retval;
{
    register struct filedesc *fdp = p_fd;
    register int msk, i, j, fd;
    register fd_mask bits;
    struct file *fp;
    int n = 0;
    static int flag[3] = { FREAD, FWRITE, 0 };

    for (msk = 0; msk < 3; msk++) {
        for (i = 0; i < nfd; i += NFDBITS) {
            bits = ibits[msk].fds_bits[i/NFDBITS];
            while ((j = ffs(bits)) && (fd = i + --j) < nfd) {
                bits &= ~(1 << j);
                mtx_lock(&fd_mtx);
                fp = fdp->fd_ofiles[fd];
                mtx_unlock(&fd_mtx);
                
                if (fp == NULL)
                    return (EBADF);

                if ((*fp->f_ops->fo_select)(fp, flag[msk])) {
                    FD_SET(fd, &obits[msk]);
                    n++;
                }
            }
        }
    }
   
    
    *retval = n;
    return (0);
}
    
int select(int nfds, fd_set *readfds, fd_set *writefds,
           fd_set *exceptfds, struct timeval *timeout) {

    fd_set ibits[3], obits[3];
    int error;
    int retval;
    
    if (readfds) {
        FD_COPY(readfds, &ibits[0]);
    } else {
        FD_ZERO(&ibits[0]);        
    } 

    if (writefds) {
        FD_COPY(writefds, &ibits[1]);
    } else {
        FD_ZERO(&ibits[1]);                
    }

    if (exceptfds) {
        FD_COPY(exceptfds, &ibits[2]);        
    } else {
        FD_ZERO(&ibits[2]);                        
    }
    
    FD_ZERO(&obits[0]);
    FD_ZERO(&obits[1]);
    FD_ZERO(&obits[2]);
    
    //do {
        error = selscan(ibits, obits, nfds, &retval);
        if (error != 0) {
            errno = error;
            return -1;
        }
    //} while (retval == 0);

    if (readfds) {
        FD_COPY(&obits[0], readfds);
    }
    
    if (writefds) {
        FD_COPY(&obits[1], writefds);    
    }
    
    if (exceptfds) {
        FD_COPY(&obits[2], exceptfds);    
    }

    return retval;
}
