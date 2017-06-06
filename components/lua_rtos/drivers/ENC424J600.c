/*
 * Lua RTOS, ENC424J600 ethernet driver
 *
 * Copyright (C) 2015 - 2016
 * IBEROXARXA SERVICIOS INTEGRALES, S.L.
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

#include "sdkconfig.h"

#if CONFIG_SPI_ETHERNET

#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "freertos/queue.h"

#include "esp_intr.h"
#include "esp_event.h"
#include "esp_system.h"

#include "lwip/err.h"
#include "lwip/netif.h"
#include "netif/etharp.h"

#include "ENC424J600.h"

#include <time.h>
#include <stdint.h>

#include <sys/delay.h>
#include <sys/status.h>
#include <sys/syslog.h>

#include <drivers/gpio.h>
#include <drivers/spi.h>

#define UDA_WINDOW		(0x1)
#define GP_WINDOW		(0x2)
#define RX_WINDOW		(0x4)

extern void spi_ethernetif_input(struct netif *neti);

#define ENC424J600_INT_MASK (EIE_INTIE | EIR_LINKIF | EIR_PKTIF)

static volatile int currentBank;
static uint16_t nextPacketPointer;
static int spi_device;
static struct netif *interface;

static xQueueHandle ether_q = NULL;

static uint16_t exec_8_op(uint8_t op);
static uint16_t exec_16_op(uint8_t op, uint16_t data);
static uint32_t exec_32_op(uint8_t op, uint32_t data);
static void change_bank_if_needed(uint8_t bank);
static void write_reg(uint16_t address, uint16_t data);
static uint16_t read_reg(uint16_t address);
static void write_phy_reg(uint8_t address, uint16_t Data);
static void bfs_reg(uint16_t address, uint16_t bitMask);
static void bfc_reg(uint16_t address, uint16_t bitMask);
static int phy_reset();
static void mac_flush(void);
static void write_n(uint8_t op, uint8_t* data, uint16_t len);
static void read_n(uint8_t op, uint8_t* data, uint16_t len);
static void write_memory_window(uint8_t window, uint8_t *data, uint16_t len);
static void read_memory_window(uint8_t window, uint8_t *data, uint16_t len);
static int is_phy_linked(void);
static void link_status_change();

/*
 * Helper functions
 */
static uint16_t exec_8_op(uint8_t op) {
	uint8_t readed = 0;

	spi_ll_select(spi_device);
	spi_ll_transfer(spi_device, op, &readed);
	spi_ll_deselect(spi_device);

	return (uint16_t) readed;
}

static uint16_t exec_16_op(uint8_t op, uint16_t data) {
	uint16_t tmp = data;

	spi_ll_select(spi_device);
	spi_ll_transfer(spi_device, op, NULL);
	spi_ll_bulk_rw(spi_device, 2, (uint8_t *) &tmp);
	spi_ll_deselect(spi_device);

	return tmp;
}

static uint32_t exec_32_op(uint8_t op, uint32_t data) {
	uint32_t tmp = data;
	spi_ll_select(spi_device);
	spi_ll_transfer(spi_device, op, NULL);
	spi_ll_bulk_rw(spi_device, 3, (uint8_t *) &tmp);
	spi_ll_deselect(spi_device);

	return tmp;
}

static void change_bank_if_needed(uint8_t bank) {
	if (bank != currentBank) {
		if (bank == (0x0u << 5))
			exec_8_op(B0SEL);
		else if (bank == (0x1u << 5))
			exec_8_op(B1SEL);
		else if (bank == (0x2u << 5))
			exec_8_op(B2SEL);
		else if (bank == (0x3u << 5))
			exec_8_op(B3SEL);

		currentBank = bank;
	}
}

static void write_reg(uint16_t address, uint16_t data) {
	uint8_t bank;

	bank = ((uint8_t) address) & 0xE0;
	if (bank <= (0x3u << 5)) {
		change_bank_if_needed(bank);
		exec_16_op(WCR | (address & 0x1F), data);
	} else {
		uint32_t data32;
		((uint8_t*) &data32)[0] = (uint8_t) address;
		((uint8_t*) &data32)[1] = ((uint8_t*) &data)[0];
		((uint8_t*) &data32)[2] = ((uint8_t*) &data)[1];
		exec_32_op(WCRU, data32);
	}
}

static uint16_t read_reg(uint16_t address) {
	uint16_t returnValue;
	uint8_t bank;

	bank = ((uint8_t) address) & 0xE0;
	if (bank <= (0x3u << 5)) {
		change_bank_if_needed(bank);
		returnValue = exec_16_op(RCR | (address & 0x1F), 0x0000);
	} else {
		uint32_t returnValue32 = exec_32_op(RCRU, (uint32_t) address);
		((uint8_t*) &returnValue)[0] = ((uint8_t*) &returnValue32)[1];
		((uint8_t*) &returnValue)[1] = ((uint8_t*) &returnValue32)[2];
	}

	return returnValue;
}

static void write_phy_reg(uint8_t address, uint16_t Data) {
	// Write the register address
	write_reg(MIREGADR, 0x0100 | address);

	// Write the data
	write_reg(MIWR, Data);

	// Wait until the PHY register has been written
	while (read_reg(MISTAT) & MISTAT_BUSY)
		;
}

static void bfs_reg(uint16_t address, uint16_t bitMask) {
	uint8_t bank;

	bank = ((uint8_t) address) & 0xE0;
	change_bank_if_needed(bank);
	exec_16_op(BFS | (address & 0x1F), bitMask);
}

static void bfc_reg(uint16_t address, uint16_t bitMask) {
	uint8_t bank;

	bank = ((uint8_t) address) & 0xE0;
	change_bank_if_needed(bank);
	exec_16_op(BFC | (address & 0x1F), bitMask);
}

static int phy_reset() {
	uint16_t ret = 0;
	time_t start, now;

	do {
		// Set and clear a few bits that clears themselves upon reset.
		// If EUDAST cannot be written to and your code gets stuck in this
		// loop, you have a hardware problem of some sort (SPI or PMP not
		// initialized correctly, I/O pins aren't connected or are
		// shorted to something, power isn't available, etc.)

		time(&start);
		do {
			time(&now);
			if (now - start > 2) {
				return -1;
			}
			write_reg(EUDAST, 0x1234);
		} while ((ret = read_reg(EUDAST)) != 0x1234);

		// Issue a reset and wait for it to complete
		bfs_reg(ECON2, ECON2_ETHRST);
		currentBank = 0;

		#define RST_MASK (ESTAT_CLKRDY | ESTAT_RSTDONE | ESTAT_PHYRDY)
		time(&start);
		while (((ret = read_reg(ESTAT)) & RST_MASK) != RST_MASK) {
			time(&now);
			if (now - start > 2) {
				return -1;
			}
		}

		// Check to see if the reset operation was successful by
		// checking if EUDAST went back to its reset default.  This test
		// should always pass, but certain special conditions might make
		// this test fail, such as a PSP pin shorted to logic high.
	} while (((ret = read_reg(EUDAST)) != 0x0000u) && (ret != 0xffff));

	if (ret == 0xffff) {
		return -1;
	}

	// Really ensure reset is done and give some time for power to be stable
	delay(100);

	return 0;
}

/*
 * SPI Ethernet task. Waits the ISR for process an interrupt.
 *
 */
static void ether_task(void *taskArgs) {
	unsigned int flag;
	unsigned int dummy;

	for(;;) {
		// Wait the ISR
		xQueueReceive(ether_q, &dummy, portMAX_DELAY);

		// Get cause
		flag = read_reg(EIR);
		if (flag) {
			if (flag & EIR_LINKIF) {
				link_status_change();
			}

			if (flag & EIR_PKTIF) {
				spi_ethernetif_input(interface);
			}
		}

		// Clear all interrupt flags
		bfc_reg(EIR, 0xfff);
	}
}

static void IRAM_ATTR ether_intr(void* arg) {
	unsigned int dummy = 0;

	// Inform the ethernet task that there is a new interrupt
	// for process. A dummy value is queued, because we don't
	// want to pass data.
	xQueueSendFromISR(ether_q, &dummy, NULL);
}

static void mac_flush(void) {
	uint16_t w;

	// Check to see if the duplex status has changed.  This can
	// change if the user unplugs the cable and plugs it into a
	// different node.  Auto-negotiation will automatically set
	// the duplex in the PHY, but we must also update the MAC
	// inter-packet gap timing and duplex state to match.
	if (read_reg(EIR) & EIR_LINKIF) {
		bfc_reg(EIR, EIR_LINKIF);

		// Update MAC duplex settings to match PHY duplex setting
		w = read_reg(MACON2);
		if (read_reg(ESTAT) & ESTAT_PHYDPX) {
			// Switching to full duplex
			write_reg(MABBIPG, 0x15);
			w |= MACON2_FULDPX;
		} else {
			// Switching to half duplex
			write_reg(MABBIPG, 0x12);
			w &= ~MACON2_FULDPX;
		}
		write_reg(MACON2, w);
	}

	// Start the transmission, but only if we are linked.  Supressing
	// transmissing when unlinked is necessary to avoid stalling the TX engine
	// if we are in PHY energy detect power down mode and no link is present.
	// A stalled TX engine won't do any harm in itself, but will cause the
	// MACIsTXReady() function to continuously return FALSE, which will
	// ultimately stall the Microchip TCP/IP stack since there is blocking code
	// elsewhere in other files that expect the TX engine to always self-free
	// itself very quickly.
	if (read_reg(ESTAT) & ESTAT_PHYLNK)
		bfs_reg(ECON1, ECON1_TXRTS);
}

static void write_n(uint8_t op, uint8_t* data, uint16_t len) {
	spi_ll_select(spi_device);
	spi_ll_transfer(spi_device, op, NULL);
	spi_ll_bulk_write(spi_device, len, data);
	spi_ll_deselect(spi_device);
}

static void read_n(uint8_t op, uint8_t* data, uint16_t len) {
	spi_ll_select(spi_device);
	spi_ll_transfer(spi_device, op, NULL);
	spi_ll_bulk_read(spi_device, len, data);
	spi_ll_deselect(spi_device);
}

static void write_memory_window(uint8_t window, uint8_t *data, uint16_t len) {
	uint8_t op = RBMUDA;

	if (window & GP_WINDOW)
		op = WBMGP;
	if (window & RX_WINDOW)
		op = WBMRX;

	write_n(op, data, len);
}

static void read_memory_window(uint8_t window, uint8_t *data, uint16_t len) {
	if (len == 0u)
		return;

	uint8_t op = RBMUDA;

	if (window & GP_WINDOW)
		op = RBMGP;
	if (window & RX_WINDOW)
		op = RBMRX;

	read_n(op, data, len);
}

static int is_phy_linked(void) {
	return (read_reg(ESTAT) & ESTAT_PHYLNK) != 0u;
}

static void link_status_change() {
	system_event_t evt;
	uint16_t w;

	if (is_phy_linked()) {
		// Update MAC duplex settings to match PHY duplex setting
		w = read_reg(MACON2);
		if (read_reg(ESTAT) & ESTAT_PHYDPX) {
			// Switching to full duplex
			write_reg(MABBIPG, 0x15);
			w |= MACON2_FULDPX;
		} else {
			// Switching to half duplex
			write_reg(MABBIPG, 0x12);
			w &= ~MACON2_FULDPX;
		}

		write_reg(MACON2, w);

		evt.event_id = SYSTEM_EVENT_SPI_ETH_CONNECTED;
		esp_event_send(&evt);
	} else {
		evt.event_id = SYSTEM_EVENT_SPI_ETH_DISCONNECTED;
		esp_event_send(&evt);
	}
}

/*
 * Operation functions
 */
int enc424j600_init(struct netif *netif) {
	currentBank = 0;

	interface = netif;

	// Configure SPI
	driver_error_t *error;

	if ((error = spi_setup(CONFIG_SPI_ETHERNET_SPI, 1, CONFIG_SPI_ETHERNET_CS, 0, CONFIG_SPI_ETHERNET_SPEED,
			SPI_FLAG_WRITE | SPI_FLAG_READ, &spi_device))) {
		syslog(LOG_ERR, "enc424j600 cannot open spi%d", CONFIG_SPI_ETHERNET_SPI);
		return 0;
	}

	syslog(LOG_INFO, "enc424j600 is at spi%d, pin cs=%s%d, speed %d Mhz",
			CONFIG_SPI_ETHERNET_SPI,
			gpio_portname(CONFIG_SPI_ETHERNET_CS), gpio_name(CONFIG_SPI_ETHERNET_CS),
			CONFIG_SPI_ETHERNET_SPEED / 1000000);

	if (phy_reset() < 0) {
		syslog(LOG_ERR, "enc424j600 can't reset");

		return ERR_IF;
	}

	// Initialize RX tracking variables and other control state flags
	nextPacketPointer = ENC424J600_RXSTART;

	// Set up TX/RX/UDA buffer addresses
	write_reg(ETXST, ENC424J600_TXSTART);
	write_reg(ERXST, ENC424J600_RXSTART);
	write_reg(ERXTAIL, ENC424J600_RAMSIZE - 2);
	write_reg(EUDAST, ENC424J600_RAMSIZE);
	write_reg(EUDAND, ENC424J600_RAMSIZE + 1);

	// Get MAC adress
	uint16_t regValue;
	regValue = read_reg(MAADR1);

	netif->hwaddr_len = ETHARP_HWADDR_LEN;

	netif->hwaddr[0] = ((uint8_t*) &regValue)[0];
	netif->hwaddr[1] = ((uint8_t*) &regValue)[1];

	regValue = read_reg(MAADR2);
	netif->hwaddr[2] = ((uint8_t*) &regValue)[0];
	netif->hwaddr[3] = ((uint8_t*) &regValue)[1];

	regValue = read_reg(MAADR3);
	netif->hwaddr[4] = ((uint8_t*) &regValue)[0];
	netif->hwaddr[5] = ((uint8_t*) &regValue)[1];

	syslog(LOG_INFO, "enc424j600 mac address is %02x:%02x:%02x:%02x:%02x:%02x",
			netif->hwaddr[0], netif->hwaddr[1], netif->hwaddr[2],
			netif->hwaddr[3], netif->hwaddr[4], netif->hwaddr[5]);

	// Set PHY Auto-negotiation to support 10BaseT Half duplex,
	// 10BaseT Full duplex, 100BaseTX Half Duplex, 100BaseTX Full Duplex,
	// and symmetric PAUSE capability
	write_phy_reg(PHANA,
			PHANA_ADPAUS0 | PHANA_AD10FD | PHANA_AD10 | PHANA_AD100FD
					| PHANA_AD100 | PHANA_ADIEEE0);

	// Enable RX packet reception
	bfs_reg(ECON1, ECON1_RXEN);

    // Disable all interrupts
	bfc_reg(EIE, ENC424J600_INT_MASK);

    // Clear all interrupt flags
    bfc_reg(EIR, 0xfff);

    // Configure external interrupt line
	if (!ether_q) {
		ether_q = xQueueCreate(100, sizeof(unsigned int));
	}

	// ISR related task must run on the same core that ISR handler is added
    xTaskCreatePinnedToCore(ether_task, "eth", 1024 * 4, NULL, configMAX_PRIORITIES - 2, NULL, xPortGetCoreID());

	if (!status_get(STATUS_ISR_SERVICE_INSTALLED)) {
		gpio_install_isr_service(0);

		status_set(STATUS_ISR_SERVICE_INSTALLED);
	}

	gpio_pin_input(CONFIG_SPI_ETHERNET_INT);

	gpio_set_intr_type(CONFIG_SPI_ETHERNET_INT, GPIO_INTR_NEGEDGE);
	gpio_isr_handler_add(CONFIG_SPI_ETHERNET_INT, ether_intr, NULL);

    // Enable interrupts
	bfs_reg(EIE, ENC424J600_INT_MASK);

	return ERR_OK;
}

struct pbuf *enc424j600_input(struct netif *netif) {
    RXSTATUS statusVector;
    uint16_t newRXTail;
    struct pbuf *p = NULL, *q = NULL;
    u16_t len = 0;
    u16_t frame_size = 0;

    // Ensure that are pending packets
	if (!(read_reg(ESTAT) & 0b11111111)) {
		return NULL;
	}

    // Set the RX Read Pointer to the beginning of the next unprocessed packet
    write_reg(ERXRDPT, nextPacketPointer);

    // Read the adress of next packet
    read_memory_window(RX_WINDOW, (uint8_t*) & nextPacketPointer, sizeof (nextPacketPointer));

    // Read the receive status vector
    read_memory_window(RX_WINDOW, (uint8_t*) & statusVector, sizeof (statusVector));

    // Check the packet
    if (
    	statusVector.bits.Zero || statusVector.bits.ZeroH || statusVector.bits.CRCError ||
		statusVector.bits.ByteCount > 1522u || !statusVector.bits.ReceiveOk
	) {
    	goto exit;
    }

    // Get the packet length
    len = statusVector.bits.ByteCount - 4;

    // If we don't receive nothing, exit
    if (len == 0) {
    	goto exit;
    }

    frame_size = len;

    #if ETH_PAD_SIZE
    len += ETH_PAD_SIZE; /* allow room for Ethernet padding */
    #endif

    /* We allocate a pbuf chain of pbufs from the pool. */
    p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
    if (p != NULL) {
        #if ETH_PAD_SIZE
        pbuf_header(p, -ETH_PAD_SIZE); /* drop the padding word */
        #endif

        q = p;

        read_memory_window(RX_WINDOW, q->payload, frame_size);

        #if ETH_PAD_SIZE
        pbuf_header(p, ETH_PAD_SIZE); /* reclaim the padding word */
        #endif

        LINK_STATS_INC(link.recv);
    } else {
        LINK_STATS_INC(link.memerr);
        LINK_STATS_INC(link.drop);
    }

exit:
	newRXTail = nextPacketPointer - 2;

	//Special situation if nextPacketPointer is exactly RXSTART
	if (nextPacketPointer == ENC424J600_RXSTART)
		newRXTail = ENC424J600_RAMSIZE - 2;

	//Packet decrement
	bfs_reg(ECON1, ECON1_PKTDEC);

	//Write new RX tail
	write_reg(ERXTAIL, newRXTail);

    return p;
}

err_t enc424j600_output(struct netif *netif, struct pbuf *p) {
	struct pbuf *q;

	#if ETH_PAD_SIZE
	pbuf_header(p, -ETH_PAD_SIZE); /* drop the padding word */
#endif

	q = p;
	if (q->len > 0) {
		write_memory_window(GP_WINDOW, q->payload, q->len);

		write_reg(EGPWRPT, ENC424J600_TXSTART);
		write_reg(ETXLEN, q->len);

		mac_flush();
	}

#if ETH_PAD_SIZE
	pbuf_header(p, ETH_PAD_SIZE); /* reclaim the padding word */
#endif

	LINK_STATS_INC(link.xmit);

	return ERR_OK;
}

#endif
