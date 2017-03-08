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

#include "lrotable.h"

#ifndef ROTABLE_CACHE_H
#define ROTABLE_CACHE_H

#define ROTABLE_CACHE_LENGTH 8

struct rotable_cache_entry {
	struct rotable_cache_entry *previous; // Previous entry
	struct rotable_cache_entry *next;     // Next entry

	luaR_entry *rotable; // cached rotable
	luaR_entry *entry;   // cached entry

	uint32_t used;
};

typedef struct {
	uint32_t miss; // Number of cache misses
	uint32_t hit;  // Number of cache hits

	struct rotable_cache_entry *first; // First entry
	struct rotable_cache_entry *last;  // Last entry
} rotable_cache_t;

void rotable_cache_dump();
int rotable_cache_init();
const TValue *rotable_cache_get(const luaR_entry *rotable, const char *strkey);
void rotable_cache_put(const luaR_entry *rotable, const luaR_entry *entry);

#endif

#endif
