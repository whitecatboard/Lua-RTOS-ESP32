/*
 * Lua RTOS, main program
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

#include <pthread.h>

#include "whitecat.h"
#include "FreeRTOS.h"
#include "task.h"
#include "portable.h"
#include "timers.h"

#include <stdio.h>
#include <fcntl.h>
#include <setjmp.h>
#include <stdlib.h>

#include <sys/drivers/gpio.h>
#include <sys/drivers/uart.h>
#include <sys/drivers/clock.h>
#include <sys/syslog.h>
#include <sys/panic.h>
#include <sys/delay.h>
#include <sys/debug.h>

// Configuration bits
PIC32_DEVCFG (
    DEVCFG0_DEBUG_OFF               |    
    DEVCFG0_JTAGEN_OFF              |
    DEVCFG0_ICESEL_ICS_PGx2         |
    DEVCFG0_TRCEN_OFF               | 
    DEVCFG0_BOOTISA_MIPS32          |
    DEVCFG0_FECCCON_OFF_UNLOCKED    |
    DEVCFG0_FSLEEP_VREGS            |
    DEVCFG0_DBGPER_PG_ALL           | 
    DEVCFG0_SMCLR_MCLR_NORM         | 
    DEVCFG0_SOSCGAIN_GAIN_2X        | 
    DEVCFG0_SOSCBOOST_OFF           | 
    DEVCFG0_POSCGAIN_GAIN_2X        | 
    DEVCFG0_POSCBOOST_OFF           | 
    DEVCFG0_EJTAGBEN_NORMAL,

    DEVCFG1_FNOSC_SPLL              |  
    DEVCFG1_DMTINTV_WIN_127_128     |        
    DEVCFG1_FSOSCEN_OFF             |
    DEVCFG1_IESO_OFF                |
    DEVCFG1_POSCMOD_EC              |
    DEVCFG1_OSCIOFNC_OFF            |
    DEVCFG1_FCKSM_CSDCMD            |
    DEVCFG1_WDTPS_PS2048         |	
    DEVCFG1_WDTSPGM_STOP            |
    DEVCFG1_WINDIS_NORMAL           |        
    DEVCFG1_FWDTEN_OFF              |
    DEVCFG1_FWDTWINSZ_WINSZ_25      |
    DEVCFG1_DMTCNT_DMT8             |
    DEVCFG1_FDMTEN_OFF,
        
    DEVCFG2_FPLLIDIV_DIV_3          |
    DEVCFG2_FPLLRNG_RANGE_5_10_MHZ  | 
    DEVCFG2_FPLLICLK_PLL_POSC       |
    DEVCFG2_FPLLMULT_MUL_50         |
    DEVCFG2_FPLLODIV_DIV_2          |
    DEVCFG2_UPLLFSEL_FREQ_24MHZ,
		
    DEVCFG3_FMIIEN_OFF              |
    DEVCFG3_FETHIO_ON               |
    DEVCFG3_PGL1WAY_OFF             |
    DEVCFG3_PMDL1WAY_OFF            |		
    DEVCFG3_IOL1WAY_OFF             |		
    DEVCFG3_FUSBIDIO_OFF);

// Boot code
asm ("          .section .startup,\"ax\",@progbits");
asm ("          .globl  _boot");
asm ("          .type   _boot, function");
asm ("_boot:    la      $ra, start");
asm ("          jr      $ra");
asm ("          .text");


const char *__progname = "Lua RTOS";

void app_main();

int main(void) {   
	app_main();
	
    // Start scheluder
    vTaskStartScheduler();
	
	for(;;);
        
    return 0;
}

void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName ) {
    taskDISABLE_INTERRUPTS();
    
    uart_writes(CONSOLE_UART,"rtos: stack overflow, task '");
    uart_writes(CONSOLE_UART,pcTaskName);
    uart_writes(CONSOLE_UART,"'\r\n");

    for( ;; );
}

void vApplicationTickHook( void ) {
    __newTick();
}

void vAssertCalled( const char * pcFile, unsigned long ulLine ) {
    volatile char *pcFileName;
    volatile unsigned long ulLineNumber;

    /* Prevent things that are useful to view in the debugger from being
    optimised away. */
    pcFileName = ( char * ) pcFile;
    ( void ) pcFileName;
    ulLineNumber = ulLine;

    printf("assert: at %s, line %lu\n", pcFileName, ulLine);

    /* Set ulLineNumber to 0 in the debugger to break out of this loop and
    return to the line that triggered the assert. */
    while( ulLineNumber != 0 ) {
	    __asm volatile( "NOP" );
	    __asm volatile( "NOP" );
	    __asm volatile( "NOP" );
	    __asm volatile( "NOP" );
	    __asm volatile( "NOP" );
    }
}
