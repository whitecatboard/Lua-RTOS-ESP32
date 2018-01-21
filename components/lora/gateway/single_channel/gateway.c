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
 */

#include "sdkconfig.h"

#if CONFIG_LUA_RTOS_LORA_HW_TYPE_SX1276 || CONFIG_LUA_RTOS_LORA_HW_TYPE_SX1272

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

#define _localtime(t,r)  	((void)(r)->tm_sec, localtime(t))

// LoRa WAN data from phy
typedef struct {
	uint8_t dio;
} lora_phy_data_t;

// LoRA WAN data from node
typedef struct {
	uint8_t payload[256];
	uint8_t size;
	uint8_t freq_idx;
	uint8_t sf_idx;
	int8_t rssi;
	float lsnr;
} lora_data_t;

static int spi_device;							// SPI device where phy is attached
static xQueueHandle lora_phy_queue  = NULL; 	// LoRa WAN phy queue
static xQueueHandle lora_data_queue = NULL; 	// LoRa WAN data queue
static TaskHandle_t lora_phy_task = NULL;       // LoRa WAN phy task
static TaskHandle_t lora_ttn_up_task = NULL;    // TTN upload task
static TaskHandle_t lora_ttn_timer_task = NULL; // TTN timer task

static struct sockaddr_in up_address;

static uint8_t sf_idx    = 0;
static uint8_t freq_idx  = 0;
static uint8_t sf[]      = { 7, 8, 9, 10, 11, 12 };
static uint32_t freq[]   = { 868100000, 868300000, 868500000, 867100000, 867300000, 867500000, 867700000, 867900000, 868800000 };

static uint8_t gw_eui[8];     // Gateway EUI

static int up_socket;

/*
 	 This is the ISR function attached to the PHY DIO pins.

 	 When a new LoRa frame is available the ISR is invoked and data is prepared for
 	 process the interrupt in a deferred function, using a queue.

 */
static void IRAM_ATTR dio_intr_handler(void* arg) {
	portBASE_TYPE high_priority_task_awoken = 0;
	lora_phy_data_t data;

	// Obtain DIO signal
	data.dio = (uint8_t)((uint32_t)arg);

	// Send deferred data
	xQueueSendFromISR(lora_phy_queue, &data, &high_priority_task_awoken);

    if (high_priority_task_awoken == pdTRUE) {
        portYIELD_FROM_ISR();
    }
}

/*
 	 This task is the deferred function for process the ISR function attached to the
 	 PHY DIO pins.

 	 The task waits for new data in a queue (queued in the ISR), gets the payload,
 	 calculates the rssi and the lsnr, and queues this data to a queue.

 	 Queued data is processed later in the ttn_up_task task that sends the data to TTN.

 */
static void phy_task(void *arg) {
	portBASE_TYPE high_priority_task_awoken = 0;
	lora_phy_data_t deferred;
	lora_data_t lora_data;

	uint8_t data;
    uint8_t rx_addr;

    for(;;) {
        xQueueReceive(lora_phy_queue, &deferred, portMAX_DELAY);

        // Clear rxDone
    	stx1276_write_reg(spi_device, SX1276_REG_IRQ_FLAGS, 0x40);

        // Read irq flags
    	stx1276_read_reg(spi_device, SX1276_REG_IRQ_FLAGS, &data);

        /*  payload crc: 0x20 */
        if ((data & 0x20) == 0x20) {
        	stx1276_write_reg(spi_device, SX1276_REG_IRQ_FLAGS, 0x20);
        } else {
            // Calculate SNR
        	stx1276_read_reg(spi_device, SX1276_REG_PKT_SNR_VALUE, &data);
            if( data & 0x80 ) { /* The SNR sign bit is 1 */
            	data = ( ( ~data + 1 ) & 0xFF ); /* Invert and divide by 4 */
              lora_data.lsnr = -data / 4.0f;
            } else {
              lora_data.lsnr = ( data & 0xFF ) / 4.0f; /* Divide by 4 */
            }

            // Read packet rssi
            stx1276_read_reg(spi_device, 0x1A, &data);
            lora_data.rssi = -137 + data;

            // Read payload
            stx1276_read_reg(spi_device, SX1276_REG_FIFO_RX_CURRENT_ADDR, &rx_addr);
            stx1276_read_reg(spi_device, SX1276_REG_RX_NB_BYTES, &lora_data.size);

        	stx1276_write_reg(spi_device, SX1276_REG_FIFO_ADDR_PTR, rx_addr);

        	if (lora_data.size > 0) {
				stx1276_read_buff(spi_device, SX1276_REG_FIFO, lora_data.payload, lora_data.size);

				lora_data.freq_idx = freq_idx;
				lora_data.sf_idx = sf_idx;

				xQueueSendFromISR(lora_data_queue, &lora_data, &high_priority_task_awoken);

			    if (high_priority_task_awoken == pdTRUE) {
			        portYIELD_FROM_ISR();
			    }
        	}
        }
    }
}

/*
 	 This task waits for new LoRa data received from a node, reading a queue. Previously whe data was queued
 	 by the phy_task task.

 	 When new LoRa data is received there are sent to TTN.

 */
static void ttn_up_task(void *arg) {
	driver_error_t *error;
	lora_data_t lora_data;
    char json_rxpk[512];
    char b64_payload[350];
    uint8_t buffer[12 + 512];

	for(;;) {
        xQueueReceive(lora_data_queue, &lora_data, portMAX_DELAY);

		// Compose json_rxpk
		time_t t = time(NULL);
		struct tm tmr, *stm;
		stm = _localtime(&t, &tmr);

		bin_to_b64(lora_data.payload, lora_data.size, b64_payload, sizeof(b64_payload));

		sprintf(
			json_rxpk,
			"{\"rxpk\":[{\"time\":\"%04d-%02d-%02dT%02d:%02d:%02d.00000Z\",\"tmst\":%u,\"chan\":%u,\"rfch\":%u,\"freq\":%.5lf,\"stat\":%u,\"modu\":\"%s\",\"datr\":\"SF%uBW125\",\"codr\":\"%s\",\"rssi\":%d,\"lsnr\":%.1f,\"size\":%u,\"data\":\"%s\"}]}\n",
			(stm->tm_year + 1900),                  /* UTC time of pkt RX, us precision, ISO 8601 'compact' format */
			(stm->tm_mon + 1),                      /* UTC time of pkt RX, us precision, ISO 8601 'compact' format */
			stm->tm_mday,                           /* UTC time of pkt RX, us precision, ISO 8601 'compact' format */
			stm->tm_hour,                           /* UTC time of pkt RX, us precision, ISO 8601 'compact' format */
			stm->tm_min,                            /* UTC time of pkt RX, us precision, ISO 8601 'compact' format */
			stm->tm_sec,                            /* UTC time of pkt RX, us precision, ISO 8601 'compact' format */
			(uint32_t)time(NULL),                   /* Internal timestamp of "RX finished" event (32b unsigned) */
			lora_data.freq_idx,                     /* Concentrator "IF" channel used for RX (unsigned integer) */
			0,                                      /* Concentrator "RF chain" used for RX (unsigned integer) */
			(double)freq[freq_idx]/1000000,         /* RX central frequency in MHz (unsigned float, Hz precision) */
			1,                                      /* CRC status: 1 = OK, -1 = fail, 0 = no CRC */
			"LORA",                                 /* Modulation identifier "LORA" or "FSK" */
			lora_data.sf_idx + 7,                   /* LoRa datarate identifier (eg. SF12BW500) */
			"4/5",                                  /* LoRa ECC coding rate identifier */
			lora_data.rssi,                         /* RSSI in dBm (signed integer, 1 dB precision) */
			lora_data.lsnr,                         /* Lora SNR ratio in dB (signed float, 0.1 dB precision) */
			lora_data.size,                         /* RF packet payload size in bytes (unsigned integer) */
			b64_payload								/* Base64 encoded RF packet payload, padded */
		);

		buffer[0]  = 0x01;			/* protocol version = 1 */
		buffer[1]  = (uint8_t)rand();	/* random token */
		buffer[2]  = (uint8_t)rand();	/* random token */
		buffer[3]  = 0x00;		    /* PUSH_DATA identifier 0x00 */
		buffer[4]  = gw_eui[0];		/* Gateway unique identifier (MAC address) */
		buffer[5]  = gw_eui[1];		/* Gateway unique identifier (MAC address) */
		buffer[6]  = gw_eui[2];		/* Gateway unique identifier (MAC address) */
		buffer[7]  = gw_eui[3];		/* Gateway unique identifier (MAC address) */
		buffer[8]  = gw_eui[4];		/* Gateway unique identifier (MAC address) */
		buffer[9]  = gw_eui[5];		/* Gateway unique identifier (MAC address) */
		buffer[10] = gw_eui[6];		/* Gateway unique identifier (MAC address) */
		buffer[11] = gw_eui[7];		/* Gateway unique identifier (MAC address) */

		/* JSON object, starting with {, ending with }, see section 4 */
		strcpy((char *)&buffer[12], json_rxpk);

		socklen_t slen = sizeof(up_address);

		// Send packet to TTN if network connection is available
		while ((error = net_check_connectivity())) {
			syslog(LOG_INFO, "lora gw: no network connection");

	    	free(error);

	    	if ((error = net_reconnect())) {
	    		free(error);
	    		continue;
	    	} else {
				syslog(LOG_INFO, "lora gw: network connection re-established");
	    	}
	    }

		sendto(up_socket, buffer, strlen((char *)&buffer[12]) + 12, 0, (struct sockaddr *)&up_address, slen);
	}
}

static void ttn_timer_task(void *arg) {
	driver_error_t *error;

    uint8_t rxnb = 0;
    uint8_t rxok = 0;
    uint8_t rxfw = 0;
    uint8_t ackr = 0;
    uint8_t dwnb = 0;
    uint8_t txnb = 0;

    char json_stat[512];
    uint8_t buffer[12 + 512];

    int len;

    for(;;) {
    	// Compose json_stat

		// Get current time
		time_t t = time(NULL);
		struct tm tmr, *stm;
		stm = _localtime(&t, &tmr);

	    sprintf(json_stat, "{\"stat\":{\"time\":\"%04d-%02d-%02d %02d:%02d:%02d GMT\",\"lati\":%.5f,\"long\":%.5f,\"alti\":%d,\"rxnb\":%u,\"rxok\":%u,\"rxfw\":%u,\"ackr\":%u,\"dwnb\":%u,\"txnb\":%u}}\n",
	    (stm->tm_year + 1900),              /* UTC 'system' time of the gateway, ISO 8601 'expanded' format */
	    (stm->tm_mon + 1),                  /* UTC 'system' time of the gateway, ISO 8601 'expanded' format */
	    stm->tm_mday,                       /* UTC 'system' time of the gateway, ISO 8601 'expanded' format */
	    stm->tm_hour,                       /* UTC 'system' time of the gateway, ISO 8601 'expanded' format */
	    stm->tm_min,                        /* UTC 'system' time of the gateway, ISO 8601 'expanded' format */
	    stm->tm_sec,                        /* UTC 'system' time of the gateway, ISO 8601 'expanded' format */
	    0.0,	                            /* GPS latitude of the gateway in degree (float, N is +) */
	    0.0,                                /* GPS latitude of the gateway in degree (float, E is +) */
	    0,                                  /* GPS altitude of the gateway in meter RX (integer) */
	    rxnb,                               /* Number of radio packets received (unsigned integer) */
	    rxok,                               /* Number of radio packets received with a valid PHY CRC */
	    rxfw,                               /* Number of radio packets forwarded (unsigned integer) */
	    ackr,                               /* Percentage of upstream datagrams that were acknowledged */
	    dwnb,                               /* Number of downlink datagrams received (unsigned integer) */
	    txnb);                              /* Number of packets emitted (unsigned integer) */

		buffer[0]  = 0x01;					/* protocol version = 1 */
		buffer[1]  = (uint8_t)rand();		/* random token */
		buffer[2]  = (uint8_t)rand();		/* random token */
		buffer[3]  = 0x00;		    		/* PUSH_DATA identifier 0x00 */
		buffer[4]  = gw_eui[0];				/* Gateway unique identifier (MAC address) */
		buffer[5]  = gw_eui[1];				/* Gateway unique identifier (MAC address) */
		buffer[6]  = gw_eui[2];				/* Gateway unique identifier (MAC address) */
		buffer[7]  = gw_eui[3];				/* Gateway unique identifier (MAC address) */
		buffer[8]  = gw_eui[4];				/* Gateway unique identifier (MAC address) */
		buffer[9]  = gw_eui[5];				/* Gateway unique identifier (MAC address) */
		buffer[10] = gw_eui[6];				/* Gateway unique identifier (MAC address) */
		buffer[11] = gw_eui[7];				/* Gateway unique identifier (MAC address) */

		/* JSON object, starting with {, ending with }, see section 4 */
		strcpy((char *)&buffer[12], json_stat);

		socklen_t slen = sizeof(up_address);

		// Send stats to TTN if network connection is available
		while ((error = net_check_connectivity())) {
			syslog(LOG_INFO, "lora gw: no network connection");

	    	free(error);

	    	if ((error = net_reconnect())) {
	    		free(error);
	    		continue;
	    	} else {
				syslog(LOG_INFO, "lora gw: network connection re-established");
	    	}
	    }

		if (sendto(up_socket, buffer, strlen((char *)&buffer[12]) + 12, 0, (struct sockaddr *)&up_address, slen) <= 0) {
			syslog(LOG_ERR, "lora gw: can't send\n");
		}

		memset(buffer, 0, sizeof(buffer));

		// Read response from TTN if network connection is available
		while ((error = net_check_connectivity())) {
			syslog(LOG_INFO, "lora gw: no network connection");

	    	free(error);

	    	if ((error = net_reconnect())) {
	    		free(error);
	    		continue;
	    	} else {
				syslog(LOG_INFO, "lora gw: network connection re-established");
	    	}
	    }

		len = recvfrom(up_socket, buffer, sizeof(buffer), 0, (struct sockaddr *)&up_address, &slen);
        if (len > 0) {
        	if (buffer[3] == 0x01) {
        	}
        }

        // Wait 30 seconds
		vTaskDelay(30000 / portTICK_PERIOD_MS);
	}
}

#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
driver_error_t *lora_gw_lock_resources(int unit) {
    driver_unit_lock_error_t *lock_error = NULL;

	if ((lock_error = driver_lock(LORA_DRIVER, unit, SPI_DRIVER, CONFIG_LUA_RTOS_LORA_SPI, DRIVER_ALL_FLAGS, NULL))) {
		// Revoked lock on pin
		return driver_lock_error(LORA_DRIVER, lock_error);
	}

	#if !CONFIG_LUA_RTOS_USE_POWER_BUS && (CONFIG_LUA_RTOS_LORA_RST >= 0)
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

driver_error_t *lora_gw_setup(int band, const char *host, int port) {
	driver_error_t *error;
	uint8_t mac[6];        // Mac address

	syslog(LOG_INFO, "lora gw: starting");

	// Check network connection
	if ((error = net_check_connectivity())) {
		free(error);

		return driver_error(LORA_DRIVER, LORA_ERR_CANT_SETUP, "no network connection available");
	}

	// Get MAC
	if ((error = wifi_get_mac(mac))) {
		free(error);

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

	// Init SPI bus
	if ((error = spi_setup(CONFIG_LUA_RTOS_LORA_SPI, 1, CONFIG_LUA_RTOS_LORA_CS, 0, 1000000, SPI_FLAG_WRITE | SPI_FLAG_READ, &spi_device))) {
        return error;
    }

#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
	// Lock pins
    if ((error = lora_gw_lock_resources(0))) {
    	return error;
    }
#endif

	syslog(LOG_INFO, "lora gw: single gateway is at spi%d, pin cs=%s%d", CONFIG_LUA_RTOS_LORA_SPI,
        gpio_portname(CONFIG_LUA_RTOS_LORA_CS), gpio_name(CONFIG_LUA_RTOS_LORA_CS)
	);

	syslog(LOG_INFO, "lora gw: gateway EUI %02x%02x%02x%02x%02x%02x%02x%02x",
		gw_eui[0],gw_eui[1],gw_eui[2],gw_eui[3],gw_eui[4],gw_eui[5], gw_eui[6], gw_eui[7]
	);

	#if !CONFIG_LUA_RTOS_USE_POWER_BUS && (CONFIG_LUA_RTOS_LORA_RST >= 0)
		// Init RESET pin
		gpio_pin_output(CONFIG_LUA_RTOS_LORA_RST);
	#endif

	// Init DIO pins
	#if CONFIG_LUA_RTOS_LORA_DIO0 >= 0
		if ((error = gpio_isr_attach(CONFIG_LUA_RTOS_LORA_DIO0, dio_intr_handler, GPIO_INTR_POSEDGE, (void*)0))) {
			error->msg = "DIO0";
			return error;
		}
	#endif

	#if CONFIG_LUA_RTOS_LORA_DIO1 >= 0
		if ((error = gpio_isr_attach(CONFIG_LUA_RTOS_LORA_DIO1, dio_intr_handler, GPIO_INTR_POSEDGE, (void*)1))) {
			error->msg = "DIO1";
			return error;
		}
	#endif

	#if CONFIG_LUA_RTOS_LORA_DIO2 >= 0
		if ((error = gpio_isr_attach(CONFIG_LUA_RTOS_LORA_DIO2, dio_intr_handler, GPIO_INTR_POSEDGE, (void*)2))) {
			error->msg = "DIO2";
			return error;
		}
	#endif

	syslog(LOG_INFO, "lora gw: network coordinator set to %s:%d", host, port);

	// Create socket
	if ((up_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		return driver_error(LORA_DRIVER, LORA_ERR_CANT_SETUP, "can't create socket");
	}

	// Resolve name
	if ((error = net_lookup(host,port, &up_address))) {
		free(error);
		lora_gw_unsetup();

		return driver_error(LORA_DRIVER, LORA_ERR_CANT_SETUP, "can't resolve network coordinator hostname / port");
	}

	// Create queues if needed
	if (!lora_phy_queue) {
		lora_phy_queue = xQueueCreate(100, sizeof(lora_phy_data_t));
		if (!lora_phy_queue) {
			lora_gw_unsetup();
			return driver_error(LORA_DRIVER, LORA_ERR_NO_MEM, NULL);
		}
	}

	if (!lora_data_queue) {
		lora_data_queue = xQueueCreate(100, sizeof(lora_data_t));
		if (!lora_data_queue) {
			lora_gw_unsetup();
			return driver_error(LORA_DRIVER, LORA_ERR_NO_MEM, NULL);
		}
	}

	// Create tasks if needed
	if (!lora_phy_task) {
		BaseType_t xReturn;

		xReturn = xTaskCreatePinnedToCore(phy_task, "loragw", CONFIG_LUA_RTOS_LUA_THREAD_STACK_SIZE, NULL, CONFIG_LUA_RTOS_LUA_THREAD_PRIORITY, &lora_phy_task, xPortGetCoreID());
		if (xReturn != pdPASS) {
			lora_gw_unsetup();
			return driver_error(LORA_DRIVER, LORA_ERR_NO_MEM, NULL);
		}
	}

	if (!lora_ttn_up_task) {
		BaseType_t xReturn;

		xReturn = xTaskCreatePinnedToCore(ttn_up_task, "lorattn", CONFIG_LUA_RTOS_LUA_THREAD_STACK_SIZE, NULL, CONFIG_LUA_RTOS_LUA_THREAD_PRIORITY, &lora_ttn_up_task, xPortGetCoreID());
		if (xReturn != pdPASS) {
			lora_gw_unsetup();
			return driver_error(LORA_DRIVER, LORA_ERR_NO_MEM, NULL);
		}
	}

	if (!lora_ttn_timer_task) {
		BaseType_t xReturn;

		xReturn = xTaskCreatePinnedToCore(ttn_timer_task, "ttn_timer", CONFIG_LUA_RTOS_LUA_THREAD_STACK_SIZE, NULL, CONFIG_LUA_RTOS_LUA_THREAD_PRIORITY, &lora_ttn_timer_task, xPortGetCoreID());
		if (xReturn != pdPASS) {
			lora_gw_unsetup();
			return driver_error(LORA_DRIVER, LORA_ERR_NO_MEM, NULL);
		}
	}

	// Reset transceiver
	sx1276_reset(0);

	// Reset pin float
	sx1276_reset(2);

	delay(6);

	// Check transceiver
	uint8_t data;

	// Sleep mode
    stx1276_read_reg(spi_device, SX1276_REG_OP_MODE, &data);
	stx1276_write_reg(spi_device, SX1276_REG_OP_MODE, (data & ~OPMODE_MASK) | OPMODE_SLEEP);

	stx1276_read_reg(spi_device, SX1276_REG_VERSION, &data);
	if (data == 0x12) {
	    stx1276_read_reg(spi_device, SX1276_REG_OP_MODE, &data);
		stx1276_write_reg(spi_device, SX1276_REG_OP_MODE, (data & ~OPMODE_MASK) | OPMODE_LORA);

		// Set frequency
		uint64_t frf = ((uint64_t) freq[freq_idx] << 19) / 32000000;
		stx1276_write_reg(spi_device, SX1276_REG_FR_MSB, (uint8_t)(frf >> 16));
		stx1276_write_reg(spi_device, SX1276_REG_FR_MID, (uint8_t)(frf >> 8));
		stx1276_write_reg(spi_device, SX1276_REG_FR_LSB, (uint8_t)(frf >> 0));

		// LoRaWAN public sync word
		stx1276_write_reg(spi_device, SX1276_REG_SYNC_WORD, 0x34);

		// Configure modem parameters based on sf
		stx1276_write_reg(spi_device, SX1276_REG_MODEM_CONFIG_1,0x72);
		stx1276_write_reg(spi_device, SX1276_REG_MODEM_CONFIG_2,(sf[sf_idx] << 4) | 0x04);
		stx1276_write_reg(spi_device, SX1276_REG_MODEM_CONFIG_3, 0x04);
		stx1276_write_reg(spi_device, SX1276_REG_SYMB_TIMEOUT_LSB, 0x08);

		if (sf[sf_idx] == 10) stx1276_write_reg(spi_device, SX1276_REG_SYMB_TIMEOUT_LSB, 0x05);

		if (sf[sf_idx] == 11 || sf[sf_idx] == 12) {
			stx1276_write_reg(spi_device, SX1276_REG_MODEM_CONFIG_3, 0x0C);
			stx1276_write_reg(spi_device, SX1276_REG_SYMB_TIMEOUT_LSB, 0x05);
		}

		stx1276_write_reg(spi_device, SX1276_REG_HOP_PERIOD, 0xFF);
		stx1276_write_reg(spi_device, SX1276_REG_PAYLOAD_LENGTH, 0x40);
		stx1276_write_reg(spi_device, SX1276_REG_MAX_PAYLOAD_LENGTH, 0x80);

		stx1276_read_reg(spi_device, SX1276_REG_FIFO_RX_BASE_ADDR, &data);
		stx1276_write_reg(spi_device, SX1276_REG_FIFO_ADDR_PTR, data);

		// Max lna gain
		stx1276_write_reg(spi_device, SX1276_REG_LNA, 0x23);

		// Continous Receive Mode see CAD mode for multiple channel GW
	    stx1276_read_reg(spi_device, SX1276_REG_OP_MODE, &data);
		stx1276_write_reg(spi_device, SX1276_REG_OP_MODE, (data & ~OPMODE_MASK) | OPMODE_CONTINOUS);
	    stx1276_read_reg(spi_device, SX1276_REG_OP_MODE, &data);
	} else {
		lora_gw_unsetup();
		return driver_error(LORA_DRIVER, LORA_ERR_CANT_SETUP, "radio transceiver not detected");
	}

	syslog(LOG_INFO, "lora gw: started");

	return NULL;
}

void lora_gw_unsetup() {
	if (lora_phy_task) {
		vTaskDelete(lora_phy_task);
		lora_phy_task = NULL;
	}

	if (lora_ttn_up_task) {
		vTaskDelete(lora_ttn_up_task);
		lora_ttn_up_task = NULL;
	}

	if (lora_ttn_timer_task) {
		vTaskDelete(lora_ttn_timer_task);
		lora_ttn_timer_task = NULL;
	}

	if (lora_phy_queue) {
		vQueueDelete(lora_phy_queue);
		lora_phy_queue = NULL;
	}

	if (lora_data_queue) {
		vQueueDelete(lora_data_queue);
		lora_data_queue = NULL;
	}
}

#endif
