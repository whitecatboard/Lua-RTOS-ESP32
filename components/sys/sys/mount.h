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

#ifndef _SYS_MOUNT_H
#define	_SYS_MOUNT_H

/**
 * @brief In Lua RTOS each virtual file system is mounted into a logical file system,
 *        each of them mounted into a folder of the logical file system. This folder
 *        is known as the mount point.
 *
 *        For example, if support for SPIFFS and FAT file systems are enabled,
 *        SPIFFS can be mounted into the / folder, and the FAT can be mounted into
 *        the /sd folder.
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
 * @brief Resolve a physical path to a logical path.
 *
 * @param path The physical path to be resolved. This path can be a relative or an
 *             absolute path.
 *
 * @return
 *     - A pointer to the logical path, that is allocated into the heap. The
 *       programmer has the responsibility to free this pointer where no longer is
 *       needed.
 *
 *     - If an error occurs NULL is returned and errno is set to indicate the error.
 *
 *       ENOMEM: the is not enough space to allocate the logical path.
 */
char *mount_resolve_to_logical(const char *path);

/**
 * @brief Check if a given file system is mounted.
 *
 * @param fs The file system name.
 *
 * @return 1 if it is mounted, or 0 if it is not mounted.
 */
int mount_is_mounted(const char *fs);

/**
 * @brief Mark the mount status (mounted or not mounted) of a given file system.
 *
 * @param fs The file system name.
 * @param mounted The file system mount status: 1 for mounted, 0 for not mounted.
 *
 */
void mount_set_mounted(const char *fs, unsigned int mounted);

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
char *mount_readdir(const char *path, char *buf);

#endif

