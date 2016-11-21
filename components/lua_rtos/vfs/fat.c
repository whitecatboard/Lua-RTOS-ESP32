/*
 * Lua RTOS, FAT vfs operations
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

#include "luartos.h"
#include "freertos/FreeRTOS.h"
#include "esp_vfs.h"
#include "esp_attr.h"

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>

#include <fat/ff.h>

#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/syslog.h>

#include <syscalls/syscalls.h>

#include <drivers/sd.h>

static int IRAM_ATTR vfs_fat_open(const char *path, int flags, int mode);
static size_t IRAM_ATTR vfs_fat_write(int fd, const void *data, size_t size);
static ssize_t IRAM_ATTR vfs_fat_read(int fd, void * dst, size_t size);
static int IRAM_ATTR vfs_fat_fstat(int fd, struct stat * st);
static int IRAM_ATTR vfs_fat_close(int fd);
static off_t IRAM_ATTR vfs_fat_lseek(int fd, off_t size, int mode);

#define	EACCESS	5 // Permission denied

static FATFS sd_fs[NSD];

/*
 * This function translate error codes from FAT to errno error codes
 *
 */
int IRAM_ATTR fat_result(FRESULT res) {
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

static int IRAM_ATTR vfs_fat_open(const char *path, int flags, int mode) {
	struct file *fp;
	int fd, error;

	// Allocate new file
    error = falloc(&fp, &fd);
    if (error) {
        errno = error;
        return -1;
    }

    fp->f_fd      = fd;
    fp->f_fs      = NULL;
    fp->f_dir     = NULL;
    fp->f_path 	  = NULL;
    fp->f_fs_type = FS_FAT;
    fp->f_flag    = FFLAGS(flags) & FMASK;

    // Open file
    int fat_mode = 0;
    FIL *FP;
    char *path_copy;
    int result = 0;

    // Translate flags to FAT flags
    if (flags == O_RDONLY)
    	fat_mode |= FA_READ;

    if (flags & O_WRONLY)
    	fat_mode |= FA_WRITE;

    if (flags & O_RDWR) {
    	fat_mode |= FA_READ;
    	fat_mode |= FA_WRITE;
    }

    if (flags & O_EXCL) {
        if (fat_mode & FA_READ)
        	fat_mode |= FA_OPEN_EXISTING;

        if (fat_mode & FA_WRITE)
        	fat_mode |= FA_CREATE_NEW;
    }

    if (flags & O_CREAT) {
    	fat_mode |= FA_OPEN_ALWAYS;
    }

    if (flags & O_TRUNC) {
    	fat_mode |= FA_CREATE_ALWAYS;
    }

    // Create a FAT FIL structure
    FP = (FIL *)malloc(sizeof(FIL));
    if (!FP) {
        return ENOMEM;
    }

    // Store FAT file
    fp->f_fs = FP;

    // Create a copy for path
    path_copy = (char *)malloc(strlen(path) + 1);
    if (!path_copy) {
    	closef(fp);
        errno = ENOMEM;
        return -1;
    }

    strcpy(path_copy, path);

    // Store a copy of file path
    fp->f_path = path_copy;

    // Open FAT file
    result = f_open(FP, path, fat_mode);
    result = fat_result(result);
    if ((result == EISDIR) && !(flags & O_RDONLY)) {
        result = 0;
    }

    if (result != 0) {
    	closef(fp);
    	errno = result;
    	return -1;
    }

    return fd;
}

static size_t IRAM_ATTR vfs_fat_write(int fd, const void *data, size_t size) {
	struct file *fp;

	// Get file from file descriptor
	if (!(fp = get_file(fd))) {
		errno = EBADF;
		return -1;
	}

    // Write FAT file
	int res;
	unsigned int bw;

    res = f_write((FIL *)fp->f_fs, data, size, &bw);
    if (res == FR_OK) {
//        if (res == FR_OK) {
  //          res = f_sync((FIL *)fp->f_fs);
    //    }

    	return (size_t)bw;
    } else {
    	errno = fat_result(res);
    	return -1;
    }
}

static ssize_t IRAM_ATTR vfs_fat_read(int fd, void * dst, size_t size) {
	struct file *fp;

	// Get file from file descriptor
	if (!(fp = get_file(fd))) {
		errno = EBADF;
		return -1;
	}

    // Read FAT file
	int res;
	unsigned int br;

    res = f_read((FIL *)fp->f_fs, dst, size, &br);
    if (res == FR_OK) {
        return (ssize_t)br;
    } else {
    	errno = fat_result(res);
    	return -1;
    }

	return -1;
}

static int IRAM_ATTR vfs_fat_fstat(int fd, struct stat * st) {
	struct file *fp;
    int res;

    // Get file from file descriptor
	if (!(fp = get_file(fd))) {
		errno = EBADF;
		return -1;
	}

	// Set block size for this file system
    st->st_blksize = 512;

    // Get file statistics
    FILINFO fno;

    fno.lfname = NULL;
    fno.lfsize = 0;

    res = f_stat(fp->f_path, &fno);
    if (res == FR_OK) {
    	st->st_size = fno.fsize;
    } else {
        st->st_size = 0;

        errno = fat_result(res);
        return -1;
    }

    if (fno.fattrib & AM_DIR) {
    	st->st_mode = S_IFDIR;
    }

    if ((fno.fattrib & AM_LFN) || (fno.fattrib & AM_ARC)) {
    	st->st_mode = S_IFREG;
    }

    return 0;
}

static int IRAM_ATTR vfs_fat_close(int fd) {
	struct file *fp;
	int res = 0;
	int res_file = 0;
	int res_dir = 0;

	// Get file from file descriptor
	if (!(fp = get_file(fd))) {
		errno = EBADF;
		return -1;
	}

    // Close FAT file, if needed
    if (fp->f_fs) {
        res = f_close((FIL *)fp->f_fs);
        if (res != 0) {
        	res_file = fat_result(res);
        }
    }

	// Close FAT directory, if needed
    if (fp->f_dir) {
        res = f_closedir((FDIR *)fp->f_dir);
        if (res != 0) {
        	res_dir = fat_result(res);
        }
    }

	closef(fp);

	if (res_file < 0) {
		errno = res_file;
		return -1;
	}

	if (res_dir < 0) {
		errno = res_dir;
		return -1;
	}

	return 0;
}

static off_t IRAM_ATTR vfs_fat_lseek(int fd, off_t size, int mode) {
	struct file *fp;
	int res;
    int off = size;

	// Get file from file descriptor
	if (!(fp = get_file(fd))) {
		errno = EBADF;
		return -1;
	}

    switch (mode) {
        case SEEK_CUR: off = ((FIL *)fp->f_fs)->fptr + size;break;
        case SEEK_END: off = ((FIL *)fp->f_fs)->fsize - size;break;
    }

    res = f_lseek(((FIL *)fp->f_fs), off);
    if (res != 0) {
    	errno = fat_result(res);
    	return -1;
    }

    return 0;
}

static int IRAM_ATTR vfs_fat_stat(const char * path, struct stat * st) {
	int fd;
	int res;

	fd = vfs_fat_open(path, 0, 0);
	res = vfs_fat_fstat(fd, st);
	vfs_fat_close(fd);

	return res;
}

static int IRAM_ATTR vfs_fat_unlink(const char *path) {
    return fat_result(f_unlink(path));
}

static int IRAM_ATTR vfs_fat_rename(const char *src, const char *dst) {
    return fat_result(f_rename(src, dst));
}

void vfs_fat_register() {
    esp_vfs_t vfs = {
        .fd_offset = 0,
        .flags = ESP_VFS_FLAG_DEFAULT,
        .write = &vfs_fat_write,
        .open = &vfs_fat_open,
        .fstat = &vfs_fat_fstat,
        .close = &vfs_fat_close,
        .read = &vfs_fat_read,
        .lseek = &vfs_fat_lseek,
        .stat = &vfs_fat_stat,
        .link = NULL,
        .unlink = &vfs_fat_unlink,
        .rename = &vfs_fat_rename
    };
	
    ESP_ERROR_CHECK(esp_vfs_register("/fat", &vfs, NULL));

    // Mount sdcard
    if (sd_init(0)) {
        syslog(LOG_INFO, "fat init file system");

        if (!sd_has_partitions(0)) {
            syslog(LOG_ERR, "fat sdcard is not partitioned");
            return;
        }

        if (
                !sd_has_partition(0, 0x01) &&  sd_has_partition(0, 0x04) &&
                !sd_has_partition(0, 0x06) && !sd_has_partition(0, 0x0b) &&
                !sd_has_partition(0, 0x0e) && !sd_has_partition(0, 0x0c)
            ) {
            syslog(LOG_ERR, "fat hasn't a FAT32 partition");
            return;
        }

        // Mount FAT
        if (f_mount(&sd_fs[0], "", 1) != FR_OK) {
            syslog(LOG_ERR, "fat can't mount filesystem");
            return;
        }

    	mount_set_mounted("fat", 1);

        syslog(LOG_INFO, "fat%d mounted", 0);
    } else {
    	syslog(LOG_INFO, "fat%d can't mounted", 0);
    }

}
