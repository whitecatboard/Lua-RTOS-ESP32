/* 
 * File:   syscalls.h
 * Author: jolive
 *
 * Created on 11 / octubre / 2015, 20:11
 */

#ifndef SYSCALLS_H
#define	SYSCALLS_H

#include <limits.h>

#include <sys/cdefs.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <sys/filedesc.h>
#include <sys/uio.h>
#include <sys/file.h>
#include <sys/dirent.h>
#include <sys/syscalls/device.h>
#include <sys/mutex.h>
#include <sys/mount.h>

#include <reent.h>

typedef int32_t register_t;

/* pseudo-errors returned inside kernel to modify return to process */
#define ERESTART        -1      /* restart syscall */
#define EJUSTRETURN     -2      /* don't modify regs, just return */

#define FFLAGS(oflags)  ((oflags) + 1)
#define OFLAGS(fflags)  ((fflags) - 1)
#define FFSYNC FSYNC
#define FNONBLOCK FNONBIO
#define FIONBIO FNONBIO
#define FMASK       (FREAD|FWRITE|FAPPEND|FASYNC|FFSYNC|FNONBLOCK)
#define FCNTLFLAGS  (FAPPEND|FASYNC|FFSYNC|FNONBLOCK)

#define min(a,b) (((a)<(b))?(a):(b))
#define max(a,b) (((a)>(b))?(a):(b))

/*
 * Types of memory to be allocated. Really we don't use this in whitecat, but
 * are defined here because are used on original syscalls implementations.
 */
#define M_FREE      0   /* should be on free list */
#define M_MBUF      1   /* mbuf */
#define M_DEVBUF    2   /* device driver memory */
#define M_SOCKET    3   /* socket structure */
#define M_PCB       4   /* protocol control block */
#define M_RTABLE    5   /* routing tables */
#define M_HTABLE    6   /* IMP host tables */
#define M_FTABLE    7   /* fragment reassembly header */
#define M_ZOMBIE    8   /* zombie proc status */
#define M_IFADDR    9   /* interface address */
#define M_SOOPTS    10  /* socket options */
#define M_SONAME    11  /* socket name */
#define M_NAMEI     12  /* namei path name buffer */
#define M_GPROF     13  /* kernel profiling buffer */
#define M_IOCTLOPS  14  /* ioctl data buffer */
#define M_MAPMEM    15  /* mapped memory descriptors */
#define M_CRED      16  /* credentials */
#define M_PGRP      17  /* process group header */
#define M_SESSION   18  /* session header */
#define M_IOV       19  /* large iov's */
#define M_MOUNT     20  /* vfs mount struct */
#define M_FHANDLE   21  /* network file handle */
#define M_NFSREQ    22  /* NFS request header */
#define M_NFSMNT    23  /* NFS mount structure */
#define M_NFSNODE   24  /* NFS vnode private part */
#define M_VNODE     25  /* Dynamically allocated vnodes */
#define M_CACHE     26  /* Dynamically allocated cache entries */
#define M_DQUOT     27  /* UFS quota entries */
#define M_UFSMNT    28  /* UFS mount structure */
#define M_SHM       29  /* SVID compatible shared memory segments */
#define M_VMMAP     30  /* VM map structures */
#define M_VMMAPENT  31  /* VM map entry structures */
#define M_VMOBJ     32  /* VM object structure */
#define M_VMOBJHASH 33  /* VM object hash structure */
#define M_VMPMAP    34  /* VM pmap */
#define M_VMPVENT   35  /* VM phys-virt mapping entry */
#define M_VMPAGER   36  /* XXX: VM pager struct */
#define M_VMPGDATA  37  /* XXX: VM pager private data */
#define M_FILE      38  /* Open file structure */
#define M_FILEDESC  39  /* Open file descriptor table */
#define M_LOCKF     40  /* Byte-range locking structures */
#define M_PROC      41  /* Proc structures */
#define M_SUBPROC   42  /* Proc sub-structures */
#define M_SEGMENT   43  /* Segment for LFS */
#define M_LFSNODE   44  /* LFS vnode private part */
#define M_FFSNODE   45  /* FFS vnode private part */
#define M_MFSNODE   46  /* MFS vnode private part */
#define M_NQLEASE   47  /* Nqnfs lease */
#define M_NQMHOST   48  /* Nqnfs host address table */
#define M_NETADDR   49  /* Export host address structure */
#define M_NFSSVC    50  /* Nfs server structure */
#define M_NFSUID    51  /* Nfs uid mapping structure */
#define M_NFSD      52  /* Nfs server daemon structure */
#define M_IPMOPTS   53  /* internet multicast options */
#define M_IPMADDR   54  /* internet multicast address */
#define M_IFMADDR   55  /* link-level multicast address */
#define M_MRTABLE   56  /* multicast routing tables */
#define M_ISOFSMNT  57  /* ISOFS mount structure */
#define M_ISOFSNODE 58  /* ISOFS vnode private part */
#define M_NFSRVDESC 59  /* NFS server socket descriptor */
#define M_NFSDIROFF 60  /* NFS directory offset data */
#define M_NFSBIGFH  61  /* NFS version 3 file handle */
#define M_TTYS      62  /* allocated tty structures */
#define M_EXEC      63  /* argument lists & other mem used by exec */
#define M_TEMP      74  /* misc temporary data buffers */
#define M_LAST      75  /* Must be last type + 1 */

/*
 * Allocation memory macros. Originally this macros allocate different types
 * of variables. In whitecat there are only a heap. Type and flags are not used.
 */
#define MALLOC(space, cast, size, type, flags) \
    (space) = (cast)malloc((u_long)(size))

#define FREE(addr, type) free(addr)

// File descriptor mutex
struct mtx fd_mtx;

int tty_open(struct file *fp, int flags);
int tty_close(struct file *fp);
int tty_read(struct file *fp, struct uio *uio);
int tty_write(struct file *fp, struct uio *uio);
int tty_stat(struct file *fp, struct stat *sb);
void tty_lock();
void tty_unlock();

int fat_init();
int fat_open(struct file *fp, int flags);
int fat_close(struct file *fp);
int fat_read(struct file *fp, struct uio *uio);
int fat_write(struct file *fp, struct uio *uio);
int fat_stat(struct file *fp, struct stat *sb);
off_t fat_seek(struct file *fp, off_t offset, int where);
int fat_rename(const char *old_filename, const char *new_filename);
int fat_opendir(struct file *fp);
int fat_readdir(struct file *fp, struct dirent *ent);
int fat_unlink(struct file *fp);
int fat_mkdir(const char *pathname);
int fat_format();

int net_open(struct file *fp, int flags);
int net_close(struct file *fp);
int net_read(struct file *fp, struct uio *uio);
int net_write(struct file *fp, struct uio *uio);
int net_stat(struct file *fp, struct stat *sb);
int net_select(struct file *fp, int which);
int net_ioctl(struct file *fp, u_long com, caddr_t data);

int spiffs_open_op(struct file *fp, int flags);
int spiffs_close_op(struct file *fp);
int spiffs_read_op(struct file *fp, struct uio *uio);
int spiffs_write_op(struct file *fp, struct uio *uio);
int spiffs_stat_op(struct file *fp, struct stat *sb);
off_t spiffs_seek_op(struct file *fp, off_t offset, int where);
int spiffs_rename_op(const char *old_filename, const char *new_filename);
int spiffs_unlink_op(struct file *fp);
int spiffs_opendir_op(struct file *fp);
int spiffs_mkdir_op(const char *path);
int spiffs_readdir_op(struct file *fp, struct dirent *ent);
int spiffs_format();

struct file *getfile(int fd);
int closef(register struct file *fp);

//#define FD_SET(n, p)    ((p)->fds_bits[(n)/NFDBITS] |= (1 << ((n) % NFDBITS)))
//#define FD_CLR(n, p)    ((p)->fds_bits[(n)/NFDBITS] &= ~(1 << ((n) % NFDBITS)))
//#define FD_ISSET(n, p)  ((p)->fds_bits[(n)/NFDBITS] & (1 << ((n) % NFDBITS)))
#define FD_COPY(f, t)   bcopy(f, t, sizeof(*(f)))
//#define FD_ZERO(p)      bzero(p, sizeof(*(p)))

char *normalize_path(const char *path);

int __open(struct _reent *r, const char *path, int flags, ...);
int __read(struct _reent *r, int fd, void *buf, size_t nbyte);
int __write(struct _reent *r, int fd, const void *buf, size_t nbyte);
int __close(struct _reent *r, int fd);
int __fstat(struct _reent *r, int fd, struct stat *sb);
int __stat(struct _reent *r, const char *str, struct stat *sb);
off_t __lseek(struct _reent *r, int fd, off_t offset, int whence);
int __unlink(struct _reent *r, const char *path);
int __rename(struct _reent *r, const char *old_filename, const char *new_filename);
pid_t __getpid(struct _reent *r);
void __attribute__((__noreturn__)) __exit(int status);
clock_t __times(struct _reent *r, struct tms *ptms);
clock_t __clock();
int __gettimeofday(struct _reent *r, struct timeval *tv, void *tz);

#endif	/* SYSCALLS_H */

