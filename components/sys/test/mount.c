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
 * Lua RTOS, sys mount test cases
 *
 */

#include "sdkconfig.h"

#include "unity.h"

#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/mount.h>
#include <sys/stat.h>

struct mount_path {
    const char *cwd;
    const char *a;
    const char *b;
};

static const struct mount_path norm_test[] = {
    {"/","","/"},
    {"/",".","/"},
    {"/","..","/"},
    {"/","./","/"},
    {"/","../","/"},
    {"/","../..","/"},
    {"/","../../","/"},
    {"/","//","/"},
    {"/","./a","/a"},
    {"/","./a/","/a"},
    {"/","a","/a"},
    {"/","a/b","/a/b"},
    {"/","a/../b","/b"},
    {"/","a/../b/c/../d","/b/d"},
    {"/","a/..///b/c/..///d","/b/d"},
    {"/","a/..///b/c/..///d//","/b/d"},
    {"/","a/./","/a"},
    {"/","a/.a/","/a/.a"},
    {"/","a/..a/","/a/..a"},
    {"/","a/.a./","/a/.a."},
    {"/","a/..a./","/a/..a."},
    {"/e","","/e"},
    {"/e",".","/e"},
    {"/e","..","/"},
    {"/e","./","/e"},
    {"/e","../","/"},
    {"/e","//","/"},
    {"/e","./a","/e/a"},
    {"/e","a","/e/a"},
    {"/e","a/","/e/a"},
    {"/e","a/b","/e/a/b"},
    {"/e","a/../b","/e/b"},
    {"/e","a/../b/c/../d","/e/b/d"},
    {"/e","a/..///b/c/..///d","/e/b/d"},
    {"/e","a/..///b/c/..///d//","/e/b/d"},
    {"/e","a/./","/e/a"},
    {"/e","a/.a/","/e/a/.a"},
    {"/e","a/..a/","/e/a/..a"},
    {"/e","a/.a./","/e/a/.a."},
    {"/e","a/..a./","/e/a/..a."},
    {NULL, NULL, NULL}
};

TEST_CASE("sys", "[mount_normalize_path]") {
    const struct mount_path *ctest;
    char *npath;
    char tmp[1024];

    mkdir("/e",0755);

    // mount_normalize_path test
    ctest = norm_test;
    while (ctest->cwd) {
        chdir(ctest->cwd);
        npath = mount_normalize_path(ctest->a);
        sprintf(tmp, "[%s|%s|%s] => %s", ctest->cwd, ctest->a, ctest->b, npath);
        TEST_ASSERT_MESSAGE(strcmp(npath,ctest->b) == 0, tmp);
        free(npath);
        ctest++;
    }

    rmdir("/e");
}
