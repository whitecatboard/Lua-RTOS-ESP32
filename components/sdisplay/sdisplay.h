#ifndef SDISPLAY_H_
#define SDISPLAY_H_

#include <sys/driver.h>

typedef enum {
	SDisplayNoType = 0,
	SDisplayTwoWire = 1,
	SDisplayI2C = 2
} sdisplay_type_t;

struct sdisplay;

typedef struct {
	uint8_t chipset; // Chipset
	sdisplay_type_t type;
	driver_error_t *(*setup)(struct sdisplay *);
	driver_error_t *(*clear)(struct sdisplay *);
	driver_error_t *(*write)(struct sdisplay *, const char *);
	driver_error_t *(*brightness)(struct sdisplay *, uint8_t);
} sdisplay_t;

typedef struct sdisplay{
	const sdisplay_t *display;
	int id;
	uint8_t brightness;
	uint8_t digits;

	union {
		struct {
			int clk;
			int dio;
		} wire;

		struct {
			int device;
			int address;
		} i2c;
	} config;
} sdisplay_device_t;

#define CHIPSET_TM1637 1
#define CHIPSET_HT16K3 2

sdisplay_type_t sdisplay_type(uint8_t chipset);
driver_error_t *sdisplay_setup(uint8_t chipset, sdisplay_device_t *device, uint8_t digits, ...) ;
driver_error_t *sdisplay_clear(sdisplay_device_t *device);
driver_error_t *sdisplay_write(sdisplay_device_t *device, const char *data);
driver_error_t *sdisplay_brightness(sdisplay_device_t *device, uint8_t brightness);

//  Driver errors
#define  SDISPLAY_ERR_INVALID_CHIPSET     (DRIVER_EXCEPTION_BASE(SDISPLAY_DRIVER_ID) |  0)
#define  SDISPLAY_ERR_TIMEOUT             (DRIVER_EXCEPTION_BASE(SDISPLAY_DRIVER_ID) |  1)
#define  SDISPLAY_ERR_INVALID_BRIGHTNESS  (DRIVER_EXCEPTION_BASE(SDISPLAY_DRIVER_ID) |  2)
#define  SDISPLAY_ERR_NOT_ENOUGH_MEMORY   (DRIVER_EXCEPTION_BASE(SDISPLAY_DRIVER_ID) |  3)
#define  SDISPLAY_ERR_INVALID_DIGITS      (DRIVER_EXCEPTION_BASE(SDISPLAY_DRIVER_ID) |  4)

extern const int sdisplay_errors;
extern const int sdisplay_error_map;

#endif /* SDISPLAY_H_ */
