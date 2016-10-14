#ifndef CFI_H
#define	CFI_H

#define CFI_KHZ 50000L // Speed 50 Mhz

#define CMD_WRR   0x01 // Write Registers
#define CMD_PP    0x02 // Page Program
#define CMD_READ  0x03 // Read
#define CMD_WRDI  0x04 // Write Disable
#define CMD_RDSR1 0x05 // Read Status Register 1
#define CMD_WREN  0x06 // Write Enable
#define CMD_RDSR2 0x07 // Read Status Register 2        
#define CMD_BRRD  0x16 // Bank Register Read
#define CMD_BRWR  0x17 // Bank Register Write
#define CMD_P4E   0x20 // Sector erase (4 Kb)
#define CMF_ASPRD 0x2b // Read ASP register
#define CMD_CLSR  0x30 // Clear Status Register
#define CMD_RDCR  0x35 // Read Configuration Register
#define CMD_BLKE  0x60 // Bulk Erase
#define CMD_SE    0xd8 // Sector erase (64 Kb or 256 Kb)
#define CMD_REMS  0x90 // Read Identification
#define CMD_BRAC  0xb9 // Bank Register Access
#define CMD_PPBE  0xe4 // PPB Erase
#define CMD_RESET 0xf0 // Reset
#define CMD_RDID  0x9f // Read ID (JEDEC Manufacturer ID and JEDEC CFI)

#define SR1_SRWD_MASK   (1 << 7)
#define SR1_P_ERR_MASK  (1 << 6)
#define SR1_E_ERR_MASK  (1 << 5)
#define SR1_BP2_MASK    (1 << 4)
#define SR1_BP1_MASK    (1 << 3)
#define SR1_BP0_MASK    (1 << 2)
#define SR1_WEL_MASK    (1 << 1)
#define SR1_WIP_MASK    (1 << 0)

#define CR1_LC1_MASK    (1 << 7)
#define CR1_LC0_MASK    (1 << 6)
#define CR1_TBPROT_MASK (1 << 5)
#define CR1_RFU_MASK    (1 << 4)
#define CR1_BPNV_MASK   (1 << 3)
#define CR1_TBPARM_MASK (1 << 2)
#define CR1_QUAD_MASK   (1 << 1)
#define CR1_FREEZE_MASK (1 << 0)

#define SR2_D8h_O_MASK  (1 << 7)
#define SR2_02h_O_MASK  (1 << 6)

struct idcfi {
    char manid;
    char mint;
    char dens;
    char idcfilen;
    char sectarch;
    char famid;
    char mod[2];    
};

// Driver structure
struct cfi {
    struct idcfi idcfi;
    int pagesiz;
    int sectsiz;
    int sectors;
    int spi;
    int unit;
    int cs;
    int speed;
};

struct cfi *cfi_get(int unit);
void cfi_read(int unit, int addr, int size, char *buf);
int cfi_write(int unit, int addr, int size, char *buf);
void cfi_erase(int unit, int addr, int size);
int cfi_init(int unit);

#endif	/* CFI_H */

