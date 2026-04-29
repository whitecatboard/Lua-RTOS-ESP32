/*!
 * \file      gpio-board.c
 *
 * \brief     Target board GPIO driver implementation
 *
 * \copyright Revised BSD License, see section \ref LICENSE.
 *
 * \code
 *                ______                              _
 *               / _____)             _              | |
 *              ( (____  _____ ____ _| |_ _____  ____| |__
 *               \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 *               _____) ) ____| | | || |_| ____( (___| | | |
 *              (______/|_____)_|_|_| \__)_____)\____)_| |_|
 *              (C)2013-2017 Semtech
 *
 * \endcode
 *
 * \author    Miguel Luis ( Semtech )
 *
 * \author    Gregory Cristian ( Semtech )
 */
#include "utilities.h"
#include "rtc-board.h"
#include "gpio-board.h"

#include "driver.h"

#include <drivers/gpio.h>
#include <drivers/spi.h>

#include "sx1276-board.h"

extern void _unblock_lora_taskFromISR(void *arg);
int SpiGetDevice(void);
	
static IRAM_ATTR void _gpio_isr(void *arg) {
	_unblock_lora_taskFromISR(arg);
}

void GpioMcuInit( Gpio_t *obj, PinNames pin, PinModes mode, PinConfigs config, PinTypes type, uint32_t value )
{
	driver_error_t *error;
	
	obj->pin = pin;
	obj->pull = type;
	
	// Configure input / output
	if (mode == PIN_INPUT) {
		error = gpio_pin_input(obj->pin);
		if (error) {
			goto driver_error;
		}
	} else if (mode == PIN_OUTPUT) {
		error = gpio_pin_output(obj->pin);
		if (error) {
			goto driver_error;
		}		
	}
	
	// Configure pull type
	if (type == PIN_PULL_UP) {
		error = gpio_pin_pullup(obj->pin);
		if (error) {
			goto driver_error;
		}
	} else if (type == PIN_PULL_DOWN) {
		error = gpio_pin_pulldwn(obj->pin);
		if (error) {
			goto driver_error;
		}
	} else {
		error = gpio_pin_nopull(obj->pin);
		if (error) {
			goto driver_error;
		}
	}
	
	// Set initial value
	GpioWrite(obj, value);
	
	return;
	
driver_error:
	driver_error_log_and_destroy(error);
}

void GpioMcuSetContext( Gpio_t *obj, void* context )
{
	obj->Context = context;
}

void GpioMcuSetInterrupt( Gpio_t *obj, IrqModes irqMode, IrqPriorities irqPriority, GpioIrqHandler *irqHandler )
{
	driver_error_t *error;
	
    error = gpio_isr_attach(obj->pin, _gpio_isr, irqMode, (void *)irqHandler);
    if (error) {
		goto driver_error;
	}
	
	return;
	
driver_error:
	driver_error_log_and_destroy(error);	
}

void GpioMcuRemoveInterrupt( Gpio_t *obj )
{
	driver_error_t *error;
	
	error = gpio_isr_detach(obj->pin);
    if (error) {
		goto driver_error;
	}
	
	return;
	
driver_error:
	driver_error_log_and_destroy(error);
}

void GpioMcuWrite( Gpio_t *obj, uint32_t value )
{
	if (value == 0) {
		gpio_ll_pin_clr(obj->pin);		
		
		if (obj->pin == SX1276.Spi.Nss.pin) {
			spi_ll_select(SpiGetDevice());	
		}
	} else {
		gpio_ll_pin_set(obj->pin);
		
		if (obj->pin == SX1276.Spi.Nss.pin) {
			spi_ll_deselect(SpiGetDevice());	
		}
	}
}

void GpioMcuToggle( Gpio_t *obj )
{
	gpio_ll_pin_inv(obj->pin);
}

uint32_t GpioMcuRead( Gpio_t *obj )
{
	uint8_t val;

	gpio_pin_get(obj->pin, &val);

	return (uint32_t)val;
}