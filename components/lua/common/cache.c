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
 * Lua RTOS read only table cache
 *
 */

#include "luartos.h"

#if LUA_USE_ROTABLE && CONFIG_LUA_RTOS_LUA_USE_ROTABLE_CACHE && !CONFIG_LUA_RTOS_LUA_USE_JIT_BYTECODE_OPTIMIZER

#include "cache.h"

#include "esp_attr.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>


static rotable_cache_t cache;
static portMUX_TYPE lock = portMUX_INITIALIZER_UNLOCKED;

void rotable_cache_dump() {
	struct rotable_cache_entry *entry = cache.first;
	int i = 0;

	portENTER_CRITICAL(&lock);

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

	portEXIT_CRITICAL(&lock);

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

	return 0;
}

const IRAM_ATTR TValue *rotable_cache_get(const luaR_entry *rotable, const char *strkey) {
	// rotable strkey is cached?
	int len = strlen(strkey);

    portENTER_CRITICAL(&lock);

    struct rotable_cache_entry *entry = cache.first;

	while (entry) {
		if ((entry->rotable == rotable) && (entry->entry->key.len == len) &&  (!strcmp(entry->entry->key.id.strkey, strkey))) {
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

			portEXIT_CRITICAL(&lock);
			return &entry->entry->value;
		}

		entry = entry->next;
	}

	// miss
	cache.miss++;

	portEXIT_CRITICAL(&lock);

	return NULL;
}

void IRAM_ATTR rotable_cache_put(const luaR_entry *rotable, const luaR_entry *entry) {
    portENTER_CRITICAL(&lock);

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

	portEXIT_CRITICAL(&lock);
}

/*

bisect, 10000 times

cache = 0, 6.441 secs
cache = 8, 5.055 secs

*/
#endif
