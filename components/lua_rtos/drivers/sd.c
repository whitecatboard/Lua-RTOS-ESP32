/*
 * SD flash card disk driver.
 *
 * Copyright (C) 2014 Serge Vakulenko, <serge@vak.ru>
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

#include "luartos.h"

#if CONFIG_LUA_RTOS_USE_FAT

#include <strings.h>
#include <stdio.h>

#include <sys/mutex.h>
#include <sys/disklabel.h>
#include <sys/syslog.h>

#include <drivers/gpio.h>
#include <drivers/spi.h>
#include <drivers/sd.h>

#define RAWPART         0               /* 'x' partition */


#if USE_LED_ACT
extern unsigned int activity;
#endif

/*
 * The structure of a disk drive.
 */
struct disk {
    /* the partition table */
    struct  diskpart part   [NPARTITIONS+1];
    int     spi_device;      /* interface to SPI port */
    int     unit;           /* physical unit number */
    int     open;           /* open/closed refcnt */
    int     wlabel;         /* label writable? */
    int     dkindex;        /* disk index for statistics */
    unsigned int   copenpart;      /* character units open on this drive */
    unsigned int   bopenpart;      /* block units open on this drive */
    unsigned int   openpart;       /* all units open on this drive */
};

static struct disk sddrives[NSD];       /* Table of units */
static int sd_type[NSD];                /* Card type */
static struct mtx sd_mtx;               /* SDCARD mutex */

#define TYPE_UNKNOWN    0
#define TYPE_I          1
#define TYPE_II         2
#define TYPE_SDHC       3

#define TIMO_WAIT_WDONE 400000
#define TIMO_WAIT_WIDLE 200000
#define TIMO_WAIT_CMD   100000
#define TIMO_WAIT_WDATA 30000
#define TIMO_READ       90000
#define TIMO_SEND_OP    8000
#define TIMO_CMD        7000
#define TIMO_SEND_CSD   6000
#define TIMO_WAIT_WSTOP 5000

unsigned int sd_timo_cmd;                /* Max timeouts, for sysctl */
unsigned int sd_timo_send_op;
unsigned int sd_timo_send_csd;
unsigned int sd_timo_read;
unsigned int sd_timo_wait_cmd;
unsigned int sd_timo_wait_wdata;
unsigned int sd_timo_wait_wdone;
unsigned int sd_timo_wait_wstop;
unsigned int sd_timo_wait_widle;

/*
 * Definitions for MMC/SDC commands.
 */
#define CMD_GO_IDLE             0       /* CMD0 */
#define CMD_SEND_OP_MMC         1       /* CMD1 (MMC) */
#define CMD_SEND_IF_COND        8
#define CMD_SEND_CSD            9
#define CMD_SEND_CID            10
#define CMD_STOP                12
#define CMD_SEND_STATUS         13      /* CMD13 */
#define CMD_SET_BLEN            16
#define CMD_READ_SINGLE         17
#define CMD_READ_MULTIPLE       18
#define CMD_SET_BCOUNT          23      /* (MMC) */
#define CMD_SET_WBECNT          23      /* ACMD23 (SDC) */
#define CMD_WRITE_SINGLE        24
#define CMD_WRITE_MULTIPLE      25
#define CMD_SEND_OP_SDC         41      /* ACMD41 (SDC) */
#define CMD_APP                 55      /* CMD55 */
#define CMD_READ_OCR            58

#define DATA_START_BLOCK        0xFE    /* start data for single block */
#define STOP_TRAN_TOKEN         0xFD    /* stop token for write multiple */
#define WRITE_MULTIPLE_TOKEN    0xFC    /* start data for write multiple */

/*
 * Wait while busy, up to 300 msec.
 */
static void sd_wait_ready(int spi, unsigned int limit, unsigned int *maxcount)
{
    unsigned int i;
    unsigned char reply;

    spi_ll_transfer(spi, 0xFF, NULL);
    for (i=0; i<limit; i++) {
    	spi_ll_transfer(spi, 0xFF, &reply);
        if (reply == 0xFF) {
            if (*maxcount < i)
                *maxcount = i;
            return;
        }
    }
    syslog(LOG_ERR, "sd: wait_ready(%d) failed", limit);
}

/*
 * Send a command and address to SD media.
 * Return response:
 *   FF - timeout
 *   00 - command accepted
 *   01 - command received, card in idle state
 *
 * Other codes:
 *   bit 0 = Idle state
 *   bit 1 = Erase Reset
 *   bit 2 = Illegal command
 *   bit 3 = Communication CRC error
 *   bit 4 = Erase sequence error
 *   bit 5 = Address error
 *   bit 6 = Parameter error
 *   bit 7 = Always 0
 */
static int card_cmd(unsigned int unit, unsigned int cmd, unsigned int addr)
{
    int spi = sddrives[unit].spi_device;
    unsigned int i;
    unsigned char reply;

    /* Wait for not busy, up to 300 msec. */
    if (cmd != CMD_GO_IDLE)
        sd_wait_ready(spi, TIMO_WAIT_CMD, &sd_timo_wait_cmd);

    /* Send a comand packet (6 bytes). */
    spi_ll_transfer(spi, cmd | 0x40, NULL);
    spi_ll_transfer(spi, addr >> 24, NULL);
    spi_ll_transfer(spi, addr >> 16, NULL);
    spi_ll_transfer(spi, addr >> 8, NULL);
    spi_ll_transfer(spi, addr, NULL);

    /* Send cmd checksum for CMD_GO_IDLE.
     * For all other commands, CRC is ignored. */
    if (cmd == CMD_GO_IDLE)
        spi_ll_transfer(spi, 0x95, NULL);
    else if (cmd == CMD_SEND_IF_COND)
        spi_ll_transfer(spi, 0x87, NULL);
    else
        spi_ll_transfer(spi, 0xFF, NULL);

    /* Wait for a response. */
    for (i=0; i<TIMO_CMD; i++)
    {
        spi_ll_transfer(spi, 0xFF, &reply);
        if (! (reply & 0x80))
        {
            if (sd_timo_cmd < i)
                sd_timo_cmd = i;
            return reply;
        }
    }
    if (cmd != CMD_GO_IDLE)
    {
        syslog(LOG_ERR, "sd%d card_cmd timeout, cmd=%02x, addr=%08x, reply=%02x",
            unit, cmd, addr, reply);
    }
    return reply;
}

/*
 * Control an LED to show SD activity
 */
static inline void
sd_led(int val) {
#if USE_LED_ACT
    gpio_pin_output(SD_LED);
    if (val) {
        activity = 1;
        gpio_pin_set(SD_LED);
    } else {
        gpio_pin_clr(SD_LED);
        activity = 0;
    }
#endif
}

/*
 * Add extra clocks after a deselect
 */
static inline void
sd_deselect(int spi)
{
    spi_ll_deselect(spi);
    spi_ll_transfer(spi, 0xFF, NULL);
    sd_led(0);
}

/*
 * Select the SPI port, and light the LED
 */
static inline void
sd_select(int spi)
{
    sd_led(1);
    spi_ll_select(spi);
}

/*
 * Initialize a card.
 * Return nonzero if successful.
 */
static int card_init(int unit)
{
    int spi = sddrives[unit].spi_device;
    unsigned int i;
    unsigned char reply;
    unsigned char response[4];
    int timeout = 4;

    /* Slow speed: 250 kHz */
    spi_ll_set_speed(spi, 250000);

    sd_type[unit] = TYPE_UNKNOWN;

    do {
        /* Unselect the card. */
        sd_deselect(spi);

        /* Send 80 clock cycles for start up. */
        for (i=0; i<10; i++)
            spi_ll_transfer(spi, 0xFF, NULL);

        /* Select the card and send a single GO_IDLE command. */
        sd_select(spi);
        timeout--;
        reply = card_cmd(unit, CMD_GO_IDLE, 0);

    } while ((reply != 0x01) && (timeout != 0));

    sd_deselect(spi);
    if (reply != 1)
    {
        /* It must return Idle. */
        return 0;
    }

    /* Check SD version. */
    sd_select(spi);
    reply = card_cmd(unit, CMD_SEND_IF_COND, 0x1AA);
    if (reply & 4)
    {
        /* Illegal command: card type 1. */
        sd_deselect(spi);
        sd_type[unit] = TYPE_I;
    } else {
        spi_ll_transfer(spi, 0xFF, &response[0]);
        spi_ll_transfer(spi, 0xFF, &response[1]);
        spi_ll_transfer(spi, 0xFF, &response[2]);
        spi_ll_transfer(spi, 0xFF, &response[3]);
        sd_deselect(spi);
        if (response[3] != 0xAA)
        {
            syslog(LOG_ERR, "sd%d cannot detect card type, response=%02x-%02x-%02x-%02x",
                unit, response[0], response[1], response[2], response[3]);
            return 0;
        }
        sd_type[unit] = TYPE_II;
    }


    /* Send repeatedly SEND_OP until Idle terminates. */
    for (i=0; ; i++)
    {
        sd_select(spi);
        card_cmd(unit,CMD_APP, 0);
        reply = card_cmd(unit,CMD_SEND_OP_SDC,
                         (sd_type[unit] == TYPE_II) ? 0x40000000 : 0);
        sd_select(spi);
        if (reply == 0)
            break;
        if (i >= TIMO_SEND_OP)
        {
            /* Init timed out. */
            syslog(LOG_ERR, "card_init SEND_OP timed out, reply = %d", reply);
            return 0;
        }
    }
    if (sd_timo_send_op < i)
        sd_timo_send_op = i;

    /* If SD2 read OCR register to check for SDHC card. */
    if (sd_type[unit] == TYPE_II)
    {
        sd_select(spi);
        reply = card_cmd(unit, CMD_READ_OCR, 0);
        if (reply != 0)
        {
            sd_deselect(spi);
            printf("sd%d: READ_OCR failed, reply=%02x", unit, reply);
            return 0;
        }
        spi_ll_transfer(spi, 0xFF, &response[0]);
        spi_ll_transfer(spi, 0xFF, &response[1]);
        spi_ll_transfer(spi, 0xFF, &response[2]);
        spi_ll_transfer(spi, 0xFF, &response[3]);
        sd_deselect(spi);
        if ((response[0] & 0xC0) == 0xC0)
        {
            sd_type[unit] = TYPE_SDHC;
        }
    }

    /* Fast speed. */
    spi_ll_set_speed(spi, CONFIG_LUA_RTOS_SD_HZ);
    return 1;
}

/*
 * Get number of sectors on the disk.
 * Return nonzero if successful.
 */
int card_size(int unit)
{
    int spi = sddrives[unit].spi_device;
    unsigned char csd [16];
    unsigned csize, n;
    unsigned char reply;
    unsigned int i;
    int nsectors;

    sd_select(spi);
    reply = card_cmd(unit,CMD_SEND_CSD, 0);
    if (reply != 0)
    {
        /* Command rejected. */
        sd_deselect(spi);
        return 0;
    }
    /* Wait for a response. */
    for (i=0; ; i++)
    {
        spi_ll_transfer(spi, 0xFF, &reply);
        if (reply == DATA_START_BLOCK)
            break;
        if (i >= TIMO_SEND_CSD)
        {
            /* Command timed out. */
            sd_deselect(spi);
            syslog(LOG_INFO, "sd%d card_size SEND_CSD timed out, reply = %d",
                unit, reply);
            return 0;
        }
    }
    if (sd_timo_send_csd < i)
        sd_timo_send_csd = i;

    /* Read data. */
    for (i=0; i<sizeof(csd); i++)
    {
        spi_ll_transfer(spi, 0xFF, &csd[i]);
    }
    /* Ignore CRC. */
    spi_ll_transfer(spi, 0xFF, NULL);
    spi_ll_transfer(spi, 0xFF, NULL);

    /* Disable the card. */
    sd_deselect(spi);

    /* CSD register has different structure
     * depending upon protocol version. */
    switch (csd[0] >> 6)
    {
        case 1:                 /* SDC ver 2.00 */
            csize = csd[9] + (csd[8] << 8) + 1;
            nsectors = csize << 10;
            break;
        case 0:                 /* SDC ver 1.XX or MMC. */
            n = (csd[5] & 15) + ((csd[10] & 128) >> 7) + ((csd[9] & 3) << 1) + 2;
            csize = (csd[8] >> 6) + (csd[7] << 2) + ((csd[6] & 3) << 10) + 1;
            nsectors = csize << (n - 9);
            break;
        default:                /* Unknown version. */
            return 0;
    }
    return nsectors;
}

/*
 * Read a block of data.
 * Return nonzero if successful.
 */
int
card_read(int unit, unsigned int offset, char *data, unsigned int bcount)
{
    int spi = sddrives[unit].spi_device;
    unsigned char reply;
    unsigned int i;

    mtx_lock(&sd_mtx);
    
    /* Send read-multiple command. */
    sd_select(spi);
    if (sd_type[unit] != TYPE_SDHC)
        offset <<= 9;

    reply = card_cmd(unit, CMD_READ_MULTIPLE, offset);
    if (reply != 0)
    {
        /* Command rejected. */
        syslog(LOG_ERR, "sd%d card_read bad READ_MULTIPLE reply = %d, offset = %08x",
            unit, reply, offset);
        sd_deselect(spi);
        mtx_unlock(&sd_mtx);
        return 0;
    }

again:
    /* Wait for a response. */
    for (i=0; ; i++)
    {
        spi_ll_transfer(spi, 0xFF, &reply);
        if (reply == DATA_START_BLOCK)
            break;
        if (i >= TIMO_READ)
        {
            /* Command timed out. */
            syslog(LOG_ERR, "sd%d card_read READ_MULTIPLE timed out, reply = %d",
                unit, reply);
            sd_deselect(spi);
            mtx_unlock(&sd_mtx);
            return 0;
        }
    }
    if (sd_timo_read < i)
        sd_timo_read = i;

    /* Read data. */
    if (bcount >= SECTSIZE)
    {
        spi_ll_bulk_read32_be(spi, SECTSIZE/4, (uint32_t *)data);
//printf("    %08x %08x %08x %08x ...\n",
//((int*)data)[0], ((int*)data)[1], ((int*)data)[2], ((int*)data)[3]);
        data += SECTSIZE;
    } else {
        spi_ll_bulk_read(spi, bcount, (uint8_t *)data);
//printf("    %08x %08x %08x %08x ...\n",
//((int*)data)[0], ((int*)data)[1], ((int*)data)[2], ((int*)data)[3]);
        data += bcount;
        for (i=bcount; i<SECTSIZE; i++)
            spi_ll_transfer(spi, 0xFF, NULL);
    }
    /* Ignore CRC. */
    spi_ll_transfer(spi, 0xFF, NULL);
    spi_ll_transfer(spi, 0xFF, NULL);

    if (bcount > SECTSIZE)
    {
        /* Next sector. */
        bcount -= SECTSIZE;
        goto again;
    }

    /* Stop a read-multiple sequence. */
    card_cmd(unit, CMD_STOP, 0);
    sd_deselect(spi);
//printf("%s: done\n", __func__);
    
    mtx_unlock(&sd_mtx);
    return 1;
}

/*
 * Write a block of data.
 * Return nonzero if successful.
 */
int
card_write(int unit, unsigned offset, char *data, unsigned bcount)
{
    int spi = sddrives[unit].spi_device;
    unsigned char reply;
	int i;

//printf("--- %s: unit = %d, blkno = %d, bcount = %d\n", __func__, unit, offset, bcount);

    mtx_lock(&sd_mtx);

    /* Send pre-erase count. */
    sd_select(spi);
    card_cmd(unit, CMD_APP, 0);
    reply = card_cmd(unit, CMD_SET_WBECNT, (bcount + SECTSIZE - 1) / SECTSIZE);
    if (reply != 0)
    {
        /* Command rejected. */
        sd_deselect(spi);
        syslog(LOG_ERR, "sd%d card_write: bad SET_WBECNT reply = %02x, count = %u",
            unit, reply, (bcount + SECTSIZE - 1) / SECTSIZE);
        
        mtx_unlock(&sd_mtx);
        return 0;
    }

    /* Send write-multiple command. */
    if (sd_type[unit] != TYPE_SDHC)
        offset <<= 9;
    reply = card_cmd(unit, CMD_WRITE_MULTIPLE, offset);
    if (reply != 0)
    {
        /* Command rejected. */
        sd_deselect(spi);
        syslog(LOG_ERR, "sd%d card_write: bad WRITE_MULTIPLE reply = %02x", unit, reply);
        mtx_unlock(&sd_mtx);
        return 0;
    }
    sd_deselect(spi);
again:
    /* Select, wait while busy. */
    sd_select(spi);
    sd_wait_ready(spi, TIMO_WAIT_WDATA, &sd_timo_wait_wdata);

    /* Send data. */
    spi_ll_transfer(spi, WRITE_MULTIPLE_TOKEN, NULL);
    if (bcount >= SECTSIZE)
    {
        spi_ll_bulk_write32_be(spi, SECTSIZE/4, (uint32_t *)data);
        data += SECTSIZE;
    } else {
        spi_ll_bulk_write(spi, bcount, (uint8_t *)data);
        data += bcount;
        for (i=bcount; i<SECTSIZE; i++)
            spi_ll_transfer(spi, 0xFF, NULL);
    }
    /* Send dummy CRC. */
    spi_ll_transfer(spi, 0xFF, NULL);
    spi_ll_transfer(spi, 0xFF, NULL);

    /* Check if data accepted. */
    spi_ll_transfer(spi, 0xFF, &reply);
    if ((reply & 0x1f) != 0x05)
    {
        /* Data rejected. */
        sd_deselect(spi);
        syslog(LOG_ERR, "sd%d card_write: data rejected, reply = %02x", unit,reply);
        mtx_unlock(&sd_mtx);
        return 0;
    }

    /* Wait for write completion. */
    sd_wait_ready(spi, TIMO_WAIT_WDONE, &sd_timo_wait_wdone);
    sd_deselect(spi);

    if (bcount > SECTSIZE)
    {
        /* Next sector. */
        bcount -= SECTSIZE;
        goto again;
    }

    /* Stop a write-multiple sequence. */
    sd_select(spi);
    sd_wait_ready(spi, TIMO_WAIT_WSTOP, &sd_timo_wait_wstop);
    spi_ll_transfer(spi, STOP_TRAN_TOKEN, NULL);
    sd_wait_ready(spi, TIMO_WAIT_WIDLE, &sd_timo_wait_widle);
    sd_deselect(spi);
    mtx_unlock(&sd_mtx);
    return 1;
}

/*
 * Setup the SD card interface.
 * Get the card type and size.
 * Read a partition table.
 * Return 0 on failure.
 */
static int
sd_setup(struct disk *u)
{
    int unit = u->unit;
	uint32_t speed;

    if (! card_init(unit)) {
        syslog(LOG_ERR, "sd%d no SD card detected", unit);
        return 0;
    }
    
    /* Get the size of raw partition. */
    bzero(u->part, sizeof(u->part));
    u->part[RAWPART].dp_offset = 0;
    u->part[RAWPART].dp_size = card_size(unit);
    if (u->part[RAWPART].dp_size == 0) {
        syslog(LOG_ERR, "sd%d cannot get card size", unit);
        return 0;
    }

    spi_get_speed(u->spi_device, &speed);

    syslog(LOG_INFO, "sd%d type %s, size %u kbytes, speed %u Mhz", unit,
        sd_type[unit]==TYPE_SDHC ? "SDHC" :
        sd_type[unit]==TYPE_II ? "II" : "I",
        u->part[RAWPART].dp_size / 2,
        speed / 1000000);

    /* Read partition table. */
    unsigned short buf[256];
   //??? int s = splbio();
    if (! card_read(unit, 0, (char*)buf, sizeof(buf))) {
        //??? splx(s);
        syslog(LOG_ERR, "sd%d cannot read partition table", unit);
        return 0;
    }
//???    splx(s);
    if (buf[255] == MBR_MAGIC) {
        bcopy(&buf[223], &u->part[1], 64);
#if 1
        int i;
        for (i=1; i<=NPARTITIONS; i++) {
            if (u->part[i].dp_type != 0)
                syslog(LOG_INFO, "sd%d%c partition type %02x, sector %u, size %u kbytes",
                    unit, i+'a'-1, u->part[i].dp_type,
                    u->part[i].dp_offset,
                    u->part[i].dp_size / 2);
        }
#endif
    }
    return 1;
}

/*
 * Initialize a drive.
 */
//int
//sdopen(int unit) {
//    struct disk *u;
//    int unit = sdunit(dev);
//    int part = sdpart(dev);
//    unsigned mask, i;
//
//    u = &sddrives[unit];
//    u->unit = unit;
//
//    /*
//     * Setup the SD card interface.
//     */
//    if (u->part[RAWPART].dp_size == 0) {
//        if (! sd_setup(u)) {
//            return ENODEV;
//        }
//    }
//    u->open++;
//
//    /*
//     * Warn if a partion is opened
//     * that overlaps another partition which is open
//     * unless one is the "raw" partition (whole disk).
//     */
//    mask = 1 << part;
//    if (part != RAWPART && ! (u->openpart & mask)) {
//        unsigned start = u->part[part].dp_offset;
//        unsigned end = start + u->part[part].dp_size;
//
//        /* Check for overlapped partitions. */
//        for (i=0; i<=NPARTITIONS; i++) {
//            struct diskpart *pp = &u->part[i];
//
//            if (i == part || i == RAWPART)
//                continue;
//
//            if (pp->dp_offset + pp->dp_size <= start ||
//                pp->dp_offset >= end)
//                continue;
//
//            if (u->openpart & (1 << i))
//                log(LOG_WARNING, "sd%d%c: overlaps open partition (sd%d%c)\n",
//                    unit, part + 'a' - 1,
//                    unit, pp - u->part + 'a' - 1);
//        }
//    }
//
//    u->openpart |= mask;
//    switch (mode) {
//    case S_IFCHR:
//        u->copenpart |= mask;
//        break;
//    case S_IFBLK:
//        u->bopenpart |= mask;
//        break;
//    }
//    return 0;
//}

/*
 * Read/write routine for a buffer.  Finds the proper unit, range checks
 * arguments, and schedules the transfer.  Does not wait for the transfer
 * to complete.  Multi-page transfers are supported.  All I/O requests must
 * be a multiple of a sector in length.
 */
//void
//sdstrategy(bp)
//    struct buf *bp;
//{
//    struct disk *u;    /* Disk unit to do the IO.  */
//    int unit = sdunit(bp->b_dev);
//    int s;
//    unsigned offset;
////printf("%s: unit = %d, blkno = %d, bcount = %d\n", __func__, unit, bp->b_blkno, bp->b_bcount);
//
//    if (unit >= NSD || bp->b_blkno < 0) {
//        log("sdstrategy: unit = %d, blkno = %d, bcount = %d\n",
//            unit, bp->b_blkno, bp->b_bcount);
//        bp->b_error = EINVAL;
//        goto bad;
//    }
//    u = &sddrives[unit];
//    offset = bp->b_blkno;
//    if (u->open) {
//        /*
//         * Determine the size of the transfer, and make sure it is
//         * within the boundaries of the partition.
//         */
//        struct diskpart *p = &u->part[sdpart(bp->b_dev)];
//        long maxsz = p->dp_size;
//        long sz = (bp->b_bcount + DEV_BSIZE - 1) >> DEV_BSHIFT;
//
//        offset += p->dp_offset;
////printf("%s: sdpart=%u, offset=%u, maxsz=%u, sz=%u\n", __func__, sdpart(bp->b_dev), offset, maxsz, sz);
//        if (offset == 0 &&
//            ! (bp->b_flags & B_READ) && ! u->wlabel) {
//                /* Write to partition table not allowed. */
//                bp->b_error = EROFS;
//                goto bad;
//        }
//        if (bp->b_blkno + sz > maxsz) {
//                /* if exactly at end of disk, return an EOF */
//                if (bp->b_blkno == maxsz) {
//                        bp->b_resid = bp->b_bcount;
//                        biodone(bp);
////printf("%s: done EOF\n", __func__);
//                        return;
//                }
//                /* or truncate if part of it fits */
//                sz = maxsz - bp->b_blkno;
//                if (sz <= 0) {
//                        bp->b_error = EINVAL;
//                        goto bad;
//                }
//                bp->b_bcount = sz << DEV_BSHIFT;
//        }
//    } else {
//        /* Reading the partition table. */
////printf("%s: reading the partition table\n", __func__);
//        offset = 0;
//    }
//    if (u->dkindex >= 0) {
//        /* Update disk statistics. */
//        dk_busy |= 1 << u->dkindex;
//        dk_xfer[u->dkindex]++;
//        dk_wds[u->dkindex] += bp->b_bcount >> 6;
//    }
//
//    s = splbio();
//    if (bp->b_flags & B_READ) {
//        card_read(unit, offset, bp->b_un.b_addr, bp->b_bcount);
//    } else {
//        card_write(unit, offset, bp->b_un.b_addr, bp->b_bcount);
//    }
//    biodone(bp);
//    splx(s);
////printf("%s: done OK\n", __func__);
//
//    if (u->dkindex >= 0)
//        dk_busy &= ~(1 << u->dkindex);
//    return;
//
//bad:
//    bp->b_flags |= B_ERROR;
//    biodone(bp);
////printf("%s: failed \n", __func__);
//}

//int
//sdsize(dev)
//    dev_t dev;
//{
//    int unit = sdunit(dev);
//    int part = sdpart(dev);
//    struct disk *u = &sddrives[unit];
//
//    if (unit >= NSD || part > NPARTITIONS)
//        return -1;
//
//    /*
//     * Setup the SD card interface, if not done yet.
//     */
//    if (u->part[RAWPART].dp_size == 0) {
//        if (! sd_setup(u)) {
//            return -1;
//        }
//    }
//    return u->part[part].dp_size;
//}

//int sdread(int unit, int spi) {
////    return physio(sdstrategy, 0, unit, B_READ, minphys, spi);
//}

//sdwrite(int unit, int spi)
//{
////    return physio(sdstrategy, 0, unit, B_WRITE, minphys, spi);
//}

/*
 * Test to see if device is present.
 * Return true if found and initialized ok.
 */
int sd_init(int unit) {
    struct disk *du = &sddrives[unit];
    int ok = 1;

    du->unit = unit;

    // Create mutex
    mtx_init(&sd_mtx, NULL, NULL, 0);

    driver_error_t *error;
    if ((error = spi_setup(CONFIG_LUA_RTOS_SD_SPI, 1, CONFIG_LUA_RTOS_SD_CS, 0, CONFIG_LUA_RTOS_SD_HZ, SPI_FLAG_WRITE | SPI_FLAG_READ, &du->spi_device))) {
    	free(error);
        syslog(LOG_ERR, "sd%u cannot open spi%u port (%s)", unit, du->spi_device, driver_get_err_msg(error));
        return 0;
    }

	syslog(LOG_INFO, "sd%u is at spi%d, pin cs=%s%d", unit,
		CONFIG_LUA_RTOS_SD_SPI,
        gpio_portname(CONFIG_LUA_RTOS_SD_CS), gpio_name(CONFIG_LUA_RTOS_SD_CS)
	);
    
    /* Assign disk index. */
    du->dkindex = 1;

    ok = sd_setup(du);
    if (!ok) {
        syslog(LOG_ERR, "sd%u can't init sdcard", unit);
    }
    
    return ok;
}

int sd_has_partitions(int unit) {
    struct disk *u = &sddrives[unit];
    int i;
    
    for (i=1; i<=NPARTITIONS; i++) {
        if (u->part[i].dp_type != 0) {
            return 1;
        }
    }
    
    return 0;
}

int sd_has_partition(int unit, int type) {
    struct disk *u = &sddrives[unit];
    int i;
    
    for (i=1; i<=NPARTITIONS; i++) {
        if (u->part[i].dp_type == type) {
            return 1;
        }
    }
    
    return 0;
}

#endif
