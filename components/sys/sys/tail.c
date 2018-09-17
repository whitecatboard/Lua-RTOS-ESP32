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
 * Lua RTOS, file tail utilities
 *
 */

#include "tail.h"

#include <limits.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int file_tails(const char *name, int length) {
    int size = 0;     // File size
    char c;           // Current character
    int tail_pos = 0; // File position to tail
    int res;

    FILE *fp = fopen(name,"r+");
    if (!fp) {
        return -1;
    }

    int wp, rp; // Write / read position

    // Compute the tail position in file
    int bytes = length;

    fseek(fp, 0, SEEK_SET);
    while (fread(&c,1,1,fp) == 1) {
        bytes--;
        tail_pos++;
        size++;

        if ((c == '\n') && (bytes <= 0)) {
            break;
        }
    }

    int current;
    char *buffer = calloc(1, tail_pos);
    if (!buffer) {
        fclose(fp);

        errno = ENOMEM;
        return -1;
    }

    rp = tail_pos;
    wp = 0;

    while ((current = fread(buffer, 1, length, fp)) == length)  {
        rp += current;
        size += current;

        fseek(fp, wp, SEEK_SET);
        fwrite(buffer, 1, current, fp);
        wp += current;
        fseek(fp, rp, SEEK_SET);
    }

    size += current;

    fseek(fp, wp, SEEK_SET);
    fwrite(buffer, 1, current, fp);

    if ((res = ftruncate(fileno(fp), size - tail_pos)) < 0) {
        free(buffer);
        return -1;
    }

    fclose(fp);

    free(buffer);

    return 0;
}
