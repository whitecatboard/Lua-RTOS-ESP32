/*
 * SPI driver for pic32.
 *
 * Copyright (C) 2014 Serge Vakulenko, <serge@vak.ru>
 *
 * Lua RTOS, SPI driver
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

#include "FreeRTOS.h"
#include "whitecat.h"
#include <sys/drivers/spi.h>
#include <sys/drivers/gpio.h>

#include <sys/syslog.h>
#include <machine/pic32mz.h>
#include <machine/endian.h>

struct spireg {
    volatile unsigned con;		/* Control */
    volatile unsigned conclr;
    volatile unsigned conset;
    volatile unsigned coninv;
    volatile unsigned stat;		/* Status */
    volatile unsigned statclr;
    volatile unsigned statset;
    volatile unsigned statinv;
    volatile unsigned buf;		/* Transmit and receive buffer */
    volatile unsigned unused1;
    volatile unsigned unused2;
    volatile unsigned unused3;
    volatile unsigned brg;		/* Baud rate generator */
    volatile unsigned brgclr;
    volatile unsigned brgset;
    volatile unsigned brginv;
};

struct spi {
    struct spireg  *reg;
    int             cs;
    unsigned int    divisor;
    unsigned int    mode;
};

 struct spi spi[NSPI] = {
     {
         (struct spireg*)&SPI1CON,0,0,0
     },
     {
         (struct spireg*)&SPI2CON,0,0,0
     },
     {
         (struct spireg*)&SPI3CON,0,0,0
     },
     {
         (struct spireg*)&SPI4CON,0,0,0
     },
};

/*
 * Setup SPI connection in default mode.
 * Use further function calls to set baud rate, clock phase, etc.
 */
int spi_setup(int unit) {
    int channel = unit - 1;
    struct spi *dev = &spi[channel];

    // Set up the device
    if (channel < 0 || channel >= NSPI)
        return -1;

    dev->mode = PIC32_SPICON_MSTEN | PIC32_SPICON_ON | PIC32_SPICON_SMP;

    return 0;
}

void spi_set_cspin(int unit, int pin)
{
    int channel = unit - 1;
    struct spi *dev = &spi[channel];

    if (pin) {
        dev->cs = pin;
        
        gpio_pin_output(dev->cs);
        gpio_pin_set(dev->cs);
    }
}

/*
 * Set the SPI bit rate for a device (in kHz).
 */
void spi_set_speed(int unit, unsigned int sck)
{
    int channel = unit - 1;
    struct spi *dev = &spi[channel];
    int divisor;
    int speed;
    
    divisor = ((PBCLK2_HZ / 1000L) / (2 * sck)) - 1;

    if (divisor < 0) {
        divisor = 0;
    }

    if (divisor > 1023) {
        divisor = 1023;
    }

    speed = ((PBCLK2_HZ / 1000L) / (2 * (divisor + 1)));
    if (speed > sck) {
        if (divisor > 0)
            divisor++;
    }

    dev->divisor = divisor;
}

/*
 * Assert the CS pin of a device.
 * Not only do we set the CS pin, but before we do so we also reconfigure
 * the SPI bus to the required settings for this device.
 */
void spi_select(int unit)
{
    int channel = unit - 1;
    struct spi *dev = &spi[channel];

    dev->reg->brg = dev->divisor;
    dev->reg->con = dev->mode;

    if (dev->cs) {
        gpio_pin_clr(dev->cs);
    }
}

/*
 * Deassert the CS pin of a device.
 */
void spi_deselect(int unit)
{
    int channel = unit - 1;
    struct spi *dev = &spi[channel];

    if (dev->cs) {
        gpio_pin_set(dev->cs);
    }
}

/*
 * Set a mode setting or two - just updates the internal records,
 * the actual mode is changed next time the CS is asserted.
 */
void spi_set(int unit, unsigned int set)
{
    int channel = unit - 1;
    struct spi *dev = &spi[channel];

    dev->mode |= set;
}

void spi_clr_and_set(int unit, unsigned int set)
{
    int channel = unit - 1;
    struct spi *dev = &spi[channel];

    dev->mode = set;
}


/*
 * Clear a mode setting or two - just updates the internal records,
 * the actual mode is changed next time the CS is asserted.
 */
void spi_clr(int unit, unsigned int set)
{
    int channel = unit - 1;
    struct spi *dev = &spi[channel];

    dev->mode &= ~set;
}

/*
 * Return the current status of the SPI bus for the device in question
 * Just returns the ->stat entry in the register set.
 */
unsigned int spi_status(int unit)
{
    int channel = unit - 1;
    struct spi *dev = &spi[channel];

    if (! dev->reg)
        return 0;

    return dev->reg->stat;
}

/*
 * Transfer one word of data, and return the read word of data.
 * The actual number of bits sent depends on the mode of the transfer.
 * This is blocking, and waits for the transfer to complete
 * before returning.  Times out after a certain period.
 */
unsigned int spi_transfer(int unit, unsigned int data)
{
    unsigned int dr;
    
    int channel = unit - 1;

    struct spi *dev = &spi[channel];

    struct spireg *reg = dev->reg;

    unsigned int cnt = 100000;

    reg->buf = data;

    while ((
            !(reg->stat & PIC32_SPISTAT_SPIRBF) || 
            !(reg->stat & PIC32_SPISTAT_SPITBE)
           ) && (cnt > 0)) {
        cnt--;
    }
    
    if (cnt == 0) {
        return -1;
    }
    
    dr = reg->buf;
    
    return dr;
}

/*
 * Send a chunk of 8-bit data.
 */
void spi_bulk_write(int unit, unsigned int nbytes, unsigned char *data)
{
    unsigned i;


    int rup = mips_di();
    for (i=0; i<nbytes; i++) {
        spi_transfer(unit, *data++);
    }
    mtc0_Status(rup);
}

/*
 * Receive a chunk of 8-bit data.
 */
void spi_bulk_read(int unit, unsigned int nbytes, unsigned char *data)
{
    unsigned i;

    int rup = mips_di();
    for(i=0; i<nbytes; i++) {
        *data++ = spi_transfer(unit, 0xFF);
    }
    mtc0_Status(rup);
}

/*
 * Send and receive a chunk of 8-bit data.
 */
void spi_bulk_rw(int unit, unsigned int nbytes, unsigned char *data)
{
    unsigned int i;

    int rup = mips_di();
    for(i=0; i<nbytes; i++) {
        *data = spi_transfer(unit, *data);
        data++;
    }
    mtc0_Status(rup);
}

void spi_bulk_write16(int unit, unsigned int words, short *data)
{
    int channel = unit - 1;
    struct spi *dev = &spi[channel];

    struct spireg *reg = dev->reg;
    unsigned int nread = 0;
    unsigned int nwrite = words;

    int rup = mips_di();
    reg->conset = PIC32_SPICON_MODE16 | PIC32_SPICON_ENHBUF;
    while (nread < words) {
        if (nwrite > 0 && ! (reg->stat & PIC32_SPISTAT_SPITBF)) {
            reg->buf = *data++;
            nwrite--;
        }
        if (! (reg->stat & PIC32_SPISTAT_SPIRBE)) {
            (void) reg->buf;
            nread++;
        }
    }
    reg->con = dev->mode;
    mtc0_Status(rup);
}

/*
 * Write a chunk of 32-bit data as fast as possible.
 * Switches in to 32-bit mode regardless, and uses the enhanced buffer mode.
 * Data should be a multiple of 32 bits.
 */
void spi_bulk_write32(int unit, unsigned int words, int *data)
{
    int channel = unit - 1;
    struct spi *dev = &spi[channel];

    struct spireg *reg = dev->reg;
    unsigned int nread = 0;
    unsigned int nwrite = words;

    int rup = mips_di();
    reg->conset = PIC32_SPICON_MODE32 | PIC32_SPICON_ENHBUF;
    while (nread < words) {
        if (nwrite > 0 && ! (reg->stat & PIC32_SPISTAT_SPITBF)) {
            reg->buf = *data++;
            nwrite--;
        }
        if (! (reg->stat & PIC32_SPISTAT_SPIRBE)) {
            (void) reg->buf;
            nread++;
        }
    }
    reg->con = dev->mode;
    mtc0_Status(rup);
}

void spi_bulk_write32_be(int unit, unsigned int words, int *data)
{
    int channel = unit - 1;
    struct spi *dev = &spi[channel];

    struct spireg *reg = dev->reg;
    unsigned int nread = 0;
    unsigned int nwrite = words;

    int rup = mips_di();
    reg->conset = PIC32_SPICON_MODE32 | PIC32_SPICON_ENHBUF;
    while (nread < words) {
        if (nwrite > 0 && ! (reg->stat & PIC32_SPISTAT_SPITBF)) {
            reg->buf = __bswap32__(*data++);
            nwrite--;
        }
        if (! (reg->stat & PIC32_SPISTAT_SPIRBE)) {
            (void) reg->buf;
            nread++;
        }
    }
    reg->con = dev->mode;
    mtc0_Status(rup);
}

// Read a huge chunk of data as fast and as efficiently as
// possible.  Switches in to 32-bit mode regardless, and uses
// the enhanced buffer mode.
// Data should be a multiple of 32 bits.
void spi_bulk_read32_be(int unit, unsigned int words, int *data)
{
    int channel = unit - 1;
    struct spi *dev = &spi[channel];

    struct spireg *reg = dev->reg;
    unsigned int nread = 0;
    unsigned int nwrite = words;

    int rup = mips_di();
    reg->conset = PIC32_SPICON_MODE32 | PIC32_SPICON_ENHBUF;
    while (nread < words) {
        if (nwrite > 0 && ! (reg->stat & PIC32_SPISTAT_SPITBF)) {
            reg->buf = ~0;
            nwrite--;
        }
        if (! (reg->stat & PIC32_SPISTAT_SPIRBE)) {
            *data++ = __bswap32__(reg->buf);
            nread++;
        }
    }
    reg->con = dev->mode;
    mtc0_Status(rup);
}

void spi_bulk_read32(int unit, unsigned int words, int *data)
{
    int channel = unit - 1;
    struct spi *dev = &spi[channel];

    struct spireg *reg = dev->reg;
    unsigned int nread = 0;
    unsigned int nwrite = words;

    int rup = mips_di();
    reg->conset = PIC32_SPICON_MODE32 | PIC32_SPICON_ENHBUF;
    while (nread < words) {
        if (nwrite > 0 && ! (reg->stat & PIC32_SPISTAT_SPITBF)) {
            reg->buf = ~0;
            nwrite--;
        }
        if (! (reg->stat & PIC32_SPISTAT_SPIRBE)) {
            *data++ = reg->buf;
            nread++;
        }
    }
    reg->con = dev->mode;
    mtc0_Status(rup);
}

void spi_bulk_read16(int unit, unsigned int words, short *data)
{
    int channel = unit - 1;
    struct spi *dev = &spi[channel];

    struct spireg *reg = dev->reg;
    unsigned int nread = 0;
    unsigned int nwrite = words;

    int rup = mips_di();
    reg->conset = PIC32_SPICON_MODE16 | PIC32_SPICON_ENHBUF;
    while (nread < words) {
        if (nwrite > 0 && ! (reg->stat & PIC32_SPISTAT_SPITBF)) {
            reg->buf = ~0;
            nwrite--;
        }
        if (! (reg->stat & PIC32_SPISTAT_SPIRBE)) {
            *data++ = __bswap16__(reg->buf);
            nread++;
        }
    }
    reg->con = dev->mode;
    mtc0_Status(rup);
}

void spi_bulk_rw32_be(int unit, unsigned int words, int *writep)
{
    int channel = unit - 1;
    struct spi *dev = &spi[channel];

    struct spireg *reg = dev->reg;
    int *readp = writep;
    unsigned int nread = 0;
    unsigned int nwrite = words;

    int rup = mips_di();
    reg->conset = PIC32_SPICON_MODE32 | PIC32_SPICON_ENHBUF;
    while (nread < words) {
        if (nwrite > 0 && ! (reg->stat & PIC32_SPISTAT_SPITBF)) {
            reg->buf = *writep++;
            nwrite--;
        }
        if (! (reg->stat & PIC32_SPISTAT_SPIRBE)) {
            *readp++ = __bswap32__(reg->buf);
            nread++;
        }
    }
    reg->con = dev->mode;
    mtc0_Status(rup);
}

void spi_bulk_rw32(int unit, unsigned int words, int *writep)
{
    int channel = unit - 1;
    struct spi *dev = &spi[channel];
   
    struct spireg *reg = dev->reg;
    int *readp = writep;
    unsigned int nread = 0;
    unsigned int nwrite = words;

    int rup = mips_di();
    reg->conset = PIC32_SPICON_MODE32 | PIC32_SPICON_ENHBUF;
    while (nread < words) {
        if (nwrite > 0 && ! (reg->stat & PIC32_SPISTAT_SPITBF)) {
            reg->buf = *writep++;
            nwrite--;
        }
        if (! (reg->stat & PIC32_SPISTAT_SPIRBE)) {
            *readp++ = reg->buf;
            nread++;
        }
    }
    reg->con = dev->mode;
    mtc0_Status(rup);
}

void spi_bulk_rw16(int unit, unsigned int words, short *writep)
{
    int channel = unit - 1;
    struct spi *dev = &spi[channel];
    
    struct spireg *reg = dev->reg;
    short *readp = writep;
    unsigned int nread = 0;
    unsigned int nwrite = words;

    int rup = mips_di();
    reg->conset = PIC32_SPICON_MODE16 | PIC32_SPICON_ENHBUF;
    while (nread < words) {
        if (nwrite > 0 && ! (reg->stat & PIC32_SPISTAT_SPITBF)) {
            reg->buf = *writep++;
            nwrite--;
        }
        if (! (reg->stat & PIC32_SPISTAT_SPIRBE)) {
            *readp++ = __bswap16__(reg->buf);
            nread++;
        }
    }
    reg->con = dev->mode;
    mtc0_Status(rup);
}

/*
 * Return the name of the SPI bus for a device.
 */
const char *spi_name(int unit)
{
    static const char *name[6] = { "spi1", "spi2", "spi3", "spi4", "spi5", "spi6" };
    return name[unit - 1];
}

/*
 * Return the port name (A-K) of the chip select pin for a device.
 */
char spi_csname(int unit)
{
    int channel = unit - 1;
    struct spi *dev = &spi[channel];

    unsigned int n = ((dev->cs >> 4) & 15) - 1;

    if (n < 10)
        return "ABCDEFGHJK"[n];
        
    return '?';
}

/*
 * Return the pin index of the chip select pin for a device.
 */
int spi_cspin(int unit)
{
    int channel = unit - 1;
    struct spi *dev = &spi[channel];

    if (!dev->cs)
        return -1;

    return dev->cs & 15;
}

/*
 * Return the speed in kHz.
 */
unsigned int spi_get_speed(int unit)
{
    int channel = unit - 1;
    struct spi *dev = &spi[channel];

    if (! dev->reg)
        return 0;

    return ((PBCLK2_HZ / 1000L) / (2 * (dev->divisor + 1)));
}

/*
 * Assign SDIx signal to specified pin.
 */
static void assign_sdi(int channel, int pin)
{    
    gpio_disable_analog(pin);
    gpio_pin_input(pin);

    switch (channel) {
        case 0: SDI1R = gpio_input_map1(pin); break;
        case 1: SDI2R = gpio_input_map2(pin); break;
        case 2: SDI3R = gpio_input_map1(pin); break;
        case 3: SDI4R = gpio_input_map2(pin); break;
        case 4: SDI5R = gpio_input_map1(pin); break;
        case 5: SDI6R = gpio_input_map4(pin); break;
    }
}

static int output_map1 (unsigned channel)
{
    switch (channel) {
        case 0: return 5;   // 0101 = SDO1
        case 1: return 6;   // 0110 = SDO2
        case 2: return 7;   // 0111 = SDO3
        case 4: return 9;   // 1001 = SDO5
    }
    syslog(LOG_ERR, "spi%u cannot map SDO pin, group 1", channel);
    return 0;
}

static int output_map2 (unsigned channel)
{
    switch (channel) {
    case 0: return 5;   // 0101 = SDO1
    case 1: return 6;   // 0110 = SDO2
    case 2: return 7;   // 0111 = SDO3
    case 3: return 8;   // 1000 = SDO4
    case 4: return 9;   // 1001 = SDO5
    }
    syslog(LOG_ERR, "spi%u cannot map SDO pin, group 2", channel);
    return 0;
}

static int output_map3 (unsigned channel)
{
    switch (channel) {
    case 5: return 10;  // 1010 = SDO6
    }
    syslog(LOG_ERR, "spi%u cannot map SDO pin, group 3", channel);
    return 0;
}

static int output_map4 (unsigned channel)
{
    switch (channel) {
    case 3: return 8;   // 1000 = SDO4
    case 5: return 10;  // 1010 = SDO6
    }
    syslog(LOG_ERR, "spi%u cannot map SDO pin, group 4", channel);
    return 0;
}

/*
 * Assign SDOx signal to specified pin.
 */
static void assign_sdo(int channel, int pin)
{
    switch (pin) {
    case RP('A',14): RPA14R = output_map1(channel); return;
    case RP('A',15): RPA15R = output_map2(channel); return;
    case RP('B',0):  RPB0R  = output_map3(channel); return;
    case RP('B',10): RPB10R = output_map1(channel); return;
    case RP('B',14): RPB14R = output_map4(channel); return;
    case RP('B',15): RPB15R = output_map3(channel); return;
    case RP('B',1):  RPB1R  = output_map2(channel); return;
    case RP('B',2):  RPB2R  = output_map4(channel); return;
    case RP('B',3):  RPB3R  = output_map2(channel); return;
    case RP('B',5):  RPB5R  = output_map1(channel); return;
    case RP('B',6):  RPB6R  = output_map4(channel); return;
    case RP('B',7):  RPB7R  = output_map3(channel); return;
    case RP('B',8):  RPB8R  = output_map3(channel); return;
    case RP('B',9):  RPB9R  = output_map1(channel); return;
    case RP('C',13): RPC13R = output_map2(channel); return;
    case RP('C',14): RPC14R = output_map1(channel); return;
    case RP('C',1):  RPC1R  = output_map1(channel); return;
    case RP('C',2):  RPC2R  = output_map4(channel); return;
    case RP('C',3):  RPC3R  = output_map3(channel); return;
    case RP('C',4):  RPC4R  = output_map2(channel); return;
    case RP('D',0):  RPD0R  = output_map4(channel); return;
    case RP('D',10): RPD10R = output_map1(channel); return;
    case RP('D',11): RPD11R = output_map2(channel); return;
    case RP('D',12): RPD12R = output_map3(channel); return;
    case RP('D',14): RPD14R = output_map1(channel); return;
    case RP('D',15): RPD15R = output_map2(channel); return;
    case RP('D',1):  RPD1R  = output_map4(channel); return;
    case RP('D',2):  RPD2R  = output_map1(channel); return;
    case RP('D',3):  RPD3R  = output_map2(channel); return;
    case RP('D',4):  RPD4R  = output_map3(channel); return;
    case RP('D',5):  RPD5R  = output_map4(channel); return;
    case RP('D',6):  RPD6R  = output_map1(channel); return;
    case RP('D',7):  RPD7R  = output_map2(channel); return;
    case RP('D',9):  RPD9R  = output_map3(channel); return;
    case RP('E',3):  RPE3R  = output_map3(channel); return;
    case RP('E',5):  RPE5R  = output_map2(channel); return;
    case RP('E',8):  RPE8R  = output_map4(channel); return;
    case RP('E',9):  RPE9R  = output_map3(channel); return;
    case RP('F',0):  RPF0R  = output_map2(channel); return;
    case RP('F',12): RPF12R = output_map3(channel); return;
    case RP('F',13): RPF13R = output_map4(channel); return;
    case RP('F',1):  RPF1R  = output_map1(channel); return;
    case RP('F',2):  RPF2R  = output_map4(channel); return;
    case RP('F',3):  RPF3R  = output_map4(channel); return;
    case RP('F',4):  RPF4R  = output_map1(channel); return;
    case RP('F',5):  RPF5R  = output_map2(channel); return;
    case RP('F',8):  RPF8R  = output_map3(channel); return;
    case RP('G',0):  RPG0R  = output_map2(channel); return;
    case RP('G',1):  RPG1R  = output_map1(channel); return;
    case RP('G',6):  RPG6R  = output_map3(channel); return;
    case RP('G',7):  RPG7R  = output_map2(channel); return;
    case RP('G',8):  RPG8R  = output_map1(channel); return;
    case RP('G',9):  RPG9R  = output_map4(channel); return;
    }
    syslog(LOG_ERR, "spi%u cannot map SDO pin %c%d",
        channel, gpio_portname(pin), gpio_pinno(pin));
    
    gpio_pin_output(pin);
}

void spi_pins(int unit, unsigned char *sdi, unsigned char *sdo, unsigned char*sck) {
    int channel = unit - 1;

    switch (channel) {
        case 0:
            *sdi = SPI1_PINS >> 8 & 0xFF;
            *sdo = SPI1_PINS & 0xFF;
            break;
        case 1:
            *sdi = SPI2_PINS >> 8 & 0xFF;
            *sdo = SPI2_PINS & 0xFF;
            break;
        case 2:
            *sdi = SPI3_PINS >> 8 & 0xFF;
            *sdo = SPI3_PINS & 0xFF;
            break;
        case 3:
            *sdi = SPI4_PINS >> 8 & 0xFF;
            *sdo = SPI4_PINS & 0xFF;
            break;
    }

    static const int sck_tab[6] = {
        RP('D',1),  /* SCK1 */
        RP('G',6),  /* SCK2 */
        RP('B',14), /* SCK3 */
        RP('D',10), /* SCK4 */
        RP('F',13), /* SCK5 */
        RP('D',15), /* SCK6 */
    };   
    
    *sck = sck_tab[channel];
}

/*
 * Test to see if device is present.
 * Return 0 if found and initialized ok.
 * SPI ports are always present, if configured.
 */
int spi_init(int unit) {
    int channel = unit - 1;
    unsigned char sdi, sdo, sck;

    if (channel < 0 || channel >= NSPI)
        return 1;
    
    // Enable module
    switch (unit) {
        case 1: PMD5CLR = SPI1MD; break;
        case 2: PMD5CLR = SPI2MD; break;
        case 3: PMD5CLR = SPI3MD; break;
        case 4: PMD5CLR = SPI4MD; break;
        case 5: PMD5CLR = SPI5MD; break;
        case 6: PMD5CLR = SPI6MD; break;
    }

    spi_pins(unit, &sdi, &sdo, &sck);
    
    syslog(LOG_INFO,
        "spi%u at pins sdi=%c%d/sdo=%c%d/sck=%c%d", unit,
        gpio_portname(sdi), gpio_pinno(sdi),
        gpio_portname(sdo), gpio_pinno(sdo),
        gpio_portname(sck), gpio_pinno(sck)
    );

    /* Assign SDI and SDO pins. */
    assign_sdi (channel, sdi);
    assign_sdo (channel, sdo);

    spi_setup(unit);
    
    return 0;
}
