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
 * Lua RTOS, file system mount functions
 *
 */

#ifndef _SYS_MOUNT_H
#define _SYS_MOUNT_H

#include <dirent.h>
#include <stdint.h>
#include <stddef.h>

typedef int (*mount_mount_f_t)(const char *);
typedef int (*mount_umount_f_t)(const char *);
typedef int (*mount_format_f_t)(const char *);
typedef int (*mount_fsstat_f_t)(const char *, uint32_t *total, uint32_t *used);

// Mount point structure
struct mount_pt {
    char *fpath;              // mount path
    const char *fs;           // target file system
    mount_mount_f_t mount;    // mount function
    mount_umount_f_t umount;  // unmount function
    mount_format_f_t format;  // format function
    mount_fsstat_f_t fsstat;  // fs stat function
    uint8_t mounted;          // Is the file system mounted?
};

/**
 * @brief Initialize the mount functions. Must be called before calling any mount
 *        function.
 */
void _mount_init();

/**
 * @brief In Lua RTOS each virtual file system is mounted into a logical file system,
 *        each of them mounted into a folder of the logical file system. This folder
 *        is known as the mount point.
 *
 *        For example, if support for SPIFFS and FAT file systems are enabled,
 *        SPIFFS can be mounted into the / folder, and FAT can be mounted into
 *        the /sd folder, while they are physically mounted into the /spiffs and
 *        /fat folders respectively.
 *
 *        A set of functions are provided to deal with logical (the used by the
 *        programmer) and physical paths (the used by Lua RTOS). For example:
 *
 *        Logical path    Physical path
 *        ------------    -------------
 *        /               /spiffs
 *        /a              /spiffs/a
 *        /sd             /fat
 *        /sd/a           /fat/a
 *
 */

/**
 * @brief Normalize a given path. A normalized path has the following characteristics:
 *
 *        - It is an absolute path (begins with the / character), so if the given path
 *          is a relative path, the current working is prepend to the path.
 *
 *        - It doesn't have references to the current directory (./), or the parent
 *          directory (../).
 *
 *        - It never ends with the / character, except if it is the root directory.
 *
 * @param path The path to be normalized.
 *
 * @return
 *     - A pointer to the normalized path, that is allocated into the heap. The
 *       programmer has the responsibility to free this pointer where no longer is
 *       needed.
 *
 *     - If an error occurs NULL is returned and errno is set to indicate the error.
 *
 *       ENOMEM: the is not enough space to allocate the normalized path.
 *       ENAMETOOLONG: the length of the normalized path exceeds PATH_MAX.
 */
char *mount_normalize_path(const char *path);

/**
 * @brief Resolve a logical path to a physical path.
 *
 * @param path The logical path to be resolved. This path can be a relative or an
 *             absolute path.
 *
 * @return
 *     - A pointer to the physical path, that is allocated into the heap. The
 *       programmer has the responsibility to free this pointer where no longer is
 *       needed.
 *
 *     - If an error occurs NULL is returned and errno is set to indicate the error.
 *
 *       ENOMEM: the is not enough space to allocate the physical path.
 */
char *mount_resolve_to_physical(const char *path);

/**
 * @brief Get the mount point name that corresponds to a given path into the logical
 *        file system.
 *
 * @param path The path.
 * @param buf A pointer to a buffer to store the mount point name.
 *
 * @return
 *     - NULL if there is not any point name for the given path.
 *     - The buff pointer.
 */
struct dirent* mount_readdir(DIR* pdir) ;

/*
 * @brief Get the location of the history file.
 */
char *mount_history_file(char *location, size_t loc_size);

/*
 * @brief Get the location of the messages.log file.
 */
char *mount_messages_file(char *location, size_t loc_size);

/*
 * @brief Get the root mount point.
 *
 * @return
 *     - A pointer to a mount_pt structure with the mount information
 *       for the file system which is mounter in the root folder.
 *
 *     - NULL if no file system is mounted in the root folder.
 */
struct mount_pt *mount_get_root();

/*
 * @brief Get the mount path for a given file system.
 *
 * @param fs The name of the file system.
 *
 * @return
 *     - The mount path.
 *     - NULL if the file system doesn't exists.
 */
const char *mount_get_mount_path(const char *fs);

/*
 * @brief Get the mount point for a given file path.
 *
 * @param path The path.
 *
 * @return
 *     - The mount point.
 *     - NULL if no file system matches.
 */
struct mount_pt *mount_get_mount_point_for_path(const char *path);

/*
 * @brief Get the mount point for a given file system.
 *
 * @param fs The file system.
 *
 * @return
 *     - The mount point.
 *     - NULL if no file system matches.
 */
struct mount_pt *mount_get_mount_point_for_fs(const char *fs);

int mount(const char *target, const char *fs);
int umount(const char *target);

#endif
