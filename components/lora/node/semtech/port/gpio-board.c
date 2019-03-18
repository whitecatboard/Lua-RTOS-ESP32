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

#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include "gpio-board.h"
#include "sx1276-board.h"

#include <sys/delay.h>

#include <sys/drivers/gpio.h>
#include <sys/drivers/spi.h>
#include <sys/drivers/power_bus.h>

int SpiDevice(Spi_t *obj);

typedef struct {
	uint8_t dio;
} dio_deferred_t;

static xQueueHandle dio_q = NULL;
static TaskHandle_t dio_t = NULL;
static GpioIrqHandler *dio_handler[6];

static void IRAM_ATTR dio_isr(void *arg) {
	int dio = (int)arg;
	dio_deferred_t data;

	portBASE_TYPE high_priority_task_awoken = 0;

	data.dio = dio;
	xQueueSendFromISR(dio_q, &data, &high_priority_task_awoken);

    if (high_priority_task_awoken == pdTRUE) {
        portYIELD_FROM_ISR();
    }
}

static void dio_task(void *arg) {
	dio_deferred_t data;

    for(;;) {
        xQueueReceive(dio_q, &data, portMAX_DELAY);
		dio_handler[data.dio](NULL);
    }
}

void GpioMcuInit( Gpio_t *obj, PinNames pin, PinModes mode, PinConfigs config, PinTypes type, uint32_t value )
{
    obj->pin = pin;

    // Special cases
    if (SX1276.Reset.pin == obj->pin) {
    	// pin is reset pin
		#if (CONFIG_LUA_RTOS_POWER_BUS_PIN >= 0)
			if (SX1276.Reset.pin >= 0) {
				pwbus_on();
			} else {
				if (value == 0) {
					pwbus_off();
					delay(1);
					pwbus_on();
					delay(5);

					return;
				}
			}
		#endif
    } else if (SX1276.Spi.Nss.pin == obj->pin) {
        // pun is NSS, then select / deselect SPI
    	if (value == 0) {
    		spi_ll_select(SpiDevice(&SX1276.Spi));
    	} else {
    		spi_ll_deselect(SpiDevice(&SX1276.Spi));
    	}

    	return;
    } else {
    	if (obj->pin < 0) {
    	    // Not connected
    		return;
    	}
    }

    // Set pin mode
    if (mode == PIN_INPUT) {
		gpio_pin_input(obj->pin);
	} else if (mode == PIN_OUTPUT) {
		gpio_pin_output(obj->pin);
	}

    // Set pull configuration
    if (type == PIN_NO_PULL) {
    	gpio_pin_nopull(obj->pin);
    } else if (type == PIN_PULL_UP) {
    	gpio_pin_pullup(obj->pin);
    } else if (type == PIN_PULL_DOWN) {
    	gpio_pin_pulldwn(obj->pin);
    }

    // Set initial value
    if (mode == PIN_OUTPUT) {
    	GpioMcuWrite( obj, value );
    }
}

void GpioMcuSetInterrupt( Gpio_t *obj, IrqModes irqMode, IrqPriorities irqPriority, GpioIrqHandler *GpioIrqHandler )
{
	driver_error_t *error;
	gpio_int_type_t type;
	int dio;

    if (obj->pin < 0) {
        // Not connected
    	return;
    }

    // If pin corresponds to a DIO pin, get the DIO number
    dio = -1;
    if (obj->pin == SX1276.DIO0.pin) {
    	dio = 0;
    } else if (obj->pin == SX1276.DIO1.pin) {
    	dio = 1;
    } else if (obj->pin == SX1276.DIO2.pin) {
    	dio = 2;
    } else if (obj->pin == SX1276.DIO3.pin) {
    	dio = 3;
    } else if (obj->pin == SX1276.DIO4.pin) {
    	dio = 4;
    } else if (obj->pin == SX1276.DIO5.pin) {
    	dio = 5;
    }

    if (irqMode == IRQ_RISING_EDGE) {
    	type = GPIO_INTR_POSEDGE;
    } else if (irqMode == IRQ_FALLING_EDGE) {
    	type = GPIO_INTR_NEGEDGE;
    } else if (irqMode == IRQ_RISING_FALLING_EDGE) {
    	type = GPIO_INTR_ANYEDGE;
    }

    if (dio >= 0) {
        // If pin corresponds to a DIO pin, use a deferred interrupt
    	dio_handler[dio] = GpioIrqHandler;

		// Create queue if not created
		if (!dio_q) {
			dio_q = xQueueCreate(10, sizeof(dio_deferred_t));
			if (!dio_q) {
				// TO DO
				return;
			}
		}

		// Create task if not created
		if (!dio_t) {
			BaseType_t xReturn;

			xReturn = xTaskCreatePinnedToCore(dio_task, "dio", CONFIG_LUA_RTOS_LUA_THREAD_STACK_SIZE, NULL, CONFIG_LUA_RTOS_LUA_THREAD_PRIORITY, &dio_t, xPortGetCoreID());
			if (xReturn != pdPASS) {
				// TO DO
				return;
			}

	        if ((error = gpio_isr_attach(obj->pin, dio_isr, type, (void* )dio))) {
	        	// TO DO
	        }
		}
    } else {
        if ((error = gpio_isr_attach(obj->pin, (gpio_isr_t)GpioIrqHandler, type, (void*)0))) {
        	// TO DO
        }
    }
}

void GpioMcuRemoveInterrupt( Gpio_t *obj )
{
	driver_error_t *error;

	if (obj->pin < 0) {
        // Not connected
    	return;
    }

    if ((error = gpio_isr_detach(obj->pin))) {
    	// TO DO
    }
}

void GpioMcuWrite( Gpio_t *obj, uint32_t value )
{
    if (obj->pin < 0) {
        // Not connected
    	return;
    }

    // Special cases
    if (SX1276.Spi.Nss.pin == obj->pin) {
		// pun is NSS, then select / deselect SPI
		if (value == 0) {
			spi_ll_select(SpiDevice(&SX1276.Spi));
		} else {
			spi_ll_deselect(SpiDevice(&SX1276.Spi));
		}

		return;
    }

    if (value == 0) {
		gpio_pin_set(obj->pin);
	} else {
		gpio_pin_clr(obj->pin);
	}
}

void GpioMcuToggle( Gpio_t *obj )
{
    if (obj->pin < 0) {
        // Not connected
    	return;
    }
}

uint32_t GpioMcuRead( Gpio_t *obj )
{
	uint8_t value;

    if (obj->pin < 0) {
        // Not connected
    	return 0;
    }

    gpio_pin_get(obj->pin, &value);

	return value;
}

void EXTI0_IRQHandler( void )
{
#if 0
#if !defined( USE_NO_TIMER )
    RtcRecoverMcuStatus( );
#endif
    HAL_GPIO_EXTI_IRQHandler( GPIO_PIN_0 );
#endif
}

void EXTI1_IRQHandler( void )
{
#if 0
#if !defined( USE_NO_TIMER )
    RtcRecoverMcuStatus( );
#endif
    HAL_GPIO_EXTI_IRQHandler( GPIO_PIN_1 );
#endif
}

void EXTI2_IRQHandler( void )
{
#if 0
#if !defined( USE_NO_TIMER )
    RtcRecoverMcuStatus( );
#endif
    HAL_GPIO_EXTI_IRQHandler( GPIO_PIN_2 );
#endif
}

void EXTI3_IRQHandler( void )
{
#if 0

	#if !defined( USE_NO_TIMER )
    RtcRecoverMcuStatus( );
#endif
    HAL_GPIO_EXTI_IRQHandler( GPIO_PIN_3 );
#endif
}

void EXTI4_IRQHandler( void )
{
#if 0
#if !defined( USE_NO_TIMER )
    RtcRecoverMcuStatus( );
#endif
    HAL_GPIO_EXTI_IRQHandler( GPIO_PIN_4 );
#endif
}

void EXTI9_5_IRQHandler( void )
{
#if 0

#if !defined( USE_NO_TIMER )
    RtcRecoverMcuStatus( );
#endif
    HAL_GPIO_EXTI_IRQHandler( GPIO_PIN_5 );
    HAL_GPIO_EXTI_IRQHandler( GPIO_PIN_6 );
    HAL_GPIO_EXTI_IRQHandler( GPIO_PIN_7 );
    HAL_GPIO_EXTI_IRQHandler( GPIO_PIN_8 );
    HAL_GPIO_EXTI_IRQHandler( GPIO_PIN_9 );
#endif
}

void EXTI15_10_IRQHandler( void )
{
#if 0

#if !defined( USE_NO_TIMER )
    RtcRecoverMcuStatus( );
#endif
    HAL_GPIO_EXTI_IRQHandler( GPIO_PIN_10 );
    HAL_GPIO_EXTI_IRQHandler( GPIO_PIN_11 );
    HAL_GPIO_EXTI_IRQHandler( GPIO_PIN_12 );
    HAL_GPIO_EXTI_IRQHandler( GPIO_PIN_13 );
    HAL_GPIO_EXTI_IRQHandler( GPIO_PIN_14 );
    HAL_GPIO_EXTI_IRQHandler( GPIO_PIN_15 );
#endif
}

void HAL_GPIO_EXTI_Callback( uint16_t gpioPin )
{
#if 0

    uint8_t callbackIndex = 0;

    if( gpioPin > 0 )
    {
        while( gpioPin != 0x01 )
        {
            gpioPin = gpioPin >> 1;
            callbackIndex++;
        }
    }

    if( GpioIrq[callbackIndex] != NULL )
    {
        GpioIrq[callbackIndex]( );
    }
#endif
}
