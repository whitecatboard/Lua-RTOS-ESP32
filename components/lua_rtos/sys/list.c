/*
 * Lua RTOS, list data structure
 *
 * Copyright (C) 2015 - 2017
 * IBEROXARXA SERVICIOS INTEGRALES, S.L. & CSS IBÉRICA, S.L.
 * 
 * Author: Jaume Olivé (jolive@iberoxarxa.com / jolive@whitecatboard.org)
 * 
 * All rights reserved.  
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appear in all
 * copies and that both that the copyright notice and this
 * permission notice and warranty disclaimer appear in supporting
 * documentation, and that the name of the author not be used in
 * advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * The author disclaim all warranties with regard to this
 * software, including all implied warranties of merchantability
 * and fitness.  In no event shall the author be liable for any
 * special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether
 * in an action of contract, negligence or other tortious action,
 * arising out of or in connection with the use or performance of
 * this software.
 */

#include "esp_attr.h"

#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <sys/list.h>
#include <sys/mutex.h>

void list_init(struct list *list, int first_index) {
    // Create the mutex
    mtx_init(&list->mutex, NULL, NULL, 0);
    
    mtx_lock(&list->mutex);
    
    list->indexes =  0;
    list->free = NULL;
    list->index = NULL;
    list->first_index = first_index;
    
    mtx_unlock(&list->mutex);    
}

int list_add(struct list *list, void *item, int *item_index) {
    struct list_index *index = NULL;
    struct list_index *indexa = NULL;
    int grow = 0;
        
    mtx_lock(&list->mutex);
    
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
        indexa = (struct list_index *)malloc(sizeof(struct list_index) * list->indexes);     
        if (!indexa) {
            mtx_unlock(&list->mutex);
            return ENOMEM;            
        }
        
        if (list->index) {
            // Copy current index array to new created
            bcopy(list->index, indexa, sizeof(struct list_index) * (list->indexes - 1));

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
    *item_index = index->index + list->first_index;
            
    mtx_unlock(&list->mutex);
    
    return 0;
}

int IRAM_ATTR list_get(struct list *list, int index, void **item) {
    struct list_index *cindex = NULL;
    int iindex;

    mtx_lock(&list->mutex);

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
    
    *item = cindex->item;
    
    mtx_unlock(&list->mutex);

    return 0;
}

int list_remove(struct list *list, int index, int destroy) {
    struct list_index *cindex = NULL;
    int iindex;

    mtx_lock(&list->mutex);

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
    
    cindex->next = list->free;
    cindex->deleted = 1;
    list->free = cindex;
    
    mtx_unlock(&list->mutex);
    
    return 0;
}

int IRAM_ATTR list_first(struct list *list) {
    int index;
    int res = -1;
    
    mtx_lock(&list->mutex);
    
    for(index=0;index < list->indexes;index++) {
        if (!list->index[index].deleted) {
            res = index + list->first_index;
            break;
        }
    }
    
    mtx_unlock(&list->mutex);

    return res;
}

int IRAM_ATTR list_next(struct list *list, int index) {
    int res = -1;
    int iindex;
    
    mtx_lock(&list->mutex);

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
    
    mtx_unlock(&list->mutex);
    
    return res;
}

void list_destroy(struct list *list, int items) {
    int index;
    
    mtx_lock(&list->mutex);
    
    if (items) {
        for(index=0;index < list->indexes;index++) {
            if (!list->index[index].deleted) {
                free(list->index[index].item);
            }
        }        
    }
    
    free(list->index);
    
    mtx_unlock(&list->mutex);    
    mtx_destroy(&list->mutex);
}
