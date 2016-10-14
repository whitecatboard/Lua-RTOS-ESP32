/*
 * Copyright (c) 1992, 1993
 *      The Regents of the University of California.  All rights reserved.
 * Copyright (c) 2014 Serge Vakulenko
 *
 * This code is derived from software contributed to Berkeley by
 * Ralph Campbell and Rick Macklem.
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
 *      @(#)machConst.h 8.1 (Berkeley) 6/10/93
 *
 * machConst.h --
 *
 *      Machine dependent constants.
 *
 *      Copyright (C) 1989 Digital Equipment Corporation.
 *      Permission to use, copy, modify, and distribute this software and
 *      its documentation for any purpose and without fee is hereby granted,
 *      provided that the above copyright notice appears in all copies.
 *      Digital Equipment Corporation makes no representations about the
 *      suitability of this software for any purpose.  It is provided "as is"
 *      without express or implied warranty.
 *
 * from: $Header: /sprite/src/kernel/mach/ds3100.md/RCS/machConst.h,
 *      v 9.2 89/10/21 15:55:22 jhh Exp $ SPRITE (DECWRL)
 * from: $Header: /sprite/src/kernel/mach/ds3100.md/RCS/machAddrs.h,
 *      v 1.2 89/08/15 18:28:21 rab Exp $ SPRITE (DECWRL)
 * from: $Header: /sprite/src/kernel/vm/ds3100.md/RCS/vmPmaxConst.h,
 *      v 9.1 89/09/18 17:33:00 shirriff Exp $ SPRITE (DECWRL)
 */

#ifndef _MACHCONST
#define _MACHCONST

#define MACH_CACHED_MEMORY_ADDR         0x80000000
#define MACH_UNCACHED_MEMORY_ADDR       0xa0000000
#define MACH_KSEG2_ADDR                 0xc0000000

#define MACH_VIRT_TO_PHYS(x)     ((unsigned)(x) & 0x1fffffff)
#define MACH_PHYS_TO_CACHED(x)   ((unsigned)(x) | MACH_CACHED_MEMORY_ADDR)
#define MACH_PHYS_TO_UNCACHED(x) ((unsigned)(x) | MACH_UNCACHED_MEMORY_ADDR)

/*--------------------------------------
 * Coprocessor 0 registers.
 */
#define mfc0_Index()        mips_mfc0(0,0)     /* Index into the TLB array */
#define mtc0_Index(v)       mips_mtc0(0,0,v)

#define mfc0_Random()       mips_mfc0(1,0)     /* Randomly generated index into the TLB array */

#define mfc0_EntryLo0()     mips_mfc0(2,0)     /* Low-order portion of the TLB entry for */
#define mtc0_EntryLo0(v)    mips_mtc0(2,0,v)   /* even-numbered virtual pages */

#define mfc0_EntryLo1()     mips_mfc0(3,0)     /* Low-order portion of the TLB entry for */
#define mtc0_EntryLo1(v)    mips_mtc0(3,0,v)   /* odd-numbered virtual pages */

#define C0_CONTEXT          4,0     /* Pointer to the page table entry in memory */
#define C0_USERLOCAL        4,2     /* User information that can be written by
                                     * privileged software and read via the RDHWR instruction */
#define C0_PAGEMASK         5,0     /* Variable page sizes in TLB entries */
#define C0_PAGEGRAIN        5,1     /* Support of 1 KB pages in the TLB */

#define mtc0_Wired(v)       mips_mtc0(6,0,v)    /* Controls the number of fixed TLB entries */

#define C0_HWRENA           7,0     /* Enables access via the RDHWR instruction
                                     * to selected hardware registers in Non-privileged mode */
#define C0_BADVADDR         8,0     /* Reports the address for the most recent
                                     * address-related exception */
#define mfc0_Count()        mips_mfc0(9,0)      /* Processor cycle count */
#define mtc0_Count(v)       mips_mtc0(9,0,v)

#define mfc0_EntryHi()      mips_mfc0(10,0)     /* High-order portion of the TLB entry */
#define mtc0_EntryHi(v)     mips_mtc0(10,0,v)

#define mfc0_Compare()      mips_mfc0(11,0)     /* Core timer interrupt control */
#define mtc0_Compare(v)     mips_mtc0(11,0,v)

#define mfc0_Status()       mips_mfc0(12,0)     /* Processor status and control */
#define mtc0_Status(v)      mips_mtc0(12,0,v)

#define mfc0_IntCtl()       mips_mfc0(12,1)     /* Interrupt control of vector spacing */
#define mtc0_IntCtl(v)      mips_mtc0(12,1,v)

// WHITECAT ADDITION BEGIN
#define MACH_Fcsr_FS		(1 << 24)

#define mfc0_SRSCtl(v)		mips_mfc0(12,2)
#define mfc0_SRSMap(v)		mips_mfc0(12,3)
#define mfc0_SRSMap2(v)		mips_mfc0(12,5)

#define mfc0_Debug(v)		mips_mfc0(23,0)

#define mfc1_Fcsr()         mips_mfc0(31,0)     /* Floating Point Control and Status Register */
#define mtc1_Fcsr(v)        mips_mtc0(31,0,v)
// WHITECAT ADDITION END

#define C0_SRSCTL           12,2    /* Shadow register set control */
#define C0_SRSMAP           12,3    /* Shadow register mapping control */
#define C0_VIEW_IPL         12,4    /* Allows the Priority Level to be read/written
                                     * without extracting or inserting that bit from/to the Status register */
#define C0_SRSMAP2          12,5    /* Contains two 4-bit fields that provide
                                     * the mapping from a vector number to
                                     * the shadow set number to use when servicing such an interrupt */

#define mfc0_Cause()        mips_mfc0(13,0)     /* Describes the cause of the last exception */
#define mtc0_Cause(v)       mips_mtc0(13,0,v)

#define C0_NESTEDEXC        13,1    /* Contains the error and exception level
                                     * status bit values that existed prior to the current exception */
#define C0_VIEW_RIPL        13,2    /* Enables read access to the RIPL bit that
                                     * is available in the Cause register */
#define C0_EPC              14,0    /* Program counter at last exception */
#define C0_NESTEDEPC        14,1    /* Contains the exception program counter
                                     * that existed prior to the current exception */

#define mfc0_PRId()         mips_mfc0(15,0)     /* Processor identification and revision */

#define mfc0_EBase()        mips_mfc0(15,1)     /* Exception base address of exception vectors */
#define mtc0_EBase(v)       mips_mtc0(15,1,v)

#define C0_CDMMBASE         15,2    /* Common device memory map base */

#define mfc0_Config()       mips_mfc0(16,0)     /* Configuration register */
#define mtc0_Config(v)      mips_mtc0(16,0,v)

#define mfc0_Config1()      mips_mfc0(16,1)     /* Configuration register 1 */

#define C0_CONFIG2          16,2    /* Configuration register 2 */
#define C0_CONFIG3          16,3    /* Configuration register 3 */
#define C0_CONFIG4          16,4    /* Configuration register 4 */
#define C0_CONFIG5          16,5    /* Configuration register 5 */
#define C0_CONFIG7          16,7    /* Configuration register 7 */
#define C0_LLADDR           17,0    /* Load link address */
#define C0_WATCHLO          18,0    /* Low-order watchpoint address */
#define C0_WATCHHI          19,0    /* High-order watchpoint address */
#define C0_DEBUG            23,0    /* EJTAG debug register */
#define C0_TRACECONTROL     23,1    /* EJTAG trace control */
#define C0_TRACECONTROL2    23,2    /* EJTAG trace control 2 */
#define C0_USERTRACEDATA1   23,3    /* EJTAG user trace data 1 register */
#define C0_TRACEBPC         23,4    /* EJTAG trace breakpoint register */
#define C0_DEBUG2           23,5    /* Debug control/exception status 1 */
#define C0_DEPC             24,0    /* Program counter at last debug exception */
#define C0_USERTRACEDATA2   24,1    /* EJTAG user trace data 2 register */
#define C0_PERFCTL0         25,0    /* Performance counter 0 control */
#define C0_PERFCNT0         25,1    /* Performance counter 0 */
#define C0_PERFCTL1         25,2    /* Performance counter 1 control */
#define C0_PERFCNT1         25,3    /* Performance counter 1 */
#define C0_ERRCTL           26,0    /* Software test enable of way-select and data
                                     * RAM arrays for I-Cache and D-Cache */
#define C0_TAGLO            28,0    /* Low-order portion of cache tag interface */
#define C0_DATALO           28,1    /* Low-order portion of cache tag interface */
#define C0_ERROREPC         30,0    /* Program counter at last error exception */
#define C0_DESAVE           31,0    /* Debug exception save */

/*
 * The bits in the cause register.
 */
#define MACH_Cause_BD           0x80000000      /* Exception occured in delay slot */
#define MACH_Cause_TI           0x40000000      /* Timer interrupt is pending */
#define MACH_Cause_CE           0x30000000      /* Coprocessor exception */
#define MACH_Cause_DC           0x08000000      /* Disable COUNT register */
#define MACH_Cause_PCI          0x04000000      /* Performance counter interrupt */
#define MACH_Cause_IC           0x02000000      /* Interrupt changing since last IRET */
#define MACH_Cause_AP           0x01000000      /* Exception occured in automated prolog */
#define MACH_Cause_IV           0x00800000      /* Use special interrupt vector 0x200 */
#define MACH_Cause_WP           0x00400000      /* Watch exception pending */
#define MACH_Cause_FDCI         0x00200000      /* Fast debug channel interrupt */
#define MACH_Cause_RIPL(r)      ((r)>>10 & 63)  /* Requested interrupt priority level */
#define MACH_Cause_IP1          0x00000200      /* Request software interrupt 1 */
#define MACH_Cause_IP0          0x00000100      /* Request software interrupt 0 */
#define MACH_Cause_ExcCode      0x0000007c      /* Exception code */
#define MACH_Cause_ExcCode_SHIFT        2

/*
 * The bits in the status register.
 */
#define MACH_Status_CU1         0x20000000      /* Access to coprocessor 1 allowed (in user mode) */
#define MACH_Status_FR          0x04000000      /* Floating-point registers can contain any data type */
#define MACH_Status_CU0         0x10000000      /* Access to coprocessor 0 allowed (in user mode) */
#define MACH_Status_RP          0x08000000      /* Enable reduced power mode */
#define MACH_Status_RE          0x02000000      /* Reverse endianness (in user mode) */
#define MACH_Status_MX          0x01000000      /* DSP resource enable */
#define MACH_Status_BEV         0x00400000      /* Exception vectors: bootstrap */
#define MACH_Status_TS          0x00200000      /* TLB shutdown control */
#define MACH_Status_SR          0x00100000      /* Soft reset */
#define MACH_Status_NMI         0x00080000      /* NMI reset */
#define MACH_Status_IPL(x)      ((x) << 10)     /* Current interrupt priority level */
#define MACH_Status_IPL_MASK    0x0000fc00
#define MACH_Status_IPL_SHIFT   10
#define MACH_Status_UM          0x00000010      /* User mode */
#define MACH_Status_ERL         0x00000004      /* Error level */
#define MACH_Status_EXL         0x00000002      /* Exception level */
#define MACH_Status_IE          0x00000001      /* Interrupt enable */

/*
 * Coprocessor 0 registers.
 */
#define MACH_C0_Index           $0      /* TLB index */
#define MACH_C0_Random          $1      /* TLB random */
#define MACH_C0_EntryLo0        $2      /* TLB entry low 0 */
#define MACH_C0_EntryLo1        $3      /* TLB entry low 1 */
#define MACH_C0_Context         $4      /* TLB context */
#define MACH_C0_PageMask        $5
#define MACH_C0_Wired           $6
#define MACH_C0_BadVAddr        $8      /* Bad virtual address */
#define MACH_C0_Count           $9
#define MACH_C0_EntryHi         $10     /* TLB entry high */
#define MACH_C0_Compare         $11
#define MACH_C0_Status          $12     /* Status register */
#define MACH_C0_SRSCtl          $12,2
#define MACH_C0_Cause           $13     /* Exception cause register */
#define MACH_C0_EPC             $14     /* Exception PC */
#define MACH_C0_PRId            $15     /* Processor revision identifier */
#define MACH_C0_Config          $16
#define MACH_C0_Config1         $16,1
#define MACH_C0_Config7         $16,7
#define MACH_C0_WatchLo         $18
#define MACH_C0_WatchHi         $19
#define MACH_C0_ITagLo          $28
#define MACH_C0_DTagLo          $28,2
#define MACH_C0_ErrPC           $30

/*
 * Values for the code field in a break instruction.
 */
#define MACH_BREAK_INSTR        0x0000000d
#define MACH_BREAK_VAL_MASK     0x03ff0000
#define MACH_BREAK_VAL_SHIFT    16
#define MACH_BREAK_KDB_VAL      512
#define MACH_BREAK_SSTEP_VAL    513
#define MACH_BREAK_BRKPT_VAL    514
#define MACH_BREAK_KDB          (MACH_BREAK_INSTR | \
                                (MACH_BREAK_KDB_VAL << MACH_BREAK_VAL_SHIFT))
#define MACH_BREAK_SSTEP        (MACH_BREAK_INSTR | \
                                (MACH_BREAK_SSTEP_VAL << MACH_BREAK_VAL_SHIFT))
#define MACH_BREAK_BRKPT        (MACH_BREAK_INSTR | \
                                (MACH_BREAK_BRKPT_VAL << MACH_BREAK_VAL_SHIFT))

/*
 * The floating point version and status registers.
 */
#define MACH_FPC_ID     $0
#define MACH_FPC_CSR    $31

/*
 * The floating point coprocessor status register bits.
 */
#define MACH_FPC_ROUNDING_BITS          0x00000003
#define MACH_FPC_ROUND_RN               0x00000000
#define MACH_FPC_ROUND_RZ               0x00000001
#define MACH_FPC_ROUND_RP               0x00000002
#define MACH_FPC_ROUND_RM               0x00000003
#define MACH_FPC_STICKY_BITS            0x0000007c
#define MACH_FPC_STICKY_INEXACT         0x00000004
#define MACH_FPC_STICKY_UNDERFLOW       0x00000008
#define MACH_FPC_STICKY_OVERFLOW        0x00000010
#define MACH_FPC_STICKY_DIV0            0x00000020
#define MACH_FPC_STICKY_INVALID         0x00000040
#define MACH_FPC_ENABLE_BITS            0x00000f80
#define MACH_FPC_ENABLE_INEXACT         0x00000080
#define MACH_FPC_ENABLE_UNDERFLOW       0x00000100
#define MACH_FPC_ENABLE_OVERFLOW        0x00000200
#define MACH_FPC_ENABLE_DIV0            0x00000400
#define MACH_FPC_ENABLE_INVALID         0x00000800
#define MACH_FPC_EXCEPTION_BITS         0x0003f000
#define MACH_FPC_EXCEPTION_INEXACT      0x00001000
#define MACH_FPC_EXCEPTION_UNDERFLOW    0x00002000
#define MACH_FPC_EXCEPTION_OVERFLOW     0x00004000
#define MACH_FPC_EXCEPTION_DIV0         0x00008000
#define MACH_FPC_EXCEPTION_INVALID      0x00010000
#define MACH_FPC_EXCEPTION_UNIMPL       0x00020000
#define MACH_FPC_COND_BIT               0x00800000
#define MACH_FPC_MBZ_BITS               0xff7c0000

/*
 * Constants to determine if have a floating point instruction.
 */
#define MACH_OPCODE_SHIFT               26
#define MACH_OPCODE_C1                  0x11

/*
 * The number of TLB entries and the first one that write random hits.
 */
#define VMMACH_NUM_TLB_ENTRIES          16
#define VMMACH_FIRST_RAND_ENTRY         1

/*
 * The number of process id entries.
 */
#define VMMACH_NUM_PIDS                 256

/*
 * TLB probe return codes.
 */
#define VMMACH_TLB_NOT_FOUND            0
#define VMMACH_TLB_FOUND                1
#define VMMACH_TLB_FOUND_WITH_PATCH     2
#define VMMACH_TLB_PROBE_ERROR          3

/*
 * Kernel virtual address for user page table entries
 * (i.e., the address for the context register).
 */
#define VMMACH_PTE_BASE         0xFFC00000


/*
 * Empty memory write buffer.
 */
#define mips_sync()             asm volatile("sync")

/*
 * Read C0 coprocessor register.
 */
#define mips_mfc0(reg, sel) ({ int __value; \
    asm volatile ( \
    "mfc0    %0, $%1, %2" \
    : "=r" (__value) : "K" (reg), "K" (sel)); \
    __value; })

/*
 * Write coprocessor 0 register.
 */
#define mips_mtc0(reg, sel, value) asm volatile ( \
    "mtc0    %z0, $%1, %2" \
    : : "r" ((unsigned) (value)), "K" (reg), "K" (sel))

/*
 * Disable the hardware interrupts,
 * saving the value of Status register.
 */
#define mips_di() ({ int __value; \
    asm volatile ( \
    "di      %0" \
    : "=r" (__value)); \
    __value; })

// WHITECAT ADDITION BEGIN
#define mips_ei() ({ int __value; \
    asm volatile ( \
    "ei      %0" \
    : "=r" (__value)); \
    __value; })

/*
* Read C1 coprocessor register.
*/
#define mips_mfc1(reg, sel) ({ int __value; \
   asm volatile ( \
   "mfc1    %0, $%1, %2" \
   : "=r" (__value) : "K" (reg), "K" (sel)); \
   __value; })

/*
* Write coprocessor 1 register.
*/
#define mips_mtc1(reg, sel, value) asm volatile ( \
   "mtc1    %z0, $%1, %2" \
   : : "r" ((unsigned) (value)), "K" (reg), "K" (sel))
							   
// WHITECAT ADDITION END

/*
 * Clear a range of bits.
 */
#define mips_clear_bits(word, offset, width) asm volatile ( \
    "ins     %0, $zero, %2, %3" \
    : "=r" (word) : "0" (word), "K" (offset), "K" (width))

/*
 * Modify a range of bits.
 */
#define mips_ins(word, value, offset, width) asm volatile ( \
    "ins     %0, %2, %3, %4" \
    : "=r" (word) : "0" (word), "r" (value), "K" (offset), "K" (width))

#ifndef LOCORE
/*
 * Count a number of leading (most significant) zero bits in a word.
 */
static int inline __attribute__ ((always_inline))
mips_clz (unsigned x)
{
    int n;

    asm volatile ("clz     %0, %1"
            : "=r" (n) : "r" (x));
    return n;
}
#endif /* LOCORE */

#endif /* _MACHCONST */
