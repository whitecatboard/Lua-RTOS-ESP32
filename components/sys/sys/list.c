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
 * Lua RTOS list data structure
 *
 */

#include "esp_attr.h"

#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <sys/list.h>
#include <sys/mutex.h>

void lstinit(struct list *list, int first_index, uint8_t flags) {
    // Create the mutex
    mtx_init(&list->mutex, NULL, NULL, 0);

    mtx_lock(&list->mutex);

    list->indexes = 0;
    list->free = NULL;
    list->index = NULL;
    list->last = NULL;
    list->first_index = first_index;
    list->flags = flags;
    list->init = 1;

    mtx_unlock(&list->mutex);
}

int lstadd(struct list *list, void *item, int *item_index) {
    struct lstindex *index = NULL;
    struct lstindex *indexa = NULL;
    int grow = 0;

    mtx_lock(&list->mutex);

    if (list->flags & LIST_DEFAULT) {
        // Get an index
        if (list->free) {
            // Get first free element
            index = list->free;
            list->free = index->next;
        } else {
            // Must grow index array
            grow = 1;
        }

        if (grow) {
            // Increment index count
            list->indexes++;

            // Create a new index array for allocate new index
            indexa = (struct lstindex *)malloc(sizeof(struct lstindex) * list->indexes);
            if (!indexa) {
                mtx_unlock(&list->mutex);
                return ENOMEM;
            }

            if (list->index) {
                // Copy current index array to new created
                bcopy(list->index, indexa, sizeof(struct lstindex) * (list->indexes - 1));

                // Free current index array
                free(list->index);
            }

            // Store new index array
            list->index = indexa;

            // Current index
            index = list->index + list->indexes - 1;

            // Initialize new index
            index->index = list->indexes - 1;
        }

        index->next = NULL;
        index->item = item;
        index->deleted = 0;

        // Return index
        if (item_index) {
                *item_index = index->index + list->first_index;
        }
    } else if (list->flags & LIST_NOT_INDEXED) {
            // Create a new element
            index = (struct lstindex *)calloc(1, sizeof(struct lstindex));
            if (!index) {
                return ENOMEM;
            }

        index->item = item;

        if ((list->index == NULL) && (list->last == NULL)) {
                // First element
                list->index = index;
            } else {
                // Almost there is one element in list
                assert(list->last != NULL);

                list->last->next = index;
                index->previous = list->last;
            }

            list->last = index;

             if (item_index) {
                 *item_index = (int)item;
             }
    }

    mtx_unlock(&list->mutex);

    return 0;
}

int IRAM_ATTR lstget(struct list *list, int index, void **item) {
    struct lstindex *cindex = NULL;
    int iindex;

    mtx_lock(&list->mutex);

    if (list->flags & LIST_DEFAULT) {
        if (!list->indexes) {
            mtx_unlock(&list->mutex);
            return EINVAL;
        }

        // Check index
        if (index < list->first_index) {
            mtx_unlock(&list->mutex);
            return EINVAL;
        }

        // Get new internal index
        iindex = index - list->first_index;

        // Test for a valid index
        if (iindex > list->indexes) {
            mtx_unlock(&list->mutex);
            return EINVAL;
        }

        cindex = list->index + iindex;

        if (cindex->deleted) {
            mtx_unlock(&list->mutex);
            return EINVAL;
        }
    } else if (list->flags & LIST_NOT_INDEXED) {
            cindex = list->index;

            while (cindex) {
                if (cindex->item == (void *)index) {
                    *item = cindex->item;
                    break;
                }

                cindex = cindex->next;
            }

            if (!cindex) {
                mtx_unlock(&list->mutex);
                return EINVAL;
            }
    }

    *item = cindex->item;

    mtx_unlock(&list->mutex);

    return 0;
}

int lstremovec(struct list *list, int index, int destroy, bool compact) {
    struct lstindex *cindex = NULL;
    int iindex;

    mtx_lock(&list->mutex);

    if (list->flags & LIST_DEFAULT) {
        // Check index
        if (index < list->first_index) {
            mtx_unlock(&list->mutex);
            return EINVAL;
        }

        // Get new internal index
        iindex = index - list->first_index;

        // Test for a valid index
        if ((iindex < 0) || (iindex > list->indexes)) {
            mtx_unlock(&list->mutex);
            return EINVAL;
        }

        cindex = &list->index[iindex];

        if (destroy) {
            free(cindex->item);
        }

        if (compact) {
            bcopy(&list->index[iindex+1], &list->index[iindex], sizeof(struct lstindex) * (list->indexes - iindex - 1));
            iindex = list->indexes-1;
            cindex = &list->index[iindex];
        }

        cindex->next = list->free;
        cindex->deleted = 1;
        list->free = cindex;
    } else if (list->flags & LIST_NOT_INDEXED) {
            cindex = list->index;

            while (cindex) {
                if (cindex->item == (void *)index) {
                    if (cindex->next) {
                        cindex->next->previous = cindex->previous;
                    } else {
                        list->last = cindex->previous;
                    }

                    if (cindex->previous) {
                        cindex->previous->next = cindex->next;
                    } else {
                        list->index = cindex->next;
                    }

                    if (destroy) {
                        free(cindex->item);
                    }

                    free(cindex);

                    break;
                }

                cindex = cindex->next;
            }

    }

    mtx_unlock(&list->mutex);

    return 0;
}

int lstremove(struct list *list, int index, int destroy) {
    return lstremovec(list, index, destroy, false);
}

int IRAM_ATTR lstfirst(struct list *list) {
    int index;
    int res = -1;

    mtx_lock(&list->mutex);

    if (list->flags & LIST_DEFAULT) {
        for(index=0;index < list->indexes;index++) {
            if (!list->index[index].deleted) {
                res = index + list->first_index;
                break;
            }
        }
    } else if (list->flags & LIST_NOT_INDEXED) {
            if ((list->index != NULL) && (list->last != NULL)) {
                res = (int)list->index;
            }
    }

    mtx_unlock(&list->mutex);

    return res;
}

int IRAM_ATTR lstlast(struct list *list) {
    int index;
    int res = -1;

    mtx_lock(&list->mutex);

    if (list->flags & LIST_DEFAULT) {
        for(index = list->indexes - 1;index >= 0;index--) {
            if (!list->index[index].deleted) {
                res = index + list->first_index;
                break;
            }
        }
    } else if (list->flags & LIST_NOT_INDEXED) {
            if ((list->index != NULL) && (list->last != NULL)) {
                res = (int)list->last;
            }
    }

    mtx_unlock(&list->mutex);

    return res;
}

int IRAM_ATTR lstnext(struct list *list, int index) {
    int res = -1;
    int iindex;

    mtx_lock(&list->mutex);

    if (list->flags & LIST_DEFAULT) {
        // Check index
        if (index < list->first_index) {
            mtx_unlock(&list->mutex);
            return -1;
        }

        // Get new internal index
        iindex = index - list->first_index + 1;

        // Get next non deleted item on list
        for(;iindex < list->indexes;iindex++) {
            if (!list->index[iindex].deleted) {
               res = iindex + list->first_index;
               break;
            }
        }
    } else if (list->flags & LIST_NOT_INDEXED) {
            struct lstindex *cindex = NULL;

            cindex = list->index;

            while (cindex) {
                if (cindex->item == (void *)index) {
                    if (cindex->next) {
                        res = (int)cindex->next;
                    }

                    break;
                }

                cindex = cindex->next;
            }
    }

    mtx_unlock(&list->mutex);

    return res;
}

void lstdestroy(struct list *list, int items) {
    int index;

    if (!list->init) return;

    mtx_lock(&list->mutex);

    if (list->flags & LIST_DEFAULT) {
        if (items) {
            for(index=0;index < list->indexes;index++) {
                if (!list->index[index].deleted) {
                    free(list->index[index].item);
                }
            }
        }

        if (0 != list->index) {
            free(list->index);
            list->index = 0;
        }

    } else if (list->flags & LIST_NOT_INDEXED) {
        struct lstindex *cindex = NULL;
        struct lstindex *pindex = NULL;

        cindex = list->index;

        while (cindex) {
            pindex = cindex;
            cindex = cindex->next;

            free(pindex);
        }
    }

    list->init = 0;

    mtx_unlock(&list->mutex);
    mtx_destroy(&list->mutex);
}
