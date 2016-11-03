/*
 * Lua RTOS, DMA driver
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
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

#include "sys/drivers/dma.h"
#include <sys/kmem.h>
#include <stdio.h> 

struct dma_mod_reg {
    volatile unsigned con;
    volatile unsigned conclr;
    volatile unsigned conset;
    volatile unsigned coninv;
    volatile unsigned stat;
    volatile unsigned statclr;
    volatile unsigned statset;
    volatile unsigned statinv;
    volatile unsigned addr;
    volatile unsigned addrclr;
    volatile unsigned addrset;
    volatile unsigned addrinv;
 };
 
 struct dma_chan_reg {
    volatile unsigned con;
    volatile unsigned conclr;
    volatile unsigned conset;
    volatile unsigned coninv;
    volatile unsigned econ;
    volatile unsigned econclr;
    volatile unsigned econset;
    volatile unsigned econinv;
    volatile unsigned in;
    volatile unsigned inclr;
    volatile unsigned inset;
    volatile unsigned ininv;
    volatile unsigned ssa;
    volatile unsigned ssaclr;
    volatile unsigned ssaset;
    volatile unsigned ssainv;
    volatile unsigned dsa;
    volatile unsigned dsaclr;
    volatile unsigned dsaset;
    volatile unsigned dsainv;
    volatile unsigned ssiz;
    volatile unsigned ssizclr;
    volatile unsigned ssizset;
    volatile unsigned ssizinv;
    volatile unsigned dsiz;
    volatile unsigned dsizclr;
    volatile unsigned dsizset;
    volatile unsigned dsizinv;
    volatile unsigned sptr;
    volatile unsigned sptrclr;
    volatile unsigned sptrset;
    volatile unsigned sptrinv;
    volatile unsigned dptr;
    volatile unsigned dptrclr;
    volatile unsigned dptrset;
    volatile unsigned dptrinv;
    volatile unsigned csiz;
    volatile unsigned csizclr;
    volatile unsigned csizset;
    volatile unsigned csizinv;
    volatile unsigned cptr;
    volatile unsigned cptrclr;
    volatile unsigned cptrset;
    volatile unsigned cptrinv;
    volatile unsigned dat;
    volatile unsigned datclr;
    volatile unsigned datset;
    volatile unsigned datinv;
 };

 struct dma {
    unsigned char        used;
    struct dma_mod_reg  *mreg;
    struct dma_chan_reg *creg;
    void (*callback)(void *, uint32_t);
    void *cba1;
    unsigned long cba2;
};

struct dma dma[NDMA] = {
    {0, (struct dma_mod_reg*) &DMACON, (struct dma_chan_reg*) &DCH0CON, NULL},
    {0, (struct dma_mod_reg*) &DMACON, (struct dma_chan_reg*) &DCH1CON, NULL},
    {0, (struct dma_mod_reg*) &DMACON, (struct dma_chan_reg*) &DCH2CON, NULL},
    {0, (struct dma_mod_reg*) &DMACON, (struct dma_chan_reg*) &DCH3CON, NULL},
    {0, (struct dma_mod_reg*) &DMACON, (struct dma_chan_reg*) &DCH4CON, NULL},
    {0, (struct dma_mod_reg*) &DMACON, (struct dma_chan_reg*) &DCH5CON, NULL},
    {0, (struct dma_mod_reg*) &DMACON, (struct dma_chan_reg*) &DCH6CON, NULL},
    {0, (struct dma_mod_reg*) &DMACON, (struct dma_chan_reg*) &DCH7CON, NULL},
};

int dma_first_free() {
    int i;

    for(i=0;i < NDMA;i++) {
        if (!dma[i].used) {
            break;
        }
    }
    
    if (i < NDMA) {
        dma[i].used = 1;
    } else {
        i = -1;
    }

    return i;    
}

void dma_open(int channel, int src_irq, void *src, void *dest, 
            unsigned int src_size, unsigned int dest_size, 
            unsigned int cell_size, 
            void (*callback)(void * pvParameter1, uint32_t ulParameter2),
            void *cba1, unsigned long cba2
) {
    struct dma_mod_reg *mreg;
    struct dma_chan_reg *creg;
    int irqn;
    
    irqn = PIC32_IRQ_DMA0 + channel;
    
    mreg = dma[channel].mreg;
    creg = dma[channel].creg;

    dma[channel].callback = callback;
    dma[channel].cba1 = cba1;
    dma[channel].cba2 = cba2;
    
    mreg->conset = (1 << 15);             // Enable DMA
    
    creg->conclr = (1 << 15);             // Disable DMA channel

    IECCLR(irqn >> 5) = 1 << (irqn & 31); // Disable interrupts
    IFSCLR(irqn >> 5) = 1 << (irqn & 31); // Clear interrupt flag
        
    creg->con   = (1 << 4) |              // Channel Automatic Enable
                   0x01;                  // Channel priority 1

    creg->econ = (src_irq << 8) |         // IRQn will initiate DMA transfer
                 (1 << 4);                // Start channel cell transfer if an
                                          // interrupt matching

    creg->inclr = 0x0000ffff;             // Clear existing events, disable all 
                                          // interrupts
    
    creg->inset  =  (1 << 19);            // Channel Block Transfer Complete

    creg->dsa  = KVA_TO_PA(dest);         // Destination address
    creg->dsiz = dest_size;               // Size of destination
    creg->ssa  = KVA_TO_PA(src);          // Source address
    creg->ssiz = src_size;                // Size of source
    creg->csiz = cell_size;               // Cell size

    // Clear IRQ priority and sub priority
    IPCCLR(irqn >> 2) = 0x1f << (8 * (irqn & 0x03));
    
    // Set IRQ priority = 2, sub priority = 0
    IPCSET(irqn >> 2) = (0x1f << (8 * (irqn & 0x03))) & 0x08080808;

    IECSET(irqn >> 5) = 1 << (irqn & 31); // Enable interrupts
    
    creg->conset = (1 << 7);              // Enable DMA channel
}

void dma_close(int channel) {
    struct dma_mod_reg *mreg;

    mreg = dma[channel].mreg;

    mreg->conclr = (1 << 15);             // Disable DMA module

    if (dma[channel].used) {
        dma[channel].used = 0;
    }
}

void dma_intr(int channel) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    
    struct dma_mod_reg *mreg;
    struct dma_chan_reg *creg;    
    int irqn;
    int flags;
    
    irqn = PIC32_IRQ_DMA0 + channel;

    mreg = dma[channel].mreg;
    creg = dma[channel].creg;
    
    // Read DAM interrupt flags
    flags = creg->in & 0xff;
    if (flags & (1 << 3)) {
        // Channel Block Transfer Complete
        
        if (xTimerPendFunctionCallFromISR(
                dma[channel].callback, dma[channel].cba1,dma[channel].cba2,
                &xHigherPriorityTaskWoken) == pdFALSE
            ) {
            printf("Can't defer execution of dma transfer callback \n");
        }
    }
    
    // Clear DMA interrupt flags
    creg->inclr = 0xff;
    
    // Clear the interrupt flag
    IFSCLR(irqn >> 5) = 1 << (irqn & 31);   
    
    portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}
