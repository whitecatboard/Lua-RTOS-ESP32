/*
 * Copyright (C) 2015 - 2020, IBEROXARXA SERVICIOS INTEGRALES, S.L.
 * Copyright (C) 2015 - 2020, Jaume Olivé Petrus (jolive@whitecatboard.org)
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the <organization> nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *     * The WHITECAT logotype cannot be changed, you can remove it, but you
 *       cannot change it in any way. The WHITECAT logotype is:
 *
 *          /\       /\
 *         /  \_____/  \
 *        /_____________\
 *        W H I T E C A T
 *
 *     * Redistributions in binary form must retain all copyright notices printed
 *       to any local or remote output device. This include any reference to
 *       Lua RTOS, whitecatboard.org, Lua, and other copyright notices that may
 *       appear in the future.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Lua RTOS, ENC424J600 ethernet driver
 *
 */

#include <string.h>
#include <stdlib.h>
#include <sys/cdefs.h>

#include "esp_log.h"
#include "esp_system.h"
#include "esp_cpu.h"
#include "esp_intr_alloc.h"
#include "esp_heap_caps.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "enc424j600.h"
#include "sdkconfig.h"

#include "gpio.h"
#include "spi.h"
#include "syslog.h"
#include "delay.h"


static const char *TAG = "enc424j600";
#define MAC_CHECK(a, str, goto_tag, ret_value, ...)                               \
    do                                                                            \
    {                                                                             \
        if (!(a))                                                                 \
        {                                                                         \
            ESP_LOGE(TAG, "%s(%d): " str, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
            ret = ret_value;                                                      \
            goto goto_tag;                                                        \
        }                                                                         \
    } while (0)

#define ENC424J600_SPI_LOCK_TIMEOUT_MS (50)
#define ENC424J600_REG_TRANS_LOCK_TIMEOUT_MS (150)
#define ENC424J600_PHY_OPERATION_TIMEOUT_US (150)

#define ENC424J600_INT_MASK (EIE_INTIE | EIR_LINKIF | EIR_PKTIF)
#define ENC424J600_RST_MASK (ESTAT_CLKRDY | ESTAT_RSTDONE | ESTAT_PHYRDY)

typedef struct {
    esp_eth_mac_t parent;
    esp_eth_mediator_t *eth;
    int spi_device;
    SemaphoreHandle_t lock;
    TaskHandle_t rx_task_hdl;
    uint32_t sw_reset_timeout_ms;
    uint8_t addr[6];
    uint8_t last_bank;
    bool packets_remain;
    uint16_t next_packet_pointer;
    uint32_t revision;
} emac_enc424j600_t;

static inline bool lock(emac_enc424j600_t *emac) {
    return xSemaphoreTake(emac->lock, pdMS_TO_TICKS(ENC424J600_SPI_LOCK_TIMEOUT_MS)) == pdTRUE;
}

static inline bool unlock(emac_enc424j600_t *emac) {
    return xSemaphoreGive(emac->lock) == pdTRUE;
}

static uint16_t exec_8_op(emac_enc424j600_t *emac, uint8_t op) {
    uint8_t readed = 0;

    spi_ll_select(emac->spi_device);
    spi_ll_transfer(emac->spi_device, op, &readed);
    spi_ll_deselect(emac->spi_device);

    return (uint16_t) readed;
}

static uint16_t exec_16_op(emac_enc424j600_t *emac, uint8_t op, uint16_t data) {
    uint16_t tmp = data;

    spi_ll_select(emac->spi_device);
    spi_ll_transfer(emac->spi_device, op, NULL);
    spi_ll_bulk_rw(emac->spi_device, 2, (uint8_t *) &tmp);
    spi_ll_deselect(emac->spi_device);

    return tmp;
}

static uint32_t exec_32_op(emac_enc424j600_t *emac, uint8_t op, uint32_t data) {
    uint32_t tmp = data;
    spi_ll_select(emac->spi_device);
    spi_ll_transfer(emac->spi_device, op, NULL);
    spi_ll_bulk_rw(emac->spi_device, 3, (uint8_t *) &tmp);
    spi_ll_deselect(emac->spi_device);

    return tmp;
}

static void change_bank_if_needed(emac_enc424j600_t *emac, uint8_t bank) {
    if (bank != emac->last_bank) {
        if (bank == (0x0u << 5)) {
            exec_8_op(emac, B0SEL);
        } else if (bank == (0x1u << 5)) {
            exec_8_op(emac, B1SEL);
        } else if (bank == (0x2u << 5)) {
            exec_8_op(emac, B2SEL);
        } else if (bank == (0x3u << 5)) {
            exec_8_op(emac, B3SEL);
        }

        emac->last_bank = bank;
    }
}

static void write_reg(emac_enc424j600_t *emac, uint16_t address, uint16_t data) {
    uint8_t bank;

    bank = ((uint8_t) address) & 0xE0;
    if (bank <= (0x3u << 5)) {
        change_bank_if_needed(emac, bank);
        exec_16_op(emac, WCR | (address & 0x1F), data);
    } else {
        uint32_t data32;
        ((uint8_t*) &data32)[0] = (uint8_t) address;
        ((uint8_t*) &data32)[1] = ((uint8_t*) &data)[0];
        ((uint8_t*) &data32)[2] = ((uint8_t*) &data)[1];
        exec_32_op(emac, WCRU, data32);
    }
}

static uint16_t read_reg(emac_enc424j600_t *emac, uint16_t address) {
    uint16_t returnValue;
    uint8_t bank;

    bank = ((uint8_t) address) & 0xE0;
    if (bank <= (0x3u << 5)) {
        change_bank_if_needed(emac, bank);
        returnValue = exec_16_op(emac, RCR | (address & 0x1F), 0x0000);
    } else {
        uint32_t returnValue32 = exec_32_op(emac, RCRU, (uint32_t) address);
        ((uint8_t*) &returnValue)[0] = ((uint8_t*) &returnValue32)[1];
        ((uint8_t*) &returnValue)[1] = ((uint8_t*) &returnValue32)[2];
    }

    return returnValue;
}

static int write_phy_reg(emac_enc424j600_t *emac, uint8_t address, uint16_t data) {
    if (!lock(emac)) {
    	return -1;
    }

    // Write the register address
    write_reg(emac, MIREGADR, 0x0100 | address);

    // Write the data
    write_reg(emac, MIWR, data);

    // Wait until the PHY register has been written
    uint32_t to = 0;
    uint16_t status;

    do {
        esp_rom_delay_us(15);
        status = read_reg(emac, MISTAT);
        to += 15;
    } while ((status & MISTAT_BUSY) && (to < ENC424J600_PHY_OPERATION_TIMEOUT_US));

    if (status & MISTAT_BUSY) {
    	unlock(emac);
    	return -1;
    }

    unlock(emac);

    return 0;
}

static int read_phy_reg(emac_enc424j600_t *emac, uint8_t address, uint16_t *data) {
    if (!lock(emac)) {
    	return -1;
    }

    // Write the register address
    write_reg(emac, MIREGADR, 0x0100 | address);

    // Set MII read enable bit
    uint16_t cmd;

    cmd = read_reg(emac, MICMD_MIIRD);
    cmd |= MICMD_MIIRD;
    write_reg(emac, MICMD, cmd);

    // Polling the busy flag
    uint32_t to = 0;
    uint16_t status;

    do {
        esp_rom_delay_us(15);
        status = read_reg(emac, MISTAT);
        to += 15;
    } while ((status & MISTAT_BUSY) && (to < ENC424J600_PHY_OPERATION_TIMEOUT_US));

    if (status & MISTAT_BUSY) {
    	unlock(emac);
    	return -1;
    }

    // Clear MII read enable bit
    cmd = read_reg(emac, MICMD_MIIRD);
    cmd &= ~MICMD_MIIRD;
    write_reg(emac, MICMD, cmd);

    // Read the data
    *data = read_reg(emac, MIRD);

    unlock(emac);
    return 0;
}

static void bfs_reg(emac_enc424j600_t *emac, uint16_t address, uint16_t bitMask) {
    uint8_t bank;

    bank = ((uint8_t) address) & 0xE0;
    change_bank_if_needed(emac, bank);
    exec_16_op(emac, BFS | (address & 0x1F), bitMask);
}

static void bfc_reg(emac_enc424j600_t *emac, uint16_t address, uint16_t bitMask) {
    uint8_t bank;

    bank = ((uint8_t) address) & 0xE0;
    change_bank_if_needed(emac, bank);
    exec_16_op(emac, BFC | (address & 0x1F), bitMask);
}

static int reset(emac_enc424j600_t *emac) {
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
            write_reg(emac, EUDAST, 0x1234);
        } while ((ret = read_reg(emac, EUDAST)) != 0x1234);

        // Issue a reset and wait for it to complete
        bfs_reg(emac, ECON2, ECON2_ETHRST);

        emac->last_bank = 0;

        time(&start);
        while (((ret = read_reg(emac, ESTAT)) & ENC424J600_RST_MASK) != ENC424J600_RST_MASK) {
            time(&now);
            if (now - start > 2) {
                return -1;
            }
        }

        // Check to see if the reset operation was successful by
        // checking if EUDAST went back to its reset default.  This test
        // should always pass, but certain special conditions might make
        // this test fail, such as a PSP pin shorted to logic high.
    } while (((ret = read_reg(emac, EUDAST)) != 0x0000u) && (ret != 0xffff));

    if (ret == 0xffff) {
        return -1;
    }

    // Really ensure reset is done and give some time for power to be stable
    delay(100);

    return 0;
}

static void mac_flush(emac_enc424j600_t *emac) {
    uint16_t w;

    // Check to see if the duplex status has changed.  This can
    // change if the user unplugs the cable and plugs it into a
    // different node.  Auto-negotiation will automatically set
    // the duplex in the PHY, but we must also update the MAC
    // inter-packet gap timing and duplex state to match.
    if (read_reg(emac, EIR) & EIR_LINKIF) {
        bfc_reg(emac, EIR, EIR_LINKIF);

        // Update MAC duplex settings to match PHY duplex setting
        w = read_reg(emac, MACON2);
        if (read_reg(emac, ESTAT) & ESTAT_PHYDPX) {
            // Switching to full duplex
            write_reg(emac, MABBIPG, 0x15);
            w |= MACON2_FULDPX;
        } else {
            // Switching to half duplex
            write_reg(emac, MABBIPG, 0x12);
            w &= ~MACON2_FULDPX;
        }
        write_reg(emac, MACON2, w);
    }

    // Start the transmission, but only if we are linked.  Supressing
    // transmissing when unlinked is necessary to avoid stalling the TX engine
    // if we are in PHY energy detect power down mode and no link is present.
    // A stalled TX engine won't do any harm in itself, but will cause the
    // MACIsTXReady() function to continuously return FALSE, which will
    // ultimately stall the Microchip TCP/IP stack since there is blocking code
    // elsewhere in other files that expect the TX engine to always self-free
    // itself very quickly.
    if (read_reg(emac, ESTAT) & ESTAT_PHYLNK) {
        bfs_reg(emac, ECON1, ECON1_TXRTS);
    }
}

static void write_n(emac_enc424j600_t *emac, uint8_t op, uint8_t* data, uint16_t len) {
    spi_ll_select(emac->spi_device);
    spi_ll_transfer(emac->spi_device, op, NULL);
    spi_ll_bulk_write(emac->spi_device, len, data);
    spi_ll_deselect(emac->spi_device);
}

static void read_n(emac_enc424j600_t *emac, uint8_t op, uint8_t* data, uint16_t len) {
    spi_ll_select(emac->spi_device);
    spi_ll_transfer(emac->spi_device, op, NULL);
    spi_ll_bulk_read(emac->spi_device, len, data);
    spi_ll_deselect(emac->spi_device);
}

static void write_memory_window(emac_enc424j600_t *emac, uint8_t window, uint8_t *data, uint16_t len) {
    uint8_t op = RBMUDA;

    if (window & GP_WINDOW)
        op = WBMGP;
    if (window & RX_WINDOW)
        op = WBMRX;

    write_n(emac, op, data, len);
}

static void read_memory_window(emac_enc424j600_t *emac, uint8_t window, uint8_t *data, uint16_t len) {
    if (len == 0u)
        return;

    uint8_t op = RBMUDA;

    if (window & GP_WINDOW)
        op = RBMGP;
    if (window & RX_WINDOW)
        op = RBMRX;

    read_n(emac, op, data, len);
}

/**
 * @brief Write enc424j600 internal PHY register
 */
static esp_err_t emac_enc424j600_write_phy_reg(esp_eth_mac_t *mac, uint32_t phy_addr,
        uint32_t phy_reg, uint32_t reg_value) {
    emac_enc424j600_t *emac = __containerof(mac, emac_enc424j600_t, parent);

	if (write_phy_reg(emac, (uint8_t)phy_reg, (uint16_t)reg_value) < 0) {
		return ESP_ERR_TIMEOUT;
	}

    return ESP_OK;
}

/**
 * @brief Read enc424j600 internal PHY register
 */
static esp_err_t emac_enc424j600_read_phy_reg(esp_eth_mac_t *mac, uint32_t phy_addr,
        uint32_t phy_reg, uint32_t *reg_value) {
    emac_enc424j600_t *emac = __containerof(mac, emac_enc424j600_t, parent);

	if (read_phy_reg(emac, phy_reg, (uint16_t *)reg_value) < 0) {
		return ESP_ERR_TIMEOUT;
	}

    return ESP_OK;
}

/**
 * @brief Verify chip revision ID
 */
static esp_err_t enc424j600_verify_id(emac_enc424j600_t *emac)
{
    esp_err_t ret = ESP_OK;

    uint16_t eidled;
    uint16_t devid;
    uint16_t revid;

    eidled = read_reg(emac, EIDLED);
    devid = (eidled &  (EIDLED_DEVID2 | EIDLED_DEVID1 | EIDLED_DEVID0)) >> 5;
    revid = eidled &  (EIDLED_REVID4 | EIDLED_REVID3 | EIDLED_REVID2 | EIDLED_REVID1 | EIDLED_REVID0);

    MAC_CHECK(devid == 0x01, "wrong chip ID", out, ESP_ERR_INVALID_VERSION);

    emac->revision = revid;

out:
    return ret;
}

/**
 * @brief Set mediator for Ethernet MAC
 */
static esp_err_t emac_enc424j600_set_mediator(esp_eth_mac_t *mac, esp_eth_mediator_t *eth) {
    esp_err_t ret = ESP_OK;
    MAC_CHECK(eth, "can't set mac's mediator to null", out, ESP_ERR_INVALID_ARG);
    emac_enc424j600_t *emac = __containerof(mac, emac_enc424j600_t, parent);
    emac->eth = eth;
out:
    return ret;
}

/**
 * @brief Start enc424j600: enable interrupt and start receive
 */
static esp_err_t emac_enc424j600_start(esp_eth_mac_t *mac) {
    esp_err_t ret = ESP_OK;

    emac_enc424j600_t *emac = __containerof(mac, emac_enc424j600_t, parent);

    if (!lock(emac)) {
    	return ESP_ERR_TIMEOUT;
    }

    // Clear all interrupt flags
    bfc_reg(emac, EIR, 0xfff);

    // Enable interrupts
    bfs_reg(emac, EIE, ENC424J600_INT_MASK);

    // Enable RX packet reception
    bfs_reg(emac, ECON1, ECON1_RXEN);

    unlock(emac);

    return ret;
}

/**
 * @brief   Stop enc424j600: disable interrupt and stop receiving packets
 */
static esp_err_t emac_enc424j600_stop(esp_eth_mac_t *mac) {
    esp_err_t ret = ESP_OK;
    emac_enc424j600_t *emac = __containerof(mac, emac_enc424j600_t, parent);

    if (!lock(emac)) {
    	return ESP_ERR_TIMEOUT;
    }

    // Disable interrupts
    bfc_reg(emac, EIE, ENC424J600_INT_MASK);

    // Disable RX packet reception
    bfc_reg(emac, ECON1, ECON1_RXEN);

    unlock(emac);

    return ret;
}

static esp_err_t emac_enc424j600_set_addr(esp_eth_mac_t *mac, uint8_t *addr) {
    return ESP_OK;
}

static esp_err_t emac_enc424j600_get_addr(esp_eth_mac_t *mac, uint8_t *addr) {
    esp_err_t ret = ESP_OK;
    MAC_CHECK(addr, "can't set mac addr to null", out, ESP_ERR_INVALID_ARG);
    emac_enc424j600_t *emac = __containerof(mac, emac_enc424j600_t, parent);

    if (!lock(emac)) {
    	return ESP_ERR_TIMEOUT;
    }

    // Get MAC adress
    uint8_t buff[6];
    uint16_t regValue;
    regValue = read_reg(emac, MAADR1);

    buff[0] = ((uint8_t*) &regValue)[0];
    buff[1] = ((uint8_t*) &regValue)[1];

    regValue = read_reg(emac, MAADR2);
    buff[2] = ((uint8_t*) &regValue)[0];
    buff[3] = ((uint8_t*) &regValue)[1];

    regValue = read_reg(emac, MAADR3);
    buff[4] = ((uint8_t*) &regValue)[0];
    buff[5] = ((uint8_t*) &regValue)[1];

    memcpy(emac->addr, buff, 6);
    memcpy(addr, emac->addr, 6);

    unlock(emac);

out:
    return ret;
}

static void enc424j600_isr_handler(void *arg) {
    emac_enc424j600_t *emac = (emac_enc424j600_t *)arg;
    BaseType_t high_task_wakeup = pdFALSE;

    vTaskNotifyGiveFromISR(emac->rx_task_hdl, &high_task_wakeup);
    if (high_task_wakeup != pdFALSE) {
        portYIELD_FROM_ISR();
    }
}

static void emac_enc424j600_task(void *arg) {
    emac_enc424j600_t *emac = (emac_enc424j600_t *)arg;

    unsigned int flag;
    uint8_t *buffer = NULL;
    uint32_t length = 0;

    for(;;) {
        // Block until notification received
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // Get cause
        flag = read_reg(emac, EIR);
        if (flag) {
            if (flag & EIR_PKTIF) {
                do {
                    length = ETH_MAX_PACKET_SIZE;
                    buffer = heap_caps_malloc(length, MALLOC_CAP_DMA);
                    if (!buffer) {
                        ESP_LOGE(TAG, "no mem for receive buffer");
                    } else if (emac->parent.receive(&emac->parent, buffer, &length) == ESP_OK) {
                        /* pass the buffer to stack (e.g. TCP/IP layer) */
                        if (length) {
                            emac->eth->stack_input(emac->eth, buffer, length);
                        } else {
                            free(buffer);
                        }
                    } else {
                        free(buffer);
                    }
                } while (emac->packets_remain);
            }
        }

        // Clear all interrupt flags
        bfc_reg(emac, EIR, 0xfff);
    }
}

static esp_err_t emac_enc424j600_set_link(esp_eth_mac_t *mac, eth_link_t link) {
    esp_err_t ret = ESP_OK;

    switch (link) {
		case ETH_LINK_UP:
			MAC_CHECK(mac->start(mac) == ESP_OK, "enc424j600 start failed", out, ESP_FAIL);
			break;
		case ETH_LINK_DOWN:
			MAC_CHECK(mac->stop(mac) == ESP_OK, "enc424j600 stop failed", out, ESP_FAIL);
			break;
		default:
			MAC_CHECK(false, "unknown link status", out, ESP_ERR_INVALID_ARG);
			break;
    }
out:
    return ret;
}

static esp_err_t emac_enc424j600_set_speed(esp_eth_mac_t *mac, eth_speed_t speed) {
    return ESP_OK;
}

static esp_err_t emac_enc424j600_set_duplex(esp_eth_mac_t *mac, eth_duplex_t duplex) {
    esp_err_t ret = ESP_OK;
    emac_enc424j600_t *emac = __containerof(mac, emac_enc424j600_t, parent);
    uint8_t mac2 = 0;

    if (!lock(emac)) {
    	return ESP_ERR_TIMEOUT;
    }

    mac2 = read_reg(emac, MACON2);

    switch (duplex) {
		case ETH_DUPLEX_HALF:
			mac2 &= ~MACON2_FULDPX;
			write_reg(emac, MABBIPG, 0x12);
			break;
		case ETH_DUPLEX_FULL:
			mac2 |= MACON2_FULDPX;
			write_reg(emac, MABBIPG, 0x15);
			break;
		default:
			unlock(emac);
			return ESP_ERR_INVALID_ARG;
			break;
    }

    write_reg(emac, MACON2, mac2);

    unlock(emac);

    return ret;
}

static esp_err_t emac_enc424j600_set_promiscuous(esp_eth_mac_t *mac, bool enable) {
# if 0
    esp_err_t ret = ESP_OK;
    emac_enc424j600_t *emac = __containerof(mac, emac_enc424j600_t, parent);
    if (enable) {
        MAC_CHECK(enc424j600_register_write(emac, enc424j600_ERXFCON, 0x00) == ESP_OK,
                  "write ERXFCON failed", out, ESP_FAIL);
    }
out:
    return ret;
#else
    return ESP_OK;
#endif
}

static esp_err_t emac_enc424j600_transmit(esp_eth_mac_t *mac, uint8_t *buf, uint32_t length) {
    esp_err_t ret = ESP_OK;
    emac_enc424j600_t *emac = __containerof(mac, emac_enc424j600_t, parent);

    if (!lock(emac)) {
    	return ESP_ERR_TIMEOUT;
    }

	write_memory_window(emac, GP_WINDOW, buf, length);

	write_reg(emac, EGPWRPT, ENC424J600_TXSTART);
	write_reg(emac, ETXLEN, length);

    mac_flush(emac);

    unlock(emac);

    return ret;
}

static esp_err_t emac_enc424j600_receive(esp_eth_mac_t *mac, uint8_t *buf, uint32_t *length) {
    esp_err_t ret = ESP_OK;
    emac_enc424j600_t *emac = __containerof(mac, emac_enc424j600_t, parent);

    RXSTATUS statusVector;
    uint16_t len = 0;

    if (!lock(emac)) {
    	return ESP_ERR_TIMEOUT;
    }

    // Ensure that are pending packets
    if (!(read_reg(emac, ESTAT) & 0b11111111)) {
    	unlock(emac);
        return ESP_FAIL;
    }

    // Set the RX read pointer to the beginning of the next unprocessed packet
    write_reg(emac, ERXRDPT, emac->next_packet_pointer);

    // Read the address of the next packet
    read_memory_window(emac, RX_WINDOW, (uint8_t *)&emac->next_packet_pointer, sizeof(emac->next_packet_pointer));

    // Read the receive status vector
    read_memory_window(emac, RX_WINDOW, (uint8_t*)&statusVector, sizeof(statusVector));

    // Check the packet
    if (
        statusVector.bits.Zero || statusVector.bits.ZeroH || statusVector.bits.CRCError ||
        (statusVector.bits.ByteCount > 1522u) || !statusVector.bits.ReceiveOk
    ) {
        goto exit;
    }

    // Get the packet length
    len = statusVector.bits.ByteCount - 4;

    // If we don't receive nothing, exit
    if (len == 0) {
        goto exit;
    }

    // Read packet content
    read_memory_window(emac, RX_WINDOW, buf, len);

exit:
	// Compute new RX tail
	uint16_t newRXTail = emac->next_packet_pointer - 2;

    // Special situation if next_packet_pointer is exactly RXSTART
    if (emac->next_packet_pointer == ENC424J600_RXSTART) {
        newRXTail = ENC424J600_RAMSIZE - 2;
    }

    // Packet decrement
    bfs_reg(emac, ECON1, ECON1_PKTDEC);

    // Write new RX tail
    write_reg(emac, ERXTAIL, newRXTail);

    // Check if we have pending packets
    uint8_t pending = (read_reg(emac, ESTAT) & 0b11111111);

    emac->packets_remain = (pending > 0);

    unlock(emac);

    return ret;
}

static esp_err_t emac_enc424j600_init(esp_eth_mac_t *mac) {
	esp_err_t ret = ESP_OK;
    emac_enc424j600_t *emac = __containerof(mac, emac_enc424j600_t, parent);
    esp_eth_mediator_t *eth = emac->eth;

	// Init INT pin to get enc424j600 interrupt
    gpio_pin_input(CONFIG_SPI_ETHERNET_INT);
    gpio_isr_attach(CONFIG_SPI_ETHERNET_INT, enc424j600_isr_handler, GPIO_INTR_NEGEDGE, emac);

    // Reset
    MAC_CHECK(reset(emac) >= 0, "reset failed", out, ESP_FAIL);
    MAC_CHECK(enc424j600_verify_id(emac) == ESP_OK, "unexpected chip ID", out, ESP_FAIL);

    syslog(LOG_INFO, "enc424j600 rev %d is at spi%d, cs=%s%d, int=%s%d, speed %d Mhz",
			emac->revision,
            CONFIG_SPI_ETHERNET_SPI,
            gpio_portname(CONFIG_SPI_ETHERNET_CS), gpio_name(CONFIG_SPI_ETHERNET_CS),
            gpio_portname(CONFIG_SPI_ETHERNET_INT), gpio_name(CONFIG_SPI_ETHERNET_INT),
            CONFIG_SPI_ETHERNET_SPEED / 1000000);

    // Initialize RX tracking variables and other control state flags
    emac->next_packet_pointer = ENC424J600_RXSTART;

    // Set up TX/RX/UDA buffer addresses
    write_reg(emac, ETXST, ENC424J600_TXSTART);
    write_reg(emac, ERXST, ENC424J600_RXSTART);
    write_reg(emac, ERXTAIL, ENC424J600_RAMSIZE - 2);
    write_reg(emac, EUDAST, ENC424J600_RAMSIZE);
    write_reg(emac, EUDAND, ENC424J600_RAMSIZE + 1);

    // TO DO: set mac address

    // Set PHY Auto-negotiation to support 10BaseT Half duplex,
    // 10BaseT Full duplex, 100BaseTX Half Duplex, 100BaseTX Full Duplex,
    // and symmetric PAUSE capability
    write_phy_reg(emac,
    		      PHANA,
                  PHANA_ADPAUS0 | PHANA_AD10FD | PHANA_AD10 | PHANA_AD100FD |
				  PHANA_AD100 | PHANA_ADIEEE0
    );


    eth->on_state_changed(eth, ETH_STATE_LLINIT, NULL);

    return ESP_OK;

out:
	gpio_isr_detach(CONFIG_SPI_ETHERNET_INT);
    eth->on_state_changed(eth, ETH_STATE_DEINIT, NULL);
	return ret;
}

static esp_err_t emac_enc424j600_deinit(esp_eth_mac_t *mac) {
    emac_enc424j600_t *emac = __containerof(mac, emac_enc424j600_t, parent);
    esp_eth_mediator_t *eth = emac->eth;

    mac->stop(mac);
	gpio_isr_detach(CONFIG_SPI_ETHERNET_INT);
    eth->on_state_changed(eth, ETH_STATE_DEINIT, NULL);

    return ESP_OK;
}

static esp_err_t emac_enc424j600_del(esp_eth_mac_t *mac) {
    emac_enc424j600_t *emac = __containerof(mac, emac_enc424j600_t, parent);

    if (emac->rx_task_hdl) {
    	vTaskDelete(emac->rx_task_hdl);
    	emac->rx_task_hdl = NULL;
    }

    if (emac->lock) {
		vSemaphoreDelete(emac->lock);
		emac->lock = NULL;
    }

    spi_unsetup(emac->spi_device);

    free(emac);

    return ESP_OK;
}

esp_eth_mac_t *esp_eth_mac_new_enc424j600(const eth_mac_config_t *mac_config) {
    esp_eth_mac_t *ret = NULL;
    emac_enc424j600_t *emac = NULL;
    MAC_CHECK(mac_config, "can't set mac config to null", err, NULL);
    emac = calloc(1, sizeof(emac_enc424j600_t));
    MAC_CHECK(emac, "calloc emac failed", err, NULL);

    // Configure SPI
    driver_error_t *error;

    if ((error = spi_setup(CONFIG_SPI_ETHERNET_SPI, 1, CONFIG_SPI_ETHERNET_CS, 0,
    		               CONFIG_SPI_ETHERNET_SPEED, SPI_FLAG_WRITE | SPI_FLAG_READ,
						   &emac->spi_device)))
    {
        syslog(LOG_ERR, "enc424j600 cannot open spi%d", CONFIG_SPI_ETHERNET_SPI);
        free(error);
        return NULL;
    }

    emac->last_bank = 0xFF;
    emac->next_packet_pointer = ENC424J600_RXSTART;
    /* bind methods and attributes */
    emac->sw_reset_timeout_ms = mac_config->sw_reset_timeout_ms;

    emac->parent.set_mediator = emac_enc424j600_set_mediator;
    emac->parent.init = emac_enc424j600_init;
    emac->parent.deinit = emac_enc424j600_deinit;
    emac->parent.start = emac_enc424j600_start;
    emac->parent.stop = emac_enc424j600_stop;
    emac->parent.del = emac_enc424j600_del;
    emac->parent.write_phy_reg = emac_enc424j600_write_phy_reg;
    emac->parent.read_phy_reg = emac_enc424j600_read_phy_reg;
    emac->parent.set_addr = emac_enc424j600_set_addr;
    emac->parent.get_addr = emac_enc424j600_get_addr;
    emac->parent.set_speed = emac_enc424j600_set_speed;
    emac->parent.set_duplex = emac_enc424j600_set_duplex;
    emac->parent.set_link = emac_enc424j600_set_link;
    emac->parent.set_promiscuous = emac_enc424j600_set_promiscuous;
    emac->parent.transmit = emac_enc424j600_transmit;
    emac->parent.receive = emac_enc424j600_receive;

    // Create lock
    emac->lock = xSemaphoreCreateMutex();

    /* create enc424j600 task */
    BaseType_t core_num = tskNO_AFFINITY;
    if (mac_config->flags & ETH_MAC_FLAG_PIN_TO_CORE) {
        core_num = esp_cpu_get_core_id();
    }

    BaseType_t xReturned = xTaskCreatePinnedToCore(emac_enc424j600_task, "enc424j600_tsk", mac_config->rx_task_stack_size, emac,
                           mac_config->rx_task_prio, &emac->rx_task_hdl, core_num);
    MAC_CHECK(xReturned == pdPASS, "create enc424j600 task failed", err, NULL);

    return &(emac->parent);

err:
    if (emac) {
        if (emac->rx_task_hdl) {
            vTaskDelete(emac->rx_task_hdl);
        }
        free(emac);
    }
    return ret;
}
