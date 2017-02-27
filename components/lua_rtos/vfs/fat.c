/*
 * Lua RTOS, FAT vfs operations
 *
 * Copyright (C) 2015 - 2017
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

#if USE_FAT

#include "freertos/FreeRTOS.h"
#include "esp_vfs.h"
#include "esp_attr.h"

#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>

#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/syslog.h>
#include <sys/list.h>
#include <sys/fcntl.h>

#include <fat/ff.h>

#include <drivers/sd.h>

static int IRAM_ATTR vfs_fat_open(const char *path, int flags, int mode);
static size_t IRAM_ATTR vfs_fat_write(int fd, const void *data, size_t size);
static ssize_t IRAM_ATTR vfs_fat_read(int fd, void * dst, size_t size);
static int IRAM_ATTR vfs_fat_fstat(int fd, struct stat * st);
static int IRAM_ATTR vfs_fat_close(int fd);
static off_t IRAM_ATTR vfs_fat_lseek(int fd, off_t size, int mode);

typedef struct {
	DIR dir;
	FDIR fat_dir;
	char path[MAXNAMLEN + 1];
	struct dirent ent;
	uint8_t read_mount;
} vfs_fat_dir_t;

typedef struct {
	FIL fat_file;
	char path[MAXNAMLEN + 1];
	uint8_t is_dir;
} vfs_fat_file_t;

#define	EACCESS	5 // Permission denied

static FATFS sd_fs[NSD];
static struct list files;

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
	int fd;

	// Allocate new file
	vfs_fat_file_t *file = calloc(1, sizeof(vfs_fat_file_t));
	if (!file) {
		errno = ENOMEM;
		return -1;
	}

    // Add file to file list. List index is file descriptor.
    int res = list_add(&files, file, &fd);
    if (res) {
    	free(file);
    	errno = res;
    	return -1;
    }

	// Make a copy of path
	strlcpy(file->path, path, MAXNAMLEN);

    // Open file
    int fat_mode = 0;
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

    // Open FAT file
    result = f_open(&file->fat_file, path, fat_mode);
    result = fat_result(result);
    if ((result == EISDIR) && !(flags & O_RDONLY)) {
        result = 0;
        file->is_dir = 1;
    }

    if (result != 0) {
    	list_remove(&files, fd, 1);
    	errno = result;
    	return -1;
    }

    return fd;
}

static size_t IRAM_ATTR vfs_fat_write(int fd, const void *data, size_t size) {
	vfs_fat_file_t *file;
	int res;

    res = list_get(&files, fd, (void **)&file);
    if (res) {
		errno = EBADF;
		return -1;
    }

    if (file->is_dir) {
		errno = EBADF;
		return -1;
    }

    // Write FAT file
	unsigned int bw;

    res = f_write(&file->fat_file, data, size, &bw);
    if (res == FR_OK) {
    	return (size_t)bw;
    } else {
    	errno = fat_result(res);
    	return -1;
    }
}

static ssize_t IRAM_ATTR vfs_fat_read(int fd, void * dst, size_t size) {
	vfs_fat_file_t *file;
	int res;

    res = list_get(&files, fd, (void **)&file);
    if (res) {
		errno = EBADF;
		return -1;
    }

    if (file->is_dir) {
		errno = EBADF;
		return -1;
    }

    // Read FAT file
	unsigned int br;

    res = f_read(&file->fat_file, dst, size, &br);
    if (res == FR_OK) {
        return (ssize_t)br;
    } else {
    	errno = fat_result(res);
    	return -1;
    }

	return -1;
}

static int IRAM_ATTR vfs_fat_fstat(int fd, struct stat * st) {
	vfs_fat_file_t *file;
	int res;

    res = list_get(&files, fd, (void **)&file);
    if (res) {
		errno = EBADF;
		return -1;
    }

	// Set block size for this file system
    st->st_blksize = 512;

    // Get file statistics
    FILINFO fno;

    fno.lfname = NULL;
    fno.lfsize = 0;

    res = f_stat(file->path, &fno);
    if (res == FR_OK) {
    	struct tm tm_info;

    	st->st_size = fno.fsize;

    	tm_info.tm_year = (fno.fdate >> 9) + 80;	// year from 1980 -> year from 1900
    	tm_info.tm_mon = (fno.fdate >> 5 & 15) - 1;	// month 1~12 -> month 0~11
    	tm_info.tm_mday = fno.fdate & 31;
    	tm_info.tm_hour = fno.ftime >> 11;
    	tm_info.tm_min = fno.ftime >> 5 & 63;
    	tm_info.tm_sec = (fno.ftime & 31) << 1;		// second * 2
    	st->st_atime = mktime(&tm_info);
    } else {
        st->st_size = 0;

        errno = fat_result(res);
        return -1;
    }

    if (fno.fattrib & AM_DIR) {
    	st->st_mode = S_IFDIR;
    	st->st_size = 0;
    }

    if ((fno.fattrib & AM_LFN) || (fno.fattrib & AM_ARC)) {
    	st->st_mode = S_IFREG;
    }

    return 0;
}

static int IRAM_ATTR vfs_fat_close(int fd) {
	int res = 0;

	vfs_fat_file_t *file;

    res = list_get(&files, fd, (void **)&file);
    if (res) {
		errno = EBADF;
		return -1;
    }

    // Close FAT file
	res = f_close(&file->fat_file);
	if (res != 0) {
		res = fat_result(res);
	}

	if (res < 0) {
		errno = res;
		return -1;
	}

	list_remove(&files, fd, 1);

	return 0;
}

static off_t IRAM_ATTR vfs_fat_lseek(int fd, off_t size, int mode) {
    int off = size;

	vfs_fat_file_t *file;
	int res;

    res = list_get(&files, fd, (void **)&file);
    if (res) {
		errno = EBADF;
		return -1;
    }

    if (file->is_dir) {
		errno = EBADF;
		return -1;
    }

    switch (mode) {
        case SEEK_CUR:
        	off = file->fat_file.fptr + size;break;

        case SEEK_END:
        	off = file->fat_file.fsize - size;
        	f_sync(&file->fat_file);
        	break;
    }

    res = f_lseek(&file->fat_file, off);
    if (res != 0) {
    	errno = fat_result(res);
    	return -1;
    }

    return 0;
}

static int IRAM_ATTR vfs_fat_stat(const char *path, struct stat * st) {
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

static int IRAM_ATTR vfs_fat_mkdir(const char *path, mode_t mode) {
	int res;

	res = f_mkdir(path);
	if (res != 0) {
		errno = fat_result(res);
		return -1;
	}

	return 0;
}

static DIR* vfs_fat_opendir(const char* name) {
	vfs_fat_dir_t *dir = calloc(1, sizeof(vfs_fat_dir_t));
	int res;

	if (!dir) {
		errno = ENOMEM;
		return NULL;
	}

    if ((res = f_opendir(&dir->fat_dir, name)) != FR_OK) {
        free(dir);

        errno = fat_result(res);
        return NULL;
    }

	strlcpy(dir->path, name, MAXNAMLEN);

	return (DIR *)dir;
}

static struct dirent* vfs_fat_readdir(DIR* pdir) {
	vfs_fat_dir_t* dir = (vfs_fat_dir_t*) pdir;
    struct dirent *ent = &dir->ent;
	int res = 0, entries = 0;

	FILINFO fno;
	char lfname[(_MAX_LFN + 1) * 2];

    char *fn;

    fno.lfname = lfname;
    fno.lfsize = (_MAX_LFN + 1) * 2;

    // Clear current dirent
    memset(ent,0,sizeof(struct dirent));

    // If this is the first call to readdir for pdir, and
    // directory is the root path, return the mounted point if any
    if (!dir->read_mount) {
    	if (strcmp(dir->path,"/") == 0) {
    	    char mdir[PATH_MAX + 1];

    	    if (mount_readdir("fat", dir->path, 0, mdir)) {
    	    	strlcpy(ent->d_name, mdir, PATH_MAX);
    	        ent->d_type = DT_DIR;
    	        ent->d_fsize = 0;

    	        dir->read_mount = 1;

    	        return ent;
    	    }
    	}

    	dir->read_mount = 1;
    }

    for(;;) {
    	*(fno.lfname) = '\0';

        // Read directory
        res = f_readdir(&dir->fat_dir, &fno);

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
            ent->d_fsize = 0;
        }

        if (fno.fattrib & AM_ARC) {
            ent->d_type = DT_REG;
            ent->d_fsize = fno.fsize;
        }

        if (!ent->d_type) {
            continue;
        }

        strlcpy(ent->d_name, fn, MAXNAMLEN);

        entries++;

        break;
    }

    if (entries > 0) {
    	return ent;
    } else {
    	return NULL;
    }
}

static int IRAM_ATTR vfs_fat_closedir(DIR* pdir) {
	vfs_fat_dir_t* dir = (vfs_fat_dir_t*) pdir;
	int res;

	if (!pdir) {
		errno = EBADF;
		return -1;
	}

    if ((res = f_closedir(&dir->fat_dir)) != 0) {
    	errno = fat_result(res);
    	return -1;
    }

	free(dir);

    return 0;
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
        .rename = &vfs_fat_rename,
		.mkdir = &vfs_fat_mkdir,
		.opendir = &vfs_fat_opendir,
		.readdir = &vfs_fat_readdir,
		.closedir = &vfs_fat_closedir,
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

        list_init(&files, 0);

        syslog(LOG_INFO, "fat%d mounted", 0);
    } else {
    	syslog(LOG_INFO, "fat%d can't mounted", 0);
    }

}

#endif
