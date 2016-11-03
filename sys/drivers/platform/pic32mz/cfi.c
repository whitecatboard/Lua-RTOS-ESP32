/*
 * Lua RTOS, Common Flash Interface driver
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

#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include <machine/pic32mz.h>

#include <sys/drivers/spi.h>
#include <sys/drivers/cfi.h>
#include <sys/drivers/gpio.h>
#include <sys/syslog.h>
#include <sys/delay.h>

extern unsigned int activity;

struct cfi cfid[1];

static int inited = 0;

/*
 * Control an LED to show cfi activity
 */
static inline void cfi_led(int val) {
#ifdef SD_LED
    gpio_pin_output(CFI_LED);
    if (val) {
        activity = 1;
        gpio_pin_set(CFI_LED);
    } else {
        gpio_pin_clr(CFI_LED);
        activity = 0;
    }
#endif
}

static int cfi_size(struct cfi *cfi) {
    int mbits = 0;
    
    switch (cfi->idcfi.dens) {
        case 0x18:
            mbits = 128;
    }
    
    return (mbits / 8);
}

static char *cfi_model(struct cfi *cfi, char *buff) {
    int dens = 0;
    
    switch (cfi->idcfi.dens) {
        case 0x18:
            dens = 127;
    }    
    
    sprintf(buff, "S25FL%dS", dens);
    
    return buff;
}

static void cfi_send_command(
    struct cfi *cfi, char cmd, int *addr, char *buf, int len, int write) 
{
    int spi = cfi->spi;
    int i;

    cfi_led(1);
    spi_select(spi);
    spi_transfer(spi, cmd);
    
    if (addr) {
        spi_transfer(spi, *addr >> 16);
        spi_transfer(spi, *addr >> 8);
        spi_transfer(spi, *addr);  
    }

    if (buf) {
        for(i=0;i < len; i++) {
            if (write) {
                spi_transfer(spi, *buf++);
            } else {
                *buf++ = spi_transfer(spi, 0x00);
            }
        }                
    }
        
    spi_deselect(spi);
    cfi_led(0);
}

static char cfi_read_reg(struct cfi *cfi, char reg) {    
    char reply;
    
    cfi_send_command(cfi, reg, NULL, &reply, 1, 0);
        
    return reply;
}

static void cfi_wait_ready(struct cfi *cfi) {
    while (cfi_read_reg(cfi, CMD_RDSR1) & SR1_WIP_MASK);   
}


static int enable_write(struct cfi *cfi) {
    cfi_wait_ready(cfi);
    cfi_send_command(cfi, CMD_WREN, NULL, NULL, 0, 0);
    
    // Wait for write enable latch
    while (!(cfi_read_reg(cfi, CMD_RDSR1) & SR1_WEL_MASK));
    
    return 0;
}

static int cfi_setup(struct cfi *cfi) {
    char buff[15];
    char sr2;
        
    // Read status and configuration registers
    sr2 = cfi_read_reg(cfi, CMD_RDSR2);

    cfi->pagesiz = 256;
    if (sr2 & SR2_02h_O_MASK) {
        cfi->pagesiz = 512;
    }
    
    cfi->sectsiz = 64 * 1024;
    if (sr2 & SR2_D8h_O_MASK) {
        cfi->sectsiz = 256 * 1024;
    }

    // Read device information
    cfi_send_command(cfi, CMD_RDID, NULL, (char *)&cfi->idcfi, sizeof(struct idcfi), 0);

    cfi->sectors = (cfi_size(cfi) * 1024 * 1024) / cfi->sectsiz;

    if (cfi_size(cfi) <= 0) {
        syslog(LOG_ERR, "cfi%d can't detect spi flash", cfi->unit);
        return 0;
    }
   
    syslog(LOG_INFO, "cfi%d %s, %d Mbytes, %d bytes/page, %d Kb/sector, speed %d Mhz", 
          cfi->unit,
          cfi_model(cfi, buff),
          cfi_size(cfi),
          cfi->pagesiz,
          cfi->sectsiz / 1024,
          spi_get_speed(cfi->spi) / 1000
    );
    
    inited = 1;
    
    return 1;    
}

int cfi_init(int unit) {
    struct cfi *cfi = &cfid[unit];
    int spi;

    if (inited) return 1;
    
    switch (unit) {
        case 0:
            cfi->spi = CFI_SPI;
            cfi->unit = 0;
            cfi->cs = CFI_CS;
            cfi->speed = CFI_KHZ;
            break;
    }
    
    spi = cfi->spi;
    
    // Init spi port
    if (spi_init(spi) != 0) {
        syslog(LOG_ERR, "cfi%u cannot open spi%u port", unit, spi);
        return 0;
    }
    
    spi_set_cspin(spi, cfi->cs);
    spi_set_speed(spi, cfi->speed);
    spi_set(spi, PIC32_SPICON_CKE);

    syslog(LOG_INFO, 
           "cfi%u is at port %s, pin cs=%c%d",
           cfi->unit, spi_name(spi), spi_csname(spi), spi_cspin(spi)
    );

    // Setup driver
    return cfi_setup(cfi);
}

int cfi_write(int unit, int addr, int size, char *buf) {
    struct cfi *cfi = &cfid[unit];
    
    enable_write(cfi);
    cfi_send_command(cfi, CMD_PP, &addr, buf, size, 1);
    cfi_wait_ready(cfi);   
    
    return 0; 
}

void cfi_read(int unit, int addr, int size, char *buf) {
    struct cfi *cfi = &cfid[unit];

    cfi_wait_ready(cfi);
    cfi_send_command(cfi, CMD_READ, &addr, buf, size, 0);
    cfi_wait_ready(cfi);    
}

void cfi_erase(int unit, int addr, int size) {
    struct cfi *cfi = &cfid[unit];
    int i, block;    

    // Get the block number
    block = (addr - cfi->pagesiz) / cfi->sectsiz;
    
    if (block == 255) {
        for(i=0;i < 16;i++) {
            enable_write(cfi);
            cfi_send_command(cfi, CMD_P4E, &addr, NULL, 0, 1);  
            addr += cfi->sectsiz / 16;
        }
    } else {
        enable_write(cfi);
        cfi_send_command(cfi, CMD_SE, &addr, NULL, 0, 1);   
    }  
    
    cfi_wait_ready(cfi); 
}

void cfi_bulk_erase(int unit) {
    struct cfi *cfi = &cfid[unit];

    enable_write(cfi);
    cfi_send_command(cfi, CMD_BLKE, NULL, NULL, 0, 1);
}

struct cfi *cfi_get(int unit) {
    return &cfid[unit];
}

void cfi_test() {
    struct cfi *cfi;
    char test_buff[2] = {0x2c,0x04};
    char read_buff[2];
    int addr;
    int i;
    
    delay(5 * 1000);

    printf("cfi test ...\n");

    cfi_init(0);  
    
    cfi = &cfid[0];
    
    addr = 0xf80000;
    for(i=0;i<256;i++) {
        cfi_erase(0, addr, cfi->sectsiz);
        cfi_write(0, addr + 0x01fe, 2, test_buff);  
        cfi_write(0, addr + 0x01fc, 2, test_buff);  
        addr += cfi->sectsiz;
    }

    addr = 0xf80000;
    for(i=0;i<256;i++) {
        cfi_read(0, addr + 0x01fe, 2, read_buff);
        if ((read_buff[0] != 0x2c) || (read_buff[1] != 0x04)) {
            printf("error block %d\n",i);
        }
        cfi_read(0, addr + 0x01fc, 2, read_buff);
        if ((read_buff[0] != 0x2c) || (read_buff[1] != 0x04)) {
            printf("error block %d\n",i);
        }
        addr += cfi->sectsiz;
    }

    printf("finish");
}
