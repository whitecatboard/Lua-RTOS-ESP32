/*
 * Copyright (C) 2015 - 2018, IBEROXARXA SERVICIOS INTEGRALES, S.L.
 * Copyright (C) 2015 - 2018, Jaume Olivé Petrus (jolive@whitecatboard.org)
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the <organization> nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *     * The WHITECAT logotype cannot be changed, you can remove it, but you
 *       cannot change it in any way. The WHITECAT logotype is:
 *
 *          /\       /\
 *         /  \_____/  \
 *        /_____________\
 *        W H I T E C A T
 *
 *     * Redistributions in binary form must retain all copyright notices printed
 *       to any local or remote output device. This include any reference to
 *       Lua RTOS, whitecatboard.org, Lua, and other copyright notices that may
 *       appear in the future.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Lua RTOS spiffs vfs
 *
 */

#include "sdkconfig.h"

#if CONFIG_LUA_RTOS_USE_SPIFFS

#include "esp_partition.h"

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

static int vfs_spiffs_open(const char *path, int flags, int mode);
static ssize_t vfs_spiffs_write(int fd, const void *data, size_t size);
static ssize_t vfs_spiffs_read(int fd, void * dst, size_t size);
static int vfs_spiffs_fstat(int fd, struct stat * st);
static int vfs_spiffs_close(int fd);
static off_t vfs_spiffs_lseek(int fd, off_t size, int mode);

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

static void check_path(const char *path, uint8_t *base_is_dir, uint8_t *full_is_dir, uint8_t *is_file, int *files) {
    char bpath[PATH_MAX + 1]; // Base path
    char fpath[PATH_MAX + 1]; // Full path
    struct spiffs_dirent e;
    spiffs_DIR d;
    int file_num = 0;

    *files = 0;
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

		if (!strncmp(fpath, (const char *)e.name, min(strlen((char *)e.name), strlen(fpath) - 1))) {
			if (strcmp(fpath, (const char *)e.name)) {
				file_num++;
			}
		}
	}
	SPIFFS_closedir(&d);

	*files = file_num;
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

static int vfs_spiffs_open(const char *path, int flags, int mode) {
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
    int file_num = 0;

    check_path(path, &base_is_dir, &full_is_dir, &is_file, &file_num);

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

static ssize_t vfs_spiffs_write(int fd, const void *data, size_t size) {
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

static ssize_t vfs_spiffs_read(int fd, void * dst, size_t size) {
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

static int vfs_spiffs_fstat(int fd, struct stat * st) {
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

static int vfs_spiffs_close(int fd) {
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

static off_t vfs_spiffs_lseek(int fd, off_t size, int mode) {
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

static int vfs_spiffs_stat(const char * path, struct stat * st) {
	int fd;
	int res;

	fd = vfs_spiffs_open(path, 0, 0);
	if (fd < 0) {
    	errno = spiffs_result(fs.err_code);
    	return -1;
	}

	res = vfs_spiffs_fstat(fd, st);
	if (fd < 0) {
    	errno = spiffs_result(fs.err_code);
    	return -1;
	}

	vfs_spiffs_close(fd);

	return res;
}

static int vfs_spiffs_unlink(const char *path) {
    char npath[PATH_MAX + 1];

    // Check path
    uint8_t base_is_dir = 0;
    uint8_t full_is_dir = 0;
    uint8_t is_file = 0;
    int file_num = 0;

    check_path(path, &base_is_dir, &full_is_dir, &is_file, &file_num);

    strlcpy(npath, path, PATH_MAX);
    if (full_is_dir) {
    	errno = EISDIR;
    	return -1;
    } else {
    	if (!is_file) {
    		errno = ENOENT;
    		return -1;
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
    }

	return 0;
}

static int vfs_spiffs_rename(const char *src, const char *dst) {
    char dpath[PATH_MAX + 1];
    char *csrc;
    char *cname;

    // Check paths
    uint8_t src_base_is_dir = 0;
    uint8_t src_full_is_dir = 0;
    uint8_t src_is_file = 0;
    int src_files = 0;

    uint8_t dst_base_is_dir = 0;
    uint8_t dst_full_is_dir = 0;
    uint8_t dst_is_file = 0;
    int dst_files = 0;

    check_path(src, &src_base_is_dir, &src_full_is_dir, &src_is_file, &src_files);
    check_path(dst, &dst_base_is_dir, &dst_full_is_dir, &dst_is_file, &dst_files);

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
//    		if (!strncmp(src, (const char *)e.name, min(strlen((char *)e.name), strlen(src)))) {
    		if ( !strncmp(src, (const char *)e.name, strlen(src) ) && e.name[strlen(src)] == '/' ) {
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
    char npath[PATH_MAX + 1];
	vfs_spiffs_dir_t *dir = calloc(1, sizeof(vfs_spiffs_dir_t));

	if (!dir) {
		errno = ENOMEM;
		return NULL;
	}

    // Check path, must be an existing directory
    uint8_t base_is_dir = 0;
    uint8_t full_is_dir = 0;
    uint8_t is_file = 0;
    int file_num = 0;

    check_path(name, &base_is_dir, &full_is_dir, &is_file, &file_num);

	strlcpy(npath, name, PATH_MAX);
    if (full_is_dir) {
    	if (!SPIFFS_opendir(&fs, name, &dir->spiffs_dir)) {
            free(dir);
            errno = spiffs_result(fs.err_code);
            return NULL;
        }

    	strlcpy(dir->path, name, MAXNAMLEN);

    	return (DIR *)dir;
    } else {
    	free(dir);
    	errno = ENOENT;
    	return NULL;
    }
}

static int vfs_spiffs_rmdir(const char* name) {
    char npath[PATH_MAX + 1];
	vfs_spiffs_dir_t *dir = calloc(1, sizeof(vfs_spiffs_dir_t));

	if (!dir) {
		errno = ENOMEM;
		return NULL;
	}

    // Check path, must be an existing directory
	// files in directory
    uint8_t base_is_dir = 0;
    uint8_t full_is_dir = 0;
    uint8_t is_file = 0;
    int file_num = 0;

    check_path(name, &base_is_dir, &full_is_dir, &is_file, &file_num);
    if (full_is_dir) {
    	if (file_num > 0) {
    		free(dir);
    		errno = ENOTEMPTY;
    		return -1;
    	} else {
    	    strlcpy(npath, name, PATH_MAX);
    	    dir_path(npath, 0);

	        // Open SPIFFS file
	    	spiffs_file FP = SPIFFS_open(&fs, npath, SPIFFS_RDWR, 0);
	        if (FP < 0) {
	        	free(dir);
	        	errno = spiffs_result(fs.err_code);
	        	return -1;
	        }

	        // Remove SPIFSS file
	        if (SPIFFS_fremove(&fs, FP) < 0) {
	        	free(dir);
	        	errno = spiffs_result(fs.err_code);
	        	SPIFFS_close(&fs, FP);
	        	return -1;
	        }

        	SPIFFS_close(&fs, FP);
        	free(dir);
        	return 0;
    	}
    } else {
       	free(dir);
       	errno = ENOENT;
       	return -1;
    }
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

static int vfs_piffs_closedir(DIR* pdir) {
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

static int vfs_spiffs_mkdir(const char *path, mode_t mode) {
    char npath[PATH_MAX + 1];
    //vfs_spiffs_meta_t meta;
    int res;

    // Check path
    uint8_t base_is_dir = 0;
    uint8_t full_is_dir = 0;
    uint8_t is_file = 0;
    int file_num = 0;

    check_path(path, &base_is_dir, &full_is_dir, &is_file, &file_num);

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

static int vfs_spiffs_fsync(int fd) {
	vfs_spiffs_file_t *file;
	int res;

	res = list_get(&files, fd, (void **)&file);
    if (res) {
		errno = EBADF;
		printf("1\r\n");
		return -1;
    }

    if (file->is_dir) {
		errno = EBADF;
		printf("2\r\n");
		return -1;
    }

    res = SPIFFS_fflush(&fs, file->spiffs_file);
	if (res >= 0) {
		return res;
	} else {
		res = spiffs_result(fs.err_code);
		if (res != 0) {
			errno = res;
			printf("3\r\n");

			return -1;
		}

		errno = EIO;
		printf("4\r\n");

		return -1;
	}

	return 0;
}

void vfs_spiffs_register() {
    esp_vfs_t vfs = {
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
		.rmdir = &vfs_spiffs_rmdir,
		.fsync = &vfs_spiffs_fsync,
    };
	
    ESP_ERROR_CHECK(esp_vfs_register("/spiffs", &vfs, NULL));

    // Mount spiffs file system
    spiffs_config cfg;
    int unit = 0;
    int res = 0;
    int retries = 0;

    // Find partition
    const esp_partition_t *partition =
    	esp_partition_find_first(ESP_PARTITION_TYPE_DATA, 0xfe, "spiffs");

    if (!partition) {
    	syslog(LOG_ERR, "spiffs%d can't find spiffs partition", unit);
    } else {
		cfg.phys_addr 	 = partition->address;
		cfg.phys_size 	 = partition->size;
    }

    cfg.phys_erase_block = CONFIG_LUA_RTOS_SPIFFS_ERASE_SIZE;
    cfg.log_page_size    = CONFIG_LUA_RTOS_SPIFFS_LOG_PAGE_SIZE;
    cfg.log_block_size   = CONFIG_LUA_RTOS_SPIFFS_LOG_BLOCK_SIZE;

    if (partition) {
        syslog(LOG_INFO, "spiffs%d start address at 0x%x, size %d Kb, partition %s",
               unit, cfg.phys_addr, cfg.phys_size / 1024,
			   partition->label
        );
    } else {
        syslog(LOG_INFO, "spiffs%d start address at 0x%x, size %d Kb",
               unit, cfg.phys_addr, cfg.phys_size / 1024
        );
    }

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
        syslog(LOG_ERR, "spiffs%d can't create root folder (%s)",unit, strerror(spiffs_result(fs.err_code)));
        return;
    }

    if (SPIFFS_close(&fs, fd) < 0) {
        syslog(LOG_ERR, "spiffs%d can't create root folder (%s)",unit, strerror(spiffs_result(fs.err_code)));
        return;
    }
}

#endif
