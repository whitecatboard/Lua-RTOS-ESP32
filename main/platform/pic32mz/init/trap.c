/*
 * Lua RTOS, interrupt traps
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

#include <stdio.h>

#include <machine/pic32mz.h>
#include <sys/drivers/gpio.h>
#include <sys/drivers/uart.h>
#include <sys/drivers/lora.h>

extern void vPortIncrementTick();
extern char stepper_timer;
extern char lmic_timer;

typedef enum {
        EXCEP_IRQ = 0, 	/* interrupt */
        EXCEP_AdEL = 4, /* address error exception (load or ifetch) */
        EXCEP_AdES, 	/* address error exception (store) */
        EXCEP_IBE, 	/* bus error (ifetch) */
        EXCEP_DBE, 	/* bus error (load/store) */
        EXCEP_Sys, 	/* syscall */
        EXCEP_Bp, 	/* breakpoint */
        EXCEP_RI, 	/* reserved instruction */
        EXCEP_CpU, 	/* coprocessor unusable */
        EXCEP_Overflow,	/* arithmetic overflow */
        EXCEP_Trap, 	/* trap (possible divide by zero) */
        EXCEP_IS1 = 16,	/* implementation specfic 1 */
        EXCEP_CEU, 	/* CorExtend Unuseable */
        EXCEP_C2E 	/* coprocessor 2 */
} exception_code;
    
unsigned exception(statusReg, causeReg, vadr, pc, args)
    unsigned statusReg;     /* status register at time of the exception */
    unsigned causeReg;      /* cause register at time of exception */
    unsigned vadr;          /* address (if any) the fault occured on */
    unsigned pc;            /* program counter where to continue */
	unsigned args;  
{
    // Get exception code
    exception_code exception;
    
    exception = (causeReg & 0x0000007C) >> 2;

    // Write exception information

    printf("rtos: exception, code = %d, adress = 0x%08x. ", exception, vadr);

    switch (exception) {
        case EXCEP_AdEL:     printf("Address error exception - load or instruction fetch");break;
        case EXCEP_AdES:     printf("Address error exception - store");break;
        case EXCEP_IBE:      printf("Bus error exception - instruction fetch");break;
        case EXCEP_DBE:      printf("Bus error exception - data reference - load or store");break;
        case EXCEP_Sys:      printf("Sycall exception");break;
        case EXCEP_Bp:       printf("Breakpoint exception");break;
        case EXCEP_RI:       printf("Reserved instruction exception");break;
        case EXCEP_CpU:      printf("Coprocessor unusable exception");break;
        case EXCEP_Overflow: printf("Arithmetic owerflow exception");break;
        case EXCEP_Trap:     printf("Trap exception");break;
        default: ;
    }
    
    printf(".\r\n");
    
    for(;;) {}
}

void interrupt() {
    // Get the current irq numbers
    int intstat = INTSTAT;
    int irq = PIC32_INTSTAT_VEC(intstat);
	
    // Call to related interrupt handler
    switch (irq) {
        case PIC32_IRQ_T1:
            vPortIncrementTick();break;	

        case PIC32_IRQ_T2:  
            #if USE_STEPPER
            if (stepper_timer == 2) {
                stepper_intr(PIC32_IRQ_T2);
            }
            #endif
			
			#if USE_LMIC
			if (lmic_timer == 2) {
			    lmic_intr(PIC32_IRQ_T2);
			}
			#endif
			break;
                
        case PIC32_IRQ_T3:    
            #if USE_STEPPER
            if (stepper_timer == 3) {
                stepper_intr(PIC32_IRQ_T3);
            }
            #endif

			#if USE_LMIC
			if (lmic_timer == 3) {
			    lmic_intr(PIC32_IRQ_T3);
			}
			#endif
            break;

        case PIC32_IRQ_T4: 
            #if USE_STEPPER
            if (stepper_timer == 4) {
                stepper_intr(PIC32_IRQ_T4);
            }
            #endif
            break;

        case PIC32_IRQ_T5:    
            #if USE_STEPPER
            if (stepper_timer == 5) {
                stepper_intr(PIC32_IRQ_T5);
            }
            #endif

			#if USE_LMIC
			if (lmic_timer == 5) {
			    lmic_intr(PIC32_IRQ_T5);
			}
			#endif
            break;

        case PIC32_IRQ_T6:    
            #if USE_STEPPER
            if (stepper_timer == 6) {
                stepper_intr(PIC32_IRQ_T6);
            }
            #endif

			#if USE_LMIC
			if (lmic_timer == 6) {
			    lmic_intr(PIC32_IRQ_T6);
			}
			#endif
            break;

        case PIC32_IRQ_T7:    
            #if USE_STEPPER
            if (stepper_timer == 7) {
                stepper_intr(PIC32_IRQ_T7);
            }
            #endif

			#if USE_LMIC
			if (lmic_timer == 7) {
			    lmic_intr(PIC32_IRQ_T7);
			}
			#endif
            break;

        case PIC32_IRQ_T8:    
            #if USE_STEPPER
            if (stepper_timer == 8) {
                stepper_intr(PIC32_IRQ_T8);
            }
            #endif

			#if USE_LMIC
			if (lmic_timer == 8) {
			    lmic_intr(PIC32_IRQ_T8);
			}
			#endif
            break;

        case PIC32_IRQ_T9:    
            #if USE_STEPPER
            if (stepper_timer == 9) {
                stepper_intr(PIC32_IRQ_T9);
            }
            #endif

			#if USE_LMIC
			if (lmic_timer == 9) {
			    lmic_intr(PIC32_IRQ_T9);
			}
			#endif
            break;

        /* UART */
        case PIC32_IRQ_U1RX:  uart_intr_rx(0); break;
        case PIC32_IRQ_U2RX:  uart_intr_rx(1); break;
        case PIC32_IRQ_U3RX:  uart_intr_rx(2); break;
        case PIC32_IRQ_U4RX:  uart_intr_rx(3); break;
        case PIC32_IRQ_U5RX:  uart_intr_rx(4); break;
        case PIC32_IRQ_U6RX:  uart_intr_rx(5); break;
        case PIC32_IRQ_U1TX:  uart_intr_tx(0); break;
        case PIC32_IRQ_U2TX:  uart_intr_tx(1); break;
        case PIC32_IRQ_U3TX:  uart_intr_tx(2); break;
        case PIC32_IRQ_U4TX:  uart_intr_tx(3); break;
        case PIC32_IRQ_U5TX:  uart_intr_tx(4); break;
        case PIC32_IRQ_U6TX:  uart_intr_tx(5); break;
        case PIC32_IRQ_U1E:   uart_intr_er(0); break;
        case PIC32_IRQ_U2E:   uart_intr_er(1); break;
        case PIC32_IRQ_U3E:   uart_intr_er(2); break;
        case PIC32_IRQ_U4E:   uart_intr_er(3); break;
        case PIC32_IRQ_U5E:   uart_intr_er(4); break;
        case PIC32_IRQ_U6E:   uart_intr_er(5); break;

#if USE_CAN
        /* CAN */
        case PIC32_IRQ_CAN1:  can_intr(0); break;
        case PIC32_IRQ_CAN2:  can_intr(1); break;
#endif

       // case PIC32_IRQ_DMA0:  dma_intr(0); break;
       // case PIC32_IRQ_DMA1:  dma_intr(1); break;
       // case PIC32_IRQ_DMA2:  dma_intr(2); break;
       // case PIC32_IRQ_DMA3:  dma_intr(3); break;
       // case PIC32_IRQ_DMA4:  dma_intr(4); break;
       // case PIC32_IRQ_DMA5:  dma_intr(5); break;
       // case PIC32_IRQ_DMA6:  dma_intr(6); break;
       // case PIC32_IRQ_DMA7:  dma_intr(7); break;

#if USE_SPI_PHY
        /* ETHERNET */		
        case SPI_PHY_INT:  ether_intr(); break;
#endif
        
#if USE_RTC
        case PIC32_IRQ_RTCC:  rtc_intr(); break;
#endif
        
    }	 
}
