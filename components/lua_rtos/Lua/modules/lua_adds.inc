/*
 * Lua RTOS, adds to lua.c
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

#include "pthread.h"
#include "linenoise.h"
#include "shell.h"
#include "cache.h"

#include <limits.h>

#include <unistd.h>
#include <sys/status.h>
#include <sys/debug.h>

static int dofile (lua_State *L, const char *name);

extern int threadInited;

#if LUA_USE_LUA_LOCK
pthread_mutex_t lua_mutex;

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
  
#if LUA_USE_SHELL
  if (status_get(STATUS_LUA_SHELL)) {
	  lua_shell(L, buffer);
  }
#endif
  
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
  status_set(STATUS_LUA_RUNNING);

#if LUA_USE_ROTABLE && CONFIG_LUA_RTOS_LUA_USE_ROTABLE_CACHE
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
  
  status_set(STATUS_LUA_INTERPRETER);

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
