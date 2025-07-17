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
 * Lua RTOS, ROM file system
 *
 */

#include "romfs.h"

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

static int traverse(romfs_t *fs, const char *path, romfs_entry_t **entry, int creat, romfs_entry_type_t type);
static romfs_off_t romfs_file_seek_internal(romfs_t *fs, romfs_file_t *file, romfs_off_t offset, romfs_whence_t whence);

#ifdef MKROMFS
static int add_entry(romfs_t *fs, const char *name, romfs_entry_t *pa_parent, romfs_entry_t **pa_entry, romfs_entry_type_t entry_type);

static romfs_ptr_t romfs_calloc(romfs_t *fs, size_t nmemb, size_t size) {
    romfs_ptr_t addr = 0xffffffff;

    // Compute the total size to be allocated in the file system heap
    size_t total_size = nmemb * size;

    // Check that we can assign the requested memory
    if (fs->heap + total_size <= fs->size) {
        // The assigned address is the current heap position
        addr = (romfs_ptr_t)fs->heap;

        // Update the heap
        fs->heap += total_size;

        memset(ROMFS_PA(void *, addr), 0, total_size);
    }

    // Return the assigned virtual address
    return addr;
}

static int add_entry(romfs_t *fs, const char *name, romfs_entry_t *pa_parent, romfs_entry_t **pa_entry, romfs_entry_type_t entry_type) {
    // Initialize entry as NULL
    *pa_entry = NULL;

    // Check for space
    int name_len = strlen(name);

    romfs_size_t entry_size = sizeof(romfs_entry_t) + name_len - 1;
    romfs_size_t file_entry_size = 0;

    if (entry_type == ROMFS_FILE) {
        file_entry_size = sizeof(romfs_file_content_t);
    }

    if (fs->current_size + entry_size + file_entry_size > fs->size) {
        return ROMFS_ERR_NOSPC;
    }

    // Create the entry, and get both physical and virtual address for this entry
    romfs_ptr_t va_new_entry = romfs_calloc(fs, 1, entry_size);
    romfs_entry_t *pa_new_entry = ROMFS_PA(romfs_entry_t *, va_new_entry);

    if (!pa_new_entry) {
        return ROMFS_ERR_NOMEM;
    }

    pa_new_entry->flags |= entry_type;

    // Create the file entry, and get both physical and virtual address for this entry
    if (entry_type == ROMFS_FILE) {
        romfs_ptr_t va_new_file = romfs_calloc(fs, 1, file_entry_size);
        romfs_file_content_t *pa_new_file = ROMFS_PA(romfs_file_content_t *, va_new_file);

        if (!pa_new_file) {
            return ROMFS_ERR_NOMEM;
        }

        pa_new_entry->file.content = htole32(va_new_file);
        pa_new_file->data = htole32(fs->heap);
    } else {
        pa_new_entry->dir.child = htole32(ROMFS_VA(NULL));
    }

    pa_new_entry->next = htole32(ROMFS_VA(NULL));

    // Set the entry name and the length
    memcpy(pa_new_entry->name, name, name_len);

    pa_new_entry->flags |= (name_len << ROMFS_ENTRY_NAME_LEN_POS);

    // Update the file system size
    fs->current_size += entry_size + file_entry_size;

    // Add the entry to the file system
    if (!fs->child) {
        // The entry is the first child of the root directory
        fs->child = ROMFS_PA(romfs_entry_t *, va_new_entry);
    } else {
        if (pa_parent) {
            // Add the entry to its parent (which is always a directory)

            romfs_ptr_t va_child = le32toh(pa_parent->dir.child);
            romfs_entry_t *pa_child = ROMFS_PA(romfs_entry_t *, va_child);

            if (!pa_child) {
                // The entry is the first child of the director
                pa_parent->dir.child = htole32(va_new_entry);
            } else {
                // Add the entry into the directory child chain
                romfs_entry_t *centry = pa_child;

                while (ROMFS_PA(romfs_entry_t *, le32toh(centry->next))) {
                    centry = ROMFS_PA(romfs_entry_t *, le32toh(centry->next));
                }

                centry->next = htole32(va_new_entry);
            }
        } else {
            // Add the entry to the child chain of the root directory
            romfs_entry_t *centry = fs->child;

            while (ROMFS_PA(romfs_entry_t *, le32toh(centry->next))) {
                centry = ROMFS_PA(romfs_entry_t *, le32toh(centry->next));
            }

            centry->next = htole32(va_new_entry);
        }
    }

    *pa_entry = pa_new_entry;

    return ROMFS_ERR_OK;
}
#endif

static int traverse(romfs_t *fs, const char *path, romfs_entry_t **entry, int creat, romfs_entry_type_t type) {
    romfs_entry_t *centry; // Current entry

    // A copy of path to use with strtok_r
    char path_copy[PATH_MAX + 1];

    char *component; // Current path component
    char *rest;      // Rest of path

    // For now, no entry
    *entry = NULL;

    if (!path || !*path) {
        return ROMFS_ERR_NOENT;
    }

    if (strlen(path) > PATH_MAX) {
        return ROMFS_ERR_NAMETOOLONG;
    }

    // Make a copy of path to use it with strtok_r
    strcpy(path_copy, path);
    rest = path_copy;

    // Start at the root directory
    centry = fs->child;

#ifdef MKROMFS
    romfs_error_t ret;

    if (!centry) {
        // The file system doesn't contain any entry yet, then create a new entry
        component = strtok_r(rest, "/", &rest);

        if (creat && ((ret = add_entry(fs, component, NULL, entry, type)) != ROMFS_ERR_OK)) {
            return ret;
        }

        fs->child = *entry;

        return ROMFS_ERR_NOENT;
    }
#endif

    *entry = centry;

    int name_len;

#ifdef MKROMFS
    romfs_entry_t *parent = NULL;
#endif

    while ((component = strtok_r(rest, "/", &rest))) {
        while (centry) {
            name_len = ((centry->flags & ROMFS_ENTRY_NAME_LEN_MSK) >> ROMFS_ENTRY_NAME_LEN_POS);
            if ((strlen(component) == name_len) && bcmp(centry->name, component, name_len) == 0) {
                // The entry has been found
                *entry = centry;
                break;
            } else {
                // Next entry
                centry = ROMFS_PA(romfs_entry_t *, le32toh(centry->next));
            }
        }

        if (!centry) {
            // The entry was not found, create it, if it corresponds to the last
            // path component
#ifdef MKROMFS
            if (!rest) {
                if (creat && ((ret = add_entry(fs, component, parent, entry, type)) != ROMFS_ERR_OK)) {
                    return ret;
                }
            } else {
                *entry = NULL;
            }
#else
            if (rest) {
                *entry = NULL;
            }
#endif

            // If entry is created, entry is a yet physical address
            return ROMFS_ERR_NOENT;
        } else {
#ifdef MKROMFS
            parent = centry;
#endif

            if ((centry->flags & ROMFS_ENTRY_TYPE_MSK) == ROMFS_DIR) {
                // For next path component, start the search into the directory child chain
                centry = ROMFS_PA(romfs_entry_t *, le32toh(centry->dir.child));
            } else {
                // The current path component is a file, check that there are not more
                // path components
                if (rest) {
                    return ROMFS_ERR_NOTDIR;
                }
            }
        }
    }

    return ROMFS_ERR_OK;
}

static romfs_off_t romfs_file_seek_internal(romfs_t *fs, romfs_file_t *file, romfs_off_t offset, romfs_whence_t whence) {
    romfs_entry_t *entry = file->entry;
    romfs_file_content_t *content = ROMFS_PA(romfs_file_content_t *, le32toh(entry->file.content));

    if (whence == ROMFS_SEEK_SET) {
        if (offset >= 0) {
            file->offset = offset;
        } else {
            return ROMFS_ERR_INVAL;
        }
    } else if (whence == ROMFS_SEEK_CUR) {
        if (file->offset + offset >= 0) {
            file->offset += offset;
        } else {
            return ROMFS_ERR_INVAL;
        }
    } else if (whence == ROMFS_SEEK_END) {
        if (offset + (int32_t)le32toh(content->size) >= 0) {
            file->offset = offset + le32toh(content->size);
        } else {
            return ROMFS_ERR_INVAL;
        }
    } else {
        return ROMFS_ERR_INVAL;
    }

    file->ptr = ROMFS_PA(uint8_t *, le32toh(content->data)) + file->offset;

    return file->offset;
}

int romfs_mount(romfs_t *fs, romfs_config_t *config) {
    memset(fs, 0, sizeof(romfs_t));
#ifdef MKROMFS
    fs->size = config->size;
    fs->base = config->base;
    fs->child = NULL;

	fs->current_size = 0;
#else
	fs->base = config->base;

    fs->child = ROMFS_PA(romfs_entry_t *, 0);
#endif

    return ROMFS_ERR_OK;
}

int romfs_umount(romfs_t *fs) {
    memset(fs, 0, sizeof(romfs_t));

    return ROMFS_ERR_OK;
}

#ifdef MKROMFS
int romfs_mkdir(romfs_t *fs, const char *path) {
    romfs_entry_t *entry;
    romfs_error_t ret;

    if (strcmp(path,"/") == 0) {
        return ROMFS_ERR_EXIST;
    }

    ret = traverse(fs, path, &entry, 1, ROMFS_DIR);
    if (ret != ROMFS_ERR_OK) {
        if (ret == ROMFS_ERR_NOENT) {
            if (entry) {
                return ROMFS_ERR_OK;
            } else {
                return ROMFS_ERR_NOENT;
            }
        } else {
            return ret;
        }
    } else {
        return ROMFS_ERR_EXIST;
    }

    return ROMFS_ERR_OK;
}
#else
int romfs_dir_open(romfs_t *fs, romfs_dir_t *dir, const char *path) {
    romfs_entry_t *entry;
    romfs_error_t ret;

    memset(dir, 0, sizeof(romfs_dir_t));

    if (strcmp(path,"/") == 0) {
        dir->child = fs->child;
    } else {
        ret = traverse(fs, path, &entry, 0, 0);
        if (ret != ROMFS_ERR_OK) {
            dir->offset = -1;

            return ret;
        }

        if ((entry->flags & ROMFS_ENTRY_TYPE_MSK) != ROMFS_DIR) {
            return ROMFS_ERR_NOTDIR;
        }

        dir->entry = entry;
        dir->child = ROMFS_PA(romfs_entry_t *, le32toh(entry->dir.child));
    }

    return ROMFS_ERR_OK;
}

int romfs_dir_read(romfs_t *fs, romfs_dir_t *dir, romfs_info_t *info) {
    if (dir->offset < 0) {
        return ROMFS_ERR_BADF;
    }

    if (!dir->child) {
        dir->offset = -1;

        return ROMFS_ERR_NOENT;
    }

    int name_len = ((dir->child->flags & ROMFS_ENTRY_NAME_LEN_MSK) >> ROMFS_ENTRY_NAME_LEN_POS);

    memcpy(info->name, dir->child->name, name_len);
    *(info->name + name_len) = 0;

    info->type = (dir->child->flags & ROMFS_ENTRY_TYPE_MSK);
    info->size = 0;

    if (info->type == ROMFS_FILE) {
        info->size = le32toh(ROMFS_PA(romfs_file_content_t *, le32toh(dir->child->file.content))->size);
    }

    dir->child = ROMFS_PA(romfs_entry_t *, le32toh(dir->child->next));

    dir->offset++;

    return ROMFS_ERR_OK;
}

int romfs_dir_close(romfs_t *fs, romfs_dir_t *dir) {
    return ROMFS_ERR_OK;
}

romfs_off_t romfs_telldir(romfs_t *fs, romfs_dir_t *dir) {
    if (dir->offset < 0) {
        return ROMFS_ERR_BADF;
    }

    return dir->offset;
}

int romfs_stat(romfs_t *fs, const char *path, romfs_info_t *info) {
    romfs_entry_t *entry;
    romfs_error_t ret;

    memset(info, 0, sizeof(romfs_info_t));

    if (strcmp(path,"/") == 0) {
        strcpy(info->name, "");
        info->type = ROMFS_DIR;
    } else {
        ret = traverse(fs, path, &entry, 0, 0);
        if (ret != ROMFS_ERR_OK) {
            return ret;
        }

        int name_len = ((entry->flags & ROMFS_ENTRY_NAME_LEN_MSK) >> ROMFS_ENTRY_NAME_LEN_POS);

        memcpy(info->name, entry->name, name_len);
        *(info->name + name_len) = 0;

        info->type = entry->flags & ROMFS_ENTRY_TYPE_MSK;

        if (info->type == ROMFS_FILE) {
            info->size = le32toh(ROMFS_PA(romfs_file_content_t *, le32toh(entry->file.content))->size);
        }
    }

    return ROMFS_ERR_OK;
}

int romfs_file_stat(romfs_t *fs, romfs_file_t *file, romfs_info_t *info) {
    if (!file->entry) {
        return ROMFS_ERR_BADF;

    }

    memset(info, 0, sizeof(romfs_info_t));

    romfs_entry_t *entry = file->entry;

    int name_len = ((entry->flags & ROMFS_ENTRY_NAME_LEN_MSK) >> ROMFS_ENTRY_NAME_LEN_POS);

    memcpy(info->name, entry->name, name_len);
    *(info->name + name_len) = 0;

    info->type = entry->flags & ROMFS_ENTRY_TYPE_MSK;
    info->size = 0;

    if (info->type == ROMFS_FILE) {
        info->size = le32toh(ROMFS_PA(romfs_file_content_t *, le32toh(entry->file.content))->size);
    }

    return ROMFS_ERR_OK;
}
#endif

int romfs_file_open(romfs_t *fs, romfs_file_t *file, const char *path, int flags) {
    romfs_entry_t *entry;
    romfs_error_t ret;

    int access_mode = (flags & ROMFS_ACCMODE);

#ifdef MKROMFS
    if ((access_mode != ROMFS_O_RDONLY) && (access_mode != ROMFS_O_WRONLY) && (access_mode != ROMFS_O_RDWR)) {
        return ROMFS_ERR_ACCESS;
    }
#else
    if (access_mode != ROMFS_O_RDONLY) {
        return ROMFS_ERR_ACCESS;
    }
#endif

#ifdef MKROMFS
    ret = traverse(fs, path, &entry, flags & ROMFS_O_CREAT, ROMFS_FILE);
    if (ret == ROMFS_ERR_NOENT) {
        // File doesn't exist
        if (!(flags & ROMFS_O_CREAT)) {
            // If no O_CREAT file is present, exit
            return ROMFS_ERR_NOENT;
        }
    } else if (ret != ROMFS_ERR_OK) {
        return ret;
    }
#else
    ret = traverse(fs, path, &entry, 0, ROMFS_FILE);
    if (ret != ROMFS_ERR_OK) {
        return ret;
    }
#endif

    // Prepare file structure
    memset(file, 0, sizeof(romfs_file_t));

    file->entry = entry;
    file->flags = flags;

    // Set file position
    ret = romfs_file_seek_internal(fs, file, 0, ROMFS_SEEK_SET);
    assert(ret >= 0);

    return ROMFS_ERR_OK;
}

romfs_size_t romfs_file_read(romfs_t *fs, romfs_file_t *file, void *buffer, romfs_size_t size) {
    int access_mode = (file->flags & ROMFS_ACCMODE);

#ifdef MKROMFS
    if ((access_mode != ROMFS_O_RDONLY) && (access_mode != ROMFS_O_RDWR)) {
        return ROMFS_ERR_BADF;
    }
#else
    if (access_mode != ROMFS_O_RDONLY) {
        return ROMFS_ERR_BADF;
    }
#endif

    int file_size = le32toh(ROMFS_PA(romfs_file_content_t *, le32toh(file->entry->file.content))->size);

    romfs_size_t reads = 0;
    while ((reads < size) && (file->offset < file_size)) {
        *(((uint8_t *)buffer++)) = *(file->ptr++);

        reads++;
        file->offset++;
    }

    return reads;
}

#ifdef MKROMFS
romfs_size_t romfs_file_write(romfs_t *fs, romfs_file_t *file, const void *buffer, romfs_size_t size) {
    int access_mode = (file->flags & ROMFS_ACCMODE);

    if ((access_mode != ROMFS_O_WRONLY) && (access_mode != ROMFS_O_RDWR)) {
        return ROMFS_ERR_BADF;
    }

    romfs_file_content_t *content = ROMFS_PA(romfs_file_content_t *, le32toh(file->entry->file.content));

    romfs_size_t writes = 0;
    while (writes < size) {
    		if (fs->current_size + 1 > fs->size) {
    			return ROMFS_ERR_NOSPC;
    		}

        *(file->ptr++) = *(((uint8_t *)buffer++));
        fs->heap++;
        fs->current_size++;

        writes++;
        file->offset++;

        if (file->offset > le32toh(content->size)) {
            content->size = htole32(le32toh(content->size) + 1);
        }
    }

    return writes;
}
#endif

int romfs_file_close(romfs_t *fs, romfs_file_t *file) {
    memset(file, 0, sizeof(romfs_file_t));

    return ROMFS_ERR_OK;
}

romfs_off_t romfs_file_seek(romfs_t *fs, romfs_file_t *file, romfs_off_t offset, romfs_whence_t whence) {
    romfs_off_t ret;

    ret = romfs_file_seek_internal(fs, file, offset, whence);

    return ret;
}
