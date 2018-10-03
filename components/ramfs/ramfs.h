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
 * Lua RTOS, RAM file system
 *
 */

/**
 * @brief RAM file system (RAMFS)
 *
 * The RAMFS is a POSIX-compliance file system entirely stored in RAM, without
 * persistence, which means that all the data stored in the file system is
 * lost on each reboot.
 *
 * In RAMFS, the file system is stored in a tree structure, in which there are
 * 2 types of nodes (entries): directories, and files.
 *
 * RAMFS structure overview:
 *
 * ----------
 * - RAMFS  -
 * ----------
 *      |
 *      | child
 *     \|/
 * ----------------   next    ----------------   next    ----------------
 * - directory or -  ------>  - file         -  ------>  - directory    -
 * - file entry   -           - entry        -           - entry        -
 * ----------------           ----------------           ----------------
 *                                   |                           |
 *                                   | header                    | child
 *                                  \|/                         \|/
 *                            ---------------            ----------------
 *                            - file header -            - directory or -
 *                            ---------------            - file entry   -
 *                                   |                   ----------------
 *                                   |
 *                                  \|/
 *                               ---------  next  ---------
 *                               - block -  ----> - block -
 *                               ---------        ---------
 */

#ifndef _RAMFS_H_
#define _RAMFS_H_

#include <stdint.h>
#include <stddef.h>
#include <limits.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mutex.h>

#if PATH_MAX > 64
#error "ramfs, PATH_MAX must be <= 64"
#endif

/*
 * To use the file system in a multi-threaded environment,
 * define the lock type (mutex) used in your platform.
 */
#define ramfs_lock_t struct mtx

/*
 * If ramfs_lock_t is defined, define the following macros:
 *
 * ramfs_lock_init(l): initialize / create the lock
 * ramfs_lock_destroy(l): destroy the lock
 * ramfs_lock(l): obtain the lock
 * ramfs_unlock(l): release the lock
 */
#ifdef ramfs_lock_t
#define ramfs_lock_init(l) \
    mtx_init(&l, NULL, NULL, 0);

#define ramfs_lock_destroy(l) \
    mtx_destroy(&l);

#define ramfs_lock(l) \
    mtx_lock(&l);

#define ramfs_unlock(l) \
    mtx_unlock(&l);
#else
#define ramfs_lock_init()
#define ramfs_lock_destroy()
#define ramfs_lock()
#define ramfs_unlock()
#endif

typedef int32_t ramfs_off_t;
typedef int32_t ramfs_size_t;

/**
 * @brief File system entry flags.
 *
 * bit0..bit0: entry type
 * bit1..bit6: length of the name of the entry
 * bit7..bit7: entry is deleted from the file system, but not still removed
 *
 */
typedef uint8_t ramfs_entry_flags_t;

#define RAMFS_ENTRY_TYPE_MSK     0b00000001
#define RAMFS_ENTRY_NAME_LEN_MSK 0b01111110
#define RAMFS_ENTRY_NAME_LEN_POS 1
#define RAMFS_ENTRY_RM_LEN_MSK   0b10000000

typedef enum {
    RAMFS_DIR  = 0,
    RAMFS_FILE = 1
} ramfs_entry_type_t;

typedef enum {
    RAMFS_ERR_OK          =  0,
    RAMFS_ERR_NOMEM       = -1,
    RAMFS_ERR_NOENT       = -2,
    RAMFS_ERR_EXIST       = -3,
    RAMFS_ERR_NOTDIR      = -4,
    RAMFS_ERR_BADF        = -5,
    RAMFS_ERR_ACCESS      = -6,
    RAMFS_ERR_NOSPC       = -7,
    RAMFS_ERR_INVAL       = -8,
    RAMFS_ERR_ISDIR       = -9,
    RAMFS_ERR_NOTEMPTY    = -10,
    RAMFS_ERR_BUSY        = -11,
    RAMFS_ERR_PERM        = -12,
    RAMFS_ERR_NAMETOOLONG = -13,
} ramfs_error_t;

typedef enum {
    RAMFS_O_RDONLY = 1,      // Open a file as read only
    RAMFS_O_WRONLY = 2,      // Open a file as write only
    RAMFS_O_RDWR   = 3,      // Open a file as read and write
    RAMFS_O_CREAT  = 0x0100, // Create a file if it does not exist
    RAMFS_O_EXCL   = 0x0200, // Fail if a file already exists
    RAMFS_O_TRUNC  = 0x0400, // Truncate the existing file to zero size
    RAMFS_O_APPEND = 0x0800, // Move to end of file on every write
} ramfs_flags_t;

#define RAMFS_ACCMODE (RAMFS_O_RDONLY | RAMFS_O_WRONLY | RAMFS_O_RDWR)

typedef enum {
    RAMFS_SEEK_SET = 1,
    RAMFS_SEEK_CUR = 2,
    RAMFS_SEEK_END = 3,
} ramfs_whence_t;

typedef struct ramfs_block {
    struct ramfs_block *next; /*!< Next block in chain */
    uint8_t data[1];          /*!< Block data */
} ramfs_block_t;

typedef struct ram_file_header {
    ramfs_block_t *head; /*!< File head */
    ramfs_block_t *tail; /*!< File tail */
    ramfs_size_t  size;  /*!< File size */
} ram_file_header_t;

typedef struct ramfs_entry {
    ramfs_entry_flags_t flags; /*!< Entry flags */
    struct ramfs_entry *next;  /*!< Next entry */
    union {
        struct {
            struct ram_file_header *header; /*!< File header */
        } file;
        struct {
            struct ramfs_entry *child; /*!< Entry type */
        } dir;
    };
    char name[1]; /*!< Entry name */
} ramfs_entry_t;

typedef struct {
    ramfs_off_t offset;   /*!< Current seek offset */
    ramfs_entry_t *entry; /*!< Directory entry */
    ramfs_entry_t *child; /*!< Directory child chain */
} ramfs_dir_t;

typedef struct {
    ramfs_entry_t *entry; /*!< File entry reference */
    uint32_t flags;       /*!< Open flags */
    ramfs_off_t offset;   /*!< Current seek offset */
    ramfs_block_t *block; /*!< Current read/write block */
    uint8_t *ptr;         /*!< Current read/write pointer into current block */
} ramfs_file_t;

typedef struct {
    char name[PATH_MAX + 1];
    ramfs_entry_type_t type;
    ramfs_size_t size;
} ramfs_info_t;

typedef struct ramfs_entry_ref {
    ramfs_entry_t *entry;
    uint8_t uses;
    struct ramfs_entry_ref *next;
} ramfs_entry_ref_t;

typedef struct {
    ramfs_entry_t *child;    /*!< Root directory child chain */
    ramfs_entry_ref_t *ref;  /*< Open references to file system entries */
    ramfs_size_t size;
    ramfs_size_t current_size;
    ramfs_size_t block_size;
#ifdef ramfs_lock_t
    ramfs_lock_t lock;
#endif
} ramfs_t;

typedef struct {
    ramfs_size_t size;
    ramfs_size_t block_size;
} ramfs_config_t;

int ramfs_mount(ramfs_t *fs, ramfs_config_t *config);
int ramfs_umount(ramfs_t *fs);
int ramfs_mkdir(ramfs_t *fs, const char *path);
int ramfs_dir_open(ramfs_t *fs, ramfs_dir_t *dir, const char *path);
int ramfs_dir_read(ramfs_t *fs, ramfs_dir_t *dir, ramfs_info_t *info);
int ramfs_dir_close(ramfs_t *fs, ramfs_dir_t *dir);
int ramfs_stat(ramfs_t *fs, const char *path, ramfs_info_t *info);
int ramfs_file_open(ramfs_t *fs, ramfs_file_t *file, const char *path, int flags);
int ramfs_file_sync(ramfs_t *fs, ramfs_file_t *file);
ramfs_size_t ramfs_file_read(ramfs_t *fs, ramfs_file_t *file, void *buffer, ramfs_size_t size);
ramfs_size_t ramfs_file_write(ramfs_t *fs, ramfs_file_t *file, const void *buffer, ramfs_size_t size);
int ramfs_file_close(ramfs_t *fs, ramfs_file_t *file);
ramfs_off_t ramfs_file_seek(ramfs_t *fs, ramfs_file_t *file, ramfs_off_t offset, ramfs_whence_t whence);
int ramfs_rename(ramfs_t *fs, const char *oldpath, const char *newpath);
int ramfs_rmdir(ramfs_t *fs, const char *path);
int ramfs_unlink(ramfs_t *fs, const char *pathname);
ramfs_off_t ramfs_telldir(ramfs_t *fs, ramfs_dir_t *dir);
int ramfs_file_truncate(ramfs_t *fs, ramfs_file_t *file, ramfs_off_t size);
int ramfs_file_stat(ramfs_t *fs, ramfs_file_t *file, ramfs_info_t *info);

#endif /* _RAMFS_H_ */
