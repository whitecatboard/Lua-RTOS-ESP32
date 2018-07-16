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

// File system mount structure
struct moun_fs {
    const char *fs;  // File system name
    uint8_t mounted; // Is the file system mounted?
};

// Mount point structure
struct mount_pt {
    const char *path;  // mount path
    const char *name;  // mount entry name
    const char *fpath; // mount full path (path + entry name)
    const char *fs;    // target file system
};

// Current mount points
static const struct mount_pt mountps[] = {
#if CONFIG_SD_CARD_MMC || CONFIG_SD_CARD_SPI
    {"/", "sd", "/sd", "fat"},
#endif
    {"/", "dev", "/dev", NULL},
#if CONFIG_LUA_RTOS_USE_SPIFFS
    {"/", "", "/", "spiffs"},
#endif
    {NULL, NULL, NULL, NULL, NULL}
};

// Current mounted file systems
struct moun_fs mount_fs[] = {
#if CONFIG_LUA_RTOS_USE_SPIFFS
    {"spiffs", 0},
#endif
#if CONFIG_SD_CARD_MMC || CONFIG_SD_CARD_SPI
    {"fat", 0},
#endif
    {NULL, 0}
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

static const char *mount_get_mount_path(const char *fs) {
    const struct mount_pt *cmount = &mountps[0];

    while (cmount->path) {
        if (strcmp(fs, cmount->fs) == 0) {
            return cmount->fpath;
        }

        cmount++;
    }

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
            is_slash = 0;

            continue;
        } else if (is_dot) {
            cpath = dot(rpath, cpath);

            is_dot = 0;
            is_dot_dot = 0;
            maybe_is_dot = 0;
            maybe_is_dot_dot = 0;
            is_slash = 0;

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

static const char *mount_get_fs_from_physical_path(const char *path, char **rpath) {
    // Extract the file system from the physical path
    const char *cpath = path;
    if (*cpath++ != '/') {
        return NULL;
    }

    const char *fs_s = cpath;
    const char *fs_e = cpath;
    while (*fs_e && (*fs_e != '/')) {
        fs_e++;
    }

    fs_e--;

    // File system name is between fs_e and fs_s
    struct moun_fs *cmount = mount_fs;
    while (cmount->fs) {
        if (strncmp(cmount->fs, fs_s, fs_e - fs_s + 1) == 0) {
            *rpath = (char *)(fs_e + 1);

            return cmount->fs;
        }
        cmount++;
    }

    *rpath = (char *)path;

    return NULL;
}

static const char *mount_get_fs_from_logical_path(const char *path, char **rpath) {
    const struct mount_pt *cmount = &mountps[0];
    const char *fs_e = NULL;
    const char *cpath;
    const char *cfpath;

    while (cmount->path) {
        cpath = path;
        cfpath = cmount->fpath;

        while (*cpath && *cfpath && (*cpath == *cfpath)) {
            cpath++;
            cfpath++;
        }

        if ((!*cfpath) && ((*cpath == '/') || (!*cpath) || (strlen(cmount->fpath) == 1))) {
            fs_e = cmount->fs;

            if (rpath) {
                *rpath = (char *)cpath;
            }

            break;
        }

        cmount++;
    }

    return fs_e;
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

char *mount_resolve_to_logical(const char *path) {
    // Normalize path
    char *npath = mount_normalize_path_internal(path, 0);
    if (!npath) {
        return NULL;
    }

    const char *fs;
    char *rpath;
    char *lpath;

    lpath = NULL;
    if ((fs = mount_get_fs_from_physical_path(npath, &rpath))) {
        // Get mount path for file system
        const char *mount_path = mount_get_mount_path(fs);
        if (mount_path) {
            if (!(lpath = (char *)calloc(1, strlen(mount_path) + strlen(rpath) + 1))) {
                errno = ENOMEM;
                free(npath);
                return NULL;
            }

            if ((*(mount_path + strlen(mount_path) - 1) != '/') || (strlen(rpath) == 0)) {
                strcat(lpath, mount_path);
            }

            strcat(lpath, rpath);
        }
    }

    free(npath);

    return lpath;;
}

int mount_is_mounted(const char *fs) {
    struct moun_fs *cmount= &mount_fs[0];

    while (cmount->fs) {
        if (strcmp(cmount->fs, fs) == 0) {
            return cmount->mounted;
        }

        cmount++;
    }

    return 0;
}

void mount_set_mounted(const char *fs, unsigned int mounted) {
    struct moun_fs *cmount= &mount_fs[0];

    while (cmount->fs) {
        if (strcmp(cmount->fs, fs) == 0) {
            cmount->mounted = mounted;
        }

        cmount++;
    }
}

char *mount_readdir(const char *path, char *buf) {
    const struct mount_pt *cmount = &mountps[0];

    if (!buf) return NULL;

    while (cmount->path) {
        if (strcmp(path, cmount->path) == 0) {
            strcpy(buf, cmount->name);
            return buf;
        }

        cmount++;
    }

    return NULL;
}
