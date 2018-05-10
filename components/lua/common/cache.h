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

#include "sdkconfig.h"

#if LUA_USE_ROTABLE && CONFIG_LUA_RTOS_LUA_USE_ROTABLE_CACHE && !CONFIG_LUA_RTOS_LUA_USE_JIT_BYTECODE_OPTIMIZER

#include "lrotable.h"

#ifndef ROTABLE_CACHE_H
#define ROTABLE_CACHE_H

#define ROTABLE_CACHE_LENGTH 8

#include <sys/mutex.h>

struct rotable_cache_entry {
	struct rotable_cache_entry *previous; // Previous entry
	struct rotable_cache_entry *next;     // Next entry

	luaR_entry *rotable; // cached rotable
	luaR_entry *entry;   // cached entry
	uint32_t used;
};

typedef struct {
	uint32_t miss;     // Number of cache misses
	uint32_t hit;      // Number of cache hits

	struct rotable_cache_entry *first; // First entry
	struct rotable_cache_entry *last;  // Last entry
} rotable_cache_t;

void rotable_cache_dump();
int rotable_cache_init();
const TValue *rotable_cache_get(const luaR_entry *rotable, const char *strkey);
void rotable_cache_put(const luaR_entry *rotable, const luaR_entry *entry);

#endif

#endif
