/*
 * Copyright (c) 1992, 1993
 *      The Regents of the University of California.  All rights reserved.
 * Copyright (c) 2014 Serge Vakulenko
 *
 * This code is derived from software contributed to Berkeley by
 * Digital Equipment Corporation and Ralph Campbell.
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
 * Copyright (C) 1989 Digital Equipment Corporation.
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appears in all copies.
 * Digital Equipment Corporation makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * from: $Header: /sprite/src/kernel/mach/ds3100.md/RCS/loMem.s,
 *      v 1.1 89/07/11 17:55:04 nelson Exp $ SPRITE (DECWRL)
 * from: $Header: /sprite/src/kernel/mach/ds3100.md/RCS/machAsm.s,
 *      v 9.2 90/01/29 18:00:39 shirriff Exp $ SPRITE (DECWRL)
 * from: $Header: /sprite/src/kernel/vm/ds3100.md/vmPmaxAsm.s,
 *      v 1.1 89/07/10 14:27:41 nelson Exp $ SPRITE (DECWRL)
 *
 *      @(#)locore.s    8.7 (Berkeley) 6/2/95
 */

/*
 *      Contains code that is the first executed at boot time plus
 *      assembly language support routines.
 */

#include "FreeRTOSConfig.h"
#include <asm.h>
#include "ISR_Support.h"
 
#include <syscall.h>

#include <machine/param.h>
#include <machine/psl.h>
#include <machine/reg.h>
#include <machine/machAsmDefs.h>
#include <machine/pte.h>
#include <machine/assym.h>

#define _INTSTAT (0xBF800000+(0x10020))

        .set    noreorder               # Don't allow the assembler to reorder instructions.
        .set    noat
			
/*-----------------------------------
 * Reset/NMI exception handler.
 */
        .globl  start
        .type   start, @function
        //
        // CPU initialization code copied from MIPS MD00901 Application Note
        // "Boot-MIPS: Example Boot Code for MIPS Cores"
        // http://www.imgtec.com/downloads/app-notes/MD00901-2B-CPS-APP-01.03.zip
        //
start:
        mfc0    k0, _CP0_STATUS
        ext     k0, k0, 0x13, 0x1
        beqz    k0, _no_nmi
        nop

_nmi:
        mfc0    k0, _CP0_STATUS                   # retrieve STATUS
        lui     k1, ~(_CP0_STATUS_BEV_MASK >> 16) & 0xffff
        ori     k1, k1, ~_CP0_STATUS_BEV_MASK & 0xffff
        and     k0, k0, k1                        # Clear BEV
        mtc0    k0, _CP0_STATUS                   # store STATUS
        eret

_no_nmi:
        mtc0    zero, MACH_C0_Count     # Clear cp0 Count (Used to measure boot time.)

        //
        // Set all GPRs of all register sets to predefined state.
        //
init_gpr:
        li      $1, 0xdeadbeef          # 0xdeadbeef stands out, kseg2 mapped, odd.

        # Determine how many shadow sets are implemented (in addition to the base register set.)
        # the first time thru the loop it will initialize using $1 set above.
        # At the bottom og the loop, 1 is  subtract from $30
        # and loop back to next_shadow_set to start the next loop and the next lowest set number.
        mfc0    $29, MACH_C0_SRSCtl     # read SRSCtl
        ext     $30, $29, 26, 4         # extract HSS

next_shadow_set:                        # set PSS to shadow set to be initialized
        ins     $29, $30, 6, 4          # insert PSS
        mtc0    $29, MACH_C0_SRSCtl     # write SRSCtl

        wrpgpr  $1, $1
        wrpgpr  $2, $1
        wrpgpr  $3, $1
        wrpgpr  $4, $1
        wrpgpr  $5, $1
        wrpgpr  $6, $1
        wrpgpr  $7, $1
        wrpgpr  $8, $1
        wrpgpr  $9, $1
        wrpgpr  $10, $1
        wrpgpr  $11, $1
        wrpgpr  $12, $1
        wrpgpr  $13, $1
        wrpgpr  $14, $1
        wrpgpr  $15, $1
        wrpgpr  $16, $1
        wrpgpr  $17, $1
        wrpgpr  $18, $1
        wrpgpr  $19, $1
        wrpgpr  $20, $1
        wrpgpr  $21, $1
        wrpgpr  $22, $1
        wrpgpr  $23, $1
        wrpgpr  $24, $1
        wrpgpr  $25, $1
        wrpgpr  $26, $1
        wrpgpr  $27, $1
        wrpgpr  $28, $1
        beqz    $30, init_cp0
        wrpgpr  $29, $1

        wrpgpr  $30, $1
        wrpgpr  $31, $1
        b       next_shadow_set
        add     $30, -1                 # Decrement to the next lower number

        //
        // Init CP0 Status, Count, Compare, Watch*, and Cause.
        //
init_cp0:	
        # Initialize Status
        li      v1, MACH_Status_BEV | MACH_Status_ERL
        mtc0    v1, MACH_C0_Status      # write Status

        # Initialize Watch registers if implemented.
        mfc0    v0, MACH_C0_Config1     # read Config1
        ext     v1, v0, 3, 1            # extract bit 3 WR (Watch registers implemented)
        beq     v1, zero, done_wr
        li      v1, 0x7                 # (M_WatchHiI | M_WatchHiR | M_WatchHiW)

        # Clear Watch Status bits and disable watch exceptions
        mtc0    v1, MACH_C0_WatchHi     # write WatchHi0
        mfc0    v0, MACH_C0_WatchHi     # read WatchHi0
        bgez    v0, done_wr             # Check for bit 31 (sign bit) for more Watch registers
        mtc0    zero, MACH_C0_WatchLo   # clear WatchLo0

        mtc0    v1, MACH_C0_WatchHi, 1  # write WatchHi1
        mfc0    v0, MACH_C0_WatchHi, 1  # read WatchHi1
        bgez    v0, done_wr             # Check for bit 31 (sign bit) for more Watch registers
        mtc0    zero, MACH_C0_WatchLo,1 # clear WatchLo1

        mtc0    v1, MACH_C0_WatchHi, 2  # write WatchHi2
        mfc0    v0, MACH_C0_WatchHi, 2  # read WatchHi2
        bgez    v0, done_wr             # Check for bit 31 (sign bit) for more Watch registers
        mtc0    zero, MACH_C0_WatchLo,2 # clear WatchLo2

        mtc0    v1, MACH_C0_WatchHi, 3  # write WatchHi3
        mfc0    v0, MACH_C0_WatchHi, 3  # read WatchHi3
        bgez    v0, done_wr             # Check for bit 31 (sign bit) for more Watch registers
        mtc0    zero, MACH_C0_WatchLo,3 # clear WatchLo3

        mtc0    v1, MACH_C0_WatchHi, 4  # write WatchHi4
        mfc0    v0, MACH_C0_WatchHi, 4  # read WatchHi4
        bgez    v0, done_wr             # Check for bit 31 (sign bit) for more Watch registers
        mtc0    zero, MACH_C0_WatchLo,4 # clear WatchLo4

        mtc0    v1, MACH_C0_WatchHi, 5  # write WatchHi5
        mfc0    v0, MACH_C0_WatchHi, 5  # read WatchHi5
        bgez    v0, done_wr             # Check for bit 31 (sign bit) for more Watch registers
        mtc0    zero, MACH_C0_WatchLo,5 # clear WatchLo5

        mtc0    v1, MACH_C0_WatchHi, 6  # write WatchHi6
        mfc0    v0, MACH_C0_WatchHi, 6  # read WatchHi6
        bgez    v0, done_wr             # Check for bit 31 (sign bit) for more Watch registers
        mtc0    zero, MACH_C0_WatchLo,6 # clear WatchLo6

        mtc0    v1, MACH_C0_WatchHi, 7  # write WatchHi7
        mtc0    zero, MACH_C0_WatchLo,7 # clear WatchLo7

done_wr:
        # Clear WP bit to avoid watch exception upon user code entry, IV, and software interrupts.
        mtc0    zero, MACH_C0_Cause     # clear Cause: init AFTER init of WatchHi/Lo registers.

        # Clear timer interrupt. (Count was cleared at the reset vector to allow timing boot.)
        mtc0    zero, MACH_C0_Compare   # clear Compare

/*-----------------------------------
 * Initialization.
 */
        //
        // Clear TLB: generate unique EntryHi contents per entry pair.
        //
init_tlb:
        # Determine if we have a TLB
        mfc0    v1, MACH_C0_Config      # read Config
        ext     v1, v1, 7, 3            # extract MT field
        li      a3, 0x1                 # load a 1 to check against
        bne     v1, a3, init_icache

        # Config1MMUSize == Number of TLB entries - 1
        mfc0    v0, MACH_C0_Config1     # Config1
        ext     v1, v0, 25, 6           # extract MMU Size
        mtc0    zero, MACH_C0_EntryLo0  # clear EntryLo0
        mtc0    zero, MACH_C0_EntryLo1  # clear EntryLo1
        mtc0    zero, MACH_C0_PageMask  # clear PageMask
        mtc0    zero, MACH_C0_Wired     # clear Wired
        li      a0, 0x80000000

next_tlb_entry:
        mtc0    v1, MACH_C0_Index       # write Index
        mtc0    a0, MACH_C0_EntryHi     # write EntryHi
        ehb
        tlbwi
        add     a0, 2<<13               # Add 8K to the address to avoid TLB conflict with previous entry

        bne     v1, zero, next_tlb_entry
        add     v1, -1


        //
        // Clear L1 instruction cache.
        //
init_icache:
        # Determine how big the I-cache is
        mfc0    v0, MACH_C0_Config1     # read Config1
        ext     v1, v0, 19, 3           # extract I-cache line size
        beq     v1, zero, done_icache   # Skip ahead if no I-cache
        nop

        mfc0    s1, MACH_C0_Config7     # Read Config7
        ext     s1, s1, 18, 1           # extract HCI
        bnez    s1, done_icache         # Skip when Hardware Cache Initialization bit set

        li      a2, 2
        sllv    v1, a2, v1              # Now have true I-cache line size in bytes

        ext     a0, v0, 22, 3           # extract IS
        li      a2, 64
        sllv    a0, a2, a0              # I-cache sets per way

        ext     a1, v0, 16, 3           # extract I-cache Assoc - 1
        add     a1, 1
        mul     a0, a0, a1              # Total number of sets
        lui     a2, 0x8000              # Get a KSeg0 address for cacheops

        mtc0    zero, MACH_C0_ITagLo    # Clear ITagLo register
        move    a3, a0

next_icache_tag:
        # Index Store Tag Cache Op
        # Will invalidate the tag entry, clear the lock bit, and clear the LRF bit
        cache   0x8, 0(a2)              # ICIndexStTag
        add     a3, -1                  # Decrement set counter
        bne     a3, zero, next_icache_tag
        add     a2, v1                  # Get next line address
done_icache:

        //
        // Enable cacheability of kseg0 segment.
        // Need to switch to kseg1, modify kseg0 CCA, then switch back.
        //
        la      a2, enable_k0_cache
        li      a1, 0xf
        ins     a2, a1, 29, 1           # changed to KSEG1 address by setting bit 29
        jr      a2
        nop

enable_k0_cache:
        # Set CCA for kseg0 to cacheable.
        # NOTE! This code must be executed in KSEG1 (not KSEG0 uncached)
        mfc0    v0, MACH_C0_Config      # read Config
        li      v1, 3                   # CCA for single-core processors
        ins     v0, v1, 0, 3            # insert K0
        mtc0    v0, MACH_C0_Config      # write Config

        la      a2, init_dcache
        jr      a2                      # switch back to KSEG0
        ehb

        //
        // Initialize the L1 data cache
        //
init_dcache:
        mfc0    v0, MACH_C0_Config1     # read Config1
        ext     v1, v0, 10, 3           # extract D-cache line size
        beq     v1, zero, done_dcache   # Skip ahead if no D-cache
        nop

        mfc0    s1, MACH_C0_Config7     # Read Config7
        ext     s1, s1, 18, 1           # extract HCI
        bnez    s1, done_dcache         # Skip when Hardware Cache Initialization bit set

        li      a2, 2
        sllv    v1, a2, v1              # Now have true D-cache line size in bytes

        ext     a0, v0, 13, 3           # extract DS
        li      a2, 64
        sllv    a0, a2, a0              # D-cache sets per way

        ext     a1, v0, 7, 3            # extract D-cache Assoc - 1
        add     a1, 1
        mul     a0, a0, a1              # Get total number of sets
        lui     a2, 0x8000              # Get a KSeg0 address for cacheops

        mtc0    zero, MACH_C0_ITagLo    # Clear ITagLo/DTagLo registers
        mtc0    zero, MACH_C0_DTagLo
        move    a3, a0

next_dcache_tag:
        # Index Store Tag Cache Op
        # Will invalidate the tag entry, clear the lock bit, and clear the LRF bit
        cache   0x9, 0(a2)              # DCIndexStTag
        add     a3, -1                  # Decrement set counter
        bne     a3, zero, next_dcache_tag
        add     a2, v1                  # Get next line address
		
done_dcache:

/*
 * Amount to take off of the stack for the benefit of the debugger.
 */
#define START_FRAME     ((4 * 4) + 4 + 4)

        .set    at

        di                                      # Disable interrupts
		
        la      sp, _eram - START_FRAME
        la      gp, _gp
        sw      zero, START_FRAME - 4(sp)       # Zero out old ra for debugger
        jal     mach_init                       # mach_init()
        sw      zero, START_FRAME - 8(sp)       # Zero out old fp for debugger
init_unix:
        la      sp, _eram - START_FRAME         # switch to standard stack
        jal     main                            # main(frame)
        move    a0, zero
		
/*
 * GCC2 seems to want to call __main in main() for some reason.
 */
LEAF(__main)
        j       ra
        nop
END(__main)

/*-----------------------------------
 * Exception handlers and data for bootloader.
 */
        .section .exception
        .set    noat
/*
 * TLB exception vector: handle TLB translation misses.
 * The BaddVAddr, Context, and EntryHi registers contain the failed
 * virtual address.
 */
        .org    0
        .globl  _tlb_vector
_tlb_vector:
        .type   _tlb_vector, @function
        j 		_tlb_handler
 		nop
		
#         mfc0    k0, MACH_C0_Status              # Get the status register
#         and     k0, MACH_Status_UM              # test for user mode
#
# kern_tlb_refill:
#         mfc0    k0, MACH_C0_BadVAddr            # get the fault address
#         li      k1, 0xc0000000                  # VM_MIN_KERNEL_ADDRESS
#         subu    k0, k1                          # compute index
#         srl     k0, PGSHIFT
#         lw      k1, Sysmapsize                  # index within range?
#         sltu    k1, k0, k1
#         beqz    k1, check_stack                 # No. check for valid stack
#         andi    k1, k0, 1                       # check for odd page
#         bnez    k1, odd_page
#         sll     k0, 2                           # compute offset from index
# even_page:
#         lw      k1, Sysmap
#         addu    k1, k0
#         lw      k0, 0(k1)                       # get even page PTE
#         lw      k1, 4(k1)                       # get odd page PTE
#         mtc0    k0, MACH_C0_EntryLo0            # save PTE entry
#         and     k0, PG_V                        # check for valid entry
#         beqz    k0, kern_exception              # PTE invalid
#         mtc0    k1, MACH_C0_EntryLo1
#         tlbwr                                   # update TLB
#         eret
# odd_page:
#         lw      k1, Sysmap
#         addu    k1, k0
#         lw      k0, -4(k1)                      # get even page PTE
#         lw      k1, 0(k1)                       # get odd page PTE
#         mtc0    k1, MACH_C0_EntryLo1
#         and     k1, PG_V                        # check for valid entry
#         beqz    k1, kern_exception              # PTE invalid
#         mtc0    k0, MACH_C0_EntryLo0            # save PTE entry
#         tlbwr                                   # update TLB
#         eret

/*
 * Data for bootloader.
 */
        .org    0xf8
        .type   _ebase, @object
_ebase:
        .word   _tlb_vector                     # EBase value

        .type   _imgptr, @object
_imgptr:
        .word   -1                              # Image header pointer

/*
 * General exception vector address:
 * handle all execptions except RESET and TLBMiss.
 * Find out what mode we came from and jump to the proper handler.
 */
        .org    0x180
_exception_vector:
        .type   _exception_vector, @function

		/*
		 * Call the exception handler.
 	  	 */
                j   _exception_handler
		nop
		
/*
 * General interrupt vector address.
 * Find out what mode we came from and jump to the proper handler.
 */
        .org    0x200
_interrupt_vector:
        .type   _interrupt_vector, @function

		 /*
        	  * Get IRQ number
		  */
		  li   k1, _INTSTAT
		  lw   k0, 0(k1)
		  ext  k0, k0, 0, 8

		 /*
            	  * Test for software interrupt
		  */
		  li   k1, PIC32_IRQ_CS0
		  bne  k0, k1, 1f
		  nop
		  j    vPortYieldISR
                  nop

1:	
		 /*
		  * Hardware interrupt
		  */
		  portSAVE_CONTEXT

		  jal interrupt
		  nop
	
		  portRESTORE_CONTEXT

_exception_handler:
	portSAVE_CONTEXT
	
        mfc0	a0, MACH_C0_Status
        mfc0	a1, MACH_C0_Cause
        mfc0	a2, MACH_C0_EPC
	jal     exception
	nop
	portRESTORE_CONTEXT
	
_tlb_handler:
	portSAVE_CONTEXT

	portRESTORE_CONTEXT
