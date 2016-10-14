/*
 * MIPS exception codes.
 *
 * Copyright (c) 2014 Serge Vakulenko
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

/*
 * See MIPS achitecture docs for description of Cause.ExcCode field.
 */
#define TRAP_Int    0       /* Interrupt */
#define TRAP_MOD    1       /* TLB modified */
#define TRAP_TLBL   2       /* TLB refill (load or fetch) */
#define TRAP_TLBS   3       /* TLB refill (store) */
#define TRAP_AdEL   4       /* Address error (load or fetch) */
#define TRAP_AdES   5       /* Address error (store) */
#define TRAP_IBE    6       /* Bus error (instruction fetch) */
#define TRAP_DBE    7       /* Bus error (data load or store) */
#define TRAP_Sys    8       /* Syscall */
#define TRAP_Bp     9       /* Breakpoint */
#define TRAP_RI     10      /* Reserved instruction */
#define TRAP_CPU    11      /* Coprocessor Unusable */
#define TRAP_Ov     12      /* Arithmetic Overflow */
#define TRAP_Tr     13      /* Trap */
#define TRAP_TLBRI  19      /* TLB read-inhibit */
#define TRAP_TLBEI  20      /* TLB execute-inhibit */
#define TRAP_WATCH  23      /* Reference to WatchHi/WatchLo address */
#define TRAP_MCheck 24      /* Machine check */
#define TRAP_DSPDis 26      /* DSP disabled */

#define TRAP_USER   0x20    /* user-mode flag or'ed with type */
