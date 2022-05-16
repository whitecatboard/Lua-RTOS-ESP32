/*
 * Copyright (C) 2015 - 2020, IBEROXARXA SERVICIOS INTEGRALES, S.L.
 * Copyright (C) 2015 - 2020, Jaume Olivé Petrus (jolive@whitecatboard.org)
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
 * Lua RTOS path utilities
 *
 */

#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syslimits.h>
#include <sys/stat.h>

#include "path.h"
#include "mount.h"

static int mkpath_internal(const char *path, int last_is_file) {
    char current[PATH_MAX + 1]; // Current path
    char *dir;   // Current directory
    char *npath; // Normalized path

    // Normalize path
    npath = mount_normalize_path(path);
    if (!npath) {
        return -1;
    }

    // Current directory is empty
    strcpy(current, "");

    // Get the first directory in path
    dir = strtok(npath, "/");
    while (dir) {
        if (strstr(dir,".") != NULL) break;

        // Append current directory to current path
        strncat(current, "/", PATH_MAX);
        strncat(current, dir, PATH_MAX);

        // Get next directory in path
        dir = strtok(NULL, "/");

        if (!last_is_file || dir) {
            // Check the existence of the current path and create
            // it if it doesn't exists
            struct stat sb;

            if (stat(current, &sb) != 0 || !S_ISDIR(sb.st_mode)) {
                mkdir(current, 0755);
            }
        }
    }

    free(npath);

    return 0;
}

int mkpath(const char *path) {
    return mkpath_internal(path, 0);
}

int mkfile(const char *path) {
    char *npath; // Normalized path

    // Normalize path
    npath = mount_normalize_path(path);
    if (!npath) {
        return -1;
    }

    mkpath_internal(path, 1);

    FILE *fp = fopen(npath, "r");
    if (fp) {
        fclose(fp);
        free(npath);
        errno = EEXIST;
        return -1;
    } else {
        fp = fopen(npath, "a+");
    }

    fclose(fp);
    free(npath);

    return 0;
}
