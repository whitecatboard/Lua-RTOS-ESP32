/*
 * RTC driver for pic32.
 *
 * Copyright (C) 2014 Serge Vakulenko, <serge@vak.ru>
 * 
 * 
 * Lua RTOS, RTC driver
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

#if USE_RTC

#include <time.h>
#include <sys/syslog.h>
#include <machine/pic32mz.h>
#include <machine/machConst.h>

#define RTC_BCD_CODE(a,b,c, d) \
    d = ((((a / 10) << 4) | (a % 10)) << 24) | \
        ((((b / 10) << 4) | (b % 10)) << 16) | \
        ((((c / 10) << 4) | (c % 10)) << 8);
              
#define RTC_BCD_DECODE(a, b, c, d) \
    a = (((d & (0xf0 << 24)) >> 28) * 10) + \
        (((d & (0x0f << 24)) >> 24)); \
    b = (((d & (0xf0 << 16)) >> 20) * 10) + \
        (((d & (0x0f << 16)) >> 16)); \
    c = (((d & (0xf0 <<  8)) >> 12) * 10) + \
        (((d & (0x0f <<  8)) >> 8));
        
static void rtc_set_time(struct tm *info) {
    int year =  info->tm_year - 100;
    int month = info->tm_mon + 1;
    
    // Set date
    RTC_BCD_CODE(year, month, info->tm_mday, RTCDATE);
            
    // Set time
    RTC_BCD_CODE(info->tm_hour, info->tm_min, info->tm_sec, RTCTIME);
}

static void rtc_set_alarm(struct tm *info) {
    int year =  info->tm_year - 100;
    int month = info->tm_mon + 1;
    
    // Set date
    RTC_BCD_CODE(year, month, info->tm_mday, ALRMDATE);
            
    // Set time
    RTC_BCD_CODE(info->tm_hour, info->tm_min, info->tm_sec, ALRMTIME);    
}

void rtc_alarm_at(time_t time) {
    struct tm *info;
    
    // Convert from epoch time to tm struct
    info = localtime(&time);

    // Disable RTCC interrupts
    IECCLR(PIC32_IRQ_RTCC >> 5) = 1 << (PIC32_IRQ_RTCC & 31); 
    //printf("iecclr %x\n", 1 << (PIC32_IRQ_RTCC & 31));
    
    // Clear RTCC flag
    IFSCLR(PIC32_IRQ_RTCC >> 5) = 1 << (PIC32_IRQ_RTCC & 31); 
    //printf("ifsclr %x\n", 1 << (PIC32_IRQ_RTCC & 31));
    
    // Clear the RTCC priority and sub-priority
    IPCCLR(PIC32_IRQ_RTCC >> 2) = 0x1f << (5 * (PIC32_IRQ_RTCC & 0x03));
    //printf("ipcclr %x\n",  0x1f << (5 * (PIC32_IRQ_RTCC & 0x03)));

    // Set RTCC IPL 3, sub-priority 1
    IPCSET(PIC32_IRQ_RTCC >> 2) = (0x1f << (8 * (PIC32_IRQ_RTCC & 0x03))) & 0x0d0d0d0d;
    //printf("ipcclr %x\n",  (0x1f << (8 * (PIC32_IRQ_RTCC & 0x03))) & 0x0d0d0d0d);

    // Lock sequence
    SYSKEY = 0;			
    SYSKEY = UNLOCK_KEY_0;
    SYSKEY = UNLOCK_KEY_1;

    // Enable write
    RTCCONSET = (1 << 3);

    // Alarm off
    // Chime is disabled
    // PIV is read-only and returns the state of the Alarm Pulse
    // Alarm mask to once per day
    // Alarm will trigger one time
    RTCALRM = (1 << 13) |
              (0b0110 << 8);
    
    // Wait for ALRMSYNC to be 0
    while(RTCALRM & (1 << 12));
    
    rtc_set_alarm(info);
    
    // Enable alarm
    RTCALRMSET = (1 << 15);

    // Disable write
    RTCCONSET = (1 << 3);

    // Unlock
    SYSKEY = 0;	
    
    // Enable RTCC interrupts
    IECSET(PIC32_IRQ_RTCC >> 5) = 1 << (PIC32_IRQ_RTCC & 31); 
    //printf("iecset %x\n",  1 << (PIC32_IRQ_RTCC & 31));
}

void rtc_init(time_t time) {
    struct tm *info;
    
    // Convert from epoch time to tm struct
    info = localtime(&time);

    // Lock sequence
    SYSKEY = 0;			
    SYSKEY = UNLOCK_KEY_0;
    SYSKEY = UNLOCK_KEY_1;

    // RTCC module is disabled
    // Continue normal operation when CPU enters Idle mode
    // RTCC uses the internal 32 kHz oscillator (LPRC)
    // Real-time clock value registers can be read without concern about a rollover ripple
    // First half period of a second
    // RTCC output is not enabled
    // Disable write
    RTCCON = 0;

    // Enable write
    RTCCONSET = (1 << 3);

    rtc_set_time(info);

    // Turn-on RTC
    RTCCONSET = (1 << 15);

    // Disable write
    RTCCONCLR = (1 << 3);
    
    // Unlock
    SYSKEY = 0;	

    // Wait for RTC on
    while(!(RTCCON & 0x40));
}

void rtc_update_clock() {
    struct tm info;
    unsigned int timev;
    unsigned int datev;
    
    // Wait for read window
    while (RTCCON & (1 << 2));
    
    timev = RTCTIME;
    datev = RTCDATE;
    
    RTC_BCD_DECODE(info.tm_year, info.tm_mon, info.tm_mday, datev);
    RTC_BCD_DECODE(info.tm_hour, info.tm_min, info.tm_sec, timev);

    info.tm_year = info.tm_year + 100;
    info.tm_mon = info.tm_mon - 1;
                
    info.tm_isdst = -1;
    
    set_time_s(mktime(&info));
}

void rtc_intr(void) {
    // Clear RTCC flag
    IFSCLR(PIC32_IRQ_RTCC >> 5) = 1 << (PIC32_IRQ_RTCC & 31);     
}

#endif
