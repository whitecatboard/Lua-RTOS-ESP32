/*
 * owire.h
 *
 *  Created on: Jan 16, 2017
 *      Author: jaumeolivepetrus
 */

#include "luartos.h"

#ifndef _OWIRE_H_
#define _OWIRE_H_

#if USE_OWIRE

/**
 * ONE WIRE driver for Lua-RTOS-ESP32
 * author: LoBo (loboris@gmail.com)
 * based on TM_ONEWIRE (author  Tilen Majerle)
 */

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

#define MAX_ONEWIRE_SENSORS 8

typedef struct {
	int 		  pin;          			// GPIO Pin to be used for I/O functions
	unsigned char LastDiscrepancy;       	// Search private
	unsigned char LastFamilyDiscrepancy; 	// Search private
	unsigned char LastDeviceFlag;        	// Search private
	unsigned char ROM_NO[8];             	// 8-bytes address of last search device
} TM_One_Wire_t;

TM_One_Wire_t OW_DEVICE;

unsigned char TM_OneWire_ReadBit();
void TM_OneWire_GetFullROM(unsigned char *firstIndex);
unsigned char TM_OneWire_First();
unsigned char TM_OneWire_Next();
unsigned char TM_OneWire_Reset();
int owdevice_input();
void TM_OneWire_SelectWithPointer(unsigned char *ROM);
unsigned char TM_OneWire_CRC8(unsigned char *addr, unsigned char len);
void TM_OneWire_WriteByte(unsigned char byte);
unsigned char TM_OneWire_ReadByte();
driver_error_t *owire_setup_pin(int8_t pin);

#endif

#endif /* _OWIRE_H_ */
