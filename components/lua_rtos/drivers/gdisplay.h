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

#ifndef DRIVERS_GDISPLAY_H_
#define DRIVERS_GDISPLAY_H_

#include "sdkconfig.h"
#include <stdint.h>

#if CONFIG_LUA_RTOS_LUA_USE_GDISPLAY

#include <gdisplay/gdisplay.h>

#define DELAY 0x80

typedef enum {
	GDisplaySPIInterface,
	GDisplayI2CInterface
} gdisplay_interface_t;

typedef struct {
	uint16_t width;
	uint16_t height;
	uint16_t xstart;
	uint16_t ystart;
    void (*addr_window)(uint8_t, int, int , int, int);
    void (*on)();
    void (*off)();
    void (*invert)(uint8_t);
    void (*orientation)(uint8_t);
    void (*touch_get)(int *, int *, int *, uint8_t);
    void (*touch_cal)(int, int);
	int device;
	uint8_t bytes_per_pixel;
	uint8_t monochrome_white;
	uint8_t rdepth;
	uint8_t gdepth;
	uint8_t bdepth;
	uint8_t orient;
	uint16_t phys_width;
	uint16_t phys_height;
	gdisplay_interface_t interface;
	uint8_t address;
} gdisplay_caps_t;

/**
 * @brief Sends a command to the display
 *
 * @param command Command to send.
 *
 */
void gdisplay_ll_command(uint8_t command);

/**
 * @brief Sends a buffer of 1-byte data to the display
 *
 * @param data Pointer to a uint8_t buffer, with data to send to
 *             the display.
 *
 * @param len Data length
 *
 */
void gdisplay_ll_data(uint8_t *data, int len);

/**
 * @brief Sends a word (4-byte) to the display
 *
 * @param data Data to send.
 *
 */
void gdisplay_ll_data32(uint32_t data);

void gdisplay_ll_command_list(const uint8_t *addr);
gdisplay_caps_t *gdisplay_ll_get_caps();
void gdisplay_ll_invalidate_buffer();
uint8_t *gdisplay_ll_allocate_buffer(uint32_t size);
uint8_t *gdisplay_ll_get_buffer();
uint32_t gdisplay_ll_get_buffer_size();
void gdisplay_ll_update(int x0, int y0, int x1, int y1, uint8_t *buffer);
void gdisplay_ll_set_pixel(int x, int y, uint32_t color, uint8_t *buffer, int buffw, int buffh);
uint32_t gdisplay_ll_get_pixel(int x, int y, uint8_t *buffer, int buffw, int buffh);
void gdisplay_ll_set_bitmap(int x, int y, uint8_t *buffer, uint8_t *buff, int buffw, int buffh);
void gdisplay_ll_get_bitmap(int x, int y, uint8_t *buffer, uint8_t *buff, int buffw, int buffh);
void gdisplay_ll_on();
void gdisplay_ll_off();
void gdisplay_ll_invert(uint8_t on);
void gdisplay_ll_set_orientation(uint8_t m);

#endif

#endif /* DRIVERS_GDISPLAY_H_ */
