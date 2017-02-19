/**
 * DS1820 Family temperature sensor driver for Lua-RTOS-ESP32
 * author: LoBo (loboris@gmail.com)
 * based on TM_ONEWIRE (author  Tilen Majerle)
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "rom/ets_sys.h"
#include <drivers/sensor.h>

//#define DS18B20ALARMFUNC
//#define DS18B20_USE_CRC

/* TM_DS18B20_Macros
*  Every onewire chip has different ROM code, but all the same chips has same family code
*  in case of DS18B20 this is 0x28 and this is first byte of ROM address
*/
#define DS18B20_FAMILY_CODE			0x28
#define DS18S20_FAMILY_CODE			0x10
#define DS1822_FAMILY_CODE			0x22
#define DS28EA00_FAMILY_CODE		0x42
#define DS18B20_CMD_ALARMSEARCH		0xEC

/* DS18B20 read temperature command */
#define DS18B20_CMD_CONVERTTEMP		0x44 	/* Convert temperature */
#define DS18B20_DECIMAL_STEPS_12BIT	0.0625
#define DS18B20_DECIMAL_STEPS_11BIT	0.125
#define DS18B20_DECIMAL_STEPS_10BIT	0.25
#define DS18B20_DECIMAL_STEPS_9BIT	0.5

/* Bits locations for resolution */
#define DS18B20_RESOLUTION_R1		6
#define DS18B20_RESOLUTION_R0		5

/* CRC enabled */
#ifdef DS18B20_USE_CRC
#define DS18B20_DATA_LEN			9
#else
#define DS18B20_DATA_LEN			2
#endif

/* TM_DS18B20_Typedefs */

/* DS1820 errors */
typedef enum {
  ow_OK = 0,
  owError_NoDevice,
  owError_Not18b20,
  owError_NotFinished,
  owError_BadCRC,
  owError_NotReady,
  owError_Convert
} owState_t;


/* DS18B0 Resolutions available */
typedef enum {
  TM_DS18B20_Resolution_9bits = 	 9, /*!< DS18B20 9 bits resolution */
  TM_DS18B20_Resolution_10bits = 	10, /*!< DS18B20 10 bits resolution */
  TM_DS18B20_Resolution_11bits = 	11, /*!< DS18B20 11 bits resolution */
  TM_DS18B20_Resolution_12bits = 	12  /*!< DS18B20 12 bits resolution */
} TM_DS18B20_Resolution_t;

driver_error_t *ds1820_setup(sensor_instance_t *unit);
driver_error_t *ds1820_acquire(sensor_instance_t *unit, sensor_value_t *values);
driver_error_t *ds1820_set(sensor_instance_t *unit, const char *id, sensor_value_t *property);
driver_error_t *ds1820_get(sensor_instance_t *unit, const char *id, sensor_value_t *property);

unsigned char TM_DS18B20_Is(unsigned char *ROM);

void ds1820_getrom(sensor_instance_t *unit, char *ROM);
void ds1820_gettype(sensor_instance_t *unit, char *buf);
uint8_t ds1820_get_res(sensor_instance_t *unit);
void ow_list(sensor_instance_t *unit);
uint8_t ds1820_numdev(sensor_instance_t *unit);
