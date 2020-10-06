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
 * Lua RTOS, memory pool management
 *
 */
#include "sdkconfig.h"

#include "pool.h"
#include "esp_attr.h"

#include <errno.h>
#include <stdlib.h>

int pool_setup(size_t size, size_t item_size, mem_pool_t **pool) {
  if (item_size > 32) {
    *pool = NULL;
    return EINVAL;
  }

  *pool = calloc(1, sizeof(mem_pool_t));
  if (*pool == NULL) {
    *pool = NULL;
    return ENOMEM;
  }

  // Allocate pool data
  (*pool)->data = calloc(size, item_size);
  if ((*pool)->data == NULL) {
    free(*pool);
    *pool = NULL;
    return ENOMEM;
  }

  // All items in pool free
  (*pool)->status = 0xffffffff;

  (*pool)->size = size;
  (*pool)->item_size = item_size;

  return 0;
}

void *IRAM_ATTR pool_get(mem_pool_t *pool, uint8_t *item_id) {
  if (item_id == NULL) {
    return NULL;
  }

  if (pool == NULL) {
    return NULL;
  }

  // Get the first unused element of the pool
  uint8_t uiFirstFree = __builtin_ffs(pool->status);

  if (uiFirstFree == 0) {
    // No more space on pool
    return NULL;
  }

  *item_id = (uiFirstFree - 1);

  // Update pool status
  pool->status &= ~(1 << (uiFirstFree - 1));

  // Return data
  return pool->data + (pool->item_size * (uiFirstFree - 1));
}

void IRAM_ATTR pool_free(mem_pool_t *pool, uint8_t item_id) {
  if (pool == NULL) {
    return;
  }

  // Update pool status
  pool->status |= (1 << item_id);
}
