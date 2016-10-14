#include "syscalls.h"
#include <stdarg.h>

extern const struct filedesc *p_fd;

int read(int fd, void *buf, size_t nbyte) {
    register struct file *fp;
    const struct filedesc *fdp = p_fd;
    struct uio auio;
    struct iovec aiov;
    long cnt, error = 0;

    mtx_lock(&fd_mtx);
    if (((u_int)fd) >= fdp->fd_nfiles ||
        (fp = fdp->fd_ofiles[fd]) == NULL ||
        (fp->f_flag & FREAD) == 0) {
        mtx_unlock(&fd_mtx);
        errno = EBADF;
        return -1;
    }
    mtx_unlock(&fd_mtx);

    aiov.iov_base = (caddr_t)buf;
    aiov.iov_len = nbyte;
    auio.uio_iov = &aiov;
    auio.uio_iovcnt = 1;
    auio.uio_resid = nbyte;
    auio.uio_rw = UIO_READ;

    cnt = nbyte;
    error = (*fp->f_ops->fo_read)(fp, &auio, NULL);
    if (error && auio.uio_resid != cnt &&
        (error == ERESTART || error == EINTR || error == EWOULDBLOCK))
        error = 0;

    cnt -= auio.uio_resid;
    
    if (error) {
        errno = error;
        return -1;
    }
    
    return cnt;
}

int write(int fd, const void *buf, size_t nbyte) {
    register struct file *fp;
    const struct filedesc *fdp = p_fd;
    struct uio auio;
    struct iovec aiov;
    long cnt, error = 0;

    mtx_lock(&fd_mtx);
    if (((u_int)fd) >= fdp->fd_nfiles ||
        (fp = fdp->fd_ofiles[fd]) == NULL ||
        (fp->f_flag & FWRITE) == 0) {
        mtx_unlock(&fd_mtx);
        errno = EBADF;
        return -1;
    }
    mtx_unlock(&fd_mtx);

    aiov.iov_base = (caddr_t)buf;
    aiov.iov_len = nbyte;
    auio.uio_iov = &aiov;
    auio.uio_iovcnt = 1;
    auio.uio_resid = nbyte;
    auio.uio_rw = UIO_WRITE;

    cnt = nbyte;

    error = (*fp->f_ops->fo_write)(fp, &auio, NULL);
    if (error) {
        if (auio.uio_resid != cnt && (error == ERESTART ||
            error == EINTR || error == EWOULDBLOCK))
            error = 0;
     //   if (error == EPIPE)
//TO DO
            //            psignal(p, SIGPIPE);
    }
    cnt -= auio.uio_resid;
    
    if (error) {
        errno = error;
        return -1;
    }
    
    return cnt;
}
    
/*
 * Gather write system call
 */
ssize_t writev(int fd, struct iovec *iovp, int iovcnt) {
    register struct file *fp;
    const struct filedesc *fdp = p_fd;
    struct uio auio;
    register struct iovec *iov;
    struct iovec *needfree;
    struct iovec aiov[UIO_SMALLIOV];
    long i, cnt = 0, error = 0;
    u_int iovlen;
#ifdef KTRACE
    struct iovec *ktriov = NULL;
#endif

    mtx_lock(&fd_mtx);
    if (fd >= fdp->fd_nfiles ||
        (fp = fdp->fd_ofiles[fd]) == NULL ||
        (fp->f_flag & FWRITE) == 0) {
        mtx_unlock(&fd_mtx);
        errno = EBADF;
        return -1;
    }
    mtx_unlock(&fd_mtx);
 
    /* note: can't use iovlen until iovcnt is validated */
    iovlen = iovcnt * sizeof (struct iovec);
    if (iovcnt > UIO_SMALLIOV) {
        if (iovcnt > UIO_MAXIOV) {
            errno = EINVAL;
            return -1;
        }

        MALLOC(iov, struct iovec *, iovlen, M_IOV, M_WAITOK);
        needfree = iov;
    } else {
        iov = aiov;
        needfree = NULL;
    }

    auio.uio_iov = iov;
    auio.uio_iovcnt = iovcnt;
    auio.uio_rw = UIO_WRITE;
    
    bcopy(iovp, iov, iovlen);

    auio.uio_resid = 0;
    for (i = 0; i < iovcnt; i++) {
        if (auio.uio_resid + iov->iov_len < auio.uio_resid) {
            error = EINVAL;
            goto done;
        }
        auio.uio_resid += iov->iov_len;
        iov++;
    }

#ifdef KTRACE
    /*
     * if tracing, save a copy of iovec
     */
    if (KTRPOINT(p, KTR_GENIO))  {
        MALLOC(ktriov, struct iovec *, iovlen, M_TEMP, M_WAITOK);
        bcopy((caddr_t)auio.uio_iov, (caddr_t)ktriov, iovlen);
    }
#endif
    cnt = auio.uio_resid;
    error = (*fp->f_ops->fo_write)(fp, &auio, NULL);
    if (error) {
        if (auio.uio_resid != cnt && (error == ERESTART ||
            error == EINTR || error == EWOULDBLOCK))
            error = 0;
    }
    
    cnt -= auio.uio_resid;

#ifdef KTRACE
    if (ktriov != NULL) {
        if (error == 0)
            ktrgenio(p->p_tracep, SCARG(uap, fd), UIO_WRITE,
                ktriov, cnt, error);
        FREE(ktriov, M_TEMP);
    }
#endif

done:
    if (needfree)
        FREE(needfree, M_IOV);

    if (error) {
        errno = error;
        return -1;
    }

    return (cnt);
}