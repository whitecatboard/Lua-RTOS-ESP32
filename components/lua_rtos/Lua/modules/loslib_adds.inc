#include "lauxlib.h"

#include "luartos.h"

#include <freertos/FreeRTOS.h>

#include <limits.h>

#include <sys/dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <vfs.h>

#include <sys/stat.h>
#include <sys/syslog.h>
#include <sys/status.h>
#include <sys/console.h>
#include <drivers/cpu.h>
#include <sys/mount.h>

#include <drivers/uart.h>

extern const char *__progname;
extern uint32_t boot_count;
extern uint8_t flash_unique_id[8];

char lua_syslog_level = 0xff;
FILE *lua_stdout_file = NULL;

void luaC_fullgc (lua_State *L, int isemergency) ;
int edit_main(int argc, char *argv[]);

static int os_stdout(lua_State *L) {
    int total = lua_gettop(L);
    const char *path = NULL;

    if (total == 1) {
    	path = luaL_checkstring(L, 1);
    }

    if (path) {
    	if (lua_stdout_file) {
    		fclose(lua_stdout_file);
    	}

    	lua_stdout_file = fopen(path,"a+");
    } else {
    	if (lua_stdout_file) {
    		fclose(lua_stdout_file);
    		lua_stdout_file = NULL;
    	}
    }
    return 0;
}

static int os_shell(lua_State *L) { 
    luaL_checktype(L, 1, LUA_TBOOLEAN);

	if (lua_toboolean( L, 1 )) {
		status_set(STATUS_LUA_SHELL);
	} else {
		status_clear(STATUS_LUA_SHELL);
	}
    
    return 0;
}

#if LUA_USE_EDITOR
static int os_edit (lua_State *L) {
    const char *path = luaL_checkstring(L, 1);

    // Create file if does not exists
    FILE *fp = fopen(path, "a");
    if (!fp) {
        return luaL_fileresult(L, 0, path);
    }
    fclose(fp);
  
    char* lua_argv[] = {(char *)"edit", (char *)path, NULL};
    edit_main(2, lua_argv);
    console_clear();
    return 0;    
}
#endif

static int os_sleep(lua_State *L) {
	unsigned int seconds = luaL_checkinteger(L, 1);

	cpu_sleep(seconds);
	
	return 0;
}

static int os_reset_reason(lua_State *L) {
	lua_pushinteger(L, cpu_reset_reason());
	return 1;
}

static int os_loglevel(lua_State *L) {   
    int total = lua_gettop(L);
    int mask = 0;
    int flag = 0;
    int i;
    
    for(i=1;i<=total;i++) {
        flag = luaL_checkinteger(L, i);
        if (((flag < 0) || (flag > 7)) && (flag != 0xff)) {
            return luaL_error(L, "invalid flag");        
        }
        
        if (flag == 0xff) {
            mask |= LOG_UPTO(LOG_DEBUG);          
        } else {
            mask |= LOG_UPTO(flag);
        }
    }
    
    setlogmask(mask);

    return 0;    
}

static int more(const char *path, int stop) {
    FILE *fp;
    int rows = 0;
    int cols = 0;
    int c;
    char ch;
    int lines;

    fp = fopen(path,"r");
    if (!fp) {
        return -1;
    }

    if (stop) {
        console_size(&rows, &cols);
        console_clear();

        rows--;
        lines = 0;
    }

    while((c = fgetc(fp)) != EOF) {
        if ((c == '\n') && (stop)) {
            lines++;
            if (lines == rows) {
                console_statusline(path,": any key for next page, q for exit");
                ch = getchar();
                if ((ch == 'q') || (ch == 'Q')) {
                    console_clearstatusline();
                    break;
                }

                lines = 0;
                console_clear();
            }
        }

        printf("%c",c);
    }

    fclose(fp);

    return 0;
}

static int os_cat(lua_State *L) {
    const char *path = luaL_checkstring(L, 1);
    int res;
    
    if ((res = more(path, 0)) < 0) {
        return luaL_fileresult(L, 0, path);
    }

    return 0;
}

static int os_more(lua_State *L) {
    const char *path = luaL_checkstring(L, 1);
    int res;
    
    if ((res = more(path, 1)) < 0) {
        return luaL_fileresult(L, 0, path);
    }

    return 0;
}

static int os_dmesg(lua_State *L) {
    int res;    
    const char *fname = NULL;

    if (mount_is_mounted("fat")) {
    	if (mount_is_mounted("spiffs")) {
    		fname = "/sd/log/messages.log";
    	} else {
    		fname = "/log/messages.log";
    	}
    }

    if ((res = more(fname, 1)) < 0) {
        return luaL_fileresult(L, 0, fname);
    }

    return 0;
}

static int os_cd (lua_State *L) {
    const char *path = luaL_optstring(L, 1, "/");
      
    if (chdir(path) < 0) {
        return luaL_fileresult(L, 0, path);
    }

    return 0;
}

static int os_pwd (lua_State *L) {
    char path[PATH_MAX + 1];
    
    if (getcwd(path, PATH_MAX)) {
        lua_pushstring(L, path);
        return 1;
    } else {
        return luaL_fileresult(L, 0, path);
    }
}

static int os_mkdir (lua_State *L) {
    const char *path = luaL_optstring(L, 1, NULL);
    char cpath[PATH_MAX + 1];
    
    // If path is not present get the current directory as path
    if (!path) {
        if (!getcwd(cpath, PATH_MAX)) {
            return luaL_fileresult(L, 0, cpath);
        }    
        
        path = cpath;
    }
    
    return luaL_fileresult(L, mkdir(path, 0) == 0, path);
}

static int os_ls (lua_State *L) {
    const char *path = luaL_optstring(L, 1, NULL);
    DIR *dir = NULL;
    struct dirent *ent;
    char type;
    char size[9];
    char cpath[PATH_MAX];
    char tpath[PATH_MAX];
    char tbuffer[250];
    struct stat sb;
    struct tm *tm_info;

    // If path is not present get the current directory as path
    if (!path) {
        if (!getcwd(cpath, PATH_MAX)) {
            return luaL_fileresult(L, 0, cpath);
        }    
        
        path = cpath;
    }

    // Open directory
    dir = opendir(path);
    if (!dir) {
        return luaL_fileresult(L, 0, path);
    }

    //tty_lock();
    
    // Read entries
    while ((ent = readdir(dir)) != NULL) {
        strcpy(tpath, path);
        strcat(tpath,"/");
        strcat(tpath,ent->d_name);
        tbuffer[0] = '\0';

        if (strcmp((const char *)mount_device(tpath),"fat") == 0) {
        	if (stat(tpath, &sb) == 0) {
				tm_info = localtime(&sb.st_atime);
				strftime(tbuffer, 250, "%c", tm_info);
            }
        }

        type = 'd';
        if (ent->d_type == DT_REG) {
            type = 'f';
            sprintf(size,"%8d", ent->d_fsize);
        //} else if (ent->d_type == DT_LNK) {
        //    type = 'l';
        //    sprintf(size,"%8d", ent->d_fsize);
        } else {
            strcpy(size, "       -");
        }
        
        printf("%c\t%s\t%s\t%s\n",
            type,
            size,
            tbuffer,
            ent->d_name
        );
    }

    //tty_unlock();
    
    closedir(dir);

    return 0;    
}
 
static int os_clear (lua_State *L) {
    console_clear();
    
    return 0;
}

static int os_version(lua_State *L) {
   lua_pushstring(L, "Lua RTOS");
   lua_pushstring(L, LUA_OS_VER);   
   lua_pushinteger(L, BUILD_TIME);
   lua_pushstring(L, BUILD_COMMIT);
   
   return 4;
}

static int os_cpu(lua_State *L) {
	int revission;
	
    char model[18];
    char cpuInfo[26];
    
    cpu_model(model);

	revission = cpu_revission();
	if (revission) {
		sprintf(cpuInfo, "%s rev A%d", model, cpu_revission());
	} else {
		sprintf(cpuInfo, "%s", model);
	}
    
    lua_pushstring(L, cpuInfo);
    
    return 1;
}

static int os_board(lua_State *L) {
    lua_pushstring(L, LUA_RTOS_BOARD);

    return 1;
}

static int os_logcons(lua_State *L) { 
    int mask = LOG_NDELAY;
    
    luaL_checktype(L, 1, LUA_TBOOLEAN);
    int cons = lua_toboolean( L, 1 );

    if (cons) {
        mask = mask | LOG_CONS;
    }
    
    closelog();
    openlog(__progname, mask , LOG_LOCAL1);

    return 0;
}

static int os_stats(lua_State *L) {
    const char *stat = luaL_optstring(L, 1, NULL);

	// Do a garbage collection
	lua_lock(L);
	luaC_fullgc(L, 1);
	lua_unlock(L);
	
    if (stat && strcmp(stat,"mem") == 0) {
        lua_pushinteger(L, xPortGetFreeHeapSize());
        return 1;
    } else {
        printf("Free mem: %d\n",xPortGetFreeHeapSize());        
    }
    
    return 0;
}

static int os_format(lua_State *L) {
    const char *device = luaL_checkstring(L, 1);
    char response = ' ';

    // This if check is for future use, to determine drive number
    if (strcmp(device,"spiffs") == 0) {
    } else if (strcmp(device,"sd") == 0) {
    } else {
        return luaL_error(L, "device %s not exists for format", device); 
    }
    
    if (strcmp(device,"sd") == 0) {
        return luaL_error(L, "device %s not allowed for format", device);
    }

    // Check for mounted device
    if (!mount_is_mounted(device)) {
         return luaL_error(L, "device %s is not mounted", device);            
    }
    
    // Confirmation
    while ((response != 'y') && (response != 'Y') && (response != 'n') && (response != 'N')) {
        printf("\r");
        console_erase_l();
        printf("All data in %s will be deleted. Continue? [y/n]: ", device);
        response = fgetc(stdin);
    }
    
    printf("\n");
    
    if ((response == 'y') || (response == 'Y')) {
        if (strcmp(device,"spiffs") == 0) {
        	printf("Formatting...\r\n");
        	vfs_spiffs_format();
        }
    } else {
        printf("Format cancelled\n");
    }

    return 0;
}

static int os_lua_running(lua_State *L) { 
    lua_pushboolean(L, status_get(STATUS_LUA_RUNNING));

    return 1;
}

static int os_lua_interpreter(lua_State *L) { 
    lua_pushboolean(L, status_get(STATUS_LUA_INTERPRETER));

    return 1;
}

static int os_history(lua_State *L) { 
    luaL_checktype(L, 1, LUA_TBOOLEAN);
    
	if (lua_toboolean( L, 1 )) {
		status_set(STATUS_LUA_HISTORY);
	} else {
		status_clear(STATUS_LUA_HISTORY);
	}
    
    return 0;
}

static int os_cp(lua_State *L) {
    const char *src = luaL_optstring(L, 1, NULL);
    const char *dst = luaL_optstring(L, 2, NULL);
    FILE *fsrc, *fdst;
    char c;
    int res1, res2;

    fsrc = fopen(src,"r");
    if (!fsrc) {
        return luaL_fileresult(L, 0, src);
    }

    fdst = fopen(dst,"w");
    if (!fdst) {
        fclose(fsrc);
        return luaL_fileresult(L, 0, dst);
    }
    
    c = fgetc(fsrc);
    while (!feof(fsrc)) {
        fputc(c, fdst);
        c = fgetc(fsrc);    
    }
    
    res1 = fclose(fsrc);
    res2 = fclose(fdst);
    
    if (res1 != 0) {
        return luaL_fileresult(L, 0, src);
    }

    if (res2 != 0) {
        return luaL_fileresult(L, 0, dst);
    }
    
    lua_pushboolean(L, 1);
    return 1;
}

static int os_run (lua_State *L) {
    const char *argCode = luaL_optstring(L, 1, "");
    unsigned int i;
    int done;
	int status;
	int from_uart = 0;

    char *code = NULL;
    int code_size = 0;
    
    char *cchunk;
    char chunk_size;

    if (*argCode) {
        code = (char *)argCode;
        lua_pop(L, 1);

        
        goto skip;
    }
    
    from_uart = 1;
    code = (char *)calloc(1,1024);

    lua_pop(L, 1);

    // Lock tty, avoid other threads to write to console
    uart_ll_lock(CONSOLE_UART);

    // Clear received buffer
    uart_consume(CONSOLE_UART);

    // Send 'C' for start
    uart_write(CONSOLE_UART, 'C');
    uart_write(CONSOLE_UART, '\n');

    done = 0;

    for(;;) {
        // Wait for chunk size
        if (!uart_read(CONSOLE_UART, &chunk_size, 1000)) {
            break;
        }

        // More chunks?
        if (chunk_size == 0) {
            done = 1;
            break;
        }
        
        code = realloc(code, code_size + chunk_size + 1);

        // Read chunk
        cchunk = code + code_size;
        for(i=0; i < chunk_size; i++) {
            if (!uart_read(CONSOLE_UART, cchunk++, 1000)) {
                break;
            }
        }
        
        *cchunk = 0x00;
        
        code_size = code_size + chunk_size;

        // Send 'C' for start
        uart_write(CONSOLE_UART, 'C');
        uart_write(CONSOLE_UART, '\n');
    }
        
    if (!done) {
        // Unlock tty, permit other threads to write to console
    	uart_unlock(CONSOLE_UART);

        free(code);

        return luaL_error(L, "timeout");
    }
       
	uart_unlock(CONSOLE_UART);

skip:
	// Call load
    lua_getglobal(L, "load"); 
    lua_pushstring(L, (const char *)code);

    status = lua_pcall(L, 1, 2, 0);
    if (status != LUA_OK) {
        if (code && from_uart) free(code);
        return luaL_error(L, lua_tostring(L, -1));
    }

    lua_remove(L, -1);
    
    lua_getglobal(L, "thread"); 
    lua_getfield(L, -1, "start");

    lua_remove(L, -2);   // Remove "thread" from the stack
    lua_insert(L, -2);   // Switch "thread" with parsed function
    
    status = lua_pcall(L, 1, 1, 0);
    if (status != LUA_OK) {
        if (code && from_uart) free(code);
        return luaL_error(L, lua_tostring(L, -1));
    }

    lua_pop(L, 1);

    if (code && from_uart) free(code);

    return 0;
}

static int os_bootcount(lua_State *L) {
	lua_pushinteger(L, boot_count);

	return 1;
}

static int os_flash_unique_id(lua_State *L) {
	#if CONFIG_LUA_RTOS_READ_FLASH_UNIQUE_ID
	char buffer[17];

	sprintf(buffer,
			"%02x%02x%02x%02x%02x%02x%02x%02x",
			flash_unique_id[0], flash_unique_id[1],
			flash_unique_id[2], flash_unique_id[3],
			flash_unique_id[4], flash_unique_id[5],
			flash_unique_id[6], flash_unique_id[7]
	);

    lua_pushstring(L, buffer);
	#else
    lua_pushnil(L);
	#endif
	return 1;
}

static int os_exists(lua_State *L) {
	const char *fname;
	struct stat sb;
	size_t len;

	fname = luaL_checklstring( L, 1, &len );
	if (stat(fname, &sb) != 0) {
		lua_pushboolean(L, false);
	}
	else lua_pushboolean(L, true);

	return 1;
}
