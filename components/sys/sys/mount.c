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
 * Lua RTOS, file system mount functions
 *
 */

#include "luartos.h"
#include "mount.h"
#include "params.h"

#include <unistd.h>
#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <sys/mutex.h>

#include <sys/vfs/vfs.h>

struct mtx mtx;

void _mount_init() {
	mtx_init(&mtx, NULL, NULL, MTX_RECURSE);
}

// Current mount points
struct mount_pt mountps[] = {
#if (CONFIG_SD_CARD_MMC || CONFIG_SD_CARD_SPI) && CONFIG_LUA_RTOS_USE_FAT
    {NULL, "fat", &vfs_fat_mount, &vfs_fat_umount, &vfs_fat_format, 0},
#endif
#if CONFIG_LUA_RTOS_USE_LFS
    {NULL, "lfs", &vfs_lfs_mount, &vfs_lfs_umount, &vfs_lfs_format, 0},
#endif
#if CONFIG_LUA_RTOS_USE_RAM_FS
    {NULL, "rfs", &vfs_ramfs_mount, &vfs_ramfs_umount, &vfs_ramfs_format, 0},
#endif
#if CONFIG_LUA_RTOS_USE_SPIFFS
   {NULL, "spiffs", &vfs_spiffs_mount, &vfs_spiffs_umount, &vfs_spiffs_format, 0},
#endif
    {"/dev", "dev", NULL, NULL, NULL, 0},
    {NULL, NULL, NULL, NULL, NULL, 0}
};

static char *dot_dot(char *rpath, char *cpath) {
    char *last;

    if (cpath >= rpath + strlen(rpath)) {
        last = rpath + strlen(rpath);
    } else {
        last = cpath + 1;
    }

    while ((cpath - 1 >= rpath) && (*--cpath != '/'));
    while ((cpath - 1 >= rpath) && (*--cpath != '/'));

    char *tpath = ++cpath;
    while (*last) {
        *tpath++ = *last++;
    }
    *tpath = '\0';

    return cpath;
}

static char *dot(char *rpath, char *cpath) {
    char *last;

    if (cpath >= rpath + strlen(rpath)) {
        last = rpath + strlen(rpath);
    } else {
        last = cpath + 1;
    }

    while ((cpath - 1 >= rpath) && (*--cpath != '/'));

    char *tpath = ++cpath;
    while (*last) {
        *tpath++ = *last++;
    }
    *tpath = '\0';

    return cpath;
}

static char *slash_slash(char *rpath, char *cpath) {
    char *last = cpath + 1;

    while ((cpath - 1 >= rpath) && (*--cpath != '/'));

    char *tpath = ++cpath;
    while (*last) {
        *tpath++ = *last++;
    }
    *tpath = '\0';

    return cpath;

}

const char *mount_get_mount_path(const char *fs) {
	mtx_lock(&mtx);

    struct mount_pt *cmount = &mountps[0];

    while (cmount->fs) {
        if (cmount->fs && (strcmp(fs, cmount->fs) == 0)) {
        		mtx_unlock(&mtx);
            return cmount->fpath;
        }

        cmount++;
    }

	mtx_unlock(&mtx);
    return NULL;
}

static char *mount_normalize_path_internal(const char *path, uint8_t check) {
    char *rpath;
    char *cpath;
    int maybe_is_dot = 0;
    int maybe_is_dot_dot = 0;
    int is_dot = 0;
    int is_dot_dot = 0;
    int is_slash = 0;
    int is_slash_slash = 0;

    // We need as many characters as the initial path + PATH_MAX + 1
    // to prepend the current directory + 1 additional character for
    // the end of the string.
    //
    // We can use strcpy / strcat functions because destination string has enough
    // allocated space.
    size_t size = strlen(path) + PATH_MAX + 2;

    // Allocate space
    rpath = calloc(1, size);
    if (!rpath) {
        errno = ENOMEM;
        return NULL;
    }

    if (*path != '/') {
        // It's a relative path, so prepend the current working directory
        if (!getcwd(rpath, PATH_MAX)) {
            free(rpath);
            return NULL;
        }

        // Append / if the current working directory doesn't end with /
        if (*(rpath + strlen(rpath) - 1) != '/') {
            strcat(rpath, "/");
        }

        // Append initial path
        strcat(rpath, path);
    } else {
        strcpy(rpath, path);
    }

    cpath = rpath;
    while (*cpath) {
        if (*cpath == '.') {
            maybe_is_dot_dot = is_slash && maybe_is_dot;
            maybe_is_dot = is_slash && !maybe_is_dot_dot;
        } else if (*cpath == '/') {
            is_dot_dot = maybe_is_dot_dot;
            is_dot = maybe_is_dot && !is_dot_dot;
            is_slash = 1;
        } else {
            maybe_is_dot = 0;
            maybe_is_dot_dot = 0;
            is_slash = 0;
        }

        if (is_dot_dot) {
            cpath = dot_dot(rpath, cpath);

            is_dot = 0;
            is_dot_dot = 0;
            maybe_is_dot = 0;
            maybe_is_dot_dot = 0;

            continue;
        } else if (is_dot) {
            cpath = dot(rpath, cpath);

            is_dot = 0;
            is_dot_dot = 0;
            maybe_is_dot = 0;
            maybe_is_dot_dot = 0;

            continue;
        }

        cpath++;
    }

    // End case
    is_dot_dot = maybe_is_dot_dot;
    is_dot = maybe_is_dot && !is_dot_dot;

    if (is_dot_dot) {
        cpath = dot_dot(rpath, cpath);
    } else if (is_dot || maybe_is_dot) {
        cpath = dot(rpath, cpath);
    }

    // Normalize //
    cpath = rpath;
    while (*cpath) {
        if (*cpath == '/') {
            is_slash_slash = is_slash;
            is_slash = 1;
        } else {
            is_slash = 0;
            is_slash_slash = 0;
        }

        if (is_slash_slash) {
            cpath = slash_slash(rpath, cpath);

            is_slash = 1;
            is_slash_slash = 0;

            continue;
        }
        cpath++;
    }

    // End case
    if (is_slash_slash) {
        cpath = slash_slash(rpath, cpath);
    }

    // A normalized path must never end by /, except if it is the
    // root folder
    cpath = (strlen(rpath) > 1)?(rpath + strlen(rpath) - 1):NULL;
    if (cpath && (*cpath == '/')) {
        *cpath = '\0';
    }

    // Check that fits in PATH_MAX
    if (check && (strlen(rpath) > PATH_MAX)) {
        free(rpath);

        errno = ENAMETOOLONG;
        return NULL;
    } else {
        char *tmp = strdup(rpath);
        if (tmp) {
            free(rpath);
            rpath = tmp;
        }
    }

    return rpath;
}

struct mount_pt *mount_get_root() {
	mtx_lock(&mtx);

	struct mount_pt *cmount = &mountps[0];

    while (cmount->fs) {
        if (cmount->fpath && (*cmount->fpath) && (*cmount->fpath == '/') && (*(cmount->fpath + 1) == '\0')) {
        		mtx_unlock(&mtx);

            return cmount;
        }
        cmount++;
    }

	mtx_unlock(&mtx);

    return NULL;
}

static const char *mount_get_fs_from_logical_path(const char *path, char **rpath) {
	mtx_lock(&mtx);

    struct mount_pt *cmount = &mountps[0];

    struct mount_pt *root = NULL;
    const char *cpath;
    const char *cfpath;

    root = mount_get_root();

    while (cmount->fs) {
        // Skip the root
        if (cmount == root) {
            cmount++;
            continue;
        }

        // Check if the first directory of the path corresponds to a mount point
        cpath = path;
        cfpath = cmount->fpath;

        if (cfpath) {
            while (*cpath && *cfpath && (*cpath == *cfpath)) {
                cpath++;
                cfpath++;
            }

            if ((!*cfpath) && ((*cpath == '/') || (*cpath == 0x00))) {
                if (rpath) {
                    *rpath = (char *)cpath;
                }

                mtx_unlock(&mtx);

                return cmount->fs;
            }
        }

        cmount++;
    }

    if (rpath) {
        if (*path == '/') {
            *rpath = (char *)path + 1;
        } else {
            *rpath = (char *)path;
        }
    }

	// Path is in the root file system
    if (root) {
		mtx_unlock(&mtx);
    		return root->fs;
    }

	mtx_unlock(&mtx);
    return NULL;
}

char *mount_normalize_path(const char *path) {
    return mount_normalize_path_internal(path, 1);
}

char *mount_resolve_to_physical(const char *path) {
    const char *fs;

    char *npath;
    char *rpath;
    char *ppath;

    // Normalize path
    npath = mount_normalize_path_internal(path, 0);
    if (!npath){
        return NULL;
    }

    // We need as many characters as the normalized path + CONFIG_VFS_PATH_MAX + 1
    // to prepend the file system were path is physically mounted + 1 additional
    // character for the end of the string.
    //
    // We can use strcpy / strcat functions because destination string has enough
    // allocated space.
    size_t size = strlen(npath) + CONFIG_VFS_PATH_MAX + 2;

    // Allocate space for physical path, and build it
    ppath = calloc(1, size);
    if (!ppath) {
        errno = ENOMEM;
        free(npath);
        return NULL;
    }

    // Get the file system where path is mounted
    if ((fs = mount_get_fs_from_logical_path(npath, &rpath))) {
        if (*fs != '/') {
            strcpy(ppath, "/");
            strcat(ppath, fs);
        }

        if (*rpath != '/') {
            strcat(ppath, "/");
        }

        strcat(ppath, rpath);
    } else {
        strcpy(ppath, npath);
    }

    // A physical path must never end by /, except if it is the
    // root folder
    char * cpath = (strlen(ppath) > 1)?(ppath + strlen(ppath) - 1):NULL;
    if (cpath && (*cpath == '/')) {
        *cpath = '\0';
    }

    free(npath);

    return ppath;
}

static int mount_num() {
	int count = 0;

	mtx_lock(&mtx);

	struct mount_pt *cmount = mountps;

    while (cmount->fs) {
    		if (cmount->mounted) {
    			count++;
    		}

        cmount++;
    }

	mtx_unlock(&mtx);

    return count;
}

static int mount_is_mounted(const char *fs) {
	mtx_lock(&mtx);

	struct mount_pt *cmount = mountps;

    while (cmount->fs) {
        if (strcmp(cmount->fs, fs) == 0) {
        		mtx_unlock(&mtx);
            return cmount->mounted;
        }

        cmount++;
    }

	mtx_unlock(&mtx);
    return 0;
}

void mount_set_mounted(const char *fs, unsigned int mounted) {
	mtx_lock(&mtx);

    struct mount_pt *cmount = mountps;

    while (cmount->fs) {
        if (strcmp(cmount->fs, fs) == 0) {
            cmount->mounted = mounted;
        }

        cmount++;
    }

    mtx_unlock(&mtx);
}

struct dirent* mount_readdir(DIR* pdir) {
    vfs_dir_t* dir = (vfs_dir_t*) pdir;
    struct dirent *ent = &dir->ent;

    if (dir->mount) {
        while (dir->mount->fs) {
            if (dir->mount->fpath && ((*(dir->mount->fpath) != '/') || (*(dir->mount->fpath + 1) != 0x00)) && (dir->mount->mounted)) {
                memset(ent, 0, sizeof(struct dirent));
                strlcpy(ent->d_name, dir->mount->fpath + 1, PATH_MAX);
                ent->d_type = DT_DIR;

                dir->mount++;

                return ent;
            } else {
                dir->mount++;
            }
        }

        if (!dir->mount->fpath) {
            dir->mount = NULL;
            return NULL;
        }
    }

    return NULL;
}

char *mount_history_file(char *location, size_t loc_size) {
    const char *path = NULL;

    if (mount_is_mounted("fat")) {
        path = mount_get_mount_path("fat");
    } else if (mount_is_mounted("rfs")) {
        path = mount_get_mount_path("rfs");
    }

    if (path) {
        *location = 0x00;

        strncpy(location,path,loc_size);
        strncat(location, "/history",loc_size);

        return location;
    }

    return NULL;
}

char *mount_messages_file(char *location, size_t loc_size) {
    const char *path = NULL;

    if (mount_is_mounted("fat")) {
        path = mount_get_mount_path("fat");
    }

    if (path) {
        *location = 0x00;

        strncpy(location,path,loc_size -1 );
        strncat(location, "/log/messages.log",loc_size);

        return location;
    }

    return NULL;
}

struct mount_pt *mount_get_mount_point_for_path(const char *path) {
	mtx_lock(&mtx);

	struct mount_pt *cmount = &mountps[0];

    if (!path) {
		mtx_unlock(&mtx);

        errno = EFAULT;
        return NULL;
    }

    if (!*path) {
		mtx_unlock(&mtx);

		errno = ENOENT;
        return NULL;
    }

    char *ppath = mount_resolve_to_physical(path);
    if (ppath) {
        ppath++;
        while (cmount->fs) {
            if (strcmp(ppath, cmount->fs) == 0) {
                free(--ppath);

                mtx_unlock(&mtx);
                return cmount;
            }

            cmount++;
        }

        free(--ppath);
    }

	mtx_unlock(&mtx);

	return NULL;
}


struct mount_pt *mount_get_mount_point_for_fs(const char *fs) {
	mtx_lock(&mtx);

	struct mount_pt *cmount = &mountps[0];

    if (!fs) {
		mtx_unlock(&mtx);
        errno = EFAULT;
        return NULL;
    }

    if (!*fs) {
		mtx_unlock(&mtx);
        errno = ENOENT;
        return NULL;
    }

	while (cmount->fs) {
		if (strcmp(fs, cmount->fs) == 0) {
			mtx_unlock(&mtx);
			return cmount;
		}

		cmount++;
    }

	mtx_unlock(&mtx);

	return NULL;
}

int mount(const char *target, const char *fs) {
    struct mount_pt *mount;

    if (!target || !fs) {
        errno = EFAULT;
        return -1;
    }

    if (!*target) {
        errno = ENOENT;
        return -1;
    }

	mtx_lock(&mtx);

	char *npath = mount_normalize_path(target);
    if (!npath) {
		mtx_unlock(&mtx);
    		return -1;
    }

    char *cnpath = npath;
    int count = 0;

    while (*cnpath) {
        if (*cnpath == '/') {
            count++;
        }

        cnpath++;
    }

    if (count != 1) {
        free(npath);
        errno = ENOTDIR;
        return -1;
    }

    if (!(mount = mount_get_mount_point_for_fs(fs))) {
    		free(npath);

    		mtx_unlock(&mtx);

    		// File system is not enabled
    		errno = ENODEV;
    		return -1;
    }

    if (mount_is_mounted(fs)) {
		free(npath);

		mtx_unlock(&mtx);

		// fs is mounted, can't be mounted again
		errno = EBUSY;
		return -1;
    }

    if (mount_get_mount_point_for_path(npath)) {
		free(npath);

		mtx_unlock(&mtx);

		// The is one file system mounted for this target
		errno = EINVAL;
		return -1;
    }

    if (mount->mount) {
    		if (!(mount->fpath = strdup(npath))) {
        		free(npath);

        		mtx_unlock(&mtx);

        		errno = ENOMEM;
    			return -1;
    		}

    		// Avoid mounting spiffs when lfs is mounted and vice-versa
    		if (	((strcmp(fs,"spiffs") == 0) && mount_is_mounted("lfs")) ||
    			((strcmp(fs,"lfs") == 0) && mount_is_mounted("spiffs"))) {
        		free(npath);
        		errno = EPERM;
    			return -1;
    		}

    		mount->mount();
    		mount->mounted = 1;
    }

	free(npath);

	mtx_unlock(&mtx);

	return 0;
}

int umount(const char *target) {
    if (!target) {
        errno = EFAULT;
        return -1;
    }

    if (!*target) {
        errno = ENOENT;
        return -1;
    }

    struct mount_pt *mount;

	mtx_lock(&mtx);

	char *npath = mount_normalize_path(target);
    if (!npath) {
		mtx_unlock(&mtx);

		return -1;
    }

    char *cnpath = npath;
    int count = 0;

    while (*cnpath) {
        if (*cnpath == '/') {
            count++;
        }

        cnpath++;
    }

    if (count != 1) {
        free(npath);
        errno = ENOTDIR;
        return -1;
    }

    if (!(mount = mount_get_mount_point_for_path(npath))) {
    		free(npath);

    		mtx_unlock(&mtx);

    		errno = EINVAL;
    		return -1;
    }

    if ((strcmp(npath, "/") == 0) && (mount_num() > 1)) {
		// If path is the root folder, check that no other file systems are mounted
		errno = EPERM;
		return -1;
    }

    if (mount->mounted) {
    		if (mount->umount) {
        		mount->umount();
        		mount->mounted = 0;

        		free(mount->fpath);
        		mount->fpath = NULL;
    		} else {
        		free(npath);

        		mtx_unlock(&mtx);

        		errno = EINVAL;
    			return -1;
    		}
    } else {
		free(npath);

		mtx_unlock(&mtx);

		errno = EBUSY;
		return -1;
    }

	free(npath);

	mtx_unlock(&mtx);

	return 0;
}
