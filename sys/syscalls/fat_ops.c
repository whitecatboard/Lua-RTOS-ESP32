/*
 * Lua RTOS, FAT file system operations
 *
 * Copyright (C) 2015 - 2016
 * IBEROXARXA SERVICIOS INTEGRALES, S.L. & CSS IBÉRICA, S.L.
 * 
 * Author: Jaume Olivé (jolive@iberoxarxa.com / jolive@whitecatboard.org)
 * 
 * All rights reserved.  
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appear in all
 * copies and that both that the copyright notice and this
 * permission notice and warranty disclaimer appear in supporting
 * documentation, and that the name of the author not be used in
 * advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * The author disclaim all warranties with regard to this
 * software, including all implied warranties of merchantability
 * and fitness.  In no event shall the author be liable for any
 * special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether
 * in an action of contract, negligence or other tortious action,
 * arising out of or in connection with the use or performance of
 * this software.
 */

#if USE_FAT

#include <sys/fat/ff.h>

#include <limits.h>

#include <string.h>
#include <ctype.h>
#include <unistd.h> 
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/filedesc.h>
#include <sys/uio.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/syslog.h>

#include <sys/drivers/cpu.h>
#include <sys/drivers/sd.h>

#define	EACCESS	5 // Permission denied

static FATFS sd_fs[NSD];


int fat_init() {
    syslog(LOG_INFO, "fat init file system");
            
    if (!sd_has_partitions(0)) {
        syslog(LOG_ERR, "fat sdcard is not partitioned");
        return -1;
    }

    if (
            !sd_has_partition(0, 0x01) && sd_has_partition(0, 0x04) && 
            !sd_has_partition(0, 0x06) && !sd_has_partition(0, 0x0b) && 
            !sd_has_partition(0, 0x0e) && !sd_has_partition(0, 0x0c)
        ) {
        syslog(LOG_ERR, "fat hasn't a FAT3 partition");
        return -1;
    }

    if (f_mount(&sd_fs[0], "", 1) != FR_OK) {
        syslog(LOG_ERR, "fat can't mount filesystem");
        return -1;
    }

    return 1;
}

int fat_result(FRESULT res) {
    switch (res) {
        case FR_IS_DIR:
            return EISDIR;
            
        case FR_OK:
            return 0;
        
        case FR_NO_FILE:
        case FR_NO_PATH:
        case FR_INVALID_NAME:
            return ENOENT;
            
        case FR_INVALID_DRIVE:
        case FR_WRITE_PROTECTED:
        case FR_DENIED:
            return EACCESS;
            
        case FR_EXIST:
            return EEXIST;
            
        default:
            return res;
    }
}

int fat_open(struct file *fp, int flags) {
    int mode = 0;
    FIL *FP;
    char *path = fp->f_path;
    int result;

    // Calculate open mode
    if (flags == O_RDONLY)
        mode |= FA_READ;

    if (flags & O_WRONLY)
        mode |= FA_WRITE;
 
    if (flags & O_RDWR) {
        mode |= FA_READ;
        mode |= FA_WRITE;
    }
    
    if (flags & O_EXCL) {
        if (mode & FA_READ)
            mode |= FA_OPEN_EXISTING;

        if (mode & FA_WRITE)
            mode |= FA_CREATE_NEW;
    }
    
    if (flags & O_CREAT) {
        mode |= FA_OPEN_ALWAYS;
    }

    if (flags & O_TRUNC) {
        mode |= FA_CREATE_ALWAYS;
    }
    
    // Create a FIL structure
    FP = (FIL *)malloc(sizeof(FIL));
    if (!FP) {
        return ENOMEM;
    }
    
    // Store FIL into file
    fp->f_fs = FP;
    fp->f_type = DTYPE_VNODE;
    fp->f_dir = NULL;
    
    // Open file
    result = f_open(FP, path, mode);   

    result = fat_result(result);
    if ((result == EISDIR) && !(flags & O_RDONLY)) {
        result = 0;
    }
    
    return result;
}

int fat_close(struct file *fp) {
    FRESULT res;

    if (fp->f_fs) {
        res = f_close((FIL *)fp->f_fs);
        if (res != 0) {
            return fat_result(res);
        }
    }
    
    if (fp->f_dir) {
        res = f_closedir((DIR *)fp->f_dir);
        if (res != 0) {
            return fat_result(res);
        }
    }
    
    return fat_result(res);
}

int fat_read(struct file *fp, struct uio *uio) {
    FRESULT res;
    char *buf = uio->uio_iov->iov_base;
    unsigned int size = uio->uio_iov->iov_len;
    unsigned int br = 0;

    res = f_read((FIL *)fp->f_fs, buf, size, &br);
    if (res == FR_OK) {
        uio->uio_resid = uio->uio_resid - br;
    }

    return fat_result(res);
}

int fat_write(struct file *fp, struct uio *uio) {
    FRESULT res;
    char *buf = uio->uio_iov->iov_base;
    unsigned int size = uio->uio_iov->iov_len;
    unsigned int bw = 0;
    
    res = f_write((FIL *)fp->f_fs, buf, size, &bw);
    if (res == FR_OK) {
        uio->uio_resid = uio->uio_resid - bw;
    }
    
    if (res == FR_OK) {
        res = f_sync((FIL *)fp->f_fs);
    }
    
    return fat_result(res); 
}

int fat_stat(struct file *fp, struct stat *sb) {
    FRESULT res;
    FILINFO fno;

    fno.lfname = NULL;
    fno.lfsize = 0;

    res = f_stat(fp->f_path, &fno);
    if (res == FR_OK) {
    	sb->st_size = fno.fsize;
    } else {
        sb->st_size = 0;
    }

    sb->st_blksize = 512;

    if (fno.fattrib & AM_DIR) {
        sb->st_mode = S_IFDIR;
    } 

    if ((fno.fattrib & AM_LFN) || (fno.fattrib & AM_ARC)) {
        sb->st_mode = S_IFREG;
    }    

    // Update modified time
    //struct tm info;
    
    //info.tm_year = ((fno.fdate & 0b1111111000000000) >> 9) + 80;
    //info.tm_mon = ((fno.fdate & 0b0000000111100000) >> 5) - 1;
    //info.tm_mday = (fno.fdate & 0b0000000000011111);
    
    //info.tm_sec = (fno.ftime & 0b0000000000011111) * 2;
    //info.tm_min = (fno.ftime & 0b0000011111100000) >> 5;
    //info.tm_hour = (fno.ftime & 0b1111100000000000) >> 11;
    
    //sb->st_atimespec.ts_sec = mktime(&info);
    //sb->st_atimespec.ts_nsec = 0;
    
    //sb->st_ctimespec = sb->st_atimespec;
    //sb->st_mtimespec = sb->st_atimespec;
            
    return fat_result(res);
}

off_t fat_seek(struct file *fp, off_t offset, int where) {
    FRESULT res;
    long long off = offset;
    
    switch (where) {
        case SEEK_CUR: off = ((FIL *)fp->f_fs)->fptr + offset;break;
        case SEEK_END: off = ((FIL *)fp->f_fs)->fsize - offset;break;
    }
    
    res = f_lseek(((FIL *)fp->f_fs), off);
    
    return fat_result(res);
}


/*


int fat_eof(struct open_file *f, int *eof) {
    FIL *fp;
    
    fp = f->f_fsdata;
    
    *eof = ((int)((fp)->fptr == (fp)->fsize)); 
    
    return 0;
}
*/

int fat_rename(const char *old_filename, const char *new_filename) {
    return fat_result(f_rename(old_filename, new_filename));
}

int fat_mkdir(const char *pathname) {
    return fat_result(f_mkdir(pathname));
}

int fat_unlink(struct file *fp) {
    return fat_result(f_unlink(fp->f_path));
}

int fat_getcwd(char *pt, size_t size) {
    return fat_result(f_getcwd(pt,size));
}

int fat_chdir(const char *path) {
    return fat_result(f_chdir(path));
}

int fat_opendir(struct file *fp) {
    FRESULT res;

    fp->f_dir = malloc(sizeof(DIR));
    if (!fp->f_dir) {
        return ENOMEM;
    }
    
    res = f_opendir((DIR *)fp->f_dir, fp->f_path);
    if (res != FR_OK) {
        free(fp->f_dir);
    }

    return fat_result(res);
}

int fat_readdir(struct file *fp, struct dirent *ent) {
    FRESULT res;
    FILINFO fno;
    char *fn;
    
    fno.lfname = malloc((_MAX_LFN + 1) * 2);
    if (!fno.lfname) {
        return ENOMEM;
    }
    
    fno.lfsize = (_MAX_LFN + 1) * 2;

    *ent->d_name = '\0';
    for(;;) {
        *(fno.lfname) = '\0';
        
        // Read directory
        res = f_readdir((DIR *)fp->f_dir, &fno);

        // Break condition
        if (res != FR_OK || fno.fname[0] == 0) {
            break;
        }            

        if (fno.fname[0] == '.') {
            if (fno.fname[1] == '.') {
                if (!fno.fname[2]) {
                    continue;
                }
            } 

            if (!fno.fname[1]) {
                continue;
            } 
        }
        
        if (fno.fattrib & (AM_HID | AM_SYS | AM_VOL)) {
            continue;
        }
        
        // Get name
        if (*(fno.lfname)) {
            fn = fno.lfname;
        } else {
            fn = fno.fname;
        }
        
        ent->d_type = 0;

        if (fno.fattrib & AM_DIR) {
            ent->d_type = DT_DIR;
            ent->d_reclen = 0;
        }

        if (fno.fattrib & AM_ARC) {
            ent->d_type = DT_REG;
            ent->d_reclen = fno.fsize;
        }
        
        ent->d_namlen = strlen(fn);

        if (!ent->d_type) {
            continue;
        }
        
        strcpy(ent->d_name, fn);
        

    break;
    }
    
    free(fno.lfname);

    return fat_result(res);
}

int fat_format() {
    FRESULT res;
    
    res = f_mkfs("", 0, 512);
    if (res == FR_NOT_ENABLED) {
        res = f_mount(&sd_fs[0], "", 1);
        if (res == FR_OK) {
            res = f_mkfs("", 0, 512);
        }
    }
    
    res = f_mount(&sd_fs[0], "", 1);
    
    if (res == FR_OK) {
       cpu_reset();
    }
    
    return res;
}

#endif

