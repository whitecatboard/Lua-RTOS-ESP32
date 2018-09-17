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

#include "ramfs.h"

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

static int add_reference(ramfs_t *fs, ramfs_entry_t *entry);
static void remove_reference(ramfs_t *fs, ramfs_entry_t *entry);
static int get_reference_uses(ramfs_t *fs, ramfs_entry_t *entry);
static void remove_block(ramfs_t *fs, ramfs_file_t *file);
static int add_block(ramfs_t *fs, ramfs_file_t *file);
static int add_entry(ramfs_t *fs, const char *name, ramfs_entry_t *parent, ramfs_entry_t **entry, ramfs_entry_type_t entry_type);
static void remove_entry(ramfs_t *fs, ramfs_entry_t *entry, ramfs_entry_t *parent_entry, ramfs_entry_t *prev_entry, int remove);
static int traverse(ramfs_t *fs, const char *path, ramfs_entry_t **entry, ramfs_entry_t **parent_entry, ramfs_entry_t **prev_entry, int creat, ramfs_entry_type_t type);
static ramfs_off_t ramfs_file_seek_internal(ramfs_t *fs, ramfs_file_t *file, ramfs_off_t offset, ramfs_whence_t whence);
static int ramfs_file_truncate_internal(ramfs_t *fs, ramfs_file_t *file, ramfs_off_t size);

static int add_reference(ramfs_t *fs, ramfs_entry_t *entry) {
    ramfs_entry_ref_t *cref;

    // Find entry into the reference set
    cref = fs->ref;
    while (cref && (cref->entry != entry)) {
        cref = cref->next;
    }

    if (!cref) {
        // Entry not found, add it
        ramfs_entry_ref_t *ref = calloc(1, sizeof(ramfs_entry_ref_t));
        if (!ref) {
            return RAMFS_ERR_NOMEM;
        }

        ref->entry = entry;
        ref->next = fs->ref;
        ref->uses = 1;

        fs->ref = ref;
    } else {
        // Entry found, just increment the uses counter
        cref->uses++;
    }

    return RAMFS_ERR_OK;
}

static void remove_reference(ramfs_t *fs, ramfs_entry_t *entry) {
    ramfs_entry_ref_t *cref;
    ramfs_entry_ref_t *prev_ref = NULL;

    if (!entry) return;

    // Find entry into the reference set
    cref = fs->ref;
    while (cref && (cref->entry != entry)) {
        prev_ref = cref;
        cref = cref->next;
    }

    if (cref) {
        // Entry found
        if (cref->uses > 0) {
            // Decrement the uses counter
            cref->uses--;
        }

        if (cref->uses == 0) {
            // Not used, remove it
            if (!prev_ref) {
                // It is the first reference
                fs->ref = cref->next;
            } else {
                prev_ref->next = cref->next;
            }

            free(cref);

            if (entry->flags & RAMFS_ENTRY_RM_LEN_MSK) {
                // The entry is marked for remove, remove now
                remove_entry(fs, entry, NULL, NULL, ((entry->flags & RAMFS_ENTRY_TYPE_MSK) == RAMFS_FILE));
            }
        }
    }
}

static int get_reference_uses(ramfs_t *fs, ramfs_entry_t *entry) {
    ramfs_entry_ref_t *cref;

    // Find entry into the reference set
    cref = fs->ref;
    while (cref && (cref->entry != entry)) {
        cref = cref->next;
    }

    int uses = 0;

    if (cref) {
        uses = cref->uses;
    }

    return uses;
}

static void remove_block(ramfs_t *fs, ramfs_file_t *file) {
    ramfs_block_t *block;

    block = file->entry->file.header->tail;
    if (block) {
        // Find the block into the file block chain to
        // update the block chain
        ramfs_block_t *prev_block = NULL;
        ramfs_block_t *cblock = file->entry->file.header->head;

        while (cblock && (cblock != block)) {
            prev_block = cblock;
            cblock = cblock->next;
        }

        if (prev_block) {
            prev_block->next = NULL;
            file->entry->file.header->tail = prev_block;
        }    else {
            file->entry->file.header->head = NULL;
            file->entry->file.header->tail = NULL;
        }

        // Free block
        free(block);

        // Update the file system size
        fs->current_size -= sizeof(ramfs_block_t) + fs->block_size - 1;
    }
}

static int add_block(ramfs_t *fs, ramfs_file_t *file) {
    // Check for space
    ramfs_size_t size = sizeof(ramfs_block_t) + fs->block_size - 1;

    if (fs->current_size + size > fs->size) {
        return RAMFS_ERR_NOSPC;
    }

    // Create block
    ramfs_block_t *block = (ramfs_block_t *)calloc(1, size);
    if (!block) {
        return RAMFS_ERR_NOMEM;
    }

    if (!file->entry->file.header->head) {
        // First block of the file
        file->entry->file.header->head = block;
        file->entry->file.header->tail = block;
    } else {
        // Add the block to the end of the file block chain
        file->entry->file.header->tail->next = block;
        file->entry->file.header->tail = block;
    }

    file->block = block;
    file->ptr = block->data;

    // Update the file system size
    fs->current_size += size;

    return RAMFS_ERR_OK;
}

static int add_entry(ramfs_t *fs, const char *name, ramfs_entry_t *parent, ramfs_entry_t **entry, ramfs_entry_type_t entry_type) {
    int name_len = strlen(name);

    // Check for space
    ramfs_size_t entry_size = sizeof(ramfs_entry_t) + name_len - 1;
    ramfs_size_t header_size = 0;

    if (entry_type == RAMFS_FILE) {
        header_size = sizeof(ram_file_header_t);
    }

    if (fs->current_size + entry_size + header_size > fs->size) {
        return RAMFS_ERR_NOSPC;
    }

    // Create the entry
    *entry = (ramfs_entry_t *)calloc(1, entry_size);
    if (!*entry) {
        return RAMFS_ERR_NOMEM;
    }

    (*entry)->flags |= entry_type;

    // Create the file header
    if (entry_type == RAMFS_FILE) {
        (*entry)->file.header = (ram_file_header_t *)calloc(1, header_size);
        if (!((*entry)->file.header)) {
            free(*entry);

            return RAMFS_ERR_NOMEM;
        }

        (*entry)->file.header->head = NULL;
        (*entry)->file.header->tail = NULL;
        (*entry)->file.header->size = 0;
    }

    // Set the entry name and len
    memcpy((*entry)->name, name, name_len);

    (*entry)->flags |= (name_len << RAMFS_ENTRY_NAME_LEN_POS);

    // Update the file system size
    fs->current_size += entry_size + header_size;

    // Add the entry to the file system
    if (!fs->child) {
        // The entry is the first child of the root directory
        fs->child = *entry;
    } else {
        if (parent) {
            // Add the entry to its parent (which is always a directory)
            if (!parent->dir.child) {
                // The entry is the first child of the directory
                parent->dir.child = *entry;
            } else {
                // Add the entry into the directory child chain
                ramfs_entry_t *centry = parent->dir.child;

                while (centry->next) {
                    centry = centry->next;
                }

                centry->next = *entry;
            }
        } else {
            // Add the entry to the child chain of the root directory
            ramfs_entry_t *centry = fs->child;

            while (centry->next) {
                centry = centry->next;
            }

            centry->next = *entry;
        }
    }

    return RAMFS_ERR_OK;
}

static void remove_entry(ramfs_t *fs, ramfs_entry_t *entry, ramfs_entry_t *parent_entry, ramfs_entry_t *prev_entry, int remove) {
    if (!entry) return;

     // Update the chain
    if (prev_entry) {
        prev_entry->next = entry->next;
    }

    if (parent_entry && (parent_entry->dir.child == entry)) {
        parent_entry->dir.child = entry->next;
    } else if (!parent_entry) {
        if (fs->child == entry) {
            fs->child = entry->next;
        }
    }

    // If the entry to remove is used, mark it for remove later
    if (get_reference_uses(fs, entry) > 0) {
        entry->flags |= RAMFS_ENTRY_RM_LEN_MSK;
        return;
    }

    // While removing the entry, compute the entry size to update the file system size later
    ramfs_size_t size = sizeof(ramfs_entry_t) + ((entry->flags & RAMFS_ENTRY_NAME_LEN_MSK) >> RAMFS_ENTRY_NAME_LEN_POS) - 1;

    if (remove && ((entry->flags & RAMFS_ENTRY_TYPE_MSK) == RAMFS_FILE)) {
        // Free file blocks
        ramfs_block_t *block = entry->file.header->head;
        ramfs_block_t *tmp;

        size += sizeof(ram_file_header_t);

        while (block) {
            size += sizeof(ramfs_block_t) + fs->block_size - 1;

            tmp = block;
            block = block->next;

            free(tmp);
        }

        free(entry->file.header);
    }

    free(entry);

    // Update the file system size
    fs->current_size -= size;
}

static int traverse(ramfs_t *fs, const char *path, ramfs_entry_t **entry, ramfs_entry_t **parent_entry, ramfs_entry_t **prev_entry, int creat, ramfs_entry_type_t type) {
    ramfs_error_t ret;

    ramfs_entry_t *centry; // Curren entry

    // A copy of path to use with strtok_r
    char path_copy[PATH_MAX + 1];

    char *component; // Current path component
    char *rest;      // Rest of path

    *entry = NULL;

    if (prev_entry) {
        *prev_entry = NULL;
    }

    if (!path || !*path) {
        return RAMFS_ERR_NOENT;
    }

    if (strlen(path) > PATH_MAX) {
        return RAMFS_ERR_NAMETOOLONG;
    }

    // Make a copy of path to use it with strtok_r
    strcpy(path_copy, path);
    rest = path_copy;

    // Start at the root directory
    centry = fs->child;
    *entry = centry;

    if (!centry) {
        // The file system doesn't contain any entry yet, then create a new entry
        component = strtok_r(rest, "/", &rest);

        if (creat && ((ret = add_entry(fs, component, NULL, entry, type)) != RAMFS_ERR_OK)) {
            return ret;
        }

        return RAMFS_ERR_NOENT;
    }


    int name_len;

    ramfs_entry_t *parent = NULL;

    if (parent_entry) {
        *parent_entry = parent;
    }

    while ((component = strtok_r(rest, "/", &rest))) {
        while (centry) {
            name_len = ((centry->flags & RAMFS_ENTRY_NAME_LEN_MSK) >> RAMFS_ENTRY_NAME_LEN_POS);
            if ((strlen(component) == name_len) && bcmp(centry->name, component, name_len) == 0) {
                // The entry has been found
                *entry = centry;
                break;
            } else {
                // Next entry
                if (prev_entry) {
                    *prev_entry = centry;
                }

                centry = centry->next;
            }
        }

        if (!centry) {
            // The entry was not found, create it, if it corresponds to the last
            // path component
            if (!rest) {
                if (creat && ((ret = add_entry(fs, component, parent, entry, type)) != RAMFS_ERR_OK)) {
                    return ret;
                }
            } else {
                *entry = NULL;

                if (parent_entry) {
                    *parent_entry = NULL;
                }

                if (prev_entry) {
                    *prev_entry = NULL;
                }
            }

            return RAMFS_ERR_NOENT;
        } else {
            parent = centry;

            if ((centry->flags & RAMFS_ENTRY_TYPE_MSK) == RAMFS_DIR) {
                // For next path component, start the search into the directory child chain
                centry = centry->dir.child;

                if (rest) {
                    if (parent_entry) {
                        *parent_entry = parent;
                    }

                    if (prev_entry) {
                        *prev_entry = NULL;
                    }
                }
            } else {
                // The current path component is a file, check that there are not more
                // path components
                if (rest) {
                    return RAMFS_ERR_NOTDIR;
                }
            }
        }
    }

    return RAMFS_ERR_OK;
}

static ramfs_off_t ramfs_file_seek_internal(ramfs_t *fs, ramfs_file_t *file, ramfs_off_t offset, ramfs_whence_t whence) {
    if (whence == RAMFS_SEEK_SET) {
        if (offset >= 0) {
            file->offset = offset;
        } else {
            return RAMFS_ERR_INVAL;
        }
    } else if (whence == RAMFS_SEEK_CUR) {
        if (file->offset + offset >= 0) {
            file->offset += offset;
        } else {
            return RAMFS_ERR_INVAL;
        }
    } else if (whence == RAMFS_SEEK_END) {
        if (offset + file->entry->file.header->size >= 0) {
            file->offset = offset + file->entry->file.header->size;
        } else {
            return RAMFS_ERR_INVAL;
        }
    } else {
        return RAMFS_ERR_INVAL;
    }

    int block_num = file->offset / fs->block_size;
    int block_off = file->offset % fs->block_size;

    if (file->entry->file.header->head) {
        ramfs_block_t *block = file->entry->file.header->head;
        int curr_block;

        for(curr_block = 0;block && (curr_block < block_num);curr_block++) {
            block = block->next;
        }

        file->block = block;
        file->ptr = (block?block->data + block_off:NULL);
    } else {
        file->block = NULL;
        file->ptr = NULL;
    }

    return block_num * fs->block_size + block_off;
}

static int ramfs_file_truncate_internal(ramfs_t *fs, ramfs_file_t *file, ramfs_off_t size) {
    ramfs_error_t ret;

    int access_mode = (file->flags & RAMFS_ACCMODE);

    if ((access_mode != RAMFS_O_WRONLY) && (access_mode != RAMFS_O_RDWR)) {
        return RAMFS_ERR_BADF;
    }

    if ((file->entry->flags & RAMFS_ENTRY_TYPE_MSK) == RAMFS_DIR) {
        return RAMFS_ERR_ISDIR;
    }

    if (size < 0) {
        return RAMFS_ERR_INVAL;
    }

    int block_delta = ((size - 1) / fs->block_size) - ((file->entry->file.header->size - 1) / fs->block_size);

    if (block_delta < 0) {
        // Decrease size
        while (block_delta < 0) {
            remove_block(fs, file);
            block_delta++;
        }
    } else if (block_delta > 0) {
        // Increase size
        while (block_delta > 0) {
            if ((ret = add_block(fs, file)) != RAMFS_ERR_OK) {
                return ret;
            }

            block_delta--;
        }
    }

    file->entry->file.header->size = size;

    ramfs_file_seek_internal(fs, file, file->offset, RAMFS_SEEK_SET);

    return RAMFS_ERR_OK;
}

int ramfs_mount(ramfs_t *fs, ramfs_config_t *config) {
    memset(fs, 0, sizeof(ramfs_t));

    ramfs_lock_init(fs->lock);

    fs->size = config->size;
    fs->block_size = config->block_size;

    return RAMFS_ERR_OK;
}

int ramfs_umount(ramfs_t *fs) {
    int top = 0;
    ramfs_entry_t *stack[256];
    ramfs_entry_t *centry;
    ramfs_entry_t *next_entry = NULL;
    ramfs_entry_t *parent_entry = NULL;

    ramfs_lock(fs->lock);

    stack[top] = NULL;

    while (top >= 0) {
        centry = stack[top];
        if (!centry) {
            top--;
            centry = fs->child;
            parent_entry = NULL;
        } else {
            parent_entry = centry;
            centry = centry->dir.child;
        }

        while (centry) {
            next_entry = centry->next;

            if ((centry->flags & RAMFS_ENTRY_TYPE_MSK) == RAMFS_FILE) {
                remove_entry(fs, centry,  NULL, NULL, 1);
            } else if ((centry->flags & RAMFS_ENTRY_TYPE_MSK) == RAMFS_DIR) {
                if (!centry->dir.child) {
                    remove_entry(fs, centry,  NULL, NULL, 1);
                } else {
                    stack[++top] = centry;
                }
            }

            centry = next_entry;
        }

        if (parent_entry && (stack[top] == parent_entry)) {
            remove_entry(fs, parent_entry,  NULL, NULL, 1);
            top--;
        }
    }

    ramfs_lock_destroy(fs->lock);

    memset(fs, 0, sizeof(ramfs_t));

    return RAMFS_ERR_OK;
}


int ramfs_mkdir(ramfs_t *fs, const char *path) {
    ramfs_entry_t *entry;
    ramfs_error_t ret;

    if (strcmp(path,"/") == 0) {
        return RAMFS_ERR_EXIST;
    }

    ramfs_lock(fs->lock);
    ret = traverse(fs, path, &entry, NULL, NULL, 1, RAMFS_DIR);
    ramfs_unlock(fs->lock);

    if (ret != RAMFS_ERR_OK) {
        if (ret == RAMFS_ERR_NOENT) {
            if (entry) {
                return RAMFS_ERR_OK;
            } else {
                return RAMFS_ERR_NOENT;
            }
        } else {
            return ret;
        }
    } else {
        return RAMFS_ERR_EXIST;
    }

    return RAMFS_ERR_OK;
}

int ramfs_dir_open(ramfs_t *fs, ramfs_dir_t *dir, const char *path) {
    ramfs_entry_t *entry;
    ramfs_error_t ret;

    memset(dir, 0, sizeof(ramfs_dir_t));

    if (strcmp(path,"/") == 0) {
        ramfs_lock(fs->lock);
        dir->child = fs->child;
        ramfs_unlock(fs->lock);
    } else {
        ramfs_lock(fs->lock);

        ret = traverse(fs, path, &entry, NULL, NULL, 0, 0);
        if (ret != RAMFS_ERR_OK) {
            ramfs_unlock(fs->lock);

            dir->offset = -1;

            return ret;
        }

        if ((entry->flags & RAMFS_ENTRY_TYPE_MSK) != RAMFS_DIR) {
            ramfs_unlock(fs->lock);

            return RAMFS_ERR_NOTDIR;
        }

        ret = add_reference(fs, entry);
        if (ret != RAMFS_ERR_OK) {
            ramfs_unlock(fs->lock);

            return ret;
        }

        dir->entry = entry;
        dir->child = entry->dir.child;

        ramfs_unlock(fs->lock);
    }

    return RAMFS_ERR_OK;
}

int ramfs_dir_read(ramfs_t *fs, ramfs_dir_t *dir, ramfs_info_t *info) {
    if (dir->offset < 0) {
        return RAMFS_ERR_BADF;
    }

    if (!dir->child) {
        dir->offset = -1;

        return RAMFS_ERR_NOENT;
    }

    int name_len = ((dir->child->flags & RAMFS_ENTRY_NAME_LEN_MSK) >> RAMFS_ENTRY_NAME_LEN_POS);

    memcpy(info->name, dir->child->name, name_len);
    *(info->name + name_len) = 0;

    info->type = (dir->child->flags & RAMFS_ENTRY_TYPE_MSK);
    info->size = 0;

    if (info->type == RAMFS_FILE) {
        info->size = dir->child->file.header->size;
    }

    dir->child = dir->child->next;

    dir->offset++;

    return RAMFS_ERR_OK;
}

int ramfs_dir_close(ramfs_t *fs, ramfs_dir_t *dir) {
    ramfs_lock(fs->lock);
    remove_reference(fs, dir->entry);
    ramfs_unlock(fs->lock);

    return RAMFS_ERR_OK;
}

ramfs_off_t ramfs_telldir(ramfs_t *fs, ramfs_dir_t *dir) {
    if (dir->offset < 0) {
        return RAMFS_ERR_BADF;
    }

    return dir->offset;
}

int ramfs_stat(ramfs_t *fs, const char *path, ramfs_info_t *info) {
    ramfs_entry_t *entry;
    ramfs_error_t ret;

    if (strcmp(path,"/") == 0) {
        strcpy(info->name, "");
        info->type = RAMFS_DIR;
    } else {
        ramfs_lock(fs->lock);

        ret = traverse(fs, path, &entry, NULL, NULL, 0, 0);
        if (ret != RAMFS_ERR_OK) {
            ramfs_unlock(fs->lock);

            return ret;
        }

        int name_len = ((entry->flags & RAMFS_ENTRY_NAME_LEN_MSK) >> RAMFS_ENTRY_NAME_LEN_POS);

        memcpy(info->name, entry->name, name_len);
        *(info->name + name_len) = 0;

        info->type = entry->flags & RAMFS_ENTRY_TYPE_MSK;
        info->size = 0;

        if (info->type == RAMFS_FILE) {
            info->size = entry->file.header->size;
        }

        ramfs_unlock(fs->lock);
    }

    return RAMFS_ERR_OK;
}

int ramfs_file_stat(ramfs_t *fs, ramfs_file_t *file, ramfs_info_t *info) {
    if (!file->entry) {
        return RAMFS_ERR_BADF;

    }

    ramfs_lock(fs->lock);

    int name_len = ((file->entry->flags & RAMFS_ENTRY_NAME_LEN_MSK) >> RAMFS_ENTRY_NAME_LEN_POS);

    memcpy(info->name, file->entry->name, name_len);
    *(info->name + name_len) = 0;

    info->type = file->entry->flags & RAMFS_ENTRY_TYPE_MSK;
    info->size = 0;

    if (info->type == RAMFS_FILE) {
        info->size = file->entry->file.header->size;
    }

    ramfs_unlock(fs->lock);

    return RAMFS_ERR_OK;
}

int ramfs_file_open(ramfs_t *fs, ramfs_file_t *file, const char *path, int flags) {
    ramfs_entry_t *entry;
    ramfs_error_t ret;

    int access_mode = (flags & RAMFS_ACCMODE);
    if ((access_mode != RAMFS_O_RDONLY) && (access_mode != RAMFS_O_WRONLY) && (access_mode != RAMFS_O_RDWR)) {
        return RAMFS_ERR_ACCESS;
    }

    ramfs_lock(fs->lock);

    ret = traverse(fs, path, &entry, NULL, NULL, flags & RAMFS_O_CREAT, RAMFS_FILE);
    if (ret == RAMFS_ERR_NOENT) {
        // File doesn't exist
        if (!(flags & RAMFS_O_CREAT)) {
            ramfs_unlock(fs->lock);

            // If no O_CREAT file is present, exit
            return RAMFS_ERR_NOENT;
        }
    } else if ((ret == RAMFS_ERR_OK) && (flags & RAMFS_O_EXCL) && (flags & RAMFS_O_CREAT)) {
        ramfs_unlock(fs->lock);

        return RAMFS_ERR_EXIST;
    } else if (ret != RAMFS_ERR_OK) {
        ramfs_unlock(fs->lock);

        return ret;
    }

    ret = add_reference(fs, entry);
    if (ret != RAMFS_ERR_OK) {
        ramfs_unlock(fs->lock);

        return ret;
    }

    // Prepare file structure
    memset(file, 0, sizeof(ramfs_file_t));

    file->entry = entry;
    file->flags = flags;

    // Truncate file, if required
    if ((flags & RAMFS_O_TRUNC) && ((access_mode == RAMFS_O_WRONLY) || (access_mode == RAMFS_O_RDWR))) {
        ret = ramfs_file_truncate_internal(fs, file, 0);
        if (ret != RAMFS_ERR_OK) {
            ramfs_unlock(fs->lock);

            return ret;
        }
    }

    // Set file position
    if (flags & RAMFS_O_APPEND){
        ret = ramfs_file_seek_internal(fs, file, 0, RAMFS_SEEK_END);
        assert(ret >= 0);
    } else {
        ret = ramfs_file_seek_internal(fs, file, 0, RAMFS_SEEK_SET);
        assert(ret >= 0);
    }

    ramfs_unlock(fs->lock);

    return RAMFS_ERR_OK;
}

int ramfs_file_sync(ramfs_t *fs, ramfs_file_t *file) {
    return RAMFS_ERR_OK;
}

ramfs_size_t ramfs_file_read(ramfs_t *fs, ramfs_file_t *file, void *buffer, ramfs_size_t size) {
    int access_mode = (file->flags & RAMFS_ACCMODE);

    if ((access_mode != RAMFS_O_RDONLY) && (access_mode != RAMFS_O_RDWR)) {
        return RAMFS_ERR_BADF;
    }

    ramfs_lock(fs->lock);

    ramfs_size_t reads = 0;
    while ((reads < size) && (file->offset < file->entry->file.header->size)) {
        if ((!file->block) || (file->ptr > file->block->data + fs->block_size - 1)) {
            if (file->block) {
                file->block = file->block->next;
                if (file->block) {
                    file->ptr = file->block->data;
                } else {
                    file->ptr = NULL;
                    ramfs_unlock(fs->lock);

                    return reads;
                }
            } else {
                file->block = NULL;
                file->ptr = NULL;

                ramfs_unlock(fs->lock);
                return reads;
            }
        }

        *(((uint8_t *)buffer++)) = *(file->ptr++);

        reads++;
        file->offset++;
    }

    ramfs_unlock(fs->lock);

    return reads;
}

ramfs_size_t ramfs_file_write(ramfs_t *fs, ramfs_file_t *file, const void *buffer, ramfs_size_t size) {
    ramfs_error_t ret;

    int access_mode = (file->flags & RAMFS_ACCMODE);

    if ((access_mode != RAMFS_O_WRONLY) && (access_mode != RAMFS_O_RDWR)) {
        return RAMFS_ERR_BADF;
    }

    ramfs_lock(fs->lock);

    ramfs_size_t writes = 0;
    while (writes < size) {
        if ((!file->block) || (!file->ptr) || (file->ptr > file->block->data + fs->block_size - 1)) {
            if (file->block && file->block->next) {
                file->block = file->block->next;
                file->ptr = file->block->data;
            } else {
                ret = add_block(fs, file);
                if (ret != RAMFS_ERR_OK) {
                    file->block = NULL;
                    file->ptr = NULL;
                    ramfs_unlock(fs->lock);
                    return ret;
                }
            }
        }

        *(file->ptr++) = *(((uint8_t *)buffer++));

        writes++;
        file->offset++;

        if (file->offset > file->entry->file.header->size) {
            file->entry->file.header->size++;
        }
    }

    ramfs_unlock(fs->lock);

    return writes;
}

int ramfs_file_truncate(ramfs_t *fs, ramfs_file_t *file, ramfs_off_t size) {
    ramfs_error_t ret;

    ramfs_lock(fs->lock);
    ret = ramfs_file_truncate_internal(fs, file, size);
    ramfs_unlock(fs->lock);

    return ret;
}

int ramfs_file_close(ramfs_t *fs, ramfs_file_t *file) {
    ramfs_lock(fs->lock);
    remove_reference(fs, file->entry);
    ramfs_unlock(fs->lock);

    memset(file, 0, sizeof(ramfs_file_t));

    return RAMFS_ERR_OK;
}

ramfs_off_t ramfs_file_seek(ramfs_t *fs, ramfs_file_t *file, ramfs_off_t offset, ramfs_whence_t whence) {
    ramfs_off_t ret;

    ramfs_lock(fs->lock);
    ret = ramfs_file_seek_internal(fs, file, offset, whence);
    ramfs_unlock(fs->lock);

    return ret;
}

int ramfs_rename(ramfs_t *fs, const char *oldpath, const char *newpath) {
    ramfs_entry_t *old_entry;
    ramfs_entry_t *parent_old_entry;
    ramfs_entry_t *prev_old_entry;
    ramfs_entry_t *new_entry;
    ramfs_error_t ret;

    ramfs_lock(fs->lock);

    ret = traverse(fs, oldpath, &old_entry, &parent_old_entry, &prev_old_entry, 0, 0);
    if (ret != RAMFS_ERR_OK) {
        ramfs_unlock(fs->lock);
        return ret;
    }

    ret = traverse(fs, newpath, &new_entry, NULL, NULL, 1, old_entry->flags & RAMFS_ENTRY_TYPE_MSK);
    if (ret == RAMFS_ERR_OK) {
        if (((new_entry->flags & RAMFS_ENTRY_TYPE_MSK) == RAMFS_DIR) && ((old_entry->flags & RAMFS_ENTRY_TYPE_MSK) == RAMFS_FILE)) {
            ramfs_unlock(fs->lock);
            return RAMFS_ERR_ISDIR;
        } else if (((new_entry->flags & RAMFS_ENTRY_TYPE_MSK) == RAMFS_DIR) && (new_entry->dir.child))  {
            ramfs_unlock(fs->lock);
            return RAMFS_ERR_NOTEMPTY;
        }
    } else if (ret != RAMFS_ERR_NOENT) {
        ramfs_unlock(fs->lock);
        return ret;
    }

    if ((new_entry->flags & RAMFS_ENTRY_TYPE_MSK) == RAMFS_DIR) {
        // New and old entries are directories
        // New directory is empty

        // Move all sub-directories and files of the old entry to the new entry
        new_entry->dir.child = old_entry->dir.child;
    } else if ((old_entry->flags & RAMFS_ENTRY_TYPE_MSK) == RAMFS_DIR) {
        new_entry->dir.child = old_entry->dir.child;
    } else if ((old_entry->flags & RAMFS_ENTRY_TYPE_MSK) == RAMFS_FILE) {
        new_entry->file.header->head = old_entry->file.header->head;
        new_entry->file.header->tail = old_entry->file.header->tail;
        new_entry->file.header->size = old_entry->file.header->size;
    }

    remove_entry(fs, old_entry, parent_old_entry, prev_old_entry, 0);

    ramfs_unlock(fs->lock);

    return RAMFS_ERR_OK;
}

int ramfs_rmdir(ramfs_t *fs, const char *path) {
    ramfs_entry_t *entry;
    ramfs_entry_t *parent_entry;
    ramfs_entry_t *prev_entry;
    ramfs_error_t ret;

    if (strcmp(path,"/") == 0) {
        return RAMFS_ERR_BUSY;
    }

    ramfs_lock(fs->lock);

    ret = traverse(fs, path, &entry, &parent_entry, &prev_entry, 0, 0);
    if (ret != RAMFS_ERR_OK) {
        ramfs_unlock(fs->lock);
        return ret;
    }

    if ((entry->flags & RAMFS_ENTRY_TYPE_MSK) != RAMFS_DIR) {
        ramfs_unlock(fs->lock);
        return RAMFS_ERR_NOTDIR;
    }

    if (entry->dir.child) {
        ramfs_unlock(fs->lock);
        return RAMFS_ERR_NOTEMPTY;
    }

    remove_entry(fs, entry, parent_entry, prev_entry, 0);

    ramfs_unlock(fs->lock);

    return RAMFS_ERR_OK;
}

int ramfs_unlink(ramfs_t *fs, const char *pathname) {
    ramfs_entry_t *entry;
    ramfs_entry_t *parent_entry;
    ramfs_entry_t *prev_entry;
    ramfs_error_t ret;

    ramfs_lock(fs->lock);

    ret = traverse(fs, pathname, &entry, &parent_entry, &prev_entry, 0, 0);
    if (ret != RAMFS_ERR_OK) {
        ramfs_unlock(fs->lock);
        return ret;
    }

    if ((entry->flags & RAMFS_ENTRY_TYPE_MSK) != RAMFS_FILE) {
        ramfs_unlock(fs->lock);
        return RAMFS_ERR_PERM;
    }

    remove_entry(fs, entry, parent_entry, prev_entry, 1);

    ramfs_unlock(fs->lock);

    return RAMFS_ERR_OK;
}
