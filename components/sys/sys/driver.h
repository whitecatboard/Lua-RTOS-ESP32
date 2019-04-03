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
 * Lua RTOS driver common functions
 *
 */

#ifndef DRIVER_H
#define DRIVER_H

#include "luartos.h"
#include "lobject.h"
#include <modules.h>

#include <sys/list.h>
#include <sys/resource.h>
#include <sys/driver.h>

#include <sys/drivers/cpu.h>

#define DRIVER_ALL_FLAGS   0xff

#define ADC_DRIVER_ID      1
#define GPIO_DRIVER_ID     2
#define I2C_DRIVER_ID      3
#define UART_DRIVER_ID     4
#define SPI_DRIVER_ID      5
#define LORA_DRIVER_ID     6
#define PWM_DRIVER_ID      7
#define WIFI_DRIVER_ID     8
#define NET_DRIVER_ID      9
#define SENSOR_DRIVER_ID   10
#define OWIRE_DRIVER_ID    11
#define MQTT_DRIVER_ID     12
#define SERVO_DRIVER_ID    13
#define THREAD_DRIVER_ID   14
#define PWBUS_DRIVER_ID    15
#define NZR_DRIVER_ID      16
#define NEOPIXEL_DRIVER_ID 17
#define STEPPER_DRIVER_ID  18
#define SDISPLAY_DRIVER_ID 19
#define EVENT_DRIVER_ID    20
#define SPI_ETH_DRIVER_ID  21
#define CAN_DRIVER_ID      22
#define SYSTEM_DRIVER_ID   23
#define GDISPLAY_DRIVER_ID 24
#define TIMER_DRIVER_ID    25
#define ENCODER_DRIVER_ID  26
#define MDNS_DRIVER_ID     27
#define CPU_DRIVER_ID      28
#define ULP_DRIVER_ID      29
#define ETH_DRIVER_ID      30
#define BT_DRIVER_ID       31
#define TOUCH_DRIVER_ID    32
#define RCSWITCH_DRIVER_ID 33
#define RMT_DRIVER_ID      34
#define RTC_DRIVER_ID      35

#define GPIO_DRIVER driver_get_by_name("gpio")
#define UART_DRIVER driver_get_by_name("uart")
#define SPI_DRIVER driver_get_by_name("spi")
#define I2C_DRIVER driver_get_by_name("i2c")
#define SENSOR_DRIVER driver_get_by_name("sensor")
#define ADC_DRIVER driver_get_by_name("adc")
#define MQTT_DRIVER driver_get_by_name("mqtt")
#define OWIRE_DRIVER driver_get_by_name("owire")
#define SERVO_DRIVER driver_get_by_name("servo")
#define NZR_DRIVER driver_get_by_name("nzr")
#define NEOPIXEL_DRIVER driver_get_by_name("neopixel")
#define STEPPER_DRIVER driver_get_by_name("stepper")
#define TM1637_DRIVER driver_get_by_name("tm1637")
#define EVENT_DRIVER driver_get_by_name("event")
#define SPI_ETH_DRIVER driver_get_by_name("spi_eth")
#define CAN_DRIVER driver_get_by_name("can")
#define PWBUS_DRIVER driver_get_by_name("pwbus")
#define SYSTEM_DRIVER driver_get_by_name("system")
#define PCD8544_DRIVER driver_get_by_name("pcd8544")
#define GDISPLAY_DRIVER driver_get_by_name("gdisplay")
#define ST7735_DRIVER driver_get_by_name("st7735")
#define ILI9341_DRIVER driver_get_by_name("ili9341")
#define TIMER_DRIVER driver_get_by_name("timer")
#define ENCODER_DRIVER driver_get_by_name("encoder")
#define MDNS_DRIVER driver_get_by_name("mdns")
#define CPU_DRIVER driver_get_by_name("cpu")
#define ULP_DRIVER driver_get_by_name("ulp")
#define ETH_DRIVER driver_get_by_name("eth")
#define SDISPLAY_DRIVER driver_get_by_name("sdisplay")
#define BT_DRIVER driver_get_by_name("bt")
#define TOUCH_DRIVER driver_get_by_name("touch")
#define RCSWITCH_DRIVER driver_get_by_name("rcswitch")
#define RMT_DRIVER driver_get_by_name("rmt")
#define RTC_DRIVER driver_get_by_name("rtc")

#define DRIVER_EXCEPTION_BASE(n) (n << 24)

struct driver;
struct driver_error;

#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
struct driver_unit_lock;
struct driver_unit_lock_error;
#endif

typedef struct {
    uint32_t exception;
    const char *message;
} driver_message_t;

typedef enum {
#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
    LOCK,        // Someone needs a resource which is locked
#endif
    OPERATION   // Something fails during normal operation
} driver_error_type;


typedef struct {
    driver_error_type       type;      // Type of error
    const struct driver    *driver;    // Driver that caused error
    int                     unit;      // Driver unit that caused error
    uint32_t                exception; // Exception code
    const char             *msg;       // Error message
#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
    struct driver_unit_lock_error *lock_error;
#endif
} driver_error_t;

/**
 * @brief This structure maintains basic information about a
 *        driver, such as the driver name, the error messages for
 *        each exception, etc ...
 */
typedef struct driver {
    const char *name;                     /*!< Driver name */
    const uint32_t  exception_base;       /*!< The exception base number for this driver. When a exception is raised the exception number is exception_base + exception number */
    const driver_message_t *error;        /*!< Array of exception error messages */
#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
    const struct driver_unit_lock **lock; /*!< Array locks */
    const int locks;                      /*!< Number of locks */
#endif
    void (*init)();                       /*!< Driver initialization functions, called at system init */

#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
    // Driver lock function
    driver_error_t *(*lock_resources)(int,uint8_t, void *);
#endif
} driver_t;

#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
/**
 * @brief This structure maintains information about a lock. Drivers
 *        usually define an array of driver_unit_lock_t structures
 *        (one for each driver unit), and each array entry keep the
 *        driver unit that owns the lock.
 *
 *        Example:
 *
 *        The GPIO driver defines the gpio_locks array,
 *        with one entry for each GPIO.
 *
 *        Imagine that the UART driver requires a lock on GPIO15 (rx pin)
 *        and on GPIO14 (tx pin) for the UART0.This information is stored
 *        in gpio_locks[14] and gpio_locks[15], with the following information:
 *
 *        gpio_locks[14] = {pointer to UART driver structure, 0}
 *        gpio_locks[15] = {pointer to UART driver structure, 0}
 *
 *        gpio_locks[14] says that GPIO14 is owned by UART0.
 *        gpio_locks[15] says that GPIO15 is owned by UART0.
 */
typedef struct driver_unit_lock {
    const driver_t *owner; /*!< Which driver owns the lock */
    int unit;               /*!< Which unit driver owns the lock */
    const char *tag;       /*!< A tag, used, for example for set the signal name */
} driver_unit_lock_t;

typedef struct driver_unit_lock_error {
    driver_unit_lock_t *lock;

    const driver_t *owner_driver;
    int owner_unit;

    const driver_t *target_driver;
    int target_unit;
} driver_unit_lock_error_t;
#endif

/**
 * @brief Search for a driver into the driver's array by name and get
 *        the driver structure.
 *
 * @param name Driver name.
 *
 * @return NULL if driver not found, or a pointer to a driver_t structure
 *         if driver is found.
 */
const driver_t *driver_get_by_name(const char *name);

/**
 * @brief Search for a driver into the driver's array by exception base
 *        and get the driver structure.
 *
 * @param exception_base Driver exception base.
 *
 * @return NULL if driver not found, or a pointer to a driver_t structure
 *         if driver is found.
 */
const driver_t *driver_get_by_exception_base(const uint32_t exception_base);

const char *driver_get_err_msg(driver_error_t *error);
const char *driver_get_err_msg_by_exception(uint32_t exception);
const char *driver_get_name(driver_error_t *error);

#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
driver_error_t *driver_lock_error(const driver_t *driver, driver_unit_lock_error_t *lock_error);
driver_unit_lock_error_t *driver_lock(const driver_t *owner_driver, int owner_unit, const driver_t *target_driver, int target_unit, uint8_t flags, const char *tag);
void driver_unlock_all(const driver_t *owner_driver, int owner_unit);
void driver_unlock(const driver_t *owner_driver, int owner_unit, const driver_t *target_driver, int target_unit);
#endif

driver_error_t *driver_error(const driver_t *driver, unsigned int code, const char *msg);

void _driver_init();
char *driver_target_name(const driver_t *target_driver, int target_unit, const char *tag);

#define DRIVER_SECTION(s) __attribute__((used,unused,section(s)))

#define DRIVER_PASTER_WITH_SEP(x,y,z) x##y##z
#define DRIVER_EVALUATOR_WITH_SEP(x,y,z) DRIVER_PASTER_WITH_SEP(x,y,z)
#define DRIVER_CONCAT_WITH_SEP(x,y,z) DRIVER_EVALUATOR_WITH_SEP(x,y,z)

#define DRIVER_PASTER(x,y) x##y
#define DRIVER_EVALUATOR(x,y) DRIVER_PASTER(x,y)
#define DRIVER_CONCAT(x,y) DRIVER_EVALUATOR(x,y)

#define DRIVER_TOSTRING_PASTER(x) #x
#define DRIVER_TOSTRING_EVALUATOR(x) DRIVER_TOSTRING_PASTER(x)
#define DRIVER_TOSTRING(x) DRIVER_TOSTRING_EVALUATOR(x)

#define DRIVER_REGISTER_BEGIN(name,lname,locks,initf,lockf) \
const DRIVER_SECTION(DRIVER_TOSTRING(.driver_error)) int DRIVER_CONCAT_WITH_SEP(lname,_,errors_end) = 0; \
const DRIVER_SECTION(DRIVER_TOSTRING(.driver_error_map)) int DRIVER_CONCAT_WITH_SEP(lname,_,error_map_end) = 0;

#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
#define DRIVER_REGISTER_END(name,lname,lockn,initf,lockf) \
driver_unit_lock_t *DRIVER_CONCAT_WITH_SEP(lname,_,locks);\
const DRIVER_SECTION(DRIVER_TOSTRING(.driver_error)) int DRIVER_CONCAT_WITH_SEP(lname,_,errors) = 0; \
const DRIVER_SECTION(DRIVER_TOSTRING(.driver_error_map)) int DRIVER_CONCAT_WITH_SEP(lname,_,error_map) = 0; \
const DRIVER_SECTION(DRIVER_TOSTRING(.drivers)) driver_t DRIVER_CONCAT(driver_,lname) = {DRIVER_TOSTRING(lname),  DRIVER_EXCEPTION_BASE(DRIVER_CONCAT(name,_DRIVER_ID)),  (void *)((&(DRIVER_CONCAT(lname,_errors)))+1), (const struct driver_unit_lock **)&(DRIVER_CONCAT_WITH_SEP(lname,_,locks)), lockn, initf, lockf};
#else
#define DRIVER_REGISTER_END(name,lname,locks,initf,lockf) \
const DRIVER_SECTION(DRIVER_TOSTRING(.driver_error)) int DRIVER_CONCAT_WITH_SEP(lname,_,errors) = 0; \
const DRIVER_SECTION(DRIVER_TOSTRING(.driver_error_map)) int DRIVER_CONCAT_WITH_SEP(lname,_,error_map) = 0; \
const DRIVER_SECTION(DRIVER_TOSTRING(.drivers)) driver_t DRIVER_CONCAT(driver_,lname) = {DRIVER_TOSTRING(lname),  DRIVER_EXCEPTION_BASE(DRIVER_CONCAT(name,_DRIVER_ID)),  (void *)((&(DRIVER_CONCAT(lname,_errors)))+1), initf};
#endif

#endif

#define DRIVER_REGISTER_ERROR(name, lname, key, msg, exception) \
    const DRIVER_SECTION(DRIVER_TOSTRING(.driver_error)) driver_message_t DRIVER_CONCAT(lname,DRIVER_CONCAT(key,_errors)) = {exception, msg}; \
    const DRIVER_SECTION(DRIVER_TOSTRING(.driver_error_map)) LUA_REG_TYPE DRIVER_CONCAT(lname,DRIVER_CONCAT(key,_error_map)) = {LSTRKEY(DRIVER_TOSTRING(key)), LINTVAL(exception)};

#define DRIVER_REGISTER_LUA_ERRORS(lname) \
    {LSTRKEY("error"), LROVAL( ((LUA_REG_TYPE *)((&DRIVER_CONCAT_WITH_SEP(lname,_,error_map)) + 1)) )},

