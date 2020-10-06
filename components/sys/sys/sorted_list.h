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
 * Lua RTOS, sorted list data structure
 *
 */

#ifndef _SYS_SORTED_LIST_H_
#define _SYS_SORTED_LIST_H_

#include "pool.h"

#include <stdint.h>

typedef int8_t (*sorted_list_cmp_func_t)(void *, void *);

struct sorted_list_item {
  struct sorted_list_item *prev;
  struct sorted_list_item *next;
  void *obj;
  uint8_t pool_index;
};

typedef struct {
  sorted_list_cmp_func_t cmp;
  struct sorted_list_item *first;
  uint8_t is_static;
  mem_pool_t *pool;
} sorted_list_t;

typedef struct {
  sorted_list_t *list;
  struct sorted_list_item *current;
} sorted_list_iterator_t;

/**
 * @brief Configure a sorted list.
 *
 * @param list   A pointer to a sorted_list_t structure, with the sorted list data.
 *
 * @param cmp    A pointer to a custom user compare function. This function compares 2 sorted list items,
 *               and must return 0 when the items are equal, -1 when the first item is smaller than the
 *               second one, and 1 when the first item is greater that the other one.
 *
 * @return
 *     0 if all is ok, otherwise one of the following error codes:
 *
 *       EINVAL: sanity check error
 *       ENOMEM: not enough memory
 */
int sorted_list_setup(sorted_list_t *list, sorted_list_cmp_func_t cmp);

/**
 * @brief Configure a sorted list, using an internal preallocated pool of list of items. This type of list is intended
 *        to be used inside ISRs to avoid the use of heap operations, that can introduce delays in ISR processing.
 *
 * @param list   A pointer to a sorted_list_t structure, with the sorted list data.
 *
 * @param cmp    A pointer to a custom user compare function. This function compares 2 sorted list items,
 *               and must return 0 when the items are equal, -1 when the first item is smaller than the
 *               second one, and 1 when the first item is greater that the other one.
 *
 * @return
 *     0 if all is ok, otherwise one of the following error codes:
 *
 *       EINVAL: sanity check error
 *       ENOMEM: not enough memory
 */
int sorted_list_setup_static(sorted_list_t *list, sorted_list_cmp_func_t cmp);

/**
 * @brief Add an item into a sorted list.
 *
 * @param list      A pointer to a sorted_list_t structure, with the sorted list data.
 * @param obj       A pointer to the object to be added.
 * @param iterator  A pointer to a sorted_list_iterator_t structure, with the iterator data.
 *                  This argument can be NULL. If not NULL, iterator points to the added
 *                  item.
 *
 * @return
 *     0 if all is ok, otherwise one of the following error codes:
 *
 *       EINVAL: sanity check error
 *       ENOMEM: not enough memory
 */
int sorted_list_add(sorted_list_t *list, void *obj, sorted_list_iterator_t *iterator);

/**
 * @brief Get an iterator over a sorted list.
 *
 * @param list     A pointer to a sorted_list_t structure, with the sorted list data.
 * @param iterator A pointer to a sorted_list_iterator_t structure, with the iterator data.
 *
 * @return
 *     0 if all is ok, otherwise one of the following error codes:
 *
 *       EINVAL: sanity check error
 */
int sorted_list_iterate(sorted_list_t *list, sorted_list_iterator_t *iterator);

/**
 * @brief Get an iterator over a sorted list. Fast version, without sanity checks and with
 *        inline code.
 *
 * @param list     A pointer to a sorted_list_t structure, with the sorted list data.
 * @param iterator A pointer to a sorted_list_iterator_t structure, with the iterator data.
 */
void sorted_list_iterate_fast(sorted_list_t *list, sorted_list_iterator_t *iterator);

/**
 * @brief Obtain the next item of a sorted list using a specific iterator.
 *
 * @param iterator     A pointer to a sorted_list_iterator_t structure, with the iterator data.
 * @param item        A pointer to a pointer that will contain the reference to the item.
 *                     If there are no more items, NULL is returned.
 *
 * @return
 *     0 if all is ok, otherwise one of the following error codes:
 *
 *       EINVAL: sanity check error
 */
int sorted_list_next(sorted_list_iterator_t *iterator, void **item);

/**
 * @brief Obtain the next item of a sorted list using a specific iterator. Fast version, without sanity checks and
 *        with inline code.
 *
 * @param iterator    A pointer to a sorted_list_iterator_t structure, with the iterator data.
 * @param item        A pointer to a pointer that will contain the reference to the item.
 *                    If there are no more items, NULL is returned.
 */
void sorted_list_next_fast(sorted_list_iterator_t *iterator, void **item);

/**
 * @brief Obtain the previous item of a sorted list using a specific iterator.
 *
 * @param iterator     A pointer to a sorted_list_iterator_t structure, with the iterator data.
 * @param item         A pointer to a pointer that will contain the reference to the item.
 *                     If there are no more items, NULL is returned.
 *
 * @return
 *     0 if all is ok, otherwise one of the following error codes:
 *
 *       EINVAL: sanity check error
 */
int sorted_list_previous(sorted_list_iterator_t *iterator, void **item);

/**
 * @brief Obtain the previous item of a sorted list using a specific iterator. Fast version, without sanity checks and
 *        with inline code.
 *
 * @param iterator     A pointer to a sorted_list_iterator_t structure, with the iterator data.
 * @param item         A pointer to a pointer that will contain the reference to the item.
 *                     If there are no more items, NULL is returned.
 */
void sorted_list_previous_fast(sorted_list_iterator_t *iterator, void **item);

/**
 * @brief Remove the sorted list item which is pointer by the iterator. After removing, the
 *        iterator points to the previous item.
 *
 * @param iterator     A pointer to a sorted_list_iterator_t structure, with the iterator data.
 *
 * @return
 *     0 if all is ok, otherwise one of the following error codes:
 *
 *       EINVAL: sanity check error
 */
int sorted_list_remove(sorted_list_iterator_t *iterator);

/**
 * @brief Remove the sorted list item which is pointer by the iterator. After removing, the
 *        iterator points to the previous item. Fast version, without sanity checks and with
 *        inline code.
 *
 * @param iterator     A pointer to a sorted_list_iterator_t structure, with the iterator data.
 *
 */
void sorted_list_remove_fast(sorted_list_iterator_t *iterator);

#endif /* _SYS_SORTED_LIST_H_ */
