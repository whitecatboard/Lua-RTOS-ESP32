/*
 * Hardware register defines for Microchip PIC32MZ microcontroller.
 *
 * Copyright (C) 2013 Serge Vakulenko
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
#ifndef _IO_PIC32MZ_H
#define _IO_PIC32MZ_H

#ifdef __LANGUAGE_ASSEMBLY__
#define _CP0_STATUS_BEV_MASK                   (1 << 22)

#define _CP0_INDEX                             $0, 0
#define _CP0_INX                               $0, 0
#define _CP0_RANDOM                            $1, 0
#define _CP0_RAND                              $1, 0
#define _CP0_ENTRYLO0                          $2, 0
#define _CP0_TLBLO0                            $2, 0
#define _CP0_ENTRYLO1                          $3, 0
#define _CP0_TLBLO1                            $3, 0
#define _CP0_CONTEXT                           $4, 0
#define _CP0_CTXT                              $4, 0
#define _CP0_USERLOCAL                         $4, 2
#define _CP0_PAGEMASK                          $5, 0
#define _CP0_PAGEGRAIN                         $5, 1
#define _CP0_WIRED                             $6, 0
#define _CP0_HWRENA                            $7, 0
#define _CP0_BADVADDR                          $8, 0
#define _CP0_COUNT                             $9, 0
#define _CP0_ENTRYHI                           $10, 0
#define _CP0_COMPARE                           $11, 0
#define _CP0_STATUS                            $12, 0
#define _CP0_INTCTL                            $12, 1
#define _CP0_SRSCTL                            $12, 2
#define _CP0_SRSMAP                            $12, 3
#define _CP0_VIEW_IPL                          $12, 4
#define _CP0_SRSMAP2                           $12, 5
#define _CP0_CAUSE                             $13, 0
#define _CP0_VIEW_RIPL                         $13, 1
#define _CP0_NESTEDEXC                         $13, 2
#define _CP0_EPC                               $14, 0
#define _CP0_NESTEDEPC                         $14, 2
#define _CP0_PRID                              $15, 0
#define _CP0_EBASE                             $15, 1
#define _CP0_CDMMBASE                          $15, 2
#define _CP0_CONFIG                            $16, 0
#define _CP0_CONFIG1                           $16, 1
#define _CP0_CONFIG2                           $16, 2
#define _CP0_CONFIG3                           $16, 3
#define _CP0_CONFIG4                           $16, 4
#define _CP0_CONFIG5                           $16, 5
#define _CP0_CONFIG7                           $16, 7
#define _CP0_LLADDR                            $17, 0
#define _CP0_WATCHLO                           $18, 0
#define _CP0_WATCHHI                           $19, 0
#define _CP0_DEBUG                             $23, 0
#define _CP0_TRACECONTROL                      $23, 1
#define _CP0_TRACECONTROL2                     $23, 2
#define _CP0_USERTRACEDATA                     $23, 3
#define _CP0_TRACEBPC                          $23, 4
#define _CP0_DEBUG2                            $23, 5
#define _CP0_DEPC                              $24, 0
#define _CP0_USERTRACEDATA2                    $24, 1
#define _CP0_PERFCNT0_CONTROL                  $25, 0
#define _CP0_PERFCNT0_COUNT                    $25, 1
#define _CP0_PERFCNT1_CONTROL                  $25, 2
#define _CP0_PERFCNT1_COUNT                    $25, 3
#define _CP0_ERRCTL                            $26, 0
#define _CP0_CACHEERR                          $27, 0
#define _CP0_TAGLO                             $28, 0
#define _CP0_DATALO                            $28, 1
#define _CP0_ERROREPC                          $30, 0
#define _CP0_DESAVE                            $31, 0
#else
#define _CP0_COUNT                            9
#define _CP0_COMPARE					  	  11
#define _CP0_INTCTL                           11
#define _CP0_STATUS                           12
#define _CP0_CAUSE                            13
#define _CP0_EPC                              14
#define _CP0_EBASE                            15
#endif

#define _CP0_GET_CAUSE()       mips_mfc0 (_CP0_CAUSE, 0)
#define _CP0_SET_CAUSE(val)    mips_mtc0 (_CP0_CAUSE, 0, val)


#define UNLOCK_KEY_0					( 0xAA996655UL )
#define UNLOCK_KEY_1					( 0x556699AAUL )

#define _IFS0_CTIF_POSITION                      0x00000000
#define _IFS0_CTIF_MASK                          0x00000001
#define _IFS0_CTIF_LENGTH                        0x00000001

#define _IFS0_CS0IF_POSITION                     0x00000001
#define _IFS0_CS0IF_MASK                         0x00000002
#define _IFS0_CS0IF_LENGTH                       0x00000001

#define _IFS0_CS1IF_POSITION                     0x00000002
#define _IFS0_CS1IF_MASK                         0x00000004
#define _IFS0_CS1IF_LENGTH                       0x00000001

#define _IFS0_INT0IF_POSITION                    0x00000003
#define _IFS0_INT0IF_MASK                        0x00000008
#define _IFS0_INT0IF_LENGTH                      0x00000001

#define _IFS0_T1IF_POSITION                      0x00000004
#define _IFS0_T1IF_MASK                          0x00000010
#define _IFS0_T1IF_LENGTH                        0x00000001

#define _IFS0_IC1EIF_POSITION                    0x00000005
#define _IFS0_IC1EIF_MASK                        0x00000020
#define _IFS0_IC1EIF_LENGTH                      0x00000001

#define _IFS0_IC1IF_POSITION                     0x00000006
#define _IFS0_IC1IF_MASK                         0x00000040
#define _IFS0_IC1IF_LENGTH                       0x00000001

#define _IFS0_OC1IF_POSITION                     0x00000007
#define _IFS0_OC1IF_MASK                         0x00000080
#define _IFS0_OC1IF_LENGTH                       0x00000001

#define _IFS0_INT1IF_POSITION                    0x00000008
#define _IFS0_INT1IF_MASK                        0x00000100
#define _IFS0_INT1IF_LENGTH                      0x00000001

#define _IFS0_T2IF_POSITION                      0x00000009
#define _IFS0_T2IF_MASK                          0x00000200
#define _IFS0_T2IF_LENGTH                        0x00000001

#define _IFS0_IC2EIF_POSITION                    0x0000000A
#define _IFS0_IC2EIF_MASK                        0x00000400
#define _IFS0_IC2EIF_LENGTH                      0x00000001

#define _IFS0_IC2IF_POSITION                     0x0000000B
#define _IFS0_IC2IF_MASK                         0x00000800
#define _IFS0_IC2IF_LENGTH                       0x00000001

#define _IFS0_OC2IF_POSITION                     0x0000000C
#define _IFS0_OC2IF_MASK                         0x00001000
#define _IFS0_OC2IF_LENGTH                       0x00000001

#define _IFS0_INT2IF_POSITION                    0x0000000D
#define _IFS0_INT2IF_MASK                        0x00002000
#define _IFS0_INT2IF_LENGTH                      0x00000001

#define _IFS0_T3IF_POSITION                      0x0000000E
#define _IFS0_T3IF_MASK                          0x00004000
#define _IFS0_T3IF_LENGTH                        0x00000001

#define _IFS0_IC3EIF_POSITION                    0x0000000F
#define _IFS0_IC3EIF_MASK                        0x00008000
#define _IFS0_IC3EIF_LENGTH                      0x00000001

#define _IFS0_IC3IF_POSITION                     0x00000010
#define _IFS0_IC3IF_MASK                         0x00010000
#define _IFS0_IC3IF_LENGTH                       0x00000001

#define _IFS0_OC3IF_POSITION                     0x00000011
#define _IFS0_OC3IF_MASK                         0x00020000
#define _IFS0_OC3IF_LENGTH                       0x00000001

#define _IFS0_INT3IF_POSITION                    0x00000012
#define _IFS0_INT3IF_MASK                        0x00040000
#define _IFS0_INT3IF_LENGTH                      0x00000001

#define _IFS0_T4IF_POSITION                      0x00000013
#define _IFS0_T4IF_MASK                          0x00080000
#define _IFS0_T4IF_LENGTH                        0x00000001

#define _IFS0_IC4EIF_POSITION                    0x00000014
#define _IFS0_IC4EIF_MASK                        0x00100000
#define _IFS0_IC4EIF_LENGTH                      0x00000001

#define _IFS0_IC4IF_POSITION                     0x00000015
#define _IFS0_IC4IF_MASK                         0x00200000
#define _IFS0_IC4IF_LENGTH                       0x00000001

#define _IFS0_OC4IF_POSITION                     0x00000016
#define _IFS0_OC4IF_MASK                         0x00400000
#define _IFS0_OC4IF_LENGTH                       0x00000001

#define _IFS0_INT4IF_POSITION                    0x00000017
#define _IFS0_INT4IF_MASK                        0x00800000
#define _IFS0_INT4IF_LENGTH                      0x00000001

#define _IFS0_T5IF_POSITION                      0x00000018
#define _IFS0_T5IF_MASK                          0x01000000
#define _IFS0_T5IF_LENGTH                        0x00000001

#define _IFS0_IC5EIF_POSITION                    0x00000019
#define _IFS0_IC5EIF_MASK                        0x02000000
#define _IFS0_IC5EIF_LENGTH                      0x00000001

#define _IFS0_IC5IF_POSITION                     0x0000001A
#define _IFS0_IC5IF_MASK                         0x04000000
#define _IFS0_IC5IF_LENGTH                       0x00000001

#define _IFS0_OC5IF_POSITION                     0x0000001B
#define _IFS0_OC5IF_MASK                         0x08000000
#define _IFS0_OC5IF_LENGTH                       0x00000001

#define _IFS0_T6IF_POSITION                      0x0000001C
#define _IFS0_T6IF_MASK                          0x10000000
#define _IFS0_T6IF_LENGTH                        0x00000001

#define _IFS0_IC6EIF_POSITION                    0x0000001D
#define _IFS0_IC6EIF_MASK                        0x20000000
#define _IFS0_IC6EIF_LENGTH                      0x00000001

#define _IFS0_IC6IF_POSITION                     0x0000001E
#define _IFS0_IC6IF_MASK                         0x40000000
#define _IFS0_IC6IF_LENGTH                       0x00000001

#define _IFS0_OC6IF_POSITION                     0x0000001F
#define _IFS0_OC6IF_MASK                         0x80000000
#define _IFS0_OC6IF_LENGTH                       0x00000001

#define _IFS0_w_POSITION                         0x00000000
#define _IFS0_w_MASK                             0xFFFFFFFF
#define _IFS0_w_LENGTH                           0x00000020

#define _IPC0_CTIS_POSITION                      0x00000000
#define _IPC0_CTIS_MASK                          0x00000003
#define _IPC0_CTIS_LENGTH                        0x00000002

#define _IPC0_CTIP_POSITION                      0x00000002
#define _IPC0_CTIP_MASK                          0x0000001C
#define _IPC0_CTIP_LENGTH                        0x00000003

#define _IPC0_CS0IS_POSITION                     0x00000008
#define _IPC0_CS0IS_MASK                         0x00000300
#define _IPC0_CS0IS_LENGTH                       0x00000002

#define _IPC0_CS0IP_POSITION                     0x0000000A
#define _IPC0_CS0IP_MASK                         0x00001C00
#define _IPC0_CS0IP_LENGTH                       0x00000003

#define _IPC0_CS1IS_POSITION                     0x00000010
#define _IPC0_CS1IS_MASK                         0x00030000
#define _IPC0_CS1IS_LENGTH                       0x00000002

#define _IPC0_CS1IP_POSITION                     0x00000012
#define _IPC0_CS1IP_MASK                         0x001C0000
#define _IPC0_CS1IP_LENGTH                       0x00000003

#define _IPC0_INT0IS_POSITION                    0x00000018
#define _IPC0_INT0IS_MASK                        0x03000000
#define _IPC0_INT0IS_LENGTH                      0x00000002

#define _IPC0_INT0IP_POSITION                    0x0000001A
#define _IPC0_INT0IP_MASK                        0x1C000000
#define _IPC0_INT0IP_LENGTH                      0x00000003

#define _IPC0_w_POSITION                         0x00000000
#define _IPC0_w_MASK                             0xFFFFFFFF
#define _IPC0_w_LENGTH                           0x00000020

#define _IEC0_CTIE_POSITION                      0x00000000
#define _IEC0_CTIE_MASK                          0x00000001
#define _IEC0_CTIE_LENGTH                        0x00000001

#define _IEC0_CS0IE_POSITION                     0x00000001
#define _IEC0_CS0IE_MASK                         0x00000002
#define _IEC0_CS0IE_LENGTH                       0x00000001

#define _IEC0_CS1IE_POSITION                     0x00000002
#define _IEC0_CS1IE_MASK                         0x00000004
#define _IEC0_CS1IE_LENGTH                       0x00000001

#define _IEC0_INT0IE_POSITION                    0x00000003
#define _IEC0_INT0IE_MASK                        0x00000008
#define _IEC0_INT0IE_LENGTH                      0x00000001

#define _IEC0_T1IE_POSITION                      0x00000004
#define _IEC0_T1IE_MASK                          0x00000010
#define _IEC0_T1IE_LENGTH                        0x00000001

#define _IEC0_IC1EIE_POSITION                    0x00000005
#define _IEC0_IC1EIE_MASK                        0x00000020
#define _IEC0_IC1EIE_LENGTH                      0x00000001

#define _IEC0_IC1IE_POSITION                     0x00000006
#define _IEC0_IC1IE_MASK                         0x00000040
#define _IEC0_IC1IE_LENGTH                       0x00000001

#define _IEC0_OC1IE_POSITION                     0x00000007
#define _IEC0_OC1IE_MASK                         0x00000080
#define _IEC0_OC1IE_LENGTH                       0x00000001

#define _IEC0_INT1IE_POSITION                    0x00000008
#define _IEC0_INT1IE_MASK                        0x00000100
#define _IEC0_INT1IE_LENGTH                      0x00000001

#define _IEC0_T2IE_POSITION                      0x00000009
#define _IEC0_T2IE_MASK                          0x00000200
#define _IEC0_T2IE_LENGTH                        0x00000001

#define _IEC0_IC2EIE_POSITION                    0x0000000A
#define _IEC0_IC2EIE_MASK                        0x00000400
#define _IEC0_IC2EIE_LENGTH                      0x00000001

#define _IEC0_IC2IE_POSITION                     0x0000000B
#define _IEC0_IC2IE_MASK                         0x00000800
#define _IEC0_IC2IE_LENGTH                       0x00000001

#define _IEC0_OC2IE_POSITION                     0x0000000C
#define _IEC0_OC2IE_MASK                         0x00001000
#define _IEC0_OC2IE_LENGTH                       0x00000001

#define _IEC0_INT2IE_POSITION                    0x0000000D
#define _IEC0_INT2IE_MASK                        0x00002000
#define _IEC0_INT2IE_LENGTH                      0x00000001

#define _IEC0_T3IE_POSITION                      0x0000000E
#define _IEC0_T3IE_MASK                          0x00004000
#define _IEC0_T3IE_LENGTH                        0x00000001

#define _IEC0_IC3EIE_POSITION                    0x0000000F
#define _IEC0_IC3EIE_MASK                        0x00008000
#define _IEC0_IC3EIE_LENGTH                      0x00000001

#define _IEC0_IC3IE_POSITION                     0x00000010
#define _IEC0_IC3IE_MASK                         0x00010000
#define _IEC0_IC3IE_LENGTH                       0x00000001

#define _IEC0_OC3IE_POSITION                     0x00000011
#define _IEC0_OC3IE_MASK                         0x00020000
#define _IEC0_OC3IE_LENGTH                       0x00000001

#define _IEC0_INT3IE_POSITION                    0x00000012
#define _IEC0_INT3IE_MASK                        0x00040000
#define _IEC0_INT3IE_LENGTH                      0x00000001

#define _IEC0_T4IE_POSITION                      0x00000013
#define _IEC0_T4IE_MASK                          0x00080000
#define _IEC0_T4IE_LENGTH                        0x00000001

#define _IEC0_IC4EIE_POSITION                    0x00000014
#define _IEC0_IC4EIE_MASK                        0x00100000
#define _IEC0_IC4EIE_LENGTH                      0x00000001

#define _IEC0_IC4IE_POSITION                     0x00000015
#define _IEC0_IC4IE_MASK                         0x00200000
#define _IEC0_IC4IE_LENGTH                       0x00000001

#define _IEC0_OC4IE_POSITION                     0x00000016
#define _IEC0_OC4IE_MASK                         0x00400000
#define _IEC0_OC4IE_LENGTH                       0x00000001

#define _IEC0_INT4IE_POSITION                    0x00000017
#define _IEC0_INT4IE_MASK                        0x00800000
#define _IEC0_INT4IE_LENGTH                      0x00000001

#define _IEC0_T5IE_POSITION                      0x00000018
#define _IEC0_T5IE_MASK                          0x01000000
#define _IEC0_T5IE_LENGTH                        0x00000001

#define _IEC0_IC5EIE_POSITION                    0x00000019
#define _IEC0_IC5EIE_MASK                        0x02000000
#define _IEC0_IC5EIE_LENGTH                      0x00000001

#define _IEC0_IC5IE_POSITION                     0x0000001A
#define _IEC0_IC5IE_MASK                         0x04000000
#define _IEC0_IC5IE_LENGTH                       0x00000001

#define _IEC0_OC5IE_POSITION                     0x0000001B
#define _IEC0_OC5IE_MASK                         0x08000000
#define _IEC0_OC5IE_LENGTH                       0x00000001

#define _IEC0_T6IE_POSITION                      0x0000001C
#define _IEC0_T6IE_MASK                          0x10000000
#define _IEC0_T6IE_LENGTH                        0x00000001

#define _IEC0_IC6EIE_POSITION                    0x0000001D
#define _IEC0_IC6EIE_MASK                        0x20000000
#define _IEC0_IC6EIE_LENGTH                      0x00000001

#define _IEC0_IC6IE_POSITION                     0x0000001E
#define _IEC0_IC6IE_MASK                         0x40000000
#define _IEC0_IC6IE_LENGTH                       0x00000001

#define _IEC0_OC6IE_POSITION                     0x0000001F
#define _IEC0_OC6IE_MASK                         0x80000000
#define _IEC0_OC6IE_LENGTH                       0x00000001

#define _IEC0_w_POSITION                         0x00000000
#define _IEC0_w_MASK                             0xFFFFFFFF
#define _IEC0_w_LENGTH                           0x00000020




/*
 * Status register.
 */
#define ST_CU1          0x20000000      /* Access to coprocessor 1 allowed (in user mode) */
#define ST_CU0          0x10000000      /* Access to coprocessor 0 allowed (in user mode) */
#define ST_MX           0x01000000      /* MIPS DSP Resource Enable bit */
#define ST_RP           0x08000000      /* Enable reduced power mode */
#define ST_FR 			0x04000000		/* Floating-point registers can contain any data type */
#define ST_RE           0x02000000      /* Reverse endianness (in user mode) */
#define ST_BEV          0x00400000      /* Exception vectors: bootstrap */
#define ST_SR           0x00100000      /* Soft reset */
#define ST_NMI          0x00080000      /* NMI reset */
#define ST_IPL(x)       ((x) << 10)     /* Current interrupt priority level */
#define ST_UM           0x00000010      /* User mode */
#define ST_ERL          0x00000004      /* Error level */
#define ST_EXL          0x00000002      /* Exception level */
#define ST_IE           0x00000001      /* Interrupt enable */

/*
 * Сause register.
 */
#define CA_BD           0x80000000      /* Exception occured in delay slot */
#define CA_TI           0x40000000      /* Timer interrupt is pending */
#define CA_CE           0x30000000      /* Coprocessor exception */
#define CA_DC           0x08000000      /* Disable COUNT register */
#define CA_IV           0x00800000      /* Use special interrupt vector 0x200 */
#define CA_RIPL(r)      ((r)>>10 & 63)  /* Requested interrupt priority level */
#define CA_IP1          0x00020000      /* Request software interrupt 1 */
#define CA_IP0          0x00010000      /* Request software interrupt 0 */
#define CA_EXC_CODE     0x0000007c      /* Exception code */

#define CA_Int          0               /* Interrupt */
#define CA_AdEL         (4 << 2)        /* Address error, load or instruction fetch */
#define CA_AdES         (5 << 2)        /* Address error, store */
#define CA_IBE          (6 << 2)        /* Bus error, instruction fetch */
#define CA_DBE          (7 << 2)        /* Bus error, load or store */
#define CA_Sys          (8 << 2)        /* Syscall */
#define CA_Bp           (9 << 2)        /* Breakpoint */
#define CA_RI           (10 << 2)       /* Reserved instruction */
#define CA_CPU          (11 << 2)       /* Coprocessor unusable */
#define CA_Ov           (12 << 2)       /* Arithmetic overflow */
#define CA_Tr           (13 << 2)       /* Trap */

#define DB_DBD          (1 << 31)       /* Debug exception in a branch delay slot */
#define DB_DM           (1 << 30)       /* Debug mode */
#define DB_NODCR        (1 << 29)       /* No dseg present */
#define DB_LSNM         (1 << 28)       /* Load/stores in dseg go to main memory */
#define DB_DOZE         (1 << 27)       /* Processor was in low-power mode */
#define DB_HALT         (1 << 26)       /* Internal system bus clock was running */
#define DB_COUNTDM      (1 << 25)       /* Count register is running in Debug mode */
#define DB_IBUSEP       (1 << 24)       /* Instruction fetch bus error exception */
#define DB_DBUSEP       (1 << 21)       /* Data access bus error exception */
#define DB_IEXI         (1 << 20)       /* Imprecise error exception */
#define DB_VER          (7 << 15)       /* EJTAG version number */
#define DB_DEXCCODE     (0x1f << 10)    /* Cause of exception in Debug mode */
#define DB_SST          (1 << 8)        /* Single step exception enabled */
#define DB_DIBImpr      (1 << 6)        /* Imprecise debug instruction break */
#define DB_DINT         (1 << 5)        /* Debug interrupt exception */
#define DB_DIB          (1 << 4)        /* Debug instruction break exception */
#define DB_DDBS         (1 << 3)        /* Debug data break exception on store */
#define DB_DDBL         (1 << 2)        /* Debug data break exception on load */
#define DB_DBP          (1 << 1)        /* Debug software breakpoint exception */
#define DB_DSS          (1 << 0)        /* Debug single-step exception */


/*
 * Register memory map:
 *
 *  BF80 0000...03FF    Configuration
 *  BF80 0600...07FF    Flash Controller
 *  BF80 0800...09FF    Watchdog Timer
 *  BF80 0A00...0BFF    Deadman Timer
 *  BF80 0C00...0DFF    RTCC
 *  BF80 0E00...0FFF    CVref
 *  BF80 1200...13FF    Oscillator
 *  BF80 1400...17FF    PPS
 *
 *  BF81 0000...0FFF    Interrupt Controller
 *  BF81 1000...1FFF    DMA
 *
 *  BF82 0000...09FF    I2C1 - I2C5
 *  BF82 1000...1BFF    SPI1 - SPI6
 *  BF82 2000...2BFF    UART1 - UART6
 *  BF82 E000...E1FF    PMP
 *
 *  BF84 0000...11FF    Timer1 - Timer9
 *  BF84 2000...31FF    IC1 - IC9
 *  BF84 4000...51FF    OC1 - OC9
 *  BF84 B000...B3FF    ADC1
 *  BF84 C000...C1FF    Comparator 1, 2
 *
 *  BF86 0000...09FF    PORTA - PORTK
 *
 *  BF88 0000...1FFF    CAN1 and CAN2
 *  BF88 2000...2FFF    Ethernet
 *
 *  BF8E 0000...0FFF    Prefetch
 *  BF8E 1000...1FFF    EBI
 *  BF8E 2000...2FFF    SQI1
 *  BF8E 3000...3FFF    USB
 *  BF8E 5000...5FFF    Crypto
 *  BF8E 6000...6FFF    RNG
 *
 *  BF8F 0000...FFFF    System Bus
 */

/*--------------------------------------
 * Configuration registers.
 */
#define DEVCFG0         0x9fc0fffc
#define DEVCFG1         0x9fc0fff8
#define DEVCFG2         0x9fc0fff4
#define DEVCFG3         0x9fc0fff0

#define DEVCFG0_UNUSED          0xbfc00880
#define DEVCFG1_UNUSED          0x00003800
#define DEVCFG2_UNUSED          0xbff88008
#define DEVCFG3_UNUSED          0x84ff0000

#define PIC32_DEVCFG(cfg0, cfg1, cfg2, cfg3) \
    unsigned __DEVCFG0 __attribute__ ((section(".config0"))) = (cfg0) | DEVCFG0_UNUSED; \
    unsigned __DEVCFG1 __attribute__ ((section(".config1"))) = (cfg1) | DEVCFG1_UNUSED; \
    unsigned __DEVCFG2 __attribute__ ((section(".config2"))) = (cfg2) | DEVCFG2_UNUSED; \
    unsigned __DEVCFG3 __attribute__ ((section(".config3"))) = (cfg3) | DEVCFG3_UNUSED

#define DEVCFG3_FMIIEN_OFF	0x00000000	/* RMII Enabled */
#define DEVCFG3_FMIIEN_ON	0x01000000	/* MII Enabled */
#define DEVCFG3_FETHIO_OFF	0x00000000	/* Alternate Ethernet I/O */
#define DEVCFG3_FETHIO_ON	0x02000000	/* Default Ethernet I/O */
#define DEVCFG3_PGL1WAY_OFF	0x00000000	/* Allow multiple reconfigurations */
#define DEVCFG3_PGL1WAY_ON	0x08000000	/* Allow only one reconfiguration */
#define DEVCFG3_PMDL1WAY_OFF	0x00000000	/* Allow multiple reconfigurations */
#define DEVCFG3_PMDL1WAY_ON	0x10000000	/* Allow only one reconfiguration */
#define DEVCFG3_IOL1WAY_OFF	0x00000000	/* Allow multiple reconfigurations */
#define DEVCFG3_IOL1WAY_ON	0x20000000	/* Allow only one reconfiguration */
#define DEVCFG3_FUSBIDIO_OFF	0x00000000	/* Controlled by Port Function */
#define DEVCFG3_FUSBIDIO_ON	0x40000000	/* Controlled by the USB Module */
#define DEVCFG2_FPLLIDIV_DIV_1	0x00000000	/* 1x Divider */
#define DEVCFG2_FPLLIDIV_DIV_2	0x00000001	/* 2x Divider */
#define DEVCFG2_FPLLIDIV_DIV_3	0x00000002	/* 3x Divider */
#define DEVCFG2_FPLLIDIV_DIV_4	0x00000003	/* 4x Divider */
#define DEVCFG2_FPLLIDIV_DIV_5	0x00000004	/* 5x Divider */
#define DEVCFG2_FPLLIDIV_DIV_6	0x00000005	/* 6x Divider */
#define DEVCFG2_FPLLIDIV_DIV_7	0x00000006	/* 7x Divider */
#define DEVCFG2_FPLLIDIV_DIV_8	0x00000007	/* 8x Divider */
#define DEVCFG2_FPLLRNG_RANGE_BYPASS	0x00000000	/* Bypass */
#define DEVCFG2_FPLLRNG_RANGE_5_10_MHZ	0x00000010	/* 5-10 MHz Input */
#define DEVCFG2_FPLLRNG_RANGE_8_16_MHZ	0x00000020	/* 8-16 MHz Input */
#define DEVCFG2_FPLLRNG_RANGE_13_26_MHZ	0x00000030	/* 13-26 MHz Input */
#define DEVCFG2_FPLLRNG_RANGE_21_42_MHZ	0x00000040	/* 21-42 MHz Input */
#define DEVCFG2_FPLLRNG_RANGE_34_68_MHZ	0x00000050	/* 34-68 MHz Input */
#define DEVCFG2_FPLLICLK_PLL_POSC	0x00000000	/* POSC is input to the System PLL */
#define DEVCFG2_FPLLICLK_PLL_FRC	0x00000080	/* FRC is input to the System PLL */
#define DEVCFG2_FPLLMULT_MUL_1	0x00000000	/* PLL Multiply by 1 */
#define DEVCFG2_FPLLMULT_MUL_2	0x00000100	/* PLL Multiply by 2 */
#define DEVCFG2_FPLLMULT_MUL_3	0x00000200	/* PLL Multiply by 3 */
#define DEVCFG2_FPLLMULT_MUL_4	0x00000300	/* PLL Multiply by 4 */
#define DEVCFG2_FPLLMULT_MUL_5	0x00000400	/* PLL Multiply by 5 */
#define DEVCFG2_FPLLMULT_MUL_6	0x00000500	/* PLL Multiply by 6 */
#define DEVCFG2_FPLLMULT_MUL_7	0x00000600	/* PLL Multiply by 7 */
#define DEVCFG2_FPLLMULT_MUL_8	0x00000700	/* PLL Multiply by 8 */
#define DEVCFG2_FPLLMULT_MUL_9	0x00000800	/* PLL Multiply by 9 */
#define DEVCFG2_FPLLMULT_MUL_10	0x00000900	/* PLL Multiply by 10 */
#define DEVCFG2_FPLLMULT_MUL_11	0x00000A00	/* PLL Multiply by 11 */
#define DEVCFG2_FPLLMULT_MUL_12	0x00000B00	/* PLL Multiply by 12 */
#define DEVCFG2_FPLLMULT_MUL_13	0x00000C00	/* PLL Multiply by 13 */
#define DEVCFG2_FPLLMULT_MUL_14	0x00000D00	/* PLL Multiply by 14 */
#define DEVCFG2_FPLLMULT_MUL_15	0x00000E00	/* PLL Multiply by 15 */
#define DEVCFG2_FPLLMULT_MUL_16	0x00000F00	/* PLL Multiply by 16 */
#define DEVCFG2_FPLLMULT_MUL_17	0x00001000	/* PLL Multiply by 17 */
#define DEVCFG2_FPLLMULT_MUL_18	0x00001100	/* PLL Multiply by 18 */
#define DEVCFG2_FPLLMULT_MUL_19	0x00001200	/* PLL Multiply by 19 */
#define DEVCFG2_FPLLMULT_MUL_20	0x00001300	/* PLL Multiply by 20 */
#define DEVCFG2_FPLLMULT_MUL_21	0x00001400	/* PLL Multiply by 21 */
#define DEVCFG2_FPLLMULT_MUL_22	0x00001500	/* PLL Multiply by 22 */
#define DEVCFG2_FPLLMULT_MUL_23	0x00001600	/* PLL Multiply by 23 */
#define DEVCFG2_FPLLMULT_MUL_24	0x00001700	/* PLL Multiply by 24 */
#define DEVCFG2_FPLLMULT_MUL_25	0x00001800	/* PLL Multiply by 25 */
#define DEVCFG2_FPLLMULT_MUL_26	0x00001900	/* PLL Multiply by 26 */
#define DEVCFG2_FPLLMULT_MUL_27	0x00001A00	/* PLL Multiply by 27 */
#define DEVCFG2_FPLLMULT_MUL_28	0x00001B00	/* PLL Multiply by 28 */
#define DEVCFG2_FPLLMULT_MUL_29	0x00001C00	/* PLL Multiply by 29 */
#define DEVCFG2_FPLLMULT_MUL_30	0x00001D00	/* PLL Multiply by 30 */
#define DEVCFG2_FPLLMULT_MUL_31	0x00001E00	/* PLL Multiply by 31 */
#define DEVCFG2_FPLLMULT_MUL_32	0x00001F00	/* PLL Multiply by 32 */
#define DEVCFG2_FPLLMULT_MUL_33	0x00002000	/* PLL Multiply by 33 */
#define DEVCFG2_FPLLMULT_MUL_34	0x00002100	/* PLL Multiply by 34 */
#define DEVCFG2_FPLLMULT_MUL_35	0x00002200	/* PLL Multiply by 35 */
#define DEVCFG2_FPLLMULT_MUL_36	0x00002300	/* PLL Multiply by 36 */
#define DEVCFG2_FPLLMULT_MUL_37	0x00002400	/* PLL Multiply by 37 */
#define DEVCFG2_FPLLMULT_MUL_38	0x00002500	/* PLL Multiply by 38 */
#define DEVCFG2_FPLLMULT_MUL_39	0x00002600	/* PLL Multiply by 39 */
#define DEVCFG2_FPLLMULT_MUL_40	0x00002700	/* PLL Multiply by 40 */
#define DEVCFG2_FPLLMULT_MUL_41	0x00002800	/* PLL Multiply by 41 */
#define DEVCFG2_FPLLMULT_MUL_42	0x00002900	/* PLL Multiply by 42 */
#define DEVCFG2_FPLLMULT_MUL_43	0x00002A00	/* PLL Multiply by 43 */
#define DEVCFG2_FPLLMULT_MUL_44	0x00002B00	/* PLL Multiply by 44 */
#define DEVCFG2_FPLLMULT_MUL_45	0x00002C00	/* PLL Multiply by 45 */
#define DEVCFG2_FPLLMULT_MUL_46	0x00002D00	/* PLL Multiply by 46 */
#define DEVCFG2_FPLLMULT_MUL_47	0x00002E00	/* PLL Multiply by 47 */
#define DEVCFG2_FPLLMULT_MUL_48	0x00002F00	/* PLL Multiply by 48 */
#define DEVCFG2_FPLLMULT_MUL_49	0x00003000	/* PLL Multiply by 49 */
#define DEVCFG2_FPLLMULT_MUL_50	0x00003100	/* PLL Multiply by 50 */
#define DEVCFG2_FPLLMULT_MUL_51	0x00003200	/* PLL Multiply by 51 */
#define DEVCFG2_FPLLMULT_MUL_52	0x00003300	/* PLL Multiply by 52 */
#define DEVCFG2_FPLLMULT_MUL_53	0x00003400	/* PLL Multiply by 53 */
#define DEVCFG2_FPLLMULT_MUL_54	0x00003500	/* PLL Multiply by 54 */
#define DEVCFG2_FPLLMULT_MUL_55	0x00003600	/* PLL Multiply by 55 */
#define DEVCFG2_FPLLMULT_MUL_56	0x00003700	/* PLL Multiply by 56 */
#define DEVCFG2_FPLLMULT_MUL_57	0x00003800	/* PLL Multiply by 57 */
#define DEVCFG2_FPLLMULT_MUL_58	0x00003900	/* PLL Multiply by 58 */
#define DEVCFG2_FPLLMULT_MUL_59	0x00003A00	/* PLL Multiply by 59 */
#define DEVCFG2_FPLLMULT_MUL_60	0x00003B00	/* PLL Multiply by 60 */
#define DEVCFG2_FPLLMULT_MUL_61	0x00003C00	/* PLL Multiply by 61 */
#define DEVCFG2_FPLLMULT_MUL_62	0x00003D00	/* PLL Multiply by 62 */
#define DEVCFG2_FPLLMULT_MUL_63	0x00003E00	/* PLL Multiply by 63 */
#define DEVCFG2_FPLLMULT_MUL_64	0x00003F00	/* PLL Multiply by 64 */
#define DEVCFG2_FPLLMULT_MUL_65	0x00004000	/* PLL Multiply by 65 */
#define DEVCFG2_FPLLMULT_MUL_66	0x00004100	/* PLL Multiply by 66 */
#define DEVCFG2_FPLLMULT_MUL_67	0x00004200	/* PLL Multiply by 67 */
#define DEVCFG2_FPLLMULT_MUL_68	0x00004300	/* PLL Multiply by 68 */
#define DEVCFG2_FPLLMULT_MUL_69	0x00004400	/* PLL Multiply by 69 */
#define DEVCFG2_FPLLMULT_MUL_70	0x00004500	/* PLL Multiply by 70 */
#define DEVCFG2_FPLLMULT_MUL_71	0x00004600	/* PLL Multiply by 71 */
#define DEVCFG2_FPLLMULT_MUL_72	0x00004700	/* PLL Multiply by 72 */
#define DEVCFG2_FPLLMULT_MUL_73	0x00004800	/* PLL Multiply by 73 */
#define DEVCFG2_FPLLMULT_MUL_74	0x00004900	/* PLL Multiply by 74 */
#define DEVCFG2_FPLLMULT_MUL_75	0x00004A00	/* PLL Multiply by 75 */
#define DEVCFG2_FPLLMULT_MUL_76	0x00004B00	/* PLL Multiply by 76 */
#define DEVCFG2_FPLLMULT_MUL_77	0x00004C00	/* PLL Multiply by 77 */
#define DEVCFG2_FPLLMULT_MUL_78	0x00004D00	/* PLL Multiply by 78 */
#define DEVCFG2_FPLLMULT_MUL_79	0x00004E00	/* PLL Multiply by 79 */
#define DEVCFG2_FPLLMULT_MUL_80	0x00004F00	/* PLL Multiply by 80 */
#define DEVCFG2_FPLLMULT_MUL_81	0x00005000	/* PLL Multiply by 81 */
#define DEVCFG2_FPLLMULT_MUL_82	0x00005100	/* PLL Multiply by 82 */
#define DEVCFG2_FPLLMULT_MUL_83	0x00005200	/* PLL Multiply by 83 */
#define DEVCFG2_FPLLMULT_MUL_84	0x00005300	/* PLL Multiply by 84 */
#define DEVCFG2_FPLLMULT_MUL_85	0x00005400	/* PLL Multiply by 85 */
#define DEVCFG2_FPLLMULT_MUL_86	0x00005500	/* PLL Multiply by 86 */
#define DEVCFG2_FPLLMULT_MUL_87	0x00005600	/* PLL Multiply by 87 */
#define DEVCFG2_FPLLMULT_MUL_88	0x00005700	/* PLL Multiply by 88 */
#define DEVCFG2_FPLLMULT_MUL_89	0x00005800	/* PLL Multiply by 89 */
#define DEVCFG2_FPLLMULT_MUL_90	0x00005900	/* PLL Multiply by 90 */
#define DEVCFG2_FPLLMULT_MUL_91	0x00005A00	/* PLL Multiply by 91 */
#define DEVCFG2_FPLLMULT_MUL_92	0x00005B00	/* PLL Multiply by 92 */
#define DEVCFG2_FPLLMULT_MUL_93	0x00005C00	/* PLL Multiply by 93 */
#define DEVCFG2_FPLLMULT_MUL_94	0x00005D00	/* PLL Multiply by 94 */
#define DEVCFG2_FPLLMULT_MUL_95	0x00005E00	/* PLL Multiply by 95 */
#define DEVCFG2_FPLLMULT_MUL_96	0x00005F00	/* PLL Multiply by 96 */
#define DEVCFG2_FPLLMULT_MUL_97	0x00006000	/* PLL Multiply by 97 */
#define DEVCFG2_FPLLMULT_MUL_98	0x00006100	/* PLL Multiply by 98 */
#define DEVCFG2_FPLLMULT_MUL_99	0x00006200	/* PLL Multiply by 99 */
#define DEVCFG2_FPLLMULT_MUL_100	0x00006300	/* PLL Multiply by 100 */
#define DEVCFG2_FPLLMULT_MUL_101	0x00006400	/* PLL Multiply by 101 */
#define DEVCFG2_FPLLMULT_MUL_102	0x00006500	/* PLL Multiply by 102 */
#define DEVCFG2_FPLLMULT_MUL_103	0x00006600	/* PLL Multiply by 103 */
#define DEVCFG2_FPLLMULT_MUL_104	0x00006700	/* PLL Multiply by 104 */
#define DEVCFG2_FPLLMULT_MUL_105	0x00006800	/* PLL Multiply by 105 */
#define DEVCFG2_FPLLMULT_MUL_106	0x00006900	/* PLL Multiply by 106 */
#define DEVCFG2_FPLLMULT_MUL_107	0x00006A00	/* PLL Multiply by 107 */
#define DEVCFG2_FPLLMULT_MUL_108	0x00006B00	/* PLL Multiply by 108 */
#define DEVCFG2_FPLLMULT_MUL_109	0x00006C00	/* PLL Multiply by 109 */
#define DEVCFG2_FPLLMULT_MUL_110	0x00006D00	/* PLL Multiply by 110 */
#define DEVCFG2_FPLLMULT_MUL_111	0x00006E00	/* PLL Multiply by 111 */
#define DEVCFG2_FPLLMULT_MUL_112	0x00006F00	/* PLL Multiply by 112 */
#define DEVCFG2_FPLLMULT_MUL_113	0x00007000	/* PLL Multiply by 113 */
#define DEVCFG2_FPLLMULT_MUL_114	0x00007100	/* PLL Multiply by 114 */
#define DEVCFG2_FPLLMULT_MUL_115	0x00007200	/* PLL Multiply by 115 */
#define DEVCFG2_FPLLMULT_MUL_116	0x00007300	/* PLL Multiply by 116 */
#define DEVCFG2_FPLLMULT_MUL_117	0x00007400	/* PLL Multiply by 117 */
#define DEVCFG2_FPLLMULT_MUL_118	0x00007500	/* PLL Multiply by 118 */
#define DEVCFG2_FPLLMULT_MUL_119	0x00007600	/* PLL Multiply by 119 */
#define DEVCFG2_FPLLMULT_MUL_120	0x00007700	/* PLL Multiply by 120 */
#define DEVCFG2_FPLLMULT_MUL_121	0x00007800	/* PLL Multiply by 121 */
#define DEVCFG2_FPLLMULT_MUL_122	0x00007900	/* PLL Multiply by 122 */
#define DEVCFG2_FPLLMULT_MUL_123	0x00007A00	/* PLL Multiply by 123 */
#define DEVCFG2_FPLLMULT_MUL_124	0x00007B00	/* PLL Multiply by 124 */
#define DEVCFG2_FPLLMULT_MUL_125	0x00007C00	/* PLL Multiply by 125 */
#define DEVCFG2_FPLLMULT_MUL_126	0x00007D00	/* PLL Multiply by 126 */
#define DEVCFG2_FPLLMULT_MUL_127	0x00007E00	/* PLL Multiply by 127 */
#define DEVCFG2_FPLLMULT_MUL_128	0x00007F00	/* PLL Multiply by 128 */
#define DEVCFG2_FPLLODIV_DIV_2	0x00010000	/* 2x Divider */
#define DEVCFG2_FPLLODIV_DIV_4	0x00020000	/* 4x Divider */
#define DEVCFG2_FPLLODIV_DIV_8	0x00030000	/* 8x Divider */
#define DEVCFG2_FPLLODIV_DIV_16	0x00040000	/* 16x Divider */
#define DEVCFG2_FPLLODIV_DIV_32	0x00050000	/* 32x Divider */
#define DEVCFG2_UPLLFSEL_FREQ_12MHZ	0x00000000	/* USB PLL input is 12 MHz */
#define DEVCFG2_UPLLFSEL_FREQ_24MHZ	0x40000000	/* USB PLL input is 24 MHz */
#define DEVCFG1_FNOSC_SPLL	0x00000001	/* System PLL */
#define DEVCFG1_FNOSC_POSC	0x00000002	/* Primary Osc (HS */
#define DEVCFG1_FNOSC_SOSC	0x00000004	/* Low Power Secondary Osc (SOSC) */
#define DEVCFG1_FNOSC_LPRC	0x00000005	/* Low Power RC Osc (LPRC) */
#define DEVCFG1_FNOSC_FRCDIV	0x00000007	/* Fast RC Osc w/Div-by-N (FRCDIV) */
#define DEVCFG1_DMTINTV_WIN_0	0x00000000	/* Window/Interval value is zero */
#define DEVCFG1_DMTINTV_WIN_1_2	0x00000008	/* Window/Interval value is 1/2 counter value */
#define DEVCFG1_DMTINTV_WIN_3_4	0x00000010	/* Window/Interval value is 3/4 counter value */
#define DEVCFG1_DMTINTV_WIN_7_8	0x00000018	/* Window/Interval value is 7/8 counter value */
#define DEVCFG1_DMTINTV_WIN_15_16	0x00000020	/* Window/Interval value is 15/16 counter value */
#define DEVCFG1_DMTINTV_WIN_31_32	0x00000028	/* Window/Interval value is 31/32 counter value */
#define DEVCFG1_DMTINTV_WIN_63_64	0x00000030	/* Window/Interval value is 63/64 counter value */
#define DEVCFG1_DMTINTV_WIN_127_128	0x00000038	/* Window/Interval value is 127/128 counter value */
#define DEVCFG1_FSOSCEN_OFF	0x00000000	/* Disable SOSC */
#define DEVCFG1_FSOSCEN_ON	0x00000040	/* Enable SOSC */
#define DEVCFG1_IESO_OFF	0x00000000	/* Disabled */
#define DEVCFG1_IESO_ON	0x00000080	/* Enabled */
#define DEVCFG1_POSCMOD_EC	0x00000000	/* External clock mode */
#define DEVCFG1_POSCMOD_HS	0x00000200	/* HS osc mode */
#define DEVCFG1_POSCMOD_OFF	0x00000300	/* Primary osc disabled */
#define DEVCFG1_OSCIOFNC_ON	0x00000000	/* Enabled */
#define DEVCFG1_OSCIOFNC_OFF	0x00000400	/* Disabled */
#define DEVCFG1_FCKSM_CSDCMD	0x00000000	/* Clock Switch Disabled */
#define DEVCFG1_FCKSM_CSECMD	0x00004000	/* Clock Switch Enabled */
#define DEVCFG1_FCKSM_CSDCME	0x00008000	/* Clock Switch Disabled */
#define DEVCFG1_FCKSM_CSECME	0x0000C000	/* Clock Switch Enabled */
#define DEVCFG1_WDTPS_PS1	0x00000000	/* 1 */
#define DEVCFG1_WDTPS_PS2	0x00010000	/* 1 */
#define DEVCFG1_WDTPS_PS4	0x00020000	/* 1 */
#define DEVCFG1_WDTPS_PS8	0x00030000	/* 1 */
#define DEVCFG1_WDTPS_PS16	0x00040000	/* 1 */
#define DEVCFG1_WDTPS_PS32	0x00050000	/* 1 */
#define DEVCFG1_WDTPS_PS64	0x00060000	/* 1 */
#define DEVCFG1_WDTPS_PS128	0x00070000	/* 1 */
#define DEVCFG1_WDTPS_PS256	0x00080000	/* 1 */
#define DEVCFG1_WDTPS_PS512	0x00090000	/* 1 */
#define DEVCFG1_WDTPS_PS1024	0x000A0000	/* 1 */
#define DEVCFG1_WDTPS_PS2048	0x000B0000	/* 1 */
#define DEVCFG1_WDTPS_PS4096	0x000C0000	/* 1 */
#define DEVCFG1_WDTPS_PS8192	0x000D0000	/* 1 */
#define DEVCFG1_WDTPS_PS16384	0x000E0000	/* 1 */
#define DEVCFG1_WDTPS_PS32768	0x000F0000	/* 1 */
#define DEVCFG1_WDTPS_PS65536	0x00100000	/* 1 */
#define DEVCFG1_WDTPS_PS131072	0x00110000	/* 1 */
#define DEVCFG1_WDTPS_PS262144	0x00120000	/* 1 */
#define DEVCFG1_WDTPS_PS524288	0x00130000	/* 1 */
#define DEVCFG1_WDTPS_PS1048576	0x00140000	/* 1 */
#define DEVCFG1_WDTSPGM_RUN	0x00000000	/* WDT runs during Flash programming */
#define DEVCFG1_WDTSPGM_STOP	0x00200000	/* WDT stops during Flash programming */
#define DEVCFG1_WINDIS_WINDOW	0x00000000	/* Watchdog Timer is in Window mode */
#define DEVCFG1_WINDIS_NORMAL	0x00400000	/* Watchdog Timer is in non-Window mode */
#define DEVCFG1_FWDTEN_OFF	0x00000000	/* WDT Disabled */
#define DEVCFG1_FWDTEN_ON	0x00800000	/* WDT Enabled */
#define DEVCFG1_FWDTWINSZ_WINSZ_75	0x00000000	/* Window size is 75% */
#define DEVCFG1_FWDTWINSZ_WINSZ_50	0x01000000	/* Window size is 50% */
#define DEVCFG1_FWDTWINSZ_WINSZ_37	0x02000000	/* Window size is 37.5% */
#define DEVCFG1_FWDTWINSZ_WINSZ_25	0x03000000	/* Window size is 25% */
#define DEVCFG1_DMTCNT_DMT8	0x00000000	/* 2^8 (256) */
#define DEVCFG1_DMTCNT_DMT9	0x04000000	/* 2^9 (512) */
#define DEVCFG1_DMTCNT_DMT10	0x08000000	/* 2^10 (1024) */
#define DEVCFG1_DMTCNT_DMT11	0x0C000000	/* 2^11 (2048) */
#define DEVCFG1_DMTCNT_DMT12	0x10000000	/* 2^12 (4096) */
#define DEVCFG1_DMTCNT_DMT13	0x14000000	/* 2^13 (8192) */
#define DEVCFG1_DMTCNT_DMT14	0x18000000	/* 2^14 (16384) */
#define DEVCFG1_DMTCNT_DMT15	0x1C000000	/* 2^15 (32768) */
#define DEVCFG1_DMTCNT_DMT16	0x20000000	/* 2^16 (65536) */
#define DEVCFG1_DMTCNT_DMT17	0x24000000	/* 2^17 (131072) */
#define DEVCFG1_DMTCNT_DMT18	0x28000000	/* 2^18 (262144) */
#define DEVCFG1_DMTCNT_DMT19	0x2C000000	/* 2^19 (524288) */
#define DEVCFG1_DMTCNT_DMT20	0x30000000	/* 2^20 (1048576) */
#define DEVCFG1_DMTCNT_DMT21	0x34000000	/* 2^21 (2097152) */
#define DEVCFG1_DMTCNT_DMT22	0x38000000	/* 2^22 (4194304) */
#define DEVCFG1_DMTCNT_DMT23	0x3C000000	/* 2^23 (8388608) */
#define DEVCFG1_DMTCNT_DMT24	0x40000000	/* 2^24 (16777216) */
#define DEVCFG1_DMTCNT_DMT25	0x44000000	/* 2^25 (33554432) */
#define DEVCFG1_DMTCNT_DMT26	0x48000000	/* 2^26 (67108864) */
#define DEVCFG1_DMTCNT_DMT27	0x4C000000	/* 2^27 (134217728) */
#define DEVCFG1_DMTCNT_DMT28	0x50000000	/* 2^28 (268435456) */
#define DEVCFG1_DMTCNT_DMT29	0x54000000	/* 2^29 (536870912) */
#define DEVCFG1_DMTCNT_DMT30	0x58000000	/* 2^30 (1073741824) */
#define DEVCFG1_DMTCNT_DMT31	0x5C000000	/* 2^31 (2147483648) */
#define DEVCFG1_FDMTEN_OFF	0x00000000	/* Deadman Timer is disabled */
#define DEVCFG1_FDMTEN_ON	0x80000000	/* Deadman Timer is enabled */
#define DEVCFG0_DEBUG_ON	0x00000001	/* Debugger is enabled */
#define DEVCFG0_DEBUG_OFF	0x00000003	/* Debugger is disabled */
#define DEVCFG0_JTAGEN_OFF	0x00000000	/* JTAG Disabled */
#define DEVCFG0_JTAGEN_ON	0x00000004	/* JTAG Port Enabled */
#define DEVCFG0_ICESEL_ICS_PGx2	0x00000010	/* Communicate on PGEC2/PGED2 */
#define DEVCFG0_ICESEL_ICS_PGx1	0x00000018	/* Communicate on PGEC1/PGED1 */
#define DEVCFG0_TRCEN_OFF	0x00000000	/* Trace features in the CPU are disabled */
#define DEVCFG0_TRCEN_ON	0x00000020	/* Trace features in the CPU are enabled */
#define DEVCFG0_BOOTISA_MICROMIPS	0x00000000	/* Boot code and Exception code is microMIPS */
#define DEVCFG0_BOOTISA_MIPS32	0x00000040	/* Boot code and Exception code is MIPS32 */
#define DEVCFG0_FECCCON_ON	0x00000000	/* Flash ECC is enabled (ECCCON bits are locked) */
#define DEVCFG0_FECCCON_DYNAMIC	0x00000100	/* Dynamic Flash ECC is enabled (ECCCON bits are locked) */
#define DEVCFG0_FECCCON_OFF_LOCKED	0x00000200	/* ECC and Dynamic ECC are disabled (ECCCON bits are locked) */
#define DEVCFG0_FECCCON_OFF_UNLOCKED	0x00000300	/* ECC and Dynamic ECC are disabled (ECCCON bits are writable) */
#define DEVCFG0_FSLEEP_VREGS	0x00000000	/* Flash power down is controlled by the VREGS bit */
#define DEVCFG0_FSLEEP_OFF	0x00000400	/* Flash is powered down when the device is in Sleep mode */
#define DEVCFG0_DBGPER_DENY_PG0	0x00000000	/* Deny CPU access to Permission Group 0 permission regions */
#define DEVCFG0_DBGPER_DENY_PG1	0x00000000	/* Deny CPU access to Permission Group 1 permission regions */
#define DEVCFG0_DBGPER_DENY_PG2	0x00000000	/* Deny CPU access to Permission Group 2 permission regions */
#define DEVCFG0_DBGPER_ALLOW_PG0	0x00001000	/* Allow CPU access to Permission Group 0 permission regions */
#define DEVCFG0_DBGPER_ALLOW_PG1	0x00002000	/* Allow CPU access to Permission Group 1 permission regions */
#define DEVCFG0_DBGPER_ALLOW_PG2	0x00004000	/* Allow CPU access to Permission Group 2 permission regions */
#define DEVCFG0_DBGPER_PG_2_1	0x00006000	/* PG0 */
#define DEVCFG0_DBGPER_PG_ALL	0x00007000	/* Allow CPU access to all permission regions */
#define DEVCFG0_SMCLR_MCLR_POR	0x00000000	/* MCLR pin generates an emulated POR Reset */
#define DEVCFG0_SMCLR_MCLR_NORM	0x00008000	/* MCLR pin generates a normal system Reset */
#define DEVCFG0_SOSCGAIN_GAIN_1X	0x00000000	/* 1x gain setting */
#define DEVCFG0_SOSCGAIN_GAIN_0_5X	0x00010000	/* 0.5x gain setting */
#define DEVCFG0_SOSCGAIN_GAIN_1_5X	0x00020000	/* 1.5x gain setting */
#define DEVCFG0_SOSCGAIN_GAIN_2X	0x00030000	/* 2x gain setting */
#define DEVCFG0_SOSCBOOST_OFF	0x00000000	/* Normal start of the oscillator */
#define DEVCFG0_SOSCBOOST_ON	0x00040000	/* Boost the kick start of the oscillator */
#define DEVCFG0_POSCGAIN_GAIN_1X	0x00000000	/* 1x gain setting */
#define DEVCFG0_POSCGAIN_GAIN_0_5X	0x00080000	/* 0.5x gain setting */
#define DEVCFG0_POSCGAIN_GAIN_1_5X	0x00100000	/* 1.5x gain setting */
#define DEVCFG0_POSCGAIN_GAIN_2X	0x00180000	/* 2x gain setting */
#define DEVCFG0_POSCBOOST_OFF	0x00000000	/* Normal start of the oscillator */
#define DEVCFG0_POSCBOOST_ON	0x00200000	/* Boost the kick start of the oscillator */
#define DEVCFG0_EJTAGBEN_REDUCED	0x00000000	/* Reduced EJTAG functionality */
#define DEVCFG0_EJTAGBEN_NORMAL	0x40000000	/* Normal EJTAG functionality */

/*--------------------------------------
 * Peripheral registers.
 */
#define PIC32_R(a)              *(volatile unsigned*)(0xBF800000 + (a))

/*--------------------------------------
 * Port A-K registers.
 */
#define ANSELA          PIC32_R (0x60000) /* Port A: analog select */
#define ANSELACLR       PIC32_R (0x60004)
#define ANSELASET       PIC32_R (0x60008)
#define ANSELAINV       PIC32_R (0x6000C)
#define TRISA           PIC32_R (0x60010) /* Port A: mask of inputs */
#define TRISACLR        PIC32_R (0x60014)
#define TRISASET        PIC32_R (0x60018)
#define TRISAINV        PIC32_R (0x6001C)
#define PORTA           PIC32_R (0x60020) /* Port A: read inputs, write outputs */
#define PORTACLR        PIC32_R (0x60024)
#define PORTASET        PIC32_R (0x60028)
#define PORTAINV        PIC32_R (0x6002C)
#define LATA            PIC32_R (0x60030) /* Port A: read/write outputs */
#define LATACLR         PIC32_R (0x60034)
#define LATASET         PIC32_R (0x60038)
#define LATAINV         PIC32_R (0x6003C)
#define ODCA            PIC32_R (0x60040) /* Port A: open drain configuration */
#define ODCACLR         PIC32_R (0x60044)
#define ODCASET         PIC32_R (0x60048)
#define ODCAINV         PIC32_R (0x6004C)
#define CNPUA           PIC32_R (0x60050) /* Port A: input pin pull-up enable */
#define CNPUACLR        PIC32_R (0x60054)
#define CNPUASET        PIC32_R (0x60058)
#define CNPUAINV        PIC32_R (0x6005C)
#define CNPDA           PIC32_R (0x60060) /* Port A: input pin pull-down enable */
#define CNPDACLR        PIC32_R (0x60064)
#define CNPDASET        PIC32_R (0x60068)
#define CNPDAINV        PIC32_R (0x6006C)
#define CNCONA          PIC32_R (0x60070) /* Port A: interrupt-on-change control */
#define CNCONACLR       PIC32_R (0x60074)
#define CNCONASET       PIC32_R (0x60078)
#define CNCONAINV       PIC32_R (0x6007C)
#define CNENA           PIC32_R (0x60080) /* Port A: input change interrupt enable */
#define CNENACLR        PIC32_R (0x60084)
#define CNENASET        PIC32_R (0x60088)
#define CNENAINV        PIC32_R (0x6008C)
#define CNSTATA         PIC32_R (0x60090) /* Port A: status */
#define CNSTATACLR      PIC32_R (0x60094)
#define CNSTATASET      PIC32_R (0x60098)
#define CNSTATAINV      PIC32_R (0x6009C)

#define ANSELB          PIC32_R (0x60100) /* Port B: analog select */
#define ANSELBCLR       PIC32_R (0x60104)
#define ANSELBSET       PIC32_R (0x60108)
#define ANSELBINV       PIC32_R (0x6010C)
#define TRISB           PIC32_R (0x60110) /* Port B: mask of inputs */
#define TRISBCLR        PIC32_R (0x60114)
#define TRISBSET        PIC32_R (0x60118)
#define TRISBINV        PIC32_R (0x6011C)
#define PORTB           PIC32_R (0x60120) /* Port B: read inputs, write outputs */
#define PORTBCLR        PIC32_R (0x60124)
#define PORTBSET        PIC32_R (0x60128)
#define PORTBINV        PIC32_R (0x6012C)
#define LATB            PIC32_R (0x60130) /* Port B: read/write outputs */
#define LATBCLR         PIC32_R (0x60134)
#define LATBSET         PIC32_R (0x60138)
#define LATBINV         PIC32_R (0x6013C)
#define ODCB            PIC32_R (0x60140) /* Port B: open drain configuration */
#define ODCBCLR         PIC32_R (0x60144)
#define ODCBSET         PIC32_R (0x60148)
#define ODCBINV         PIC32_R (0x6014C)
#define CNPUB           PIC32_R (0x60150) /* Port B: input pin pull-up enable */
#define CNPUBCLR        PIC32_R (0x60154)
#define CNPUBSET        PIC32_R (0x60158)
#define CNPUBINV        PIC32_R (0x6015C)
#define CNPDB           PIC32_R (0x60160) /* Port B: input pin pull-down enable */
#define CNPDBCLR        PIC32_R (0x60164)
#define CNPDBSET        PIC32_R (0x60168)
#define CNPDBINV        PIC32_R (0x6016C)
#define CNCONB          PIC32_R (0x60170) /* Port B: interrupt-on-change control */
#define CNCONBCLR       PIC32_R (0x60174)
#define CNCONBSET       PIC32_R (0x60178)
#define CNCONBINV       PIC32_R (0x6017C)
#define CNENB           PIC32_R (0x60180) /* Port B: input change interrupt enable */
#define CNENBCLR        PIC32_R (0x60184)
#define CNENBSET        PIC32_R (0x60188)
#define CNENBINV        PIC32_R (0x6018C)
#define CNSTATB         PIC32_R (0x60190) /* Port B: status */
#define CNSTATBCLR      PIC32_R (0x60194)
#define CNSTATBSET      PIC32_R (0x60198)
#define CNSTATBINV      PIC32_R (0x6019C)

#define ANSELC          PIC32_R (0x60200) /* Port C: analog select */
#define ANSELCCLR       PIC32_R (0x60204)
#define ANSELCSET       PIC32_R (0x60208)
#define ANSELCINV       PIC32_R (0x6020C)
#define TRISC           PIC32_R (0x60210) /* Port C: mask of inputs */
#define TRISCCLR        PIC32_R (0x60214)
#define TRISCSET        PIC32_R (0x60218)
#define TRISCINV        PIC32_R (0x6021C)
#define PORTC           PIC32_R (0x60220) /* Port C: read inputs, write outputs */
#define PORTCCLR        PIC32_R (0x60224)
#define PORTCSET        PIC32_R (0x60228)
#define PORTCINV        PIC32_R (0x6022C)
#define LATC            PIC32_R (0x60230) /* Port C: read/write outputs */
#define LATCCLR         PIC32_R (0x60234)
#define LATCSET         PIC32_R (0x60238)
#define LATCINV         PIC32_R (0x6023C)
#define ODCC            PIC32_R (0x60240) /* Port C: open drain configuration */
#define ODCCCLR         PIC32_R (0x60244)
#define ODCCSET         PIC32_R (0x60248)
#define ODCCINV         PIC32_R (0x6024C)
#define CNPUC           PIC32_R (0x60250) /* Port C: input pin pull-up enable */
#define CNPUCCLR        PIC32_R (0x60254)
#define CNPUCSET        PIC32_R (0x60258)
#define CNPUCINV        PIC32_R (0x6025C)
#define CNPDC           PIC32_R (0x60260) /* Port C: input pin pull-down enable */
#define CNPDCCLR        PIC32_R (0x60264)
#define CNPDCSET        PIC32_R (0x60268)
#define CNPDCINV        PIC32_R (0x6026C)
#define CNCONC          PIC32_R (0x60270) /* Port C: interrupt-on-change control */
#define CNCONCCLR       PIC32_R (0x60274)
#define CNCONCSET       PIC32_R (0x60278)
#define CNCONCINV       PIC32_R (0x6027C)
#define CNENC           PIC32_R (0x60280) /* Port C: input change interrupt enable */
#define CNENCCLR        PIC32_R (0x60284)
#define CNENCSET        PIC32_R (0x60288)
#define CNENCINV        PIC32_R (0x6028C)
#define CNSTATC         PIC32_R (0x60290) /* Port C: status */
#define CNSTATCCLR      PIC32_R (0x60294)
#define CNSTATCSET      PIC32_R (0x60298)
#define CNSTATCINV      PIC32_R (0x6029C)

#define ANSELD          PIC32_R (0x60300) /* Port D: analog select */
#define ANSELDCLR       PIC32_R (0x60304)
#define ANSELDSET       PIC32_R (0x60308)
#define ANSELDINV       PIC32_R (0x6030C)
#define TRISD           PIC32_R (0x60310) /* Port D: mask of inputs */
#define TRISDCLR        PIC32_R (0x60314)
#define TRISDSET        PIC32_R (0x60318)
#define TRISDINV        PIC32_R (0x6031C)
#define PORTD           PIC32_R (0x60320) /* Port D: read inputs, write outputs */
#define PORTDCLR        PIC32_R (0x60324)
#define PORTDSET        PIC32_R (0x60328)
#define PORTDINV        PIC32_R (0x6032C)
#define LATD            PIC32_R (0x60330) /* Port D: read/write outputs */
#define LATDCLR         PIC32_R (0x60334)
#define LATDSET         PIC32_R (0x60338)
#define LATDINV         PIC32_R (0x6033C)
#define ODCD            PIC32_R (0x60340) /* Port D: open drain configuration */
#define ODCDCLR         PIC32_R (0x60344)
#define ODCDSET         PIC32_R (0x60348)
#define ODCDINV         PIC32_R (0x6034C)
#define CNPUD           PIC32_R (0x60350) /* Port D: input pin pull-up enable */
#define CNPUDCLR        PIC32_R (0x60354)
#define CNPUDSET        PIC32_R (0x60358)
#define CNPUDINV        PIC32_R (0x6035C)
#define CNPDD           PIC32_R (0x60360) /* Port D: input pin pull-down enable */
#define CNPDDCLR        PIC32_R (0x60364)
#define CNPDDSET        PIC32_R (0x60368)
#define CNPDDINV        PIC32_R (0x6036C)
#define CNCOND          PIC32_R (0x60370) /* Port D: interrupt-on-change control */
#define CNCONDCLR       PIC32_R (0x60374)
#define CNCONDSET       PIC32_R (0x60378)
#define CNCONDINV       PIC32_R (0x6037C)
#define CNEND           PIC32_R (0x60380) /* Port D: input change interrupt enable */
#define CNENDCLR        PIC32_R (0x60384)
#define CNENDSET        PIC32_R (0x60388)
#define CNENDINV        PIC32_R (0x6038C)
#define CNSTATD         PIC32_R (0x60390) /* Port D: status */
#define CNSTATDCLR      PIC32_R (0x60394)
#define CNSTATDSET      PIC32_R (0x60398)
#define CNSTATDINV      PIC32_R (0x6039C)

#define ANSELE          PIC32_R (0x60400) /* Port E: analog select */
#define ANSELECLR       PIC32_R (0x60404)
#define ANSELESET       PIC32_R (0x60408)
#define ANSELEINV       PIC32_R (0x6040C)
#define TRISE           PIC32_R (0x60410) /* Port E: mask of inputs */
#define TRISECLR        PIC32_R (0x60414)
#define TRISESET        PIC32_R (0x60418)
#define TRISEINV        PIC32_R (0x6041C)
#define PORTE           PIC32_R (0x60420) /* Port E: read inputs, write outputs */
#define PORTECLR        PIC32_R (0x60424)
#define PORTESET        PIC32_R (0x60428)
#define PORTEINV        PIC32_R (0x6042C)
#define LATE            PIC32_R (0x60430) /* Port E: read/write outputs */
#define LATECLR         PIC32_R (0x60434)
#define LATESET         PIC32_R (0x60438)
#define LATEINV         PIC32_R (0x6043C)
#define ODCE            PIC32_R (0x60440) /* Port E: open drain configuration */
#define ODCECLR         PIC32_R (0x60444)
#define ODCESET         PIC32_R (0x60448)
#define ODCEINV         PIC32_R (0x6044C)
#define CNPUE           PIC32_R (0x60450) /* Port E: input pin pull-up enable */
#define CNPUECLR        PIC32_R (0x60454)
#define CNPUESET        PIC32_R (0x60458)
#define CNPUEINV        PIC32_R (0x6045C)
#define CNPDE           PIC32_R (0x60460) /* Port E: input pin pull-down enable */
#define CNPDECLR        PIC32_R (0x60464)
#define CNPDESET        PIC32_R (0x60468)
#define CNPDEINV        PIC32_R (0x6046C)
#define CNCONE          PIC32_R (0x60470) /* Port E: interrupt-on-change control */
#define CNCONECLR       PIC32_R (0x60474)
#define CNCONESET       PIC32_R (0x60478)
#define CNCONEINV       PIC32_R (0x6047C)
#define CNENE           PIC32_R (0x60480) /* Port E: input change interrupt enable */
#define CNENECLR        PIC32_R (0x60484)
#define CNENESET        PIC32_R (0x60488)
#define CNENEINV        PIC32_R (0x6048C)
#define CNSTATE         PIC32_R (0x60490) /* Port E: status */
#define CNSTATECLR      PIC32_R (0x60494)
#define CNSTATESET      PIC32_R (0x60498)
#define CNSTATEINV      PIC32_R (0x6049C)

#define ANSELF          PIC32_R (0x60500) /* Port F: analog select */
#define ANSELFCLR       PIC32_R (0x60504)
#define ANSELFSET       PIC32_R (0x60508)
#define ANSELFINV       PIC32_R (0x6050C)
#define TRISF           PIC32_R (0x60510) /* Port F: mask of inputs */
#define TRISFCLR        PIC32_R (0x60514)
#define TRISFSET        PIC32_R (0x60518)
#define TRISFINV        PIC32_R (0x6051C)
#define PORTF           PIC32_R (0x60520) /* Port F: read inputs, write outputs */
#define PORTFCLR        PIC32_R (0x60524)
#define PORTFSET        PIC32_R (0x60528)
#define PORTFINV        PIC32_R (0x6052C)
#define LATF            PIC32_R (0x60530) /* Port F: read/write outputs */
#define LATFCLR         PIC32_R (0x60534)
#define LATFSET         PIC32_R (0x60538)
#define LATFINV         PIC32_R (0x6053C)
#define ODCF            PIC32_R (0x60540) /* Port F: open drain configuration */
#define ODCFCLR         PIC32_R (0x60544)
#define ODCFSET         PIC32_R (0x60548)
#define ODCFINV         PIC32_R (0x6054C)
#define CNPUF           PIC32_R (0x60550) /* Port F: input pin pull-up enable */
#define CNPUFCLR        PIC32_R (0x60554)
#define CNPUFSET        PIC32_R (0x60558)
#define CNPUFINV        PIC32_R (0x6055C)
#define CNPDF           PIC32_R (0x60560) /* Port F: input pin pull-down enable */
#define CNPDFCLR        PIC32_R (0x60564)
#define CNPDFSET        PIC32_R (0x60568)
#define CNPDFINV        PIC32_R (0x6056C)
#define CNCONF          PIC32_R (0x60570) /* Port F: interrupt-on-change control */
#define CNCONFCLR       PIC32_R (0x60574)
#define CNCONFSET       PIC32_R (0x60578)
#define CNCONFINV       PIC32_R (0x6057C)
#define CNENF           PIC32_R (0x60580) /* Port F: input change interrupt enable */
#define CNENFCLR        PIC32_R (0x60584)
#define CNENFSET        PIC32_R (0x60588)
#define CNENFINV        PIC32_R (0x6058C)
#define CNSTATF         PIC32_R (0x60590) /* Port F: status */
#define CNSTATFCLR      PIC32_R (0x60594)
#define CNSTATFSET      PIC32_R (0x60598)
#define CNSTATFINV      PIC32_R (0x6059C)

#define ANSELG          PIC32_R (0x60600) /* Port G: analog select */
#define ANSELGCLR       PIC32_R (0x60604)
#define ANSELGSET       PIC32_R (0x60608)
#define ANSELGINV       PIC32_R (0x6060C)
#define TRISG           PIC32_R (0x60610) /* Port G: mask of inputs */
#define TRISGCLR        PIC32_R (0x60614)
#define TRISGSET        PIC32_R (0x60618)
#define TRISGINV        PIC32_R (0x6061C)
#define PORTG           PIC32_R (0x60620) /* Port G: read inputs, write outputs */
#define PORTGCLR        PIC32_R (0x60624)
#define PORTGSET        PIC32_R (0x60628)
#define PORTGINV        PIC32_R (0x6062C)
#define LATG            PIC32_R (0x60630) /* Port G: read/write outputs */
#define LATGCLR         PIC32_R (0x60634)
#define LATGSET         PIC32_R (0x60638)
#define LATGINV         PIC32_R (0x6063C)
#define ODCG            PIC32_R (0x60640) /* Port G: open drain configuration */
#define ODCGCLR         PIC32_R (0x60644)
#define ODCGSET         PIC32_R (0x60648)
#define ODCGINV         PIC32_R (0x6064C)
#define CNPUG           PIC32_R (0x60650) /* Port G: input pin pull-up enable */
#define CNPUGCLR        PIC32_R (0x60654)
#define CNPUGSET        PIC32_R (0x60658)
#define CNPUGINV        PIC32_R (0x6065C)
#define CNPDG           PIC32_R (0x60660) /* Port G: input pin pull-down enable */
#define CNPDGCLR        PIC32_R (0x60664)
#define CNPDGSET        PIC32_R (0x60668)
#define CNPDGINV        PIC32_R (0x6066C)
#define CNCONG          PIC32_R (0x60670) /* Port G: interrupt-on-change control */
#define CNCONGCLR       PIC32_R (0x60674)
#define CNCONGSET       PIC32_R (0x60678)
#define CNCONGINV       PIC32_R (0x6067C)
#define CNENG           PIC32_R (0x60680) /* Port G: input change interrupt enable */
#define CNENGCLR        PIC32_R (0x60684)
#define CNENGSET        PIC32_R (0x60688)
#define CNENGINV        PIC32_R (0x6068C)
#define CNSTATG         PIC32_R (0x60690) /* Port G: status */
#define CNSTATGCLR      PIC32_R (0x60694)
#define CNSTATGSET      PIC32_R (0x60698)
#define CNSTATGINV      PIC32_R (0x6069C)

#define ANSELH          PIC32_R (0x60700) /* Port H: analog select */
#define ANSELHCLR       PIC32_R (0x60704)
#define ANSELHSET       PIC32_R (0x60708)
#define ANSELHINV       PIC32_R (0x6070C)
#define TRISH           PIC32_R (0x60710) /* Port H: mask of inputs */
#define TRISHCLR        PIC32_R (0x60714)
#define TRISHSET        PIC32_R (0x60718)
#define TRISHINV        PIC32_R (0x6071C)
#define PORTH           PIC32_R (0x60720) /* Port H: read inputs, write outputs */
#define PORTHCLR        PIC32_R (0x60724)
#define PORTHSET        PIC32_R (0x60728)
#define PORTHINV        PIC32_R (0x6072C)
#define LATH            PIC32_R (0x60730) /* Port H: read/write outputs */
#define LATHCLR         PIC32_R (0x60734)
#define LATHSET         PIC32_R (0x60738)
#define LATHINV         PIC32_R (0x6073C)
#define ODCH            PIC32_R (0x60740) /* Port H: open drain configuration */
#define ODCHCLR         PIC32_R (0x60744)
#define ODCHSET         PIC32_R (0x60748)
#define ODCHINV         PIC32_R (0x6074C)
#define CNPUH           PIC32_R (0x60750) /* Port H: input pin pull-up enable */
#define CNPUHCLR        PIC32_R (0x60754)
#define CNPUHSET        PIC32_R (0x60758)
#define CNPUHINV        PIC32_R (0x6075C)
#define CNPDH           PIC32_R (0x60760) /* Port H: input pin pull-down enable */
#define CNPDHCLR        PIC32_R (0x60764)
#define CNPDHSET        PIC32_R (0x60768)
#define CNPDHINV        PIC32_R (0x6076C)
#define CNCONH          PIC32_R (0x60770) /* Port H: interrupt-on-change control */
#define CNCONHCLR       PIC32_R (0x60774)
#define CNCONHSET       PIC32_R (0x60778)
#define CNCONHINV       PIC32_R (0x6077C)
#define CNENH           PIC32_R (0x60780) /* Port H: input change interrupt enable */
#define CNENHCLR        PIC32_R (0x60784)
#define CNENHSET        PIC32_R (0x60788)
#define CNENHINV        PIC32_R (0x6078C)
#define CNSTATH         PIC32_R (0x60790) /* Port H: status */
#define CNSTATHCLR      PIC32_R (0x60794)
#define CNSTATHSET      PIC32_R (0x60798)
#define CNSTATHINV      PIC32_R (0x6079C)

#define ANSELJ          PIC32_R (0x60800) /* Port J: analog select */
#define ANSELJCLR       PIC32_R (0x60804)
#define ANSELJSET       PIC32_R (0x60808)
#define ANSELJINV       PIC32_R (0x6080C)
#define TRISJ           PIC32_R (0x60810) /* Port J: mask of inputs */
#define TRISJCLR        PIC32_R (0x60814)
#define TRISJSET        PIC32_R (0x60818)
#define TRISJINV        PIC32_R (0x6081C)
#define PORTJ           PIC32_R (0x60820) /* Port J: read inputs, write outputs */
#define PORTJCLR        PIC32_R (0x60824)
#define PORTJSET        PIC32_R (0x60828)
#define PORTJINV        PIC32_R (0x6082C)
#define LATJ            PIC32_R (0x60830) /* Port J: read/write outputs */
#define LATJCLR         PIC32_R (0x60834)
#define LATJSET         PIC32_R (0x60838)
#define LATJINV         PIC32_R (0x6083C)
#define ODCJ            PIC32_R (0x60840) /* Port J: open drain configuration */
#define ODCJCLR         PIC32_R (0x60844)
#define ODCJSET         PIC32_R (0x60848)
#define ODCJINV         PIC32_R (0x6084C)
#define CNPUJ           PIC32_R (0x60850) /* Port J: input pin pull-up enable */
#define CNPUJCLR        PIC32_R (0x60854)
#define CNPUJSET        PIC32_R (0x60858)
#define CNPUJINV        PIC32_R (0x6085C)
#define CNPDJ           PIC32_R (0x60860) /* Port J: input pin pull-down enable */
#define CNPDJCLR        PIC32_R (0x60864)
#define CNPDJSET        PIC32_R (0x60868)
#define CNPDJINV        PIC32_R (0x6086C)
#define CNCONJ          PIC32_R (0x60870) /* Port J: interrupt-on-change control */
#define CNCONJCLR       PIC32_R (0x60874)
#define CNCONJSET       PIC32_R (0x60878)
#define CNCONJINV       PIC32_R (0x6087C)
#define CNENJ           PIC32_R (0x60880) /* Port J: input change interrupt enable */
#define CNENJCLR        PIC32_R (0x60884)
#define CNENJSET        PIC32_R (0x60888)
#define CNENJINV        PIC32_R (0x6088C)
#define CNSTATJ         PIC32_R (0x60890) /* Port J: status */
#define CNSTATJCLR      PIC32_R (0x60894)
#define CNSTATJSET      PIC32_R (0x60898)
#define CNSTATJINV      PIC32_R (0x6089C)

#define TRISK           PIC32_R (0x60910) /* Port K: mask of inputs */
#define TRISKCLR        PIC32_R (0x60914)
#define TRISKSET        PIC32_R (0x60918)
#define TRISKINV        PIC32_R (0x6091C)
#define PORTK           PIC32_R (0x60920) /* Port K: read inputs, write outputs */
#define PORTKCLR        PIC32_R (0x60924)
#define PORTKSET        PIC32_R (0x60928)
#define PORTKINV        PIC32_R (0x6092C)
#define LATK            PIC32_R (0x60930) /* Port K: read/write outputs */
#define LATKCLR         PIC32_R (0x60934)
#define LATKSET         PIC32_R (0x60938)
#define LATKINV         PIC32_R (0x6093C)
#define ODCK            PIC32_R (0x60940) /* Port K: open drain configuration */
#define ODCKCLR         PIC32_R (0x60944)
#define ODCKSET         PIC32_R (0x60948)
#define ODCKINV         PIC32_R (0x6094C)
#define CNPUK           PIC32_R (0x60950) /* Port K: input pin pull-up enable */
#define CNPUKCLR        PIC32_R (0x60954)
#define CNPUKSET        PIC32_R (0x60958)
#define CNPUKINV        PIC32_R (0x6095C)
#define CNPDK           PIC32_R (0x60960) /* Port K: input pin pull-down enable */
#define CNPDKCLR        PIC32_R (0x60964)
#define CNPDKSET        PIC32_R (0x60968)
#define CNPDKINV        PIC32_R (0x6096C)
#define CNCONK          PIC32_R (0x60970) /* Port K: interrupt-on-change control */
#define CNCONKCLR       PIC32_R (0x60974)
#define CNCONKSET       PIC32_R (0x60978)
#define CNCONKINV       PIC32_R (0x6097C)
#define CNENK           PIC32_R (0x60980) /* Port K: input change interrupt enable */
#define CNENKCLR        PIC32_R (0x60984)
#define CNENKSET        PIC32_R (0x60988)
#define CNENKINV        PIC32_R (0x6098C)
#define CNSTATK         PIC32_R (0x60990) /* Port K: status */
#define CNSTATKCLR      PIC32_R (0x60994)
#define CNSTATKSET      PIC32_R (0x60998)
#define CNSTATKINV      PIC32_R (0x6099C)

/*--------------------------------------
 * Timer registers.
 */
#define T1CON           PIC32_R (0x40000) /* Timer 1: Control */
#define T1CONCLR        PIC32_R (0x40004)
#define T1CONSET        PIC32_R (0x40008)
#define T1CONINV        PIC32_R (0x4000C)
#define TMR1            PIC32_R (0x40010) /* Timer 1: Count */
#define TMR1CLR         PIC32_R (0x40014)
#define TMR1SET         PIC32_R (0x40018)
#define TMR1INV         PIC32_R (0x4001C)
#define PR1             PIC32_R (0x40020) /* Timer 1: Period register */
#define PR1CLR          PIC32_R (0x40024)
#define PR1SET          PIC32_R (0x40028)
#define PR1INV          PIC32_R (0x4002C)
#define T2CON           PIC32_R (0x40200) /* Timer 2: Control */
#define T2CONCLR        PIC32_R (0x40204)
#define T2CONSET        PIC32_R (0x40208)
#define T2CONINV        PIC32_R (0x4020C)
#define TMR2            PIC32_R (0x40210) /* Timer 2: Count */
#define TMR2CLR         PIC32_R (0x40214)
#define TMR2SET         PIC32_R (0x40218)
#define TMR2INV         PIC32_R (0x4021C)
#define PR2             PIC32_R (0x40220) /* Timer 2: Period register */
#define PR2CLR          PIC32_R (0x40224)
#define PR2SET          PIC32_R (0x40228)
#define PR2INV          PIC32_R (0x4022C)
#define T3CON           PIC32_R (0x40400) /* Timer 3: Control */
#define T3CONCLR        PIC32_R (0x40404)
#define T3CONSET        PIC32_R (0x40408)
#define T3CONINV        PIC32_R (0x4040C)
#define TMR3            PIC32_R (0x40410) /* Timer 3: Count */
#define TMR3CLR         PIC32_R (0x40414)
#define TMR3SET         PIC32_R (0x40418)
#define TMR3INV         PIC32_R (0x4041C)
#define PR3             PIC32_R (0x40420) /* Timer 3: Period register */
#define PR3CLR          PIC32_R (0x40424)
#define PR3SET          PIC32_R (0x40428)
#define PR3INV          PIC32_R (0x4042C)
#define T4CON           PIC32_R (0x40600) /* Timer 4: Control */
#define T4CONCLR        PIC32_R (0x40604)
#define T4CONSET        PIC32_R (0x40608)
#define T4CONINV        PIC32_R (0x4060C)
#define TMR4            PIC32_R (0x40610) /* Timer 4: Count */
#define TMR4CLR         PIC32_R (0x40614)
#define TMR4SET         PIC32_R (0x40618)
#define TMR4INV         PIC32_R (0x4061C)
#define PR4             PIC32_R (0x40620) /* Timer 4: Period register */
#define PR4CLR          PIC32_R (0x40624)
#define PR4SET          PIC32_R (0x40628)
#define PR4INV          PIC32_R (0x4062C)
#define T5CON           PIC32_R (0x40800) /* Timer 5: Control */
#define T5CONCLR        PIC32_R (0x40804)
#define T5CONSET        PIC32_R (0x40808)
#define T5CONINV        PIC32_R (0x4080C)
#define TMR5            PIC32_R (0x40810) /* Timer 5: Count */
#define TMR5CLR         PIC32_R (0x40814)
#define TMR5SET         PIC32_R (0x40818)
#define TMR5INV         PIC32_R (0x4081C)
#define PR5             PIC32_R (0x40820) /* Timer 5: Period register */
#define PR5CLR          PIC32_R (0x40824)
#define PR5SET          PIC32_R (0x40828)
#define PR5INV          PIC32_R (0x4082C)
#define T6CON           PIC32_R (0x40A00) /* Timer 6: Control */
#define T6CONCLR        PIC32_R (0x40A04)
#define T6CONSET        PIC32_R (0x40A08)
#define T6CONINV        PIC32_R (0x40A0C)
#define TMR6            PIC32_R (0x40A10) /* Timer 6: Count */
#define TMR6CLR         PIC32_R (0x40A14)
#define TMR6SET         PIC32_R (0x40A18)
#define TMR6INV         PIC32_R (0x40A1C)
#define PR6             PIC32_R (0x40A20) /* Timer 6: Period register */
#define PR6CLR          PIC32_R (0x40A24)
#define PR6SET          PIC32_R (0x40A28)
#define PR6INV          PIC32_R (0x40A2C)
#define T7CON           PIC32_R (0x40C00) /* Timer 7: Control */
#define T7CONCLR        PIC32_R (0x40C04)
#define T7CONSET        PIC32_R (0x40C08)
#define T7CONINV        PIC32_R (0x40C0C)
#define TMR7            PIC32_R (0x40C10) /* Timer 7: Count */
#define TMR7CLR         PIC32_R (0x40C14)
#define TMR7SET         PIC32_R (0x40C18)
#define TMR7INV         PIC32_R (0x40C1C)
#define PR7             PIC32_R (0x40C20) /* Timer 7: Period register */
#define PR7CLR          PIC32_R (0x40C24)
#define PR7SET          PIC32_R (0x40C28)
#define PR7INV          PIC32_R (0x40C2C)
#define T8CON           PIC32_R (0x40E00) /* Timer 8: Control */
#define T8CONCLR        PIC32_R (0x40E04)
#define T8CONSET        PIC32_R (0x40E08)
#define T8CONINV        PIC32_R (0x40E0C)
#define TMR8            PIC32_R (0x40E10) /* Timer 8: Count */
#define TMR8CLR         PIC32_R (0x40E14)
#define TMR8SET         PIC32_R (0x40E18)
#define TMR8INV         PIC32_R (0x40E1C)
#define PR8             PIC32_R (0x40E20) /* Timer 8: Period register */
#define PR8CLR          PIC32_R (0x40E24)
#define PR8SET          PIC32_R (0x40E28)
#define PR8INV          PIC32_R (0x40E2C)
#define T9CON           PIC32_R (0x41000) /* Timer 9: Control */
#define T9CONCLR        PIC32_R (0x41004)
#define T9CONSET        PIC32_R (0x41008)
#define T9CONINV        PIC32_R (0x4100C)
#define TMR9            PIC32_R (0x41010) /* Timer 9: Count */
#define TMR9CLR         PIC32_R (0x41014)
#define TMR9SET         PIC32_R (0x41018)
#define TMR9INV         PIC32_R (0x4101C)
#define PR9             PIC32_R (0x41020) /* Timer 9: Period register */
#define PR9CLR          PIC32_R (0x41024)
#define PR9SET          PIC32_R (0x41028)
#define PR9INV          PIC32_R (0x4102C)

#define TCON(n)         *(&T1CON + (n - 1) * (&T2CON - &T1CON))
#define TCONCLR(n)      *(&T1CONCLR + (n - 1) * (&T2CONCLR - &T1CONCLR))
#define TCONSET(n)      *(&T1CONSET + (n - 1) * (&T2CONSET - &T1CONSET))
#define TCONINV(n)      *(&T1CONINV + (n - 1) * (&T2CONINV - &T1CONINV))

#define TMR(n)         *(&TMR1 + (n - 1) * (&TMR2 - &TMR1))
#define TMRCLR(n)      *(&TMR1CLR + (n - 1) * (&TMR2CLR - &TMR1CLR))
#define TMRSET(n)      *(&TMR1SET + (n - 1) * (&TMR2SET - &TMR1SET))
#define TMRINV(n)      *(&TMR1INV + (n - 1) * (&TMR2INV - &TMR1INV))

#define PR(n)         *(&PR1 + (n - 1) * (&PR2 - &PR1))
#define PRCLR(n)      *(&PR1CLR + (n - 1) * (&PR2CLR - &PR1CLR))
#define PRSET(n)      *(&PR1SET + (n - 1) * (&PR2SET - &PR1SET))
#define PRINV(n)      *(&PR1INV + (n - 1) * (&PR2INV - &PR1INV))

/*--------------------------------------
 * Parallel master port registers.
 */
#define PMCON           PIC32_R (0x2E000) /* Control */
#define PMCONCLR        PIC32_R (0x2E004)
#define PMCONSET        PIC32_R (0x2E008)
#define PMCONINV        PIC32_R (0x2E00C)
#define PMMODE          PIC32_R (0x2E010) /* Mode */
#define PMMODECLR       PIC32_R (0x2E014)
#define PMMODESET       PIC32_R (0x2E018)
#define PMMODEINV       PIC32_R (0x2E01C)
#define PMADDR          PIC32_R (0x2E020) /* Address */
#define PMADDRCLR       PIC32_R (0x2E024)
#define PMADDRSET       PIC32_R (0x2E028)
#define PMADDRINV       PIC32_R (0x2E02C)
#define PMDOUT          PIC32_R (0x2E030) /* Data output */
#define PMDOUTCLR       PIC32_R (0x2E034)
#define PMDOUTSET       PIC32_R (0x2E038)
#define PMDOUTINV       PIC32_R (0x2E03C)
#define PMDIN           PIC32_R (0x2E040) /* Data input */
#define PMDINCLR        PIC32_R (0x2E044)
#define PMDINSET        PIC32_R (0x2E048)
#define PMDININV        PIC32_R (0x2E04C)
#define PMAEN           PIC32_R (0x2E050) /* Pin enable */
#define PMAENCLR        PIC32_R (0x2E054)
#define PMAENSET        PIC32_R (0x2E058)
#define PMAENINV        PIC32_R (0x2E05C)
#define PMSTAT          PIC32_R (0x2E060) /* Status (slave only) */
#define PMSTATCLR       PIC32_R (0x2E064)
#define PMSTATSET       PIC32_R (0x2E068)
#define PMSTATINV       PIC32_R (0x2E06C)

/*
 * PMP Control register.
 */
#define PIC32_PMCON_RDSP        0x0001 /* Read strobe polarity active-high */
#define PIC32_PMCON_WRSP        0x0002 /* Write strobe polarity active-high */
#define PIC32_PMCON_CS1P        0x0008 /* Chip select 0 polarity active-high */
#define PIC32_PMCON_CS2P        0x0010 /* Chip select 1 polarity active-high */
#define PIC32_PMCON_ALP         0x0020 /* Address latch polarity active-high */
#define PIC32_PMCON_CSF         0x00C0 /* Chip select function bitmask: */
#define PIC32_PMCON_CSF_NONE    0x0000 /* PMCS2 and PMCS1 as A[15:14] */
#define PIC32_PMCON_CSF_CS2     0x0040 /* PMCS2 as chip select */
#define PIC32_PMCON_CSF_CS21    0x0080 /* PMCS2 and PMCS1 as chip select */
#define PIC32_PMCON_PTRDEN      0x0100 /* Read/write strobe port enable */
#define PIC32_PMCON_PTWREN      0x0200 /* Write enable strobe port enable */
#define PIC32_PMCON_PMPTTL      0x0400 /* TTL input buffer select */
#define PIC32_PMCON_ADRMUX      0x1800 /* Address/data mux selection bitmask: */
#define PIC32_PMCON_ADRMUX_NONE 0x0000 /* Address and data separate */
#define PIC32_PMCON_ADRMUX_AD   0x0800 /* Lower address on PMD[7:0], high on PMA[15:8] */
#define PIC32_PMCON_ADRMUX_D8   0x1000 /* All address on PMD[7:0] */
#define PIC32_PMCON_ADRMUX_D16  0x1800 /* All address on PMD[15:0] */
#define PIC32_PMCON_SIDL        0x2000 /* Stop in idle */
#define PIC32_PMCON_FRZ         0x4000 /* Freeze in debug exception */
#define PIC32_PMCON_ON          0x8000 /* Parallel master port enable */

/*
 * PMP Mode register.
 */
#define PIC32_PMMODE_WAITE(x)   ((x)<<0) /* Wait states: data hold after RW strobe */
#define PIC32_PMMODE_WAITM(x)   ((x)<<2) /* Wait states: data RW strobe */
#define PIC32_PMMODE_WAITB(x)   ((x)<<6) /* Wait states: data setup to RW strobe */
#define PIC32_PMMODE_MODE       0x0300  /* Mode select bitmask: */
#define PIC32_PMMODE_MODE_SLAVE 0x0000  /* Legacy slave */
#define PIC32_PMMODE_MODE_SLENH 0x0100  /* Enhanced slave */
#define PIC32_PMMODE_MODE_MAST2 0x0200  /* Master mode 2 */
#define PIC32_PMMODE_MODE_MAST1 0x0300  /* Master mode 1 */
#define PIC32_PMMODE_MODE16     0x0400  /* 16-bit mode */
#define PIC32_PMMODE_INCM       0x1800  /* Address increment mode bitmask: */
#define PIC32_PMMODE_INCM_NONE  0x0000  /* No increment/decrement */
#define PIC32_PMMODE_INCM_INC   0x0800  /* Increment address */
#define PIC32_PMMODE_INCM_DEC   0x1000  /* Decrement address */
#define PIC32_PMMODE_INCM_SLAVE 0x1800  /* Slave auto-increment */
#define PIC32_PMMODE_IRQM       0x6000  /* Interrupt request bitmask: */
#define PIC32_PMMODE_IRQM_DIS   0x0000  /* No interrupt generated */
#define PIC32_PMMODE_IRQM_END   0x2000  /* Interrupt at end of read/write cycle */
#define PIC32_PMMODE_IRQM_A3    0x4000  /* Interrupt on address 3 */
#define PIC32_PMMODE_BUSY       0x8000  /* Port is busy */

/*
 * PMP Address register.
 */
#define PIC32_PMADDR_PADDR      0x3FFF /* Destination address */
#define PIC32_PMADDR_CS1        0x4000 /* Chip select 1 is active */
#define PIC32_PMADDR_CS2        0x8000 /* Chip select 2 is active */

/*
 * PMP status register (slave only).
 */
#define PIC32_PMSTAT_OB0E       0x0001 /* Output buffer 0 empty */
#define PIC32_PMSTAT_OB1E       0x0002 /* Output buffer 1 empty */
#define PIC32_PMSTAT_OB2E       0x0004 /* Output buffer 2 empty */
#define PIC32_PMSTAT_OB3E       0x0008 /* Output buffer 3 empty */
#define PIC32_PMSTAT_OBUF       0x0040 /* Output buffer underflow */
#define PIC32_PMSTAT_OBE        0x0080 /* Output buffer empty */
#define PIC32_PMSTAT_IB0F       0x0100 /* Input buffer 0 full */
#define PIC32_PMSTAT_IB1F       0x0200 /* Input buffer 1 full */
#define PIC32_PMSTAT_IB2F       0x0400 /* Input buffer 2 full */
#define PIC32_PMSTAT_IB3F       0x0800 /* Input buffer 3 full */
#define PIC32_PMSTAT_IBOV       0x4000 /* Input buffer overflow */
#define PIC32_PMSTAT_IBF        0x8000 /* Input buffer full */

/*--------------------------------------
 * UART registers.
 */
#define U1MODE          PIC32_R (0x22000) /* Mode */
#define U1MODECLR       PIC32_R (0x22004)
#define U1MODESET       PIC32_R (0x22008)
#define U1MODEINV       PIC32_R (0x2200C)
#define U1STA           PIC32_R (0x22010) /* Status and control */
#define U1STACLR        PIC32_R (0x22014)
#define U1STASET        PIC32_R (0x22018)
#define U1STAINV        PIC32_R (0x2201C)
#define U1TXREG         PIC32_R (0x22020) /* Transmit */
#define U1RXREG         PIC32_R (0x22030) /* Receive */
#define U1BRG           PIC32_R (0x22040) /* Baud rate */
#define U1BRGCLR        PIC32_R (0x22044)
#define U1BRGSET        PIC32_R (0x22048)
#define U1BRGINV        PIC32_R (0x2204C)

#define U2MODE          PIC32_R (0x22200) /* Mode */
#define U2MODECLR       PIC32_R (0x22204)
#define U2MODESET       PIC32_R (0x22208)
#define U2MODEINV       PIC32_R (0x2220C)
#define U2STA           PIC32_R (0x22210) /* Status and control */
#define U2STACLR        PIC32_R (0x22214)
#define U2STASET        PIC32_R (0x22218)
#define U2STAINV        PIC32_R (0x2221C)
#define U2TXREG         PIC32_R (0x22220) /* Transmit */
#define U2RXREG         PIC32_R (0x22230) /* Receive */
#define U2BRG           PIC32_R (0x22240) /* Baud rate */
#define U2BRGCLR        PIC32_R (0x22244)
#define U2BRGSET        PIC32_R (0x22248)
#define U2BRGINV        PIC32_R (0x2224C)

#define U3MODE          PIC32_R (0x22400) /* Mode */
#define U3MODECLR       PIC32_R (0x22404)
#define U3MODESET       PIC32_R (0x22408)
#define U3MODEINV       PIC32_R (0x2240C)
#define U3STA           PIC32_R (0x22410) /* Status and control */
#define U3STACLR        PIC32_R (0x22414)
#define U3STASET        PIC32_R (0x22418)
#define U3STAINV        PIC32_R (0x2241C)
#define U3TXREG         PIC32_R (0x22420) /* Transmit */
#define U3RXREG         PIC32_R (0x22430) /* Receive */
#define U3BRG           PIC32_R (0x22440) /* Baud rate */
#define U3BRGCLR        PIC32_R (0x22444)
#define U3BRGSET        PIC32_R (0x22448)
#define U3BRGINV        PIC32_R (0x2244C)

#define U4MODE          PIC32_R (0x22600) /* Mode */
#define U4MODECLR       PIC32_R (0x22604)
#define U4MODESET       PIC32_R (0x22608)
#define U4MODEINV       PIC32_R (0x2260C)
#define U4STA           PIC32_R (0x22610) /* Status and control */
#define U4STACLR        PIC32_R (0x22614)
#define U4STASET        PIC32_R (0x22618)
#define U4STAINV        PIC32_R (0x2261C)
#define U4TXREG         PIC32_R (0x22620) /* Transmit */
#define U4RXREG         PIC32_R (0x22630) /* Receive */
#define U4BRG           PIC32_R (0x22640) /* Baud rate */
#define U4BRGCLR        PIC32_R (0x22644)
#define U4BRGSET        PIC32_R (0x22648)
#define U4BRGINV        PIC32_R (0x2264C)

#define U5MODE          PIC32_R (0x22800) /* Mode */
#define U5MODECLR       PIC32_R (0x22804)
#define U5MODESET       PIC32_R (0x22808)
#define U5MODEINV       PIC32_R (0x2280C)
#define U5STA           PIC32_R (0x22810) /* Status and control */
#define U5STACLR        PIC32_R (0x22814)
#define U5STASET        PIC32_R (0x22818)
#define U5STAINV        PIC32_R (0x2281C)
#define U5TXREG         PIC32_R (0x22820) /* Transmit */
#define U5RXREG         PIC32_R (0x22830) /* Receive */
#define U5BRG           PIC32_R (0x22840) /* Baud rate */
#define U5BRGCLR        PIC32_R (0x22844)
#define U5BRGSET        PIC32_R (0x22848)
#define U5BRGINV        PIC32_R (0x2284C)

#define U6MODE          PIC32_R (0x22A00) /* Mode */
#define U6MODECLR       PIC32_R (0x22A04)
#define U6MODESET       PIC32_R (0x22A08)
#define U6MODEINV       PIC32_R (0x22A0C)
#define U6STA           PIC32_R (0x22A10) /* Status and control */
#define U6STACLR        PIC32_R (0x22A14)
#define U6STASET        PIC32_R (0x22A18)
#define U6STAINV        PIC32_R (0x22A1C)
#define U6TXREG         PIC32_R (0x22A20) /* Transmit */
#define U6RXREG         PIC32_R (0x22A30) /* Receive */
#define U6BRG           PIC32_R (0x22A40) /* Baud rate */
#define U6BRGCLR        PIC32_R (0x22A44)
#define U6BRGSET        PIC32_R (0x22A48)
#define U6BRGINV        PIC32_R (0x22A4C)

/*
 * UART Mode register.
 */
#define PIC32_UMODE_STSEL       0x0001  /* 2 Stop bits */
#define PIC32_UMODE_PDSEL       0x0006  /* Bitmask: */
#define PIC32_UMODE_PDSEL_8NPAR 0x0000  /* 8-bit data, no parity */
#define PIC32_UMODE_PDSEL_8EVEN 0x0002  /* 8-bit data, even parity */
#define PIC32_UMODE_PDSEL_8ODD  0x0004  /* 8-bit data, odd parity */
#define PIC32_UMODE_PDSEL_9NPAR 0x0006  /* 9-bit data, no parity */
#define PIC32_UMODE_BRGH        0x0008  /* High Baud Rate Enable */
#define PIC32_UMODE_RXINV       0x0010  /* Receive Polarity Inversion */
#define PIC32_UMODE_ABAUD       0x0020  /* Auto-Baud Enable */
#define PIC32_UMODE_LPBACK      0x0040  /* UARTx Loopback Mode */
#define PIC32_UMODE_WAKE        0x0080  /* Wake-up on start bit during Sleep Mode */
#define PIC32_UMODE_UEN         0x0300  /* Bitmask: */
#define PIC32_UMODE_UEN_RTS     0x0100  /* Using UxRTS pin */
#define PIC32_UMODE_UEN_RTSCTS  0x0200  /* Using UxCTS and UxRTS pins */
#define PIC32_UMODE_UEN_BCLK    0x0300  /* Using UxBCLK pin */
#define PIC32_UMODE_RTSMD       0x0800  /* UxRTS Pin Simplex mode */
#define PIC32_UMODE_IREN        0x1000  /* IrDA Encoder and Decoder Enable bit */
#define PIC32_UMODE_SIDL        0x2000  /* Stop in Idle Mode */
#define PIC32_UMODE_FRZ         0x4000  /* Freeze in Debug Exception Mode */
#define PIC32_UMODE_ON          0x8000  /* UART Enable */

/*
 * UART Control and status register.
 */
#define PIC32_USTA_URXDA        0x00000001 /* Receive Data Available (read-only) */
#define PIC32_USTA_OERR         0x00000002 /* Receive Buffer Overrun */
#define PIC32_USTA_FERR         0x00000004 /* Framing error detected (read-only) */
#define PIC32_USTA_PERR         0x00000008 /* Parity error detected (read-only) */
#define PIC32_USTA_RIDLE        0x00000010 /* Receiver is idle (read-only) */
#define PIC32_USTA_ADDEN        0x00000020 /* Address Detect mode */
#define PIC32_USTA_URXISEL      0x000000C0 /* Bitmask: receive interrupt is set when... */
#define PIC32_USTA_URXISEL_NEMP 0x00000000 /* ...receive buffer is not empty */
#define PIC32_USTA_URXISEL_HALF 0x00000040 /* ...receive buffer becomes 1/2 full */
#define PIC32_USTA_URXISEL_3_4  0x00000080 /* ...receive buffer becomes 3/4 full */
#define PIC32_USTA_TRMT         0x00000100 /* Transmit shift register is empty (read-only) */
#define PIC32_USTA_UTXBF        0x00000200 /* Transmit buffer is full (read-only) */
#define PIC32_USTA_UTXEN        0x00000400 /* Transmit Enable */
#define PIC32_USTA_UTXBRK       0x00000800 /* Transmit Break */
#define PIC32_USTA_URXEN        0x00001000 /* Receiver Enable */
#define PIC32_USTA_UTXINV       0x00002000 /* Transmit Polarity Inversion */
#define PIC32_USTA_UTXISEL      0x0000C000 /* Bitmask: TX interrupt is generated when... */
#define PIC32_USTA_UTXISEL_1    0x00000000 /* ...the transmit buffer contains at least one empty space */
#define PIC32_USTA_UTXISEL_ALL  0x00004000 /* ...all characters have been transmitted */
#define PIC32_USTA_UTXISEL_EMP  0x00008000 /* ...the transmit buffer becomes empty */
#define PIC32_USTA_ADDR         0x00FF0000 /* Automatic Address Mask */
#define PIC32_USTA_ADM_EN       0x01000000 /* Automatic Address Detect */

/*
 * Compute the 16-bit baud rate divisor, given
 * the bus frequency and baud rate.
 * Round to the nearest integer.
 */
#define PIC32_BRG_BAUD(fr,bd)   ((((fr)/8 + (bd)) / (bd) / 2) - 1)

/*--------------------------------------
 * Peripheral port select registers.
 */
#define INT1R           PIC32_R (0x1404)
#define INT2R           PIC32_R (0x1408)
#define INT3R           PIC32_R (0x140C)
#define INT4R           PIC32_R (0x1410)
#define T2CKR           PIC32_R (0x1418)
#define T3CKR           PIC32_R (0x141C)
#define T4CKR           PIC32_R (0x1420)
#define T5CKR           PIC32_R (0x1424)
#define T6CKR           PIC32_R (0x1428)
#define T7CKR           PIC32_R (0x142C)
#define T8CKR           PIC32_R (0x1430)
#define T9CKR           PIC32_R (0x1434)
#define IC1R            PIC32_R (0x1438)
#define IC2R            PIC32_R (0x143C)
#define IC3R            PIC32_R (0x1440)
#define IC4R            PIC32_R (0x1444)
#define IC5R            PIC32_R (0x1448)
#define IC6R            PIC32_R (0x144C)
#define IC7R            PIC32_R (0x1450)
#define IC8R            PIC32_R (0x1454)
#define IC9R            PIC32_R (0x1458)
#define OCFAR           PIC32_R (0x1460)
#define U1RXR           PIC32_R (0x1468)
#define U1CTSR          PIC32_R (0x146C)
#define U2RXR           PIC32_R (0x1470)
#define U2CTSR          PIC32_R (0x1474)
#define U3RXR           PIC32_R (0x1478)
#define U3CTSR          PIC32_R (0x147C)
#define U4RXR           PIC32_R (0x1480)
#define U4CTSR          PIC32_R (0x1484)
#define U5RXR           PIC32_R (0x1488)
#define U5CTSR          PIC32_R (0x148C)
#define U6RXR           PIC32_R (0x1490)
#define U6CTSR          PIC32_R (0x1494)
#define SDI1R           PIC32_R (0x149C)
#define SS1R            PIC32_R (0x14A0)
#define SDI2R           PIC32_R (0x14A8)
#define SS2R            PIC32_R (0x14AC)
#define SDI3R           PIC32_R (0x14B4)
#define SS3R            PIC32_R (0x14B8)
#define SDI4R           PIC32_R (0x14C0)
#define SS4R            PIC32_R (0x14C4)
#define SDI5R           PIC32_R (0x14CC)
#define SS5R            PIC32_R (0x14D0)
#define SDI6R           PIC32_R (0x14D8)
#define SS6R            PIC32_R (0x14DC)
#define C1RXR           PIC32_R (0x14E0)
#define C2RXR           PIC32_R (0x14E4)
#define REFCLKI1R       PIC32_R (0x14E8)
#define REFCLKI3R       PIC32_R (0x14F0)
#define REFCLKI4R       PIC32_R (0x14F4)

#define RPA14R          PIC32_R (0x1538)
#define RPA15R          PIC32_R (0x153C)
#define RPB0R           PIC32_R (0x1540)
#define RPB1R           PIC32_R (0x1544)
#define RPB2R           PIC32_R (0x1548)
#define RPB3R           PIC32_R (0x154C)
#define RPB5R           PIC32_R (0x1554)
#define RPB6R           PIC32_R (0x1558)
#define RPB7R           PIC32_R (0x155C)
#define RPB8R           PIC32_R (0x1560)
#define RPB9R           PIC32_R (0x1564)
#define RPB10R          PIC32_R (0x1568)
#define RPB14R          PIC32_R (0x1578)
#define RPB15R          PIC32_R (0x157C)
#define RPC1R           PIC32_R (0x1584)
#define RPC2R           PIC32_R (0x1588)
#define RPC3R           PIC32_R (0x158C)
#define RPC4R           PIC32_R (0x1590)
#define RPC13R          PIC32_R (0x15B4)
#define RPC14R          PIC32_R (0x15B8)
#define RPD0R           PIC32_R (0x15C0)
#define RPD1R           PIC32_R (0x15C4)
#define RPD2R           PIC32_R (0x15C8)
#define RPD3R           PIC32_R (0x15CC)
#define RPD4R           PIC32_R (0x15D0)
#define RPD5R           PIC32_R (0x15D4)
#define RPD6R           PIC32_R (0x15D8)
#define RPD7R           PIC32_R (0x15DC)
#define RPD9R           PIC32_R (0x15E4)
#define RPD10R          PIC32_R (0x15E8)
#define RPD11R          PIC32_R (0x15EC)
#define RPD12R          PIC32_R (0x15F0)
#define RPD14R          PIC32_R (0x15F8)
#define RPD15R          PIC32_R (0x15FC)
#define RPE3R           PIC32_R (0x160C)
#define RPE5R           PIC32_R (0x1614)
#define RPE8R           PIC32_R (0x1620)
#define RPE9R           PIC32_R (0x1624)
#define RPF0R           PIC32_R (0x1640)
#define RPF1R           PIC32_R (0x1644)
#define RPF2R           PIC32_R (0x1648)
#define RPF3R           PIC32_R (0x164C)
#define RPF4R           PIC32_R (0x1650)
#define RPF5R           PIC32_R (0x1654)
#define RPF8R           PIC32_R (0x1660)
#define RPF12R          PIC32_R (0x1670)
#define RPF13R          PIC32_R (0x1674)
#define RPG0R           PIC32_R (0x1680)
#define RPG1R           PIC32_R (0x1684)
#define RPG6R           PIC32_R (0x1698)
#define RPG7R           PIC32_R (0x169C)
#define RPG8R           PIC32_R (0x16A0)
#define RPG9R           PIC32_R (0x16A4)

/*--------------------------------------
 * Prefetch cache controller registers.
 */
#define PRECON          PIC32_R (0xe0000)       /* Prefetch cache control */
#define PRECONCLR       PIC32_R (0xe0004)
#define PRECONSET       PIC32_R (0xe0008)
#define PRECONINV       PIC32_R (0xe000C)
#define PRESTAT         PIC32_R (0xe0010)       /* Prefetch status */
#define PRESTATCLR      PIC32_R (0xe0014)
#define PRESTATSET      PIC32_R (0xe0018)
#define PRESTATINV      PIC32_R (0xe001C)
// TODO: other prefetch registers

/*--------------------------------------
 * System controller registers.
 */
#define CFGCON          PIC32_R (0x0000)
#define DEVID           PIC32_R (0x0020)
#define SYSKEY          PIC32_R (0x0030)
#define CFGEBIA         PIC32_R (0x00c0)
#define CFGEBIACLR      PIC32_R (0x00c4)
#define CFGEBIASET      PIC32_R (0x00c8)
#define CFGEBIAINV      PIC32_R (0x00cc)
#define CFGEBIC         PIC32_R (0x00d0)
#define CFGEBICCLR      PIC32_R (0x00d4)
#define CFGEBICSET      PIC32_R (0x00d8)
#define CFGEBICINV      PIC32_R (0x00dc)
#define CFGPG           PIC32_R (0x00e0)

// WHITECAT BEGIN
#define I2C1CON          PIC32_R (0x20000)
#define I2C1CONCLR       PIC32_R (0x20004)
#define I2C1CONSET       PIC32_R (0x20008)
#define I2C1CONINV       PIC32_R (0x2000C)
#define I2C1STAT         PIC32_R (0x20010)
#define I2C1STATCLR      PIC32_R (0x20014)
#define I2C1STATSET      PIC32_R (0x20018)
#define I2C1STATINV      PIC32_R (0x2001C)
#define I2C1ADD          PIC32_R (0x20020)
#define I2C1ADDCLR       PIC32_R (0x20024)
#define I2C1ADDSET       PIC32_R (0x20028)
#define I2C1ADDINV       PIC32_R (0x2002C)
#define I2C1MSK          PIC32_R (0x20030)
#define I2C1MSKCLR       PIC32_R (0x20034)
#define I2C1MSKSET       PIC32_R (0x20038)
#define I2C1MSKINV       PIC32_R (0x2003C)
#define I2C1BRG          PIC32_R (0x20040)
#define I2C1BRGCLR       PIC32_R (0x20044)
#define I2C1BRGSET       PIC32_R (0x20048)
#define I2C1BRGINV       PIC32_R (0x2004C)
#define I2C1TRN          PIC32_R (0x20050)
#define I2C1TRNCLR       PIC32_R (0x20054)
#define I2C1TRNSET       PIC32_R (0x20058)
#define I2C1TRNINV       PIC32_R (0x2005C)
#define I2C1RCV          PIC32_R (0x20060)
#define I2C1RCVCLR       PIC32_R (0x20064)
#define I2C1RCVSET       PIC32_R (0x20068)
#define I2C1RCVINV       PIC32_R (0x2006C)
#define I2C3CON          PIC32_R (0x20400)
#define I2C3CONCLR       PIC32_R (0x20404)
#define I2C3CONSET       PIC32_R (0x20408)
#define I2C3CONINV       PIC32_R (0x2040C)
#define I2C3STAT         PIC32_R (0x20410)
#define I2C3STATCLR      PIC32_R (0x20414)
#define I2C3STATSET      PIC32_R (0x20418)
#define I2C3STATINV      PIC32_R (0x2041C)
#define I2C3ADD          PIC32_R (0x20420)
#define I2C3ADDCLR       PIC32_R (0x20424)
#define I2C3ADDSET       PIC32_R (0x20428)
#define I2C3ADDINV       PIC32_R (0x2042C)
#define I2C3MSK          PIC32_R (0x20430)
#define I2C3MSKCLR       PIC32_R (0x20434)
#define I2C3MSKSET       PIC32_R (0x20438)
#define I2C3MSKINV       PIC32_R (0x2043C)
#define I2C3BRG          PIC32_R (0x20440)
#define I2C3BRGCLR       PIC32_R (0x20444)
#define I2C3BRGSET       PIC32_R (0x20448)
#define I2C3BRGINV       PIC32_R (0x2044C)
#define I2C3TRN          PIC32_R (0x20450)
#define I2C3TRNCLR       PIC32_R (0x20454)
#define I2C3TRNSET       PIC32_R (0x20458)
#define I2C3TRNINV       PIC32_R (0x2045C)
#define I2C3RCV          PIC32_R (0x20460)
#define I2C3RCVCLR       PIC32_R (0x20464)
#define I2C3RCVSET       PIC32_R (0x20468)
#define I2C3RCVINV       PIC32_R (0x2046C)
#define I2C4CON          PIC32_R (0x20600)
#define I2C4CONCLR       PIC32_R (0x20604)
#define I2C4CONSET       PIC32_R (0x20608)
#define I2C4CONINV       PIC32_R (0x2060C)
#define I2C4STAT         PIC32_R (0x20610)
#define I2C4STATCLR      PIC32_R (0x20614)
#define I2C4STATSET      PIC32_R (0x20618)
#define I2C4STATINV      PIC32_R (0x2061C)
#define I2C4ADD          PIC32_R (0x20620)
#define I2C4ADDCLR       PIC32_R (0x20624)
#define I2C4ADDSET       PIC32_R (0x20628)
#define I2C4ADDINV       PIC32_R (0x2062C)
#define I2C4MSK          PIC32_R (0x20630)
#define I2C4MSKCLR       PIC32_R (0x20634)
#define I2C4MSKSET       PIC32_R (0x20638)
#define I2C4MSKINV       PIC32_R (0x2063C)
#define I2C4BRG          PIC32_R (0x20640)
#define I2C4BRGCLR       PIC32_R (0x20644)
#define I2C4BRGSET       PIC32_R (0x20648)
#define I2C4BRGINV       PIC32_R (0x2064C)
#define I2C4TRN          PIC32_R (0x20650)
#define I2C4TRNCLR       PIC32_R (0x20654)
#define I2C4TRNSET       PIC32_R (0x20658)
#define I2C4TRNINV       PIC32_R (0x2065C)
#define I2C4RCV          PIC32_R (0x20660)
#define I2C4RCVCLR       PIC32_R (0x20664)
#define I2C4RCVSET       PIC32_R (0x20668)
#define I2C4RCVINV       PIC32_R (0x2066C)
#define I2C5CON          PIC32_R (0x20800)
#define I2C5CONCLR       PIC32_R (0x20804)
#define I2C5CONSET       PIC32_R (0x20808)
#define I2C5CONINV       PIC32_R (0x2080C)
#define I2C5STAT         PIC32_R (0x20810)
#define I2C5STATCLR      PIC32_R (0x20814)
#define I2C5STATSET      PIC32_R (0x20818)
#define I2C5STATINV      PIC32_R (0x2081C)
#define I2C5ADD          PIC32_R (0x20820)
#define I2C5ADDCLR       PIC32_R (0x20824)
#define I2C5ADDSET       PIC32_R (0x20828)
#define I2C5ADDINV       PIC32_R (0x2082C)
#define I2C5MSK          PIC32_R (0x20830)
#define I2C5MSKCLR       PIC32_R (0x20834)
#define I2C5MSKSET       PIC32_R (0x20838)
#define I2C5MSKINV       PIC32_R (0x2083C)
#define I2C5BRG          PIC32_R (0x20840)
#define I2C5BRGCLR       PIC32_R (0x20844)
#define I2C5BRGSET       PIC32_R (0x20848)
#define I2C5BRGINV       PIC32_R (0x2084C)
#define I2C5TRN          PIC32_R (0x20850)
#define I2C5TRNCLR       PIC32_R (0x20854)
#define I2C5TRNSET       PIC32_R (0x20858)
#define I2C5TRNINV       PIC32_R (0x2085C)
#define I2C5RCV          PIC32_R (0x20860)
#define I2C5RCVCLR       PIC32_R (0x20864)
#define I2C5RCVSET       PIC32_R (0x20868)
#define I2C5RCVINV       PIC32_R (0x2086C) 

#define RTCCON          PIC32_R (0x0C00)
#define RTCCONCLR       PIC32_R (0x0C04)
#define RTCCONSET       PIC32_R (0x0C08)
#define RTCCONINV       PIC32_R (0x0C0C)
#define RTCALRM         PIC32_R (0x0C10)
#define RTCALRMCLR      PIC32_R (0x0C14)
#define RTCALRMSET      PIC32_R (0x0C18)
#define RTCALRMINV      PIC32_R (0x0C1C)
#define RTCTIME         PIC32_R (0x0C20)
#define RTCTIMECLR      PIC32_R (0x0C24)
#define RTCTIMESET      PIC32_R (0x0C28)
#define RTCTIMEINV      PIC32_R (0x0C2C)
#define RTCDATE         PIC32_R (0x0C30)
#define RTCDATECLR      PIC32_R (0x0C34)
#define RTCDATESET      PIC32_R (0x0C38)
#define RTCDATEINV      PIC32_R (0x0C3C)
#define ALRMTIME        PIC32_R (0x0C40)
#define ALRMTIMECLR     PIC32_R (0x0C44)
#define ALRMTIMESET     PIC32_R (0x0C48)
#define ALRMTIMEINV     PIC32_R (0x0C4C)
#define ALRMDATE        PIC32_R (0x0C50)
#define ALRMDATECLR     PIC32_R (0x0C54)
#define ALRMDATESET     PIC32_R (0x0C58)
#define ALRMDATEINV     PIC32_R (0x0C5C)
  
  
#define NVMCON          PIC32_R (0x0600)
#define NVMCONCLR       PIC32_R (0x0604)
#define NVMCONSET       PIC32_R (0x0608)
#define NVMCONINV       PIC32_R (0x060c)
#define NVMKEY          PIC32_R (0x0610)
#define NVMADDR         PIC32_R (0x0620)
#define NVMADDRCLR      PIC32_R (0x0624)
#define NVMADDRSET      PIC32_R (0x0628)
#define NVMADDRINV      PIC32_R (0x062c)
#define NVMDATA0        PIC32_R (0x0630)
#define NVMSRCADDR      PIC32_R (0x0670)
// WHITECAT END
  
#define OSCCON          PIC32_R (0x1200)    /* Oscillator Control */
#define OSCCONCLR       PIC32_R (0x1204)    
#define OSCCONSET       PIC32_R (0x1208)    
#define OSCCONINV       PIC32_R (0x120C)    
#define OSCTUN          PIC32_R (0x1210)
#define SPLLCON         PIC32_R (0x1220)
#define RCON            PIC32_R (0x1240)
#define RCONCLR         PIC32_R (0x1244)
#define RCONSET         PIC32_R (0x1248)
#define RCONINV         PIC32_R (0x124C)
#define RSWRST          PIC32_R (0x1250)
#define RSWRSTCLR       PIC32_R (0x1254)
#define RSWRSTSET       PIC32_R (0x1258)
#define RSWRSTINV       PIC32_R (0x125c)
#define RNMICON         PIC32_R (0x1260)
#define PWRCON          PIC32_R (0x1270)
#define REFO1CON        PIC32_R (0x1280)
#define REFO1CONCLR     PIC32_R (0x1284)
#define REFO1CONSET     PIC32_R (0x1288)
#define REFO1CONINV     PIC32_R (0x128c)
#define REFO1TRIM       PIC32_R (0x1290)
#define REFO2CON        PIC32_R (0x12A0)
#define REFO2CONCLR     PIC32_R (0x12A4)
#define REFO2CONSET     PIC32_R (0x12A8)
#define REFO2CONINV     PIC32_R (0x12Ac)
#define REFO2TRIM       PIC32_R (0x12B0)
#define REFO3CON        PIC32_R (0x12C0)
#define REFO3CONCLR     PIC32_R (0x12C4)
#define REFO3CONSET     PIC32_R (0x12C8)
#define REFO3CONINV     PIC32_R (0x12Cc)
#define REFO3TRIM       PIC32_R (0x12D0)
#define REFO4CON        PIC32_R (0x12E0)
#define REFO4CONCLR     PIC32_R (0x12E4)
#define REFO4CONSET     PIC32_R (0x12E8)
#define REFO4CONINV     PIC32_R (0x12Ec)
#define REFO4TRIM       PIC32_R (0x12F0)
#define PB1DIV          PIC32_R (0x1300)
#define PB1DIVCLR       PIC32_R (0x1304)
#define PB1DIVSET       PIC32_R (0x1308)
#define PB1DIVINV       PIC32_R (0x130c)
#define PB2DIV          PIC32_R (0x1310)
#define PB2DIVCLR       PIC32_R (0x1314)
#define PB2DIVSET       PIC32_R (0x1318)
#define PB2DIVINV       PIC32_R (0x131c)
#define PB3DIV          PIC32_R (0x1320)
#define PB3DIVCLR       PIC32_R (0x1324)
#define PB3DIVSET       PIC32_R (0x1328)
#define PB3DIVINV       PIC32_R (0x132c)
#define PB4DIV          PIC32_R (0x1330)
#define PB4DIVCLR       PIC32_R (0x1334)
#define PB4DIVSET       PIC32_R (0x1338)
#define PB4DIVINV       PIC32_R (0x133c)
#define PB5DIV          PIC32_R (0x1340)
#define PB5DIVCLR       PIC32_R (0x1344)
#define PB5DIVSET       PIC32_R (0x1348)
#define PB5DIVINV       PIC32_R (0x134c)
#define PB7DIV          PIC32_R (0x1360)
#define PB7DIVCLR       PIC32_R (0x1364)
#define PB7DIVSET       PIC32_R (0x1368)
#define PB7DIVINV       PIC32_R (0x136c)
#define PB8DIV          PIC32_R (0x1370)
#define PB8DIVCLR       PIC32_R (0x1374)
#define PB8DIVSET       PIC32_R (0x1378)
#define PB8DIVINV       PIC32_R (0x137c)

/*
 * Configuration Control register.
 */
#define PIC32_CFGCON_DMAPRI     0x02000000 /* DMA gets High Priority access to SRAM */
#define PIC32_CFGCON_CPUPRI     0x01000000 /* CPU gets High Priority access to SRAM */
#define PIC32_CFGCON_ICACLK     0x00020000 /* Input Capture alternate clock selection */
#define PIC32_CFGCON_OCACLK     0x00010000 /* Output Compare alternate clock selection */
#define PIC32_CFGCON_IOLOCK     0x00002000 /* Peripheral pin select lock */
#define PIC32_CFGCON_PMDLOCK    0x00001000 /* Peripheral module disable */
#define PIC32_CFGCON_PGLOCK     0x00000800 /* Permission group lock */
#define PIC32_CFGCON_USBSSEN    0x00000100 /* USB suspend sleep enable */
#define PIC32_CFGCON_ECC_MASK   0x00000030 /* Flash ECC Configuration */
#define PIC32_CFGCON_ECC_DISWR  0x00000030 /* ECC disabled, ECCCON<1:0> writable */
#define PIC32_CFGCON_ECC_DISRO  0x00000020 /* ECC disabled, ECCCON<1:0> locked */
#define PIC32_CFGCON_ECC_DYN    0x00000010 /* Dynamic Flash ECC is enabled */
#define PIC32_CFGCON_ECC_EN     0x00000000 /* Flash ECC is enabled */
#define PIC32_CFGCON_JTAGEN     0x00000008 /* JTAG port enable */
#define PIC32_CFGCON_TROEN      0x00000004 /* Trace output enable */
#define PIC32_CFGCON_TDOEN      0x00000001 /* 2-wire JTAG protocol uses TDO */


/*--------------------------------------
 * A/D Converter registers.
 */
#define AD1CON1         PIC32_R (0x4b000)   /* revised Control register 1 */
#define AD1CON2         PIC32_R (0x4b004)   /* revised Control register 2 */
#define AD1CON3         PIC32_R (0x4b008)   /* revised Control register 3 */
#define AD1TRGMODE      PIC32_R (0x4b00c)   /* revised */
#define AD1IMCON1       PIC32_R (0x4b010)   /* revised */
#define AD1IMCON2       PIC32_R (0x4b014)   /* revised */
#define AD1IMCON3       PIC32_R (0x4b018)   /* revised */
#define AD1GIRQEN1      PIC32_R (0x4b020)   /* revised */
#define AD1GIRQEN2      PIC32_R (0x4b024)   /* revised */
#define AD1CSS1         PIC32_R (0x4b028)   /* revised */
#define AD1CSS2         PIC32_R (0x4b02c)   /* revised */
#define AD1DSTAT1       PIC32_R (0x4b030)   /* revised */
#define AD1DSTAT2       PIC32_R (0x4b034)   /* revised */
#define AD1TRG1         PIC32_R (0x4b080)   /* revised */
#define AD1TRG2         PIC32_R (0x4b084)   /* revised */
#define AD1TRG3         PIC32_R (0x4b088)   /* revised */
#define AD1ANCON        PIC32_R (0x4b100)   /* revised */
#define AD1TRGSNS       PIC32_R (0x4b0d0)   /* revised */
#define AD10TIME        PIC32_R (0x4b0d4)   /* revised */
#define AD11TIME        PIC32_R (0x4b0d8)   /* revised */
#define AD12TIME        PIC32_R (0x4b0dc)   /* revised */
#define AD13TIME        PIC32_R (0x4b0e0)   /* revised */
#define AD14TIME        PIC32_R (0x4b0e4)   /* revised */
#define AD1DATA0        PIC32_R (0x4b200)   /* revised */
#define AD1FLTR1        PIC32_R (0x4b068)   /* revised */
#define AD1FLTR2        PIC32_R (0x4b06c)   /* revised */
#define AD1FLTR3        PIC32_R (0x4b070)   /* revised */
#define AD1FLTR4        PIC32_R (0x4b074)   /* revised */
#define AD1FLTR5        PIC32_R (0x4b078)   /* revised */
#define AD1FLTR6        PIC32_R (0x4b07c)   /* revised */
#define AD1CMPCON1      PIC32_R (0x4b0a0)   /* revised */
#define AD1CMPCON2      PIC32_R (0x4b0a4)   /* revised */
#define AD1CMPCON3      PIC32_R (0x4b0a8)   /* revised */
#define AD1CMPCON4      PIC32_R (0x4b0ac)   /* revised */
#define AD1CMPCON5      PIC32_R (0x4b0b0)   /* revised */
#define AD1CMPCON6      PIC32_R (0x4b0b4)   /* revised */
#define AD1EIEN1        PIC32_R (0x4b0f0)   /* revised */
#define AD1EIEN2        PIC32_R (0x4b0f4)   /* revised */
  
/*--------------------------------------
* PMD registers.
*/
// PMD1
#define CVRMD  (1 << 12)
#define ADCMD  (1 <<  0)

// PMD2
#define CMP2MD (1 << 17)
#define CMP1MD (1 << 17)

// PMD3
#define OC9MD   (1 << 24)
#define OC8MD   (1 << 23)
#define OC7MD   (1 << 22)
#define OC6MD   (1 << 21)
#define OC5MD   (1 << 20)
#define OC4MD   (1 << 19)
#define OC3MD   (1 << 18)
#define OC2MD   (1 << 17)
#define OC1MD   (1 << 16)
#define IC9MD   (1 <<  8)
#define IC8MD   (1 <<  7)
#define IC7MD   (1 <<  6)
#define IC6MD   (1 <<  5)
#define IC5MD   (1 <<  4)
#define IC4MD   (1 <<  3)
#define IC3MD   (1 <<  2)
#define IC2MD   (1 <<  1)
#define IC1MD   (1 <<  0)

// PMD4
#define T9MD    (1 <<  8)
#define T8MD    (1 <<  7)
#define T7MD    (1 <<  6)
#define T6MD    (1 <<  5)
#define T5MD    (1 <<  4)
#define T4MD    (1 <<  3)
#define T3MD    (1 <<  2)
#define T2MD    (1 <<  1)
#define T1MD    (1 <<  0)

// PMD5
#define CAN2MD  (1 << 29)
#define CAN1MD  (1 << 28)
#define USBMD   (1 << 24)
#define I2C5MD  (1 << 20)
#define I2C4MD  (1 << 19)
#define I2C3MD  (1 << 18)
#define I2C2MD  (1 << 17)
#define I2C1MD  (1 << 16)
#define SPI6MD  (1 << 13)
#define SPI5MD  (1 << 12)
#define SPI4MD  (1 << 11)
#define SPI3MD  (1 << 10)
#define SPI2MD  (1 <<  9)
#define SPI1MD  (1 <<  8)
#define U6MD    (1 <<  5)
#define U5MD    (1 <<  4)
#define U4MD    (1 <<  3)
#define U3MD    (1 <<  2)
#define U2MD    (1 <<  1)
#define U1MD    (1 <<  0)

// PMD6
#define ETHMD   (1 << 28)
#define SQIMD   (1 << 23)
#define EBIMD   (1 << 17)
#define PMPMD   (1 << 16)
#define REFO4MD (1 << 11)
#define REFO3MD (1 << 10)
#define REFO2MD (1 <<  9)
#define REFO1MD (1 <<  8)
#define RTCCMD  (1 <<  0)

// PMD7
#define CRYPTMD (1 << 22)
#define RNGMD   (1 << 20)
#define DMAMD   (1 <<  4)

#define PMD1             PIC32_R ( 0x00040 )
#define PMD1CLR          PIC32_R ( 0x00044 )
#define PMD1SET          PIC32_R ( 0x00048 )
#define PMD1INV          PIC32_R ( 0x0004C )
#define PMD2             PIC32_R ( 0x00050 )
#define PMD2CLR          PIC32_R ( 0x00054 )
#define PMD2SET          PIC32_R ( 0x00058 )
#define PMD2INV          PIC32_R ( 0x0005C )
#define PMD3             PIC32_R ( 0x00060 )
#define PMD3CLR          PIC32_R ( 0x00064 )
#define PMD3SET          PIC32_R ( 0x00068 )
#define PMD3INV          PIC32_R ( 0x0006C )
#define PMD4             PIC32_R ( 0x00070 )
#define PMD4CLR          PIC32_R ( 0x00074 )
#define PMD4SET          PIC32_R ( 0x00078 )
#define PMD4INV          PIC32_R ( 0x0007C )
#define PMD5             PIC32_R ( 0x00080 )
#define PMD5CLR          PIC32_R ( 0x00084 )
#define PMD5SET          PIC32_R ( 0x00088 )
#define PMD5INV          PIC32_R ( 0x0008C )
#define PMD6             PIC32_R ( 0x00090 )
#define PMD6CLR          PIC32_R ( 0x00094 )
#define PMD6SET          PIC32_R ( 0x00098 )
#define PMD6INV          PIC32_R ( 0x0009C )
#define PMD7             PIC32_R ( 0x000A0 )
#define PMD7CLR          PIC32_R ( 0x000A4 )
#define PMD7SET          PIC32_R ( 0x000A8 )
#define PMD7INV          PIC32_R ( 0x000AC )
		
/*--------------------------------------
 * SPI registers.
 */
#define SPI1CON         PIC32_R (0x21000) /* Control */
#define SPI1CONCLR      PIC32_R (0x21004)
#define SPI1CONSET      PIC32_R (0x21008)
#define SPI1CONINV      PIC32_R (0x2100c)
#define SPI1STAT        PIC32_R (0x21010) /* Status */
#define SPI1STATCLR     PIC32_R (0x21014)
#define SPI1STATSET     PIC32_R (0x21018)
#define SPI1STATINV     PIC32_R (0x2101c)
#define SPI1BUF         PIC32_R (0x21020) /* Transmit and receive buffer */
#define SPI1BRG         PIC32_R (0x21030) /* Baud rate generator */
#define SPI1BRGCLR      PIC32_R (0x21034)
#define SPI1BRGSET      PIC32_R (0x21038)
#define SPI1BRGINV      PIC32_R (0x2103c)
#define SPI1CON2        PIC32_R (0x21040) /* Control 2 */
#define SPI1CON2CLR     PIC32_R (0x21044)
#define SPI1CON2SET     PIC32_R (0x21048)
#define SPI1CON2INV     PIC32_R (0x2104c)

#define SPI2CON         PIC32_R (0x21200) /* Control */
#define SPI2CONCLR      PIC32_R (0x21204)
#define SPI2CONSET      PIC32_R (0x21208)
#define SPI2CONINV      PIC32_R (0x2120c)
#define SPI2STAT        PIC32_R (0x21210) /* Status */
#define SPI2STATCLR     PIC32_R (0x21214)
#define SPI2STATSET     PIC32_R (0x21218)
#define SPI2STATINV     PIC32_R (0x2121c)
#define SPI2BUF         PIC32_R (0x21220) /* Transmit and receive buffer */
#define SPI2BRG         PIC32_R (0x21230) /* Baud rate generator */
#define SPI2BRGCLR      PIC32_R (0x21234)
#define SPI2BRGSET      PIC32_R (0x21238)
#define SPI2BRGINV      PIC32_R (0x2123c)
#define SPI2CON2        PIC32_R (0x21240) /* Control 2 */
#define SPI2CON2CLR     PIC32_R (0x21244)
#define SPI2CON2SET     PIC32_R (0x21248)
#define SPI2CON2INV     PIC32_R (0x2124c)

#define SPI3CON         PIC32_R (0x21400) /* Control */
#define SPI3CONCLR      PIC32_R (0x21404)
#define SPI3CONSET      PIC32_R (0x21408)
#define SPI3CONINV      PIC32_R (0x2140c)
#define SPI3STAT        PIC32_R (0x21410) /* Status */
#define SPI3STATCLR     PIC32_R (0x21414)
#define SPI3STATSET     PIC32_R (0x21418)
#define SPI3STATINV     PIC32_R (0x2141c)
#define SPI3BUF         PIC32_R (0x21420) /* Transmit and receive buffer */
#define SPI3BRG         PIC32_R (0x21430) /* Baud rate generator */
#define SPI3BRGCLR      PIC32_R (0x21434)
#define SPI3BRGSET      PIC32_R (0x21438)
#define SPI3BRGINV      PIC32_R (0x2143c)
#define SPI3CON2        PIC32_R (0x21440) /* Control 2 */
#define SPI3CON2CLR     PIC32_R (0x21444)
#define SPI3CON2SET     PIC32_R (0x21448)
#define SPI3CON2INV     PIC32_R (0x2144c)

#define SPI4CON         PIC32_R (0x21600) /* Control */
#define SPI4CONCLR      PIC32_R (0x21604)
#define SPI4CONSET      PIC32_R (0x21608)
#define SPI4CONINV      PIC32_R (0x2160c)
#define SPI4STAT        PIC32_R (0x21610) /* Status */
#define SPI4STATCLR     PIC32_R (0x21614)
#define SPI4STATSET     PIC32_R (0x21618)
#define SPI4STATINV     PIC32_R (0x2161c)
#define SPI4BUF         PIC32_R (0x21620) /* Transmit and receive buffer */
#define SPI4BRG         PIC32_R (0x21630) /* Baud rate generator */
#define SPI4BRGCLR      PIC32_R (0x21634)
#define SPI4BRGSET      PIC32_R (0x21638)
#define SPI4BRGINV      PIC32_R (0x2163c)
#define SPI4CON2        PIC32_R (0x21640) /* Control 2 */
#define SPI4CON2CLR     PIC32_R (0x21644)
#define SPI4CON2SET     PIC32_R (0x21648)
#define SPI4CON2INV     PIC32_R (0x2164c)

#define SPI5CON         PIC32_R (0x21800) /* Control */
#define SPI5CONCLR      PIC32_R (0x21804)
#define SPI5CONSET      PIC32_R (0x21808)
#define SPI5CONINV      PIC32_R (0x2180c)
#define SPI5STAT        PIC32_R (0x21810) /* Status */
#define SPI5STATCLR     PIC32_R (0x21814)
#define SPI5STATSET     PIC32_R (0x21818)
#define SPI5STATINV     PIC32_R (0x2181c)
#define SPI5BUF         PIC32_R (0x21820) /* Transmit and receive buffer */
#define SPI5BRG         PIC32_R (0x21830) /* Baud rate generator */
#define SPI5BRGCLR      PIC32_R (0x21834)
#define SPI5BRGSET      PIC32_R (0x21838)
#define SPI5BRGINV      PIC32_R (0x2183c)
#define SPI5CON2        PIC32_R (0x21840) /* Control 2 */
#define SPI5CON2CLR     PIC32_R (0x21844)
#define SPI5CON2SET     PIC32_R (0x21848)
#define SPI5CON2INV     PIC32_R (0x2184c)

#define SPI6CON         PIC32_R (0x21a00) /* Control */
#define SPI6CONCLR      PIC32_R (0x21a04)
#define SPI6CONSET      PIC32_R (0x21a08)
#define SPI6CONINV      PIC32_R (0x21a0c)
#define SPI6STAT        PIC32_R (0x21a10) /* Status */
#define SPI6STATCLR     PIC32_R (0x21a14)
#define SPI6STATSET     PIC32_R (0x21a18)
#define SPI6STATINV     PIC32_R (0x21a1c)
#define SPI6BUF         PIC32_R (0x21a20) /* Transmit and receive buffer */
#define SPI6BRG         PIC32_R (0x21a30) /* Baud rate generator */
#define SPI6BRGCLR      PIC32_R (0x21a34)
#define SPI6BRGSET      PIC32_R (0x21a38)
#define SPI6BRGINV      PIC32_R (0x21a3c)
#define SPI6CON2        PIC32_R (0x21a40) /* Control 2 */
#define SPI6CON2CLR     PIC32_R (0x21a44)
#define SPI6CON2SET     PIC32_R (0x21a48)
#define SPI6CON2INV     PIC32_R (0x21a4c)

/*
 * SPI Control register.
 */
#define PIC32_SPICON_MSTEN      0x00000020      /* Master mode */
#define PIC32_SPICON_CKP        0x00000040      /* Idle clock is high level */
#define PIC32_SPICON_SSEN       0x00000080      /* Slave mode: SSx pin enable */
#define PIC32_SPICON_CKE        0x00000100      /* Output data changes on
                                                 * transition from active clock
                                                 * state to Idle clock state */
#define PIC32_SPICON_SMP        0x00000200      /* Master mode: input data sampled
                                                 * at end of data output time. */
#define PIC32_SPICON_MODE16     0x00000400      /* 16-bit data width */
#define PIC32_SPICON_MODE32     0x00000800      /* 32-bit data width */
#define PIC32_SPICON_DISSDO     0x00001000      /* SDOx pin is not used */
#define PIC32_SPICON_SIDL       0x00002000      /* Stop in Idle mode */
#define PIC32_SPICON_FRZ        0x00004000      /* Freeze in Debug mode */
#define PIC32_SPICON_ON         0x00008000      /* SPI Peripheral is enabled */
#define PIC32_SPICON_ENHBUF     0x00010000      /* Enhanced buffer enable */
#define PIC32_SPICON_SPIFE      0x00020000      /* Frame synchronization pulse
                                                 * coincides with the first bit clock */
#define PIC32_SPICON_FRMPOL     0x20000000      /* Frame pulse is active-high */
#define PIC32_SPICON_FRMSYNC    0x40000000      /* Frame sync pulse input (Slave mode) */
#define PIC32_SPICON_FRMEN      0x80000000      /* Framed SPI support */

/*
 * SPI Status register.
 */
#define PIC32_SPISTAT_SPIRBF    0x00000001      /* Receive buffer is full */
#define PIC32_SPISTAT_SPITBF    0x00000002      /* Transmit buffer is full */
#define PIC32_SPISTAT_SPITBE    0x00000008      /* Transmit buffer is empty */
#define PIC32_SPISTAT_SPIRBE    0x00000020      /* Receive buffer is empty */
#define PIC32_SPISTAT_SPIROV    0x00000040      /* Receive overflow flag */
#define PIC32_SPISTAT_SPIBUSY   0x00000800      /* SPI is busy */

/*--------------------------------------
 * USB registers.
 */
#define USBCSR0         PIC32_R (0xE3000)   /*  */
#define USBCSR1         PIC32_R (0xE3004)
#define USBCSR2         PIC32_R (0xE3008)
#define USBCSR3         PIC32_R (0xE300C)
#define USBIENCSR0      PIC32_R (0xE3010)   /*  */
#define USBIENCSR1      PIC32_R (0xE3014)
#define USBIENCSR2      PIC32_R (0xE3018)
#define USBIENCSR3      PIC32_R (0xE301C)
#define USBFIFO0        PIC32_R (0xE3020)   /*  */
#define USBFIFO1        PIC32_R (0xE3024)
#define USBFIFO2        PIC32_R (0xE3028)
#define USBFIFO3        PIC32_R (0xE302C)
#define USBFIFO4        PIC32_R (0xE3030)
#define USBFIFO5        PIC32_R (0xE3034)
#define USBFIFO6        PIC32_R (0xE3038)
#define USBFIFO7        PIC32_R (0xE303C)
#define USBOTG          PIC32_R (0xE3060)   /*  */
#define USBFIFOA        PIC32_R (0xE3064)   /*  */
#define USBHWVER        PIC32_R (0xE306C)   /*  */
#define USBINFO         PIC32_R (0xE3078)   /*  */
#define USBEOFRST       PIC32_R (0xE307C)   /*  */
#define USBE0TXA        PIC32_R (0xE3080)   /*  */
#define USBE0RXA        PIC32_R (0xE3084)   /*  */
#define USBE1TXA        PIC32_R (0xE3088)
#define USBE1RXA        PIC32_R (0xE308C)
#define USBE2TXA        PIC32_R (0xE3090)
#define USBE2RXA        PIC32_R (0xE3094)
#define USBE3TXA        PIC32_R (0xE3098)
#define USBE3RXA        PIC32_R (0xE309C)
#define USBE4TXA        PIC32_R (0xE30A0)
#define USBE4RXA        PIC32_R (0xE30A4)
#define USBE5TXA        PIC32_R (0xE30A8)
#define USBE5RXA        PIC32_R (0xE30AC)
#define USBE6TXA        PIC32_R (0xE30B0)
#define USBE6RXA        PIC32_R (0xE30B4)
#define USBE7TXA        PIC32_R (0xE30B8)
#define USBE7RXA        PIC32_R (0xE30BC)
#define USBE0CSR0       PIC32_R (0xE3100)   /*  */
#define USBE0CSR2       PIC32_R (0xE3108)
#define USBE0CSR3       PIC32_R (0xE310C)
#define USBE1CSR0       PIC32_R (0xE3110)
#define USBE1CSR1       PIC32_R (0xE3114)
#define USBE1CSR2       PIC32_R (0xE3118)
#define USBE1CSR3       PIC32_R (0xE311C)
#define USBE2CSR0       PIC32_R (0xE3120)
#define USBE2CSR1       PIC32_R (0xE3124)
#define USBE2CSR2       PIC32_R (0xE3128)
#define USBE2CSR3       PIC32_R (0xE312C)
#define USBE3CSR0       PIC32_R (0xE3130)
#define USBE3CSR1       PIC32_R (0xE3134)
#define USBE3CSR2       PIC32_R (0xE3138)
#define USBE3CSR3       PIC32_R (0xE313C)
#define USBE4CSR0       PIC32_R (0xE3140)
#define USBE4CSR1       PIC32_R (0xE3144)
#define USBE4CSR2       PIC32_R (0xE3148)
#define USBE4CSR3       PIC32_R (0xE314C)
#define USBE5CSR0       PIC32_R (0xE3150)
#define USBE5CSR1       PIC32_R (0xE3154)
#define USBE5CSR2       PIC32_R (0xE3158)
#define USBE5CSR3       PIC32_R (0xE315C)
#define USBE6CSR0       PIC32_R (0xE3160)
#define USBE6CSR1       PIC32_R (0xE3164)
#define USBE6CSR2       PIC32_R (0xE3168)
#define USBE6CSR3       PIC32_R (0xE316C)
#define USBE7CSR0       PIC32_R (0xE3170)
#define USBE7CSR1       PIC32_R (0xE3174)
#define USBE7CSR2       PIC32_R (0xE3178)
#define USBE7CSR3       PIC32_R (0xE317C)
#define USBDMAINT       PIC32_R (0xE3200)   /*  */
#define USBDMA1C        PIC32_R (0xE3204)   /*  */
#define USBDMA1A        PIC32_R (0xE3208)   /*  */
#define USBDMA1N        PIC32_R (0xE320C)   /*  */
#define USBDMA2C        PIC32_R (0xE3214)
#define USBDMA2A        PIC32_R (0xE3218)
#define USBDMA2N        PIC32_R (0xE321C)
#define USBDMA3C        PIC32_R (0xE3224)
#define USBDMA3A        PIC32_R (0xE3228)
#define USBDMA3N        PIC32_R (0xE322C)
#define USBDMA4C        PIC32_R (0xE3234)
#define USBDMA4A        PIC32_R (0xE3238)
#define USBDMA4N        PIC32_R (0xE323C)
#define USBDMA5C        PIC32_R (0xE3244)
#define USBDMA5A        PIC32_R (0xE3248)
#define USBDMA5N        PIC32_R (0xE324C)
#define USBDMA6C        PIC32_R (0xE3254)
#define USBDMA6A        PIC32_R (0xE3258)
#define USBDMA6N        PIC32_R (0xE325C)
#define USBDMA7C        PIC32_R (0xE3264)
#define USBDMA7A        PIC32_R (0xE3268)
#define USBDMA7N        PIC32_R (0xE326C)
#define USBDMA8C        PIC32_R (0xE3274)
#define USBDMA8A        PIC32_R (0xE3278)
#define USBDMA8N        PIC32_R (0xE327C)
#define USBE1RPC        PIC32_R (0xE3304)   /*  */
#define USBE2RPC        PIC32_R (0xE3308)
#define USBE3RPC        PIC32_R (0xE330C)
#define USBE4RPC        PIC32_R (0xE3310)
#define USBE5RPC        PIC32_R (0xE3314)
#define USBE6RPC        PIC32_R (0xE3318)
#define USBE7RPC        PIC32_R (0xE331C)
#define USBDPBFD        PIC32_R (0xE3340)   /*  */
#define USBTMCON1       PIC32_R (0xE3344)   /*  */
#define USBTMCON2       PIC32_R (0xE3348)   /*  */
#define USBLPMR1        PIC32_R (0xE3360)   /*  */
#define USBLMPR2        PIC32_R (0xE3364)   /*  */

/*--------------------------------------
* DMA registers.
*/
		
#define   DMACON           PIC32_R ( 0x11000 )
#define   DMACONCLR        PIC32_R ( 0x11004 )
#define   DMACONSET        PIC32_R ( 0x11008 )
#define   DMACONINV        PIC32_R ( 0x1100C )
#define   DMASTAT          PIC32_R ( 0x11010 )
#define   DMASTATCLR       PIC32_R ( 0x11014 )
#define   DMASTATSET       PIC32_R ( 0x11018 )
#define   DMASTATINV       PIC32_R ( 0x1101C )
#define   DMAADDR          PIC32_R ( 0x11020 )
#define   DMAADDRCLR       PIC32_R ( 0x11024 )
#define   DMAADDRSET       PIC32_R ( 0x11028 )
#define   DMAADDRINV       PIC32_R ( 0x1102C )
#define   DCH0CON          PIC32_R ( 0x11060 )
#define   DCH0CONCLR       PIC32_R ( 0x11064 )
#define   DCH0CONSET       PIC32_R ( 0x11068 )
#define   DCH0CONINV       PIC32_R ( 0x1106C )
#define   DCH0ECON         PIC32_R ( 0x11070 )
#define   DCH0ECONCLR      PIC32_R ( 0x11074 )
#define   DCH0ECONSET      PIC32_R ( 0x11078 )
#define   DCH0ECONINV      PIC32_R ( 0x1107C )
#define   DCH0INT          PIC32_R ( 0x11080 )
#define   DCH0INTCLR       PIC32_R ( 0x11084 )
#define   DCH0INTSET       PIC32_R ( 0x11088 )
#define   DCH0INTINV       PIC32_R ( 0x1108C )
#define   DCH0SSA          PIC32_R ( 0x11090 )
#define   DCH0SSACLR       PIC32_R ( 0x11094 )
#define   DCH0SSASET       PIC32_R ( 0x11098 )
#define   DCH0SSAINV       PIC32_R ( 0x1109C )
#define   DCH0DSA          PIC32_R ( 0x110A0 )
#define   DCH0DSACLR       PIC32_R ( 0x110A4 )
#define   DCH0DSASET       PIC32_R ( 0x110A8 )
#define   DCH0DSAINV       PIC32_R ( 0x110AC )
#define   DCH0SSIZ         PIC32_R ( 0x110B0 )
#define   DCH0SSIZCLR      PIC32_R ( 0x110B4 )
#define   DCH0SSIZSET      PIC32_R ( 0x110B8 )
#define   DCH0SSIZINV      PIC32_R ( 0x110BC )
#define   DCH0DSIZ         PIC32_R ( 0x110C0 )
#define   DCH0DSIZCLR      PIC32_R ( 0x110C4 )
#define   DCH0DSIZSET      PIC32_R ( 0x110C8 )
#define   DCH0DSIZINV      PIC32_R ( 0x110CC )
#define   DCH0SPTR         PIC32_R ( 0x110D0 )
#define   DCH0SPTRCLR      PIC32_R ( 0x110D4 )
#define   DCH0SPTRSET      PIC32_R ( 0x110D8 )
#define   DCH0SPTRINV      PIC32_R ( 0x110DC )
#define   DCH0DPTR         PIC32_R ( 0x110E0 )
#define   DCH0DPTRCLR      PIC32_R ( 0x110E4 )
#define   DCH0DPTRSET      PIC32_R ( 0x110E8 )
#define   DCH0DPTRINV      PIC32_R ( 0x110EC )
#define   DCH0CSIZ         PIC32_R ( 0x110F0 )
#define   DCH0CSIZCLR      PIC32_R ( 0x110F4 )
#define   DCH0CSIZSET      PIC32_R ( 0x110F8 )
#define   DCH0CSIZINV      PIC32_R ( 0x110FC )
#define   DCH0CPTR         PIC32_R ( 0x11100 )
#define   DCS0CPTR         PIC32_R ( 0x11100 )
#define   DCH0CPTRCLR      PIC32_R ( 0x11104 )
#define   DCS0CPTRCLR      PIC32_R ( 0x11104 )
#define   DCH0CPTRSET      PIC32_R ( 0x11108 )
#define   DCS0CPTRSET      PIC32_R ( 0x11108 )
#define   DCH0CPTRINV      PIC32_R ( 0x1110C )
#define   DCS0CPTRINV      PIC32_R ( 0x1110C )
#define   DCH0DAT          PIC32_R ( 0x11110 )
#define   DCH0DATCLR       PIC32_R ( 0x11114 )
#define   DCH0DATSET       PIC32_R ( 0x11118 )
#define   DCH0DATINV       PIC32_R ( 0x1111C )
#define   DCH1CON          PIC32_R ( 0x11120 )
#define   DCH1CONCLR       PIC32_R ( 0x11124 )
#define   DCH1CONSET       PIC32_R ( 0x11128 )
#define   DCH1CONINV       PIC32_R ( 0x1112C )
#define   DCH1ECON         PIC32_R ( 0x11130 )
#define   DCH1ECONCLR      PIC32_R ( 0x11134 )
#define   DCH1ECONSET      PIC32_R ( 0x11138 )
#define   DCH1ECONINV      PIC32_R ( 0x1113C )
#define   DCH1INT          PIC32_R ( 0x11140 )
#define   DCH1INTCLR       PIC32_R ( 0x11144 )
#define   DCH1INTSET       PIC32_R ( 0x11148 )
#define   DCH1INTINV       PIC32_R ( 0x1114C )
#define   DCH1SSA          PIC32_R ( 0x11150 )
#define   DCH1SSACLR       PIC32_R ( 0x11154 )
#define   DCH1SSASET       PIC32_R ( 0x11158 )
#define   DCH1SSAINV       PIC32_R ( 0x1115C )
#define   DCH1DSA          PIC32_R ( 0x11160 )
#define   DCH1DSACLR       PIC32_R ( 0x11164 )
#define   DCH1DSASET       PIC32_R ( 0x11168 )
#define   DCH1DSAINV       PIC32_R ( 0x1116C )
#define   DCH1SSIZ         PIC32_R ( 0x11170 )
#define   DCH1SSIZCLR      PIC32_R ( 0x11174 )
#define   DCH1SSIZSET      PIC32_R ( 0x11178 )
#define   DCH1SSIZINV      PIC32_R ( 0x1117C )
#define   DCH1DSIZ         PIC32_R ( 0x11180 )
#define   DCH1DSIZCLR      PIC32_R ( 0x11184 )
#define   DCH1DSIZSET      PIC32_R ( 0x11188 )
#define   DCH1DSIZINV      PIC32_R ( 0x1118C )
#define   DCH1SPTR         PIC32_R ( 0x11190 )
#define   DCH1SPTRCLR      PIC32_R ( 0x11194 )
#define   DCH1SPTRSET      PIC32_R ( 0x11198 )
#define   DCH1SPTRINV      PIC32_R ( 0x1119C )
#define   DCH1DPTR         PIC32_R ( 0x111A0 )
#define   DCH1DPTRCLR      PIC32_R ( 0x111A4 )
#define   DCH1DPTRSET      PIC32_R ( 0x111A8 )
#define   DCH1DPTRINV      PIC32_R ( 0x111AC )
#define   DCH1CSIZ         PIC32_R ( 0x111B0 )
#define   DCH1CSIZCLR      PIC32_R ( 0x111B4 )
#define   DCH1CSIZSET      PIC32_R ( 0x111B8 )
#define   DCH1CSIZINV      PIC32_R ( 0x111BC )
#define   DCH1CPTR         PIC32_R ( 0x111C0 )
#define   DCS1CPTR         PIC32_R ( 0x111C0 )
#define   DCH1CPTRCLR      PIC32_R ( 0x111C4 )
#define   DCS1CPTRCLR      PIC32_R ( 0x111C4 )
#define   DCH1CPTRSET      PIC32_R ( 0x111C8 )
#define   DCS1CPTRSET      PIC32_R ( 0x111C8 )
#define   DCH1CPTRINV      PIC32_R ( 0x111CC )
#define   DCS1CPTRINV      PIC32_R ( 0x111CC )
#define   DCH1DAT          PIC32_R ( 0x111D0 )
#define   DCH1DATCLR       PIC32_R ( 0x111D4 )
#define   DCH1DATSET       PIC32_R ( 0x111D8 )
#define   DCH1DATINV       PIC32_R ( 0x111DC )
#define   DCH2CON          PIC32_R ( 0x111E0 )
#define   DCH2CONCLR       PIC32_R ( 0x111E4 )
#define   DCH2CONSET       PIC32_R ( 0x111E8 )
#define   DCH2CONINV       PIC32_R ( 0x111EC )
#define   DCH2ECON         PIC32_R ( 0x111F0 )
#define   DCH2ECONCLR      PIC32_R ( 0x111F4 )
#define   DCH2ECONSET      PIC32_R ( 0x111F8 )
#define   DCH2ECONINV      PIC32_R ( 0x111FC )
#define   DCH2INT          PIC32_R ( 0x11200 )
#define   DCH2INTCLR       PIC32_R ( 0x11204 )
#define   DCH2INTSET       PIC32_R ( 0x11208 )
#define   DCH2INTINV       PIC32_R ( 0x1120C )
#define   DCH2SSA          PIC32_R ( 0x11210 )
#define   DCH2SSACLR       PIC32_R ( 0x11214 )
#define   DCH2SSASET       PIC32_R ( 0x11218 )
#define   DCH2SSAINV       PIC32_R ( 0x1121C )
#define   DCH2DSA          PIC32_R ( 0x11220 )
#define   DCH2DSACLR       PIC32_R ( 0x11224 )
#define   DCH2DSASET       PIC32_R ( 0x11228 )
#define   DCH2DSAINV       PIC32_R ( 0x1122C )
#define   DCH2SSIZ         PIC32_R ( 0x11230 )
#define   DCH2SSIZCLR      PIC32_R ( 0x11234 )
#define   DCH2SSIZSET      PIC32_R ( 0x11238 )
#define   DCH2SSIZINV      PIC32_R ( 0x1123C )
#define   DCH2DSIZ         PIC32_R ( 0x11240 )
#define   DCH2DSIZCLR      PIC32_R ( 0x11244 )
#define   DCH2DSIZSET      PIC32_R ( 0x11248 )
#define   DCH2DSIZINV      PIC32_R ( 0x1124C )
#define   DCH2SPTR         PIC32_R ( 0x11250 )
#define   DCH2SPTRCLR      PIC32_R ( 0x11254 )
#define   DCH2SPTRSET      PIC32_R ( 0x11258 )
#define   DCH2SPTRINV      PIC32_R ( 0x1125C )
#define   DCH2DPTR         PIC32_R ( 0x11260 )
#define   DCH2DPTRCLR      PIC32_R ( 0x11264 )
#define   DCH2DPTRSET      PIC32_R ( 0x11268 )
#define   DCH2DPTRINV      PIC32_R ( 0x1126C )
#define   DCH2CSIZ         PIC32_R ( 0x11270 )
#define   DCH2CSIZCLR      PIC32_R ( 0x11274 )
#define   DCH2CSIZSET      PIC32_R ( 0x11278 )
#define   DCH2CSIZINV      PIC32_R ( 0x1127C )
#define   DCH2CPTR         PIC32_R ( 0x11280 )
#define   DCS2CPTR         PIC32_R ( 0x11280 )
#define   DCH2CPTRCLR      PIC32_R ( 0x11284 )
#define   DCS2CPTRCLR      PIC32_R ( 0x11284 )
#define   DCH2CPTRSET      PIC32_R ( 0x11288 )
#define   DCS2CPTRSET      PIC32_R ( 0x11288 )
#define   DCH2CPTRINV      PIC32_R ( 0x1128C )
#define   DCS2CPTRINV      PIC32_R ( 0x1128C )
#define   DCH2DAT          PIC32_R ( 0x11290 )
#define   DCH2DATCLR       PIC32_R ( 0x11294 )
#define   DCH2DATSET       PIC32_R ( 0x11298 )
#define   DCH2DATINV       PIC32_R ( 0x1129C )
#define   DCH3CON          PIC32_R ( 0x112A0 )
#define   DCH3CONCLR       PIC32_R ( 0x112A4 )
#define   DCH3CONSET       PIC32_R ( 0x112A8 )
#define   DCH3CONINV       PIC32_R ( 0x112AC )
#define   DCH3ECON         PIC32_R ( 0x112B0 )
#define   DCH3ECONCLR      PIC32_R ( 0x112B4 )
#define   DCH3ECONSET      PIC32_R ( 0x112B8 )
#define   DCH3ECONINV      PIC32_R ( 0x112BC )
#define   DCH3INT          PIC32_R ( 0x112C0 )
#define   DCH3INTCLR       PIC32_R ( 0x112C4 )
#define   DCH3INTSET       PIC32_R ( 0x112C8 )
#define   DCH3INTINV       PIC32_R ( 0x112CC )
#define   DCH3SSA          PIC32_R ( 0x112D0 )
#define   DCH3SSACLR       PIC32_R ( 0x112D4 )
#define   DCH3SSASET       PIC32_R ( 0x112D8 )
#define   DCH3SSAINV       PIC32_R ( 0x112DC )
#define   DCH3DSA          PIC32_R ( 0x112E0 )
#define   DCH3DSACLR       PIC32_R ( 0x112E4 )
#define   DCH3DSASET       PIC32_R ( 0x112E8 )
#define   DCH3DSAINV       PIC32_R ( 0x112EC )
#define   DCH3SSIZ         PIC32_R ( 0x112F0 )
#define   DCH3SSIZCLR      PIC32_R ( 0x112F4 )
#define   DCH3SSIZSET      PIC32_R ( 0x112F8 )
#define   DCH3SSIZINV      PIC32_R ( 0x112FC )
#define   DCH3DSIZ         PIC32_R ( 0x11300 )
#define   DCH3DSIZCLR      PIC32_R ( 0x11304 )
#define   DCH3DSIZSET      PIC32_R ( 0x11308 )
#define   DCH3DSIZINV      PIC32_R ( 0x1130C )
#define   DCH3SPTR         PIC32_R ( 0x11310 )
#define   DCH3SPTRCLR      PIC32_R ( 0x11314 )
#define   DCH3SPTRSET      PIC32_R ( 0x11318 )
#define   DCH3SPTRINV      PIC32_R ( 0x1131C )
#define   DCH3DPTR         PIC32_R ( 0x11320 )
#define   DCH3DPTRCLR      PIC32_R ( 0x11324 )
#define   DCH3DPTRSET      PIC32_R ( 0x11328 )
#define   DCH3DPTRINV      PIC32_R ( 0x1132C )
#define   DCH3CSIZ         PIC32_R ( 0x11330 )
#define   DCH3CSIZCLR      PIC32_R ( 0x11334 )
#define   DCH3CSIZSET      PIC32_R ( 0x11338 )
#define   DCH3CSIZINV      PIC32_R ( 0x1133C )
#define   DCH3CPTR         PIC32_R ( 0x11340 )
#define   DCS3CPTR         PIC32_R ( 0x11340 )
#define   DCH3CPTRCLR      PIC32_R ( 0x11344 )
#define   DCS3CPTRCLR      PIC32_R ( 0x11344 )
#define   DCH3CPTRSET      PIC32_R ( 0x11348 )
#define   DCS3CPTRSET      PIC32_R ( 0x11348 )
#define   DCH3CPTRINV      PIC32_R ( 0x1134C )
#define   DCS3CPTRINV      PIC32_R ( 0x1134C )
#define   DCH3DAT          PIC32_R ( 0x11350 )
#define   DCH3DATCLR       PIC32_R ( 0x11354 )
#define   DCH3DATSET       PIC32_R ( 0x11358 )
#define   DCH3DATINV       PIC32_R ( 0x1135C )
#define   DCH4CON          PIC32_R ( 0x11360 )
#define   DCH4CONCLR       PIC32_R ( 0x11364 )
#define   DCH4CONSET       PIC32_R ( 0x11368 )
#define   DCH4CONINV       PIC32_R ( 0x1136C )
#define   DCH4ECON         PIC32_R ( 0x11370 )
#define   DCH4ECONCLR      PIC32_R ( 0x11374 )
#define   DCH4ECONSET      PIC32_R ( 0x11378 )
#define   DCH4ECONINV      PIC32_R ( 0x1137C )
#define   DCH4INT          PIC32_R ( 0x11380 )
#define   DCH4INTCLR       PIC32_R ( 0x11384 )
#define   DCH4INTSET       PIC32_R ( 0x11388 )
#define   DCH4INTINV       PIC32_R ( 0x1138C )
#define   DCH4SSA          PIC32_R ( 0x11390 )
#define   DCH4SSACLR       PIC32_R ( 0x11394 )
#define   DCH4SSASET       PIC32_R ( 0x11398 )
#define   DCH4SSAINV       PIC32_R ( 0x1139C )
#define   DCH4DSA          PIC32_R ( 0x113A0 )
#define   DCH4DSACLR       PIC32_R ( 0x113A4 )
#define   DCH4DSASET       PIC32_R ( 0x113A8 )
#define   DCH4DSAINV       PIC32_R ( 0x113AC )
#define   DCH4SSIZ         PIC32_R ( 0x113B0 )
#define   DCH4SSIZCLR      PIC32_R ( 0x113B4 )
#define   DCH4SSIZSET      PIC32_R ( 0x113B8 )
#define   DCH4SSIZINV      PIC32_R ( 0x113BC )
#define   DCH4DSIZ         PIC32_R ( 0x113C0 )
#define   DCH4DSIZCLR      PIC32_R ( 0x113C4 )
#define   DCH4DSIZSET      PIC32_R ( 0x113C8 )
#define   DCH4DSIZINV      PIC32_R ( 0x113CC )
#define   DCH4SPTR         PIC32_R ( 0x113D0 )
#define   DCH4SPTRCLR      PIC32_R ( 0x113D4 )
#define   DCH4SPTRSET      PIC32_R ( 0x113D8 )
#define   DCH4SPTRINV      PIC32_R ( 0x113DC )
#define   DCH4DPTR         PIC32_R ( 0x113E0 )
#define   DCH4DPTRCLR      PIC32_R ( 0x113E4 )
#define   DCH4DPTRSET      PIC32_R ( 0x113E8 )
#define   DCH4DPTRINV      PIC32_R ( 0x113EC )
#define   DCH4CSIZ         PIC32_R ( 0x113F0 )
#define   DCH4CSIZCLR      PIC32_R ( 0x113F4 )
#define   DCH4CSIZSET      PIC32_R ( 0x113F8 )
#define   DCH4CSIZINV      PIC32_R ( 0x113FC )
#define   DCH4CPTR         PIC32_R ( 0x11400 )
#define   DCS4CPTR         PIC32_R ( 0x11400 )
#define   DCH4CPTRCLR      PIC32_R ( 0x11404 )
#define   DCS4CPTRCLR      PIC32_R ( 0x11404 )
#define   DCH4CPTRSET      PIC32_R ( 0x11408 )
#define   DCS4CPTRSET      PIC32_R ( 0x11408 )
#define   DCH4CPTRINV      PIC32_R ( 0x1140C )
#define   DCS4CPTRINV      PIC32_R ( 0x1140C )
#define   DCH4DAT          PIC32_R ( 0x11410 )
#define   DCH4DATCLR       PIC32_R ( 0x11414 )
#define   DCH4DATSET       PIC32_R ( 0x11418 )
#define   DCH4DATINV       PIC32_R ( 0x1141C )
#define   DCH5CON          PIC32_R ( 0x11420 )
#define   DCH5CONCLR       PIC32_R ( 0x11424 )
#define   DCH5CONSET       PIC32_R ( 0x11428 )
#define   DCH5CONINV       PIC32_R ( 0x1142C )
#define   DCH5ECON         PIC32_R ( 0x11430 )
#define   DCH5ECONCLR      PIC32_R ( 0x11434 )
#define   DCH5ECONSET      PIC32_R ( 0x11438 )
#define   DCH5ECONINV      PIC32_R ( 0x1143C )
#define   DCH5INT          PIC32_R ( 0x11440 )
#define   DCH5INTCLR       PIC32_R ( 0x11444 )
#define   DCH5INTSET       PIC32_R ( 0x11448 )
#define   DCH5INTINV       PIC32_R ( 0x1144C )
#define   DCH5SSA          PIC32_R ( 0x11450 )
#define   DCH5SSACLR       PIC32_R ( 0x11454 )
#define   DCH5SSASET       PIC32_R ( 0x11458 )
#define   DCH5SSAINV       PIC32_R ( 0x1145C )
#define   DCH5DSA          PIC32_R ( 0x11460 )
#define   DCH5DSACLR       PIC32_R ( 0x11464 )
#define   DCH5DSASET       PIC32_R ( 0x11468 )
#define   DCH5DSAINV       PIC32_R ( 0x1146C )
#define   DCH5SSIZ         PIC32_R ( 0x11470 )
#define   DCH5SSIZCLR      PIC32_R ( 0x11474 )
#define   DCH5SSIZSET      PIC32_R ( 0x11478 )
#define   DCH5SSIZINV      PIC32_R ( 0x1147C )
#define   DCH5DSIZ         PIC32_R ( 0x11480 )
#define   DCH5DSIZCLR      PIC32_R ( 0x11484 )
#define   DCH5DSIZSET      PIC32_R ( 0x11488 )
#define   DCH5DSIZINV      PIC32_R ( 0x1148C )
#define   DCH5SPTR         PIC32_R ( 0x11490 )
#define   DCH5SPTRCLR      PIC32_R ( 0x11494 )
#define   DCH5SPTRSET      PIC32_R ( 0x11498 )
#define   DCH5SPTRINV      PIC32_R ( 0x1149C )
#define   DCH5DPTR         PIC32_R ( 0x114A0 )
#define   DCH5DPTRCLR      PIC32_R ( 0x114A4 )
#define   DCH5DPTRSET      PIC32_R ( 0x114A8 )
#define   DCH5DPTRINV      PIC32_R ( 0x114AC )
#define   DCH5CSIZ         PIC32_R ( 0x114B0 )
#define   DCH5CSIZCLR      PIC32_R ( 0x114B4 )
#define   DCH5CSIZSET      PIC32_R ( 0x114B8 )
#define   DCH5CSIZINV      PIC32_R ( 0x114BC )
#define   DCH5CPTR         PIC32_R ( 0x114C0 )
#define   DCS5CPTR         PIC32_R ( 0x114C0 )
#define   DCH5CPTRCLR      PIC32_R ( 0x114C4 )
#define   DCS5CPTRCLR      PIC32_R ( 0x114C4 )
#define   DCH5CPTRSET      PIC32_R ( 0x114C8 )
#define   DCS5CPTRSET      PIC32_R ( 0x114C8 )
#define   DCH5CPTRINV      PIC32_R ( 0x114CC )
#define   DCS5CPTRINV      PIC32_R ( 0x114CC )
#define   DCH5DAT          PIC32_R ( 0x114D0 )
#define   DCH5DATCLR       PIC32_R ( 0x114D4 )
#define   DCH5DATSET       PIC32_R ( 0x114D8 )
#define   DCH5DATINV       PIC32_R ( 0x114DC )
#define   DCH6CON          PIC32_R ( 0x114E0 )
#define   DCH6CONCLR       PIC32_R ( 0x114E4 )
#define   DCH6CONSET       PIC32_R ( 0x114E8 )
#define   DCH6CONINV       PIC32_R ( 0x114EC )
#define   DCH6ECON         PIC32_R ( 0x114F0 )
#define   DCH6ECONCLR      PIC32_R ( 0x114F4 )
#define   DCH6ECONSET      PIC32_R ( 0x114F8 )
#define   DCH6ECONINV      PIC32_R ( 0x114FC )
#define   DCH6INT          PIC32_R ( 0x11500 )
#define   DCH6INTCLR       PIC32_R ( 0x11504 )
#define   DCH6INTSET       PIC32_R ( 0x11508 )
#define   DCH6INTINV       PIC32_R ( 0x1150C )
#define   DCH6SSA          PIC32_R ( 0x11510 )
#define   DCH6SSACLR       PIC32_R ( 0x11514 )
#define   DCH6SSASET       PIC32_R ( 0x11518 )
#define   DCH6SSAINV       PIC32_R ( 0x1151C )
#define   DCH6DSA          PIC32_R ( 0x11520 )
#define   DCH6DSACLR       PIC32_R ( 0x11524 )
#define   DCH6DSASET       PIC32_R ( 0x11528 )
#define   DCH6DSAINV       PIC32_R ( 0x1152C )
#define   DCH6SSIZ         PIC32_R ( 0x11530 )
#define   DCH6SSIZCLR      PIC32_R ( 0x11534 )
#define   DCH6SSIZSET      PIC32_R ( 0x11538 )
#define   DCH6SSIZINV      PIC32_R ( 0x1153C )
#define   DCH6DSIZ         PIC32_R ( 0x11540 )
#define   DCH6DSIZCLR      PIC32_R ( 0x11544 )
#define   DCH6DSIZSET      PIC32_R ( 0x11548 )
#define   DCH6DSIZINV      PIC32_R ( 0x1154C )
#define   DCH6SPTR         PIC32_R ( 0x11550 )
#define   DCH6SPTRCLR      PIC32_R ( 0x11554 )
#define   DCH6SPTRSET      PIC32_R ( 0x11558 )
#define   DCH6SPTRINV      PIC32_R ( 0x1155C )
#define   DCH6DPTR         PIC32_R ( 0x11560 )
#define   DCH6DPTRCLR      PIC32_R ( 0x11564 )
#define   DCH6DPTRSET      PIC32_R ( 0x11568 )
#define   DCH6DPTRINV      PIC32_R ( 0x1156C )
#define   DCH6CSIZ         PIC32_R ( 0x11570 )
#define   DCH6CSIZCLR      PIC32_R ( 0x11574 )
#define   DCH6CSIZSET      PIC32_R ( 0x11578 )
#define   DCH6CSIZINV      PIC32_R ( 0x1157C )
#define   DCH6CPTR         PIC32_R ( 0x11580 )
#define   DCS6CPTR         PIC32_R ( 0x11580 )
#define   DCH6CPTRCLR      PIC32_R ( 0x11584 )
#define   DCS6CPTRCLR      PIC32_R ( 0x11584 )
#define   DCH6CPTRSET      PIC32_R ( 0x11588 )
#define   DCS6CPTRSET      PIC32_R ( 0x11588 )
#define   DCH6CPTRINV      PIC32_R ( 0x1158C )
#define   DCS6CPTRINV      PIC32_R ( 0x1158C )
#define   DCH6DAT          PIC32_R ( 0x11590 )
#define   DCH6DATCLR       PIC32_R ( 0x11594 )
#define   DCH6DATSET       PIC32_R ( 0x11598 )
#define   DCH6DATINV       PIC32_R ( 0x1159C )
#define   DCH7CON          PIC32_R ( 0x115A0 )
#define   DCH7CONCLR       PIC32_R ( 0x115A4 )
#define   DCH7CONSET       PIC32_R ( 0x115A8 )
#define   DCH7CONINV       PIC32_R ( 0x115AC )
#define   DCH7ECON         PIC32_R ( 0x115B0 )
#define   DCH7ECONCLR      PIC32_R ( 0x115B4 )
#define   DCH7ECONSET      PIC32_R ( 0x115B8 )
#define   DCH7ECONINV      PIC32_R ( 0x115BC )
#define   DCH7INT          PIC32_R ( 0x115C0 )
#define   DCH7INTCLR       PIC32_R ( 0x115C4 )
#define   DCH7INTSET       PIC32_R ( 0x115C8 )
#define   DCH7INTINV       PIC32_R ( 0x115CC )
#define   DCH7SSA          PIC32_R ( 0x115D0 )
#define   DCH7SSACLR       PIC32_R ( 0x115D4 )
#define   DCH7SSASET       PIC32_R ( 0x115D8 )
#define   DCH7SSAINV       PIC32_R ( 0x115DC )
#define   DCH7DSA          PIC32_R ( 0x115E0 )
#define   DCH7DSACLR       PIC32_R ( 0x115E4 )
#define   DCH7DSASET       PIC32_R ( 0x115E8 )
#define   DCH7DSAINV       PIC32_R ( 0x115EC )
#define   DCH7SSIZ         PIC32_R ( 0x115F0 )
#define   DCH7SSIZCLR      PIC32_R ( 0x115F4 )
#define   DCH7SSIZSET      PIC32_R ( 0x115F8 )
#define   DCH7SSIZINV      PIC32_R ( 0x115FC )
#define   DCH7DSIZ         PIC32_R ( 0x11600 )
#define   DCH7DSIZCLR      PIC32_R ( 0x11604 )
#define   DCH7DSIZSET      PIC32_R ( 0x11608 )
#define   DCH7DSIZINV      PIC32_R ( 0x1160C )
#define   DCH7SPTR         PIC32_R ( 0x11610 )
#define   DCH7SPTRCLR      PIC32_R ( 0x11614 )
#define   DCH7SPTRSET      PIC32_R ( 0x11618 )
#define   DCH7SPTRINV      PIC32_R ( 0x1161C )
#define   DCH7DPTR         PIC32_R ( 0x11620 )
#define   DCH7DPTRCLR      PIC32_R ( 0x11624 )
#define   DCH7DPTRSET      PIC32_R ( 0x11628 )
#define   DCH7DPTRINV      PIC32_R ( 0x1162C )
#define   DCH7CSIZ         PIC32_R ( 0x11630 )
#define   DCH7CSIZCLR      PIC32_R ( 0x11634 )
#define   DCH7CSIZSET      PIC32_R ( 0x11638 )
#define   DCH7CSIZINV      PIC32_R ( 0x1163C )
#define   DCH7CPTR         PIC32_R ( 0x11640 )
#define   DCS7CPTR         PIC32_R ( 0x11640 )
#define   DCH7CPTRCLR      PIC32_R ( 0x11644 )
#define   DCS7CPTRCLR      PIC32_R ( 0x11644 )
#define   DCH7CPTRSET      PIC32_R ( 0x11648 )
#define   DCS7CPTRSET      PIC32_R ( 0x11648 )
#define   DCH7CPTRINV      PIC32_R ( 0x1164C )
#define   DCS7CPTRINV      PIC32_R ( 0x1164C )
#define   DCH7DAT          PIC32_R ( 0x11650 )
#define   DCH7DATCLR       PIC32_R ( 0x11654 )
#define   DCH7DATSET       PIC32_R ( 0x11658 )
#define   DCH7DATINV       PIC32_R ( 0x1165C )

/*--------------------------------------
* OUTPUT COMPARE REGISTERS
*/
#define OC1CON           PIC32_R ( 0x44000 )
#define OC1CONCLR        PIC32_R ( 0x44004 )
#define OC1CONSET        PIC32_R ( 0x44008 )
#define OC1CONINV        PIC32_R ( 0x4400C )
#define OC1R             PIC32_R ( 0x44010 )
#define OC1RCLR          PIC32_R ( 0x44014 )
#define OC1RSET          PIC32_R ( 0x44018 )
#define OC1RINV          PIC32_R ( 0x4401C )
#define OC1RS            PIC32_R ( 0x44020 )
#define OC1RSCLR         PIC32_R ( 0x44024 )
#define OC1RSSET         PIC32_R ( 0x44028 )
#define OC1RSINV         PIC32_R ( 0x4402C )
#define OC2CON           PIC32_R ( 0x44200 )
#define OC2CONCLR        PIC32_R ( 0x44204 )
#define OC2CONSET        PIC32_R ( 0x44208 )
#define OC2CONINV        PIC32_R ( 0x4420C )
#define OC2R             PIC32_R ( 0x44210 )
#define OC2RCLR          PIC32_R ( 0x44214 )
#define OC2RSET          PIC32_R ( 0x44218 )
#define OC2RINV          PIC32_R ( 0x4421C )
#define OC2RS            PIC32_R ( 0x44220 )
#define OC2RSCLR         PIC32_R ( 0x44224 )
#define OC2RSSET         PIC32_R ( 0x44228 )
#define OC2RSINV         PIC32_R ( 0x4422C )
#define OC3CON           PIC32_R ( 0x44400 )
#define OC3CONCLR        PIC32_R ( 0x44404 )
#define OC3CONSET        PIC32_R ( 0x44408 )
#define OC3CONINV        PIC32_R ( 0x4440C )
#define OC3R             PIC32_R ( 0x44410 )
#define OC3RCLR          PIC32_R ( 0x44414 )
#define OC3RSET          PIC32_R ( 0x44418 )
#define OC3RINV          PIC32_R ( 0x4441C )
#define OC3RS            PIC32_R ( 0x44420 )
#define OC3RSCLR         PIC32_R ( 0x44424 )
#define OC3RSSET         PIC32_R ( 0x44428 )
#define OC3RSINV         PIC32_R ( 0x4442C )
#define OC4CON           PIC32_R ( 0x44600 )
#define OC4CONCLR        PIC32_R ( 0x44604 )
#define OC4CONSET        PIC32_R ( 0x44608 )
#define OC4CONINV        PIC32_R ( 0x4460C )
#define OC4R             PIC32_R ( 0x44610 )
#define OC4RCLR          PIC32_R ( 0x44614 )
#define OC4RSET          PIC32_R ( 0x44618 )
#define OC4RINV          PIC32_R ( 0x4461C )
#define OC4RS            PIC32_R ( 0x44620 )
#define OC4RSCLR         PIC32_R ( 0x44624 )
#define OC4RSSET         PIC32_R ( 0x44628 )
#define OC4RSINV         PIC32_R ( 0x4462C )
#define OC5CON           PIC32_R ( 0x44800 )
#define OC5CONCLR        PIC32_R ( 0x44804 )
#define OC5CONSET        PIC32_R ( 0x44808 )
#define OC5CONINV        PIC32_R ( 0x4480C )
#define OC5R             PIC32_R ( 0x44810 )
#define OC5RCLR          PIC32_R ( 0x44814 )
#define OC5RSET          PIC32_R ( 0x44818 )
#define OC5RINV          PIC32_R ( 0x4481C )
#define OC5RS            PIC32_R ( 0x44820 )
#define OC5RSCLR         PIC32_R ( 0x44824 )
#define OC5RSSET         PIC32_R ( 0x44828 )
#define OC5RSINV         PIC32_R ( 0x4482C )
#define OC6CON           PIC32_R ( 0x44A00 )
#define OC6CONCLR        PIC32_R ( 0x44A04 )
#define OC6CONSET        PIC32_R ( 0x44A08 )
#define OC6CONINV        PIC32_R ( 0x44A0C )
#define OC6R             PIC32_R ( 0x44A10 )
#define OC6RCLR          PIC32_R ( 0x44A14 )
#define OC6RSET          PIC32_R ( 0x44A18 )
#define OC6RINV          PIC32_R ( 0x44A1C )
#define OC6RS            PIC32_R ( 0x44A20 )
#define OC6RSCLR         PIC32_R ( 0x44A24 )
#define OC6RSSET         PIC32_R ( 0x44A28 )
#define OC6RSINV         PIC32_R ( 0x44A2C )
#define OC7CON           PIC32_R ( 0x44C00 )
#define OC7CONCLR        PIC32_R ( 0x44C04 )
#define OC7CONSET        PIC32_R ( 0x44C08 )
#define OC7CONINV        PIC32_R ( 0x44C0C )
#define OC7R             PIC32_R ( 0x44C10 )
#define OC7RCLR          PIC32_R ( 0x44C14 )
#define OC7RSET          PIC32_R ( 0x44C18 )
#define OC7RINV          PIC32_R ( 0x44C1C )
#define OC7RS            PIC32_R ( 0x44C20 )
#define OC7RSCLR         PIC32_R ( 0x44C24 )
#define OC7RSSET         PIC32_R ( 0x44C28 )
#define OC7RSINV         PIC32_R ( 0x44C2C )
#define OC8CON           PIC32_R ( 0x44E00 )
#define OC8CONCLR        PIC32_R ( 0x44E04 )
#define OC8CONSET        PIC32_R ( 0x44E08 )
#define OC8CONINV        PIC32_R ( 0x44E0C )
#define OC8R             PIC32_R ( 0x44E10 )
#define OC8RCLR          PIC32_R ( 0x44E14 )
#define OC8RSET          PIC32_R ( 0x44E18 )
#define OC8RINV          PIC32_R ( 0x44E1C )
#define OC8RS            PIC32_R ( 0x44E20 )
#define OC8RSCLR         PIC32_R ( 0x44E24 )
#define OC8RSSET         PIC32_R ( 0x44E28 )
#define OC8RSINV         PIC32_R ( 0x44E2C )
#define OC9CON           PIC32_R ( 0x45000 )
#define OC9CONCLR        PIC32_R ( 0x45004 )
#define OC9CONSET        PIC32_R ( 0x45008 )
#define OC9CONINV        PIC32_R ( 0x4500C )
#define OC9R             PIC32_R ( 0x45010 )
#define OC9RCLR          PIC32_R ( 0x45014 )
#define OC9RSET          PIC32_R ( 0x45018 )
#define OC9RINV          PIC32_R ( 0x4501C )
#define OC9RS            PIC32_R ( 0x45020 )
#define OC9RSCLR         PIC32_R ( 0x45024 )
#define OC9RSSET         PIC32_R ( 0x45028 )
#define OC9RSINV         PIC32_R ( 0x4502C )


/*--------------------------------------
* Can registers.
*/
#define C1CON            PIC32_R (0x80000)
#define C1CONCLR         PIC32_R (0x80004)
#define C1CONSET         PIC32_R (0x80008)
#define C1CONINV         PIC32_R (0x8000C)
#define C1CFG            PIC32_R (0x80010)
#define C1CFGCLR         PIC32_R (0x80014)
#define C1CFGSET         PIC32_R (0x80018)
#define C1CFGINV         PIC32_R (0x8001C)
#define C1INT            PIC32_R (0x80020)
#define C1INTCLR         PIC32_R (0x80024)
#define C1INTSET         PIC32_R (0x80028)
#define C1INTINV         PIC32_R (0x8002C)
#define C1VEC            PIC32_R (0x80030)
#define C1VECCLR         PIC32_R (0x80034)
#define C1VECSET         PIC32_R (0x80038)
#define C1VECINV         PIC32_R (0x8003C)
#define C1TREC           PIC32_R (0x80040)
#define C1TRECCLR        PIC32_R (0x80044)
#define C1TRECSET        PIC32_R (0x80048)
#define C1TRECINV        PIC32_R (0x8004C)
#define C1FSTAT          PIC32_R (0x80050)
#define C1FSTATCLR       PIC32_R (0x80054)
#define C1FSTATSET       PIC32_R (0x80058)
#define C1FSTATINV       PIC32_R (0x8005C)
#define C1RXOVF          PIC32_R (0x80060)
#define C1RXOVFCLR       PIC32_R (0x80064)
#define C1RXOVFSET       PIC32_R (0x80068)
#define C1RXOVFINV       PIC32_R (0x8006C)
#define C1TMR            PIC32_R (0x80070)
#define C1TMRCLR         PIC32_R (0x80074)
#define C1TMRSET         PIC32_R (0x80078)
#define C1TMRINV         PIC32_R (0x8007C)
#define C1RXM0           PIC32_R (0x80080)
#define C1RXM0CLR        PIC32_R (0x80084)
#define C1RXM0SET        PIC32_R (0x80088)
#define C1RXM0INV        PIC32_R (0x8008C)
#define C1RXM1           PIC32_R (0x80090)
#define C1RXM1CLR        PIC32_R (0x80094)
#define C1RXM1SET        PIC32_R (0x80098)
#define C1RXM1INV        PIC32_R (0x8009C)
#define C1RXM2           PIC32_R (0x800A0)
#define C1RXM2CLR        PIC32_R (0x800A4)
#define C1RXM2SET        PIC32_R (0x800A8)
#define C1RXM2INV        PIC32_R (0x800AC)
#define C1RXM3           PIC32_R (0x800B0)
#define C1RXM3CLR        PIC32_R (0x800B4)
#define C1RXM3SET        PIC32_R (0x800B8)
#define C1RXM3INV        PIC32_R (0x800BC)
#define C1FLTCON0        PIC32_R (0x800C0)
#define C1FLTCON0CLR     PIC32_R (0x800C4)
#define C1FLTCON0SET     PIC32_R (0x800C8)
#define C1FLTCON0INV     PIC32_R (0x800CC)
#define C1FLTCON1        PIC32_R (0x800D0)
#define C1FLTCON1CLR     PIC32_R (0x800D4)
#define C1FLTCON1SET     PIC32_R (0x800D8)
#define C1FLTCON1INV     PIC32_R (0x800DC)
#define C1FLTCON2        PIC32_R (0x800E0)
#define C1FLTCON2CLR     PIC32_R (0x800E4)
#define C1FLTCON2SET     PIC32_R (0x800E8)
#define C1FLTCON2INV     PIC32_R (0x800EC)
#define C1FLTCON3        PIC32_R (0x800F0)
#define C1FLTCON3CLR     PIC32_R (0x800F4)
#define C1FLTCON3SET     PIC32_R (0x800F8)
#define C1FLTCON3INV     PIC32_R (0x800FC)
#define C1FLTCON4        PIC32_R (0x80100)
#define C1FLTCON4CLR     PIC32_R (0x80104)
#define C1FLTCON4SET     PIC32_R (0x80108)
#define C1FLTCON4INV     PIC32_R (0x8010C)
#define C1FLTCON5        PIC32_R (0x80110)
#define C1FLTCON5CLR     PIC32_R (0x80114)
#define C1FLTCON5SET     PIC32_R (0x80118)
#define C1FLTCON5INV     PIC32_R (0x8011C)
#define C1FLTCON6        PIC32_R (0x80120)
#define C1FLTCON6CLR     PIC32_R (0x80124)
#define C1FLTCON6SET     PIC32_R (0x80128)
#define C1FLTCON6INV     PIC32_R (0x8012C)
#define C1FLTCON7        PIC32_R (0x80130)
#define C1FLTCON7CLR     PIC32_R (0x80134)
#define C1FLTCON7SET     PIC32_R (0x80138)
#define C1FLTCON7INV     PIC32_R (0x8013C)
#define C1RXF0           PIC32_R (0x80140)
#define C1RXF0CLR        PIC32_R (0x80144)
#define C1RXF0SET        PIC32_R (0x80148)
#define C1RXF0INV        PIC32_R (0x8014C)
#define C1RXF1           PIC32_R (0x80150)
#define C1RXF1CLR        PIC32_R (0x80154)
#define C1RXF1SET        PIC32_R (0x80158)
#define C1RXF1INV        PIC32_R (0x8015C)
#define C1RXF2           PIC32_R (0x80160)
#define C1RXF2CLR        PIC32_R (0x80164)
#define C1RXF2SET        PIC32_R (0x80168)
#define C1RXF2INV        PIC32_R (0x8016C)
#define C1RXF3           PIC32_R (0x80170)
#define C1RXF3CLR        PIC32_R (0x80174)
#define C1RXF3SET        PIC32_R (0x80178)
#define C1RXF3INV        PIC32_R (0x8017C)
#define C1RXF4           PIC32_R (0x80180)
#define C1RXF4CLR        PIC32_R (0x80184)
#define C1RXF4SET        PIC32_R (0x80188)
#define C1RXF4INV        PIC32_R (0x8018C)
#define C1RXF5           PIC32_R (0x80190)
#define C1RXF5CLR        PIC32_R (0x80194)
#define C1RXF5SET        PIC32_R (0x80198)
#define C1RXF5INV        PIC32_R (0x8019C)
#define C1RXF6           PIC32_R (0x801A0)
#define C1RXF6CLR        PIC32_R (0x801A4)
#define C1RXF6SET        PIC32_R (0x801A8)
#define C1RXF6INV        PIC32_R (0x801AC)
#define C1RXF7           PIC32_R (0x801B0)
#define C1RXF7CLR        PIC32_R (0x801B4)
#define C1RXF7SET        PIC32_R (0x801B8)
#define C1RXF7INV        PIC32_R (0x801BC)
#define C1RXF8           PIC32_R (0x801C0)
#define C1RXF8CLR        PIC32_R (0x801C4)
#define C1RXF8SET        PIC32_R (0x801C8)
#define C1RXF8INV        PIC32_R (0x801CC)
#define C1RXF9           PIC32_R (0x801D0)
#define C1RXF9CLR        PIC32_R (0x801D4)
#define C1RXF9SET        PIC32_R (0x801D8)
#define C1RXF9INV        PIC32_R (0x801DC)
#define C1RXF10          PIC32_R (0x801E0)
#define C1RXF10CLR       PIC32_R (0x801E4)
#define C1RXF10SET       PIC32_R (0x801E8)
#define C1RXF10INV       PIC32_R (0x801EC)
#define C1RXF11          PIC32_R (0x801F0)
#define C1RXF11CLR       PIC32_R (0x801F4)
#define C1RXF11SET       PIC32_R (0x801F8)
#define C1RXF11INV       PIC32_R (0x801FC)
#define C1RXF12          PIC32_R (0x80200)
#define C1RXF12CLR       PIC32_R (0x80204)
#define C1RXF12SET       PIC32_R (0x80208)
#define C1RXF12INV       PIC32_R (0x8020C)
#define C1RXF13          PIC32_R (0x80210)
#define C1RXF13CLR       PIC32_R (0x80214)
#define C1RXF13SET       PIC32_R (0x80218)
#define C1RXF13INV       PIC32_R (0x8021C)
#define C1RXF14          PIC32_R (0x80220)
#define C1RXF14CLR       PIC32_R (0x80224)
#define C1RXF14SET       PIC32_R (0x80228)
#define C1RXF14INV       PIC32_R (0x8022C)
#define C1RXF15          PIC32_R (0x80230)
#define C1RXF15CLR       PIC32_R (0x80234)
#define C1RXF15SET       PIC32_R (0x80238)
#define C1RXF15INV       PIC32_R (0x8023C)
#define C1RXF16          PIC32_R (0x80240)
#define C1RXF16CLR       PIC32_R (0x80244)
#define C1RXF16SET       PIC32_R (0x80248)
#define C1RXF16INV       PIC32_R (0x8024C)
#define C1RXF17          PIC32_R (0x80250)
#define C1RXF17CLR       PIC32_R (0x80254)
#define C1RXF17SET       PIC32_R (0x80258)
#define C1RXF17INV       PIC32_R (0x8025C)
#define C1RXF18          PIC32_R (0x80260)
#define C1RXF18CLR       PIC32_R (0x80264)
#define C1RXF18SET       PIC32_R (0x80268)
#define C1RXF18INV       PIC32_R (0x8026C)
#define C1RXF19          PIC32_R (0x80270)
#define C1RXF19CLR       PIC32_R (0x80274)
#define C1RXF19SET       PIC32_R (0x80278)
#define C1RXF19INV       PIC32_R (0x8027C)
#define C1RXF20          PIC32_R (0x80280)
#define C1RXF20CLR       PIC32_R (0x80284)
#define C1RXF20SET       PIC32_R (0x80288)
#define C1RXF20INV       PIC32_R (0x8028C)
#define C1RXF21          PIC32_R (0x80290)
#define C1RXF21CLR       PIC32_R (0x80294)
#define C1RXF21SET       PIC32_R (0x80298)
#define C1RXF21INV       PIC32_R (0x8029C)
#define C1RXF22          PIC32_R (0x802A0)
#define C1RXF22CLR       PIC32_R (0x802A4)
#define C1RXF22SET       PIC32_R (0x802A8)
#define C1RXF22INV       PIC32_R (0x802AC)
#define C1RXF23          PIC32_R (0x802B0)
#define C1RXF23CLR       PIC32_R (0x802B4)
#define C1RXF23SET       PIC32_R (0x802B8)
#define C1RXF23INV       PIC32_R (0x802BC)
#define C1RXF24          PIC32_R (0x802C0)
#define C1RXF24CLR       PIC32_R (0x802C4)
#define C1RXF24SET       PIC32_R (0x802C8)
#define C1RXF24INV       PIC32_R (0x802CC)
#define C1RXF25          PIC32_R (0x802D0)
#define C1RXF25CLR       PIC32_R (0x802D4)
#define C1RXF25SET       PIC32_R (0x802D8)
#define C1RXF25INV       PIC32_R (0x802DC)
#define C1RXF26          PIC32_R (0x802E0)
#define C1RXF26CLR       PIC32_R (0x802E4)
#define C1RXF26SET       PIC32_R (0x802E8)
#define C1RXF26INV       PIC32_R (0x802EC)
#define C1RXF27          PIC32_R (0x802F0)
#define C1RXF27CLR       PIC32_R (0x802F4)
#define C1RXF27SET       PIC32_R (0x802F8)
#define C1RXF27INV       PIC32_R (0x802FC)
#define C1RXF28          PIC32_R (0x80300)
#define C1RXF28CLR       PIC32_R (0x80304)
#define C1RXF28SET       PIC32_R (0x80308)
#define C1RXF28INV       PIC32_R (0x8030C)
#define C1RXF29          PIC32_R (0x80310)
#define C1RXF29CLR       PIC32_R (0x80314)
#define C1RXF29SET       PIC32_R (0x80318)
#define C1RXF29INV       PIC32_R (0x8031C)
#define C1RXF30          PIC32_R (0x80320)
#define C1RXF30CLR       PIC32_R (0x80324)
#define C1RXF30SET       PIC32_R (0x80328)
#define C1RXF30INV       PIC32_R (0x8032C)
#define C1RXF31          PIC32_R (0x80330)
#define C1RXF31CLR       PIC32_R (0x80334)
#define C1RXF31SET       PIC32_R (0x80338)
#define C1RXF31INV       PIC32_R (0x8033C)
#define C1FIFOBA         PIC32_R (0x80340)
#define C1FIFOBACLR      PIC32_R (0x80344)
#define C1FIFOBASET      PIC32_R (0x80348)
#define C1FIFOBAINV      PIC32_R (0x8034C)
#define C1FIFOCON0       PIC32_R (0x80350)
#define C1FIFOCON0CLR    PIC32_R (0x80354)
#define C1FIFOCON0SET    PIC32_R (0x80358)
#define C1FIFOCON0INV    PIC32_R (0x8035C)
#define C1FIFOINT0       PIC32_R (0x80360)
#define C1FIFOINT0CLR    PIC32_R (0x80364)
#define C1FIFOINT0SET    PIC32_R (0x80368)
#define C1FIFOINT0INV    PIC32_R (0x8036C)
#define C1FIFOUA0        PIC32_R (0x80370)
#define C1FIFOUA0CLR     PIC32_R (0x80374)
#define C1FIFOUA0SET     PIC32_R (0x80378)
#define C1FIFOUA0INV     PIC32_R (0x8037C)
#define C1FIFOCI0        PIC32_R (0x80380)
#define C1FIFOCI0CLR     PIC32_R (0x80384)
#define C1FIFOCI0SET     PIC32_R (0x80388)
#define C1FIFOCI0INV     PIC32_R (0x8038C)
#define C1FIFOCON1       PIC32_R (0x80390)
#define C1FIFOCON1CLR    PIC32_R (0x80394)
#define C1FIFOCON1SET    PIC32_R (0x80398)
#define C1FIFOCON1INV    PIC32_R (0x8039C)
#define C1FIFOINT1       PIC32_R (0x803A0)
#define C1FIFOINT1CLR    PIC32_R (0x803A4)
#define C1FIFOINT1SET    PIC32_R (0x803A8)
#define C1FIFOINT1INV    PIC32_R (0x803AC)
#define C1FIFOUA1        PIC32_R (0x803B0)
#define C1FIFOUA1CLR     PIC32_R (0x803B4)
#define C1FIFOUA1SET     PIC32_R (0x803B8)
#define C1FIFOUA1INV     PIC32_R (0x803BC)
#define C1FIFOCI1        PIC32_R (0x803C0)
#define C1FIFOCI1CLR     PIC32_R (0x803C4)
#define C1FIFOCI1SET     PIC32_R (0x803C8)
#define C1FIFOCI1INV     PIC32_R (0x803CC)
#define C1FIFOCON2       PIC32_R (0x803D0)
#define C1FIFOCON2CLR    PIC32_R (0x803D4)
#define C1FIFOCON2SET    PIC32_R (0x803D8)
#define C1FIFOCON2INV    PIC32_R (0x803DC)
#define C1FIFOINT2       PIC32_R (0x803E0)
#define C1FIFOINT2CLR    PIC32_R (0x803E4)
#define C1FIFOINT2SET    PIC32_R (0x803E8)
#define C1FIFOINT2INV    PIC32_R (0x803EC)
#define C1FIFOUA2        PIC32_R (0x803F0)
#define C1FIFOUA2CLR     PIC32_R (0x803F4)
#define C1FIFOUA2SET     PIC32_R (0x803F8)
#define C1FIFOUA2INV     PIC32_R (0x803FC)
#define C1FIFOCI2        PIC32_R (0x80400)
#define C1FIFOCI2CLR     PIC32_R (0x80404)
#define C1FIFOCI2SET     PIC32_R (0x80408)
#define C1FIFOCI2INV     PIC32_R (0x8040C)
#define C1FIFOCON3       PIC32_R (0x80410)
#define C1FIFOCON3CLR    PIC32_R (0x80414)
#define C1FIFOCON3SET    PIC32_R (0x80418)
#define C1FIFOCON3INV    PIC32_R (0x8041C)
#define C1FIFOINT3       PIC32_R (0x80420)
#define C1FIFOINT3CLR    PIC32_R (0x80424)
#define C1FIFOINT3SET    PIC32_R (0x80428)
#define C1FIFOINT3INV    PIC32_R (0x8042C)
#define C1FIFOUA3        PIC32_R (0x80430)
#define C1FIFOUA3CLR     PIC32_R (0x80434)
#define C1FIFOUA3SET     PIC32_R (0x80438)
#define C1FIFOUA3INV     PIC32_R (0x8043C)
#define C1FIFOCI3        PIC32_R (0x80440)
#define C1FIFOCI3CLR     PIC32_R (0x80444)
#define C1FIFOCI3SET     PIC32_R (0x80448)
#define C1FIFOCI3INV     PIC32_R (0x8044C)
#define C1FIFOCON4       PIC32_R (0x80450)
#define C1FIFOCON4CLR    PIC32_R (0x80454)
#define C1FIFOCON4SET    PIC32_R (0x80458)
#define C1FIFOCON4INV    PIC32_R (0x8045C)
#define C1FIFOINT4       PIC32_R (0x80460)
#define C1FIFOINT4CLR    PIC32_R (0x80464)
#define C1FIFOINT4SET    PIC32_R (0x80468)
#define C1FIFOINT4INV    PIC32_R (0x8046C)
#define C1FIFOUA4        PIC32_R (0x80470)
#define C1FIFOUA4CLR     PIC32_R (0x80474)
#define C1FIFOUA4SET     PIC32_R (0x80478)
#define C1FIFOUA4INV     PIC32_R (0x8047C)
#define C1FIFOCI4        PIC32_R (0x80480)
#define C1FIFOCI4CLR     PIC32_R (0x80484)
#define C1FIFOCI4SET     PIC32_R (0x80488)
#define C1FIFOCI4INV     PIC32_R (0x8048C)
#define C1FIFOCON5       PIC32_R (0x80490)
#define C1FIFOCON5CLR    PIC32_R (0x80494)
#define C1FIFOCON5SET    PIC32_R (0x80498)
#define C1FIFOCON5INV    PIC32_R (0x8049C)
#define C1FIFOINT5       PIC32_R (0x804A0)
#define C1FIFOINT5CLR    PIC32_R (0x804A4)
#define C1FIFOINT5SET    PIC32_R (0x804A8)
#define C1FIFOINT5INV    PIC32_R (0x804AC)
#define C1FIFOUA5        PIC32_R (0x804B0)
#define C1FIFOUA5CLR     PIC32_R (0x804B4)
#define C1FIFOUA5SET     PIC32_R (0x804B8)
#define C1FIFOUA5INV     PIC32_R (0x804BC)
#define C1FIFOCI5        PIC32_R (0x804C0)
#define C1FIFOCI5CLR     PIC32_R (0x804C4)
#define C1FIFOCI5SET     PIC32_R (0x804C8)
#define C1FIFOCI5INV     PIC32_R (0x804CC)
#define C1FIFOCON6       PIC32_R (0x804D0)
#define C1FIFOCON6CLR    PIC32_R (0x804D4)
#define C1FIFOCON6SET    PIC32_R (0x804D8)
#define C1FIFOCON6INV    PIC32_R (0x804DC)
#define C1FIFOINT6       PIC32_R (0x804E0)
#define C1FIFOINT6CLR    PIC32_R (0x804E4)
#define C1FIFOINT6SET    PIC32_R (0x804E8)
#define C1FIFOINT6INV    PIC32_R (0x804EC)
#define C1FIFOUA6        PIC32_R (0x804F0)
#define C1FIFOUA6CLR     PIC32_R (0x804F4)
#define C1FIFOUA6SET     PIC32_R (0x804F8)
#define C1FIFOUA6INV     PIC32_R (0x804FC)
#define C1FIFOCI6        PIC32_R (0x80500)
#define C1FIFOCI6CLR     PIC32_R (0x80504)
#define C1FIFOCI6SET     PIC32_R (0x80508)
#define C1FIFOCI6INV     PIC32_R (0x8050C)
#define C1FIFOCON7       PIC32_R (0x80510)
#define C1FIFOCON7CLR    PIC32_R (0x80514)
#define C1FIFOCON7SET    PIC32_R (0x80518)
#define C1FIFOCON7INV    PIC32_R (0x8051C)
#define C1FIFOINT7       PIC32_R (0x80520)
#define C1FIFOINT7CLR    PIC32_R (0x80524)
#define C1FIFOINT7SET    PIC32_R (0x80528)
#define C1FIFOINT7INV    PIC32_R (0x8052C)
#define C1FIFOUA7        PIC32_R (0x80530)
#define C1FIFOUA7CLR     PIC32_R (0x80534)
#define C1FIFOUA7SET     PIC32_R (0x80538)
#define C1FIFOUA7INV     PIC32_R (0x8053C)
#define C1FIFOCI7        PIC32_R (0x80540)
#define C1FIFOCI7CLR     PIC32_R (0x80544)
#define C1FIFOCI7SET     PIC32_R (0x80548)
#define C1FIFOCI7INV     PIC32_R (0x8054C)
#define C1FIFOCON8       PIC32_R (0x80550)
#define C1FIFOCON8CLR    PIC32_R (0x80554)
#define C1FIFOCON8SET    PIC32_R (0x80558)
#define C1FIFOCON8INV    PIC32_R (0x8055C)
#define C1FIFOINT8       PIC32_R (0x80560)
#define C1FIFOINT8CLR    PIC32_R (0x80564)
#define C1FIFOINT8SET    PIC32_R (0x80568)
#define C1FIFOINT8INV    PIC32_R (0x8056C)
#define C1FIFOUA8        PIC32_R (0x80570)
#define C1FIFOUA8CLR     PIC32_R (0x80574)
#define C1FIFOUA8SET     PIC32_R (0x80578)
#define C1FIFOUA8INV     PIC32_R (0x8057C)
#define C1FIFOCI8        PIC32_R (0x80580)
#define C1FIFOCI8CLR     PIC32_R (0x80584)
#define C1FIFOCI8SET     PIC32_R (0x80588)
#define C1FIFOCI8INV     PIC32_R (0x8058C)
#define C1FIFOCON9       PIC32_R (0x80590)
#define C1FIFOCON9CLR    PIC32_R (0x80594)
#define C1FIFOCON9SET    PIC32_R (0x80598)
#define C1FIFOCON9INV    PIC32_R (0x8059C)
#define C1FIFOINT9       PIC32_R (0x805A0)
#define C1FIFOINT9CLR    PIC32_R (0x805A4)
#define C1FIFOINT9SET    PIC32_R (0x805A8)
#define C1FIFOINT9INV    PIC32_R (0x805AC)
#define C1FIFOUA9        PIC32_R (0x805B0)
#define C1FIFOUA9CLR     PIC32_R (0x805B4)
#define C1FIFOUA9SET     PIC32_R (0x805B8)
#define C1FIFOUA9INV     PIC32_R (0x805BC)
#define C1FIFOCI9        PIC32_R (0x805C0)
#define C1FIFOCI9CLR     PIC32_R (0x805C4)
#define C1FIFOCI9SET     PIC32_R (0x805C8)
#define C1FIFOCI9INV     PIC32_R (0x805CC)
#define C1FIFOCON10      PIC32_R (0x805D0)
#define C1FIFOCON10CLR   PIC32_R (0x805D4)
#define C1FIFOCON10SET   PIC32_R (0x805D8)
#define C1FIFOCON10INV   PIC32_R (0x805DC)
#define C1FIFOINT10      PIC32_R (0x805E0)
#define C1FIFOINT10CLR   PIC32_R (0x805E4)
#define C1FIFOINT10SET   PIC32_R (0x805E8)
#define C1FIFOINT10INV   PIC32_R (0x805EC)
#define C1FIFOUA10       PIC32_R (0x805F0)
#define C1FIFOUA10CLR    PIC32_R (0x805F4)
#define C1FIFOUA10SET    PIC32_R (0x805F8)
#define C1FIFOUA10INV    PIC32_R (0x805FC)
#define C1FIFOCI10       PIC32_R (0x80600)
#define C1FIFOCI10CLR    PIC32_R (0x80604)
#define C1FIFOCI10SET    PIC32_R (0x80608)
#define C1FIFOCI10INV    PIC32_R (0x8060C)
#define C1FIFOCON11      PIC32_R (0x80610)
#define C1FIFOCON11CLR   PIC32_R (0x80614)
#define C1FIFOCON11SET   PIC32_R (0x80618)
#define C1FIFOCON11INV   PIC32_R (0x8061C)
#define C1FIFOINT11      PIC32_R (0x80620)
#define C1FIFOINT11CLR   PIC32_R (0x80624)
#define C1FIFOINT11SET   PIC32_R (0x80628)
#define C1FIFOINT11INV   PIC32_R (0x8062C)
#define C1FIFOUA11       PIC32_R (0x80630)
#define C1FIFOUA11CLR    PIC32_R (0x80634)
#define C1FIFOUA11SET    PIC32_R (0x80638)
#define C1FIFOUA11INV    PIC32_R (0x8063C)
#define C1FIFOCI11       PIC32_R (0x80640)
#define C1FIFOCI11CLR    PIC32_R (0x80644)
#define C1FIFOCI11SET    PIC32_R (0x80648)
#define C1FIFOCI11INV    PIC32_R (0x8064C)
#define C1FIFOCON12      PIC32_R (0x80650)
#define C1FIFOCON12CLR   PIC32_R (0x80654)
#define C1FIFOCON12SET   PIC32_R (0x80658)
#define C1FIFOCON12INV   PIC32_R (0x8065C)
#define C1FIFOINT12      PIC32_R (0x80660)
#define C1FIFOINT12CLR   PIC32_R (0x80664)
#define C1FIFOINT12SET   PIC32_R (0x80668)
#define C1FIFOINT12INV   PIC32_R (0x8066C)
#define C1FIFOUA12       PIC32_R (0x80670)
#define C1FIFOUA12CLR    PIC32_R (0x80674)
#define C1FIFOUA12SET    PIC32_R (0x80678)
#define C1FIFOUA12INV    PIC32_R (0x8067C)
#define C1FIFOCI12       PIC32_R (0x80680)
#define C1FIFOCI12CLR    PIC32_R (0x80684)
#define C1FIFOCI12SET    PIC32_R (0x80688)
#define C1FIFOCI12INV    PIC32_R (0x8068C)
#define C1FIFOCON13      PIC32_R (0x80690)
#define C1FIFOCON13CLR   PIC32_R (0x80694)
#define C1FIFOCON13SET   PIC32_R (0x80698)
#define C1FIFOCON13INV   PIC32_R (0x8069C)
#define C1FIFOINT13      PIC32_R (0x806A0)
#define C1FIFOINT13CLR   PIC32_R (0x806A4)
#define C1FIFOINT13SET   PIC32_R (0x806A8)
#define C1FIFOINT13INV   PIC32_R (0x806AC)
#define C1FIFOUA13       PIC32_R (0x806B0)
#define C1FIFOUA13CLR    PIC32_R (0x806B4)
#define C1FIFOUA13SET    PIC32_R (0x806B8)
#define C1FIFOUA13INV    PIC32_R (0x806BC)
#define C1FIFOCI13       PIC32_R (0x806C0)
#define C1FIFOCI13CLR    PIC32_R (0x806C4)
#define C1FIFOCI13SET    PIC32_R (0x806C8)
#define C1FIFOCI13INV    PIC32_R (0x806CC)
#define C1FIFOCON14      PIC32_R (0x806D0)
#define C1FIFOCON14CLR   PIC32_R (0x806D4)
#define C1FIFOCON14SET   PIC32_R (0x806D8)
#define C1FIFOCON14INV   PIC32_R (0x806DC)
#define C1FIFOINT14      PIC32_R (0x806E0)
#define C1FIFOINT14CLR   PIC32_R (0x806E4)
#define C1FIFOINT14SET   PIC32_R (0x806E8)
#define C1FIFOINT14INV   PIC32_R (0x806EC)
#define C1FIFOUA14       PIC32_R (0x806F0)
#define C1FIFOUA14CLR    PIC32_R (0x806F4)
#define C1FIFOUA14SET    PIC32_R (0x806F8)
#define C1FIFOUA14INV    PIC32_R (0x806FC)
#define C1FIFOCI14       PIC32_R (0x80700)
#define C1FIFOCI14CLR    PIC32_R (0x80704)
#define C1FIFOCI14SET    PIC32_R (0x80708)
#define C1FIFOCI14INV    PIC32_R (0x8070C)
#define C1FIFOCON15      PIC32_R (0x80710)
#define C1FIFOCON15CLR   PIC32_R (0x80714)
#define C1FIFOCON15SET   PIC32_R (0x80718)
#define C1FIFOCON15INV   PIC32_R (0x8071C)
#define C1FIFOINT15      PIC32_R (0x80720)
#define C1FIFOINT15CLR   PIC32_R (0x80724)
#define C1FIFOINT15SET   PIC32_R (0x80728)
#define C1FIFOINT15INV   PIC32_R (0x8072C)
#define C1FIFOUA15       PIC32_R (0x80730)
#define C1FIFOUA15CLR    PIC32_R (0x80734)
#define C1FIFOUA15SET    PIC32_R (0x80738)
#define C1FIFOUA15INV    PIC32_R (0x8073C)
#define C1FIFOCI15       PIC32_R (0x80740)
#define C1FIFOCI15CLR    PIC32_R (0x80744)
#define C1FIFOCI15SET    PIC32_R (0x80748)
#define C1FIFOCI15INV    PIC32_R (0x8074C)
#define C1FIFOCON16      PIC32_R (0x80750)
#define C1FIFOCON16CLR   PIC32_R (0x80754)
#define C1FIFOCON16SET   PIC32_R (0x80758)
#define C1FIFOCON16INV   PIC32_R (0x8075C)
#define C1FIFOINT16      PIC32_R (0x80760)
#define C1FIFOINT16CLR   PIC32_R (0x80764)
#define C1FIFOINT16SET   PIC32_R (0x80768)
#define C1FIFOINT16INV   PIC32_R (0x8076C)
#define C1FIFOUA16       PIC32_R (0x80770)
#define C1FIFOUA16CLR    PIC32_R (0x80774)
#define C1FIFOUA16SET    PIC32_R (0x80778)
#define C1FIFOUA16INV    PIC32_R (0x8077C)
#define C1FIFOCI16       PIC32_R (0x80780)
#define C1FIFOCI16CLR    PIC32_R (0x80784)
#define C1FIFOCI16SET    PIC32_R (0x80788)
#define C1FIFOCI16INV    PIC32_R (0x8078C)
#define C1FIFOCON17      PIC32_R (0x80790)
#define C1FIFOCON17CLR   PIC32_R (0x80794)
#define C1FIFOCON17SET   PIC32_R (0x80798)
#define C1FIFOCON17INV   PIC32_R (0x8079C)
#define C1FIFOINT17      PIC32_R (0x807A0)
#define C1FIFOINT17CLR   PIC32_R (0x807A4)
#define C1FIFOINT17SET   PIC32_R (0x807A8)
#define C1FIFOINT17INV   PIC32_R (0x807AC)
#define C1FIFOUA17       PIC32_R (0x807B0)
#define C1FIFOUA17CLR    PIC32_R (0x807B4)
#define C1FIFOUA17SET    PIC32_R (0x807B8)
#define C1FIFOUA17INV    PIC32_R (0x807BC)
#define C1FIFOCI17       PIC32_R (0x807C0)
#define C1FIFOCI17CLR    PIC32_R (0x807C4)
#define C1FIFOCI17SET    PIC32_R (0x807C8)
#define C1FIFOCI17INV    PIC32_R (0x807CC)
#define C1FIFOCON18      PIC32_R (0x807D0)
#define C1FIFOCON18CLR   PIC32_R (0x807D4)
#define C1FIFOCON18SET   PIC32_R (0x807D8)
#define C1FIFOCON18INV   PIC32_R (0x807DC)
#define C1FIFOINT18      PIC32_R (0x807E0)
#define C1FIFOINT18CLR   PIC32_R (0x807E4)
#define C1FIFOINT18SET   PIC32_R (0x807E8)
#define C1FIFOINT18INV   PIC32_R (0x807EC)
#define C1FIFOUA18       PIC32_R (0x807F0)
#define C1FIFOUA18CLR    PIC32_R (0x807F4)
#define C1FIFOUA18SET    PIC32_R (0x807F8)
#define C1FIFOUA18INV    PIC32_R (0x807FC)
#define C1FIFOCI18       PIC32_R (0x80800)
#define C1FIFOCI18CLR    PIC32_R (0x80804)
#define C1FIFOCI18SET    PIC32_R (0x80808)
#define C1FIFOCI18INV    PIC32_R (0x8080C)
#define C1FIFOCON19      PIC32_R (0x80810)
#define C1FIFOCON19CLR   PIC32_R (0x80814)
#define C1FIFOCON19SET   PIC32_R (0x80818)
#define C1FIFOCON19INV   PIC32_R (0x8081C)
#define C1FIFOINT19      PIC32_R (0x80820)
#define C1FIFOINT19CLR   PIC32_R (0x80824)
#define C1FIFOINT19SET   PIC32_R (0x80828)
#define C1FIFOINT19INV   PIC32_R (0x8082C)
#define C1FIFOUA19       PIC32_R (0x80830)
#define C1FIFOUA19CLR    PIC32_R (0x80834)
#define C1FIFOUA19SET    PIC32_R (0x80838)
#define C1FIFOUA19INV    PIC32_R (0x8083C)
#define C1FIFOCI19       PIC32_R (0x80840)
#define C1FIFOCI19CLR    PIC32_R (0x80844)
#define C1FIFOCI19SET    PIC32_R (0x80848)
#define C1FIFOCI19INV    PIC32_R (0x8084C)
#define C1FIFOCON20      PIC32_R (0x80850)
#define C1FIFOCON20CLR   PIC32_R (0x80854)
#define C1FIFOCON20SET   PIC32_R (0x80858)
#define C1FIFOCON20INV   PIC32_R (0x8085C)
#define C1FIFOINT20      PIC32_R (0x80860)
#define C1FIFOINT20CLR   PIC32_R (0x80864)
#define C1FIFOINT20SET   PIC32_R (0x80868)
#define C1FIFOINT20INV   PIC32_R (0x8086C)
#define C1FIFOUA20       PIC32_R (0x80870)
#define C1FIFOUA20CLR    PIC32_R (0x80874)
#define C1FIFOUA20SET    PIC32_R (0x80878)
#define C1FIFOUA20INV    PIC32_R (0x8087C)
#define C1FIFOCI20       PIC32_R (0x80880)
#define C1FIFOCI20CLR    PIC32_R (0x80884)
#define C1FIFOCI20SET    PIC32_R (0x80888)
#define C1FIFOCI20INV    PIC32_R (0x8088C)
#define C1FIFOCON21      PIC32_R (0x80890)
#define C1FIFOCON21CLR   PIC32_R (0x80894)
#define C1FIFOCON21SET   PIC32_R (0x80898)
#define C1FIFOCON21INV   PIC32_R (0x8089C)
#define C1FIFOINT21      PIC32_R (0x808A0)
#define C1FIFOINT21CLR   PIC32_R (0x808A4)
#define C1FIFOINT21SET   PIC32_R (0x808A8)
#define C1FIFOINT21INV   PIC32_R (0x808AC)
#define C1FIFOUA21       PIC32_R (0x808B0)
#define C1FIFOUA21CLR    PIC32_R (0x808B4)
#define C1FIFOUA21SET    PIC32_R (0x808B8)
#define C1FIFOUA21INV    PIC32_R (0x808BC)
#define C1FIFOCI21       PIC32_R (0x808C0)
#define C1FIFOCI21CLR    PIC32_R (0x808C4)
#define C1FIFOCI21SET    PIC32_R (0x808C8)
#define C1FIFOCI21INV    PIC32_R (0x808CC)
#define C1FIFOCON22      PIC32_R (0x808D0)
#define C1FIFOCON22CLR   PIC32_R (0x808D4)
#define C1FIFOCON22SET   PIC32_R (0x808D8)
#define C1FIFOCON22INV   PIC32_R (0x808DC)
#define C1FIFOINT22      PIC32_R (0x808E0)
#define C1FIFOINT22CLR   PIC32_R (0x808E4)
#define C1FIFOINT22SET   PIC32_R (0x808E8)
#define C1FIFOINT22INV   PIC32_R (0x808EC)
#define C1FIFOUA22       PIC32_R (0x808F0)
#define C1FIFOUA22CLR    PIC32_R (0x808F4)
#define C1FIFOUA22SET    PIC32_R (0x808F8)
#define C1FIFOUA22INV    PIC32_R (0x808FC)
#define C1FIFOCI22       PIC32_R (0x80900)
#define C1FIFOCI22CLR    PIC32_R (0x80904)
#define C1FIFOCI22SET    PIC32_R (0x80908)
#define C1FIFOCI22INV    PIC32_R (0x8090C)
#define C1FIFOCON23      PIC32_R (0x80910)
#define C1FIFOCON23CLR   PIC32_R (0x80914)
#define C1FIFOCON23SET   PIC32_R (0x80918)
#define C1FIFOCON23INV   PIC32_R (0x8091C)
#define C1FIFOINT23      PIC32_R (0x80920)
#define C1FIFOINT23CLR   PIC32_R (0x80924)
#define C1FIFOINT23SET   PIC32_R (0x80928)
#define C1FIFOINT23INV   PIC32_R (0x8092C)
#define C1FIFOUA23       PIC32_R (0x80930)
#define C1FIFOUA23CLR    PIC32_R (0x80934)
#define C1FIFOUA23SET    PIC32_R (0x80938)
#define C1FIFOUA23INV    PIC32_R (0x8093C)
#define C1FIFOCI23       PIC32_R (0x80940)
#define C1FIFOCI23CLR    PIC32_R (0x80944)
#define C1FIFOCI23SET    PIC32_R (0x80948)
#define C1FIFOCI23INV    PIC32_R (0x8094C)
#define C1FIFOCON24      PIC32_R (0x80950)
#define C1FIFOCON24CLR   PIC32_R (0x80954)
#define C1FIFOCON24SET   PIC32_R (0x80958)
#define C1FIFOCON24INV   PIC32_R (0x8095C)
#define C1FIFOINT24      PIC32_R (0x80960)
#define C1FIFOINT24CLR   PIC32_R (0x80964)
#define C1FIFOINT24SET   PIC32_R (0x80968)
#define C1FIFOINT24INV   PIC32_R (0x8096C)
#define C1FIFOUA24       PIC32_R (0x80970)
#define C1FIFOUA24CLR    PIC32_R (0x80974)
#define C1FIFOUA24SET    PIC32_R (0x80978)
#define C1FIFOUA24INV    PIC32_R (0x8097C)
#define C1FIFOCI24       PIC32_R (0x80980)
#define C1FIFOCI24CLR    PIC32_R (0x80984)
#define C1FIFOCI24SET    PIC32_R (0x80988)
#define C1FIFOCI24INV    PIC32_R (0x8098C)
#define C1FIFOCON25      PIC32_R (0x80990)
#define C1FIFOCON25CLR   PIC32_R (0x80994)
#define C1FIFOCON25SET   PIC32_R (0x80998)
#define C1FIFOCON25INV   PIC32_R (0x8099C)
#define C1FIFOINT25      PIC32_R (0x809A0)
#define C1FIFOINT25CLR   PIC32_R (0x809A4)
#define C1FIFOINT25SET   PIC32_R (0x809A8)
#define C1FIFOINT25INV   PIC32_R (0x809AC)
#define C1FIFOUA25       PIC32_R (0x809B0)
#define C1FIFOUA25CLR    PIC32_R (0x809B4)
#define C1FIFOUA25SET    PIC32_R (0x809B8)
#define C1FIFOUA25INV    PIC32_R (0x809BC)
#define C1FIFOCI25       PIC32_R (0x809C0)
#define C1FIFOCI25CLR    PIC32_R (0x809C4)
#define C1FIFOCI25SET    PIC32_R (0x809C8)
#define C1FIFOCI25INV    PIC32_R (0x809CC)
#define C1FIFOCON26      PIC32_R (0x809D0)
#define C1FIFOCON26CLR   PIC32_R (0x809D4)
#define C1FIFOCON26SET   PIC32_R (0x809D8)
#define C1FIFOCON26INV   PIC32_R (0x809DC)
#define C1FIFOINT26      PIC32_R (0x809E0)
#define C1FIFOINT26CLR   PIC32_R (0x809E4)
#define C1FIFOINT26SET   PIC32_R (0x809E8)
#define C1FIFOINT26INV   PIC32_R (0x809EC)
#define C1FIFOUA26       PIC32_R (0x809F0)
#define C1FIFOUA26CLR    PIC32_R (0x809F4)
#define C1FIFOUA26SET    PIC32_R (0x809F8)
#define C1FIFOUA26INV    PIC32_R (0x809FC)
#define C1FIFOCI26       PIC32_R (0x80A00)
#define C1FIFOCI26CLR    PIC32_R (0x80A04)
#define C1FIFOCI26SET    PIC32_R (0x80A08)
#define C1FIFOCI26INV    PIC32_R (0x80A0C)
#define C1FIFOCON27      PIC32_R (0x80A10)
#define C1FIFOCON27CLR   PIC32_R (0x80A14)
#define C1FIFOCON27SET   PIC32_R (0x80A18)
#define C1FIFOCON27INV   PIC32_R (0x80A1C)
#define C1FIFOINT27      PIC32_R (0x80A20)
#define C1FIFOINT27CLR   PIC32_R (0x80A24)
#define C1FIFOINT27SET   PIC32_R (0x80A28)
#define C1FIFOINT27INV   PIC32_R (0x80A2C)
#define C1FIFOUA27       PIC32_R (0x80A30)
#define C1FIFOUA27CLR    PIC32_R (0x80A34)
#define C1FIFOUA27SET    PIC32_R (0x80A38)
#define C1FIFOUA27INV    PIC32_R (0x80A3C)
#define C1FIFOCI27       PIC32_R (0x80A40)
#define C1FIFOCI27CLR    PIC32_R (0x80A44)
#define C1FIFOCI27SET    PIC32_R (0x80A48)
#define C1FIFOCI27INV    PIC32_R (0x80A4C)
#define C1FIFOCON28      PIC32_R (0x80A50)
#define C1FIFOCON28CLR   PIC32_R (0x80A54)
#define C1FIFOCON28SET   PIC32_R (0x80A58)
#define C1FIFOCON28INV   PIC32_R (0x80A5C)
#define C1FIFOINT28      PIC32_R (0x80A60)
#define C1FIFOINT28CLR   PIC32_R (0x80A64)
#define C1FIFOINT28SET   PIC32_R (0x80A68)
#define C1FIFOINT28INV   PIC32_R (0x80A6C)
#define C1FIFOUA28       PIC32_R (0x80A70)
#define C1FIFOUA28CLR    PIC32_R (0x80A74)
#define C1FIFOUA28SET    PIC32_R (0x80A78)
#define C1FIFOUA28INV    PIC32_R (0x80A7C)
#define C1FIFOCI28       PIC32_R (0x80A80)
#define C1FIFOCI28CLR    PIC32_R (0x80A84)
#define C1FIFOCI28SET    PIC32_R (0x80A88)
#define C1FIFOCI28INV    PIC32_R (0x80A8C)
#define C1FIFOCON29      PIC32_R (0x80A90)
#define C1FIFOCON29CLR   PIC32_R (0x80A94)
#define C1FIFOCON29SET   PIC32_R (0x80A98)
#define C1FIFOCON29INV   PIC32_R (0x80A9C)
#define C1FIFOINT29      PIC32_R (0x80AA0)
#define C1FIFOINT29CLR   PIC32_R (0x80AA4)
#define C1FIFOINT29SET   PIC32_R (0x80AA8)
#define C1FIFOINT29INV   PIC32_R (0x80AAC)
#define C1FIFOUA29       PIC32_R (0x80AB0)
#define C1FIFOUA29CLR    PIC32_R (0x80AB4)
#define C1FIFOUA29SET    PIC32_R (0x80AB8)
#define C1FIFOUA29INV    PIC32_R (0x80ABC)
#define C1FIFOCI29       PIC32_R (0x80AC0)
#define C1FIFOCI29CLR    PIC32_R (0x80AC4)
#define C1FIFOCI29SET    PIC32_R (0x80AC8)
#define C1FIFOCI29INV    PIC32_R (0x80ACC)
#define C1FIFOCON30      PIC32_R (0x80AD0)
#define C1FIFOCON30CLR   PIC32_R (0x80AD4)
#define C1FIFOCON30SET   PIC32_R (0x80AD8)
#define C1FIFOCON30INV   PIC32_R (0x80ADC)
#define C1FIFOINT30      PIC32_R (0x80AE0)
#define C1FIFOINT30CLR   PIC32_R (0x80AE4)
#define C1FIFOINT30SET   PIC32_R (0x80AE8)
#define C1FIFOINT30INV   PIC32_R (0x80AEC)
#define C1FIFOUA30       PIC32_R (0x80AF0)
#define C1FIFOUA30CLR    PIC32_R (0x80AF4)
#define C1FIFOUA30SET    PIC32_R (0x80AF8)
#define C1FIFOUA30INV    PIC32_R (0x80AFC)
#define C1FIFOCI30       PIC32_R (0x80B00)
#define C1FIFOCI30CLR    PIC32_R (0x80B04)
#define C1FIFOCI30SET    PIC32_R (0x80B08)
#define C1FIFOCI30INV    PIC32_R (0x80B0C)
#define C1FIFOCON31      PIC32_R (0x80B10)
#define C1FIFOCON31CLR   PIC32_R (0x80B14)
#define C1FIFOCON31SET   PIC32_R (0x80B18)
#define C1FIFOCON31INV   PIC32_R (0x80B1C)
#define C1FIFOINT31      PIC32_R (0x80B20)
#define C1FIFOINT31CLR   PIC32_R (0x80B24)
#define C1FIFOINT31SET   PIC32_R (0x80B28)
#define C1FIFOINT31INV   PIC32_R (0x80B2C)
#define C1FIFOUA31       PIC32_R (0x80B30)
#define C1FIFOUA31CLR    PIC32_R (0x80B34)
#define C1FIFOUA31SET    PIC32_R (0x80B38)
#define C1FIFOUA31INV    PIC32_R (0x80B3C)
#define C1FIFOCI31       PIC32_R (0x80B40)
#define C1FIFOCI31CLR    PIC32_R (0x80B44)
#define C1FIFOCI31SET    PIC32_R (0x80B48)
#define C1FIFOCI31INV    PIC32_R (0x80B4C)
#define C2CON            PIC32_R (0x81000)
#define C2CONCLR         PIC32_R (0x81004)
#define C2CONSET         PIC32_R (0x81008)
#define C2CONINV         PIC32_R (0x8100C)
#define C2CFG            PIC32_R (0x81010)
#define C2CFGCLR         PIC32_R (0x81014)
#define C2CFGSET         PIC32_R (0x81018)
#define C2CFGINV         PIC32_R (0x8101C)
#define C2INT            PIC32_R (0x81020)
#define C2INTCLR         PIC32_R (0x81024)
#define C2INTSET         PIC32_R (0x81028)
#define C2INTINV         PIC32_R (0x8102C)
#define C2VEC            PIC32_R (0x81030)
#define C2VECCLR         PIC32_R (0x81034)
#define C2VECSET         PIC32_R (0x81038)
#define C2VECINV         PIC32_R (0x8103C)
#define C2TREC           PIC32_R (0x81040)
#define C2TRECCLR        PIC32_R (0x81044)
#define C2TRECSET        PIC32_R (0x81048)
#define C2TRECINV        PIC32_R (0x8104C)
#define C2FSTAT          PIC32_R (0x81050)
#define C2FSTATCLR       PIC32_R (0x81054)
#define C2FSTATSET       PIC32_R (0x81058)
#define C2FSTATINV       PIC32_R (0x8105C)
#define C2RXOVF          PIC32_R (0x81060)
#define C2RXOVFCLR       PIC32_R (0x81064)
#define C2RXOVFSET       PIC32_R (0x81068)
#define C2RXOVFINV       PIC32_R (0x8106C)
#define C2TMR            PIC32_R (0x81070)
#define C2TMRCLR         PIC32_R (0x81074)
#define C2TMRSET         PIC32_R (0x81078)
#define C2TMRINV         PIC32_R (0x8107C)
#define C2RXM0           PIC32_R (0x81080)
#define C2RXM0CLR        PIC32_R (0x81084)
#define C2RXM0SET        PIC32_R (0x81088)
#define C2RXM0INV        PIC32_R (0x8108C)
#define C2RXM1           PIC32_R (0x81090)
#define C2RXM1CLR        PIC32_R (0x81094)
#define C2RXM1SET        PIC32_R (0x81098)
#define C2RXM1INV        PIC32_R (0x8109C)
#define C2RXM2           PIC32_R (0x810A0)
#define C2RXM2CLR        PIC32_R (0x810A4)
#define C2RXM2SET        PIC32_R (0x810A8)
#define C2RXM2INV        PIC32_R (0x810AC)
#define C2RXM3           PIC32_R (0x810B0)
#define C2RXM3CLR        PIC32_R (0x810B4)
#define C2RXM3SET        PIC32_R (0x810B8)
#define C2RXM3INV        PIC32_R (0x810BC)
#define C2FLTCON0        PIC32_R (0x810C0)
#define C2FLTCON0CLR     PIC32_R (0x810C4)
#define C2FLTCON0SET     PIC32_R (0x810C8)
#define C2FLTCON0INV     PIC32_R (0x810CC)
#define C2FLTCON1        PIC32_R (0x810D0)
#define C2FLTCON1CLR     PIC32_R (0x810D4)
#define C2FLTCON1SET     PIC32_R (0x810D8)
#define C2FLTCON1INV     PIC32_R (0x810DC)
#define C2FLTCON2        PIC32_R (0x810E0)
#define C2FLTCON2CLR     PIC32_R (0x810E4)
#define C2FLTCON2SET     PIC32_R (0x810E8)
#define C2FLTCON2INV     PIC32_R (0x810EC)
#define C2FLTCON3        PIC32_R (0x810F0)
#define C2FLTCON3CLR     PIC32_R (0x810F4)
#define C2FLTCON3SET     PIC32_R (0x810F8)
#define C2FLTCON3INV     PIC32_R (0x810FC)
#define C2FLTCON4        PIC32_R (0x81100)
#define C2FLTCON4CLR     PIC32_R (0x81104)
#define C2FLTCON4SET     PIC32_R (0x81108)
#define C2FLTCON4INV     PIC32_R (0x8110C)
#define C2FLTCON5        PIC32_R (0x81110)
#define C2FLTCON5CLR     PIC32_R (0x81114)
#define C2FLTCON5SET     PIC32_R (0x81118)
#define C2FLTCON5INV     PIC32_R (0x8111C)
#define C2FLTCON6        PIC32_R (0x81120)
#define C2FLTCON6CLR     PIC32_R (0x81124)
#define C2FLTCON6SET     PIC32_R (0x81128)
#define C2FLTCON6INV     PIC32_R (0x8112C)
#define C2FLTCON7        PIC32_R (0x81130)
#define C2FLTCON7CLR     PIC32_R (0x81134)
#define C2FLTCON7SET     PIC32_R (0x81138)
#define C2FLTCON7INV     PIC32_R (0x8113C)
#define C2RXF0           PIC32_R (0x81140)
#define C2RXF0CLR        PIC32_R (0x81144)
#define C2RXF0SET        PIC32_R (0x81148)
#define C2RXF0INV        PIC32_R (0x8114C)
#define C2RXF1           PIC32_R (0x81150)
#define C2RXF1CLR        PIC32_R (0x81154)
#define C2RXF1SET        PIC32_R (0x81158)
#define C2RXF1INV        PIC32_R (0x8115C)
#define C2RXF2           PIC32_R (0x81160)
#define C2RXF2CLR        PIC32_R (0x81164)
#define C2RXF2SET        PIC32_R (0x81168)
#define C2RXF2INV        PIC32_R (0x8116C)
#define C2RXF3           PIC32_R (0x81170)
#define C2RXF3CLR        PIC32_R (0x81174)
#define C2RXF3SET        PIC32_R (0x81178)
#define C2RXF3INV        PIC32_R (0x8117C)
#define C2RXF4           PIC32_R (0x81180)
#define C2RXF4CLR        PIC32_R (0x81184)
#define C2RXF4SET        PIC32_R (0x81188)
#define C2RXF4INV        PIC32_R (0x8118C)
#define C2RXF5           PIC32_R (0x81190)
#define C2RXF5CLR        PIC32_R (0x81194)
#define C2RXF5SET        PIC32_R (0x81198)
#define C2RXF5INV        PIC32_R (0x8119C)
#define C2RXF6           PIC32_R (0x811A0)
#define C2RXF6CLR        PIC32_R (0x811A4)
#define C2RXF6SET        PIC32_R (0x811A8)
#define C2RXF6INV        PIC32_R (0x811AC)
#define C2RXF7           PIC32_R (0x811B0)
#define C2RXF7CLR        PIC32_R (0x811B4)
#define C2RXF7SET        PIC32_R (0x811B8)
#define C2RXF7INV        PIC32_R (0x811BC)
#define C2RXF8           PIC32_R (0x811C0)
#define C2RXF8CLR        PIC32_R (0x811C4)
#define C2RXF8SET        PIC32_R (0x811C8)
#define C2RXF8INV        PIC32_R (0x811CC)
#define C2RXF9           PIC32_R (0x811D0)
#define C2RXF9CLR        PIC32_R (0x811D4)
#define C2RXF9SET        PIC32_R (0x811D8)
#define C2RXF9INV        PIC32_R (0x811DC)
#define C2RXF10          PIC32_R (0x811E0)
#define C2RXF10CLR       PIC32_R (0x811E4)
#define C2RXF10SET       PIC32_R (0x811E8)
#define C2RXF10INV       PIC32_R (0x811EC)
#define C2RXF11          PIC32_R (0x811F0)
#define C2RXF11CLR       PIC32_R (0x811F4)
#define C2RXF11SET       PIC32_R (0x811F8)
#define C2RXF11INV       PIC32_R (0x811FC)
#define C2RXF12          PIC32_R (0x81200)
#define C2RXF12CLR       PIC32_R (0x81204)
#define C2RXF12SET       PIC32_R (0x81208)
#define C2RXF12INV       PIC32_R (0x8120C)
#define C2RXF13          PIC32_R (0x81210)
#define C2RXF13CLR       PIC32_R (0x81214)
#define C2RXF13SET       PIC32_R (0x81218)
#define C2RXF13INV       PIC32_R (0x8121C)
#define C2RXF14          PIC32_R (0x81220)
#define C2RXF14CLR       PIC32_R (0x81224)
#define C2RXF14SET       PIC32_R (0x81228)
#define C2RXF14INV       PIC32_R (0x8122C)
#define C2RXF15          PIC32_R (0x81230)
#define C2RXF15CLR       PIC32_R (0x81234)
#define C2RXF15SET       PIC32_R (0x81238)
#define C2RXF15INV       PIC32_R (0x8123C)
#define C2RXF16          PIC32_R (0x81240)
#define C2RXF16CLR       PIC32_R (0x81244)
#define C2RXF16SET       PIC32_R (0x81248)
#define C2RXF16INV       PIC32_R (0x8124C)
#define C2RXF17          PIC32_R (0x81250)
#define C2RXF17CLR       PIC32_R (0x81254)
#define C2RXF17SET       PIC32_R (0x81258)
#define C2RXF17INV       PIC32_R (0x8125C)
#define C2RXF18          PIC32_R (0x81260)
#define C2RXF18CLR       PIC32_R (0x81264)
#define C2RXF18SET       PIC32_R (0x81268)
#define C2RXF18INV       PIC32_R (0x8126C)
#define C2RXF19          PIC32_R (0x81270)
#define C2RXF19CLR       PIC32_R (0x81274)
#define C2RXF19SET       PIC32_R (0x81278)
#define C2RXF19INV       PIC32_R (0x8127C)
#define C2RXF20          PIC32_R (0x81280)
#define C2RXF20CLR       PIC32_R (0x81284)
#define C2RXF20SET       PIC32_R (0x81288)
#define C2RXF20INV       PIC32_R (0x8128C)
#define C2RXF21          PIC32_R (0x81290)
#define C2RXF21CLR       PIC32_R (0x81294)
#define C2RXF21SET       PIC32_R (0x81298)
#define C2RXF21INV       PIC32_R (0x8129C)
#define C2RXF22          PIC32_R (0x812A0)
#define C2RXF22CLR       PIC32_R (0x812A4)
#define C2RXF22SET       PIC32_R (0x812A8)
#define C2RXF22INV       PIC32_R (0x812AC)
#define C2RXF23          PIC32_R (0x812B0)
#define C2RXF23CLR       PIC32_R (0x812B4)
#define C2RXF23SET       PIC32_R (0x812B8)
#define C2RXF23INV       PIC32_R (0x812BC)
#define C2RXF24          PIC32_R (0x812C0)
#define C2RXF24CLR       PIC32_R (0x812C4)
#define C2RXF24SET       PIC32_R (0x812C8)
#define C2RXF24INV       PIC32_R (0x812CC)
#define C2RXF25          PIC32_R (0x812D0)
#define C2RXF25CLR       PIC32_R (0x812D4)
#define C2RXF25SET       PIC32_R (0x812D8)
#define C2RXF25INV       PIC32_R (0x812DC)
#define C2RXF26          PIC32_R (0x812E0)
#define C2RXF26CLR       PIC32_R (0x812E4)
#define C2RXF26SET       PIC32_R (0x812E8)
#define C2RXF26INV       PIC32_R (0x812EC)
#define C2RXF27          PIC32_R (0x812F0)
#define C2RXF27CLR       PIC32_R (0x812F4)
#define C2RXF27SET       PIC32_R (0x812F8)
#define C2RXF27INV       PIC32_R (0x812FC)
#define C2RXF28          PIC32_R (0x81300)
#define C2RXF28CLR       PIC32_R (0x81304)
#define C2RXF28SET       PIC32_R (0x81308)
#define C2RXF28INV       PIC32_R (0x8130C)
#define C2RXF29          PIC32_R (0x81310)
#define C2RXF29CLR       PIC32_R (0x81314)
#define C2RXF29SET       PIC32_R (0x81318)
#define C2RXF29INV       PIC32_R (0x8131C)
#define C2RXF30          PIC32_R (0x81320)
#define C2RXF30CLR       PIC32_R (0x81324)
#define C2RXF30SET       PIC32_R (0x81328)
#define C2RXF30INV       PIC32_R (0x8132C)
#define C2RXF31          PIC32_R (0x81330)
#define C2RXF31CLR       PIC32_R (0x81334)
#define C2RXF31SET       PIC32_R (0x81338)
#define C2RXF31INV       PIC32_R (0x8133C)
#define C2FIFOBA         PIC32_R (0x81340)
#define C2FIFOBACLR      PIC32_R (0x81344)
#define C2FIFOBASET      PIC32_R (0x81348)
#define C2FIFOBAINV      PIC32_R (0x8134C)
#define C2FIFOCON0       PIC32_R (0x81350)
#define C2FIFOCON0CLR    PIC32_R (0x81354)
#define C2FIFOCON0SET    PIC32_R (0x81358)
#define C2FIFOCON0INV    PIC32_R (0x8135C)
#define C2FIFOINT0       PIC32_R (0x81360)
#define C2FIFOINT0CLR    PIC32_R (0x81364)
#define C2FIFOINT0SET    PIC32_R (0x81368)
#define C2FIFOINT0INV    PIC32_R (0x8136C)
#define C2FIFOUA0        PIC32_R (0x81370)
#define C2FIFOUA0CLR     PIC32_R (0x81374)
#define C2FIFOUA0SET     PIC32_R (0x81378)
#define C2FIFOUA0INV     PIC32_R (0x8137C)
#define C2FIFOCI0        PIC32_R (0x81380)
#define C2FIFOCI0CLR     PIC32_R (0x81384)
#define C2FIFOCI0SET     PIC32_R (0x81388)
#define C2FIFOCI0INV     PIC32_R (0x8138C)
#define C2FIFOCON1       PIC32_R (0x81390)
#define C2FIFOCON1CLR    PIC32_R (0x81394)
#define C2FIFOCON1SET    PIC32_R (0x81398)
#define C2FIFOCON1INV    PIC32_R (0x8139C)
#define C2FIFOINT1       PIC32_R (0x813A0)
#define C2FIFOINT1CLR    PIC32_R (0x813A4)
#define C2FIFOINT1SET    PIC32_R (0x813A8)
#define C2FIFOINT1INV    PIC32_R (0x813AC)
#define C2FIFOUA1        PIC32_R (0x813B0)
#define C2FIFOUA1CLR     PIC32_R (0x813B4)
#define C2FIFOUA1SET     PIC32_R (0x813B8)
#define C2FIFOUA1INV     PIC32_R (0x813BC)
#define C2FIFOCI1        PIC32_R (0x813C0)
#define C2FIFOCI1CLR     PIC32_R (0x813C4)
#define C2FIFOCI1SET     PIC32_R (0x813C8)
#define C2FIFOCI1INV     PIC32_R (0x813CC)
#define C2FIFOCON2       PIC32_R (0x813D0)
#define C2FIFOCON2CLR    PIC32_R (0x813D4)
#define C2FIFOCON2SET    PIC32_R (0x813D8)
#define C2FIFOCON2INV    PIC32_R (0x813DC)
#define C2FIFOINT2       PIC32_R (0x813E0)
#define C2FIFOINT2CLR    PIC32_R (0x813E4)
#define C2FIFOINT2SET    PIC32_R (0x813E8)
#define C2FIFOINT2INV    PIC32_R (0x813EC)
#define C2FIFOUA2        PIC32_R (0x813F0)
#define C2FIFOUA2CLR     PIC32_R (0x813F4)
#define C2FIFOUA2SET     PIC32_R (0x813F8)
#define C2FIFOUA2INV     PIC32_R (0x813FC)
#define C2FIFOCI2        PIC32_R (0x81400)
#define C2FIFOCI2CLR     PIC32_R (0x81404)
#define C2FIFOCI2SET     PIC32_R (0x81408)
#define C2FIFOCI2INV     PIC32_R (0x8140C)
#define C2FIFOCON3       PIC32_R (0x81410)
#define C2FIFOCON3CLR    PIC32_R (0x81414)
#define C2FIFOCON3SET    PIC32_R (0x81418)
#define C2FIFOCON3INV    PIC32_R (0x8141C)
#define C2FIFOINT3       PIC32_R (0x81420)
#define C2FIFOINT3CLR    PIC32_R (0x81424)
#define C2FIFOINT3SET    PIC32_R (0x81428)
#define C2FIFOINT3INV    PIC32_R (0x8142C)
#define C2FIFOUA3        PIC32_R (0x81430)
#define C2FIFOUA3CLR     PIC32_R (0x81434)
#define C2FIFOUA3SET     PIC32_R (0x81438)
#define C2FIFOUA3INV     PIC32_R (0x8143C)
#define C2FIFOCI3        PIC32_R (0x81440)
#define C2FIFOCI3CLR     PIC32_R (0x81444)
#define C2FIFOCI3SET     PIC32_R (0x81448)
#define C2FIFOCI3INV     PIC32_R (0x8144C)
#define C2FIFOCON4       PIC32_R (0x81450)
#define C2FIFOCON4CLR    PIC32_R (0x81454)
#define C2FIFOCON4SET    PIC32_R (0x81458)
#define C2FIFOCON4INV    PIC32_R (0x8145C)
#define C2FIFOINT4       PIC32_R (0x81460)
#define C2FIFOINT4CLR    PIC32_R (0x81464)
#define C2FIFOINT4SET    PIC32_R (0x81468)
#define C2FIFOINT4INV    PIC32_R (0x8146C)
#define C2FIFOUA4        PIC32_R (0x81470)
#define C2FIFOUA4CLR     PIC32_R (0x81474)
#define C2FIFOUA4SET     PIC32_R (0x81478)
#define C2FIFOUA4INV     PIC32_R (0x8147C)
#define C2FIFOCI4        PIC32_R (0x81480)
#define C2FIFOCI4CLR     PIC32_R (0x81484)
#define C2FIFOCI4SET     PIC32_R (0x81488)
#define C2FIFOCI4INV     PIC32_R (0x8148C)
#define C2FIFOCON5       PIC32_R (0x81490)
#define C2FIFOCON5CLR    PIC32_R (0x81494)
#define C2FIFOCON5SET    PIC32_R (0x81498)
#define C2FIFOCON5INV    PIC32_R (0x8149C)
#define C2FIFOINT5       PIC32_R (0x814A0)
#define C2FIFOINT5CLR    PIC32_R (0x814A4)
#define C2FIFOINT5SET    PIC32_R (0x814A8)
#define C2FIFOINT5INV    PIC32_R (0x814AC)
#define C2FIFOUA5        PIC32_R (0x814B0)
#define C2FIFOUA5CLR     PIC32_R (0x814B4)
#define C2FIFOUA5SET     PIC32_R (0x814B8)
#define C2FIFOUA5INV     PIC32_R (0x814BC)
#define C2FIFOCI5        PIC32_R (0x814C0)
#define C2FIFOCI5CLR     PIC32_R (0x814C4)
#define C2FIFOCI5SET     PIC32_R (0x814C8)
#define C2FIFOCI5INV     PIC32_R (0x814CC)
#define C2FIFOCON6       PIC32_R (0x814D0)
#define C2FIFOCON6CLR    PIC32_R (0x814D4)
#define C2FIFOCON6SET    PIC32_R (0x814D8)
#define C2FIFOCON6INV    PIC32_R (0x814DC)
#define C2FIFOINT6       PIC32_R (0x814E0)
#define C2FIFOINT6CLR    PIC32_R (0x814E4)
#define C2FIFOINT6SET    PIC32_R (0x814E8)
#define C2FIFOINT6INV    PIC32_R (0x814EC)
#define C2FIFOUA6        PIC32_R (0x814F0)
#define C2FIFOUA6CLR     PIC32_R (0x814F4)
#define C2FIFOUA6SET     PIC32_R (0x814F8)
#define C2FIFOUA6INV     PIC32_R (0x814FC)
#define C2FIFOCI6        PIC32_R (0x81500)
#define C2FIFOCI6CLR     PIC32_R (0x81504)
#define C2FIFOCI6SET     PIC32_R (0x81508)
#define C2FIFOCI6INV     PIC32_R (0x8150C)
#define C2FIFOCON7       PIC32_R (0x81510)
#define C2FIFOCON7CLR    PIC32_R (0x81514)
#define C2FIFOCON7SET    PIC32_R (0x81518)
#define C2FIFOCON7INV    PIC32_R (0x8151C)
#define C2FIFOINT7       PIC32_R (0x81520)
#define C2FIFOINT7CLR    PIC32_R (0x81524)
#define C2FIFOINT7SET    PIC32_R (0x81528)
#define C2FIFOINT7INV    PIC32_R (0x8152C)
#define C2FIFOUA7        PIC32_R (0x81530)
#define C2FIFOUA7CLR     PIC32_R (0x81534)
#define C2FIFOUA7SET     PIC32_R (0x81538)
#define C2FIFOUA7INV     PIC32_R (0x8153C)
#define C2FIFOCI7        PIC32_R (0x81540)
#define C2FIFOCI7CLR     PIC32_R (0x81544)
#define C2FIFOCI7SET     PIC32_R (0x81548)
#define C2FIFOCI7INV     PIC32_R (0x8154C)
#define C2FIFOCON8       PIC32_R (0x81550)
#define C2FIFOCON8CLR    PIC32_R (0x81554)
#define C2FIFOCON8SET    PIC32_R (0x81558)
#define C2FIFOCON8INV    PIC32_R (0x8155C)
#define C2FIFOINT8       PIC32_R (0x81560)
#define C2FIFOINT8CLR    PIC32_R (0x81564)
#define C2FIFOINT8SET    PIC32_R (0x81568)
#define C2FIFOINT8INV    PIC32_R (0x8156C)
#define C2FIFOUA8        PIC32_R (0x81570)
#define C2FIFOUA8CLR     PIC32_R (0x81574)
#define C2FIFOUA8SET     PIC32_R (0x81578)
#define C2FIFOUA8INV     PIC32_R (0x8157C)
#define C2FIFOCI8        PIC32_R (0x81580)
#define C2FIFOCI8CLR     PIC32_R (0x81584)
#define C2FIFOCI8SET     PIC32_R (0x81588)
#define C2FIFOCI8INV     PIC32_R (0x8158C)
#define C2FIFOCON9       PIC32_R (0x81590)
#define C2FIFOCON9CLR    PIC32_R (0x81594)
#define C2FIFOCON9SET    PIC32_R (0x81598)
#define C2FIFOCON9INV    PIC32_R (0x8159C)
#define C2FIFOINT9       PIC32_R (0x815A0)
#define C2FIFOINT9CLR    PIC32_R (0x815A4)
#define C2FIFOINT9SET    PIC32_R (0x815A8)
#define C2FIFOINT9INV    PIC32_R (0x815AC)
#define C2FIFOUA9        PIC32_R (0x815B0)
#define C2FIFOUA9CLR     PIC32_R (0x815B4)
#define C2FIFOUA9SET     PIC32_R (0x815B8)
#define C2FIFOUA9INV     PIC32_R (0x815BC)
#define C2FIFOCI9        PIC32_R (0x815C0)
#define C2FIFOCI9CLR     PIC32_R (0x815C4)
#define C2FIFOCI9SET     PIC32_R (0x815C8)
#define C2FIFOCI9INV     PIC32_R (0x815CC)
#define C2FIFOCON10      PIC32_R (0x815D0)
#define C2FIFOCON10CLR   PIC32_R (0x815D4)
#define C2FIFOCON10SET   PIC32_R (0x815D8)
#define C2FIFOCON10INV   PIC32_R (0x815DC)
#define C2FIFOINT10      PIC32_R (0x815E0)
#define C2FIFOINT10CLR   PIC32_R (0x815E4)
#define C2FIFOINT10SET   PIC32_R (0x815E8)
#define C2FIFOINT10INV   PIC32_R (0x815EC)
#define C2FIFOUA10       PIC32_R (0x815F0)
#define C2FIFOUA10CLR    PIC32_R (0x815F4)
#define C2FIFOUA10SET    PIC32_R (0x815F8)
#define C2FIFOUA10INV    PIC32_R (0x815FC)
#define C2FIFOCI10       PIC32_R (0x81600)
#define C2FIFOCI10CLR    PIC32_R (0x81604)
#define C2FIFOCI10SET    PIC32_R (0x81608)
#define C2FIFOCI10INV    PIC32_R (0x8160C)
#define C2FIFOCON11      PIC32_R (0x81610)
#define C2FIFOCON11CLR   PIC32_R (0x81614)
#define C2FIFOCON11SET   PIC32_R (0x81618)
#define C2FIFOCON11INV   PIC32_R (0x8161C)
#define C2FIFOINT11      PIC32_R (0x81620)
#define C2FIFOINT11CLR   PIC32_R (0x81624)
#define C2FIFOINT11SET   PIC32_R (0x81628)
#define C2FIFOINT11INV   PIC32_R (0x8162C)
#define C2FIFOUA11       PIC32_R (0x81630)
#define C2FIFOUA11CLR    PIC32_R (0x81634)
#define C2FIFOUA11SET    PIC32_R (0x81638)
#define C2FIFOUA11INV    PIC32_R (0x8163C)
#define C2FIFOCI11       PIC32_R (0x81640)
#define C2FIFOCI11CLR    PIC32_R (0x81644)
#define C2FIFOCI11SET    PIC32_R (0x81648)
#define C2FIFOCI11INV    PIC32_R (0x8164C)
#define C2FIFOCON12      PIC32_R (0x81650)
#define C2FIFOCON12CLR   PIC32_R (0x81654)
#define C2FIFOCON12SET   PIC32_R (0x81658)
#define C2FIFOCON12INV   PIC32_R (0x8165C)
#define C2FIFOINT12      PIC32_R (0x81660)
#define C2FIFOINT12CLR   PIC32_R (0x81664)
#define C2FIFOINT12SET   PIC32_R (0x81668)
#define C2FIFOINT12INV   PIC32_R (0x8166C)
#define C2FIFOUA12       PIC32_R (0x81670)
#define C2FIFOUA12CLR    PIC32_R (0x81674)
#define C2FIFOUA12SET    PIC32_R (0x81678)
#define C2FIFOUA12INV    PIC32_R (0x8167C)
#define C2FIFOCI12       PIC32_R (0x81680)
#define C2FIFOCI12CLR    PIC32_R (0x81684)
#define C2FIFOCI12SET    PIC32_R (0x81688)
#define C2FIFOCI12INV    PIC32_R (0x8168C)
#define C2FIFOCON13      PIC32_R (0x81690)
#define C2FIFOCON13CLR   PIC32_R (0x81694)
#define C2FIFOCON13SET   PIC32_R (0x81698)
#define C2FIFOCON13INV   PIC32_R (0x8169C)
#define C2FIFOINT13      PIC32_R (0x816A0)
#define C2FIFOINT13CLR   PIC32_R (0x816A4)
#define C2FIFOINT13SET   PIC32_R (0x816A8)
#define C2FIFOINT13INV   PIC32_R (0x816AC)
#define C2FIFOUA13       PIC32_R (0x816B0)
#define C2FIFOUA13CLR    PIC32_R (0x816B4)
#define C2FIFOUA13SET    PIC32_R (0x816B8)
#define C2FIFOUA13INV    PIC32_R (0x816BC)
#define C2FIFOCI13       PIC32_R (0x816C0)
#define C2FIFOCI13CLR    PIC32_R (0x816C4)
#define C2FIFOCI13SET    PIC32_R (0x816C8)
#define C2FIFOCI13INV    PIC32_R (0x816CC)
#define C2FIFOCON14      PIC32_R (0x816D0)
#define C2FIFOCON14CLR   PIC32_R (0x816D4)
#define C2FIFOCON14SET   PIC32_R (0x816D8)
#define C2FIFOCON14INV   PIC32_R (0x816DC)
#define C2FIFOINT14      PIC32_R (0x816E0)
#define C2FIFOINT14CLR   PIC32_R (0x816E4)
#define C2FIFOINT14SET   PIC32_R (0x816E8)
#define C2FIFOINT14INV   PIC32_R (0x816EC)
#define C2FIFOUA14       PIC32_R (0x816F0)
#define C2FIFOUA14CLR    PIC32_R (0x816F4)
#define C2FIFOUA14SET    PIC32_R (0x816F8)
#define C2FIFOUA14INV    PIC32_R (0x816FC)
#define C2FIFOCI14       PIC32_R (0x81700)
#define C2FIFOCI14CLR    PIC32_R (0x81704)
#define C2FIFOCI14SET    PIC32_R (0x81708)
#define C2FIFOCI14INV    PIC32_R (0x8170C)
#define C2FIFOCON15      PIC32_R (0x81710)
#define C2FIFOCON15CLR   PIC32_R (0x81714)
#define C2FIFOCON15SET   PIC32_R (0x81718)
#define C2FIFOCON15INV   PIC32_R (0x8171C)
#define C2FIFOINT15      PIC32_R (0x81720)
#define C2FIFOINT15CLR   PIC32_R (0x81724)
#define C2FIFOINT15SET   PIC32_R (0x81728)
#define C2FIFOINT15INV   PIC32_R (0x8172C)
#define C2FIFOUA15       PIC32_R (0x81730)
#define C2FIFOUA15CLR    PIC32_R (0x81734)
#define C2FIFOUA15SET    PIC32_R (0x81738)
#define C2FIFOUA15INV    PIC32_R (0x8173C)
#define C2FIFOCI15       PIC32_R (0x81740)
#define C2FIFOCI15CLR    PIC32_R (0x81744)
#define C2FIFOCI15SET    PIC32_R (0x81748)
#define C2FIFOCI15INV    PIC32_R (0x8174C)
#define C2FIFOCON16      PIC32_R (0x81750)
#define C2FIFOCON16CLR   PIC32_R (0x81754)
#define C2FIFOCON16SET   PIC32_R (0x81758)
#define C2FIFOCON16INV   PIC32_R (0x8175C)
#define C2FIFOINT16      PIC32_R (0x81760)
#define C2FIFOINT16CLR   PIC32_R (0x81764)
#define C2FIFOINT16SET   PIC32_R (0x81768)
#define C2FIFOINT16INV   PIC32_R (0x8176C)
#define C2FIFOUA16       PIC32_R (0x81770)
#define C2FIFOUA16CLR    PIC32_R (0x81774)
#define C2FIFOUA16SET    PIC32_R (0x81778)
#define C2FIFOUA16INV    PIC32_R (0x8177C)
#define C2FIFOCI16       PIC32_R (0x81780)
#define C2FIFOCI16CLR    PIC32_R (0x81784)
#define C2FIFOCI16SET    PIC32_R (0x81788)
#define C2FIFOCI16INV    PIC32_R (0x8178C)
#define C2FIFOCON17      PIC32_R (0x81790)
#define C2FIFOCON17CLR   PIC32_R (0x81794)
#define C2FIFOCON17SET   PIC32_R (0x81798)
#define C2FIFOCON17INV   PIC32_R (0x8179C)
#define C2FIFOINT17      PIC32_R (0x817A0)
#define C2FIFOINT17CLR   PIC32_R (0x817A4)
#define C2FIFOINT17SET   PIC32_R (0x817A8)
#define C2FIFOINT17INV   PIC32_R (0x817AC)
#define C2FIFOUA17       PIC32_R (0x817B0)
#define C2FIFOUA17CLR    PIC32_R (0x817B4)
#define C2FIFOUA17SET    PIC32_R (0x817B8)
#define C2FIFOUA17INV    PIC32_R (0x817BC)
#define C2FIFOCI17       PIC32_R (0x817C0)
#define C2FIFOCI17CLR    PIC32_R (0x817C4)
#define C2FIFOCI17SET    PIC32_R (0x817C8)
#define C2FIFOCI17INV    PIC32_R (0x817CC)
#define C2FIFOCON18      PIC32_R (0x817D0)
#define C2FIFOCON18CLR   PIC32_R (0x817D4)
#define C2FIFOCON18SET   PIC32_R (0x817D8)
#define C2FIFOCON18INV   PIC32_R (0x817DC)
#define C2FIFOINT18      PIC32_R (0x817E0)
#define C2FIFOINT18CLR   PIC32_R (0x817E4)
#define C2FIFOINT18SET   PIC32_R (0x817E8)
#define C2FIFOINT18INV   PIC32_R (0x817EC)
#define C2FIFOUA18       PIC32_R (0x817F0)
#define C2FIFOUA18CLR    PIC32_R (0x817F4)
#define C2FIFOUA18SET    PIC32_R (0x817F8)
#define C2FIFOUA18INV    PIC32_R (0x817FC)
#define C2FIFOCI18       PIC32_R (0x81800)
#define C2FIFOCI18CLR    PIC32_R (0x81804)
#define C2FIFOCI18SET    PIC32_R (0x81808)
#define C2FIFOCI18INV    PIC32_R (0x8180C)
#define C2FIFOCON19      PIC32_R (0x81810)
#define C2FIFOCON19CLR   PIC32_R (0x81814)
#define C2FIFOCON19SET   PIC32_R (0x81818)
#define C2FIFOCON19INV   PIC32_R (0x8181C)
#define C2FIFOINT19      PIC32_R (0x81820)
#define C2FIFOINT19CLR   PIC32_R (0x81824)
#define C2FIFOINT19SET   PIC32_R (0x81828)
#define C2FIFOINT19INV   PIC32_R (0x8182C)
#define C2FIFOUA19       PIC32_R (0x81830)
#define C2FIFOUA19CLR    PIC32_R (0x81834)
#define C2FIFOUA19SET    PIC32_R (0x81838)
#define C2FIFOUA19INV    PIC32_R (0x8183C)
#define C2FIFOCI19       PIC32_R (0x81840)
#define C2FIFOCI19CLR    PIC32_R (0x81844)
#define C2FIFOCI19SET    PIC32_R (0x81848)
#define C2FIFOCI19INV    PIC32_R (0x8184C)
#define C2FIFOCON20      PIC32_R (0x81850)
#define C2FIFOCON20CLR   PIC32_R (0x81854)
#define C2FIFOCON20SET   PIC32_R (0x81858)
#define C2FIFOCON20INV   PIC32_R (0x8185C)
#define C2FIFOINT20      PIC32_R (0x81860)
#define C2FIFOINT20CLR   PIC32_R (0x81864)
#define C2FIFOINT20SET   PIC32_R (0x81868)
#define C2FIFOINT20INV   PIC32_R (0x8186C)
#define C2FIFOUA20       PIC32_R (0x81870)
#define C2FIFOUA20CLR    PIC32_R (0x81874)
#define C2FIFOUA20SET    PIC32_R (0x81878)
#define C2FIFOUA20INV    PIC32_R (0x8187C)
#define C2FIFOCI20       PIC32_R (0x81880)
#define C2FIFOCI20CLR    PIC32_R (0x81884)
#define C2FIFOCI20SET    PIC32_R (0x81888)
#define C2FIFOCI20INV    PIC32_R (0x8188C)
#define C2FIFOCON21      PIC32_R (0x81890)
#define C2FIFOCON21CLR   PIC32_R (0x81894)
#define C2FIFOCON21SET   PIC32_R (0x81898)
#define C2FIFOCON21INV   PIC32_R (0x8189C)
#define C2FIFOINT21      PIC32_R (0x818A0)
#define C2FIFOINT21CLR   PIC32_R (0x818A4)
#define C2FIFOINT21SET   PIC32_R (0x818A8)
#define C2FIFOINT21INV   PIC32_R (0x818AC)
#define C2FIFOUA21       PIC32_R (0x818B0)
#define C2FIFOUA21CLR    PIC32_R (0x818B4)
#define C2FIFOUA21SET    PIC32_R (0x818B8)
#define C2FIFOUA21INV    PIC32_R (0x818BC)
#define C2FIFOCI21       PIC32_R (0x818C0)
#define C2FIFOCI21CLR    PIC32_R (0x818C4)
#define C2FIFOCI21SET    PIC32_R (0x818C8)
#define C2FIFOCI21INV    PIC32_R (0x818CC)
#define C2FIFOCON22      PIC32_R (0x818D0)
#define C2FIFOCON22CLR   PIC32_R (0x818D4)
#define C2FIFOCON22SET   PIC32_R (0x818D8)
#define C2FIFOCON22INV   PIC32_R (0x818DC)
#define C2FIFOINT22      PIC32_R (0x818E0)
#define C2FIFOINT22CLR   PIC32_R (0x818E4)
#define C2FIFOINT22SET   PIC32_R (0x818E8)
#define C2FIFOINT22INV   PIC32_R (0x818EC)
#define C2FIFOUA22       PIC32_R (0x818F0)
#define C2FIFOUA22CLR    PIC32_R (0x818F4)
#define C2FIFOUA22SET    PIC32_R (0x818F8)
#define C2FIFOUA22INV    PIC32_R (0x818FC)
#define C2FIFOCI22       PIC32_R (0x81900)
#define C2FIFOCI22CLR    PIC32_R (0x81904)
#define C2FIFOCI22SET    PIC32_R (0x81908)
#define C2FIFOCI22INV    PIC32_R (0x8190C)
#define C2FIFOCON23      PIC32_R (0x81910)
#define C2FIFOCON23CLR   PIC32_R (0x81914)
#define C2FIFOCON23SET   PIC32_R (0x81918)
#define C2FIFOCON23INV   PIC32_R (0x8191C)
#define C2FIFOINT23      PIC32_R (0x81920)
#define C2FIFOINT23CLR   PIC32_R (0x81924)
#define C2FIFOINT23SET   PIC32_R (0x81928)
#define C2FIFOINT23INV   PIC32_R (0x8192C)
#define C2FIFOUA23       PIC32_R (0x81930)
#define C2FIFOUA23CLR    PIC32_R (0x81934)
#define C2FIFOUA23SET    PIC32_R (0x81938)
#define C2FIFOUA23INV    PIC32_R (0x8193C)
#define C2FIFOCI23       PIC32_R (0x81940)
#define C2FIFOCI23CLR    PIC32_R (0x81944)
#define C2FIFOCI23SET    PIC32_R (0x81948)
#define C2FIFOCI23INV    PIC32_R (0x8194C)
#define C2FIFOCON24      PIC32_R (0x81950)
#define C2FIFOCON24CLR   PIC32_R (0x81954)
#define C2FIFOCON24SET   PIC32_R (0x81958)
#define C2FIFOCON24INV   PIC32_R (0x8195C)
#define C2FIFOINT24      PIC32_R (0x81960)
#define C2FIFOINT24CLR   PIC32_R (0x81964)
#define C2FIFOINT24SET   PIC32_R (0x81968)
#define C2FIFOINT24INV   PIC32_R (0x8196C)
#define C2FIFOUA24       PIC32_R (0x81970)
#define C2FIFOUA24CLR    PIC32_R (0x81974)
#define C2FIFOUA24SET    PIC32_R (0x81978)
#define C2FIFOUA24INV    PIC32_R (0x8197C)
#define C2FIFOCI24       PIC32_R (0x81980)
#define C2FIFOCI24CLR    PIC32_R (0x81984)
#define C2FIFOCI24SET    PIC32_R (0x81988)
#define C2FIFOCI24INV    PIC32_R (0x8198C)
#define C2FIFOCON25      PIC32_R (0x81990)
#define C2FIFOCON25CLR   PIC32_R (0x81994)
#define C2FIFOCON25SET   PIC32_R (0x81998)
#define C2FIFOCON25INV   PIC32_R (0x8199C)
#define C2FIFOINT25      PIC32_R (0x819A0)
#define C2FIFOINT25CLR   PIC32_R (0x819A4)
#define C2FIFOINT25SET   PIC32_R (0x819A8)
#define C2FIFOINT25INV   PIC32_R (0x819AC)
#define C2FIFOUA25       PIC32_R (0x819B0)
#define C2FIFOUA25CLR    PIC32_R (0x819B4)
#define C2FIFOUA25SET    PIC32_R (0x819B8)
#define C2FIFOUA25INV    PIC32_R (0x819BC)
#define C2FIFOCI25       PIC32_R (0x819C0)
#define C2FIFOCI25CLR    PIC32_R (0x819C4)
#define C2FIFOCI25SET    PIC32_R (0x819C8)
#define C2FIFOCI25INV    PIC32_R (0x819CC)
#define C2FIFOCON26      PIC32_R (0x819D0)
#define C2FIFOCON26CLR   PIC32_R (0x819D4)
#define C2FIFOCON26SET   PIC32_R (0x819D8)
#define C2FIFOCON26INV   PIC32_R (0x819DC)
#define C2FIFOINT26      PIC32_R (0x819E0)
#define C2FIFOINT26CLR   PIC32_R (0x819E4)
#define C2FIFOINT26SET   PIC32_R (0x819E8)
#define C2FIFOINT26INV   PIC32_R (0x819EC)
#define C2FIFOUA26       PIC32_R (0x819F0)
#define C2FIFOUA26CLR    PIC32_R (0x819F4)
#define C2FIFOUA26SET    PIC32_R (0x819F8)
#define C2FIFOUA26INV    PIC32_R (0x819FC)
#define C2FIFOCI26       PIC32_R (0x81A00)
#define C2FIFOCI26CLR    PIC32_R (0x81A04)
#define C2FIFOCI26SET    PIC32_R (0x81A08)
#define C2FIFOCI26INV    PIC32_R (0x81A0C)
#define C2FIFOCON27      PIC32_R (0x81A10)
#define C2FIFOCON27CLR   PIC32_R (0x81A14)
#define C2FIFOCON27SET   PIC32_R (0x81A18)
#define C2FIFOCON27INV   PIC32_R (0x81A1C)
#define C2FIFOINT27      PIC32_R (0x81A20)
#define C2FIFOINT27CLR   PIC32_R (0x81A24)
#define C2FIFOINT27SET   PIC32_R (0x81A28)
#define C2FIFOINT27INV   PIC32_R (0x81A2C)
#define C2FIFOUA27       PIC32_R (0x81A30)
#define C2FIFOUA27CLR    PIC32_R (0x81A34)
#define C2FIFOUA27SET    PIC32_R (0x81A38)
#define C2FIFOUA27INV    PIC32_R (0x81A3C)
#define C2FIFOCI27       PIC32_R (0x81A40)
#define C2FIFOCI27CLR    PIC32_R (0x81A44)
#define C2FIFOCI27SET    PIC32_R (0x81A48)
#define C2FIFOCI27INV    PIC32_R (0x81A4C)
#define C2FIFOCON28      PIC32_R (0x81A50)
#define C2FIFOCON28CLR   PIC32_R (0x81A54)
#define C2FIFOCON28SET   PIC32_R (0x81A58)
#define C2FIFOCON28INV   PIC32_R (0x81A5C)
#define C2FIFOINT28      PIC32_R (0x81A60)
#define C2FIFOINT28CLR   PIC32_R (0x81A64)
#define C2FIFOINT28SET   PIC32_R (0x81A68)
#define C2FIFOINT28INV   PIC32_R (0x81A6C)
#define C2FIFOUA28       PIC32_R (0x81A70)
#define C2FIFOUA28CLR    PIC32_R (0x81A74)
#define C2FIFOUA28SET    PIC32_R (0x81A78)
#define C2FIFOUA28INV    PIC32_R (0x81A7C)
#define C2FIFOCI28       PIC32_R (0x81A80)
#define C2FIFOCI28CLR    PIC32_R (0x81A84)
#define C2FIFOCI28SET    PIC32_R (0x81A88)
#define C2FIFOCI28INV    PIC32_R (0x81A8C)
#define C2FIFOCON29      PIC32_R (0x81A90)
#define C2FIFOCON29CLR   PIC32_R (0x81A94)
#define C2FIFOCON29SET   PIC32_R (0x81A98)
#define C2FIFOCON29INV   PIC32_R (0x81A9C)
#define C2FIFOINT29      PIC32_R (0x81AA0)
#define C2FIFOINT29CLR   PIC32_R (0x81AA4)
#define C2FIFOINT29SET   PIC32_R (0x81AA8)
#define C2FIFOINT29INV   PIC32_R (0x81AAC)
#define C2FIFOUA29       PIC32_R (0x81AB0)
#define C2FIFOUA29CLR    PIC32_R (0x81AB4)
#define C2FIFOUA29SET    PIC32_R (0x81AB8)
#define C2FIFOUA29INV    PIC32_R (0x81ABC)
#define C2FIFOCI29       PIC32_R (0x81AC0)
#define C2FIFOCI29CLR    PIC32_R (0x81AC4)
#define C2FIFOCI29SET    PIC32_R (0x81AC8)
#define C2FIFOCI29INV    PIC32_R (0x81ACC)
#define C2FIFOCON30      PIC32_R (0x81AD0)
#define C2FIFOCON30CLR   PIC32_R (0x81AD4)
#define C2FIFOCON30SET   PIC32_R (0x81AD8)
#define C2FIFOCON30INV   PIC32_R (0x81ADC)
#define C2FIFOINT30      PIC32_R (0x81AE0)
#define C2FIFOINT30CLR   PIC32_R (0x81AE4)
#define C2FIFOINT30SET   PIC32_R (0x81AE8)
#define C2FIFOINT30INV   PIC32_R (0x81AEC)
#define C2FIFOUA30       PIC32_R (0x81AF0)
#define C2FIFOUA30CLR    PIC32_R (0x81AF4)
#define C2FIFOUA30SET    PIC32_R (0x81AF8)
#define C2FIFOUA30INV    PIC32_R (0x81AFC)
#define C2FIFOCI30       PIC32_R (0x81B00)
#define C2FIFOCI30CLR    PIC32_R (0x81B04)
#define C2FIFOCI30SET    PIC32_R (0x81B08)
#define C2FIFOCI30INV    PIC32_R (0x81B0C)
#define C2FIFOCON31      PIC32_R (0x81B10)
#define C2FIFOCON31CLR   PIC32_R (0x81B14)
#define C2FIFOCON31SET   PIC32_R (0x81B18)
#define C2FIFOCON31INV   PIC32_R (0x81B1C)
#define C2FIFOINT31      PIC32_R (0x81B20)
#define C2FIFOINT31CLR   PIC32_R (0x81B24)
#define C2FIFOINT31SET   PIC32_R (0x81B28)
#define C2FIFOINT31INV   PIC32_R (0x81B2C)
#define C2FIFOUA31       PIC32_R (0x81B30)
#define C2FIFOUA31CLR    PIC32_R (0x81B34)
#define C2FIFOUA31SET    PIC32_R (0x81B38)
#define C2FIFOUA31INV    PIC32_R (0x81B3C)
#define C2FIFOCI31       PIC32_R (0x81B40)
#define C2FIFOCI31CLR    PIC32_R (0x81B44)
#define C2FIFOCI31SET    PIC32_R (0x81B48)
#define C2FIFOCI31INV    PIC32_R (0x81B4C)
		
/*--------------------------------------
 * Ethernet registers.
 */
#define ETHCON1         PIC32_R (0x82000)   /* Control 1 */
#define ETHCON1CLR      PIC32_R (0x82004)
#define ETHCON1SET      PIC32_R (0x82008)
#define ETHCON1INV      PIC32_R (0x8200c)
#define ETHCON2         PIC32_R (0x82010)   /* Control 2: RX data buffer size */
#define ETHTXST         PIC32_R (0x82020)   /* Tx descriptor start address */
#define ETHRXST         PIC32_R (0x82030)   /* Rx descriptor start address */
#define ETHHT0          PIC32_R (0x82040)   /* Hash tasble 0 */
#define ETHHT1          PIC32_R (0x82050)   /* Hash tasble 1 */
#define ETHPMM0         PIC32_R (0x82060)   /* Pattern match mask 0 */
#define ETHPMM1         PIC32_R (0x82070)   /* Pattern match mask 1 */
#define ETHPMCS         PIC32_R (0x82080)   /* Pattern match checksum */
#define ETHPMO          PIC32_R (0x82090)   /* Pattern match offset */
#define ETHRXFC         PIC32_R (0x820a0)   /* Receive filter configuration */
#define ETHRXWM         PIC32_R (0x820b0)   /* Receive watermarks */
#define ETHIEN          PIC32_R (0x820c0)   /* Interrupt enable */
#define ETHIENCLR       PIC32_R (0x820c4)
#define ETHIENSET       PIC32_R (0x820c8)
#define ETHIENINV       PIC32_R (0x820cc)
#define ETHIRQ          PIC32_R (0x820d0)   /* Interrupt request */
#define ETHIRQCLR       PIC32_R (0x820d4)
#define ETHIRQSET       PIC32_R (0x820d8)
#define ETHIRQINV       PIC32_R (0x820dc)
#define ETHSTAT         PIC32_R (0x820e0)   /* Status */
#define ETHRXOVFLOW     PIC32_R (0x82100)   /* Receive overflow statistics */
#define ETHFRMTXOK      PIC32_R (0x82110)   /* Frames transmitted OK statistics */
#define ETHSCOLFRM      PIC32_R (0x82120)   /* Single collision frames statistics */
#define ETHMCOLFRM      PIC32_R (0x82130)   /* Multiple collision frames statistics */
#define ETHFRMRXOK      PIC32_R (0x82140)   /* Frames received OK statistics */
#define ETHFCSERR       PIC32_R (0x82150)   /* Frame check sequence error statistics */
#define ETHALGNERR      PIC32_R (0x82160)   /* Alignment errors statistics */
#define EMAC1CFG1       PIC32_R (0x82200)   /* MAC configuration 1 */
#define EMAC1CFG2       PIC32_R (0x82210)   /* MAC configuration 2 */
#define EMAC1CFG2CLR    PIC32_R (0x82214)
#define EMAC1CFG2SET    PIC32_R (0x82218)
#define EMAC1CFG2INV    PIC32_R (0x8221c)
#define EMAC1IPGT       PIC32_R (0x82220)   /* MAC back-to-back interpacket gap */
#define EMAC1IPGR       PIC32_R (0x82230)   /* MAC non-back-to-back interpacket gap */
#define EMAC1CLRT       PIC32_R (0x82240)   /* MAC collision window/retry limit */
#define EMAC1MAXF       PIC32_R (0x82250)   /* MAC maximum frame length */
#define EMAC1SUPP       PIC32_R (0x82260)   /* MAC PHY support */
#define EMAC1SUPPCLR    PIC32_R (0x82264)
#define EMAC1SUPPSET    PIC32_R (0x82268)
#define EMAC1SUPPINV    PIC32_R (0x8226c)
#define EMAC1TEST       PIC32_R (0x82270)   /* MAC test */
#define EMAC1MCFG       PIC32_R (0x82280)   /* MII configuration */
#define EMAC1MCMD       PIC32_R (0x82290)   /* MII command */
#define EMAC1MCMDCLR    PIC32_R (0x82294)
#define EMAC1MCMDSET    PIC32_R (0x82298)
#define EMAC1MCMDINV    PIC32_R (0x8229c)
#define EMAC1MADR       PIC32_R (0x822a0)   /* MII address */
#define EMAC1MWTD       PIC32_R (0x822b0)   /* MII write data */
#define EMAC1MRDD       PIC32_R (0x822c0)   /* MII read data */
#define EMAC1MIND       PIC32_R (0x822d0)   /* MII indicators */
#define EMAC1SA0        PIC32_R (0x82300)   /* MAC station address 0 */
#define EMAC1SA1        PIC32_R (0x82310)   /* MAC station address 1 */
#define EMAC1SA2        PIC32_R (0x82320)   /* MAC station address 2 */

/*
 * Ethernet Control register 1.
 */
#define PIC32_ETHCON1_PTV(n)    ((n)<<16)   /* Pause timer value */
#define PIC32_ETHCON1_ON            0x8000  /* Ethernet module enabled */
#define PIC32_ETHCON1_SIDL          0x2000  /* Stop in idle mode */
#define PIC32_ETHCON1_TXRTS         0x0200  /* Transmit request to send */
#define PIC32_ETHCON1_RXEN          0x0100  /* Receive enable */
#define PIC32_ETHCON1_AUTOFC        0x0080  /* Automatic flow control */
#define PIC32_ETHCON1_MANFC         0x0010  /* Manual flow control */
#define PIC32_ETHCON1_BUFCDEC       0x0001  /* Descriptor buffer count decrement */

/*
 * Ethernet Receive Filter Configuration register.
 */
#define PIC32_ETHRXFC_HTEN          0x8000  /* Enable hash table filtering */
#define PIC32_ETHRXFC_MPEN          0x4000  /* Enable Magic Packet filtering */
#define PIC32_ETHRXFC_NOTPM         0x1000  /* Pattern match inversion */
#define PIC32_ETHRXFC_PMMODE_MAGIC  0x0900  /* Packet = magic */
#define PIC32_ETHRXFC_PMMODE_HT     0x0800  /* Hash table filter match */
#define PIC32_ETHRXFC_PMMODE_BCAST  0x0600  /* Destination = broadcast address */
#define PIC32_ETHRXFC_PMMODE_UCAST  0x0400  /* Destination = unicast address */
#define PIC32_ETHRXFC_PMMODE_STN    0x0200  /* Destination = station address */
#define PIC32_ETHRXFC_PMMODE_CSUM   0x0100  /* Successful if checksum matches */
#define PIC32_ETHRXFC_CRCERREN      0x0080  /* CRC error collection enable */
#define PIC32_ETHRXFC_CRCOKEN       0x0040  /* CRC OK enable */
#define PIC32_ETHRXFC_RUNTERREN     0x0020  /* Runt error collection enable */
#define PIC32_ETHRXFC_RUNTEN        0x0010  /* Runt filter enable */
#define PIC32_ETHRXFC_UCEN          0x0008  /* Unicast filter enable */
#define PIC32_ETHRXFC_NOTMEEN       0x0004  /* Not Me unicast enable */
#define PIC32_ETHRXFC_MCEN          0x0002  /* Multicast filter enable */
#define PIC32_ETHRXFC_BCEN          0x0001  /* Broadcast filter enable */

/*
 * Ethernet Receive Watermarks register.
 */
#define PIC32_ETHRXWM_FWM(n)    ((n)<<16)   /* Receive Full Watermark */
#define PIC32_ETHRXWM_EWM(n)    (n)         /* Receive Empty Watermark */

/*
 * Ethernet Interrupt Request register.
 */
#define PIC32_ETHIRQ_TXBUSE         0x4000  /* Transmit Bus Error */
#define PIC32_ETHIRQ_RXBUSE         0x2000  /* Receive Bus Error */
#define PIC32_ETHIRQ_EWMARK         0x0200  /* Empty Watermark */
#define PIC32_ETHIRQ_FWMARK         0x0100  /* Full Watermark */
#define PIC32_ETHIRQ_RXDONE         0x0080  /* Receive Done */
#define PIC32_ETHIRQ_PKTPEND        0x0040  /* Packet Pending */
#define PIC32_ETHIRQ_RXACT          0x0020  /* Receive Activity */
#define PIC32_ETHIRQ_TXDONE         0x0008  /* Transmit Done */
#define PIC32_ETHIRQ_TXABORT        0x0004  /* Transmitter Abort */
#define PIC32_ETHIRQ_RXBUFNA        0x0002  /* Receive Buffer Not Available */
#define PIC32_ETHIRQ_RXOVFLW        0x0001  /* Receive FIFO Overflow */

/*
 * Ethernet Status register.
 */
#define PIC32_ETHSTAT_BUFCNT    0x00ff0000  /* Packet buffer count */
#define PIC32_ETHSTAT_ETHBUSY       0x0080  /* Ethernet logic is busy */
#define PIC32_ETHSTAT_TXBUSY        0x0040  /* TX logic is receiving data */
#define PIC32_ETHSTAT_RXBUSY        0x0020  /* RX logic is receiving data */

/*
 * Ethernet MAC configuration register 1.
 */
#define PIC32_EMAC1CFG1_SOFTRESET   0x8000  /* Soft reset */
#define PIC32_EMAC1CFG1_SIMRESET    0x4000  /* Reset TX random number generator */
#define PIC32_EMAC1CFG1_RESETRMCS   0x0800  /* Reset MCS/RX logic */
#define PIC32_EMAC1CFG1_RESETRFUN   0x0400  /* Reset RX function */
#define PIC32_EMAC1CFG1_RESETTMCS   0x0200  /* Reset MCS/TX logic */
#define PIC32_EMAC1CFG1_RESETTFUN   0x0100  /* Reset TX function */
#define PIC32_EMAC1CFG1_LOOPBACK    0x0010  /* MAC Loopback mode */
#define PIC32_EMAC1CFG1_TXPAUSE     0x0008  /* MAC TX flow control */
#define PIC32_EMAC1CFG1_RXPAUSE     0x0004  /* MAC RX flow control */
#define PIC32_EMAC1CFG1_PASSALL     0x0002  /* MAC accept control frames as well */
#define PIC32_EMAC1CFG1_RXENABLE    0x0001  /* MAC Receive Enable */

/*
 * Ethernet MAC configuration register 2.
 */
#define PIC32_EMAC1CFG2_EXCESSDFR   0x4000  /* Defer to carrier indefinitely */
#define PIC32_EMAC1CFG2_BPNOBKOFF   0x2000  /* Backpressure/No Backoff */
#define PIC32_EMAC1CFG2_NOBKOFF     0x1000  /* No Backoff */
#define PIC32_EMAC1CFG2_LONGPRE     0x0200  /* Long preamble enforcement */
#define PIC32_EMAC1CFG2_PUREPRE     0x0100  /* Pure preamble enforcement */
#define PIC32_EMAC1CFG2_AUTOPAD     0x0080  /* Automatic detect pad enable */
#define PIC32_EMAC1CFG2_VLANPAD     0x0040  /* VLAN pad enable */
#define PIC32_EMAC1CFG2_PADENABLE   0x0020  /* Pad/CRC enable */
#define PIC32_EMAC1CFG2_CRCENABLE   0x0010  /* CRC enable */
#define PIC32_EMAC1CFG2_DELAYCRC    0x0008  /* Delayed CRC */
#define PIC32_EMAC1CFG2_HUGEFRM     0x0004  /* Huge frame enable */
#define PIC32_EMAC1CFG2_LENGTHCK    0x0002  /* Frame length checking */
#define PIC32_EMAC1CFG2_FULLDPLX    0x0001  /* Full-duplex operation */

/*
 * Ethernet MAC non-back-to-back interpacket gap register.
 */
#define PIC32_EMAC1IPGR(p1, p2)     ((p1)<<8 | (p2))

/*
 * Ethernet MAC collision window/retry limit register.
 */
#define PIC32_EMAC1CLRT(w, r)       ((w)<<8 | (r))

/*
 * Ethernet PHY support register.
 */
#define PIC32_EMAC1SUPP_RESETRMII   0x0800  /* Reset the RMII module */
#define PIC32_EMAC1SUPP_SPEEDRMII   0x0100  /* RMII speed: 1=100Mbps, 0=10Mbps */

/*
 * Ethernet MAC test register.
 */
#define PIC32_EMAC1TEST_TESTBP      0x0004  /* Test backpressure */
#define PIC32_EMAC1TEST_TESTPAUSE   0x0002  /* Test pause */
#define PIC32_EMAC1TEST_SHRTQNTA    0x0001  /* Shortcut pause quanta */

/*
 * Ethernet MII configuration register.
 */
#define PIC32_EMAC1MCFG_RESETMGMT   0x8000  /* Reset the MII module */
#define PIC32_EMAC1MCFG_CLKSEL_4    0x0000  /* Clock divide by 4 */
#define PIC32_EMAC1MCFG_CLKSEL_6    0x0008  /* Clock divide by 6 */
#define PIC32_EMAC1MCFG_CLKSEL_8    0x000c  /* Clock divide by 8 */
#define PIC32_EMAC1MCFG_CLKSEL_10   0x0010  /* Clock divide by 10 */
#define PIC32_EMAC1MCFG_CLKSEL_14   0x0014  /* Clock divide by 14 */
#define PIC32_EMAC1MCFG_CLKSEL_20   0x0018  /* Clock divide by 20 */
#define PIC32_EMAC1MCFG_CLKSEL_28   0x001c  /* Clock divide by 28 */
#define PIC32_EMAC1MCFG_CLKSEL_40   0x0020  /* Clock divide by 40 */
#define PIC32_EMAC1MCFG_CLKSEL_48   0x0024  /* Clock divide by 48 */
#define PIC32_EMAC1MCFG_CLKSEL_50   0x0028  /* Clock divide by 50 */
#define PIC32_EMAC1MCFG_NOPRE       0x0002  /* Suppress preamble */
#define PIC32_EMAC1MCFG_SCANINC     0x0001  /* Scan increment */

/*
 * Ethernet MII command register.
 */
#define PIC32_EMAC1MCMD_SCAN        0x0002  /* Continuous scan mode */
#define PIC32_EMAC1MCMD_READ        0x0001  /* Single read cycle */

/*
 * Ethernet MII address register.
 */
#define PIC32_EMAC1MADR(p, r)       ((p)<<8 | (r))

/*
 * Ethernet MII indicators register.
 */
#define PIC32_EMAC1MIND_LINKFAIL    0x0008  /* Link fail */
#define PIC32_EMAC1MIND_NOTVALID    0x0004  /* Read data not valid */
#define PIC32_EMAC1MIND_SCAN        0x0002  /* Scanning in progress */
#define PIC32_EMAC1MIND_MIIMBUSY    0x0001  /* Read/write cycle in progress */

/*--------------------------------------
 * Interrupt controller registers.
 */
#define INTCON          PIC32_R (0x10000)       /* Interrupt Control */
#define INTCONCLR       PIC32_R (0x10004)
#define INTCONSET       PIC32_R (0x10008)
#define INTCONINV       PIC32_R (0x1000C)
#define PRISS           PIC32_R (0x10010)       /* Priority Shadow Select */
#define PRISSCLR        PIC32_R (0x10014)
#define PRISSSET        PIC32_R (0x10018)
#define PRISSINV        PIC32_R (0x1001C)
#define INTSTAT         PIC32_R (0x10020)       /* Interrupt Status */
#define IPTMR           PIC32_R (0x10030)       /* Temporal Proximity Timer */
#define IPTMRCLR        PIC32_R (0x10034)
#define IPTMRSET        PIC32_R (0x10038)
#define IPTMRINV        PIC32_R (0x1003C)
#define IFS(n)          PIC32_R (0x10040+((n)<<4)) /* IFS(0..5) - Interrupt Flag Status */
#define IFSCLR(n)       PIC32_R (0x10044+((n)<<4))
#define IFSSET(n)       PIC32_R (0x10048+((n)<<4))
#define IFSINV(n)       PIC32_R (0x1004C+((n)<<4))
#define IFS0            IFS(0)
#define IFS0CLR         0xBF810044
#define IFS1            IFS(1)
#define IFS2            IFS(2)
#define IFS3            IFS(3)
#define IFS4            IFS(4)
#define IFS5            IFS(5)
#define IEC(n)          PIC32_R (0x100c0+((n)<<4)) /* IEC(0..5) - Interrupt Enable Control */
#define IECCLR(n)       PIC32_R (0x100c4+((n)<<4))
#define IECSET(n)       PIC32_R (0x100c8+((n)<<4))
#define IECINV(n)       PIC32_R (0x100cC+((n)<<4))
#define IEC0            IEC(0)
#define IEC1            IEC(1)
#define IEC2            IEC(2)
#define IEC3            IEC(3)
#define IEC4            IEC(4)
#define IEC5            IEC(5)
#define IPC(n)          PIC32_R (0x10140+((n)<<4)) /* IPC(0..47) - Interrupt Priority Control */
#define IPCCLR(n)       PIC32_R (0x10144+((n)<<4))
#define IPCSET(n)       PIC32_R (0x10148+((n)<<4))
#define IPCINV(n)       PIC32_R (0x1014C+((n)<<4))
#define IPC0            IPC(0)
#define IPC1            IPC(1)
#define IPC2            IPC(2)
#define IPC3            IPC(3)
#define IPC4            IPC(4)
#define IPC5            IPC(5)
#define IPC6            IPC(6)
#define IPC7            IPC(7)
#define IPC8            IPC(8)
#define IPC9            IPC(9)
#define IPC10           IPC(10)
#define IPC11           IPC(11)
#define IPC12           IPC(12)
#define IPC13           IPC(13)
#define IPC14           IPC(14)
#define IPC15           IPC(15)
#define IPC16           IPC(16)
#define IPC17           IPC(17)
#define IPC18           IPC(18)
#define IPC19           IPC(19)
#define IPC20           IPC(20)
#define IPC21           IPC(21)
#define IPC22           IPC(22)
#define IPC23           IPC(23)
#define IPC24           IPC(24)
#define IPC25           IPC(25)
#define IPC26           IPC(26)
#define IPC27           IPC(27)
#define IPC28           IPC(28)
#define IPC29           IPC(29)
#define IPC30           IPC(30)
#define IPC31           IPC(31)
#define IPC32           IPC(32)
#define IPC33           IPC(33)
#define IPC34           IPC(34)
#define IPC35           IPC(35)
#define IPC36           IPC(36)
#define IPC37           IPC(37)
#define IPC38           IPC(38)
#define IPC39           IPC(39)
#define IPC40           IPC(40)
#define IPC41           IPC(41)
#define IPC42           IPC(42)
#define IPC43           IPC(43)
#define IPC44           IPC(44)
#define IPC45           IPC(45)
#define IPC46           IPC(46)
#define IPC47           IPC(47)
#define OFF(n)          PIC32_R (0x10540+((n)<<2)) /* OFF(0..190) - Interrupt Vector Address Offset */

/*
 * Interrupt Control register.
 */
#define PIC32_INTCON_INT0EP     0x00000001  /* External interrupt 0 polarity rising edge */
#define PIC32_INTCON_INT1EP     0x00000002  /* External interrupt 1 polarity rising edge */
#define PIC32_INTCON_INT2EP     0x00000004  /* External interrupt 2 polarity rising edge */
#define PIC32_INTCON_INT3EP     0x00000008  /* External interrupt 3 polarity rising edge */
#define PIC32_INTCON_INT4EP     0x00000010  /* External interrupt 4 polarity rising edge */
#define PIC32_INTCON_TPC(x)     ((x)<<8)    /* Temporal proximity group priority */
#define PIC32_INTCON_MVEC       0x00001000  /* Multi-vectored mode */
#define PIC32_INTCON_SS0        0x00010000  /* Single vector has a shadow register set */
#define PIC32_INTCON_VS(x)      ((x)<<16)   /* Temporal proximity group priority */

/*
 * Interrupt Status register.
 */
#define PIC32_INTSTAT_VEC(s)    ((s) & 0xff)    /* Interrupt vector */
#define PIC32_INTSTAT_SRIPL(s)  ((s) >> 8 & 7)  /* Requested priority level */
#define PIC32_INTSTAT_SRIPL_MASK 0x0700

/*
 * Interrupt Prority Control register.
 */
#define PIC32_IPC_IP(a,b,c,d)   ((a)<<2 | (b)<<10 | (c)<<18 | (d)<<26)  /* Priority */
#define PIC32_IPC_IS(a,b,c,d)   ((a) | (b)<<8 | (c)<<16 | (d)<<24)      /* Subpriority */

/*
 * IRQ numbers for PIC32MZ
 */
#define PIC32_IRQ_CT        0   /* Core Timer Interrupt */
#define PIC32_IRQ_CS0       1   /* Core Software Interrupt 0 */
#define PIC32_IRQ_CS1       2   /* Core Software Interrupt 1 */
#define PIC32_IRQ_INT0      3   /* External Interrupt 0 */
#define PIC32_IRQ_T1        4   /* Timer1 */
#define PIC32_IRQ_IC1E      5   /* Input Capture 1 Error */
#define PIC32_IRQ_IC1       6   /* Input Capture 1 */
#define PIC32_IRQ_OC1       7   /* Output Compare 1 */
#define PIC32_IRQ_INT1      8   /* External Interrupt 1 */
#define PIC32_IRQ_T2        9   /* Timer2 */
#define PIC32_IRQ_IC2E      10  /* Input Capture 2 Error */
#define PIC32_IRQ_IC2       11  /* Input Capture 2 */
#define PIC32_IRQ_OC2       12  /* Output Compare 2 */
#define PIC32_IRQ_INT2      13  /* External Interrupt 2 */
#define PIC32_IRQ_T3        14  /* Timer3 */
#define PIC32_IRQ_IC3E      15  /* Input Capture 3 Error */
#define PIC32_IRQ_IC3       16  /* Input Capture 3 */
#define PIC32_IRQ_OC3       17  /* Output Compare 3 */
#define PIC32_IRQ_INT3      18  /* External Interrupt 3 */
#define PIC32_IRQ_T4        19  /* Timer4 */
#define PIC32_IRQ_IC4E      20  /* Input Capture 4 Error */
#define PIC32_IRQ_IC4       21  /* Input Capture 4 */
#define PIC32_IRQ_OC4       22  /* Output Compare 4 */
#define PIC32_IRQ_INT4      23  /* External Interrupt 4 */
#define PIC32_IRQ_T5        24  /* Timer5 */
#define PIC32_IRQ_IC5E      25  /* Input Capture 5 Error */
#define PIC32_IRQ_IC5       26  /* Input Capture 5 */
#define PIC32_IRQ_OC5       27  /* Output Compare 5 */
#define PIC32_IRQ_T6        28  /* Timer6 */
#define PIC32_IRQ_IC6E      29  /* Input Capture 6 Error */
#define PIC32_IRQ_IC6       30  /* Input Capture 6 */
#define PIC32_IRQ_OC6       31  /* Output Compare 6 */
#define PIC32_IRQ_T7        32  /* Timer7 */
#define PIC32_IRQ_IC7E      33  /* Input Capture 7 Error */
#define PIC32_IRQ_IC7       34  /* Input Capture 7 */
#define PIC32_IRQ_OC7       35  /* Output Compare 7 */
#define PIC32_IRQ_T8        36  /* Timer8 */
#define PIC32_IRQ_IC8E      37  /* Input Capture 8 Error */
#define PIC32_IRQ_IC8       38  /* Input Capture 8 */
#define PIC32_IRQ_OC8       39  /* Output Compare 8 */
#define PIC32_IRQ_T9        40  /* Timer9 */
#define PIC32_IRQ_IC9E      41  /* Input Capture 9 Error */
#define PIC32_IRQ_IC9       42  /* Input Capture 9 */
#define PIC32_IRQ_OC9       43  /* Output Compare 9 */
#define PIC32_IRQ_AD1       44  /* ADC1 Global Interrupt */
                         /* 45  -- Reserved */
#define PIC32_IRQ_AD1DC1    46  /* ADC1 Digital Comparator 1 */
#define PIC32_IRQ_AD1DC2    47  /* ADC1 Digital Comparator 2 */
#define PIC32_IRQ_AD1DC3    48  /* ADC1 Digital Comparator 3 */
#define PIC32_IRQ_AD1DC4    49  /* ADC1 Digital Comparator 4 */
#define PIC32_IRQ_AD1DC5    50  /* ADC1 Digital Comparator 5 */
#define PIC32_IRQ_AD1DC6    51  /* ADC1 Digital Comparator 6 */
#define PIC32_IRQ_AD1DF1    52  /* ADC1 Digital Filter 1 */
#define PIC32_IRQ_AD1DF2    53  /* ADC1 Digital Filter 2 */
#define PIC32_IRQ_AD1DF3    54  /* ADC1 Digital Filter 3 */
#define PIC32_IRQ_AD1DF4    55  /* ADC1 Digital Filter 4 */
#define PIC32_IRQ_AD1DF5    56  /* ADC1 Digital Filter 5 */
#define PIC32_IRQ_AD1DF6    57  /* ADC1 Digital Filter 6 */
                         /* 58  -- Reserved */
#define PIC32_IRQ_AD1D0     59  /* ADC1 Data 0 */
#define PIC32_IRQ_AD1D1     60  /* ADC1 Data 1 */
#define PIC32_IRQ_AD1D2     61  /* ADC1 Data 2 */
#define PIC32_IRQ_AD1D3     62  /* ADC1 Data 3 */
#define PIC32_IRQ_AD1D4     63  /* ADC1 Data 4 */
#define PIC32_IRQ_AD1D5     64  /* ADC1 Data 5 */
#define PIC32_IRQ_AD1D6     65  /* ADC1 Data 6 */
#define PIC32_IRQ_AD1D7     66  /* ADC1 Data 7 */
#define PIC32_IRQ_AD1D8     67  /* ADC1 Data 8 */
#define PIC32_IRQ_AD1D9     68  /* ADC1 Data 9 */
#define PIC32_IRQ_AD1D10    69  /* ADC1 Data 10 */
#define PIC32_IRQ_AD1D11    70  /* ADC1 Data 11 */
#define PIC32_IRQ_AD1D12    71  /* ADC1 Data 12 */
#define PIC32_IRQ_AD1D13    72  /* ADC1 Data 13 */
#define PIC32_IRQ_AD1D14    73  /* ADC1 Data 14 */
#define PIC32_IRQ_AD1D15    74  /* ADC1 Data 15 */
#define PIC32_IRQ_AD1D16    75  /* ADC1 Data 16 */
#define PIC32_IRQ_AD1D17    76  /* ADC1 Data 17 */
#define PIC32_IRQ_AD1D18    77  /* ADC1 Data 18 */
#define PIC32_IRQ_AD1D19    78  /* ADC1 Data 19 */
#define PIC32_IRQ_AD1D20    79  /* ADC1 Data 20 */
#define PIC32_IRQ_AD1D21    80  /* ADC1 Data 21 */
#define PIC32_IRQ_AD1D22    81  /* ADC1 Data 22 */
#define PIC32_IRQ_AD1D23    82  /* ADC1 Data 23 */
#define PIC32_IRQ_AD1D24    83  /* ADC1 Data 24 */
#define PIC32_IRQ_AD1D25    84  /* ADC1 Data 25 */
#define PIC32_IRQ_AD1D26    85  /* ADC1 Data 26 */
#define PIC32_IRQ_AD1D27    86  /* ADC1 Data 27 */
#define PIC32_IRQ_AD1D28    87  /* ADC1 Data 28 */
#define PIC32_IRQ_AD1D29    88  /* ADC1 Data 29 */
#define PIC32_IRQ_AD1D30    89  /* ADC1 Data 30 */
#define PIC32_IRQ_AD1D31    90  /* ADC1 Data 31 */
#define PIC32_IRQ_AD1D32    91  /* ADC1 Data 32 */
#define PIC32_IRQ_AD1D33    92  /* ADC1 Data 33 */
#define PIC32_IRQ_AD1D34    93  /* ADC1 Data 34 */
#define PIC32_IRQ_AD1D35    94  /* ADC1 Data 35 */
#define PIC32_IRQ_AD1D36    95  /* ADC1 Data 36 */
#define PIC32_IRQ_AD1D37    96  /* ADC1 Data 37 */
#define PIC32_IRQ_AD1D38    97  /* ADC1 Data 38 */
#define PIC32_IRQ_AD1D39    98  /* ADC1 Data 39 */
#define PIC32_IRQ_AD1D40    99  /* ADC1 Data 40 */
#define PIC32_IRQ_AD1D41    100 /* ADC1 Data 41 */
#define PIC32_IRQ_AD1D42    101 /* ADC1 Data 42 */
#define PIC32_IRQ_AD1D43    102 /* ADC1 Data 43 */
#define PIC32_IRQ_AD1D44    103 /* ADC1 Data 44 */
#define PIC32_IRQ_CPC       104 /* Core Performance Counter */
#define PIC32_IRQ_CFDC      105 /* Core Fast Debug Channel */
#define PIC32_IRQ_SB        106 /* System Bus Protection Violation */
#define PIC32_IRQ_CRPT      107 /* Crypto Engine Event */
                         /* 108 -- Reserved */
#define PIC32_IRQ_SPI1E     109 /* SPI1 Fault */
#define PIC32_IRQ_SPI1RX    110 /* SPI1 Receive Done */
#define PIC32_IRQ_SPI1TX    111 /* SPI1 Transfer Done */
#define PIC32_IRQ_U1E       112 /* UART1 Fault */
#define PIC32_IRQ_U1RX      113 /* UART1 Receive Done */
#define PIC32_IRQ_U1TX      114 /* UART1 Transfer Done */
#define PIC32_IRQ_I2C1B     115 /* I2C1 Bus Collision Event */
#define PIC32_IRQ_I2C1S     116 /* I2C1 Slave Event */
#define PIC32_IRQ_I2C1M     117 /* I2C1 Master Event */
#define PIC32_IRQ_CNA       118 /* PORTA Input Change Interrupt */
#define PIC32_IRQ_CNB       119 /* PORTB Input Change Interrupt */
#define PIC32_IRQ_CNC       120 /* PORTC Input Change Interrupt */
#define PIC32_IRQ_CND       121 /* PORTD Input Change Interrupt */
#define PIC32_IRQ_CNE       122 /* PORTE Input Change Interrupt */
#define PIC32_IRQ_CNF       123 /* PORTF Input Change Interrupt */
#define PIC32_IRQ_CNG       124 /* PORTG Input Change Interrupt */
#define PIC32_IRQ_CNH       125 /* PORTH Input Change Interrupt */
#define PIC32_IRQ_CNJ       126 /* PORTJ Input Change Interrupt */
#define PIC32_IRQ_CNK       127 /* PORTK Input Change Interrupt */
#define PIC32_IRQ_PMP       128 /* Parallel Master Port */
#define PIC32_IRQ_PMPE      129 /* Parallel Master Port Error */
#define PIC32_IRQ_CMP1      130 /* Comparator 1 Interrupt */
#define PIC32_IRQ_CMP2      131 /* Comparator 2 Interrupt */
#define PIC32_IRQ_USB       132 /* USB General Event */
#define PIC32_IRQ_USBDMA    133 /* USB DMA Event */
#define PIC32_IRQ_DMA0      134 /* DMA Channel 0 */
#define PIC32_IRQ_DMA1      135 /* DMA Channel 1 */
#define PIC32_IRQ_DMA2      136 /* DMA Channel 2 */
#define PIC32_IRQ_DMA3      137 /* DMA Channel 3 */
#define PIC32_IRQ_DMA4      138 /* DMA Channel 4 */
#define PIC32_IRQ_DMA5      139 /* DMA Channel 5 */
#define PIC32_IRQ_DMA6      140 /* DMA Channel 6 */
#define PIC32_IRQ_DMA7      141 /* DMA Channel 7 */
#define PIC32_IRQ_SPI2E     142 /* SPI2 Fault */
#define PIC32_IRQ_SPI2RX    143 /* SPI2 Receive Done */
#define PIC32_IRQ_SPI2TX    144 /* SPI2 Transfer Done */
#define PIC32_IRQ_U2E       145 /* UART2 Fault */
#define PIC32_IRQ_U2RX      146 /* UART2 Receive Done */
#define PIC32_IRQ_U2TX      147 /* UART2 Transfer Done */
#define PIC32_IRQ_I2C2B     148 /* I2C2 Bus Collision Event */
#define PIC32_IRQ_I2C2S     149 /* I2C2 Slave Event */
#define PIC32_IRQ_I2C2M     150 /* I2C2 Master Event */
#define PIC32_IRQ_CAN1      151 /* Control Area Network 1 */
#define PIC32_IRQ_CAN2      152 /* Control Area Network 2 */
#define PIC32_IRQ_ETH       153 /* Ethernet Interrupt */
#define PIC32_IRQ_SPI3E     154 /* SPI3 Fault */
#define PIC32_IRQ_SPI3RX    155 /* SPI3 Receive Done */
#define PIC32_IRQ_SPI3TX    156 /* SPI3 Transfer Done */
#define PIC32_IRQ_U3E       157 /* UART3 Fault */
#define PIC32_IRQ_U3RX      158 /* UART3 Receive Done */
#define PIC32_IRQ_U3TX      159 /* UART3 Transfer Done */
#define PIC32_IRQ_I2C3B     160 /* I2C3 Bus Collision Event */
#define PIC32_IRQ_I2C3S     161 /* I2C3 Slave Event */
#define PIC32_IRQ_I2C3M     162 /* I2C3 Master Event */
#define PIC32_IRQ_SPI4E     163 /* SPI4 Fault */
#define PIC32_IRQ_SPI4RX    164 /* SPI4 Receive Done */
#define PIC32_IRQ_SPI4TX    165 /* SPI4 Transfer Done */
#define PIC32_IRQ_RTCC      166 /* Real Time Clock */
#define PIC32_IRQ_FCE       167 /* Flash Control Event */
#define PIC32_IRQ_PRE       168 /* Prefetch Module SEC Event */
#define PIC32_IRQ_SQI1      169 /* SQI1 Event */
#define PIC32_IRQ_U4E       170 /* UART4 Fault */
#define PIC32_IRQ_U4RX      171 /* UART4 Receive Done */
#define PIC32_IRQ_U4TX      172 /* UART4 Transfer Done */
#define PIC32_IRQ_I2C4B     173 /* I2C4 Bus Collision Event */
#define PIC32_IRQ_I2C4S     174 /* I2C4 Slave Event */
#define PIC32_IRQ_I2C4M     175 /* I2C4 Master Event */
#define PIC32_IRQ_SPI5E     176 /* SPI5 Fault */
#define PIC32_IRQ_SPI5RX    177 /* SPI5 Receive Done */
#define PIC32_IRQ_SPI5TX    178 /* SPI5 Transfer Done */
#define PIC32_IRQ_U5E       179 /* UART5 Fault */
#define PIC32_IRQ_U5RX      180 /* UART5 Receive Done */
#define PIC32_IRQ_U5TX      181 /* UART5 Transfer Done */
#define PIC32_IRQ_I2C5B     182 /* I2C5 Bus Collision Event */
#define PIC32_IRQ_I2C5S     183 /* I2C5 Slave Event */
#define PIC32_IRQ_I2C5M     184 /* I2C5 Master Event */
#define PIC32_IRQ_SPI6E     185 /* SPI6 Fault */
#define PIC32_IRQ_SPI6RX    186 /* SPI6 Receive Done */
#define PIC32_IRQ_SPI6TX    187 /* SPI6 Transfer Done */
#define PIC32_IRQ_U6E       188 /* UART6 Fault */
#define PIC32_IRQ_U6RX      189 /* UART6 Receive Done */
#define PIC32_IRQ_U6TX      190 /* UART6 Transfer Done */
                         /* 191 -- Reserved */
#define PIC32_IRQ_LAST      190 /* Last valid irq number */


#define WDTCON           PIC32_R( 0x00800 )
#define WDTCONCLR        PIC32_R( 0x00804 )
#define WDTCONSET        PIC32_R( 0x00808 )
#define WDTCONINV        PIC32_R( 0x0080C )

#endif /* _IO_PIC32MZ_H */
