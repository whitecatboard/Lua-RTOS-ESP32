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
 * Lua RTOS, ROM file system
 *
 */

/**
 * @brief ROM file system (ROMFS)
 *
 * The ROMFS is a POSIX-compliance file system entirely stored in ROM,
 * which means that all the data stored in the file system is read-only.
 * Due to the fact that ROMFS is read-only and resides in ROM, ROMFS
 * must be always be created in a host computer with the mkromfs utility,
 * which creates a binary image of the file system. All data is stored in
 * little endian.
 *
 * In ROMFS, the file system is stored in a tree structure, in which there are
 * 2 types of nodes (entries): directories, and files.
 *
 * ROMFS structure overview:
 *
 * ----------
 * - ROMFS  -
 * ----------
 *      |
 *      | child
 *     \|/
 * ----------------   next    ----------------   next    ----------------
 * - directory or -  ------>  - file         -  ------>  - directory    -
 * - file entry   -           - entry        -           - entry        -
 * ----------------           ----------------           ----------------
 *                                   |                           |
 *                                   | content                   | child
 *                                  \|/                         \|/
 *                            ----------------           ----------------
 *                            - file content -           - directory or -
 *                            ----------------           - file entry   -
 *                                                       ----------------
 */

#ifndef _ROMFS_H_
#define _ROMFS_H_

#include <stdint.h>
#include <stddef.h>
#include <limits.h>
#include <unistd.h>
#include <sys/types.h>

#include <endian.h>

#if (BYTE_ORDER == BIG_ENDIAN)
#define htole16(x) __builtin_bswap16(x)
#define htole32(x) __builtin_bswap32(x)
#define le16toh(x) __builtin_bswap16(x)
#define le32toh(x) __builtin_bswap32(x)
#else
#define htole16(x) (x)
#define htole32(x) (x)
#define le16toh(x) (x)
#define le32toh(x) (x)
#endif

#define ROMFS_PA(t, addr) ((t)((((uint32_t)addr) == 0xffffffff)?NULL:(fs->base + ((uint32_t)addr))))
#define ROMFS_VA(addr) ((romfs_ptr_t)((addr == NULL)?0xffffffff:(((uint32_t)addr) - ((uint32_t)fs->base))))

typedef int32_t  romfs_off_t;
typedef int32_t  romfs_size_t;
typedef uint32_t romfs_ptr_t;

/**
 * @brief File system entry flags.
 *
 * bit0..bit0: entry type
 * bit1..bit6: length of the name of the entry
 * bit7..bit7: entry is deleted from the file system, but not still removed
 *
 */
typedef uint8_t romfs_entry_flags_t;

#define ROMFS_ENTRY_TYPE_MSK     0b00000001
#define ROMFS_ENTRY_NAME_LEN_MSK 0b01111110
#define ROMFS_ENTRY_NAME_LEN_POS 1
#define ROMFS_ENTRY_RM_LEN_MSK   0b10000000

typedef enum {
    ROMFS_DIR  = 0,
    ROMFS_FILE = 1
} romfs_entry_type_t;

typedef enum {
    ROMFS_ERR_OK          =  0,
    ROMFS_ERR_NOMEM       = -1,
    ROMFS_ERR_NOENT       = -2,
    ROMFS_ERR_EXIST       = -3,
    ROMFS_ERR_NOTDIR      = -4,
    ROMFS_ERR_BADF        = -5,
    ROMFS_ERR_ACCESS      = -6,
    ROMFS_ERR_NOSPC       = -7,
    ROMFS_ERR_INVAL       = -8,
    ROMFS_ERR_ISDIR       = -9,
    ROMFS_ERR_NOTEMPTY    = -10,
    ROMFS_ERR_BUSY        = -11,
    ROMFS_ERR_PERM        = -12,
    ROMFS_ERR_NAMETOOLONG = -13,
} romfs_error_t;

typedef enum {
    ROMFS_O_RDONLY = 1,      // Open a file as read only
#ifdef MKROMFS
    ROMFS_O_WRONLY = 2,      // Open a file as write only
    ROMFS_O_RDWR   = 3,      // Open a file as read and write
    ROMFS_O_CREAT  = 0x0100, // Create a file if it does not exist
#endif
} romfs_flags_t;

#ifdef MKROMFS
#define ROMFS_ACCMODE (ROMFS_O_RDONLY | ROMFS_O_WRONLY | ROMFS_O_RDWR)
#else
#define ROMFS_ACCMODE (ROMFS_O_RDONLY)
#endif

typedef enum {
    ROMFS_SEEK_SET = 1,
    ROMFS_SEEK_CUR = 2,
    ROMFS_SEEK_END = 3,
} romfs_whence_t;

typedef struct romfs_file_content {
    romfs_size_t size;  /*!< File size */
#ifdef MKROMFS
    romfs_ptr_t data;      /*!< File data */
#else
    uint8_t *data;         /*!< File data */
#endif
} romfs_file_content_t;

#pragma pack(push)  /* push current alignment to stack */
#pragma pack(1)     /* set alignment to 1 byte boundary */

typedef struct {
    romfs_entry_flags_t flags;   /*!< Entry flags */
    romfs_ptr_t next;
    union {
        struct {
            romfs_ptr_t content; /*!< File content */
        } file;
        struct {
            romfs_ptr_t child;   /*!< Entry type */
        } dir;
    };
    char name[1]; /*!< Entry name */
} romfs_entry_t;

#pragma pack(pop)   /* restore original alignment from stack */

#define ROMFS_MAX_ENTRY_SIZE (sizeof(romfs_entry_t) + PATH_MAX - 1)

typedef struct {
    romfs_off_t   offset; /*!< Current seek offset */
    romfs_entry_t *entry; /*!< Directory entry */
    romfs_entry_t *child; /*!< Directory child chain */
} romfs_dir_t;

typedef struct {
    romfs_entry_t *entry; /*!< File entry reference */
    uint32_t flags;       /*!< Open flags */
    romfs_off_t offset;   /*!< Current seek offset */
    uint8_t *ptr;         /*!< Current read/write pointer into file data */
} romfs_file_t;

typedef struct {
    char name[PATH_MAX + 1];
    romfs_entry_type_t type;
    romfs_size_t size;
} romfs_info_t;

#pragma pack(push)  /* push current alignment to stack */
#pragma pack(1)     /* set alignment to 1 byte boundary */

#pragma pack(pop)   /* restore original alignment from stack */

typedef struct {
    romfs_entry_t *child;      /*!< Root directory child chain */
#ifdef MKROMFS
    romfs_size_t size;         /*!< Max size of the file system */
    romfs_size_t current_size; /*!< Current size size of the file system */
    romfs_off_t  heap;
#endif
    void *base;                /*!< Base address */
} romfs_t;

typedef struct {
#ifdef MKROMFS
    romfs_size_t size; /*!< Max size of the file system */
#endif
    uint8_t     *base; /*!< Base address */
} romfs_config_t;

int romfs_mount(romfs_t *fs, romfs_config_t *config);
int romfs_umount(romfs_t *fs);
int romfs_mkdir(romfs_t *fs, const char *path);
int romfs_dir_open(romfs_t *fs, romfs_dir_t *dir, const char *path);
int romfs_dir_read(romfs_t *fs, romfs_dir_t *dir, romfs_info_t *info);
int romfs_dir_close(romfs_t *fs, romfs_dir_t *dir);
int romfs_stat(romfs_t *fs, const char *path, romfs_info_t *info);
int romfs_file_open(romfs_t *fs, romfs_file_t *file, const char *path, int flags);
int romfs_file_sync(romfs_t *fs, romfs_file_t *file);
romfs_size_t romfs_file_read(romfs_t *fs, romfs_file_t *file, void *buffer, romfs_size_t size);
romfs_size_t romfs_file_write(romfs_t *fs, romfs_file_t *file, const void *buffer, romfs_size_t size);
int romfs_file_close(romfs_t *fs, romfs_file_t *file);
romfs_off_t romfs_file_seek(romfs_t *fs, romfs_file_t *file, romfs_off_t offset, romfs_whence_t whence);
int romfs_rename(romfs_t *fs, const char *oldpath, const char *newpath);
int romfs_rmdir(romfs_t *fs, const char *path);
int romfs_unlink(romfs_t *fs, const char *pathname);
romfs_off_t romfs_telldir(romfs_t *fs, romfs_dir_t *dir);
int romfs_file_truncate(romfs_t *fs, romfs_file_t *file, romfs_off_t size);
int romfs_file_stat(romfs_t *fs, romfs_file_t *file, romfs_info_t *info);

#endif /* _ROMFS_H_ */
