/*
 * Copyright (C) 2015 - 2018, IBEROXARXA SERVICIOS INTEGRALES, S.L.
 * Copyright (C) 2015 - 2018, Jaume Olivé Petrus (jolive@whitecatboard.org)
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
 * Lua RTOS, simple channel LoRa WAN gateway
 *
 * Main idea extracted from https://github.com/things4u/ESP-1ch-Gateway-v5.0 and
 * IBM LMIC LoRa stack.
 *
 * Packet forwarder protocol:
 *
 * https://github.com/Lora-net/packet_forwarder/blob/master/PROTOCOL.TXT
 *
 */

#include "sdkconfig.h"

#if CONFIG_LUA_RTOS_LORA_HW_TYPE_SX1276

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "lora.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "lwip/ip_addr.h"
#include "lwip/ip.h"

#include "gateway.h"
#include "base64.h"

#include <time.h>
#include <stdio.h>
#include <string.h>

#include <sys/delay.h>

#include <drivers/net.h>
#include <drivers/wifi.h>

// Signal band width
#define LORA_SBW_7_8   (0b0000 << 4) // 7.8 kHz
#define LORA_SBW_10_4  (0b0001 << 4) // 10.4 kHz
#define LORA_SBW_15_6  (0b0010 << 4) // 15.6 kHz
#define LORA_SBW_20_8  (0b0011 << 4) // 20.8kHz
#define LORA_SBW_31_25 (0b0100 << 4) // 31.25 kHz
#define LORA_SBW_41_7  (0b0101 << 4) // 41.7 kHz
#define LORA_SBW_62_5  (0b0110 << 4) // 62.5 kHz
#define LORA_SBW_125   (0b0111 << 4) // 125 kHz
#define LORA_SBW_250   (0b1000 << 4) // 250 kHz
#define LORA_SBW_500   (0b1001 << 4) // 500 kHz

// Coding rates
#define LORA_CR_4_5    (0b001 << 1)  // 4/5
#define LORA_CR_4_6    (0b010 << 1)  // 4/6
#define LORA_CR_4_7    (0b011 << 1)  // 4/7
#define LORA_CR_4_8    (0b100 << 1)  // 4/8

#define SOCKET_TIMEOUT_MS   200

#define _localtime(t,r)      ((void)(r)->tm_sec, localtime(t))

// Statistics
static uint32_t rxnb = 0; // Number of radio packets received
static uint32_t rxok = 0; // Number of radio packets received with a valid PHY CRC
static uint32_t rxfw = 0; // Number of radio packets forwarded
static float    ackr = 0; // Percentage of upstream datagrams that were acknowledged
static uint32_t dwnb = 0; // Number of downlink datagrams received
static uint32_t txnb = 0; // Number of packets emitted

typedef enum {
    LoraGWModeRX = 0
} lora_gw_mode_t;

// Current gateway mode
static lora_gw_mode_t mode;

// LoRA WAN data when received or when transmitted
typedef struct {
    uint8_t freq_idx;     // Frequency index
    uint8_t sf_idx;       // Spreading factor index
    uint8_t rssi;         // RSSI
    uint8_t snr;          // SNR
    uint8_t size;         // Payload's size
    uint8_t payload[256]; // Payload
} lora_data_t;

static int spi_device;                         // SPI device where phy is attached
static xQueueHandle lora_rx_q = NULL;          // LoRa WAN data queue
static TaskHandle_t lora_ttn_up_task = NULL;   // TTN upload task
static TaskHandle_t lora_ttn_down_task = NULL; // TTN download task

// Upstream / downstream socket
static int up_socket;
static int down_socket;

// Server address
static struct sockaddr_in sever_addr;

static uint8_t sf_idx    = 0;
static uint8_t freq_idx  = 0;

#if CONFIG_LUA_RTOS_LORA_BAND_EU868
static const uint8_t sf[] = {7, 8, 9, 10, 11, 12};
static const uint32_t freq[] = { 868100000, 868300000, 868500000, 867100000, 867300000, 867500000, 867700000, 867900000, 868800000 };
#endif

#if CONFIG_LUA_RTOS_LORA_BAND_US915
#error "Not supported US915"
#endif


static uint8_t gw_eui[8];     // Gateway EUI

/*
 * Helper functions
 */

static uint8_t get_version() {
    uint8_t data;

    stx1276_read_reg(spi_device, SX1276_REG_VERSION, &data);

    return data;
}

static uint8_t get_snr() {
    uint8_t data;

    stx1276_read_reg(spi_device, SX1276_REG_PKT_SNR_VALUE, &data);

    return data;
}

static float compute_snr(uint8_t data) {
    if(data & 0x80) {
        // The SNR sign bit is 1, invert and divide by 4
        data = ((~data + 1) & 0xff);
        return -data / 4.0f;
    } else {
        // Divide by 4
        return (data & 0xFF) / 4.0f;
    }
}

static uint8_t get_rssi() {
    uint8_t data;

    stx1276_read_reg(spi_device, SX1276_REG_PKT_RSSI_VALUE, &data);

    return data;
}

static int16_t compute_rssi(uint8_t data) {
    return -137 + data;
}

static uint8_t get_payload(uint8_t *payload) {
    uint8_t rx_addr;
    uint8_t size;

    stx1276_read_reg(spi_device, SX1276_REG_FIFO_RX_CURRENT_ADDR, &rx_addr);
    stx1276_read_reg(spi_device, SX1276_REG_RX_NB_BYTES, &size);

    stx1276_write_reg(spi_device, SX1276_REG_FIFO_ADDR_PTR, rx_addr);

    if (size > 0) {
        stx1276_read_buff(spi_device, SX1276_REG_FIFO, payload, size);
    }

    return size;
}

static void set_mode(uint8_t mode) {
    if (mode == OPMODE_LORA) {
        stx1276_write_reg(spi_device, SX1276_REG_OP_MODE, mode);
    } else {
        uint8_t data;

        stx1276_read_reg(spi_device, SX1276_REG_OP_MODE, &data);
        stx1276_write_reg(spi_device, SX1276_REG_OP_MODE, (data & ~OPMODE_MASK) | mode);
    }
}

static uint8_t get_irq_flags() {
    uint8_t data;

    stx1276_read_reg(spi_device, SX1276_REG_IRQ_FLAGS, &data);

    return data;
}

static void clear_irq_flags(uint8_t flags) {
    stx1276_write_reg(spi_device, SX1276_REG_IRQ_FLAGS, flags);
}

static void set_freq(uint8_t freq_idx) {
    uint64_t frf = (((uint64_t)freq[freq_idx]) << 19) / 32000000;

    stx1276_write_reg(spi_device, SX1276_REG_FR_MSB, (uint8_t)(frf >> 16));
    stx1276_write_reg(spi_device, SX1276_REG_FR_MID, (uint8_t)(frf >> 8));
    stx1276_write_reg(spi_device, SX1276_REG_FR_LSB, (uint8_t)(frf >> 0));
}

static void set_rate(uint8_t sf_idx, uint8_t crc) {
    uint8_t mc1 = 0, mc2 = 0, mc3 = 0;

    if (sf[sf_idx] == 6) {
        mc1 = LORA_SBW_250 | LORA_CR_4_5;
    } else {
        mc1 = LORA_SBW_125 | LORA_CR_4_5;
    }

    mc2 = ((sf[sf_idx] << 4) | crc) & 0xff;
    mc3 = 0x04;

    if (sf[sf_idx] == 11 || sf[sf_idx] == 12) {
        mc3 |= 0x08;
    }

    stx1276_write_reg(spi_device, SX1276_REG_MODEM_CONFIG_1, mc1);
    stx1276_write_reg(spi_device, SX1276_REG_MODEM_CONFIG_2, mc2);
    stx1276_write_reg(spi_device, SX1276_REG_MODEM_CONFIG_3, mc3);

    if (sf[sf_idx] >= 10) {
        stx1276_write_reg(spi_device, SX1276_REG_SYMB_TIMEOUT_LSB, 0x05);
    } else {
        stx1276_write_reg(spi_device, SX1276_REG_SYMB_TIMEOUT_LSB, 0x08);
    }
}

static void modem_init() {
    uint8_t data;

    // Put transceiver in LORA mode
    set_mode(OPMODE_SLEEP);
    set_mode(OPMODE_LORA);

    // Set frequency
    set_freq(freq_idx);

    // Set initial spreading factor
    set_rate(sf_idx, 0x04);

    // Listen to LoRa preamble
    stx1276_write_reg(spi_device, SX1276_REG_SYNC_WORD, 0x34);

    // Prevent node to node communication
    stx1276_write_reg(spi_device, SX1276_REG_INVERT_IQ, 0x27);

    // Payload length
    stx1276_write_reg(spi_device, SX1276_REG_PAYLOAD_LENGTH, 0x40);
    stx1276_write_reg(spi_device, SX1276_REG_MAX_PAYLOAD_LENGTH, 0x80);

    //Set the start address for the FiFO (Which should be 0)
    stx1276_read_reg(spi_device, SX1276_REG_FIFO_RX_BASE_ADDR, &data);
    stx1276_write_reg(spi_device, SX1276_REG_FIFO_ADDR_PTR, data);

    // Use max LNA gain in receiver
    stx1276_write_reg(spi_device, SX1276_REG_LNA, 0x23);

    // No HOP
    stx1276_write_reg(spi_device, SX1276_REG_HOP_PERIOD, 0x00);
}

static void rx_mode() {
    // Set new mode
    mode = LoraGWModeRX;

    // Set DIO mapping and interrupts we want to listen to
    stx1276_write_reg(spi_device, SX1276_REG_DIO_MAPPING_1, MAP_DIO0_LORA_RXDONE | MAP_DIO1_LORA_NOP | MAP_DIO2_LORA_NOP | MAP_DIO3_LORA_NOP);

    // Mask desired interrupts
    stx1276_write_reg(spi_device, SX1276_REG_IRQ_FLAGS_MASK, (~(IRQ_LORA_RXDONE_MASK | IRQ_LORA_CRCERR_MASK)) & 0xff);

    // Clear all interrupts
    clear_irq_flags(0xff);

    // Enable RX mode
    set_mode(OPMODE_RX);
}

/*
 * DIO ISR
 */
static void dio_intr_handler(void* arg) {
    portBASE_TYPE high_priority_task_awoken = 0;

    // Get IRQ flags to decide what to do
    uint8_t flags = get_irq_flags();

    // If no flags, exit
    if (!flags) return;

    if (mode == LoraGWModeRX) {
        if (flags & (IRQ_LORA_RXDONE_MASK)) {
            // Update statistics
            rxnb++;

            // Check for CRC
            if (!(flags & IRQ_LORA_CRCERR_MASK)) {
                // Update statistics
                rxok++;

                // Packet received, and valid, read it!
                lora_data_t data;

                data.freq_idx = freq_idx;
                data.sf_idx = sf_idx;
                data.snr = get_snr();
                data.rssi = get_rssi();
                data.size = get_payload(&data.payload[0]);

                // Clear all interrupts
                clear_irq_flags(0xff);

                // Send received packet data to RX queue for later processing
                xQueueSendFromISR(lora_rx_q, &data, &high_priority_task_awoken);
            } else {
                // Clear all interrupts
                clear_irq_flags(0xff);
            }
        }
    }

    if (high_priority_task_awoken == pdTRUE) {
        portYIELD_FROM_ISR();
    }
}

static uint8_t ttn_wait_ack(uint8_t packet, uint8_t t1, uint8_t t2) {
    uint8_t buffer[4];
    int len;
    int retry;

    for(retry = 0; retry < 3;retry++) {
        len = recvfrom(up_socket, buffer, sizeof(buffer), 0, NULL, NULL);
        if (len > 0) {
            // Response received

            if ((buffer[0] == 0x01) && (buffer[1] == t1) && (buffer[2] == t2) && (buffer[3] == packet)) {
                // Received
                if (packet == 0x01) {
                    syslog(LOG_DEBUG, "lora gw: <- PUSH_ACK");
                }

                return 1;
            }
        }
    }

    if (packet == 0x01) {
        syslog(LOG_DEBUG, "lora gw: <- PUSH_ACK not received");
    }

    return 0;
}

/*
 *  This task implements the upstream protocol:
 *
 *  1) If a new packet is available into rx queue, create a rxpk message and send it to TTN.
 *
 *  2) If a new packet is not available within 30 seconds, send a stat message to TTN.
 *
 */
static void ttn_up_task(void *arg) {
    uint8_t buffer[12 + 512]; // Buffer to build TTN message, coded in a JSON object
    char b64_payload[350];    // BUffer to encode payload into base 64
    lora_data_t rx_data;      // RX data
    uint8_t  token1;
    uint8_t  token2;
    uint64_t last_stat = 0;

    char *buff_pos;

    for(;;) {
        time_t t = time(NULL);
        struct tm tmr, *stm;
        stm = _localtime(&t, &tmr);

        buff_pos = (char *)&buffer[12];
        *buff_pos = 0x00;

        // Wait for data a maximum of 1 second
        if (xQueueReceive(lora_rx_q, &rx_data, 1000 / portTICK_PERIOD_MS) == pdTRUE) {
            // We have new data

            // Encode payload in base 64
            bin_to_b64(rx_data.payload, rx_data.size, b64_payload, sizeof(b64_payload));

            // Build JSON
            sprintf(
                buff_pos,
                "{\"rxpk\":[{\"time\":\"%04d-%02d-%02dT%02d:%02d:%02d.00000Z\",\"tmst\":%u,\"chan\":%u,\"rfch\":%u,\"freq\":%.5lf,\"stat\":%u,\"modu\":\"%s\",\"datr\":\"SF%uBW125\",\"codr\":\"%s\",\"rssi\":%d,\"lsnr\":%.1f,\"size\":%u,\"data\":\"%s\"}]}",
                (stm->tm_year + 1900),                  /* UTC time of pkt RX, us precision, ISO 8601 'compact' format */
                (stm->tm_mon + 1),                      /* UTC time of pkt RX, us precision, ISO 8601 'compact' format */
                stm->tm_mday,                           /* UTC time of pkt RX, us precision, ISO 8601 'compact' format */
                stm->tm_hour,                           /* UTC time of pkt RX, us precision, ISO 8601 'compact' format */
                stm->tm_min,                            /* UTC time of pkt RX, us precision, ISO 8601 'compact' format */
                stm->tm_sec,                            /* UTC time of pkt RX, us precision, ISO 8601 'compact' format */
                (uint32_t)time(NULL),                   /* Internal timestamp of "RX finished" event (32b unsigned) */
                rx_data.freq_idx,                       /* Concentrator "IF" channel used for RX (unsigned integer) */
                0,                                      /* Concentrator "RF chain" used for RX (unsigned integer) */
                (double)freq[freq_idx]/1000000,         /* RX central frequency in MHz (unsigned float, Hz precision) */
                1,                                      /* CRC status: 1 = OK, -1 = fail, 0 = no CRC */
                "LORA",                                 /* Modulation identifier "LORA" or "FSK" */
                sf[rx_data.sf_idx],                     /* LoRa datarate identifier (eg. SF12BW500) */
                "4/5",                                  /* LoRa ECC coding rate identifier */
                compute_rssi(rx_data.rssi),             /* RSSI in dBm (signed integer, 1 dB precision) */
                compute_snr(rx_data.snr),               /* Lora SNR ratio in dB (signed float, 0.1 dB precision) */
                rx_data.size,                           /* RF packet payload size in bytes (unsigned integer) */
                b64_payload                             /* Base64 encoded RF packet payload, padded */
            );

            buff_pos += strlen(buff_pos) - 1;
        }

        // Send stats every 30 seconds +/- 1 sec
        struct timeval now;
        gettimeofday(&now, NULL);
        uint64_t now_ms = (now.tv_sec * 1000 + now.tv_usec / 1000);

        if ((now_ms - last_stat) > 30000) {
            last_stat = now_ms;

            // Append stats to data?
            uint8_t append = 0;

            if (*buff_pos == '}') {
                *(buff_pos++) = ',';
                append = 1;
            }

            if (!append) {
                *(buff_pos++) = '{';
            }

            sprintf(buff_pos, "\"stat\":{\"time\":\"%04d-%02d-%02d %02d:%02d:%02d GMT\",\"lati\":%.5f,\"long\":%.5f,\"alti\":%d,\"rxnb\":%u,\"rxok\":%u,\"rxfw\":%u,\"ackr\":%f,\"dwnb\":%u,\"txnb\":%u}}",
            (stm->tm_year + 1900),              /* UTC 'system' time of the gateway, ISO 8601 'expanded' format */
            (stm->tm_mon + 1),                  /* UTC 'system' time of the gateway, ISO 8601 'expanded' format */
            stm->tm_mday,                       /* UTC 'system' time of the gateway, ISO 8601 'expanded' format */
            stm->tm_hour,                       /* UTC 'system' time of the gateway, ISO 8601 'expanded' format */
            stm->tm_min,                        /* UTC 'system' time of the gateway, ISO 8601 'expanded' format */
            stm->tm_sec,                        /* UTC 'system' time of the gateway, ISO 8601 'expanded' format */
            0.0,                                /* GPS latitude of the gateway in degree (float, N is +) */
            0.0,                                /* GPS latitude of the gateway in degree (float, E is +) */
            0,                                  /* GPS altitude of the gateway in meter RX (integer) */
            rxnb,                               /* Number of radio packets received (unsigned integer) */
            rxok,                               /* Number of radio packets received with a valid PHY CRC */
            rxfw,                               /* Number of radio packets forwarded (unsigned integer) */
            ackr,                               /* Percentage of upstream datagrams that were acknowledged */
            dwnb,                               /* Number of downlink datagrams received (unsigned integer) */
            txnb);                              /* Number of packets emitted (unsigned integer) */
        }

        if (buffer[12] != 0x00) {
            token1 = (uint8_t)rand();
            token2 = (uint8_t)rand();

            buffer[0]  = 0x01;      /* protocol version = 1 */
            buffer[1]  = token1;    /* random token */
            buffer[2]  = token2;    /* random token */
            buffer[3]  = 0x00;      /* PUSH_DATA identifier 0x00 */
            buffer[4]  = gw_eui[0]; /* Gateway unique identifier (MAC address) */
            buffer[5]  = gw_eui[1]; /* Gateway unique identifier (MAC address) */
            buffer[6]  = gw_eui[2]; /* Gateway unique identifier (MAC address) */
            buffer[7]  = gw_eui[3]; /* Gateway unique identifier (MAC address) */
            buffer[8]  = gw_eui[4]; /* Gateway unique identifier (MAC address) */
            buffer[9]  = gw_eui[5]; /* Gateway unique identifier (MAC address) */
            buffer[10] = gw_eui[6]; /* Gateway unique identifier (MAC address) */
            buffer[11] = gw_eui[7]; /* Gateway unique identifier (MAC address) */

            syslog(LOG_DEBUG, "lora gw: -> PUSH_DATA");

            // Send packet to TTN
            int len = strlen((char *)&buffer[12]) + 12;
            if ((sendto(up_socket, buffer, len, 0, (struct sockaddr *)&sever_addr, sizeof(sever_addr)) == len)) {
                // Wait ACK
                if (ttn_wait_ack(0x01, token1, token2)) {
                    // Update statistics
                    rxfw++;
                }
            }
        }
    }
}

/*
 *  This task implements the downstream protocol:
 *
 *  1) If a new packet is available into rx queue, create a rxpk message and send it to TTN.
 *
 *  2) If a new packet is not available within 30 seconds, send a stat message to TTN.
 *
 */
static void ttn_down_task(void *arg) {
    int len;
    uint8_t buffer[12 + 512];
    uint8_t  token1;
    uint8_t  token2;
    uint8_t  alive_token1;
    uint8_t  alive_token2;
    uint64_t last_alive = 0;

    for(;;) {
        // Compute elapsed time since last alive message
        struct timeval now;
        gettimeofday(&now, NULL);
        uint64_t now_ms = (now.tv_sec * 1000 + now.tv_usec / 1000);

        if ((now_ms - last_alive) > 30000) {
            // Send an alive message

            // Record time
            last_alive = now_ms;

            alive_token1 = (uint8_t)rand();
            alive_token2 = (uint8_t)rand();

            buffer[0]  = 0x01;         /* protocol version = 1 */
            buffer[1]  = alive_token1; /* random token */
            buffer[2]  = alive_token2; /* random token */
            buffer[3]  = 0x02;         /* PULL_DATA identifier 0x02 */
            buffer[4]  = gw_eui[0];    /* Gateway unique identifier (MAC address) */
            buffer[5]  = gw_eui[1];    /* Gateway unique identifier (MAC address) */
            buffer[6]  = gw_eui[2];    /* Gateway unique identifier (MAC address) */
            buffer[7]  = gw_eui[3];    /* Gateway unique identifier (MAC address) */
            buffer[8]  = gw_eui[4];    /* Gateway unique identifier (MAC address) */
            buffer[9]  = gw_eui[5];    /* Gateway unique identifier (MAC address) */
            buffer[10] = gw_eui[6];    /* Gateway unique identifier (MAC address) */
            buffer[11] = gw_eui[7];    /* Gateway unique identifier (MAC address) */

            sendto(down_socket, buffer, 12, 0, (struct sockaddr *)&sever_addr, sizeof(sever_addr));
            syslog(LOG_DEBUG, "lora gw: -> PULL_DATA");
        }

        // Receive any packet
        memset(buffer, 0, sizeof(buffer));
        len = recvfrom(down_socket, buffer, sizeof(buffer), 0, NULL, NULL);
        if (len > 0) {
            // Packet received
            switch (buffer[3]) {
                case 0x03: // PULL_RESP
                    syslog(LOG_DEBUG, "lora gw: <- PULL_RESP");
                    break;

                case 0x04: // PULL_ACK
                    if ((buffer[1] == alive_token1) && (buffer[2] == alive_token2)) {
                        syslog(LOG_DEBUG, "lora gw: <- PULL_ACK");
                    } else {
                        syslog(LOG_DEBUG, "lora gw: <- PULL_ACK unexpected");
                    }
                    break;
            }
        }
    }

#if 0

            // Send TX_ACK
            buffer[0]  = 0x01;      /* protocol version = 1 */
            buffer[1]  = token1;    /* random token */
            buffer[2]  = token2;    /* random token */
            buffer[3]  = 0x05;      /* TX_ACK identifier 0x05 */
            buffer[4]  = gw_eui[0]; /* Gateway unique identifier (MAC address) */
            buffer[5]  = gw_eui[1]; /* Gateway unique identifier (MAC address) */
            buffer[6]  = gw_eui[2]; /* Gateway unique identifier (MAC address) */
            buffer[7]  = gw_eui[3]; /* Gateway unique identifier (MAC address) */
            buffer[8]  = gw_eui[4]; /* Gateway unique identifier (MAC address) */
            buffer[9]  = gw_eui[5]; /* Gateway unique identifier (MAC address) */
            buffer[10] = gw_eui[6]; /* Gateway unique identifier (MAC address) */
            buffer[11] = gw_eui[7]; /* Gateway unique identifier (MAC address) */
            buffer[12] = 0x00;

            sendto(down_socket, buffer, 12, 0, (struct sockaddr *)&sever_addr, sizeof(sever_addr));

            syslog(LOG_DEBUG, "lora gw: -> TX_ACK (OK)");
#endif

}

/*
 * Operation functions
 *
 */
#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
driver_error_t *lora_gw_lock_resources(int unit) {
    driver_unit_lock_error_t *lock_error = NULL;

    if ((lock_error = driver_lock(LORA_DRIVER, unit, SPI_DRIVER, CONFIG_LUA_RTOS_LORA_SPI, DRIVER_ALL_FLAGS, NULL))) {
        // Revoked lock on pin
        return driver_lock_error(LORA_DRIVER, lock_error);
    }

    #if ((CONFIG_LUA_RTOS_POWER_BUS_PIN == -1) && (CONFIG_LUA_RTOS_LORA_RST >= 0))
    if ((lock_error = driver_lock(LORA_DRIVER, unit, GPIO_DRIVER, CONFIG_LUA_RTOS_LORA_RST, DRIVER_ALL_FLAGS, "RST"))) {
        // Revoked lock on pin
        return driver_lock_error(LORA_DRIVER, lock_error);
    }
    #endif

    #if CONFIG_LUA_RTOS_LORA_DIO0 >= 0
    if ((lock_error = driver_lock(LORA_DRIVER, unit, GPIO_DRIVER, CONFIG_LUA_RTOS_LORA_DIO0, DRIVER_ALL_FLAGS, "DIO0"))) {
        // Revoked lock on pin
        return driver_lock_error(LORA_DRIVER, lock_error);
    }
    #endif

    #if CONFIG_LUA_RTOS_LORA_DIO1 >= 0
    if ((lock_error = driver_lock(LORA_DRIVER, unit, GPIO_DRIVER, CONFIG_LUA_RTOS_LORA_DIO1, DRIVER_ALL_FLAGS, "DIO1"))) {
        // Revoked lock on pin
        return driver_lock_error(LORA_DRIVER, lock_error);
    }
    #endif

    #if CONFIG_LUA_RTOS_LORA_DIO2 >= 0
    if ((lock_error = driver_lock(LORA_DRIVER, unit, GPIO_DRIVER, CONFIG_LUA_RTOS_LORA_DIO2, DRIVER_ALL_FLAGS, "DIO2"))) {
        // Revoked lock on pin
        return driver_lock_error(LORA_DRIVER, lock_error);
    }
    #endif

    return NULL;
}
#endif

driver_error_t *lora_gw_setup(int band, const char *host, int port, int frequency, int drate) {
    driver_error_t *error;

    syslog(LOG_INFO, "lora gw: starting");

    // Sanity checks
    #if CONFIG_LUA_RTOS_LORA_BAND_EU868

    // Band
    if (band != 868) {
        return driver_error(LORA_DRIVER, LORA_ERR_INVALID_BAND, NULL);
    }

    // Data Rate
    if (drate == 0)
        sf_idx = 5;
    else if (drate == 1)
        sf_idx = 4;
    else if (drate == 2)
        sf_idx = 3;
    else if (drate == 3)
        sf_idx = 2;
    else if (drate == 4)
        sf_idx = 1;
    else if (drate == 5)
        sf_idx = 0;
    else
        return driver_error(LORA_DRIVER, LORA_ERR_INVALID_DR, NULL);

    if (sf_idx + 1 > sizeof(sf))
        return driver_error(LORA_DRIVER, LORA_ERR_INVALID_DR, NULL);

    // Frequency
    int i;
    for(i=0;i<sizeof(freq);i++) {
        if (freq[i] == frequency) {
            freq_idx = i;
            break;
        }
    }

    if (i >= sizeof(freq)) {
        return driver_error(LORA_DRIVER, LORA_ERR_INVALID_FREQ, NULL);
    }

    #endif

    #if CONFIG_LUA_RTOS_LORA_BAND_US915
    if (band != 915) {
        return driver_error(LORA_DRIVER, LORA_ERR_INVALID_BAND, NULL);
    }
    #endif

    syslog(LOG_INFO, "lora gw: dr=%d, sf=%d, freq=%.1f MHz", drate, sf[sf_idx], (((double)freq[freq_idx])/(double)1000000));

    /*
     * Hardware setup
     *
     */

    // Init SPI bus
    if ((error = spi_setup(CONFIG_LUA_RTOS_LORA_SPI, 1, CONFIG_LUA_RTOS_LORA_CS, 0, 10000000, SPI_FLAG_WRITE | SPI_FLAG_READ | SPI_FLAG_NO_DMA, &spi_device))) {
        lora_gw_unsetup();
        return error;
    }

#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
    // Lock pins
    if ((error = lora_gw_lock_resources(0))) {
        lora_gw_unsetup();
        return error;
    }
#endif

    syslog(LOG_INFO, "lora gw: single gateway at spi%d, pin cs=%s%d", CONFIG_LUA_RTOS_LORA_SPI,
        gpio_portname(CONFIG_LUA_RTOS_LORA_CS), gpio_name(CONFIG_LUA_RTOS_LORA_CS)
    );

    // Create queues, if not created yet
    if (!lora_rx_q) {
        lora_rx_q = xQueueCreate(100, sizeof(lora_data_t));
        if (!lora_rx_q) {
            lora_gw_unsetup();
            return driver_error(LORA_DRIVER, LORA_ERR_NO_MEM, NULL);
        }
    }

    // Attach DIO interrupt handlers
    #if CONFIG_LUA_RTOS_LORA_DIO0 >= 0
    if ((error = gpio_isr_attach(CONFIG_LUA_RTOS_LORA_DIO0, dio_intr_handler, GPIO_INTR_POSEDGE, (void*)0))) {
        error->msg = "DIO0";
        lora_gw_unsetup();
        return error;
    }
    syslog(LOG_INFO, "lora gw: DIO0 IRQ attached, at pin %s%d", gpio_portname(CONFIG_LUA_RTOS_LORA_DIO0), gpio_name(CONFIG_LUA_RTOS_LORA_DIO0));
    #endif

    #if CONFIG_LUA_RTOS_LORA_DIO1 >= 0
    if ((error = gpio_isr_attach(CONFIG_LUA_RTOS_LORA_DIO1, dio_intr_handler, GPIO_INTR_POSEDGE, (void*)1))) {
        error->msg = "DIO1";
        lora_gw_unsetup();
        return error;
    }
    syslog(LOG_INFO, "lora gw: DIO1 IRQ attached, at pin %s%d", gpio_portname(CONFIG_LUA_RTOS_LORA_DIO1), gpio_name(CONFIG_LUA_RTOS_LORA_DIO1));
    #endif

    #if CONFIG_LUA_RTOS_LORA_DIO2 >= 0
    if ((error = gpio_isr_attach(CONFIG_LUA_RTOS_LORA_DIO2, dio_intr_handler, GPIO_INTR_POSEDGE, (void*)2))) {
        error->msg = "DIO2";
        lora_gw_unsetup();
        return error;
    }
    syslog(LOG_INFO, "lora gw: DIO2 IRQ attached, at pin %s%d", gpio_portname(CONFIG_LUA_RTOS_LORA_DIO2), gpio_name(CONFIG_LUA_RTOS_LORA_DIO2));
    #endif

    #if ((CONFIG_LUA_RTOS_POWER_BUS_PIN == -1) && (CONFIG_LUA_RTOS_LORA_RST >= 0))
    // Init RESET pin
    gpio_pin_output(CONFIG_LUA_RTOS_LORA_RST);
    #endif

    /*
     * Network setup
     *
     */

    // Wait for network
    syslog(LOG_INFO, "lora gw: waiting for network");

    if (!wait_for_network(20000)) {
        lora_gw_unsetup();
        return driver_error(LORA_DRIVER, LORA_ERR_CANT_SETUP, "no network connection available");
    }

    syslog(LOG_INFO, "lora gw: network available");

    // Get MAC
    uint8_t mac[6];

    if ((error = wifi_get_mac(mac))) {
        free(error);

        lora_gw_unsetup();
        return driver_error(LORA_DRIVER, LORA_ERR_CANT_SETUP, "can't get MAC address");
    }

    // Get GW EUI, based on MAC address
    gw_eui[0] = mac[0];
    gw_eui[1] = mac[1];
    gw_eui[2] = mac[2];
    gw_eui[3] = 0xff;
    gw_eui[4] = 0xfe;
    gw_eui[5] = mac[3];
    gw_eui[6] = mac[4];
    gw_eui[7] = mac[5];

    // Resolve name
    if ((error = net_lookup(host, port, &sever_addr))) {
        free(error);

        lora_gw_unsetup();

        return driver_error(LORA_DRIVER, LORA_ERR_CANT_SETUP, "can't resolve network coordinator hostname / port");
    }

    syslog(LOG_INFO, "lora gw: network coordinator set to %s:%d", host, port);

    // Create sockets
    if ((up_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        lora_gw_unsetup();
        return driver_error(LORA_DRIVER, LORA_ERR_CANT_SETUP, "can't create up socket");
    }

    if ((down_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        lora_gw_unsetup();
        return driver_error(LORA_DRIVER, LORA_ERR_CANT_SETUP, "can't create down socket");
    }

    // configure socket timeouts
    static struct timeval socket_timeout = {0, (SOCKET_TIMEOUT_MS * 1000)};

    if (setsockopt(up_socket, SOL_SOCKET, SO_RCVTIMEO, (void *)&socket_timeout, sizeof socket_timeout) < 0) {
        lora_gw_unsetup();
        return driver_error(LORA_DRIVER, LORA_ERR_CANT_SETUP, "can't configure up socket");
    }

    if (setsockopt(down_socket, SOL_SOCKET, SO_RCVTIMEO, (void *)&socket_timeout, sizeof socket_timeout) < 0) {
        lora_gw_unsetup();
        return driver_error(LORA_DRIVER, LORA_ERR_CANT_SETUP, "can't configure down socket");
    }

    syslog(LOG_INFO, "lora gw: gateway EUI %02x%02x%02x%02x%02x%02x%02x%02x",
        gw_eui[0],gw_eui[1],gw_eui[2],gw_eui[3],gw_eui[4],gw_eui[5], gw_eui[6], gw_eui[7]
    );


    // Reset transceiver
    sx1276_reset(0);
    udelay(100);
    sx1276_reset(2);
    delay(5);

    set_mode(OPMODE_SLEEP);

    // Check transceiver version
    if (get_version() == 0x12) {
        // Create tasks, if not created yet
        if (!lora_ttn_up_task) {
            BaseType_t xReturn;

            xReturn = xTaskCreatePinnedToCore(ttn_up_task, "lorattnu", CONFIG_LUA_RTOS_LUA_THREAD_STACK_SIZE, NULL, CONFIG_LUA_RTOS_LUA_THREAD_PRIORITY, &lora_ttn_up_task, xPortGetCoreID());
            if (xReturn != pdPASS) {
                lora_gw_unsetup();
                return driver_error(LORA_DRIVER, LORA_ERR_NO_MEM, NULL);
            }
        }

        if (!lora_ttn_down_task) {
            BaseType_t xReturn;

            xReturn = xTaskCreatePinnedToCore(ttn_down_task, "lorattnd", CONFIG_LUA_RTOS_LUA_THREAD_STACK_SIZE, NULL, CONFIG_LUA_RTOS_LUA_THREAD_PRIORITY, &lora_ttn_down_task, xPortGetCoreID());
            if (xReturn != pdPASS) {
                lora_gw_unsetup();
                return driver_error(LORA_DRIVER, LORA_ERR_NO_MEM, NULL);
            }
        }
        modem_init();
        rx_mode();
    } else {
        lora_gw_unsetup();
        return driver_error(LORA_DRIVER, LORA_ERR_CANT_SETUP, "radio transceiver not detected, or not supported");
    }

    syslog(LOG_INFO, "lora gw: started");

    return NULL;
}

void lora_gw_unsetup() {
    if (lora_ttn_up_task) {
        vTaskDelete(lora_ttn_up_task);
        lora_ttn_up_task = NULL;
    }

    if (lora_ttn_down_task) {
        vTaskDelete(lora_ttn_down_task);
        lora_ttn_down_task = NULL;
    }

    if (lora_rx_q) {
        vQueueDelete(lora_rx_q);
        lora_rx_q = NULL;
    }
}

#endif
