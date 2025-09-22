/*
 *          /\       /\
 *         /  \_____/  \
 *        /_____________\
 *        W H I T E C A T
 *
 * Copyright (C) 2015 - 2020, IBEROXARXA SERVICIOS INTEGRALES, S.L.
 * Copyright (C) 2015 - 2020, Jaume Olivé Petrus (jolive@whitecatboard.org)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "driver/rmt_common.h"
#include "esp_rom_sys.h"
#include "luartos.h"

#if CONFIG_LUA_RTOS_LUA_USE_STEPPER

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"

#include "driver.h"
#include "syslog.h"
#include "mutex.h"

#include "stepper.h"
#include "gpio.h"

#include <limits.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#include "driver/rmt_tx.h"
#include "driver/rmt_encoder.h"
#include "rmt_private.h"

#include "esp_log.h"
#include "esp_check.h"

#include "status.h"

#if !CONFIG_RMT_ISR_IRAM_SAFE
#error "Stepper requires CONFIG_RMT_ISR_IRAM_SAFE = 1. Please activate it with make menuconfig, enabling option in Driver Configurations → RMT Configuration -> RMT ISR IRAM-SafE."
#endif

#if !CONFIG_RMT_RECV_FUNC_IN_IRAM
#error "Stepper requires CONFIG_RMT_ISR_IRAM_SAFE = 1. Please activate it with make menuconfig, enabling option in Driver Configurations → RMT Configuration -> Place RMT receive function into IRAM."
#endif

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

typedef struct {
    uint8_t stepper;
    float units;
} stepper_oder_t;

typedef struct rmt_copy_encoder_t {
    rmt_encoder_t base;       // encoder base class
    stepper_t *stepper;
} stepper_encoder_t;

typedef struct {
    stepper_t *stepper;	
} stepper_encoder_config_t;

// Register driver and messages
static void stepper_init();

DRIVER_REGISTER_BEGIN(STEPPER,stepper,0,stepper_init,NULL);
    DRIVER_REGISTER_ERROR(STEPPER, stepper, NotEnoughtMemory, "not enough memory", STEPPER_ERR_NOT_ENOUGH_MEMORY);
    DRIVER_REGISTER_ERROR(STEPPER, stepper, InvalidUnit, "invalid unit", STEPPER_ERR_INVALID_UNIT);
    DRIVER_REGISTER_ERROR(STEPPER, stepper, NoMoreUnits, "no more units available", STEPPER_ERR_NO_MORE_UNITS);
    DRIVER_REGISTER_ERROR(STEPPER, stepper, UnitNotSetup, "unit is not setup", STEPPER_ERR_UNIT_NOT_SETUP);
    DRIVER_REGISTER_ERROR(STEPPER, stepper, InvalidPin, "invalid pin", STEPPER_ERR_INVALID_PIN);
    DRIVER_REGISTER_ERROR(STEPPER, stepper, InvalidDirection, "invalid direction", STEPPER_ERR_INVALID_DIRECTION);
    DRIVER_REGISTER_ERROR(STEPPER, stepper, InvalidAcceleration, "invalid acceleration", STEPPER_ERR_INVALID_ACCELERATION);
    DRIVER_REGISTER_ERROR(STEPPER, stepper, NoMoreRMT, "rmt no more channels available", STEPPER_RMT_ERR_NO_MORE_RMT);
    DRIVER_REGISTER_ERROR(STEPPER, stepper, NotSupported, "nrmt ot supported", STEPPER_RMT_ERR_NOT_SUPPORTED);
    DRIVER_REGISTER_ERROR(STEPPER, stepper, Fail, "rmt fail", STEPPER_RMT_ERR_FAIL);
DRIVER_REGISTER_END(STEPPER,stepper,0,stepper_init,NULL);

static stepper_t stepper[NSTEP];

// Acceleration profile task handle
static TaskHandle_t acceleration_profile_task_h = NULL;

// Steppers who are currently active
_Atomic static uint32_t active_mask = 0;

static struct mtx stepper_mutex;

static portMUX_TYPE spinlock = portMUX_INITIALIZER_UNLOCKED;

static EventGroupHandle_t move_event_group = NULL;

/*
 * Helper functions
 */

static size_t IRAM_ATTR encoder_copy(rmt_encoder_t *encoder, rmt_channel_handle_t channel,
                                     const void *primary_data, size_t data_size, rmt_encode_state_t *ret_state)
{
    rmt_tx_channel_t *tx_chan = __containerof(channel, rmt_tx_channel_t, base);
    stepper_encoder_t *rmt_encoder = __containerof(encoder, stepper_encoder_t, base);
    
    stepper_t *pstepper =  rmt_encoder->stepper;
     
    rmt_symbol_word_t *symbols = (rmt_symbol_word_t *)primary_data;
    rmt_encode_state_t state = RMT_ENCODING_RESET;

    // Get how many symbols should be copied from stepper buffer
    size_t symbols_want = (data_size / 4);

    // Get how many symbols are available in the stepper buffer
    size_t symbols_available = 0;
    
    uint32_t tail = atomic_load(&pstepper->rmt_data_tail);
    uint32_t head = atomic_load(&pstepper->rmt_data_head);
            
    if (tail < head) {
		symbols_available = head - tail;
	} else if (tail > head) {
		symbols_available = STEPPER_RMT_DATA_SIZE - tail + head;
	}
	
	// Adjust how many symbols should be copied from stepper buffer
	if (symbols_want > symbols_available) {
		symbols_want = symbols_available;
	}

    // Get how many symbols can be copied to internal RMT memory
    size_t symbols_have = tx_chan->mem_end - tx_chan->mem_off;

    // Get reference to internal RMT memory
    rmt_symbol_word_t *mem_to_nc = channel->hw_mem_base;
    
    // Get how many symbols will copied in this round
    size_t symbols_to_copy = MIN(symbols_want, symbols_have);

	// Copy all symbols to internal RMT memory until all symbols have
	// been copied or a termination condition has been found
    rmt_symbol_word_t symbol;
	size_t symbols_copied;
    
    symbols_copied = 0;
    symbol = symbols[tail];

    while ((symbols_to_copy > 0) && (symbol.val != 0)) {
		// Copy current item
		mem_to_nc[tx_chan->mem_off++] = symbol;
		tail = ((tail + 1) % (STEPPER_RMT_DATA_SIZE));

		// Next item			
        symbols_to_copy--;
        symbols_copied++;
        
		symbol = symbols[tail];
    }
    
    // Check if termination condition has been found
    if (symbol.val == 0) {
		// Mark as complete
        state = RMT_ENCODING_COMPLETE;	  
        
        // Mark stepper as not active
        atomic_fetch_and(&active_mask, ~(1 << pstepper->unit));	   
	}
    
    atomic_store(&pstepper->rmt_data_tail, tail);

    // Reset offset pointer when exceeds maximum range
    if (tx_chan->mem_off >= tx_chan->ping_pong_symbols * 2) {
        tx_chan->mem_off = 0;
    }
    
    if ((state & RMT_ENCODING_COMPLETE) == 0) {
		// Notify acceleration task to execute a new cycle because some symbols from the stepper buffer
		// have been consumed
	    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	    
	   	xTaskNotifyFromISR(acceleration_profile_task_h, 0, eSetValueWithOverwrite, &xHigherPriorityTaskWoken);
	
	    // Context switch if required
	    if(xHigherPriorityTaskWoken == pdTRUE) {
	        portYIELD_FROM_ISR();
	    }
	}

    *ret_state = state;
	    
    return symbols_copied;
}

static esp_err_t encoder_reset(rmt_encoder_t *encoder)
{
    return ESP_OK;
}

static esp_err_t encoder_del(rmt_encoder_t *encoder)
{
	stepper_encoder_t *copy_encoder = __containerof(encoder, stepper_encoder_t, base);
    free(copy_encoder);
    return ESP_OK;
}

static esp_err_t rmt_encoder(const stepper_encoder_config_t *config,  rmt_encoder_handle_t *ret_encoder)
{
    esp_err_t ret = ESP_OK;
    stepper_encoder_t *encoder = rmt_alloc_encoder_mem(sizeof(stepper_encoder_t));
    if (!encoder) {
    	return ESP_ERR_NO_MEM;
    }

    encoder->base.encode = encoder_copy;
    encoder->base.del = encoder_del;
    encoder->base.reset = encoder_reset;
    encoder->stepper = config->stepper;
    
    // return general encoder handle
    *ret_encoder = &encoder->base;

    return ret;
}

static int _cmp(const void *o1, const void *o2) {
    stepper_oder_t *ord1 = (stepper_oder_t *)o1;
    stepper_oder_t *ord2 = (stepper_oder_t *)o2;

    if (ord1->units < ord2->units) {
        return 1;
    } else if (ord1->units > ord2->units) {
        return -1;
    }

    return 0;
}

static void stepper_init() {
    mtx_init(&stepper_mutex, NULL, NULL, 0);
    memset(stepper,0,sizeof(stepper_t) * NSTEP);
}

static bool tx_cb(rmt_channel_handle_t channel, const rmt_tx_done_event_data_t *edata, void *args) {
    // Need to make a context switch at the end of the callback?
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    // Get stepper
    stepper_t *cstepper = (stepper_t *)args;

	// Get unit
	uint8_t unit = cstepper->unit;
	
	// Notify that the stepper movement has ended
	xEventGroupSetBitsFromISR(move_event_group, 1 << unit, &xHigherPriorityTaskWoken);
	
	return xHigherPriorityTaskWoken;
}

static driver_error_t *stepper_check(esp_err_t err) {
	switch (err) {
		case ESP_ERR_NO_MEM: return driver_error(STEPPER_DRIVER, STEPPER_ERR_NOT_ENOUGH_MEMORY, NULL);
		case ESP_ERR_NOT_FOUND: return driver_error(STEPPER_DRIVER, STEPPER_RMT_ERR_NO_MORE_RMT, NULL);
		case ESP_ERR_NOT_SUPPORTED: return driver_error(STEPPER_DRIVER, STEPPER_RMT_ERR_NOT_SUPPORTED, NULL);
		case ESP_FAIL: return driver_error(STEPPER_DRIVER, STEPPER_RMT_ERR_FAIL, NULL);
	}

	return NULL;
}

static void acceleration_profile_task(void *args) {
    stepper_t *pstepper;         // Current stepper in cycle
    uint32_t cycle_for_mask;     // Stepper involved in cycle
    uint32_t cycle_mask;         // A copy of cycle_for_mask
    rmt_symbol_word_t rmt_item;	 // RMT item for current step

    while (true) {
		BaseType_t notifiedValue = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        
        cycle_for_mask = atomic_load(&active_mask);
        
        // Run a new cycle with the involved steppers
        pstepper = stepper;
        cycle_mask = cycle_for_mask;

        while (cycle_mask) {
            if (cycle_mask & 0x01) {
                // Prepare next RMT buffer
                uint32_t tail = atomic_load(&pstepper->rmt_data_tail);
				uint32_t head = atomic_load(&pstepper->rmt_data_head);
                uint32_t next = ((head + 1) % (STEPPER_RMT_DATA_SIZE));

                if ((pstepper->steps == 0) && (next != tail)) {
					// Termination condition
					pstepper->rmt_data[head] = 0;
						
					// Mark stepper as not active
					atomic_fetch_and(&active_mask, ~(1 << pstepper->unit));

                    // Next stepper
                    cycle_mask = cycle_mask >> 1;
                    pstepper++;

                    continue;
				}

                while ((pstepper->steps > 0) && (next != tail)) {
                    // In each RMT entry we have 1 bit level, and 15 bits for period, so in each item we can represent
                    // STEPPER_RMT_MAX_DURATION tick-period.
                    uint32_t rmt_ticks = 0;
                    uint8_t first;

                    if (pstepper->rmt_ticks_remain == 0) {
                        // Compute RMT ticks for next step
                    	first = 1;

                    	pstepper->rmt_wanted_ticks = ((motion_next(&pstepper->motion) * 1000000000.0) / (float)STEPPER_RMT_NANOS_PER_TICK);
                        pstepper->rmt_ticks = floor(pstepper->rmt_wanted_ticks);
                        pstepper->rmt_missing_ticks += (pstepper->rmt_wanted_ticks - pstepper->rmt_ticks);
                        
                        rmt_ticks = pstepper->rmt_ticks;
                    } else {
                    	first = 0;
                        rmt_ticks = pstepper->rmt_ticks_remain;
                    }

                    while (pstepper->rmt_missing_ticks >= 1.0) {
                    	rmt_ticks++;
                    	pstepper->rmt_missing_ticks -= 1.0;
                    }
                    
                    while (rmt_ticks > 0) {
                        if (first) {
                            // Step signal -> H for 1 usec
                            rmt_item.level0 = 1;
                            rmt_item.duration0 = STEPPER_PULSE_TICKS;

                            // Step signal -> L for current step period - 1 usec
                            rmt_item.level1 = 0;

                            if (rmt_ticks - STEPPER_PULSE_TICKS < STEPPER_RMT_MAX_DURATION) {
                                rmt_item.duration1 = rmt_ticks - STEPPER_PULSE_TICKS;
                                rmt_ticks = 0;
                                pstepper->rmt_ticks_remain = 0;
                            } else {
                                rmt_item.duration1 = STEPPER_RMT_MAX_DURATION;
                                rmt_ticks -= STEPPER_RMT_MAX_DURATION;
                            }

                            // Write to RMT buffer
                            pstepper->rmt_data[head] = rmt_item.val;

                            // Advance
                            head = next;
                            next = ((head + 1) % (STEPPER_RMT_DATA_SIZE));
                        } else {
                            // We need more RMT items
                            // Enough space in buffer?
                            if (next != tail) {
                                // Enough space

                                // Now we can use level0 and level1
                                if (rmt_ticks < (STEPPER_RMT_MAX_DURATION << 1)) {
                                    // Enough space for this time using level0 and level1
                                    rmt_item.level0 = 0;
                                    rmt_item.duration0 = (rmt_ticks >> 1);
                                    rmt_ticks -= rmt_item.duration0;

                                    rmt_item.level1 = 0;
                                    rmt_item.duration1 = rmt_ticks;

                                    rmt_ticks = 0;
                                    pstepper->rmt_ticks_remain = 0;
                                } else {
                                    // Not enough space
                                    rmt_item.level0 = 0;
                                    rmt_item.duration0 = STEPPER_RMT_MAX_DURATION;
                                    rmt_ticks -= STEPPER_RMT_MAX_DURATION;

                                    rmt_item.level1 = 0;
                                    rmt_item.duration1 = STEPPER_RMT_MAX_DURATION;
                                    rmt_ticks -= STEPPER_RMT_MAX_DURATION;
                                }

                                // Write to RMT buffer
                                pstepper->rmt_data[head] = rmt_item.val;

                                // Advance
                                head = next;
                                next = ((head + 1) % (STEPPER_RMT_DATA_SIZE));
                            } else {
                                // No space in buffer, we need to wait for next iteration
                                pstepper->rmt_ticks_remain = rmt_ticks;
                                break;
                            }
                        }

                        first = 0;
                    }

                    if (pstepper->rmt_ticks_remain > 0) {
                        continue;
                    }

                    // Decrement steps
                    pstepper->steps--;
                }

                if (pstepper->steps == 0) {
                    // RMT end
                    if (next != tail) {
                        // Enough space in buffer?
                        // RMT end condition
                        pstepper->rmt_data[head] = 0;

                        // Advance
                        head = next;
                    } else {
                    	// Not enough space in buffer, wait for next cycle
                    }

                    pstepper->rmt_missing_ticks = 0;
                }
                
                
                atomic_store(&pstepper->rmt_data_head, head);
            }

            // Next stepper
            cycle_mask = cycle_mask >> 1;
            pstepper++;
        }

        // Start, RMT, if required
        if (notifiedValue == 1) {
			// Steppers are started in a critical section to ensure that no context-switching
			// happens, avoiding delays in the start
		   portENTER_CRITICAL(&spinlock);

	        cycle_mask = cycle_for_mask;
	        pstepper = stepper;
	
	        while (cycle_mask) {
	            if (cycle_mask & 0x01) {
	                rmt_transmit_config_t tx_config = {
	                    .loop_count = 0,
	                };
	
	        		rmt_transmit(pstepper->tx_chan, pstepper->tx_encoder, pstepper->rmt_data, STEPPER_RMT_BUFF_SIZE * 4, &tx_config);
	            }
	
	            cycle_mask = cycle_mask >> 1;
	            pstepper++;
	        }
	        
		    portEXIT_CRITICAL(&spinlock);
        }        
    }
}

/*
 * Operation functions
 */
driver_error_t *stepper_setup(uint8_t step_pin, uint8_t dir_pin, float min_spd, float max_spd, float max_acc, float stpu, uint8_t *unit) {
    driver_error_t *error;
#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
    driver_unit_lock_error_t *lock_error = NULL;
#endif
    int i;

    // Sanity checks
    if (step_pin > 31 ) {
        return driver_error(STEPPER_DRIVER, STEPPER_ERR_INVALID_PIN, "must be between 0 and 31");
    }

    mtx_lock(&stepper_mutex);

    // Find a free unit
    for(i = 0; i < NSTEP; i++) {
        if (!stepper[i].setup) {
            *unit = i;
            break;
        }
    }

    if (i > NSTEP) {
        // No free unit
        mtx_unlock(&stepper_mutex);
        return driver_error(STEPPER_DRIVER, STEPPER_ERR_NO_MORE_UNITS, NULL);
    }

    // Create RMT data circular buffer
    stepper[*unit].rmt_data = calloc(STEPPER_RMT_DATA_SIZE, sizeof(uint32_t));
    if (stepper[*unit].rmt_data == NULL) {
        mtx_unlock(&stepper_mutex);
        return driver_error(STEPPER_DRIVER, STEPPER_ERR_NOT_ENOUGH_MEMORY, NULL);
    }

	atomic_init(&active_mask, 0);
	atomic_init(&stepper[*unit].rmt_data_tail, 0);
	atomic_init(&stepper[*unit].rmt_data_head, 0);

    // Create acceleration task if not created yet
    if (acceleration_profile_task_h == NULL) {
		int task_core_id = (esp_cpu_get_core_id() + 1) % 2;				

        if (xTaskCreatePinnedToCore(acceleration_profile_task, "stepper_accel", 1024, NULL, configMAX_PRIORITIES - 1, &acceleration_profile_task_h, task_core_id) != pdTRUE) {
            mtx_unlock(&stepper_mutex);
            return driver_error(STEPPER_DRIVER, STEPPER_ERR_NOT_ENOUGH_MEMORY, NULL);
        }
        
	    syslog(LOG_INFO,"motion task at core %d", task_core_id);
    }

#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
    // Lock the step pin
    if ((lock_error = driver_lock(STEPPER_DRIVER, *unit, GPIO_DRIVER, step_pin, DRIVER_ALL_FLAGS, "STEP"))) {
        // Revoked lock on pin
        mtx_unlock(&stepper_mutex);
        return driver_lock_error(STEPPER_DRIVER, lock_error);
    }

    // Lock the dir pin
    if ((lock_error = driver_lock(STEPPER_DRIVER, *unit, GPIO_DRIVER, dir_pin, DRIVER_ALL_FLAGS, "DIR"))) {
        // Revoked lock on pin
        mtx_unlock(&stepper_mutex);
        return driver_lock_error(STEPPER_DRIVER, lock_error);
    }
#endif

    // Configure DIR pin, as output, initial low
    if ((error = gpio_pin_output(dir_pin))) {
        mtx_unlock(&stepper_mutex);
        return error;
    }

    gpio_ll_pin_clr(dir_pin);

    stepper[*unit].step_pin = step_pin;
    stepper[*unit].dir_pin = dir_pin;
    stepper[*unit].steps_per_unit = stpu;
    stepper[*unit].units_per_step = 1.0 / stpu;
    stepper[*unit].min_spd = min_spd;
    stepper[*unit].max_spd = max_spd;
    stepper[*unit].mac_acc = max_acc;
    stepper[*unit].setup = 1;
    stepper[*unit].unit = *unit;

    // Create TX channel
    rmt_tx_channel_config_t tx_chan_config = {
		.clk_src = RMT_CLK_SRC_DEFAULT, // select clock source
		.gpio_num = stepper[*unit].step_pin,
		.mem_block_symbols = STEPPER_RMT_BUFF_SIZE,
		.resolution_hz = STEPPER_RMT_FREQ,
		.trans_queue_depth = 1,
    };

    esp_err_t err;

    err = rmt_new_tx_channel(&tx_chan_config, &stepper[*unit].tx_chan);
    if ((error = stepper_check(err))) {
    	return error;
    }
    
    err = rmt_enable(stepper[*unit].tx_chan);
    if ((error = stepper_check(err))) {
    	return error;
    }

	stepper_encoder_config_t config = {0};
	
	config.stepper = &stepper[*unit];
	
    err = rmt_encoder(&config, &stepper[*unit].tx_encoder);
    if ((error = stepper_check(err))) {
    	return error;
    }

    // Install callbacks
    rmt_tx_event_callbacks_t cbs = {
        .on_trans_done = tx_cb,
    };

    err = rmt_tx_register_event_callbacks(stepper[*unit].tx_chan, &cbs, &stepper[*unit]);
    if ((err != ESP_ERR_INVALID_STATE) && (error = stepper_check(err))) {
    	return error;
    }

    if (!move_event_group) {
		move_event_group = xEventGroupCreate();
		if (move_event_group == NULL) {
			mtx_unlock(&stepper_mutex);
			return driver_error(STEPPER_DRIVER, STEPPER_ERR_NOT_ENOUGH_MEMORY, NULL);
		}
    }

    mtx_unlock(&stepper_mutex);

    syslog(LOG_INFO,"stepper%d, at core %d, pins step=%s%d, dir=%s%d", *unit,
    	   esp_cpu_get_core_id(),
           gpio_portname(step_pin),
           gpio_name(step_pin),
           gpio_portname(dir_pin),
           gpio_name(dir_pin));

    return NULL;
}

driver_error_t *stepper_move(uint8_t unit, float units, float initial_spd, float target_spd, float acc, float jerk) {
    // Sanity checks
    if (unit > NSTEP) {
        // Invalid unit
        return driver_error(STEPPER_DRIVER, STEPPER_ERR_INVALID_UNIT, NULL);
    }

    mtx_lock(&stepper_mutex);

    if (!stepper[unit].setup) {
        // Unit not setup
        mtx_unlock(&stepper_mutex);
        return driver_error(STEPPER_DRIVER, STEPPER_ERR_UNIT_NOT_SETUP, NULL);
    }

    stepper_t *pstepper = &stepper[unit];

    // Calculate direction
    pstepper->dir = (units >= 0.0);

    // Prepare motion
    motion_constraints_t constraints;

    constraints.accleration_profile = MotionSCurve;

    constraints.s_curve.v0 = initial_spd;
    constraints.s_curve.v = target_spd;
    constraints.s_curve.a = acc;
    constraints.s_curve.j = jerk;
    constraints.s_curve.s = fabs(units);
    constraints.s_curve.t = 0;
    constraints.s_curve.steps_per_unit = pstepper->steps_per_unit;
    constraints.s_curve.units_per_step = pstepper->units_per_step;

    motion_prepare(&constraints, &pstepper->motion);

    stepper[unit].steps = floor(fabs(units) * pstepper->steps_per_unit);
    stepper[unit].steps_request = stepper[unit].steps;
    stepper[unit].units = fabs(units);

    stepper[unit].rmt_missing_ticks = 0.0;
    stepper[unit].rmt_ticks_remain = 0;
    
    atomic_store(&pstepper->rmt_data_head, 0);
    atomic_store(&pstepper->rmt_data_tail, 0);

    mtx_unlock(&stepper_mutex);

    return NULL;
}

driver_error_t *stepper_get_distance(uint8_t unit, float* units) {
     // Sanity checks
    if (unit > NSTEP) {
        // Invalid unit
        return driver_error(STEPPER_DRIVER, STEPPER_ERR_INVALID_UNIT, NULL);
    }

    mtx_lock(&stepper_mutex);

    if (!stepper[unit].setup) {
        // Unit not setup
        mtx_unlock(&stepper_mutex);
        return driver_error(STEPPER_DRIVER, STEPPER_ERR_UNIT_NOT_SETUP, NULL);
    }

    stepper_t *pstepper = &stepper[unit];
    *units = stepper[unit].steps * pstepper->units_per_step;

    mtx_unlock(&stepper_mutex);
    return NULL;
}

driver_error_t *stepper_set_position(uint8_t unit, float units) {
     // Sanity checks
    if (unit > NSTEP) {
        // Invalid unit
        return driver_error(STEPPER_DRIVER, STEPPER_ERR_INVALID_UNIT, NULL);
    }

    mtx_lock(&stepper_mutex);

    if (!stepper[unit].setup) {
        // Unit not setup
        mtx_unlock(&stepper_mutex);
        return driver_error(STEPPER_DRIVER, STEPPER_ERR_UNIT_NOT_SETUP, NULL);
    }

    stepper_t *pstepper = &stepper[unit];
    pstepper->pos = units * pstepper->units_per_step;

    mtx_unlock(&stepper_mutex);
    return NULL;
}

driver_error_t *stepper_get_position(uint8_t unit, float* units) {
     // Sanity checks
    if (unit > NSTEP) {
        // Invalid unit
        return driver_error(STEPPER_DRIVER, STEPPER_ERR_INVALID_UNIT, NULL);
    }

    mtx_lock(&stepper_mutex);

    if (!stepper[unit].setup) {
        // Unit not setup
        mtx_unlock(&stepper_mutex);
        return driver_error(STEPPER_DRIVER, STEPPER_ERR_UNIT_NOT_SETUP, NULL);
    }

    stepper_t *pstepper = &stepper[unit];
    *units = pstepper->pos * pstepper->units_per_step;

    mtx_unlock(&stepper_mutex);
    return NULL;
}

driver_error_t *stepper_is_running(uint8_t unit, uint32_t* running) {
	// Sanity checks
    if (unit > NSTEP) {
        // Invalid unit
        return driver_error(STEPPER_DRIVER, STEPPER_ERR_INVALID_UNIT, NULL);
    }

	// Get current active mask
	uint32_t current_active_mask = atomic_load(&active_mask);

    *running = current_active_mask & (1<<unit) ;
    
    return NULL;
}

void stepper_start(int mask, uint8_t async) {  
    mtx_lock(&stepper_mutex);

    // For each required stepper set its direction pin if have steps request
    stepper_t *pstepper = stepper;
    int testMask = 0x01;
    uint8_t start_num = 0;

    while (testMask != (1 << (NSTEP - 1))) {
        if (mask & testMask) {
			if (pstepper->steps_request > 0) {
	            if (pstepper->dir) {
	                gpio_ll_pin_set(pstepper->dir_pin);
	            } else {
	                gpio_ll_pin_clr(pstepper->dir_pin);
	            }
	            start_num++;
            } else {
				// No steps request, so stepper is not required to be started
				mask &= ~testMask;
            }
        }
        
        // Next stepper
        testMask = testMask << 1;
        pstepper++;
    }
    
    if (start_num == 0) {
		// No steppers to start
		mtx_unlock(&stepper_mutex);
		
		return;
	} else  if (start_num > 1) {
		// Multiple steppers to start
		
        // Now, order involved stepper by displacement units
        stepper_oder_t *stepper_order;
        stepper_oder_t *pstepper_order;
        uint8_t stepper_id;
        stepper_order = calloc(start_num, sizeof(stepper_oder_t));
        assert(stepper_order != NULL);
        pstepper_order = stepper_order;

        testMask = 0x01;
        pstepper = stepper;
        stepper_id = 0;

        while (testMask != (1 << (NSTEP - 1))) {
            if (mask & testMask) {
                pstepper_order->stepper = stepper_id;
                pstepper_order->units = pstepper->units;
                pstepper_order++;
            }

            testMask = testMask << 1;
            pstepper++;
            stepper_id++;
        }

        qsort(stepper_order, start_num, sizeof(stepper_oder_t), _cmp);

        // Get total time for first stepper, this time is a constraint for the
        // other steppers involved in motion with different units
        float t = stepper[stepper_order[0].stepper].motion.s_curve.bound.total_t;
        pstepper = &stepper[stepper_order[0].stepper];

    	motion_dumnp(&pstepper->motion);

    	uint8_t order_id;

        for(order_id = 1;order_id < start_num;order_id++) {
            pstepper = &stepper[stepper_order[order_id].stepper];

            if (pstepper->motion.s_curve.s != stepper[stepper_order[0].stepper].motion.s_curve.s) {
                motion_constraint_t(&pstepper->motion, t);
            }

        	motion_dumnp(&pstepper->motion);
        }
        
        free(stepper_order);
    } else {
        stepper_t *pstepper = stepper;
        int testMask = 0x01;

        while (testMask != (1 << (NSTEP - 1))) {
            if (mask & testMask) {
            	motion_dumnp(&pstepper->motion);
            	break;
            }

            testMask = testMask << 1;
            pstepper++;
        }
    }

    mtx_unlock(&stepper_mutex);
    
    if (mask) {
		// Update active steppers
	    atomic_fetch_or(&active_mask, mask);

		// Notify acceleration task to start a new motion	    
	    xEventGroupClearBits(move_event_group, 0xff);
		xTaskNotify(acceleration_profile_task_h, 1, eSetValueWithOverwrite);
		
		if (!async) {
		    // Wait until movements done
		    xEventGroupWaitBits(move_event_group, mask, pdTRUE, pdTRUE, portMAX_DELAY);
		    
		    // Update active steppers, none active
		    atomic_store(&pstepper->rmt_data_tail, 0);		
		}
	}
}

void stepper_stop(int mask, uint8_t async) {
    stepper_t *pstepper = stepper;
    int testMask;
    int stopMask;
    
    // Stop required steppers as soon as possible, using a critical section, to avoid context-switch
    // while stopping
    portENTER_CRITICAL(&spinlock);

	// Get current active mask
	uint32_t current_active_mask = atomic_load(&active_mask);
	
    pstepper = stepper;
    testMask = 0x01;
    stopMask = 0x00;
	   
    while (testMask != (1 << (NSTEP - 1))) {
        if ((current_active_mask & testMask) && (mask & testMask)) {
			// Disable stepper temporary. This stops the transmission, and resets the internal
			// RMT state, but tx callback is not called, so we need to update the wait bits later.
			rmt_disable(pstepper->tx_chan);
			
			// Update stop mask
			stopMask |= (1 << pstepper->unit);
			
			// Movement end        
			atomic_fetch_and(&active_mask, ~(1 << pstepper->unit));  
        }

        testMask = testMask << 1;
        pstepper++;
    }
    
    // Enable disabled steppers
    pstepper = stepper;
    testMask = 0x01;
    while (testMask != (1 << (NSTEP - 1))) {
        if (stopMask & testMask) {
			rmt_enable(pstepper->tx_chan);
		}
		
        testMask = testMask << 1;
        pstepper++;
	}

    portEXIT_CRITICAL(&spinlock);
    
    if (stopMask) {
		// Notify movement end
		xEventGroupSetBits(move_event_group, stopMask);
	}
}
#endif
