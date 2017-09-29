/*
 * Lua RTOS, Read Only tables cache
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

#include "luartos.h"

#if LUA_USE_ROTABLE && CONFIG_LUA_RTOS_LUA_USE_ROTABLE_CACHE

#include "cache.h"

#include "esp_attr.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>


static rotable_cache_t cache;

void rotable_cache_dump() {
	struct rotable_cache_entry *entry = cache.first;
	int i = 0;

	mtx_lock(&cache.mtx);

	while (entry) {
		printf("[%d]: used %d ", i, entry->used);

		if (entry->rotable) {
			if (entry->entry->key.len) {
				printf("%s\r\n", entry->entry->key.id.strkey);
			}
		} else {
			printf("emppty\r\n");
		}

		entry = entry->next;
		i++;
	}

	printf("\r\n");

	printf("hit: %d, miss: %d\r\n", cache.hit, cache.miss);

	mtx_unlock(&cache.mtx);

	printf("\r\n\r\n");
}

int rotable_cache_init() {
	struct rotable_cache_entry *entries;
	int i;

	// Allocate space for cache entries
	entries = (struct rotable_cache_entry *)calloc(ROTABLE_CACHE_LENGTH, sizeof(struct rotable_cache_entry));
	if (!entries) {
		return 1;
	}

	cache.first = entries;

	// Initialize linked list
	entries->previous = NULL;

	for(i = 0;i < ROTABLE_CACHE_LENGTH;i++) {
		entries->next = (entries + 1);

		if (i > 0) {
			entries->previous = (entries - 1);
		}
		entries++;
	}

	entries--;

	entries->next = NULL;

	cache.last = entries;

	// Init mutex
	mtx_init(&cache.mtx, NULL, NULL, 0);

	return 0;
}

const IRAM_ATTR TValue *rotable_cache_get(const luaR_entry *rotable, const char *strkey) {
	struct rotable_cache_entry *entry = cache.first;

	mtx_lock(&cache.mtx);

	// rotable strkey is cached?
	while (entry) {
		if ((entry->rotable == rotable) && (!strcmp(entry->entry->key.id.strkey, strkey))) {
			// hit
			cache.hit++;
			entry->used++;

			// Promote this entry to the head
			if (entry != cache.first) {
				if (entry != cache.last) {
					entry->previous->next = entry->next;
					entry->next->previous = entry->previous;
					cache.first->previous = entry;
					entry->next = cache.first;
					cache.first = entry;
				} else {
					cache.last->previous->next = NULL;
					cache.last = cache.last->previous;
					entry->next = cache.first;
					entry->previous = NULL;
					cache.first->previous = entry;
					cache.first = entry;
				}
			}

			mtx_unlock(&cache.mtx);
			return &entry->entry->value;
			break;
		}

		entry = entry->next;
	}

	// miss
	cache.miss++;

	mtx_unlock(&cache.mtx);

	return NULL;
}

void IRAM_ATTR rotable_cache_put(const luaR_entry *rotable, const luaR_entry *entry) {
	mtx_lock(&cache.mtx);

	struct rotable_cache_entry *first  = cache.last;
	struct rotable_cache_entry *second = cache.first;
	struct rotable_cache_entry *last   = cache.last->previous;

	first->next      = second;
	first->previous  = NULL;
	second->previous = first;
	last->next       = NULL;

	first->rotable = (luaR_entry *)rotable;
	first->entry = (luaR_entry *)entry;

	cache.first = first;
	cache.first->used = 1;
	cache.last = last;

	mtx_unlock(&cache.mtx);
}

/*

bisect, 10000 times

cache = 0, 6.441 secs
cache = 8, 5.055 secs

*/
#endif
