/*
 * Lua RTOS, encoder driver
 *
 * Copyright (C) 2015 - 2017
 * IBEROXARXA SERVICIOS INTEGRALES, S.L.
 *
 * Author: Jaume Olivé (jolive@iberoxarxa.com / jolive@whitecatboard.org)
 *
 * Licenced under the GNU GPL Version 3.
 *
 * Based on sources from: https://github.com/buxtronix/arduino/blob/master/libraries/Rotary
 */

/* Rotary encoder handler for arduino. v1.1
 *
 * Copyright 2011 Ben Buxton. Licenced under the GNU GPL Version 3.
 * Contact: bb@cactii.net
 *
 * A typical mechanical rotary encoder emits a two bit gray code
 * on 3 output pins. Every step in the output (often accompanied
 * by a physical 'click') generates a specific sequence of output
 * codes on the pins.
 *
 * There are 3 pins used for the rotary encoding - one common and
 * two 'bit' pins.
 *
 * The following is the typical sequence of code on the output when
 * moving from one step to the next:
 *
 *   Position   Bit1   Bit2
 *   ----------------------
 *     Step1     0      0
 *      1/4      1      0
 *      1/2      1      1
 *      3/4      0      1
 *     Step2     0      0
 *
 * From this table, we can see that when moving from one 'click' to
 * the next, there are 4 changes in the output code.
 *
 * - From an initial 0 - 0, Bit1 goes high, Bit0 stays low.
 * - Then both bits are high, halfway through the step.
 * - Then Bit1 goes low, but Bit2 stays high.
 * - Finally at the end of the step, both bits return to 0.
 *
 * Detecting the direction is easy - the table simply goes in the other
 * direction (read up instead of down).
 *
 * To decode this, we use a simple state machine. Every time the output
 * code changes, it follows state, until finally a full steps worth of
 * code is received (in the correct order). At the final 0-0, it returns
 * a value indicating a step in one direction or the other.
 *
 * It's also possible to use 'half-step' mode. This just emits an event
 * at both the 0-0 and 1-1 positions. This might be useful for some
 * encoders where you want to detect all positions.
 *
 * If an invalid state happens (for example we go from '0-1' straight
 * to '1-0'), the state machine resets to the start until 0-0 and the
 * next valid codes occur.
 *
 * The biggest advantage of using a state machine over other algorithms
 * is that this has inherent debounce built in. Other algorithms emit spurious
 * output with switch bounce, but this one will simply flip between
 * sub-states until the bounce settles, then continue along the state
 * machine.
 * A side effect of debounce is that fast rotations can cause steps to
 * be skipped. By not requiring debounce, fast rotations can be accurately
 * measured.
 * Another advantage is the ability to properly handle bad state, such
 * as due to EMI, etc.
 * It is also a lot simpler than others - a static state table and less
 * than 10 lines of logic.
 */

#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_attr.h"
#include "encoder.h"

#include <string.h>

#include <sys/status.h>
#include <sys/driver.h>

#include <drivers/cpu.h>
#include <drivers/encoder.h>
#include <drivers/gpio.h>

DRIVER_REGISTER_BEGIN(ENCODER, encoder, 0, NULL, NULL);
    // Driver message errors
    DRIVER_REGISTER_ERROR(ENCODER, encoder, NotEnoughtMemory, "not enough memory", ENCODER_ERR_NOT_ENOUGH_MEMORY);
    DRIVER_REGISTER_ERROR(ENCODER, encoder, InvalidPin, "invalid pin", ENCODER_ERR_INVALID_PIN);
DRIVER_REGISTER_END(ENCODER, encoder, 0, NULL, NULL);

/*
 * The below state table has, for each state (row), the new state
 * to set based on the next encoder output. From left to right in,
 * the table, the encoder outputs are 00, 01, 10, 11, and the value
 * in that position is the new state to set.
 */
// Values returned by 'process'
// No complete step yet.
#define DIR_NONE 0x0
// Clockwise step.
#define DIR_CW 0x10
// Anti-clockwise step.
#define DIR_CCW 0x20
#define R_START 0x0

#ifdef HALF_STEP
// Use the half-step state table (emits a code at 00 and 11)
#define R_CCW_BEGIN 0x1
#define R_CW_BEGIN 0x2
#define R_START_M 0x3
#define R_CW_BEGIN_M 0x4
#define R_CCW_BEGIN_M 0x5
const uint8_t ttable[6][4] = {
  // R_START (00)
  {R_START_M,            R_CW_BEGIN,     R_CCW_BEGIN,  R_START},
  // R_CCW_BEGIN
  {R_START_M | DIR_CCW, R_START,        R_CCW_BEGIN,  R_START},
  // R_CW_BEGIN
  {R_START_M | DIR_CW,  R_CW_BEGIN,     R_START,      R_START},
  // R_START_M (11)
  {R_START_M,            R_CCW_BEGIN_M,  R_CW_BEGIN_M, R_START},
  // R_CW_BEGIN_M
  {R_START_M,            R_START_M,      R_CW_BEGIN_M, R_START | DIR_CW},
  // R_CCW_BEGIN_M
  {R_START_M,            R_CCW_BEGIN_M,  R_START_M,    R_START | DIR_CCW},
};
#else
// Use the full-step state table (emits a code at 00 only)
#define R_CW_FINAL 0x1
#define R_CW_BEGIN 0x2
#define R_CW_NEXT 0x3
#define R_CCW_BEGIN 0x4
#define R_CCW_FINAL 0x5
#define R_CCW_NEXT 0x6

const uint8_t ttable[7][4] = {
  // R_START
  {R_START,    R_CW_BEGIN,  R_CCW_BEGIN, R_START},
  // R_CW_FINAL
  {R_CW_NEXT,  R_START,     R_CW_FINAL,  R_START | DIR_CW},
  // R_CW_BEGIN
  {R_CW_NEXT,  R_CW_BEGIN,  R_START,     R_START},
  // R_CW_NEXT
  {R_CW_NEXT,  R_CW_BEGIN,  R_CW_FINAL,  R_START},
  // R_CCW_BEGIN
  {R_CCW_NEXT, R_START,     R_CCW_BEGIN, R_START},
  // R_CCW_FINAL
  {R_CCW_NEXT, R_CCW_FINAL, R_START,     R_START | DIR_CCW},
  // R_CCW_NEXT
  {R_CCW_NEXT, R_CCW_FINAL, R_CCW_BEGIN, R_START},
};
#endif

static xQueueHandle queue = NULL;
static TaskHandle_t task = NULL;
static uint8_t attached = 0;

/*
 * Helper functions
 */

static void encoder_task(void *arg) {
	encoder_deferred_data_t data;

    for(;;) {
        xQueueReceive(queue, &data, portMAX_DELAY);

        if (data.h->callback) {
        	data.h->callback(data.h->callback_id, data.dir, data.counter, data.button);
        }
    }
}

static void IRAM_ATTR encoder_isr(void* arg) {
	encoder_h_t *encoder = (encoder_h_t *)arg;
	encoder_deferred_data_t data;
	uint8_t result, has_data;
	int8_t dir;

	has_data = 0;
	dir = 0;

	uint8_t A = gpio_ll_pin_get(encoder->A);
	uint8_t B = gpio_ll_pin_get(encoder->B);
	uint8_t SW;

	if (encoder->SW >= 0) {
		SW = gpio_ll_pin_get(encoder->SW);
	} else {
		SW = 1;
	}

	SW = (SW==0?1:0);

	// Grab state of input pins.
	uint8_t pinstate = (A << 1) | B;

	// Determine new state from the pins and state table.
	encoder->state = ttable[encoder->state & 0xf][pinstate];
	result = encoder->state & 0x30;

	if (result == DIR_CW) {
		encoder->counter++;
		has_data = 1;
		dir = 1;
	} else if (result == DIR_CCW) {
		encoder->counter--;
		has_data = 1;
		dir = -1;
	}

	has_data |= (SW != encoder->sw_latch);

	encoder->sw_latch = SW;

	if (has_data && encoder->callback) {
		if (encoder->deferred) {
			data.h = encoder;
			data.counter = encoder->counter;
			data.button = encoder->sw_latch;
			data.dir = dir;

			xQueueSendFromISR(queue, &data, NULL);
		} else {
			encoder->callback(encoder->callback_id, dir, encoder->counter, encoder->sw_latch);
		}
	}
}

/*
 * Operation functions
 */
driver_error_t *encoder_setup(int8_t a, int8_t b, int8_t sw, encoder_h_t **h) {
#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
	driver_unit_lock_error_t *lock_error = NULL;
#endif

	// Sanity checks
	if ((a < 0) && (b < 0)) {
		return driver_error(ENCODER_DRIVER, ENCODER_ERR_INVALID_PIN, "a and b pins are required");
	}

	if (!GPIO_CHECK_INPUT(a)) {
		return driver_error(ENCODER_DRIVER, ENCODER_ERR_INVALID_PIN, "A, must be an input PIN");
	}

	if (!GPIO_CHECK_INPUT(b)) {
		return driver_error(ENCODER_DRIVER, ENCODER_ERR_INVALID_PIN, "B, must be an input PIN");
	}

	if ((sw > 0) && !GPIO_CHECK_INPUT(sw)) {
		return driver_error(ENCODER_DRIVER, ENCODER_ERR_INVALID_PIN, "SW, must be an input PIN");
	}

#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
	// Lock resources
    if ((lock_error = driver_lock(ENCODER_DRIVER, 0, GPIO_DRIVER, a, 0, "A"))) {
    	// Revoked lock on pin
    	return driver_lock_error(ENCODER_DRIVER, lock_error);
    }

    if ((lock_error = driver_lock(ENCODER_DRIVER, 0, GPIO_DRIVER, b, 0, "B"))) {
    	// Revoked lock on pin
    	return driver_lock_error(ENCODER_DRIVER, lock_error);
    }

    if ((sw >= 0) && (lock_error = driver_lock(ENCODER_DRIVER, 0, GPIO_DRIVER, sw, 0, "SW"))) {
    	// Revoked lock on pin
    	return driver_lock_error(ENCODER_DRIVER, lock_error);
    }
#endif

    // Allocate space for the encoder
    encoder_h_t *encoder = calloc(1, sizeof(encoder_h_t));
    if (!encoder) {
		return driver_error(ENCODER_DRIVER, ENCODER_ERR_NOT_ENOUGH_MEMORY, NULL);
    }

    encoder->A = a;
    encoder->B = b;
    encoder->SW = sw;
    encoder->state = R_START;

    gpio_pin_input(a);
    gpio_pin_input(b);

    gpio_isr_attach(a, encoder_isr, GPIO_INTR_ANYEDGE, (void *)encoder);
    gpio_isr_attach(b, encoder_isr, GPIO_INTR_ANYEDGE, (void *)encoder);

    if (sw >= 0) {
        gpio_pin_input(sw);
        gpio_isr_attach(sw, encoder_isr, GPIO_INTR_ANYEDGE, (void *)encoder);
    }

    *h = encoder;

    attached++;

    return NULL;
}

driver_error_t *encoder_unsetup(encoder_h_t *h) {
	portDISABLE_INTERRUPTS();

	if (attached == 0) {
		portENABLE_INTERRUPTS();
		return NULL;
	}

	// Remove interrupts
	gpio_isr_detach(h->A);
#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
    driver_unlock(ENCODER_DRIVER, 0, GPIO_DRIVER, h->A);
#endif

    gpio_isr_detach(h->B);
#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
    driver_unlock(ENCODER_DRIVER, 0, GPIO_DRIVER, h->B);
#endif

	if (h->SW >= 0) {
		gpio_isr_detach(h->SW);
#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
		driver_unlock(ENCODER_DRIVER, 0, GPIO_DRIVER, h->SW);
#endif
	}

	if (attached == 1) {
		if (task) {
			vTaskDelete(task);
			task = NULL;
		}

		if (queue) {
			vQueueDelete(queue);
			queue = NULL;
		}
	}

	attached--;

	free(h);

	portENABLE_INTERRUPTS();

	return NULL;
}

driver_error_t *encoder_read(encoder_h_t *h, int32_t *val, uint8_t *sw) {
	portDISABLE_INTERRUPTS();
	*val = h->counter;
	*sw = h->sw_latch;
	portENABLE_INTERRUPTS();

	return NULL;
}

driver_error_t *encoder_write(encoder_h_t *h, int32_t val) {
	portDISABLE_INTERRUPTS();
	h->counter = val;
	portENABLE_INTERRUPTS();

	return NULL;
}

driver_error_t *encoder_register_callback(encoder_h_t *h, encoder_callback_t callback, int callback_id, uint8_t deferred) {
	portDISABLE_INTERRUPTS();

	h->callback = callback;
	h->callback_id = callback_id;
	h->deferred = deferred;

	if (deferred) {
		if (!queue) {
			queue = xQueueCreate(10, sizeof(encoder_deferred_data_t));
			if (!queue) {
				portENABLE_INTERRUPTS();
				return driver_error(ENCODER_DRIVER, ENCODER_ERR_NOT_ENOUGH_MEMORY, NULL);
			}
		}

		if (!task) {
			BaseType_t xReturn;

			xReturn = xTaskCreatePinnedToCore(encoder_task, "encoder", CONFIG_LUA_RTOS_LUA_THREAD_STACK_SIZE, NULL, CONFIG_LUA_RTOS_LUA_THREAD_PRIORITY, &task, xPortGetCoreID());
			if (xReturn != pdPASS) {
				portENABLE_INTERRUPTS();
				return driver_error(ENCODER_DRIVER, ENCODER_ERR_NOT_ENOUGH_MEMORY, NULL);
			}
		}
	}

	portENABLE_INTERRUPTS();

	return NULL;
}
