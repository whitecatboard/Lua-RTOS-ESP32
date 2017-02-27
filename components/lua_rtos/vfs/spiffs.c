/*
 * Lua RTOS, spiffs vfs operations
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

#if USE_SPIFFS

#include <freertos/FreeRTOS.h>

#include <string.h>
#include <stdio.h>
#include <limits.h>

#include <sys/stat.h>

#include "esp_vfs.h"
#include "esp_attr.h"
#include <errno.h>

#include <spiffs.h>
#include <esp_spiffs.h>
#include <spiffs_nucleus.h>
#include <sys/syslog.h>
#include <sys/mount.h>
#include <sys/list.h>
#include <sys/fcntl.h>
#include <dirent.h>

static int IRAM_ATTR vfs_spiffs_open(const char *path, int flags, int mode);
static size_t IRAM_ATTR vfs_spiffs_write(int fd, const void *data, size_t size);
static ssize_t IRAM_ATTR vfs_spiffs_read(int fd, void * dst, size_t size);
static int IRAM_ATTR vfs_spiffs_fstat(int fd, struct stat * st);
static int IRAM_ATTR vfs_spiffs_close(int fd);
static off_t IRAM_ATTR vfs_spiffs_lseek(int fd, off_t size, int mode);

typedef struct {
	DIR dir;
	spiffs_DIR spiffs_dir;
	char path[MAXNAMLEN + 1];
	struct dirent ent;
	uint8_t read_mount;
} vfs_spiffs_dir_t;

typedef struct {
	spiffs_file spiffs_file;
	char path[MAXNAMLEN + 1];
	uint8_t is_dir;
} vfs_spiffs_file_t;


static spiffs fs;
static struct list files;

static u8_t *my_spiffs_work_buf;
static u8_t *my_spiffs_fds;
static u8_t *my_spiffs_cache;

/*
 * Test if path corresponds to a directory. Return 0 if is not a directory,
 * 1 if it's a directory.
 *
 */
static int is_dir(const char *path) {
    spiffs_DIR d;
    char npath[PATH_MAX + 1];
    int res = 0;

    struct spiffs_dirent e;

    // Add /. to path
    strlcpy(npath, path, PATH_MAX);
    if (strcmp(path,"/") != 0) {
        strlcat(npath,"/.", PATH_MAX);
    } else {
    	strlcat(npath,".", PATH_MAX);
    }

    SPIFFS_opendir(&fs, "/", &d);
    while (SPIFFS_readdir(&d, &e)) {
        if (strncmp(npath, (const char *)e.name, strlen(npath)) == 0) {
            res = 1;
            break;
        }
    }

    SPIFFS_closedir(&d);

    return res;
}

/*
 * This function translate error codes from SPIFFS to errno error codes
 *
 */
static int spiffs_result(int res) {
    switch (res) {
        case SPIFFS_OK:
        case SPIFFS_ERR_END_OF_OBJECT:
            return 0;

        case SPIFFS_ERR_NOT_FOUND:
        case SPIFFS_ERR_CONFLICTING_NAME:
            return ENOENT;

        case SPIFFS_ERR_NOT_WRITABLE:
        case SPIFFS_ERR_NOT_READABLE:
            return EACCES;

        case SPIFFS_ERR_FILE_EXISTS:
            return EEXIST;

        default:
            return res;
    }
}

static int IRAM_ATTR vfs_spiffs_open(const char *path, int flags, int mode) {
	int fd, result = 0;

	// Allocate new file
	vfs_spiffs_file_t *file = calloc(1, sizeof(vfs_spiffs_file_t));
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
    spiffs_flags spiffs_mode = 0;

    // Translate flags to SPIFFS flags
    if (flags == O_RDONLY)
    	spiffs_mode |= SPIFFS_RDONLY;

    if (flags & O_WRONLY)
    	spiffs_mode |= SPIFFS_WRONLY;

    if (flags & O_RDWR)
    	spiffs_mode = SPIFFS_RDWR;

    if (flags & O_EXCL)
    	spiffs_mode |= SPIFFS_EXCL;

    if (flags & O_CREAT)
    	spiffs_mode |= SPIFFS_CREAT;

    if (flags & O_TRUNC)
    	spiffs_mode |= SPIFFS_TRUNC;

    if (is_dir(path)) {
        char npath[PATH_MAX + 1];

        // Add /. to path
        strlcpy(npath, path, PATH_MAX);
        if (strcmp(path,"/") != 0) {
            strlcat(npath,"/.", PATH_MAX);
        } else {
        	strlcat(npath,".", PATH_MAX);
        }

        // Open SPIFFS file
        file->spiffs_file = SPIFFS_open(&fs, npath, spiffs_mode, 0);
        if (file->spiffs_file < 0) {
            result = spiffs_result(fs.err_code);
        }

    	file->is_dir = 1;
    } else {
        // Open SPIFFS file
        file->spiffs_file = SPIFFS_open(&fs, path, spiffs_mode, 0);
        if (file->spiffs_file < 0) {
            result = spiffs_result(fs.err_code);
        }
    }

    if (result != 0) {
    	list_remove(&files, fd, 1);
    	errno = result;
    	return -1;
    }

    return fd;
}

static size_t IRAM_ATTR vfs_spiffs_write(int fd, const void *data, size_t size) {
	vfs_spiffs_file_t *file;
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

    // Write SPIFFS file
	res = SPIFFS_write(&fs, file->spiffs_file, (void *)data, size);
	if (res >= 0) {
		return res;
	} else {
		res = spiffs_result(fs.err_code);
		if (res != 0) {
			errno = res;
			return -1;
		}
	}

	return -1;
}

static ssize_t IRAM_ATTR vfs_spiffs_read(int fd, void * dst, size_t size) {
	vfs_spiffs_file_t *file;
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

    // Read SPIFFS file
	res = SPIFFS_read(&fs, file->spiffs_file, dst, size);
	if (res >= 0) {
		return res;
	} else {
		res = spiffs_result(fs.err_code);
		if (res != 0) {
			errno = res;
			return -1;
		}

		// EOF
		return 0;
	}

	return -1;
}

static int IRAM_ATTR vfs_spiffs_fstat(int fd, struct stat * st) {
	vfs_spiffs_file_t *file;
    spiffs_stat stat;
	int res;

    res = list_get(&files, fd, (void **)&file);
    if (res) {
		errno = EBADF;
		return -1;
    }

	// Set block size for this file system
    st->st_blksize = SPIFFS_LOG_PAGE_SIZE;

    // First test if it's a directory entry
    if (file->is_dir) {
        st->st_mode = S_IFDIR;
        st->st_size = 0;
        return 0;
    }

    // If is not a directory get file statistics
    res = SPIFFS_stat(&fs, file->path, &stat);
    if (res == SPIFFS_OK) {
    	st->st_size = stat.size;
	} else {
		st->st_size = 0;
	    res = spiffs_result(fs.err_code);
    }

    st->st_mode = S_IFREG;

    if (res < 0) {
    	errno = res;
    	return -1;
    }

    return 0;
}

static int IRAM_ATTR vfs_spiffs_close(int fd) {
	vfs_spiffs_file_t *file;
	int res;

    res = list_get(&files, fd, (void **)&file);
    if (res) {
		errno = EBADF;
		return -1;
    }

	res = SPIFFS_close(&fs, file->spiffs_file);
	if (res) {
		res = spiffs_result(fs.err_code);
	}

	if (res < 0) {
		errno = res;
		return -1;
	}

	list_remove(&files, fd, 1);

	return 0;
}

static off_t IRAM_ATTR vfs_spiffs_lseek(int fd, off_t size, int mode) {
	vfs_spiffs_file_t *file;
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

	int whence = SPIFFS_SEEK_CUR;

    switch (mode) {
        case SEEK_SET: whence = SPIFFS_SEEK_SET;break;
        case SEEK_CUR: whence = SPIFFS_SEEK_CUR;break;
        case SEEK_END: whence = SPIFFS_SEEK_END;break;
    }

    res = SPIFFS_lseek(&fs, file->spiffs_file, size, whence);
    if (res < 0) {
        res = spiffs_result(fs.err_code);
        errno = res;
        return -1;
    }

    return res;
}

static int IRAM_ATTR vfs_spiffs_stat(const char * path, struct stat * st) {
	int fd;
	int res;

	fd = vfs_spiffs_open(path, 0, 0);
	res = vfs_spiffs_fstat(fd, st);
	vfs_spiffs_close(fd);

	return res;
}

static int IRAM_ATTR vfs_spiffs_unlink(const char *path) {
    char npath[PATH_MAX + 1];

    strlcpy(npath, path, PATH_MAX);

    if (is_dir(path)) {
	    // Add /. to path
	    if (strcmp(path,"/") != 0) {
	        strlcat(npath,"/.", PATH_MAX);
	    }
	}

    // Open SPIFFS file
	spiffs_file FP = SPIFFS_open(&fs, npath, SPIFFS_RDWR, 0);
    if (FP < 0) {
    	errno = spiffs_result(fs.err_code);
    	return -1;
    }

    // Remove SPIFSS file
    if (SPIFFS_fremove(&fs, FP) < 0) {
        errno = spiffs_result(fs.err_code);
    	SPIFFS_close(&fs, FP);
    	return -1;
    }

	SPIFFS_close(&fs, FP);

	return 0;
}

static int IRAM_ATTR vfs_spiffs_rename(const char *src, const char *dst) {
    if (SPIFFS_rename(&fs, src, dst) < 0) {
    	errno = spiffs_result(fs.err_code);
    	return -1;
    }

    return 0;
}

static DIR* vfs_spiffs_opendir(const char* name) {
	vfs_spiffs_dir_t *dir = calloc(1, sizeof(vfs_spiffs_dir_t));

	if (!dir) {
		errno = ENOMEM;
		return NULL;
	}

	if (!SPIFFS_opendir(&fs, name, &dir->spiffs_dir)) {
        free(dir);
        errno = spiffs_result(fs.err_code);
        return NULL;
    }

	strlcpy(dir->path, name, MAXNAMLEN);

	return (DIR *)dir;
}

static struct dirent* vfs_spiffs_readdir(DIR* pdir) {
    int res = 0, len = 0, entries = 0;
	vfs_spiffs_dir_t* dir = (vfs_spiffs_dir_t*) pdir;

	struct spiffs_dirent e;
    struct spiffs_dirent *pe = &e;

    struct dirent *ent = &dir->ent;

    char *fn;

    // Clear current dirent
    memset(ent,0,sizeof(struct dirent));

    // If this is the first call to readdir for pdir, and
    // directory is the root path, return the mounted point if any
    if (!dir->read_mount) {
    	if (strcmp(dir->path,"/") == 0) {
    	    char mdir[PATH_MAX + 1];

    	    if (mount_readdir("spiffs", dir->path, 0, mdir)) {
    	    	strlcpy(ent->d_name, mdir, PATH_MAX);
    	        ent->d_type = DT_DIR;
    	        ent->d_fsize = 0;
    	        dir->read_mount = 1;

    	        return ent;
    	    }
    	}

    	dir->read_mount = 1;
    }

    // Search for next entry
    for(;;) {
        // Read directory
        pe = SPIFFS_readdir(&dir->spiffs_dir, pe);
        if (!pe) {
            res = spiffs_result(fs.err_code);
            errno = res;
            break;
        }

        // Break condition
        if (pe->name[0] == 0) break;

        // Get name and length
        fn = (char *)pe->name;
        len = strlen(fn);

        // Get entry type and size
        ent->d_type = DT_REG;

        if (len >= 2) {
            if (fn[len - 1] == '.') {
                if (fn[len - 2] == '/') {
                    ent->d_type = DT_DIR;

                    fn[len - 2] = '\0';

                    len = strlen(fn);

                    // Skip root dir
                    if (len == 0) {
                        continue;
                    }
                }
            }
        }

        // Skip entries not belonged to path
        if (strncmp(fn, dir->path, strlen(dir->path)) != 0) {
            continue;
        }

        if (strlen(dir->path) > 1) {
            if (*(fn + strlen(dir->path)) != '/') {
                continue;
            }
        }

        // Skip root directory
        fn = fn + strlen(dir->path);
        len = strlen(fn);
        if (len == 0) {
            continue;
        }

        // Skip initial /
        if (len > 1) {
            if (*fn == '/') {
                fn = fn + 1;
                len--;
            }
        }

        // Skip subdirectories
        if (strchr(fn,'/')) {
            continue;
        }

        ent->d_fsize = pe->size;

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

static int IRAM_ATTR vfs_piffs_closedir(DIR* pdir) {
	vfs_spiffs_dir_t* dir = (vfs_spiffs_dir_t*) pdir;
	int res;

	if (!pdir) {
		errno = EBADF;
		return -1;
	}

	if ((res = SPIFFS_closedir(&dir->spiffs_dir)) < 0) {
		errno = spiffs_result(fs.err_code);;
		return -1;
	}

	free(dir);

    return 0;
}

static int IRAM_ATTR vfs_spiffs_mkdir(const char *path, mode_t mode) {
    char npath[PATH_MAX + 1];
    int res;

    // Add /. to path
    strlcpy(npath, path, PATH_MAX);
    if ((strcmp(path,"/") != 0) && (strcmp(path,"/.") != 0)) {
        strlcat(npath,"/.", PATH_MAX);
    }

    spiffs_file fd = SPIFFS_open(&fs, npath, SPIFFS_CREAT, 0);
    if (fd < 0) {
        res = spiffs_result(fs.err_code);
        errno = res;
        return -1;
    }

    if (SPIFFS_close(&fs, fd) < 0) {
        res = spiffs_result(fs.err_code);
        errno = res;
        return -1;
    }

    return 0;
}

void vfs_spiffs_register() {
    esp_vfs_t vfs = {
        .fd_offset = 0,
        .flags = ESP_VFS_FLAG_DEFAULT,
        .write = &vfs_spiffs_write,
        .open = &vfs_spiffs_open,
        .fstat = &vfs_spiffs_fstat,
        .close = &vfs_spiffs_close,
        .read = &vfs_spiffs_read,
        .lseek = &vfs_spiffs_lseek,
        .stat = &vfs_spiffs_stat,
        .link = NULL,
        .unlink = &vfs_spiffs_unlink,
        .rename = &vfs_spiffs_rename,
		.mkdir = &vfs_spiffs_mkdir,
		.opendir = &vfs_spiffs_opendir,
		.readdir = &vfs_spiffs_readdir,
		.closedir = &vfs_piffs_closedir,
    };
	
    ESP_ERROR_CHECK(esp_vfs_register("/spiffs", &vfs, NULL));

    // Mount spiffs file system
    spiffs_config cfg;
    int unit = 0;
    int res = 0;
    int retries = 0;

    cfg.phys_addr 		 = SPIFFS_BASE_ADDR;
    cfg.phys_size 		 = SPIFFS_SIZE;
    cfg.phys_erase_block = SPIFFS_ERASE_SIZE;
    cfg.log_page_size    = SPIFFS_LOG_PAGE_SIZE;
    cfg.log_block_size   = SPIFFS_LOG_BLOCK_SIZE;

    syslog(LOG_INFO, "spiffs%d start address at 0x%x, size %d Kb",
           unit, cfg.phys_addr, cfg.phys_size / 1024
    );

	cfg.hal_read_f  = (spiffs_read)low_spiffs_read;
	cfg.hal_write_f = (spiffs_write)low_spiffs_write;
	cfg.hal_erase_f = (spiffs_erase)low_spiffs_erase;

    my_spiffs_work_buf = malloc(cfg.log_page_size * 2);
    if (!my_spiffs_work_buf) {
    	// TO DO: assert
    }

    int fds_len = sizeof(spiffs_fd) * 5;
    my_spiffs_fds = malloc(fds_len);
    if (!my_spiffs_fds) {
        free(my_spiffs_work_buf);
    	// TO DO: assert
    }

    int cache_len = cfg.log_page_size * 5;
    my_spiffs_cache = malloc(cache_len);
    if (!my_spiffs_cache) {
        free(my_spiffs_work_buf);
        free(my_spiffs_fds);
    	// TO DO: assert
    }

    res = SPIFFS_mount(
            &fs, &cfg, my_spiffs_work_buf, my_spiffs_fds,
            fds_len, my_spiffs_cache, cache_len, NULL
    );

    if (res < 0) {
        if (fs.err_code == SPIFFS_ERR_NOT_A_FS) {
            syslog(LOG_ERR, "spiffs%d no file system detect, formating", unit);
            SPIFFS_unmount(&fs);
            res = SPIFFS_format(&fs);
            if (res < 0) {
                syslog(LOG_ERR, "spiffs%d format error",unit);
            	// TO DO: assert
            }
        }
    } else {
        if (retries > 0) {
        	// TO DO
            //spiffs_mkdir_op("/.");
        }
    }

    mount_set_mounted("spiffs", 1);

    list_init(&files, 0);

    syslog(LOG_INFO, "spiffs%d mounted", unit);
}

#endif
