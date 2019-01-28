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
 * Lua RTOS, sensor driver
 *
 */

#include "sdkconfig.h"

#if CONFIG_LUA_RTOS_LUA_USE_SENSOR

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include <time.h>
#include <string.h>

#include <sys/status.h>
#include <sys/list.h>
#include <sys/driver.h>
#include <sys/syslog.h>

#include <drivers/sensor.h>
#include <drivers/adc.h>
#include <drivers/adc_internal.h>
#include <drivers/gpio.h>
#include <drivers/owire.h>
#include <drivers/i2c.h>
#include <drivers/uart.h>
#include <drivers/power_bus.h>

// This variable is defined at linker time
extern const sensor_t sensors[];

// Register drivers and errors
DRIVER_REGISTER_BEGIN(SENSOR,sensor,0,NULL,NULL);
    DRIVER_REGISTER_ERROR(SENSOR, sensor, CannotSetup, "can't setup", SENSOR_ERR_CANT_INIT);
    DRIVER_REGISTER_ERROR(SENSOR, sensor, Timeout, "timeout", SENSOR_ERR_TIMEOUT);
    DRIVER_REGISTER_ERROR(SENSOR, sensor, NotEnoughtMemory, "not enough memory", SENSOR_ERR_NOT_ENOUGH_MEMORY);
    DRIVER_REGISTER_ERROR(SENSOR, sensor, SetupUndefined, "setup function is not defined", SENSOR_ERR_SETUP_UNDEFINED);
    DRIVER_REGISTER_ERROR(SENSOR, sensor, AcquireUndefined, "acquire function is not defined", SENSOR_ERR_ACQUIRE_UNDEFINED);
    DRIVER_REGISTER_ERROR(SENSOR, sensor, SetUndefined, "set function is not defined", SENSOR_ERR_SET_UNDEFINED);
    DRIVER_REGISTER_ERROR(SENSOR, sensor, NotFound, "not found", SENSOR_ERR_NOT_FOUND);
    DRIVER_REGISTER_ERROR(SENSOR, sensor, InterfaceNotSupported, "interface not supported", SENSOR_ERR_INTERFACE_NOT_SUPPORTED);
    DRIVER_REGISTER_ERROR(SENSOR, sensor, NotSetup, "sensor is not setup", SENSOR_ERR_NOT_SETUP);
    DRIVER_REGISTER_ERROR(SENSOR, sensor, InvalidAddress, "invalid address", SENSOR_ERR_INVALID_ADDRESS);
    DRIVER_REGISTER_ERROR(SENSOR, sensor, NoMoreCallbacks, "no more callbacks available", SENSOR_ERR_NO_MORE_CALLBACKS);
    DRIVER_REGISTER_ERROR(SENSOR, sensor, InvalidData, "invalid data", SENSOR_ERR_INVALID_DATA);
    DRIVER_REGISTER_ERROR(SENSOR, sensor, NoCallbacksAlowed, "callbacks not allowed for this sensor", SENSOR_ERR_CALLBACKS_NOT_ALLOWED);
    DRIVER_REGISTER_ERROR(SENSOR, sensor, InvalidValue, "invalid value", SENSOR_ERR_INVALID_VALUE);
    DRIVER_REGISTER_ERROR(SENSOR, sensor, SensorDetached, "sensor detached", SENSOR_ERR_DETACHED);
DRIVER_REGISTER_END(SENSOR,sensor,0,NULL,NULL);

static xQueueHandle queue = NULL;
static TaskHandle_t task = NULL;
static uint8_t attached = 0;
static uint8_t counter = 0;

/*
 * Helper functions
 */

static void sensor_task(void *arg) {
    sensor_deferred_data_t *data;

    data = calloc(1,sizeof(sensor_deferred_data_t));
    assert(data);

    for(;;) {
        xQueueReceive(queue, data, portMAX_DELAY);
        data->callback(data->callback_id, data->instance, data->data, data->latch);
    }
}

static void IRAM_ATTR debouncing(void *arg, uint8_t val) {
    // Get sensor instance
    sensor_instance_t *unit = ((sensor_setup_t *)arg)->instance;

    // Get interface
    uint8_t interface = ((sensor_setup_t *)arg)->interface;

    // Get property
    uint8_t property = SENSOR_FLAG_GET_PROPERTY(unit->sensor->interface[interface]);

    // Latch & store sensor data
    unit->latch[property].value.integerd.value = unit->data[property].integerd.value;

    if (val == 1) {
        unit->data[property].integerd.value = SENSOR_FLAG_GET_ON_H(unit->sensor->interface[interface]);
    } else if (val == 0) {
        unit->data[property].integerd.value = SENSOR_FLAG_GET_ON_L(unit->sensor->interface[interface]);
    } else {
        return;
    }

    sensor_queue_callbacks(unit, property, property);

    unit->latch[property].value.integerd.value = unit->data[property].integerd.value;
};

static void IRAM_ATTR isr(void* arg) {
    // Get sensor instance
    sensor_instance_t *unit = ((sensor_setup_t *)arg)->instance;

    // Get interface
    uint8_t interface = ((sensor_setup_t *)arg)->interface;

    // Get property
    uint8_t property = (unit->sensor->interface[interface].flags & 0xff00) >> 8;

    // Get pin value
    uint8_t val = gpio_ll_pin_get(unit->setup[interface].gpio.gpio);

    // Store sensor data
    unit->latch[property].value.integerd.value = unit->data[property].integerd.value;
    if (val == 1) {
        unit->data[property].integerd.value = SENSOR_FLAG_GET_ON_H(unit->sensor->interface[interface]);
    } else if (val == 0) {
        unit->data[property].integerd.value = SENSOR_FLAG_GET_ON_L(unit->sensor->interface[interface]);
    } else {
        return;
    }

    sensor_queue_callbacks(unit, property, property);
}

static driver_error_t *sensor_adc_setup(uint8_t interface, sensor_instance_t *unit) {
#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
    driver_unit_lock_error_t *lock_error = NULL;
#endif
    driver_error_t *error;

#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
    // Lock ADC channel
    if ((lock_error = driver_lock(SENSOR_DRIVER, unit->unit, ADC_DRIVER, unit->setup[interface].adc.channel, DRIVER_ALL_FLAGS, unit->sensor->id))) {
        // Revoked lock on ADC channel
        return driver_lock_error(SENSOR_DRIVER, lock_error);
    }
#endif

    if (
            (error = adc_setup(
                    unit->setup[interface].adc.unit, unit->setup[interface].adc.channel,
                    unit->setup[interface].adc.devid, 0,
                    unit->setup[interface].adc.max, unit->setup[interface].adc.resolution,
                    &unit->setup[interface].adc.h
            ))
        ) {
        return error;
    }

    return NULL;
}

static driver_error_t *sensor_gpio_setup(uint8_t interface, sensor_instance_t *unit) {
#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
    driver_unit_lock_error_t *lock_error = NULL;
#endif
    driver_error_t *error;

    // Sanity checks
    if (unit->setup[interface].gpio.gpio < 40) {
        // Pin ok
    }
    #if EXTERNAL_GPIO
    else {
        if (unit->setup[interface].gpio.gpio > CPU_LAST_GPIO) {
            return driver_error(GPIO_DRIVER, GPIO_ERR_INVALID_PIN, NULL);
        }
    }
    #else
    else {
        return driver_error(GPIO_DRIVER, GPIO_ERR_INVALID_PIN, NULL);
    }
    #endif

#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
    // Lock gpio
    if ((lock_error = driver_lock(SENSOR_DRIVER, unit->unit, GPIO_DRIVER, unit->setup[interface].gpio.gpio, DRIVER_ALL_FLAGS, unit->sensor->id))) {
        // Revoked lock on gpio
        return driver_lock_error(SENSOR_DRIVER, lock_error);
    }
#endif

    if (unit->sensor->interface[interface].flags & SENSOR_FLAG_ON_OFF) {
        if (unit->sensor->interface[interface].flags & SENSOR_FLAG_DEBOUNCING) {
            uint16_t threshold = SENSOR_FLAG_GET_DEBOUNCING_THRESHOLD(unit->sensor->interface[interface]);
            if ((error = gpio_debouncing_register(unit->setup[interface].gpio.gpio, threshold, debouncing, (void *)(&unit->setup[interface])))) {
                return error;
            }
        } else {
            if ((error = gpio_isr_attach(unit->setup[interface].gpio.gpio, isr, GPIO_INTR_ANYEDGE, (void *)(&unit->setup[interface])))) {
                return error;
            }
        }
    } else {
        if ((error = gpio_pin_output(unit->setup[interface].gpio.gpio))) {
            return error;
        }

        if ((error = gpio_pin_set(unit->setup[interface].gpio.gpio))) {
            return error;
        }
    }

    return NULL;
}

static driver_error_t *sensor_owire_setup(uint8_t interface, sensor_instance_t *unit) {
    driver_error_t *error;

    // By default we always can get sensor data
    gettimeofday(&unit->next, NULL);

    // Check if owire interface is setup on the given gpio
    int dev = owire_checkpin(unit->setup[interface].owire.gpio);
    if (dev < 0) {
        // setup new owire interface on given pin
        if ((error = owire_setup_pin(unit->setup[interface].owire.gpio))) {
              return error;
        }
        int dev = owire_checkpin(unit->setup[interface].owire.gpio);
        if (dev < 0) {
            return driver_error(SENSOR_DRIVER, SENSOR_ERR_CANT_INIT, NULL);
        }
        vTaskDelay(10 / portTICK_RATE_MS);
        owdevice_input(dev);
        ow_devices_init(dev);
        unit->setup[interface].owire.owdevice = dev;

        // Search for devices on owire bus
        TM_OneWire_Dosearch(dev);
    }
    else {
        unit->setup[interface].owire.owdevice = dev;
        TM_OneWire_Dosearch(dev);
    }

    // check if owire bus is setup
    if (ow_devices[unit->setup[interface].owire.owdevice].device.pin == 0) {
        return driver_error(SENSOR_DRIVER, SENSOR_ERR_CANT_INIT, NULL);
    }

    return NULL;
}

static driver_error_t *sensor_i2c_setup(uint8_t interface, sensor_instance_t *unit) {
    driver_error_t *error;
    int i2cdevice;

    if ((error = i2c_attach(unit->setup[interface].i2c.id, I2C_MASTER, unit->setup[interface].i2c.speed, 0, 0, &i2cdevice))) {
        return error;
    }

    unit->setup[interface].i2c.id = i2cdevice;

#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
    driver_unit_lock_error_t *lock_error = NULL;
    if ((lock_error = driver_lock(SENSOR_DRIVER, unit->unit, I2C_DRIVER, unit->setup[interface].i2c.id, DRIVER_ALL_FLAGS, unit->sensor->id))) {
        return driver_lock_error(SENSOR_DRIVER, lock_error);
    }
#endif

    return NULL;
}

static driver_error_t *sensor_gpio_unsetup(uint8_t interface, sensor_instance_t *unit) {
#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
    driver_unlock(SENSOR_DRIVER, unit->unit, GPIO_DRIVER, unit->setup[interface].gpio.gpio);
#endif

    return NULL;
}

static driver_error_t *sensor_i2c_unsetup(uint8_t interface, sensor_instance_t *unit) {
    driver_error_t *error;

    if ((error = i2c_detach(unit->setup[interface].i2c.id))) {
        return error;
    }

#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
    driver_unlock(SENSOR_DRIVER, unit->unit, I2C_DRIVER, unit->setup[interface].i2c.id);
#endif

    return NULL;
}

static driver_error_t *sensor_uart_setup(uint8_t interface, sensor_instance_t *unit) {
    driver_error_t *error;

#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
    driver_unit_lock_error_t *lock_error = NULL;
    if ((lock_error = driver_lock(SENSOR_DRIVER, unit->unit, UART_DRIVER,unit->setup[interface].uart.id, DRIVER_ALL_FLAGS, unit->sensor->id))) {
        return driver_lock_error(SENSOR_DRIVER, lock_error);
    }
#endif

    if ((error = uart_init(
            unit->setup[interface].uart.id, unit->setup[interface].uart.speed, unit->setup[interface].uart.data_bits,
            unit->setup[interface].uart.parity, unit->setup[interface].uart.stop_bits, DRIVER_ALL_FLAGS, 1024
    ))) {
        return error;
    }

    if ((error = uart_setup_interrupts(unit->setup[interface].uart.id))) {
        return error;
    }

    return NULL;
}

/*
 * Operation functions
 */
const sensor_t *get_sensor(const char *id) {
    const sensor_t *csensor;

    csensor = sensors;
    while (csensor->id) {
        if (strcmp(csensor->id,id) == 0) {
            return csensor;
        }
        csensor++;
    }

    return NULL;
}

const sensor_data_t *sensor_get_property(const sensor_t *sensor, const char *property) {
    int idx = 0;

    for(idx=0;idx <  SENSOR_MAX_PROPERTIES;idx++) {
        if (sensor->properties[idx].id) {
            if (strcmp(sensor->properties[idx].id,property) == 0) {
                return &(sensor->properties[idx]);
            }
        }
    }

    return NULL;
}

driver_error_t *sensor_setup(const sensor_t *sensor, sensor_setup_t *setup, sensor_instance_t **unit) {
    driver_error_t *error = NULL;
    sensor_instance_t *instance = NULL;
    int i = 0;

    // Sanity checks
    if (!sensor->acquire) {
        //return driver_error(SENSOR_DRIVER, SENSOR_ERR_ACQUIRE_UNDEFINED, NULL);
    }

    // Create a sensor instance
    if (!(instance = (sensor_instance_t *)calloc(1, sizeof(sensor_instance_t)))) {
        return driver_error(SENSOR_DRIVER, SENSOR_ERR_NOT_ENOUGH_MEMORY, NULL);
    }

    // Create mutex
    mtx_init(&instance->mtx, NULL, NULL, 0);

    // Store reference to sensor into instance
    instance->sensor = sensor;

    // Copy sensor setup configuration into instance
    memcpy(&instance->setup, setup, SENSOR_MAX_INTERFACES * sizeof(sensor_setup_t));

    for(i=0;i<SENSOR_MAX_INTERFACES;i++) {
        instance->setup[i].interface = i;
        instance->setup[i].instance = instance;
    }

    // Initialize sensor data from sensor definition into instance
    for(i=0;i<SENSOR_MAX_PROPERTIES;i++) {
        instance->data[i].type = sensor->data[i].type;
    }

    struct timeval now;
    gettimeofday(&now, NULL);

    for(i = 0;i < SENSOR_MAX_PROPERTIES;i++) {
        instance->latch[i].timeout = 0;
        instance->latch[i].t = now;
        instance->latch[i].value.raw.value = instance->data[i].raw.value;
    }

    // Initialize sensor properties from sensor definition into instance
    for(i=0;i<SENSOR_MAX_PROPERTIES;i++) {
        instance->properties[i].type = sensor->properties[i].type;
    }

    // Call to specific pre setup function, if any
    if (instance->sensor->presetup) {
        if ((error = instance->sensor->presetup(instance))) {
            mtx_destroy(&instance->mtx);
            free(instance);
            return error;
        }
    }

    instance->unit = counter++;

    // Setup sensor interfaces
    for(i=0;i<SENSOR_MAX_INTERFACES;i++) {
        if (!(sensor->interface[i].flags & SENSOR_FLAG_CUSTOM_INTERFACE_INIT) && sensor->interface[i].type) {
            switch (sensor->interface[i].type) {
                case ADC_INTERFACE: error = sensor_adc_setup(i, instance);break;
                case GPIO_INTERFACE: error = sensor_gpio_setup(i, instance);break;
                case OWIRE_INTERFACE: error = sensor_owire_setup(i, instance);break;
                case I2C_INTERFACE: error = sensor_i2c_setup(i, instance);break;
                case UART_INTERFACE: error = sensor_uart_setup(i, instance);break;
                case INTERNAL_INTERFACE: break;
                default:
                    return driver_error(SENSOR_DRIVER, SENSOR_ERR_INTERFACE_NOT_SUPPORTED, NULL);
                    break;
            }

            if (error) {
                break;
            }
        }
    }

    if (error) {
        mtx_destroy(&instance->mtx);
        free(instance);
        return error;
    }

    // Call to specific setup function
    if (instance->sensor->setup) {
        if ((error = instance->sensor->setup(instance))) {
            mtx_destroy(&instance->mtx);
            free(instance);
            return error;
        }
    }

    *unit = instance;

    attached++;

#if (CONFIG_LUA_RTOS_POWER_BUS_PIN >= 0)
    pwbus_on();
#endif

    return NULL;
}

driver_error_t *sensor_unsetup(sensor_instance_t *unit) {
    driver_error_t *error;
    int i;

    portDISABLE_INTERRUPTS();

    if (attached == 0) {
        // No sensors attached, nothing to do
        portENABLE_INTERRUPTS();
        return NULL;
    }

    // Call the the custom unsetup function, if any
    if (unit->sensor->unsetup) {
        error = unit->sensor->unsetup(unit);
        if (error) {
            portENABLE_INTERRUPTS();
            return error;
        }
    }

    // Remove interrupts
    for(i=0; i < SENSOR_MAX_INTERFACES; i++) {
        if (unit->sensor->interface[i].flags & SENSOR_FLAG_ON_OFF) {
            if (unit->sensor->interface[i].flags & SENSOR_FLAG_DEBOUNCING) {
                gpio_debouncing_unregister(unit->setup[i].gpio.gpio);
            } else {
                gpio_isr_detach(unit->setup[i].gpio.gpio);
            }
        }
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

    // Detach interface
    for(i=0;i<SENSOR_MAX_INTERFACES;i++) {
        if (!(unit->sensor->interface[i].flags & SENSOR_FLAG_CUSTOM_INTERFACE_INIT) && unit->sensor->interface[i].type) {
            switch (unit->sensor->interface[i].type) {
                case GPIO_INTERFACE: error = sensor_gpio_unsetup(i, unit);break;
                case I2C_INTERFACE: error = sensor_i2c_unsetup(i, unit);break;
                default:
                    break;
            }

            if (error) {
                break;
            }
        }
    }

    attached--;

    mtx_destroy(&unit->mtx);
    free(unit);

#if (CONFIG_LUA_RTOS_POWER_BUS_PIN >= 0)
    pwbus_off();
#endif

    portENABLE_INTERRUPTS();

    return NULL;
}

driver_error_t *sensor_acquire(sensor_instance_t *unit) {
    driver_error_t *error = NULL;
    sensor_value_t *value = NULL;
    int i = 0, j = 0;

    // Check if we can get data
    uint64_t next_available_data = unit->next.tv_sec * 1000000 +unit->next.tv_usec;

    struct timeval now;
    gettimeofday(&now, NULL);
    uint64_t now_usec = now.tv_sec * 1000000 + now.tv_usec;

    if (now_usec < next_available_data) {
        return NULL;
    }

    // Allocate space for sensor data
    if (!(value = calloc(1, sizeof(sensor_value_t) * SENSOR_MAX_PROPERTIES))) {
        return driver_error(SENSOR_DRIVER, SENSOR_ERR_NOT_ENOUGH_MEMORY, NULL);
    }

    // Call to specific acquire function, if any
    if (unit->sensor->acquire) {
        if ((error = unit->sensor->acquire(unit, value))) {
            free(value);
            return error;
        }
    }

    mtx_lock(&unit->mtx);

    // Copy sensor values into instance
    // Note that we only copy raw values as value types are set in sensor_setup from sensor
    // definition
    uint8_t is_auto;
    for(i=0;i < SENSOR_MAX_PROPERTIES;i++) {
        is_auto = 0;

        for (j=0;j < SENSOR_MAX_INTERFACES;j++) {
            is_auto = (
                (unit->sensor->interface[j].flags & (SENSOR_FLAG_AUTO_ACQ | SENSOR_FLAG_ON_OFF))
            );

            if (is_auto) break;
        }

        if (!is_auto) {
            unit->latch[i].timeout = 0;
            unit->latch[i].t = now;
            unit->latch[i].value.raw.value = unit->data[i].raw.value;
            unit->data[i].raw = value[i].raw;
        }
    }

    mtx_unlock(&unit->mtx);

    free(value);

    return NULL;
}

driver_error_t *sensor_read(sensor_instance_t *unit, const char *id, sensor_value_t **value) {
    int idx = 0;

    mtx_lock(&unit->mtx);

    *value = NULL;
    for(idx=0;idx <  SENSOR_MAX_PROPERTIES;idx++) {
        if (unit->sensor->data[idx].id) {
            if (strcmp(unit->sensor->data[idx].id, id) == 0) {
                *value = &unit->data[idx];
                mtx_unlock(&unit->mtx);
                return NULL;
            }
        }
    }

    mtx_unlock(&unit->mtx);

    return driver_error(SENSOR_DRIVER, SENSOR_ERR_NOT_FOUND, NULL);
}

driver_error_t *sensor_set(sensor_instance_t *unit, const char *id, sensor_value_t *value) {
    int idx = 0;

    // Sanity checks
    if (!unit->sensor->set) {
        return driver_error(SENSOR_DRIVER, SENSOR_ERR_SET_UNDEFINED, NULL);
    }

    mtx_lock(&unit->mtx);

    for(idx=0;idx < SENSOR_MAX_PROPERTIES;idx++) {
        if (unit->sensor->properties[idx].id) {
            if (strcmp(unit->sensor->properties[idx].id,id) == 0) {
                unit->sensor->set(unit, id, value);

                mtx_unlock(&unit->mtx);
                return NULL;
            }
        }
    }

    mtx_unlock(&unit->mtx);

    return driver_error(SENSOR_DRIVER, SENSOR_ERR_NOT_FOUND, NULL);
}

driver_error_t *sensor_get(sensor_instance_t *unit, const char *id, sensor_value_t **value) {
    int idx = 0;

    *value = NULL;

    // Sanity checks
    if (!unit->sensor->get) {
        return driver_error(SENSOR_DRIVER, SENSOR_ERR_SET_UNDEFINED, NULL);
    }

    mtx_lock(&unit->mtx);

    for(idx=0;idx < SENSOR_MAX_PROPERTIES;idx++) {
        if (unit->sensor->properties[idx].id) {
            if (strcmp(unit->sensor->properties[idx].id,id) == 0) {
                unit->sensor->get(unit, id, &unit->properties[idx]);

                *value = &unit->properties[idx];

                mtx_unlock(&unit->mtx);
                return NULL;
            }
        }
    }

    mtx_unlock(&unit->mtx);

    return driver_error(SENSOR_DRIVER, SENSOR_ERR_NOT_FOUND, NULL);
}

driver_error_t *sensor_register_callback(sensor_instance_t *unit, sensor_callback_t callback, int id, uint8_t deferred) {
    uint8_t allowed = 0;
    int i;

    // Sanity checks
    for (i=0;i < SENSOR_MAX_INTERFACES;i++) {
        allowed |= unit->sensor->interface[i].flags & (SENSOR_FLAG_AUTO_ACQ | SENSOR_FLAG_ON_OFF);
    }

    if (!allowed) {
        return driver_error(SENSOR_DRIVER, SENSOR_ERR_CALLBACKS_NOT_ALLOWED, NULL);
    }

    portDISABLE_INTERRUPTS();

    // Find for a free callback
    for(i=0;i < SENSOR_MAX_CALLBACKS;i++) {
        if (unit->callbacks[i].callback == NULL) {
            unit->callbacks[i].callback = callback;
            unit->callbacks[i].callback_id = id;
            break;
        }
    }

    if (i == SENSOR_MAX_CALLBACKS) {
        portENABLE_INTERRUPTS();
        return driver_error(SENSOR_DRIVER, SENSOR_ERR_NO_MORE_CALLBACKS, NULL);
    }

    // Create queue if needed
    if (!queue) {
        queue = xQueueCreate(10, sizeof(sensor_deferred_data_t));
        if (!queue) {
            portENABLE_INTERRUPTS();
            return driver_error(SENSOR_DRIVER, SENSOR_ERR_NOT_ENOUGH_MEMORY, NULL);
        }
    }

    // Create task if needed
    if (!task) {
        BaseType_t xReturn;

        xReturn = xTaskCreatePinnedToCore(sensor_task, "sensor", CONFIG_LUA_RTOS_LUA_THREAD_STACK_SIZE, NULL, CONFIG_LUA_RTOS_LUA_THREAD_PRIORITY, &task, xPortGetCoreID());
        if (xReturn != pdPASS) {
            portENABLE_INTERRUPTS();
            return driver_error(SENSOR_DRIVER, SENSOR_ERR_NOT_ENOUGH_MEMORY, NULL);
        }
    }

    portENABLE_INTERRUPTS();

    return NULL;
}

void IRAM_ATTR sensor_queue_callbacks(sensor_instance_t *unit, uint8_t from, uint8_t to) {
    portBASE_TYPE high_priority_task_awoken = 0;
    sensor_deferred_data_t *data;
    portBASE_TYPE yield = 0;

    int i, j;

    data = calloc(1,sizeof(sensor_deferred_data_t));
    assert(data);

    for(i=0;i < SENSOR_MAX_CALLBACKS;i++) {
        if (unit->callbacks[i].callback) {
            data->instance = unit;
            data->callback = unit->callbacks[i].callback;
            data->callback_id = unit->callbacks[i].callback_id;

            memcpy(data->data, unit->data, sizeof(sensor_value_t) * SENSOR_MAX_PROPERTIES);
            memcpy(data->latch, unit->latch, sizeof(sensor_latch_t) * SENSOR_MAX_PROPERTIES);

            for(j=0; j < SENSOR_MAX_PROPERTIES;j++) {
                if (data->latch[j].timeout || data->latch[j].repeat || (data->data[j].raw.value != data->latch[j].value.raw.value))  {
                    if (xPortInIsrContext()) {
                        xQueueSendFromISR(queue, data, &high_priority_task_awoken);
                        yield |= high_priority_task_awoken;
                    } else {
                        xQueueSend(queue, data, 0);
                    }

                    free(data);

                    if (yield == pdTRUE) {
                        portYIELD_FROM_ISR();
                    }

                    return;
                }
            }
        }
    }

    free(data);

    if (yield == pdTRUE) {
        portYIELD_FROM_ISR();
    }
}

void IRAM_ATTR sensor_init_data(sensor_instance_t *unit) {
    struct timeval now;
    int i;

    // Get current time
    gettimeofday(&now, NULL);

    for(i=0; i < SENSOR_MAX_PROPERTIES;i++) {
        unit->data[i].raw.value = 0;
        unit->latch[i].timeout = 0;
        unit->latch[i].repeat = 0;
        unit->latch[i].t = now;
    }
}

void IRAM_ATTR sensor_update_data(sensor_instance_t *unit, uint8_t from, uint8_t to, sensor_value_t *new_data, uint64_t delay, uint64_t rate, uint8_t ignore, uint64_t ignore_val) {
    struct timeval now;
    uint64_t t0, t1;
    int i;

    mtx_lock(&unit->mtx);

    if (delay || rate) {
        // Get current time
        gettimeofday(&now, NULL);

        // Convert current time to usecs
        t1 = now.tv_sec * 1000000 + now.tv_usec;
    }

    for(i = from; i <= to;i++) {
        // Latch data
        unit->latch[i].value.raw.value = unit->data[i].raw.value;

        // Update data
        unit->data[i].raw.value = new_data[i].raw.value;

        if (delay || rate) {
            // If changed update latch time
            if ((unit->latch[i].value.raw.value != unit->data[i].raw.value)) {
                unit->latch[i].timeout = 0;
                unit->latch[i].repeat = 0;
                unit->latch[i].t = now;
            } else {
                if (!ignore || (ignore && (unit->data[i].raw.value != ignore_val))) {
                    // Get last latch time in usecs
                    t0 = unit->latch[i].t.tv_sec * 1000000 + unit->latch[i].t.tv_usec;

                    unit->latch[i].timeout = ((t1 - t0) >= delay);
                    unit->latch[i].repeat = ((t1 - t0) >= rate);
                } else if (ignore) {
                    unit->latch[i].timeout = 0;
                    unit->latch[i].repeat = 0;
                    unit->latch[i].t = now;
                }
            }
        } else {
            unit->latch[i].timeout = 0;
            unit->latch[i].repeat = 0;
        }
    }

    sensor_queue_callbacks(unit, from, to);

    if (unit->latch[i].timeout || unit->latch[i].repeat) {
        unit->latch[i].t = now;
    }

    mtx_unlock(&unit->mtx);
}

void IRAM_ATTR sensor_lock(sensor_instance_t *unit) {
    mtx_lock(&unit->mtx);
}

void IRAM_ATTR sensor_unlock(sensor_instance_t *unit) {
    mtx_unlock(&unit->mtx);
}

#endif
