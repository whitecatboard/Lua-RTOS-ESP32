/*
 * SD flash card disk driver.
 *
 * Copyright (C) 2014 Serge Vakulenko, <serge@vak.ru>
 *
 * -------------------------------------------------------------
 * Copyright (C) 2015 - 2017
 * IBEROXARXA SERVICIOS INTEGRALES, S.L.
 * Lua RTOS integration
 *
 * Author: Jaume Olivé (jolive@iberoxarxa.com / jolive@whitecatboard.org)
 * -------------------------------------------------------------
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

#include "sdkconfig.h"

#if CONFIG_LUA_RTOS_USE_SPI_SD

#include <strings.h>
#include <stdio.h>

#include <sys/driver.h>
#include <sys/mutex.h>
#include <sys/disklabel.h>
#include <sys/syslog.h>

#include <drivers/gpio.h>
#include <drivers/spi.h>
#include "spi_sd.h"

#define RAWPART         0           /* whole disk */

#define MBR_MAGIC       0xaa55

#ifndef SD_KHZ
#define SD_KHZ          12500       /* speed 12.5 MHz */
#endif
#ifndef SD_FAST_KHZ
#define SD_FAST_KHZ     25000       /* up to 25 Mhz is allowed by the spec */
#endif
#ifndef SD_FASTEST_KHZ
#define SD_FASTEST_KHZ  25000       /* max speed for pic32mz SPI is 50 MHz */
#endif

#if CONFIG_LUA_RTOS_USE_LED_ACT
extern unsigned int activity;
#endif

/*
 * The structure of a disk drive.
 */
struct disk {
	/*
	 * Partition table.
	 */
	struct diskpart part[NPARTITIONS + 1];

	/*
	 * Card type.
	 */
	int card_type;
#define TYPE_UNKNOWN    0
#define TYPE_SD_LEGACY  1
#define TYPE_SD_II      2
#define TYPE_SDHC       3

	int spiio; /* interface to SPI port */
	int unit; /* physical unit number */
	u_char ocr[4]; /* operation condition register */
	u_char csd[16]; /* card-specific data */
#define TRANS_SPEED_25MHZ   0x32
#define TRANS_SPEED_50MHZ   0x5a
#define TRANS_SPEED_100MHZ  0x0b
#define TRANS_SPEED_200MHZ  0x2b

	u_short group[6]; /* function group bitmasks */
	int ma; /* power consumption */
};

static struct disk sddrives[NSD]; /* Table of units */

#define TIMO_WAIT_WDONE 400000
#define TIMO_WAIT_WIDLE 399000
#define TIMO_WAIT_CMD   100000
#define TIMO_WAIT_WDATA 30000
#define TIMO_READ       90000
#define TIMO_SEND_OP    8000
#define TIMO_CMD        7000
#define TIMO_SEND_CSD   6000
#define TIMO_WAIT_WSTOP 5000

int sd_timo_cmd; /* Max timeouts, for sysctl */
int sd_timo_send_op;
int sd_timo_send_csd;
int sd_timo_read;
int sd_timo_wait_cmd;
int sd_timo_wait_wdata;
int sd_timo_wait_wdone;
int sd_timo_wait_wstop;
int sd_timo_wait_widle;

/*
 * Definitions for MMC/SDC commands.
 */
#define CMD_GO_IDLE             0       /* CMD0 */
#define CMD_SEND_OP_MMC         1       /* CMD1 (MMC) */
#define CMD_SWITCH_FUNC         6
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

DRIVER_REGISTER_ERROR(SPI_SD, spi_sd, CannotSetup, "can't setup", SPI_SD_ERR_CANT_INIT);

/*
 * Wait while busy, up to 300 msec.
 */
static void sd_wait_ready(int io, int limit, int *maxcount) {
	int i;
	unsigned char reply;

	for (i = 0; i < limit; i++) {
		spi_ll_transfer(io, 0xFF, &reply);
		if (reply == 0xFF) {
			if (*maxcount < i)
				*maxcount = i;
			return;
		}
	}
	syslog(LOG_INFO, "sd: wait_ready(%d) failed\n", limit);
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
static int card_cmd(unsigned int unit, unsigned int cmd, unsigned int addr) {
	int io = sddrives[unit].spiio;
	int i;
	uint8_t reply;

	/* Wait for not busy, up to 300 msec. */
	if (cmd != CMD_GO_IDLE)
		sd_wait_ready(io, TIMO_WAIT_CMD, &sd_timo_wait_cmd);

	/* Send a comand packet (6 bytes). */
	spi_ll_transfer(io, cmd | 0x40, NULL);
	spi_ll_transfer(io, addr >> 24, NULL);
	spi_ll_transfer(io, addr >> 16, NULL);
	spi_ll_transfer(io, addr >> 8, NULL);
	spi_ll_transfer(io, addr, NULL);

	/* Send cmd checksum for CMD_GO_IDLE.
	 * For all other commands, CRC is ignored. */
	if (cmd == CMD_GO_IDLE)
		spi_ll_transfer(io, 0x95, NULL);
	else if (cmd == CMD_SEND_IF_COND)
		spi_ll_transfer(io, 0x87, NULL);
	else
		spi_ll_transfer(io, 0xFF, NULL);

	/* Wait for a response. */
	for (i = 0; i < TIMO_CMD; i++) {
		spi_ll_transfer(io, 0xFF, &reply);
		if (!(reply & 0x80)) {
			if (sd_timo_cmd < i)
				sd_timo_cmd = i;
			return reply;
		}
	}
	if (cmd != CMD_GO_IDLE) {
		syslog(LOG_INFO,
				"sd%d card_cmd timeout, cmd=%02x, addr=%08x, reply=%02x\n",
				unit, cmd, addr, reply);
	}
	return reply;
}

/*
 * Control an LED to show SD activity
 */
static inline void sd_led(int val) {
#if CONFIG_LUA_RTOS_USE_LED_ACT
    gpio_pin_output(CONFIG_LUA_RTOS_LED_ACT);
    if (val) {
        activity = 1;
        gpio_pin_set(CONFIG_LUA_RTOS_LED_ACT);
    } else {
        gpio_pin_clr(CONFIG_LUA_RTOS_LED_ACT);
        activity = 0;
    }
#endif
}

/*
 * Add extra clocks after a deselect
 */
static inline void sd_deselect(int io) {
	spi_ll_deselect(io);
	spi_ll_transfer(io, 0xFF, NULL);
	sd_led(0);
}

/*
 * Select the SPI port, and light the LED
 */
static inline void sd_select(int io) {
	sd_led(1);
	spi_ll_select(io);
}

/*
 * Initialize a card.
 * Return nonzero if successful.
 */
int card_init(int unit) {
	struct disk *u = &sddrives[unit];
	int io = u->spiio;
	int i, reply;
	int timeout = 4;

	/* Slow speed: 250 kHz */
	spi_ll_set_speed(io, 250000);

	u->card_type = TYPE_UNKNOWN;

	do {
		/* Unselect the card. */
		sd_deselect(io);

		/* Send 80 clock cycles for start up. */
		for (i = 0; i < 10; i++)
			spi_ll_transfer(io, 0xFF, NULL);

		/* Select the card and send a single GO_IDLE command. */
		sd_select(io);
		timeout--;
		reply = card_cmd(unit, CMD_GO_IDLE, 0);

	} while ((reply != 0x01) && (timeout != 0));

	sd_deselect(io);
	if (reply != 1) {
		/* It must return Idle. */
		return 0;
	}

	/* Check SD version. */
	sd_select(io);
	reply = card_cmd(unit, CMD_SEND_IF_COND, 0x1AA);
	if (reply & 4) {
		/* Illegal command: card type 1. */
		sd_deselect(io);
		u->card_type = TYPE_SD_LEGACY;
	} else {
		unsigned char response[4];
		spi_ll_transfer(io, 0xFF, &response[0]);
		spi_ll_transfer(io, 0xFF, &response[1]);
		spi_ll_transfer(io, 0xFF, &response[2]);
		spi_ll_transfer(io, 0xFF, &response[3]);
		sd_deselect(io);
		if (response[3] != 0xAA) {
			syslog(LOG_INFO,
					"sd%d cannot detect card type, response=%02x-%02x-%02x-%02x\n",
					unit, response[0], response[1], response[2], response[3]);
			return 0;
		}
		u->card_type = TYPE_SD_II;
	}

	/* Send repeatedly SEND_OP until Idle terminates. */
	for (i = 0;; i++) {
		sd_select(io);
		card_cmd(unit, CMD_APP, 0);
		reply = card_cmd(unit, CMD_SEND_OP_SDC,
				(u->card_type == TYPE_SD_II) ? 0x40000000 : 0);
		sd_select(io);
		if (reply == 0)
			break;
		if (i >= TIMO_SEND_OP) {
			/* Init timed out. */
			syslog(LOG_INFO, "card_init: SEND_OP timed out, reply = %d\n",
					reply);
			return 0;
		}
	}
	if (sd_timo_send_op < i)
		sd_timo_send_op = i;

	/* If SD2 read OCR register to check for SDHC card. */
	if (u->card_type == TYPE_SD_II) {
		sd_select(io);
		reply = card_cmd(unit, CMD_READ_OCR, 0);
		if (reply != 0) {
			sd_deselect(io);
			syslog(LOG_INFO, "sd%d READ_OCR failed, reply=%02x\n", unit,
					reply);
			return 0;
		}
		spi_ll_transfer(io, 0xFF, &u->ocr[0]);
		spi_ll_transfer(io, 0xFF, &u->ocr[1]);
		spi_ll_transfer(io, 0xFF, &u->ocr[2]);
		spi_ll_transfer(io, 0xFF, &u->ocr[3]);
		sd_deselect(io);
		if ((u->ocr[0] & 0xC0) == 0xC0) {
			u->card_type = TYPE_SDHC;
		}
	}

	/* Fast speed. */
	spi_ll_set_speed(io, SD_KHZ * 1000);
	return 1;
}

/*
 * Get the value of CSD register.
 */
static int sd_read_csd(int unit) {
	struct disk *u = &sddrives[unit];
	int io = u->spiio;
	uint8_t reply;
	int i;

	sd_select(io);
	reply = card_cmd(unit, CMD_SEND_CSD, 0);
	if (reply != 0) {
		/* Command rejected. */
		sd_deselect(io);
		return 0;
	}
	/* Wait for a response. */
	for (i = 0;; i++) {
		spi_ll_transfer(io, 0xFF, &reply);
		if (reply == DATA_START_BLOCK)
			break;
		if (i >= TIMO_SEND_CSD) {
			/* Command timed out. */
			sd_deselect(io);
			syslog(LOG_INFO,
					"sd%d sd_size: SEND_CSD timed out, reply = %d\n", unit,
					reply);
			return 0;
		}
	}
	if (sd_timo_send_csd < i)
		sd_timo_send_csd = i;

	/* Read data. */
	for (i = 0; i < 16; i++) {
		spi_ll_transfer(io, 0xFF, &u->csd[i]);
	}
	/* Ignore CRC. */
	spi_ll_transfer(io, 0xFF, NULL);
	spi_ll_transfer(io, 0xFF, NULL);

	/* Disable the card. */
	sd_deselect(io);

	return 1;
}

/*
 * Get number of sectors on the disk.
 * Return nonzero if successful.
 */
int sd_size(int unit) {
	struct disk *u = &sddrives[unit];
	unsigned csize, n;
	int nsectors;

	if (!sd_read_csd(unit))
		return 0;

	/* CSD register has different structure
	 * depending upon protocol version. */
	switch (u->csd[0] >> 6) {
	case 1: /* SDC ver 2.00 */
		csize = u->csd[9] + (u->csd[8] << 8) + 1;
		nsectors = csize << 10;
		break;
	case 0: /* SDC ver 1.XX or MMC. */
		n = (u->csd[5] & 15) + ((u->csd[10] & 128) >> 7)
				+ ((u->csd[9] & 3) << 1) + 2;
		csize = (u->csd[8] >> 6) + (u->csd[7] << 2) + ((u->csd[6] & 3) << 10)
				+ 1;
		nsectors = csize << (n - 9);
		break;
	default: /* Unknown version. */
		return 0;
	}
	return nsectors;
}

/*
 * Use CMD6 to enable high-speed mode.
 */
static void card_high_speed(int unit) {
	uint8_t reply;
	int i;
	struct disk *u = &sddrives[unit];
	int io = u->spiio;
	unsigned char status[64];

	/* Here we set HighSpeed 50MHz.
	 * We do not tackle the power and io driver strength yet. */
	spi_ll_select(io);
	reply = card_cmd(unit, CMD_SWITCH_FUNC, 0x80000001);
	if (reply != 0) {
		/* Command rejected. */
		sd_deselect(io);
		return;
	}

	/* Wait for a response. */
	for (i = 0;; i++) {
		spi_ll_transfer(io, 0xFF, &reply);
		if (reply == DATA_START_BLOCK)
			break;
		if (i >= 5000) {
			/* Command timed out. */
			sd_deselect(io);
			syslog(LOG_INFO,
					"sd%d sd_size: SWITCH_FUNC timed out, reply = %d\n",
					unit, reply);
			return;
		}
	}

	/* Read 64-byte status. */
	for (i = 0; i < 64; i++)
		spi_ll_transfer(io, 0xFF, &status[i]);

	/* Do at least 8 _slow_ clocks to switch into the HS mode. */
	spi_ll_transfer(io, 0xFF, NULL);
	spi_ll_transfer(io, 0xFF, NULL);
	sd_deselect(io);

	if ((status[16] & 0xF) == 1) {
		/* The card has switched to high-speed mode. */
		int khz;

		sd_read_csd(unit);
		switch (u->csd[3]) {
		default:
			syslog(LOG_INFO, "sd%d Unknown speed csd[3] = %02x\n", unit,
					u->csd[3]);
			break;
		case TRANS_SPEED_25MHZ:
			/* 25 MHz - default clock for high speed mode. */
			syslog(LOG_INFO, "sd%d fast clock 25MHz\n", unit);
			khz = SD_FAST_KHZ;
			break;
		case TRANS_SPEED_50MHZ:
			/* 50 MHz - typical clock for SDHC cards. */
			khz = SD_FASTEST_KHZ;
			break;
		case TRANS_SPEED_100MHZ:
			khz = SD_FASTEST_KHZ;
			break;
		case TRANS_SPEED_200MHZ:
			khz = SD_FASTEST_KHZ;
			break;
		}
		spi_set_speed(io, khz * 1000);
	}

	/* Save function group information for later use. */
	u->ma = status[0] << 8 | status[1];
	u->group[0] = status[12] << 8 | status[13];
	u->group[1] = status[10] << 8 | status[11];
	u->group[2] = status[8] << 8 | status[9];
	u->group[3] = status[6] << 8 | status[7];
	u->group[4] = status[4] << 8 | status[5];
	u->group[5] = status[2] << 8 | status[3];

	if (u->ma > 0) {
		syslog(LOG_INFO, "sd%d function groups %x/%x/%x/%x/%x/%x, max current %u mA\n", unit,
				u->group[5] & 0x7fff, u->group[4] & 0x7fff, u->group[3] & 0x7fff,
				u->group[2] & 0x7fff, u->group[1] & 0x7fff, u->group[0] & 0x7fff,
				u->ma);
	} else {
		syslog(LOG_INFO, "sd%d function groups %x/%x/%x/%x/%x/%x\n", unit,
				u->group[5] & 0x7fff, u->group[4] & 0x7fff, u->group[3] & 0x7fff,
				u->group[2] & 0x7fff, u->group[1] & 0x7fff, u->group[0] & 0x7fff);
	}
}

/*
 * Read a block of data.
 * Return nonzero if successful.
 */
int sd_read(int unit, unsigned int offset, char *data, unsigned int bcount) {
	struct disk *u = &sddrives[unit];
	int io = u->spiio;
	uint8_t reply;
	int i;

	/* Send read-multiple command. */
	sd_select(io);
	if (u->card_type != TYPE_SDHC)
		offset <<= 9;

	reply = card_cmd(unit, CMD_READ_MULTIPLE, offset);
	if (reply != 0) {
		/* Command rejected. */
		syslog(LOG_INFO,
				"sd%d sd_read: bad READ_MULTIPLE reply = %d, offset = %08x\n",
				unit, reply, offset);
		sd_deselect(io);
		return 0;
	}

	again:
	/* Wait for a response. */
	for (i = 0;; i++) {
		spi_ll_transfer(io, 0xFF, &reply);
		if (reply == DATA_START_BLOCK)
			break;
		if (i >= TIMO_READ) {
			/* Command timed out. */
			syslog(LOG_INFO,
					"sd%d sd_read: READ_MULTIPLE timed out, reply = %d\n",
					unit, reply);
			sd_deselect(io);
			return 0;
		}
	}
	if (sd_timo_read < i)
		sd_timo_read = i;

	/* Read data. */
	if (bcount >= SECTSIZE) {
		spi_ll_bulk_read32(io, SECTSIZE / 4, (uint32_t*) data);
		data += SECTSIZE;
	} else {
		spi_ll_bulk_read(io, bcount, (unsigned char *) data);
		data += bcount;
		for (i = bcount; i < SECTSIZE; i++)
			spi_ll_transfer(io, 0xFF, NULL);
	}
	/* Ignore CRC. */
	spi_ll_transfer(io, 0xFF, NULL);
	spi_ll_transfer(io, 0xFF, NULL);

	if (bcount > SECTSIZE) {
		/* Next sector. */
		bcount -= SECTSIZE;
		goto again;
	}

	/* Stop a read-multiple sequence. */
	card_cmd(unit, CMD_STOP, 0);
	sd_deselect(io);

	return 1;
}

/*
 * Write a block of data.
 * Return nonzero if successful.
 */
int sd_write(int unit, unsigned offset, char *data, unsigned bcount) {
	struct disk *u = &sddrives[unit];
	int io = sddrives[unit].spiio;
	uint8_t reply;
	int i;

	/* Send pre-erase count. */
	sd_select(io);
	card_cmd(unit, CMD_APP, 0);
	reply = card_cmd(unit, CMD_SET_WBECNT, (bcount + SECTSIZE - 1) / SECTSIZE);
	if (reply != 0) {
		/* Command rejected. */
		sd_deselect(io);
		syslog(LOG_INFO,
				"sd%d sd_write: bad SET_WBECNT reply = %02x, count = %u\n",
				unit, reply, (bcount + SECTSIZE - 1) / SECTSIZE);
		return 0;
	}

	/* Send write-multiple command. */
	if (u->card_type != TYPE_SDHC)
		offset <<= 9;
	reply = card_cmd(unit, CMD_WRITE_MULTIPLE, offset);
	if (reply != 0) {
		/* Command rejected. */
		sd_deselect(io);
		syslog(LOG_INFO, "sd%d sd_write: bad WRITE_MULTIPLE reply = %02x\n",
				unit, reply);
		return 0;
	}
	sd_deselect(io);
	again:
	/* Select, wait while busy. */
	sd_select(io);
	sd_wait_ready(io, TIMO_WAIT_WDATA, &sd_timo_wait_wdata);

	/* Send data. */
	spi_ll_transfer(io, WRITE_MULTIPLE_TOKEN, NULL);
	if (bcount >= SECTSIZE) {
		spi_ll_bulk_write32(io, SECTSIZE / 4, (uint32_t*) data);
		data += SECTSIZE;
	} else {
		spi_ll_bulk_write(io, bcount, (unsigned char *) data);
		data += bcount;
		for (i = bcount; i < SECTSIZE; i++)
			spi_ll_transfer(io, 0xFF, NULL);
	}
	/* Send dummy CRC. */
	spi_ll_transfer(io, 0xFF, NULL);
	spi_ll_transfer(io, 0xFF, NULL);

	/* Check if data accepted. */
	spi_ll_transfer(io, 0xFF, &reply);
	if ((reply & 0x1f) != 0x05) {
		/* Data rejected. */
		sd_deselect(io);
		syslog(LOG_INFO, "sd%d sd_write: data rejected, reply = %02x\n",
				unit, reply);
		return 0;
	}

	/* Wait for write completion. */
	sd_wait_ready(io, TIMO_WAIT_WDONE, &sd_timo_wait_wdone);
	sd_deselect(io);

	if (bcount > SECTSIZE) {
		/* Next sector. */
		bcount -= SECTSIZE;
		goto again;
	}

	/* Stop a write-multiple sequence. */
	sd_select(io);
	sd_wait_ready(io, TIMO_WAIT_WSTOP, &sd_timo_wait_wstop);
	spi_ll_transfer(io, STOP_TRAN_TOKEN, NULL);
	sd_wait_ready(io, TIMO_WAIT_WIDLE, &sd_timo_wait_widle);
	sd_deselect(io);
	return 1;
}

/*
 * Setup the SD card interface.
 * Get the card type and size.
 * Read a partition table.
 * Return 0 on failure.
 */
static int sd_setup(struct disk *u) {
	int unit = u->unit;

	if (!card_init(unit)) {
		syslog(LOG_INFO, "sd%d no SD card detected\n", unit);
		return 0;
	}
	/* Get the size of raw partition. */
	bzero(u->part, sizeof(u->part));
	u->part[RAWPART].dp_offset = 0;
	u->part[RAWPART].dp_size = sd_size(unit);
	if (u->part[RAWPART].dp_size == 0) {
		syslog(LOG_INFO, "sd%d cannot get card size\n", unit);
		return 0;
	}

	/* Switch to the high speed mode, if possible. */
	if (u->csd[4] & 0x40) {
		/* Class 10 card: switch to high-speed mode.
		 * SPI interface of pic32 allows up to 25MHz clock rate. */
		card_high_speed(unit);
	}

	uint32_t speed;

	spi_get_speed(u->spiio, &speed);

	syslog(LOG_INFO, "sd%d type %s, size %u kbytes, speed %u Mbit/sec\n", unit,
			u->card_type == TYPE_SDHC ? "SDHC" :
			u->card_type == TYPE_SD_II ? "II" : "I",
			u->part[RAWPART].dp_size / 2, speed / 1000000);

	/* Read partition table. */
	uint16_t buf[256];
	if (!sd_read(unit, 0, (char*) buf, sizeof(buf))) {
		syslog(LOG_INFO, "sd%d cannot read partition table\n", unit);
		return 0;
	}
	if (buf[255] == MBR_MAGIC) {
		bcopy(&buf[223], &u->part[1], 64);

		int i;
		for (i = 1; i <= NPARTITIONS; i++) {
			if (u->part[i].dp_type != 0)
				syslog(LOG_INFO,
						"sd%d%c: partition type %02x, sector %u, size %u kbytes\n",
						unit, i + 'a' - 1, u->part[i].dp_type,
						u->part[i].dp_offset, u->part[i].dp_size / 2);
		}
	}
	return 1;
}

/*
 * Test to see if device is present.
 * Return true if found and initialized ok.
 */
driver_error_t *sd_init(int unit) {
	driver_unit_lock_error_t *lock_error = NULL;

	struct disk *du = &sddrives[unit];
	int ok = 1;

	du->unit = unit;

	driver_error_t *error;
	if ((error = spi_setup(CONFIG_LUA_RTOS_SD_SPI, 1, CONFIG_LUA_RTOS_SD_CS, 0,
	250000, SPI_FLAG_WRITE | SPI_FLAG_READ, &du->spiio))) {
		free(error);
		syslog(LOG_ERR, "sd%u cannot open spi%u port (%s)", unit, du->spiio,
				driver_get_err_msg(error));
		return 0;
	}

	if ((lock_error = driver_lock(SPI_SD_DRIVER, unit, SPI_DRIVER, du->spiio, DRIVER_ALL_FLAGS, "spi sd"))) {
			// Revoked lock on pin
			return driver_lock_error(SPI_SD_DRIVER, lock_error);
	}

	syslog(LOG_INFO, "sd%u is at spi%d, pin cs=%s%d", unit,
	CONFIG_LUA_RTOS_SD_SPI, gpio_portname(CONFIG_LUA_RTOS_SD_CS),
			gpio_name(CONFIG_LUA_RTOS_SD_CS));

	ok = sd_setup(du);
	if (!ok) {
		syslog(LOG_ERR, "sd%u can't init sdcard", unit);
	}

	return NULL;
}

int sd_has_partitions(int unit) {
	struct disk *u = &sddrives[unit];
	int i;

	for (i = 1; i <= NPARTITIONS; i++) {
		if (u->part[i].dp_type != 0) {
			return 1;
		}
	}

	return 0;
}

int sd_has_partition(int unit, int type) {
	struct disk *u = &sddrives[unit];
	int i;

	for (i = 1; i <= NPARTITIONS; i++) {
		if (u->part[i].dp_type == type) {
			return 1;
		}
	}

	return 0;
}

DRIVER_REGISTER(SPI_SD,spi_sd,NULL, NULL,NULL);

#endif
