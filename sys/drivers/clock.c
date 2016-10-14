/*
 * Whitecat, clock driver
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

#include "whitecat.h"
#include "build.h"

#include <time.h>
#include <sys/drivers/gpio.h>
#include <sys/drivers/rtc.h>

#include <sys/time.h>
#include <sys/mutex.h>
/*
#define TM_YEAR_BASE    1900
#define EPOCH_YEAR      1970
#define	DAYS_PER_LYEAR	366
#define	DAYS_PER_NYEAR	365
#define SECS_PER_MIN    60
#define MINS_PER_HOUR   60
#define HOURS_PER_DAY   24
*/

// Tells if year is a leap year
//#define	isleap(y) ((((y) % 4) == 0 && ((y) % 100) != 0) || ((y) % 400) == 0)

// Days of month
//static int  dmsize[] =
//    { -1, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

struct mtx clock_mtx;

static volatile uint64_t tticks = 0;    // Number of ticks (1 tick = configTICK_RATE_HZ)
static volatile uint64_t tdelta = 0;    // Delta tincs from last 1000 ms
static volatile uint32_t tseconds = 0;  // Seconds counter (from boot)
static volatile unsigned long long tuseconds = 0; // Seconds counter (from boot)

#if LED_ACT
unsigned int activity = 0;
#endif

void _clock_init(void) {
    mtx_init(&clock_mtx, NULL, NULL, 0);  
    
    tseconds = BUILD_TIME;
}

void newTick(void) {
    mtx_lock(&clock_mtx);
    
    // Increment internal high tick counter
    // This is every 1 ms
    tticks++;

    // Increment usecs
    tuseconds = tuseconds + configTICK_RATE_HZ;

    // Increment delta ticks
    tdelta++;
    if (tdelta == configTICK_RATE_HZ) {
        // 1 second since last second
        tdelta = 0;
        tuseconds = 0;

        tseconds++;
        
		#if LED_ACT
        	if (activity <= 0) {
            	activity = 0;
            	gpio_pin_inv(LED_ACT);
        	}
		#endif
    }

    mtx_unlock(&clock_mtx);
}

// Sets current EPOCH time, based on epoch time value
void set_time_s(u32_t secs) {
    mtx_lock(&clock_mtx);
    tseconds = secs;

#if USE_RTC
    rtc_init(secs);
#endif
    
    mtx_unlock(&clock_mtx);
}


int _gettimeofday_r(struct timeval *tv , struct timezone *tz) {
    UNUSED_ARG(tz);

    mtx_lock(&clock_mtx);
    tv->tv_sec = (uint32_t)(tseconds);
    tv->tv_usec = tuseconds;
    mtx_unlock(&clock_mtx);
    
    return 0;
 }

 long long ticks() {
     return tticks;
 }

time_t time(time_t *t) {
    mtx_lock(&clock_mtx);

    time_t current = (tseconds);
    
    if (t) {
        *t = current;
    }

    mtx_unlock(&clock_mtx);

    return (current);
}

// Sets current EPOCH time, based on complete date - time
/*
void set_time_ymdhms(u16_t year, u8_t month, u8_t day, u8_t hours, u8_t minutes, u8_t seconds) {
    uint32_t secs = 0;

    secs = 0;
    year += TM_YEAR_BASE;

    // If year < EPOCH_YEAR, assume it's in the next century and
   // the system has not yet been patched to move TM_YEAR_BASE up yet
    if (year < EPOCH_YEAR)
        year += 100;

    if (isleap(year) && month > 2)
        ++secs;

    for (--year;year >= EPOCH_YEAR;--year)
        secs += isleap(year) ? DAYS_PER_LYEAR : DAYS_PER_NYEAR;

    while (--month)
        secs += dmsize[month];

    secs += day - 1;
    secs = HOURS_PER_DAY * secs + hours;
    secs = MINS_PER_HOUR * secs + minutes;
    secs = SECS_PER_MIN * secs + seconds;

    mtx_lock(&clock_mtx);
    tseconds = secs;

#if USE_RTC
    rtc_init(secs);
#endif

    mtx_unlock(&clock_mtx);
}
*/