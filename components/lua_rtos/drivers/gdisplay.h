#ifndef DRIVERS_GDISPLAY_H_
#define DRIVERS_GDISPLAY_H_

#include "sdkconfig.h"
#include <stdint.h>

#if CONFIG_LUA_RTOS_LUA_USE_GDISPLAY

#define DELAY 0x80

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
	int spi_device;
	uint8_t bytes_per_pixel;
	uint8_t rdepth;
	uint8_t gdepth;
	uint8_t bdepth;
	uint8_t orient;
	uint16_t phys_width;
	uint16_t phys_height;
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
