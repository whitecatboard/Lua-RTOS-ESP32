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
 * Lua RTOS TUN vfs
 *
 */

#include "sdkconfig.h"

#if CONFIG_LUA_RTOS_USE_OPENVPN

#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <sys/errno.h>
#include <sys/lock.h>
#include <sys/fcntl.h>
#include "esp_vfs.h"
#include "esp_vfs_dev.h"
#include "esp_attr.h"

#include "lwip/opt.h"

#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/stats.h"
#include "lwip/snmp.h"
#include "lwip/ethip6.h"
#include "netif/etharp.h"

#include "tcpip_adapter.h"

#include <drivers/net.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

xQueueHandle tun_queue_rx = NULL;
xQueueHandle tun_queue_tx = NULL;

static int vfs_tun_open(const char *path, int flags, int mode);
static ssize_t vfs_tun_write(int fd, const void *data, size_t size);
static ssize_t vfs_tun_read(int fd, void * dst, size_t size);
static int vfs_tun_close(int fd);
static int vfs_tun_select (int maxfdp1, fd_set *readset, fd_set *writeset, fd_set *exceptset, struct timeval *timeout);

void vfs_tun_register()
{
    esp_vfs_t vfs = {
        .flags = ESP_VFS_FLAG_DEFAULT,
        .write = &vfs_tun_write,
        .open = &vfs_tun_open,
        .fstat = NULL,
        .close = &vfs_tun_close,
        .read = &vfs_tun_read,
        .fcntl = NULL,
        .ioctl = NULL,
        .writev = NULL,
		.select = &vfs_tun_select,
    };

    ESP_ERROR_CHECK(esp_vfs_register("/dev/tun", &vfs, NULL));
}

struct netif *tcp_adapter_get_netif(tcpip_adapter_if_t tcpip_if);

static int vfs_tun_open(const char *path, int flags, int mode) {
	// Create the tx and rx queues
    tun_queue_rx = xQueueCreate(4, sizeof(struct pbuf *));
    tun_queue_tx = xQueueCreate(4, sizeof(struct pbuf *));

    // Start TUN interface without mac
    tcpip_adapter_ip_info_t tun_ip;

    memset(&tun_ip, 0, sizeof(tcpip_adapter_ip_info_t));
    tcpip_adapter_tun_start(NULL, &tun_ip);

    return 0;
}

static ssize_t vfs_tun_write(int fd, const void *data, size_t size) {
    struct pbuf *p = NULL, *q = NULL;
    u16_t len = size;
    u16_t frame_size = 0;

    if (tun_queue_tx && (len > 0)) {
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

            memcpy(q->payload, data, frame_size);

            #if ETH_PAD_SIZE
            pbuf_header(p, ETH_PAD_SIZE); /* reclaim the padding word */
            #endif

			if (xQueueSend(tun_queue_tx, &p, portMAX_DELAY) != pdTRUE) {
				pbuf_free(p);

				return 0;
			}

            return size;
        }
    }

    return 0;
}

static ssize_t vfs_tun_read(int fd, void * dst, size_t size) {
	struct pbuf *p;
	size_t len = size;

	if (tun_queue_rx && (len > 0)) {
		xQueueReceive(tun_queue_rx, &p, portMAX_DELAY);

		len = p->len;

		memcpy(dst, p->payload, p->len);

		pbuf_free(p);
	}

	return len;
}

static int vfs_tun_close(int fd) {
	tcpip_adapter_stop(TCPIP_ADAPTER_IF_TUN);
	return 0;
}

#define MAX(a,b) (((a)>(b))?(a):(b))

static int tv_to_ms_timeout(const struct timeval *tv) {
    if (tv->tv_sec == 0 && tv->tv_usec == 0) {
        return 0;
    } else {
        return MAX(tv->tv_sec * 1000 + (tv->tv_usec + 500) / 1000, 1);
    }
}

static int vfs_tun_select (int maxfdp1, fd_set *readset, fd_set *writeset, fd_set *exceptset, struct timeval *timeout) {
	struct pbuf *p;
	int fd;
	int num;

	num = 0;

	for(fd = 0;fd <= maxfdp1;fd++) {
		if (readset && FD_ISSET(fd, readset)) {
			if (xQueuePeek(tun_queue_rx, &p, tv_to_ms_timeout(timeout) / portTICK_PERIOD_MS) == pdTRUE) {
				num++;
			} else {
				FD_CLR(fd, readset);
			}
		}

		if (writeset && FD_ISSET(fd, writeset)) {
			if (uxQueueSpacesAvailable(tun_queue_tx) > 0) {
				num++;
			} else {
				FD_CLR(fd, writeset);
			}
		}

		if (exceptset && FD_ISSET(fd, exceptset)) {
		}
	}

	return num;
}

#endif
