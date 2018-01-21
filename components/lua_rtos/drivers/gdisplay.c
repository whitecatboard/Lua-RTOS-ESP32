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
 * Lua RTOS, graphic display driver
 *
 */

#include "sdkconfig.h"

#if CONFIG_LUA_RTOS_LUA_USE_GDISPLAY

#include "esp_attr.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stdlib.h>

#include <machine/endian.h>

#include <gdisplay/gdisplay.h>
#include <drivers/gdisplay.h>
#include <drivers/gpio.h>
#include <drivers/spi.h>

// Display capabilities
static gdisplay_caps_t caps = {
	0, 0, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	-1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/*
 * Internal buffer
 *
 * The mission of this buffer is to minimize the SPI transfers to the
 * display, and maximize the length for each SPI transfer to the
 * display.
 *
 */
static int buff_size = 0;     // Buffer size in pixels
static uint8_t *buff = NULL;  // Buffer

// Buffer bounding box. When the driver needs to update the display
// the bounding box is used for set the window address.
static int buff_x0 = 0;
static int buff_y0 = 0;
static int buff_x1 = 0;
static int buff_y1 = 0;

// Pixels in buffer
static int buff_pixels  = 0;

void IRAM_ATTR gdisplay_ll_command(uint8_t command) {
	gpio_ll_pin_clr(CONFIG_LUA_RTOS_GDISPLAY_CMD);
	spi_ll_select(caps.device);
	spi_ll_transfer(caps.device, command, NULL);
	spi_ll_deselect(caps.device);
}

void gdisplay_ll_data(uint8_t *data, int len) {
    if (len == 0) return;

	gpio_ll_pin_set(CONFIG_LUA_RTOS_GDISPLAY_CMD);
	spi_ll_select(caps.device);
	spi_ll_bulk_write(caps.device, len, data);
	spi_ll_deselect(caps.device);
}

void gdisplay_ll_data32(uint32_t data) {
	gpio_ll_pin_set(CONFIG_LUA_RTOS_GDISPLAY_CMD);
	spi_ll_select(caps.device);
	spi_ll_bulk_write32(caps.device, 1, &data);
	spi_ll_deselect(caps.device);
}

void gdisplay_ll_command_list(const uint8_t *addr) {
  uint8_t  numCommands, numArgs, cmd;
  uint16_t ms;

  numCommands = *addr++;         // Number of commands to follow
  while(numCommands--) {         // For each command...
    cmd = *addr++;               // save command
    numArgs  = *addr++;          //   Number of args to follow
    ms       = numArgs & DELAY;  //   If high bit set, delay follows args
    numArgs &= ~DELAY;           //   Mask out delay bit

    gdisplay_ll_command(cmd);
    gdisplay_ll_data((uint8_t *)addr, numArgs);

    addr += numArgs;

    if(ms) {
      ms = *addr++;              // Read post-command delay time (ms)
      if(ms == 255) ms = 500;    // If 255, delay for 500 ms
	  vTaskDelay(ms / portTICK_RATE_MS);
    }
  }
}

gdisplay_caps_t *gdisplay_ll_get_caps() {
	return &caps;
}

void gdisplay_ll_invalidate_buffer() {
	buff_x0 = 5000;
	buff_x1 = -1;
	buff_y0 = 5000;
	buff_y1 = -1;

	buff_pixels = 0;
}

uint8_t *gdisplay_ll_allocate_buffer(uint32_t size) {
	// Initialize buffer
	if (caps.bytes_per_pixel == 0) {
		buff = calloc(size, 1);
	} else {
		buff = calloc(size, caps.bytes_per_pixel);
	}
	if (!buff) {
		return NULL;
	}

	buff_size = size;

	gdisplay_ll_invalidate_buffer();

	return buff;
}

uint8_t *gdisplay_ll_get_buffer() {
	return buff;
}

uint32_t gdisplay_ll_get_buffer_size() {
	return buff_size;
}

void gdisplay_ll_update(int x0, int y0, int x1, int y1, uint8_t *buffer) {
	if ((buffer == (uint8_t *)buff) || (buffer == NULL)) {
		if (buff_pixels > 0) {
			if (buffer) {
				caps.addr_window(1, x0, y0, x1, y1);
			} else {
				caps.addr_window(1, buff_x0, buff_y0, buff_x1, buff_y1);
			}

			if (caps.interface == GDisplaySPIInterface) {
				// Set DC to 1 (data mode)
				gpio_ll_pin_set(CONFIG_LUA_RTOS_GDISPLAY_CMD);
				spi_ll_select(caps.device);
			}

			if (caps.bytes_per_pixel == 0) {
				if (caps.interface == GDisplaySPIInterface) {
					spi_ll_bulk_write(caps.device, buff_size, (uint8_t *)buff);
				} else{
					ssd1306_update(x0, y0, x1, y1, buff);
				}
			} else {
				if (caps.interface == GDisplaySPIInterface) {
					spi_ll_bulk_write16(caps.device, buff_pixels, (uint16_t *)buff);
				} else {
					ssd1306_update(x0, y0, x1, y1, buff);
				}
			}

			if (caps.interface == GDisplaySPIInterface) {
				spi_ll_deselect(caps.device);
			}
		}
	} else {
		caps.addr_window(1, x0, y0, x1, y1);

		if (caps.interface == GDisplaySPIInterface) {
		    // Set DC to 1 (data mode)
			gpio_ll_pin_set(CONFIG_LUA_RTOS_GDISPLAY_CMD);
			spi_ll_select(caps.device);
		}

		if (caps.bytes_per_pixel == 0) {
			memcpy(buff, buffer, buff_size);

			if (caps.interface == GDisplaySPIInterface) {
				spi_ll_bulk_write(caps.device, buff_size, (uint8_t *)buff);
			} else {
				ssd1306_update(x0, y0, x1, y1, buff);
			}
		} else {
			uint16_t *origin = (uint16_t *)buffer;
			uint16_t *destination =(uint16_t *) buff;
			int len = 0;
			int x,y;

			for(y = y0;y <= y1;y++) {
				for(x = x0;x <= x1;x++) {
					if (len == buff_size) {
						if (caps.interface == GDisplaySPIInterface) {
							spi_ll_bulk_write16(caps.device, len, (uint16_t *)buff);
						} else {
							ssd1306_update(x0, y0, x1, y1, buff);
						}

						len = 0;
						destination = (uint16_t *)buff;
					}

					*destination++ = origin[y * caps.width + x];
					len++;
				}
			}

			if (len > 0) {
				if (caps.interface == GDisplaySPIInterface) {
					spi_ll_bulk_write16(caps.device, len, (uint16_t *)buff);
				} else{
					ssd1306_update(x0, y0, x1, y1, buff);
				}

				len = 0;
			}
		}

		if (caps.interface == GDisplaySPIInterface) {
			spi_ll_deselect(caps.device);
		}
	}

	gdisplay_ll_invalidate_buffer();
}

void IRAM_ATTR gdisplay_ll_set_pixel(int x, int y, uint32_t color, uint8_t *buffer, int buffw, int buffh) {
	// Take care about endianness
	uint32_t wd;

	if (caps.bytes_per_pixel == 0) {
		// Rotate x,y according to current orientation
		// In pcd8544 this only can be done by software
		if (caps.orient == LANDSCAPE_FLIP) {
			x = (buffw!=-1?buffw:caps.phys_width) - 1 - x;
			y = (buffh!=-1?buffh:caps.phys_height) - 1 - y;
		} else if (caps.orient == PORTRAIT) {
			x = (buffh!=-1?buffh:caps.phys_height) - 1 - x;
			swap(x,y);
		} else if (caps.orient == PORTRAIT_FLIP) {
			y = (buffw!=-1?buffw:caps.phys_width) - 1 - y;
			swap(x,y);
		}
	} else {
		if (BYTE_ORDER == LITTLE_ENDIAN) {
			wd = (uint32_t)(color >> 8);
			wd |= (uint32_t)(color & 0xff) << 8;
		} else {
			wd = color;
		}
	}

	if (buffer) {
		if (caps.bytes_per_pixel == 0) {
			if (color) {
				if (caps.monochrome_white) {
					buffer[x + (y/8) * (buffw!=-1?buffw:caps.phys_width)] |= (1 << (y % 8));
				} else {
					buffer[x + (y/8) * (buffw!=-1?buffw:caps.phys_width)] &= ~(1 << (y % 8));
				}
			} else {
				if (caps.monochrome_white) {
					buffer[x + (y/8) * (buffw!=-1?buffw:caps.phys_width)] &= ~(1 << (y % 8));
				} else {
					buffer[x + (y/8) * (buffw!=-1?buffw:caps.phys_width)] |= (1 << (y % 8));
				}
			}
		} else {
			((uint16_t *)buffer)[y * (buffw!=-1?buffw:caps.width) + x] = (uint16_t)wd;
		}
	} else {
		if (buff) {
			if (caps.bytes_per_pixel == 0) {
				if (color) {
					buff[x + (y/8) * (buffw!=-1?buffw:caps.phys_width)] &= ~(1 << (y % 8));
				} else {
					buff[x + (y/8) * (buffw!=-1?buffw:caps.phys_width)] |= (1 << (y % 8));
				}
			} else {
				int index;

				// New bounding box
				int nbuff_x0, nbuff_x1, nbuff_y0, nbuff_y1;

				if (buff_pixels == 0) {
					// Buffer is empty. Bounding box is current point (x,y).
					nbuff_x0 = x;
					nbuff_x1 = x;
					nbuff_y0 = y;
					nbuff_y1 = y;

					index = 0;
				} else {
					// Buffer is not empty. Copy current bounding box into new
					// bounding box.
					nbuff_x0 = buff_x0;
					nbuff_x1 = buff_x1;
					nbuff_y0 = buff_y0;
					nbuff_y1 = buff_y1;

					// Update new bounding box
					if (x > nbuff_x1) nbuff_x1 = x;
					if (y > nbuff_y1) nbuff_y1 = y;

					// Calculate pixel's position into bounding box
					index = (y - nbuff_y0) * (nbuff_x1 - nbuff_x0 + 1) + (x - nbuff_x0);
				}

				if (index != buff_pixels) {
					gdisplay_ll_update(buff_x0, buff_y0, buff_x1, buff_y1, (uint8_t *)buff);
					index = 0;

					nbuff_x0 = x;
					nbuff_x1 = x;
					nbuff_y0 = y;
					nbuff_y1 = y;
				}

				((uint16_t *)buff)[index] = (uint16_t)wd;
				buff_pixels++;

				// Update bounding box
				buff_x0 = nbuff_x0;
				buff_x1 = nbuff_x1;
				buff_y0 = nbuff_y0;
				buff_y1 = nbuff_y1;

				// If buffer is full, flush
				if (buff_pixels == buff_size) {
					gdisplay_ll_update(buff_x0, buff_y0, buff_x1, buff_y1, (uint8_t *)buff);
				}
			}
		}
	}
}

uint32_t gdisplay_ll_get_pixel(int x, int y, uint8_t *buffer, int buffw, int buffh) {
	uint32_t color = 0;

	if (buffer == NULL) {
		return 0;
	} else {
		// Take care about endianness
		uint32_t wd;

		color = ((uint16_t *)buffer)[y * (buffw!=-1?buffw:caps.width) + x];

		if (BYTE_ORDER == LITTLE_ENDIAN) {
			wd = (uint32_t)(color >> 8);
			wd |= (uint32_t)(color & 0xff) << 8;
		}

		color = wd;
	}

	return color;
}

void gdisplay_ll_set_bitmap(int x, int y, uint8_t *buffer, uint8_t *buff, int buffw, int buffh) {
	int i, j;

	for(i=0;i < buffh;i++) {
		for(j=0;j < buffw;j++) {
			((uint16_t *)buffer)[(y + i) * caps.width + (x + j)] = ((uint16_t *)buff)[i * buffw + j];
		}
	}
}

void gdisplay_ll_get_bitmap(int x, int y, uint8_t *buffer, uint8_t *buff, int buffw, int buffh) {
	int i, j;

	for(i=0;i < buffh;i++) {
		for(j=0;j < buffw;j++) {
			((uint16_t *)buff)[i * buffw + j] = ((uint16_t *)buffer)[(y + i) * caps.width + (x + j)];
		}
	}
}

void gdisplay_ll_on() {
	caps.on();
}

void gdisplay_ll_off() {
	caps.off();
}

void gdisplay_ll_invert(uint8_t on) {
	caps.invert(on);
}

void gdisplay_ll_set_orientation(uint8_t orientation) {
	caps.orientation(orientation);
	caps.orient = orientation;

	if (caps.bytes_per_pixel == 0) {
		buff_pixels = caps.width * caps.height;
	}
}

#endif
