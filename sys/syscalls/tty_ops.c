/*
 * Lua RTOS, TTY file system operations
 * 
 * Copyright (C) 2015 - 2016
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

#include <limits.h>

#include <string.h>
#include <ctype.h>
#include <unistd.h> 
#include <sys/dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/filedesc.h>
#include <sys/uio.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/syslog.h>

#include <pthread.h>

#include <sys/drivers/uart.h>

static pthread_mutex_t tty_mutex = PTHREAD_MUTEX_INITIALIZER;                

#if USE_DISPLAY
static int redirect_to_display;

int tty_redirect_to_display() {
    return redirect_to_display;
}

void tty_to_display(int to_display) {
    redirect_to_display = to_display;
}
#endif

int tty_open(struct file *fp, int flags) {
	#if USE_DISPLAY
    redirect_to_display = 0;
	#endif
    return 0;
}

int tty_close(struct file *fp) {
    return 0;
}
       
int tty_read(struct file *fp, struct uio *uio) {
    int unit = fp->f_devunit;
    char *buf = uio->uio_iov->iov_base;

    while (uio->uio_iov->iov_len) {
        if (uart_read(unit, buf++, portMAX_DELAY)) {
            uio->uio_iov->iov_len--;
            uio->uio_resid--;
        } else {
            break;
        } 
    }

    return 0;
}

int tty_write(struct file *fp, struct uio *uio) {
    int unit = fp->f_devunit;
    char *buf = uio->uio_iov->iov_base;
	
#if USE_DISPLAY	
    char dbuf[2];    
    dbuf[1] = '\0';
#endif

    if (tty_mutex != PTHREAD_MUTEX_INITIALIZER) {
        pthread_mutexattr_t attr;

        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

        pthread_mutex_init(&tty_mutex, &attr);
    }

    pthread_mutex_lock(&tty_mutex);

    while (uio->uio_iov->iov_len) {
        if (*buf == '\n') {
            uart_write(unit, '\r');            
        }
        
        uart_write(unit, *buf);   
      
		#if USE_DISPLAY
        if (redirect_to_display) {
			dbuf[0] = *buf;
			display_text(-1,-1, NULL, -1, -1, dbuf);
		}
		#endif
		
        uio->uio_iov->iov_len--;
        uio->uio_resid--;
        buf++;
    }

    pthread_mutex_unlock(&tty_mutex);
    
    return 0;
}

int tty_stat(struct file *fp, struct stat *sb) {
    sb->st_mode = S_IFCHR;
    sb->st_blksize = 1;
    sb->st_size = 1;

    return 0;
}

void tty_lock() {
    if (tty_mutex != PTHREAD_MUTEX_INITIALIZER) {
        pthread_mutexattr_t attr;

        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

        pthread_mutex_init(&tty_mutex, &attr);
    }

    pthread_mutex_lock(&tty_mutex);
}

void tty_unlock() {
    if (tty_mutex != PTHREAD_MUTEX_INITIALIZER) {
        pthread_mutexattr_t attr;

        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

        pthread_mutex_init(&tty_mutex, &attr);
    }

    pthread_mutex_unlock(&tty_mutex);
}
