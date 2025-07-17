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
 * Lua RTOS, sys file system test cases
 *
 */

#include "sdkconfig.h"

#include "unity.h"

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mount.h>

#include <dirent.h>

static void create_file(char *name, int size) {
    FILE *fp;
    int num = 0;

    fp = fopen(name, "w");

    while (size > 0) {
        fprintf(fp, "%d", num++);
        size--;
    }

    fclose(fp);

    errno = 0;
}

static void do_test(char *rootp) {
    int res;
    char root[PATH_MAX+1];
    char path[PATH_MAX+1];
    char old[PATH_MAX+1];
    char new[PATH_MAX+1];

    strcpy(root, rootp);

    if (strlen(rootp) > 1) {
        if (*(rootp + strlen(rootp) - 1) != '/') {
            strcat(root, "/");
        }
    }

    // a is a directory
    // d is a directory
    // a/b is a file
    // d/e is a file

    sprintf(path,"%s%s",root,"a/b");
    unlink(path);

    sprintf(path,"%s%s",root,"a/c");
    unlink(path);

    sprintf(path,"%s%s",root,"a/e");
    unlink(path);

    sprintf(path,"%s%s",root,"d/e");
    unlink(path);

    sprintf(path,"%s%s",root,"d/c");
    unlink(path);

    sprintf(path,"%s%s",root,"a");
    rmdir(path);

    sprintf(path,"%s%s",root,"d");
    rmdir(path);

    errno = 0;

    // ----------------------------------------------------------------
    // mkdir
    // ----------------------------------------------------------------

    // pathname points outside your accessible address space
    res = mkdir(NULL, 0777);
    TEST_ASSERT((res < 0) && (errno == EFAULT));
    errno = 0;

    res = mkdir("", 0777);
    TEST_ASSERT((res < 0) && (errno == ENOENT));
    errno = 0;

    // creete a
    sprintf(path,"%s%s",root,"a");
    res = mkdir(path, 0777);

    TEST_ASSERT((res == 0) && (errno == 0));
    errno = 0;

    // creqte d
    sprintf(path,"%s%s",root,"d");
    res = mkdir(path, 0777);

    TEST_ASSERT((res == 0) && (errno == 0));
    errno = 0;

    // pathname already exists
    sprintf(path,"%s%s",root,"a");
    res = mkdir(path, 0777);

    // create a/b
    sprintf(path,"%s%s",root,"a/b");
    create_file(path,10);

    // create d/e
    sprintf(path,"%s%s",root,"d/e");
    create_file(path,10);

    // pathname already exists
    sprintf(path,"%s%s",root,"a/b");
    res = mkdir(path, 0777);
    TEST_ASSERT((res < 0) && (errno == EEXIST));
    errno = 0;

    // a component used as a directory in pathname is not, in fact, a
    // directory
    sprintf(path,"%s%s",root,"a/b/c");
    res = mkdir(path, 0777);
    TEST_ASSERT((res < 0) && (errno == ENOTDIR));
    errno = 0;

    // A directory component in pathname does not exist
    sprintf(path,"%s%s",root,"a/c/c");
    res = mkdir(path, 0777);
    TEST_ASSERT((res < 0) && (errno == ENOENT));
    errno = 0;

    // ----------------------------------------------------------------
    // unlink
    // ----------------------------------------------------------------

    // pathname points outside your accessible address space
    res = unlink(NULL);
    TEST_ASSERT((res < 0) && (errno == EFAULT));
    errno = 0;

    res = unlink("");
    TEST_ASSERT((res < 0) && (errno == ENOENT));
    errno = 0;

    // the system does not allow unlinking of directories
    sprintf(path,"%s%s",root,"d");
    res = unlink(path);
    TEST_ASSERT((res < 0) && ((errno == EPERM) || (errno == EISDIR)));
    errno = 0;

    // a directory component in pathname does not exist
    sprintf(path,"%s%s",root,"a/c/c");
    res = unlink(path);
    TEST_ASSERT((res < 0) && (errno == ENOENT));
    errno = 0;

    sprintf(path,"%s%s",root,"a/b");
    res = unlink(path);
    TEST_ASSERT((res == 0) && (errno == 0));
    errno = 0;

    sprintf(path,"%s%s",root,".");
    res = unlink(path);
    TEST_ASSERT((res < 0) && ((errno == EINVAL) || (errno == EISDIR)));
    errno = 0;

    sprintf(path,"%s%s",root,"..");
    res = unlink(path);
    TEST_ASSERT((res < 0) && ((errno == EACCES) || (errno == EISDIR)));
    errno = 0;

    // create a/b
    sprintf(path,"%s%s",root,"a/b");
    create_file(path,10);

    // ----------------------------------------------------------------
    // rmdir
    // ----------------------------------------------------------------

    // pathname points outside your accessible address space
    res = rmdir(NULL);
    TEST_ASSERT((res < 0) && (errno == EFAULT));
    errno = 0;

    res = rmdir("");
    TEST_ASSERT((res < 0) && (errno == ENOENT));
    errno = 0;

    // the path argument names a directory that is not an empty directory
    sprintf(path,"%s%s",root,"d");
    res = rmdir(path);
    TEST_ASSERT((res < 0) && (errno == ENOTEMPTY));
    errno = 0;

    sprintf(path,"%s%s",root,"d/e");
    unlink(path);
    errno = 0;

    sprintf(path,"%s%s",root,"d");
    res = rmdir(path);
    TEST_ASSERT((res == 0) && (errno == 0));
    errno = 0;

    // A component of path is not a directory
    sprintf(path,"%s%s",root,"a/b");
    res = rmdir(path);
    TEST_ASSERT((res < 0) && (errno == ENOTDIR));
    errno = 0;

    // the path argument contains a last component that is dot.
    sprintf(path,"%s%s",root,".");
    res = rmdir(path);
    TEST_ASSERT((res < 0) && (errno == EINVAL));
    errno = 0;

    sprintf(path,"%s%s",root,"..");
    res = rmdir(path);
    TEST_ASSERT((res < 0) && ((errno == EACCES) || (errno == ENOTEMPTY)));
    errno = 0;

    // A component of path does not name an existing file
    sprintf(path,"%s%s",root,"a/c");
    res = rmdir(path);
    TEST_ASSERT((res < 0) && (errno == ENOENT));
    errno = 0;

    // ----------------------------------------------------------------
    // rename
    // ----------------------------------------------------------------
    sprintf(path,"%s%s",root,"d");
    mkdir(path, 0777);
    errno = 0;

    sprintf(path,"%s%s",root,"a/b");
    create_file(path,10);
    errno = 0;

    sprintf(path,"%s%s",root,"d/e");
    create_file(path,10);
    errno = 0;

    // the link named by old does not name an existing file, or either old or new points to an empty string
    res = rename("", "");
    TEST_ASSERT((res < 0) && (errno == ENOENT));
    errno = 0;

    sprintf(old,"%s%s",root,"a");
    res = rename(old, "");
    TEST_ASSERT((res < 0) && (errno == ENOENT));
    errno = 0;

    sprintf(new,"%s%s",root,"f");
    res = rename("", new);
    TEST_ASSERT((res < 0) && (errno == ENOENT));
    errno = 0;

    sprintf(old,"%s%s",root,"g");
    sprintf(new,"%s%s",root,"f");
    res = rename("", new);
    TEST_ASSERT((res < 0) && (errno == ENOENT));
    errno = 0;

    sprintf(old,"%s%s",root,"a/c");
    sprintf(new,"%s%s",root,"a/d");
    res = rename(old, new);
    TEST_ASSERT((res < 0) && (errno == ENOENT));
    errno = 0;

    // the new argument points to a directory and the old argument points to a file that is not a directory
    sprintf(old,"%s%s",root,"a/b");
    sprintf(new,"%s%s",root,"a");
    res = rename(old, new);
    TEST_ASSERT((res < 0) && ((errno == EISDIR) || (errno == ENOTEMPTY)));
    errno = 0;

    sprintf(old,"%s%s",root,"a/b");
    sprintf(new,"%s%s",root,"d");
    res = rename(old, new);
    TEST_ASSERT((res < 0) && (errno == EISDIR));
    errno = 0;

    // the link named by new is a directory that is not an empty directory
    sprintf(old,"%s%s",root,"a");
    sprintf(new,"%s%s",root,"d");
    res = rename(old, new);
    TEST_ASSERT((res < 0) && (errno == ENOTEMPTY));
    errno = 0;

    // other errors
    res = rename(NULL, NULL);
    TEST_ASSERT((res < 0) && (errno == EFAULT));
    errno = 0;

    sprintf(old,"%s%s",root,"a");
    res = rename(old, NULL);
    TEST_ASSERT((res < 0) && (errno == EFAULT));
    errno = 0;

    sprintf(new,"%s%s",root,"f");
    res = rename(NULL, new);
    TEST_ASSERT((res < 0) && (errno == EFAULT));
    errno = 0;

    sprintf(old,"%s%s",root,"g");
    sprintf(new,"%s%s",root,"f");
    res = rename(NULL, new);
    TEST_ASSERT((res < 0) && (errno == EFAULT));
    errno = 0;

    sprintf(old,"%s%s",root,"a/c");
    sprintf(old,"%s%s",root,"a/d");
    res = rename(NULL, new);
    TEST_ASSERT((res < 0) && (errno == EFAULT));
    errno = 0;

    // valid conditions
    sprintf(old,"%s%s",root,"a/b");
    sprintf(new,"%s%s",root,"a/c");
    res = rename(old, new);
    TEST_ASSERT((res == 0) && (errno == 0));

    sprintf(old,"%s%s",root,"a/c");
    sprintf(new,"%s%s",root,"d/c");
    res = rename(old, new);
    TEST_ASSERT((res == 0) && (errno == 0));

    sprintf(old,"%s%s",root,"d");
    sprintf(new,"%s%s",root,"a");
    res = rename(old, new);
    TEST_ASSERT((res == 0) && (errno == 0));

    // ----------------------------------------------------------------
    // opendir
    // ----------------------------------------------------------------
    DIR *dir;

    // Directory does not exist, or name is an empty string
    dir = opendir("");
    TEST_ASSERT((dir == NULL) && (errno == ENOENT));
    errno = 0;

    sprintf(path,"%s%s",root,"b");
    dir = opendir(path);
    TEST_ASSERT((dir == NULL) && (errno == ENOENT));
    errno = 0;

    // name is not a directory
    sprintf(path,"%s%s",root,"a/c");
    dir = opendir(path);
    TEST_ASSERT((dir == NULL) && (errno == ENOTDIR));
    errno = 0;

    // valid conditions
    sprintf(path,"%s%s",root,"a");
    dir = opendir(path);
    TEST_ASSERT((dir != NULL) && (errno == 0));
    errno = 0;

    // ----------------------------------------------------------------
    // readdir
    // ----------------------------------------------------------------
    struct dirent *de;
    int pending = 2;

    while (pending > 0){
        errno = 0;
        de = readdir(dir);
        TEST_ASSERT((de != NULL) && (errno == 0));

        if ((strcmp(de->d_name,"c") == 0) || (strcmp(de->d_name,"e") == 0)) {
            pending--;
        }
    }

    errno = 0;
    de = readdir(dir);
    TEST_ASSERT((de == NULL) && (errno == 0));

    closedir(dir);

    errno = 0;
    de = readdir(dir);
    TEST_ASSERT((de == NULL) && (errno == EBADF));

    // ----------------------------------------------------------------
    // stat
    // ----------------------------------------------------------------
    struct stat st;

    sprintf(path,"%s%s",root, "a/g");
    res = stat(path, &st);
    TEST_ASSERT((res < 0) && (errno == ENOENT));
    errno = 0;

    // valid conditions
    sprintf(path,"%s%s",root, "a");
    res = stat(path, &st);
    TEST_ASSERT((res == 0) && (errno == 0) && ((st.st_mode & S_IFMT) == S_IFDIR));
    errno = 0;

    sprintf(path,"%s%s",root, "a/c");
    res = stat(path, &st);
    TEST_ASSERT((res == 0) && (errno == 0) && ((st.st_mode & S_IFMT) == S_IFREG));
    errno = 0;
}

TEST_CASE("sys", "[file system]") {
#if CONFIG_LUA_RTOS_USE_SPIFFS
    printf("Testing spiffs ...\r\n");
   do_test("/");
#endif

#if (CONFIG_SD_CARD_MMC || CONFIG_SD_CARD_SPI) && CONFIG_LUA_RTOS_USE_FAT
   printf("Testing fat ...\r\n");
   mount("/sd", "fat");
   do_test("/sd");
   umount("/sd");
#endif

#if CONFIG_LUA_RTOS_USE_RAM_FS
   printf("Testing ramfs ...\r\n");
   mount("/ramfs", "ramfs");
   do_test("/ramfs");
   umount("/ramfs");
#endif

#if CONFIG_LUA_RTOS_USE_LFS
   printf("Testing lfs ...\r\n");

   umount("/");
   mount("/", "lfs");
   do_test("/");
   umount("/");
#endif

mount("/","spiffs");

}
