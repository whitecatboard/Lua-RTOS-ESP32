/*
 * Lua RTOS, minimal mount capabilities. Mount are only allowed in
 * the default root's directory.
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
#include <stddef.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

// Mount device structure
struct mountd {
    const char *device;
    unsigned int mounted;
};

// Mount point structure
struct mountp {
    const char *path;  // mount path
    const char *name;  // mount entry name
    const char *fpath; // mount full path (path + entry name)
    const char *odev;  // origin device
    const char *ddev;  // destination device
};

static const struct mountp mountps[] = {
#if USE_SPIFFS
#if USE_FAT
    {"/", "fat", "/fat", "spiffs", "fat"},
#endif
#endif
    {NULL, NULL, NULL, NULL}
};

struct mountd mountds[] = {
#if USE_SPIFFS
    {"spiffs", 0},
#endif
#if USE_FAT
    {"fat", 0},
#endif
    {NULL, 0}
};

// This function normalize a path passed as an argument, doing the
// following normalization actions:
//
// 1) If path is a relative path, preapend the current working directory
//
//    example:
//
//		if path = "blink.lua", and current working directory is "/examples", path to
//      normalize is "/examples/blink.lua"
//
// 2) Process ".." (parent directory) elements in path
//
//    example:
//
//		if path = "/examples/../autorun.lua" normalized path is "/autorun.lua"
//
// 3) Process "." (current directory) elements in path
//
//    example:
//
//		if path = "/./autorun.lua" normalized path is "/autorun.lua"
char *mount_normalize_path(const char *path) {
    char *rpath;
    char *cpath;
    char *tpath;
    char *last;
    int maybe_is_dot = 0;
    int maybe_is_dot_dot = 0;
    int is_dot = 0;
    int is_dot_dot = 0;
    int plen = 0;

    rpath = malloc(PATH_MAX);
    if (!rpath) {
        errno = ENOMEM;
        return NULL;
    }

    // If it's a relative path preappend current working directory
    if (*path != '/') {
        if (!getcwd(rpath, PATH_MAX)) {
            free(rpath);
            return NULL;
        }

        if (*(rpath + strlen(rpath) - 1) != '/') {
            rpath = strcat(rpath, "/");
        }

        rpath = strcat(rpath, path);
    } else {
        strcpy(rpath, path);
    }

    plen = strlen(rpath);
    if (*(rpath + plen - 1) != '/') {
        rpath = strcat(rpath, "/");
        plen++;
    }

    cpath = rpath;
    while (*cpath) {
        if (*cpath == '.') {
            if (maybe_is_dot) {
                maybe_is_dot_dot = 1;
                maybe_is_dot = 0;
            } else {
                maybe_is_dot = 1;
            }
        } else {
            if (*cpath == '/') {
                is_dot_dot = maybe_is_dot_dot;
                is_dot = maybe_is_dot && !is_dot_dot;
            } else {
                maybe_is_dot_dot = 0;
                maybe_is_dot = 0;
            }
        }

        if (is_dot_dot) {
            last = cpath + 1;

            while (*--cpath != '/');
            while (*--cpath != '/');

            tpath = ++cpath;
            while (*last) {
                *tpath++ = *last++;
            }
            *tpath = '\0';

            is_dot_dot = 0;
            is_dot = 0;
            maybe_is_dot = 0;
            maybe_is_dot_dot = 0;
            continue;
        }

        if (is_dot) {
            last = cpath + 1;

            while (*--cpath != '/');

            tpath = ++cpath;
            while (*last) {
                *tpath++ = *last++;
            }
            *tpath = '\0';

            is_dot_dot = 0;
            is_dot = 0;
            maybe_is_dot = 0;
            maybe_is_dot_dot = 0;
            continue;
        }

        cpath++;
    }

    cpath--;
    if ((cpath != rpath) && (*cpath == '/')) {
        *cpath = '\0';
    }

    return rpath;
}

const char *mount_default_device() {
    struct mountd *cmountd= &mountds[0];

    while (cmountd->device) {
        if (cmountd->mounted) {
            return cmountd->device;
        }
        
        cmountd++;
    }    
    
    return "";
}

// Set the mounted state of a device
void mount_set_mounted(const char *device, unsigned int mounted) {
    struct mountd *cmountd= &mountds[0];
    
    while (cmountd->device) {
        if (strcmp(cmountd->device, device) == 0) {
            cmountd->mounted = mounted;
        }
        
        cmountd++;
    }
}

int primary_is_mounted() {
    struct mountd *cmountd= &mountds[0];
    
    return cmountd->mounted;
}

int mount_is_mounted(const char *device) {
    struct mountd *cmountd= &mountds[0];
    
    while (cmountd->device) {
        if (strcmp(cmountd->device, device) == 0) {
            return cmountd->mounted;
        }
        
        cmountd++;
    }    
    
    return 0;
}

char *mount_primary_or_secondary(const char *path) {
    struct mountd *cmountd= &mountds[0];
    char npath[PATH_MAX];
    char *device = NULL;
    int primary;
    char *ret;

    primary = 1;
    while (cmountd->device) {
        if (cmountd->mounted) {
            device = (char *)cmountd->device;
            break;
        }
        
        cmountd++;
        primary = 0;
    }
    
    if (!device) {
        return NULL;
    }
    
    if (primary) {
        strcpy(npath, path);
    } else {
        strcpy(npath,"/");
        strcat(npath, device);
        strcat(npath, path);
    }
    
    ret = (char *)malloc(strlen(npath) + 1);
    if (!ret) {
        return NULL;
    }
    
    strcpy(ret, npath);
 
    return ret;
}

char *mount_secondary_or_primary(const char *path) {
    struct mountd *cmountd= &mountds[0];
    char npath[PATH_MAX];
    char *device = NULL;
    int secondary;
    char *ret;

    secondary = 0;
    while (cmountd->device) {
        if ((cmountd->mounted) && (secondary)) {
            device = (char *)cmountd->device;
            break;
        }
        
        cmountd++;
        secondary = 1;
    }
    
    if ((!secondary) || (!device)) {
        return mount_primary_or_secondary(path);
    } else {
        strcpy(npath,"/");
        strcat(npath, device);
        strcat(npath, path);
    }
    
    ret = (char *)malloc(strlen(npath) + 1);
    if (!ret) {
        return NULL;
    }
    
    strcpy(ret, npath);
    
    return ret;
}

// Get the number of mounter dirs for a device in a path
int mount_dirs(const char *dev, const char*path) {
    const struct mountp *cmount = &mountps[0];
    int mounted = 0;
        
    while (cmount->path) {
        if (strcmp(dev, cmount->odev) == 0) {
            if (strcmp(path, cmount->path) == 0) {
                mounted++;
            }
        }
        
        cmount++;
    }    
    
    return mounted;
}

// Get idx mounted directory entry for a device in a path
char *mount_readdir(const char *dev, const char*path, int idx, char *buf) {
    const struct mountp *cmount = &mountps[0];
    int actual = 0;

    while (cmount->path) {
        if (mount_is_mounted(cmount->ddev) && (strcmp(dev, cmount->odev) == 0)) {
            if (strcmp(path, cmount->path) == 0) {
                if (actual == idx) {
                    strcpy(buf, cmount->name);
                    return buf;
                }
                actual++;
            }
        }

        cmount++;
    }    
    
    return NULL;
}

// Get the device name where path is mounted. Path is an absolute path.
const char *mount_device(const char *path) {
    const struct mountp *cmount = &mountps[0];
    const char *device = NULL;
    const char *cpath;
    const char *cfpath;
    
    while (cmount->path) {
        cpath = path;
        cfpath = cmount->fpath;
        
        while (*cpath && *cfpath && (*cpath == *cfpath)) {
            cpath++;
            cfpath++;
        }
        
        if (!*cfpath) {
            if ((!*cpath) || (*cpath == '/')) {
                device = cmount->ddev;
                break;
            }
        }
        
        cmount++;
    }
 
    if (!device) {
        device = mount_default_device();
    }
    
    return device;    
}

// Get the root path where path is mounted.
// Root path is the absolute path without the mount path.
// Path is an absolute path.
char *mount_root(char *path) {
	const char *cpath;
	const char *cdevice;

	cpath = path;

	if (*cpath != '/') {
		return path;
	}

	cpath++;
	cdevice = mountds[0].device;

	while (*cpath && *cdevice) {
		if (*cpath == *cdevice) {
			cpath++;
			cdevice++;
		} else {
			break;
		}
	}

	if (!*cdevice) {
		return ((char *)cpath);
	} else {
		return path;
	}
}

char *mount_full_path(const char *path) {
	const char*device;

	char *npath;
	char *rpath;
	char *fpath;

	device = mount_device(path);
	if (device) {
		// Normalize path
		npath = mount_normalize_path(path);

		// Remove mount device from path
		rpath = mount_root(npath);

		fpath = (char *)malloc(PATH_MAX);

		strcpy(fpath,"/");

		fpath = strcat(fpath,device);
		fpath = strcat(fpath,rpath);

		free(npath);

		return fpath;
	} else {
		return (char *)path;
	}
}
