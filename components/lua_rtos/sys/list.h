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

#ifndef _LIST_H
#define	_LIST_H

#include <stdint.h>
#include <sys/mutex.h>

struct list {
    struct mtx mutex;
    struct list_index *index;
    struct list_index *free;
    uint8_t indexes;
    uint8_t first_index;
};

struct list_index {
    void *item;
    uint8_t index;
    uint8_t deleted;
    struct list_index *next;
};

void list_init(struct list *list, int first_index);
int list_add(struct list *list, void *item, int *item_index);
int list_get(struct list *list, int index, void **item);
int list_remove(struct list *list, int index, int destroy);
int list_first(struct list *list);
int list_next(struct list *list, int index);
void list_destroy(struct list *list, int items);

#endif	/* LIST_H */

