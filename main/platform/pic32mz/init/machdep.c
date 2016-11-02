/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1992, 1993
 *      The Regents of the University of California.  All rights reserved.
 * Copyright (c) 2014 Serge Vakulenko
 * Copyright (C) 2015 - 2016
 * IBEROXARXA SERVICIOS INTEGRALES, S.L. & CSS IBÉRICA, S.L.
 * 
 * All rights reserved.  
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department, The Mach Operating System project at
 * Carnegie-Mellon University and Ralph Campbell.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *      @(#)machdep.c   8.5 (Berkeley) 6/2/95
 */

/* from: Utah $Hdr: machdep.c 1.63 91/04/24$ */

#include "FreeRTOS.h"
#include "task.h"

#include "whitecat.h"

#include <machine/machConst.h>
#include <machine/pic32mz.h>
#include <machine/assym.h>
#include <sys/types.h>
#include <machine/param.h>
#include <string.h>

#include <sys/drivers/resource.h>

extern void  mach_dev(void);
extern void _clock_init();
extern void _syscalls_init();
extern void _pthread_init();
extern void _console_init();
extern void _lora_init();
extern void _signal_init();
extern void _mtx_init();
extern void _cpu_init();
extern void _resource_init();

void vApplicationSetupTickTimerInterrupt(void) {
    unsigned int pr;
    unsigned int preescaler;
    unsigned int preescaler_bits;

    // Disable timer
    T1CON = 0;
    
    // Computes most lower preescaler for current frequency and period value
    preescaler_bits = 0;
    for(preescaler=1;preescaler <= 256;preescaler = preescaler * 2) {
        if ((preescaler != 2) && (preescaler != 4) && (preescaler !=16) && (preescaler != 32) && (preescaler != 128)) {
            pr = ( (configPERIPHERAL_CLOCK_HZ / preescaler) / configTICK_RATE_HZ ) - 1;
            if (pr <= 0xffff) {
                break;
            }
            
            preescaler_bits++;
        }
    }
            
    // Configure timer
    T1CON = (preescaler_bits << 4);
    PR1 = pr;   
    
    IPCCLR(PIC32_IRQ_T1 >> 2) = 0xf << (8 * (PIC32_IRQ_T1 & 0x03));
    IPCSET(PIC32_IRQ_T1 >> 2) = (configKERNEL_INTERRUPT_PRIORITY << 2) << (8 * (PIC32_IRQ_T1 & 0x03));

    /* Clear the interrupt as a starting condition. */
    IFSCLR(PIC32_IRQ_T1 >> 5) = 1 << (PIC32_IRQ_T1 & 31);

    /* Enable the interrupt. */
    IECSET(PIC32_IRQ_T1 >> 5) = 1 << (PIC32_IRQ_T1 & 31);    

    /* Start the timer. */
    T1CONSET = (1 << 15);	
}

void conf_performance() {
    SYSKEY = 0;	
    SYSKEY = UNLOCK_KEY_0;
    SYSKEY = UNLOCK_KEY_1;

    PB2DIV = (PB2DIV & ~0b1111111) | PBCLK2_DIV;
    PB3DIV = (PB3DIV & ~0b1111111) | PBCLK3_DIV;
    PB4DIV = (PB4DIV & ~0b1111111) | PBCLK4_DIV;
    PB5DIV = (PB5DIV & ~0b1111111) | PBCLK5_DIV;
    
    CFGCON |= (1 << 7 );  // Input Compare modules use an alternative 
                          // Timer pair as their timebase clock

    CFGCON |= (1 << 16);  // Output Compare modules use an alternative 
                          // Timer pair as their timebase clock
    
    SYSKEY = 0;			
}

/*
 * Do all the stuff that locore normally does before calling main().
 */
void mach_init() {
    extern void _ebase_address();
    extern char __data_start[], _edata[], _end[];
    extern void _etext(), _tlb_vector();
    caddr_t v;
	
    // The WDT caused a wake from sleep?
    if (RCON & 0x18) {
        asm volatile("eret");
    }
    
    // Set base address for vector exceptions
    // Modifications to EBase and VS values are only allowed when the BEV bit
    // is set to 1
    mtc0_Status(MACH_Status_CU0 | MACH_Status_BEV);
    mtc0_EBase(_tlb_vector);	
    mtc0_Status(MACH_Status_CU0);

    mtc0_IntCtl(32);            // Set vector spacing
    mtc0_Cause(MACH_Cause_IV);  // Use special interrupt vector 0x200

    /* Copy .data image from flash to RAM.
     * Linker places it at the end of .text segment. */
    bcopy(_etext, __data_start, _edata - __data_start);

    /* Clear .bss segment. */
    v = (caddr_t)_end;
    bzero(_edata, v - _edata);

#if ( __mips_hard_float == 1 )
    // Enable FPU (CU1), set FPU in 64-bit mode (FR), enable DSP (MX)
    mtc0_Status(MACH_Status_CU1 | MACH_Status_MX | MACH_Status_FR);

    // Flush-to-Zero bit (FS)
    mtc1_Fcsr(MACH_Fcsr_FS);
#else
    // Enable DSP (MX)
    mtc0_Status(MACH_Status_FR);
#endif
    
    /* Setup interrupt controller */
    INTCON = 0;			// Reset controller
    IPTMR = 0;			// Temporal proximity timer
		
    /*
     * Enable instruction prefetch.
     */
    PRECON = 2;                 /* Two wait states. */
    PRECONSET = 0x30;           /* Enable predictive prefetch for any address */
    
    conf_performance();

    /*
     * Disable all periperiphals, except TIMERS
     */
    PMD1 = 0xffffffff;
    PMD2 = 0xffffffff;
    PMD3 = 0xffffffff;
    
    PMD3 = 0xffffffff;
    PMD3CLR = T1MD;
    
    PMD5 = 0xffffffff;
    PMD6 = 0xffffffff;

#if USE_RTC    
    PMD6CLR = RTCCMD;
#endif
    
    PMD7 = 0xffffffff;
	
    mips_ei();
}