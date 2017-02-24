/*
 * Lua RTOS, minimal mount capabilities. Mount are only allowed in
 * the default root's directory.
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

#include <stddef.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

char *getcwd(char *buf, size_t size);

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

// Current mount points.
//
// Take note that we only describe secundary mount points.
//
// Primary file system is always mount to the / directory.
static const struct mountp mountps[] = {
#if USE_SPIFFS
#if USE_FAT
    {"/", "sd", "/sd", "spiffs", "fat"},
#endif
#endif
    {NULL, NULL, NULL, NULL, NULL}
};

// Current mounted devices
struct mountd mountds[] = {
#if USE_SPIFFS
    {"spiffs", 0},
#endif
#if USE_FAT
    {"fat", 0},
#endif
    {NULL, 0}
};

// Get the mounted device that corresponds to path (supposed to be a physical path)
//
// Path has format [/]xxx[/]yyyyy, where xxx is the device name.
//
// When xxx path's part is extracted function search into mountds structure where
// mounted devices are defined, and if some entry matches with xxx, xxx is
// returned as a device name, and [/]yyyyy is returned in rpath.
const char *mount_get_device_from_path(const char *path, char **rpath) {
	const char *cpath;
	const char *devices;
	const char *devicee;
	struct mountd *cmount;

	// Extract the device from path
	cpath = path;
	if (*cpath == '/') {
		cpath++;
	}

	devices = cpath;
	devicee = cpath;
	while (*devicee && (*devicee != '/')) {
		devicee++;
	}

	if (*devicee) {
		devicee--;
	}

	// Device name is beetween devicee and devices
	// Check if it's a mounted device
	cmount = mountds;
	while (cmount->device) {
		if (strncmp(cmount->device, devices, devicee - devices + 1) == 0) {
			*rpath = (char *)(devicee + 1);
			return cmount->device;
		}
		cmount++;
	}

	*rpath = (char *)path;

	return NULL;
}

// Get the mounted path that corresponds to path (supposed to be a logical path)
//
// Path has format [/]xxx[/]yyyyy, where xxx is the mount path.
//
// When xxx path's part is extracted function search into mountps structure where
// mounted paths are defined, and if some entry matches with xxx, xxx is
// returned as the mount path, and [/]yyyyy is returned in rpath.
const char *mount_get_mount_from_path(const char *path, char **rpath) {
	const char *cpath;
	const char *mounts;
	const char *mounte;
	const struct mountp *cmountp;

	// Extract the device from path
	cpath = path;

	if (*cpath == '/') {
		cpath++;
	}

	mounts = cpath;
	mounte = cpath;
	while (*mounte && (*mounte != '/')) {
		mounte++;
	}

	if (*mounte) {
		mounte--;
	}

	// Device name is beetween devicee and devices
	// Check if it's a mounted device
	cmountp = mountps;
	while (cmountp->path) {
		if (strncmp(cmountp->fpath + 1, mounts, mounte - mounts + 1) == 0) {
			*rpath = (char *)(mounte + 1);
			return cmountp->fpath;
		}
		cmountp++;
	}

	*rpath = (char *)path;

	return NULL;
}

// Gets the mount path for a device
//
// This function searches into the mountp struct, where mount points
// are defined.
const char *mount_device_mount_path(const char *device) {
    const struct mountp *cmount = &mountps[0];

    while (cmount->path) {
    	if (strcmp(device,cmount->ddev) == 0) {
    		return cmount->fpath;
    	}
        cmount++;
    }

    return "";
}

// Sets the mounted state of a device
void mount_set_mounted(const char *device, unsigned int mounted) {
    struct mountd *cmountd= &mountds[0];

    while (cmountd->device) {
        if (strcmp(cmountd->device, device) == 0) {
            cmountd->mounted = mounted;
        }

        cmountd++;
    }
}

// Get the default mounted device. This device is the first mounted
// device. It's mounted on the / directory.
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

    rpath = malloc(PATH_MAX + 1);
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
            rpath = strncat(rpath, "/", PATH_MAX);
        }

        rpath = strncat(rpath, path, PATH_MAX);
    } else {
        strncpy(rpath, path, PATH_MAX);
    }

    plen = strlen(rpath);
    if (*(rpath + plen - 1) != '/') {
        rpath = strncat(rpath, "/", PATH_MAX);
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

// Get the mount path where path is mounted. Path is an absolute path.
const char *mount_path(const char *path) {
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
                device = cmount->fpath;
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

// Resolve a path (supposed to be a logical path) to a physical path.
// First path is normalized for get a path started with / and no reference to .. and . folders.
// Once normalized we determine the device in which path is mounted and physical path is builded.
//
// Example:
//
// If path is /sd/examples/lua/.., function returns /fat/examples
char *mount_resolve_to_physical(const char *path) {
	const char*device;

	char *npath;
	char *rpath;
	char *ppath;

	// Normalize path
	npath = mount_normalize_path(path);

	// Get the device where path is mounted
	if ((device = mount_device(npath))) {
		// Remove mount device from path, if any
		mount_get_mount_from_path(npath, &rpath);

		// Allocate space for physical path, and build it
		ppath = (char *)malloc(PATH_MAX + 1);
		if (!ppath) {
			free(npath);
			return NULL;
		}

		strncpy(ppath,"/", PATH_MAX);

		ppath = strncat(ppath,device, PATH_MAX);

		if (*rpath != '/') {
			ppath = strncat(ppath,"/", PATH_MAX);
		}

		ppath = strncat(ppath,rpath, PATH_MAX);
	} else {
		// Allocate space for physical path, and build it
		ppath = (char *)malloc(PATH_MAX + 1);
		if (!ppath) {
			free(npath);
			return NULL;
		}

		strncpy(ppath, npath, PATH_MAX);
	}

	free(npath);

	return ppath;
}

// Resolve a path (supposed to be a physical path) to a logical path.
// First path is normalized for get a path started with / and no reference to .. and . folders.
// Once normalized we determine the mount point and logical path is builded.
//
// Example:
//
// If path is /fat/examples/lua/.., function returns /sd/examples
char *mount_resolve_to_logical(const char *path) {
	const char *device;
	char *rpath;
	const char *mount_path;
	char *lpath;
	char *npath;

	lpath = NULL;

	// Normalize path
	npath = mount_normalize_path(path);

	if ((device = mount_get_device_from_path(path, &rpath))) {
		mount_path = mount_device_mount_path(device);
		if (mount_path) {
			lpath = (char *)malloc(PATH_MAX + 1);
			if (!lpath) {
				free(npath);
				return NULL;
			}

			strncpy(lpath, mount_path, PATH_MAX);

			if (*rpath != '/') {
				lpath = strncat(lpath,"/", PATH_MAX);
			}

			lpath = strncat(lpath,rpath, PATH_MAX);

			if (*(lpath + strlen(lpath) - 1) == '/') {
				*(lpath + strlen(lpath) - 1) = '\0';
			}
		}
	} else {
		lpath = (char *)malloc(PATH_MAX + 1);
		if (!lpath) {
			free(npath);
			return NULL;
		}

		strncpy(lpath, npath, PATH_MAX);
	}

	free(npath);

	return lpath;
}
