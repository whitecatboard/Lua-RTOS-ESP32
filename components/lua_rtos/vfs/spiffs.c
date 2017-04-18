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

#if CONFIG_LUA_RTOS_USE_SPIFFS

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

#if !defined(max)
#define max(A,B) ( (A) > (B) ? (A):(B))
#endif

#if !defined(min)
#define min(A,B) ( (A) < (B) ? (A):(B))
#endif

#define VFS_SPIFFS_FLAGS_READONLY  (1 << 0)

typedef struct {
	uint8_t flags;
} vfs_spiffs_meta_t;

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

static void dir_path(char *npath, uint8_t base) {
	int len = strlen(npath);

	if (base) {
		char *c;

		c = &npath[len - 1];
		while (c >= npath) {
			if (*c == '/') {
				break;
			}

			len--;
			c--;
		}
	}

    if (len > 1) {
    	if (npath[len - 1] == '.') {
    		npath[len - 1] = '\0';

        	if (npath[len - 2] == '/') {
        		npath[len - 2] = '\0';
        	}
    	} else {
        	if (npath[len - 1] == '/') {
        		npath[len - 1] = '\0';
        	}
    	}
    } else {
    	if ((npath[len - 1] == '/') ||  (npath[len - 1] == '.')) {
    		npath[len - 1] = '\0';
    	}
    }

    strlcat(npath,"/.", PATH_MAX);
}

static void check_path(const char *path, uint8_t *base_is_dir, uint8_t *full_is_dir, uint8_t *is_file) {
    char bpath[PATH_MAX + 1]; // Base path
    char fpath[PATH_MAX + 1]; // Full path
    struct spiffs_dirent e;
    spiffs_DIR d;

    *base_is_dir = 0;
    *full_is_dir = 0;
    *is_file = 0;

    // Get base directory name
    strlcpy(bpath, path, PATH_MAX);
    dir_path(bpath, 1);

    // Get full directory name
    strlcpy(fpath, path, PATH_MAX);
    dir_path(fpath, 0);

    SPIFFS_opendir(&fs, "/", &d);
	while (SPIFFS_readdir(&d, &e)) {
		if (!strcmp(bpath, (const char *)e.name)) {
			*base_is_dir = 1;
		}

		if (!strcmp(fpath, (const char *)e.name)) {
			*full_is_dir = 1;
		}

		if (!strcmp(path, (const char *)e.name)) {
			*is_file = 1;
		}
	}
	SPIFFS_closedir(&d);
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
    char npath[PATH_MAX + 1];
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

    // Check path
    uint8_t base_is_dir = 0;
    uint8_t full_is_dir = 0;
    uint8_t is_file = 0;

    check_path(path, &base_is_dir, &full_is_dir, &is_file);

    if (full_is_dir) {
    	// We want to open a directory.
    	// If in flags are set some write access mode this is an error, because we only
    	// can open a directory in read mode.
    	if (spiffs_mode & (SPIFFS_WRONLY | SPIFFS_CREAT | SPIFFS_TRUNC)) {
        	list_remove(&files, fd, 1);
    		errno = EISDIR;
    		return -1;
    	}

    	// Open the directory
        strlcpy(npath, path, PATH_MAX);
        dir_path((char *)npath, 0);

        // Open SPIFFS file
        file->spiffs_file = SPIFFS_open(&fs, npath, SPIFFS_RDONLY, 0);
        if (file->spiffs_file < 0) {
            result = spiffs_result(fs.err_code);
        }

    	file->is_dir = 1;
    } else {
    	if (!base_is_dir) {
    		// If base path is not a directory we return an error
        	list_remove(&files, fd, 1);
    		errno = ENOENT;
    		return -1;
    	} else {
            // Open SPIFFS file
            file->spiffs_file = SPIFFS_open(&fs, path, spiffs_mode, 0);
            if (file->spiffs_file < 0) {
                result = spiffs_result(fs.err_code);
            }
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
    st->st_blksize = CONFIG_LUA_RTOS_SPIFFS_LOG_PAGE_SIZE;

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

    // Check path
    uint8_t base_is_dir = 0;
    uint8_t full_is_dir = 0;
    uint8_t is_file = 0;

    check_path(path, &base_is_dir, &full_is_dir, &is_file);

	strlcpy(npath, path, PATH_MAX);
    if (full_is_dir) {
    	// We need to remove all tree
        struct spiffs_dirent e;
        spiffs_DIR d;

        dir_path(npath, 0);

    	SPIFFS_opendir(&fs, "/", &d);
    	while (SPIFFS_readdir(&d, &e)) {
    		if (!strncmp(path, (const char *)e.name, min(strlen((char *)e.name), strlen(path)))) {
    	        // Open SPIFFS file
    	    	spiffs_file FP = SPIFFS_open(&fs, (char *)e.name, SPIFFS_RDWR, 0);
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
    		}
    	}
    	SPIFFS_closedir(&d);
    } else {
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
    }

	return 0;
}

static int IRAM_ATTR vfs_spiffs_rename(const char *src, const char *dst) {
    char dpath[PATH_MAX + 1];
    char *csrc;
    char *cname;

    // Check paths
    uint8_t src_base_is_dir = 0;
    uint8_t src_full_is_dir = 0;
    uint8_t src_is_file = 0;

    uint8_t dst_base_is_dir = 0;
    uint8_t dst_full_is_dir = 0;
    uint8_t dst_is_file = 0;

    check_path(src, &src_base_is_dir, &src_full_is_dir, &src_is_file);
    check_path(dst, &dst_base_is_dir, &dst_full_is_dir, &dst_is_file);

    // Sanity checks
    if (src_is_file && dst_full_is_dir) {
    	errno = EISDIR;
    	return -1;
    }

    if (src_full_is_dir && dst_is_file) {
    	errno = ENOTDIR;
    	return -1;
    }

    if (src_full_is_dir) {
    	// We need to rename all tree
        struct spiffs_dirent e;
        spiffs_DIR d;

    	SPIFFS_opendir(&fs, "/", &d);
    	while (SPIFFS_readdir(&d, &e)) {
    		if (!strncmp(src, (const char *)e.name, min(strlen((char *)e.name), strlen(src)))) {
    			strlcpy(dpath, dst, PATH_MAX);
    			csrc = (char *)src;
    			cname = (char *)e.name;

    			while (*csrc && *cname && (*csrc == *cname)) {
    				++csrc;
    				++cname;
    			}

    			strlcat(dpath, cname, PATH_MAX);

    	    	if (SPIFFS_rename(&fs, (char *)e.name, dpath) < 0) {
    	        	errno = spiffs_result(fs.err_code);
    	        	return -1;
    	        }
    		}
    	}
    	SPIFFS_closedir(&d);
    } else {
    	if (SPIFFS_rename(&fs, src, dst) < 0) {
        	errno = spiffs_result(fs.err_code);
        	return -1;
        }
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
    //vfs_spiffs_meta_t meta;
    int res;

    // Check path
    uint8_t base_is_dir = 0;
    uint8_t full_is_dir = 0;
    uint8_t is_file = 0;

    check_path(path, &base_is_dir, &full_is_dir, &is_file);

    // Sanity checks
    if (is_file) {
    	errno = ENOENT;
    	return -1;
    }

    if (full_is_dir) {
    	errno = EEXIST;
    	return -1;
    }

    if (!base_is_dir) {
    	errno = ENOTDIR;
    	return -1;
    }

    // Create directory
    strlcpy(npath, path, PATH_MAX);
    dir_path(npath, 0);

    //meta.flags = 0;

    spiffs_file fd = SPIFFS_open(&fs, npath, SPIFFS_CREAT | SPIFFS_RDWR, 0);
    if (fd < 0) {
        res = spiffs_result(fs.err_code);
        errno = res;
        return -1;
    }

    //if (SPIFFS_fupdate_meta(&fs, fd, (const void *)&meta) < 0) {
    //    res = spiffs_result(fs.err_code);
    //    errno = res;
    //    return -1;
    //}

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

    cfg.phys_addr 		 = CONFIG_LUA_RTOS_SPIFFS_BASE_ADDR;
    cfg.phys_size 		 = CONFIG_LUA_RTOS_SPIFFS_SIZE;
    cfg.phys_erase_block = CONFIG_LUA_RTOS_SPIFFS_ERASE_SIZE;
    cfg.log_page_size    = CONFIG_LUA_RTOS_SPIFFS_LOG_PAGE_SIZE;
    cfg.log_block_size   = CONFIG_LUA_RTOS_SPIFFS_LOG_BLOCK_SIZE;

    syslog(LOG_INFO, "spiffs%d start address at 0x%x, size %d Kb",
           unit, cfg.phys_addr, cfg.phys_size / 1024
    );

	cfg.hal_read_f  = (spiffs_read)low_spiffs_read;
	cfg.hal_write_f = (spiffs_write)low_spiffs_write;
	cfg.hal_erase_f = (spiffs_erase)low_spiffs_erase;

    my_spiffs_work_buf = malloc(cfg.log_page_size * 2);
    if (!my_spiffs_work_buf) {
    	syslog(LOG_ERR, "spiffs%d can't allocate memory for file system", unit);
    	return;
    }

    int fds_len = sizeof(spiffs_fd) * 5;
    my_spiffs_fds = malloc(fds_len);
    if (!my_spiffs_fds) {
        free(my_spiffs_work_buf);
    	syslog(LOG_ERR, "spiffs%d can't allocate memory for file system", unit);
    	return;
    }

    int cache_len = cfg.log_page_size * 5;
    my_spiffs_cache = malloc(cache_len);
    if (!my_spiffs_cache) {
        free(my_spiffs_work_buf);
        free(my_spiffs_fds);
    	syslog(LOG_ERR, "spiffs%d can't allocate memory for file system", unit);
    	return;
    }

    while (retries < 2) {
        res = SPIFFS_mount(
    		&fs, &cfg, my_spiffs_work_buf, my_spiffs_fds,
    		fds_len, my_spiffs_cache, cache_len, NULL
        );

        if (res < 0) {
            if (fs.err_code == SPIFFS_ERR_NOT_A_FS) {
                syslog(LOG_ERR, "spiffs%d no file system detect, formating...", unit);
                SPIFFS_unmount(&fs);
                res = SPIFFS_format(&fs);
                if (res < 0) {
                    free(my_spiffs_work_buf);
                    free(my_spiffs_fds);
                    free(my_spiffs_cache);
                    syslog(LOG_ERR, "spiffs%d format error",unit);
                    return;
                }
            } else {
                free(my_spiffs_work_buf);
                free(my_spiffs_fds);
                free(my_spiffs_cache);
                syslog(LOG_ERR, "spiffs%d can't mount file system (%s)",unit, strerror(spiffs_result(fs.err_code)));
                return;
            }
        } else {
        	break;
        }

        retries++;
    }

    mount_set_mounted("spiffs", 1);

    list_init(&files, 0);

    if (retries > 0) {
    	syslog(LOG_INFO, "spiffs%d creating root folder", unit);

		// Create the root folder
	    spiffs_file fd = SPIFFS_open(&fs, "/.", SPIFFS_CREAT | SPIFFS_RDWR, 0);
	    if (fd < 0) {
            free(my_spiffs_work_buf);
            free(my_spiffs_fds);
            free(my_spiffs_cache);
            syslog(LOG_ERR, "spiffs%d can't create root folder (%s)",unit, strerror(spiffs_result(fs.err_code)));
            return;
	    }

	    if (SPIFFS_close(&fs, fd) < 0) {
            free(my_spiffs_work_buf);
            free(my_spiffs_fds);
            free(my_spiffs_cache);
            syslog(LOG_ERR, "spiffs%d can't create root folder (%s)",unit, strerror(spiffs_result(fs.err_code)));
            return;
	    }
	}

    syslog(LOG_INFO, "spiffs%d mounted", unit);
}

void vfs_spiffs_format() {
	int res = 0;
	int unit = 0;

	// First unregister
	esp_vfs_unregister("/spiffs");
	mount_set_mounted("spiffs", 0);

	// Unmount
    SPIFFS_unmount(&fs);

    res = SPIFFS_format(&fs);
    if (res < 0) {
        free(my_spiffs_work_buf);
        free(my_spiffs_fds);
        free(my_spiffs_cache);
        syslog(LOG_ERR, "spiffs%d format error",unit);
        return;
    }

    // Free allocated space
    free(my_spiffs_work_buf);
    free(my_spiffs_fds);
    free(my_spiffs_cache);

    // Register again
    vfs_spiffs_register();

    syslog(LOG_INFO, "spiffs%d creating root folder", unit);

	// Create the root folder
    spiffs_file fd = SPIFFS_open(&fs, "/.", SPIFFS_CREAT | SPIFFS_RDWR, 0);
    if (fd < 0) {
        free(my_spiffs_work_buf);
        free(my_spiffs_fds);
        free(my_spiffs_cache);
        syslog(LOG_ERR, "spiffs%d can't create root folder (%s)",unit, strerror(spiffs_result(fs.err_code)));
        return;
    }

    if (SPIFFS_close(&fs, fd) < 0) {
        free(my_spiffs_work_buf);
        free(my_spiffs_fds);
        free(my_spiffs_cache);
        syslog(LOG_ERR, "spiffs%d can't create root folder (%s)",unit, strerror(spiffs_result(fs.err_code)));
        return;
    }
}

#endif
