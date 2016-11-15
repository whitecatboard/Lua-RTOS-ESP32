#ifndef __FILE_H
#define __FILE_H

#include <sys/fcntl.h>
#include <sys/unistd.h>
#include <sys/stat.h>

#include <sys/queue.h>

struct file {
	LIST_ENTRY(file) f_list; /* list of active files */
    short   f_count;         /* reference count */
    caddr_t f_data;          /* vnode or socket */
    short   f_flag;          /* see fcntl.h */
    void   *f_fs;			 /* this is a reference to the file system's file struct that this file refers */
    char   *f_path;          /* this is the path of the opended file */
    void   *f_dir;			 /* this is a reference to the file systems's dir struct that this file refers */
    int     f_fd;
    int     f_fs_type;       /* type of files ystem */
};

LIST_HEAD(filelist, file);
extern struct filelist filehead;    /* head of list of open files */

#endif
