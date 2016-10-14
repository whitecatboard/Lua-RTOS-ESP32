#include "syscalls.h"
#include <sys/syscalls/mount.h>

static char currdir[MAXPATHLEN] = "/";
static char currdev[4] = "";

extern struct file *getfile(int fd);

/*
 * Get filesystem statistics.
 */
/* ARGSUSED */
int
fstatfs(int fd, struct statfs *buf) {
    bzero(buf, sizeof(struct statfs));
    
    return 0;
}

int chdir(const char *path) {
    struct stat statb;
	struct file *fp;

    int fd;
    
    if (strlen(path) > MAXPATHLEN) {
        errno = ENAMETOOLONG;
        return -1;
    }
    // Check for path existence
    if ((fd = open(path, O_RDONLY)) == -1)
            return -1;
    
    // Check that path is a directory
    if (fstat(fd, &statb) || !S_ISDIR(statb.st_mode)) {
            errno = ENOTDIR;
            close(fd);
            return -1;
    }

    if (!(fp = getfile(fd))) {
        return -1;
    }
    
    strcpy(currdev, fp->f_devname);
    strcpy(currdir, fp->f_path);

    close(fd);

    return 0;
}

int access(const char *pathname, int mode) {
    return 0;
}

int fchdir(int fd) {
    struct file *fp;
    
    if (!(fp = getfile(fd))) {
        return -1;
    }
    
    return chdir(fp->f_path);            
}

void __getcwd(char *pt, size_t size) {
    if ((strcmp(currdev, mount_default_device()) != 0) && (strlen(currdev) > 0)) {
        strcpy(pt, "/");
        strcat(pt, currdev);
        if ((strcmp(currdir,"/") != 0) && (strlen(currdir) > 0)) {
            if (*currdir != '/') {
                strcat(pt, "/");
            }
            strcat(pt, currdir);            
        }
    } else {       
        bcopy(currdir, pt, size);
    }
}
