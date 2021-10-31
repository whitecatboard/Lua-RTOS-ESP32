/*
 * Copyright (C) 2015 - 2020, IBEROXARXA SERVICIOS INTEGRALES, S.L.
 * Copyright (C) 2015 - 2020, Jaume Olivé Petrus (jolive@whitecatboard.org)
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
 * Lua RTOS, lua.c additions
 *
 */

#include "pthread.h"
#include "linenoise.h"
#include "shell.h"
#include "cache.h"
#include "luaconf.h"

#include <limits.h>

#include <unistd.h>
#include <sys/status.h>
#include <sys/debug.h>

static int dofile (lua_State *L, const char *name);

extern int threadInited;

#if CONFIG_LUA_RTOS_LUA_USE_LOCKS && !CONFIG_LUA_RTOS_LUA_USE_JIT_BYTECODE_OPTIMIZER
#include <pthread.h>

pthread_mutex_t lua_mutex = PTHREAD_MUTEX_INITIALIZER;

inline void LuaLockInit(lua_State *L) {
    pthread_mutexattr_t attr;

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

    pthread_mutex_init(&lua_mutex, &attr);
}

inline void LuaLock(lua_State *L) {
    pthread_mutex_lock(&lua_mutex);
}

inline void LuaUnlock(lua_State *L) {
    pthread_mutex_unlock(&lua_mutex);
}
#else
#define LuaLockInit(L)
#define LuaLock(L)
#define LuaUnlock(L)
#endif

static void get_prompt (lua_State *L, int firstline, char *path) {
  lua_getglobal(L, firstline ? "_PROMPT" : "_PROMPT2");
  char *p = (char *)lua_tostring(L, -1);

  if (p == NULL) {
      if (firstline) {
          if (getcwd(path, PATH_MAX + 10)) {
              path = strncat(path, " ", PATH_MAX + 10);
              path = strncat(path, LUA_PROMPT, PATH_MAX + 10);
          }
      } else {

		  strncpy(path, LUA_PROMPT2, PATH_MAX + 10);
      }
  } else {
      strncpy(path, p, PATH_MAX + 10);
  }
}

static int luaos_pushline (lua_State *L, int firstline) {
  char buffer[LUA_MAXINPUT];
  char prmt[PATH_MAX + 21];

  char *b = buffer;
  size_t l;

  get_prompt(L, firstline, prmt);

  int readstatus = lua_readline(L, b, prmt);
  if (readstatus == 0)
    return 0;  /* no input (prompt will be popped by caller) */

  if (status_get(STATUS_LUA_SHELL)) {
	  lua_shell(L, buffer);
  }

  lua_pop(L, 1);  /* remove prompt */
  l = strlen(b);
  if (l > 0 && b[l-1] == '\n')  /* line ends with newline? */
    b[--l] = '\0';  /* remove it */
  if (firstline && b[0] == '=')  /* for compatibility with 5.2, ... */
    lua_pushfstring(L, "return %s", b + 1);  /* change '=' to 'return' */
  else
    lua_pushlstring(L, b, l);

  lua_freeline(L, b);

  return 1;
}

// Execute system.lua and autorun.lua at boot time
static void luaos_boot_scripts(lua_State *L) {
    int  system, autorun;
    FILE *fp;
    int i;

    const char *system_order[2];
    const char *autorun_order[2];

    system_order[0] = "/system.lua";
    system_order[1] = NULL;

#ifdef RUN_LUA_TESTS
    chdir("/tests");
    autorun_order[0] = "/tests/test.lua";
#else
    autorun_order[0] = "/autorun.lua";
#endif

    autorun_order[1] = NULL;

    printf("\n");

    // Ecexute system script
    system = -1;
    for(i = 0; i < sizeof(system_order) / sizeof(*system_order); i++) {
        if (system_order[i]) {
            if ((fp = fopen(system_order[i], "r" ))) {
                fclose(fp);
                system = i;
                break;
            }
        }
    }

    if (system >=0) {
        printf("Executing %s ...\n", system_order[system]);
        dofile(L, system_order[system]);
    }

    // Ecexute autorun script
    autorun = -1;
    for(i = 0; i < sizeof(autorun_order) / sizeof(*autorun_order); i++) {
        if (autorun_order[i]) {
            if ((fp = fopen(autorun_order[i], "r" ))) {
                fclose(fp);
                autorun = i;
                break;
            }
        }
    }

    if (autorun >=0) {
        printf("Executing %s ...\n", autorun_order[autorun]);
        dofile(L, autorun_order[autorun]);
    }

    printf("\n");
}

// LuaOS version of pmain
//
// Changes from original source are:
//
// * LuaOS always starts in interactive mode, removed unnecessary code
// * Run system.lua before executing the interpreter (in same state)
// * Run autorun.lua before executing the interpreter (in same state)
static int luaos_pmain (lua_State *L) {
  status_set(STATUS_LUA_RUNNING, 0x00000000);

#if LUA_USE_ROTABLE && CONFIG_LUA_RTOS_LUA_USE_ROTABLE_CACHE && !CONFIG_LUA_RTOS_LUA_USE_JIT_BYTECODE_OPTIMIZER
    rotable_cache_init();
#endif

  debug_free_mem_begin(luaL_openlibs);
  luaL_openlibs(L);  /* open standard libraries */
  debug_free_mem_end(luaL_openlibs, NULL);

  print_version();

  if (!status_get(STATUS_LUA_ABORT_BOOT_SCRIPTS)) {
	  luaos_boot_scripts(L);
  } else {
	  printf("Lua RTOS-boot-scripts-aborted-ESP32\r\n");
  }

  status_set(STATUS_LUA_INTERPRETER, 0x00000000);

  for(;;)
      doREPL(L);  /* do read-eval-print loop */

  lua_pushboolean(L, 1);  /* signal no errors */
  return 1;
}

// LuaOS version of lua_main
//
// Changes from original source are:
//
// * Create a global reference to Lua state for further access outside Lua
// * Call to LuaOS version of pmain
int luaos_main (void) {
  int status, result;

  LuaLockInit(NULL);

  debug_free_mem_begin(luaL_newstate);
  lua_State *L = luaL_newstate();  /* create state */
  debug_free_mem_end(luaL_newstate, NULL);
  if (L == NULL) {
    l_message("lua", "cannot create state: not enough memory");
    return EXIT_FAILURE;
  }

  //WHITECAT BEGIN
  uxSetLuaState(L);
  //WHITECAT END

  lua_pushcfunction(L, &luaos_pmain);  /* to call 'pmain' in protected mode */
  status = lua_pcall(L, 0, 1, 0);  /* do the call */

  result = lua_toboolean(L, -1);  /* get result */
  report(L, status);

  lua_close(L);

  return (result && status == LUA_OK) ? EXIT_SUCCESS : EXIT_FAILURE;
}
