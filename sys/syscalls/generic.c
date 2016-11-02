#include "syscalls.h"

extern const struct filedesc *p_fd;

// This function normalize a path passed as an argument, doing the
// following normalization actions:
//
// 1) If path is a relative path, preapend the current working directory
//
//    example:
//
//		if path = "blink.lua", and current working directory is "/examples", path to
//      normalize is "/examples/blink.lua"
//
// 2) Process ".." (parent directory) elements in path
//
//    example:
//
//		if path = "/examples/../autorun.lua" normalized path is "/autorun.lua"
//
// 3) Process "." (current directory) elements in path
//
//    example:
//
//		if path = "/./autorun.lua" normalized path is "/autorun.lua"
char *normalize_path(const char *path) {
    char *rpath;
    char *cpath;
    char *tpath;
    char *last;
    int maybe_is_dot = 0;
    int maybe_is_dot_dot = 0;
    int is_dot = 0; 
    int is_dot_dot = 0;
    int plen = 0;
    
    rpath = malloc(PATH_MAX);
    if (!rpath) {
        errno = ENOMEM;
        return NULL;
    }
    
    // If it's a relative path preappend current working directory
    if (*path != '/') {
        if (!getcwd(rpath, PATH_MAX)) {
            free(rpath);
            return NULL;
        }
         
        if (*(rpath + strlen(rpath) - 1) != '/') {
            rpath = strcat(rpath, "/");  
        }
        
        rpath = strcat(rpath, path);        
    } else {
        strcpy(rpath, path);
    }
    
    plen = strlen(rpath);
    if (*(rpath + plen - 1) != '/') {
        rpath = strcat(rpath, "/");  
        plen++;
    }
    
    cpath = rpath;
    while (*cpath) {
        if (*cpath == '.') {
            if (maybe_is_dot) {
                maybe_is_dot_dot = 1;
                maybe_is_dot = 0;
            } else {
                maybe_is_dot = 1;
            }
        } else {
            if (*cpath == '/') {
                is_dot_dot = maybe_is_dot_dot;
                is_dot = maybe_is_dot && !is_dot_dot;
            } else {
                maybe_is_dot_dot = 0;
                maybe_is_dot = 0;
            }
        }

        if (is_dot_dot) {
            last = cpath + 1;
            
            while (*--cpath != '/');
            while (*--cpath != '/');
            
            tpath = ++cpath;
            while (*last) {
                *tpath++ = *last++;
            }
            *tpath = '\0';
            
            is_dot_dot = 0;
            is_dot = 0;
            maybe_is_dot = 0;
            maybe_is_dot_dot = 0; 
            continue;
        }        

        if (is_dot) {
            last = cpath + 1;
            
            while (*--cpath != '/');
            
            tpath = ++cpath;
            while (*last) {
                *tpath++ = *last++;
            }
            *tpath = '\0';
            
            is_dot_dot = 0;
            is_dot = 0;
            maybe_is_dot = 0;
            maybe_is_dot_dot = 0; 
            continue;
        }        
        
        cpath++;           
    }
    
    cpath--;
    if ((cpath != rpath) && (*cpath == '/')) {
        *cpath = '\0';
    }
    
    return rpath;
}