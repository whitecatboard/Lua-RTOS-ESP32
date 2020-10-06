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

#include "sdkconfig.h"

#include "esp_attr.h"
#include "pool.h"
#include "sorted_list.h"

#include <errno.h>
#include <stdlib.h>

int sorted_list_setup(sorted_list_t *list, sorted_list_cmp_func_t cmp) {
  // Sanity checks
  if (list == NULL) {
    return EINVAL;
  }

  if (cmp == NULL) {
    return EINVAL;
  }

  list->cmp = cmp;
  list->first = NULL;
  list->is_static = 0;

  return 0;
}

int sorted_list_setup_static(sorted_list_t *list, sorted_list_cmp_func_t cmp) {
  // Sanity checks
  if (list == NULL) {
    return EINVAL;
  }

  if (cmp == NULL) {
    return EINVAL;
  }

  // Allocate pool
  int err = pool_setup(32, sizeof(struct sorted_list_item), &list->pool);

  list->cmp = cmp;
  list->first = NULL;
  list->is_static = 1;

  return err;
}

int IRAM_ATTR sorted_list_add(sorted_list_t *list, void *obj, sorted_list_iterator_t *iterator) {
  struct sorted_list_item *current;
  struct sorted_list_item *prev;

  // Sanity checks
  if (list == NULL) {
    return EINVAL;
  }

  // Create new item, or get it from pool
  struct sorted_list_item *item;

  if (list->is_static) {
    uint8_t item_id;

    item = (struct sorted_list_item *)pool_get(list->pool, &item_id);
    if (item != NULL) {
      item->pool_index = item_id;
    }
  } else {
    item = calloc(1, sizeof(struct sorted_list_item));
  }

  if (item == NULL) {
    return ENOMEM;
  }

  item->obj = obj;
  item->prev = NULL;
  item->next = NULL;

  // Insert item
  prev = NULL;
  current = list->first;

  while (current) {
    if (list->cmp(obj, current->obj) == -1) {
      // Smaller
      break;
    }

    prev = current;
    current = current->next;
  }

  if ((prev == NULL) && (current == NULL)) {
    // No items on list, new item is the first item
    list->first = item;
  } else if ((prev == NULL) && (current != NULL)) {
    // Insert prior to the first item
    list->first = item;

    item->next = current;
    current->prev = item;
  } else if ((prev != NULL) && (current != NULL)) {
    // Insert between 2 items
    prev->next = item;
    item->prev = prev;
    item->next = current;
    current->prev = item;
  } else if ((prev != NULL) && (current == NULL)) {
    // Insert before last item
    prev->next = item;
    item->prev = prev;
    item->next = NULL;
  }

  if (iterator != NULL) {
    iterator->current = item->obj;
  }

  return 0;
}

inline void IRAM_ATTR sorted_list_iterate_fast(sorted_list_t *list, sorted_list_iterator_t *iterator) {
  iterator->list = list;
  iterator->current = NULL;
}

int IRAM_ATTR sorted_list_iterate(sorted_list_t *list, sorted_list_iterator_t *iterator) {
  if (list == NULL) {
    return EINVAL;
  }

  if (iterator == NULL) {
    return EINVAL;
  }

  sorted_list_iterate_fast(list, iterator);

  return 0;
}

inline void IRAM_ATTR sorted_list_next_fast(sorted_list_iterator_t *iterator, void **item) {
  *item = NULL;

  if (iterator->current == NULL) {
    iterator->current = iterator->list->first;
  } else {
    iterator->current = iterator->current->next;
  }

  if (iterator->current != NULL) {
    *item = iterator->current->obj;
  }
}

int IRAM_ATTR sorted_list_next(sorted_list_iterator_t *iterator, void **item) {
  // Sanity checks
  if (iterator == NULL) {
    return EINVAL;
  }

  if (item == NULL) {
    return EINVAL;
  }

  sorted_list_next_fast(iterator, item);

  return 0;
}

inline void IRAM_ATTR sorted_list_previous_fast(sorted_list_iterator_t *iterator, void **item) {
	  *item = NULL;

	  if (iterator->current != NULL) {
	    iterator->current = iterator->current->prev;
	  }

	  if (iterator->current != NULL) {
	    *item = iterator->current->obj;
	  }
}

int IRAM_ATTR sorted_list_previous(sorted_list_iterator_t *iterator, void **item) {
  // Sanity checks
  if (iterator == NULL) {
    return EINVAL;
  }

  if (item == NULL) {
    return EINVAL;
  }

  sorted_list_previous_fast(iterator, item);

  return 0;
}

inline void IRAM_ATTR sorted_list_remove_fast(sorted_list_iterator_t *iterator) {
  if (iterator->current != NULL) {
    if (iterator->current == iterator->list->first) {
      // Remove first item
      iterator->list->first = iterator->current->next;

      if (iterator->list->first != NULL) {
        iterator->list->first->prev = NULL;
      }
    } else {
      if (iterator->current->next != NULL) {
        iterator->current->next->prev = iterator->current->prev;
      }

      if (iterator->current->prev != NULL) {
        iterator->current->prev->next = iterator->current->next;
      }
    }

    if (iterator->list->is_static) {
      pool_free(iterator->list->pool, iterator->current->pool_index);
      iterator->current = iterator->current->prev;
    } else {
      // Free memory
      struct sorted_list_item *dummy = iterator->current;

      iterator->current = iterator->current->prev;

      free(dummy);
    }
  }
}

int IRAM_ATTR sorted_list_remove(sorted_list_iterator_t *iterator) {
  // Sanity checks
  if (iterator == NULL) {
    return EINVAL;
  }

  sorted_list_remove_fast(iterator);

  return 0;
}
