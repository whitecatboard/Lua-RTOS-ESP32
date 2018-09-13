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
 * Lua RTOS, shell history functions
 *
 */

#include "history.h"

#include <errno.h>
#include <string.h>
#include <stdio.h>

#include <sys/tail.h>
#include <sys/mount.h>
#include <sys/status.h>
#include <sys/path.h>

static void history_add_internal(const char *buf) {
    // Get the history file name
    char fname[PATH_MAX + 1];
    if (!mount_history_file(fname, sizeof(fname))) {
        return;
    }

    int retries = 0;

again: {
        // Open the history file
        FILE *fp = fopen(fname,"a");
        if (!fp) {
            // File not found, try to create it, and open again
            mkfile(fname);
            fp = fopen(fname,"a");
            if (!fp) {
                return;
            }
        }

        // Get file size
        long size;

        if ((size = ftell(fp)) < 0) {
            fclose(fp);
            return;
        }

        // Write to history file
        int len = strlen(buf);

        int no_space = 0;

        if (fwrite(buf, 1, len, fp) != len) {
            no_space = (errno == ENOSPC);
        }

        if (!no_space) {
            if (fflush(fp) == EOF) {
                no_space = (errno == ENOSPC);
            }
        }

        if (no_space) {
            // Truncate file to the original size
            if (ftruncate(fileno(fp), size) < 0) {
                fclose(fp);
                return;
            }

            if (retries == 0) {
                // Tail file
                fclose(fp);
                file_tails(fname, fp->_blksize);

                retries++;
                goto again;
            }
        }

        fclose(fp);
    }
}

void history_add(const char *line) {
    history_add_internal("\n");
    history_add_internal(line);
}

int history_get(int index, int up, char *buf, int buflen) {
    // Get the history file name
    char fname[PATH_MAX + 1];
    if (!mount_history_file(fname, sizeof(fname))) {
        return -2;
    }

    // Open history file
    FILE *fp = fopen(fname,"r");
    if (!fp) {
        // File not found, try to create it, and open again
        mkfile(fname);
        fp = fopen(fname,"r");
        if (!fp) {
            return -2;
        }
    }

    int pos, len, new_pos;
    char c;

    if (index == -1) {
        fseek(fp, 0, SEEK_END);
        pos = ftell(fp);
    } else {
        pos = index;
    }

    if ((pos == 0) && (up)) {
        fclose(fp);
        return -3;
    }

    new_pos = pos;

    while ((pos >= 0) && !feof(fp)) {
        if (up)
            fseek(fp, --pos, SEEK_SET);
        else
            fseek(fp, ++pos, SEEK_SET);

        c = fgetc(fp);
        if (c == '\n') {
            break;
        }
    }

    new_pos = pos;

    if  ((pos >= 0) && !feof(fp)) {
        // Get line
        fgets(buf, buflen, fp);
        len = strlen(buf);

        // Strip \r \n chars at the end, if present
        while ((buf[len - 1] == '\r') || (buf[len - 1] == '\n')) {
            len--;
        }

        buf[len] = '\0';
    } else {
        if (feof(fp))
            new_pos = -1;
        else
            new_pos = 0;

        buf[0] = 0;
    }

    fclose(fp);

    return new_pos;
}
