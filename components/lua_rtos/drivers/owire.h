/**
 * ONE WIRE driver for Lua-RTOS-ESP32
 * author: LoBo (loboris@gmail.com)
 * based on TM_ONEWIRE (author  Tilen Majerle)
 */

#include "luartos.h"

#ifndef _OWIRE_H_
#define _OWIRE_H_

#if USE_OWIRE

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "rom/ets_sys.h"
#include <sys/driver.h>
#include <drivers/cpu.h>

// Resources used by ONE WIRE
typedef struct {
	uint8_t pin;
} owire_resources_t;

// ONE WIRE driver errors
#define OWIRE_ERR_CANT_INIT                (DRIVER_EXCEPTION_BASE(OWIRE_DRIVER_ID) |  0)
#define OWIRE_ERR_INVALID_CHANNEL          (DRIVER_EXCEPTION_BASE(OWIRE_DRIVER_ID) |  1)

/* OneWire commands */
#define ONEWIRE_CMD_RSCRATCHPAD		0xBE
#define ONEWIRE_CMD_WSCRATCHPAD		0x4E
#define ONEWIRE_CMD_CPYSCRATCHPAD	0x48
#define ONEWIRE_CMD_RECEEPROM		0xB8
#define ONEWIRE_CMD_RPWRSUPPLY		0xB4
#define ONEWIRE_CMD_SEARCHROM		0xF0
#define ONEWIRE_CMD_READROM			0x33
#define ONEWIRE_CMD_MATCHROM		0x55
#define ONEWIRE_CMD_SKIPROM			0xCC

#define MAX_ONEWIRE_PINS 4			// Maximum number of buses (pins) to be used for owire
#define MAX_ONEWIRE_SENSORS 8		// Maximum number of devices on one owire bus (gpio)

typedef struct {
	int 		  pin;          			// GPIO Pin to be used for I/O functions
	unsigned char LastDiscrepancy;       	// Search private
	unsigned char LastFamilyDiscrepancy; 	// Search private
	unsigned char LastDeviceFlag;        	// Search private
	unsigned char ROM_NO[8];             	// 8-bytes address of last search device
} TM_One_Wire_t;

typedef struct {
	TM_One_Wire_t	device;
	uint8_t			numdev;
	uint8_t			roms[MAX_ONEWIRE_SENSORS][8];
} TM_One_Wire_Devices_t;

TM_One_Wire_Devices_t ow_devices[MAX_ONEWIRE_PINS];

unsigned char TM_OneWire_ReadBit(uint8_t dev);
void TM_OneWire_GetFullROM(uint8_t dev, unsigned char *firstIndex);
unsigned char TM_OneWire_First(uint8_t dev);
unsigned char TM_OneWire_Next(uint8_t dev);
unsigned char TM_OneWire_Reset(uint8_t dev);
unsigned char TM_OneWire_Search(uint8_t dev, unsigned char command);
void owdevice_input(uint8_t dev);
void owdevice_pinpower(uint8_t dev);
void TM_OneWire_SelectWithPointer(uint8_t dev, unsigned char *ROM);
unsigned char TM_OneWire_CRC8(unsigned char *addr, unsigned char len);
void TM_OneWire_WriteByte(uint8_t dev, unsigned char byte);
unsigned char TM_OneWire_ReadByte(uint8_t dev);
driver_error_t *owire_setup_pin(int8_t pin);
int owire_checkpin(uint8_t pin);
TM_One_Wire_Devices_t *ow_getdevice(uint8_t dev);
void ow_devices_init(uint8_t dev);
uint8_t TM_OneWire_Dosearch(uint8_t dev);
int8_t owire_addess_to_dev(uint8_t sensor, uint64_t address);

#endif
#endif /* _OWIRE_H_ */
