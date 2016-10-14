/*
 * Defines of offsets into the system data structures
 * for use by assembler routines.
 */

/* Offsets for struct proc */
#define P_FORW 0
#define P_BACK 4
#define P_PRIORITY 208
#define P_UPTE 248

/* Offsets for struct user */
#define U_PCB_REGS 0
#define U_PCB_FPREGS 144
#define U_PCB_CONTEXT 276
#define U_PCB_ONFAULT 324
#define U_PCB_SEGTAB 328

/* Offsets for struct vmmeter */
#define V_SWTCH 0
