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
 * Lua RTOS, Lua sensor module
 *
 */

#include "sdkconfig.h"

#if CONFIG_LUA_RTOS_LUA_USE_SENSOR

#include "freertos/FreeRTOS.h"
#include "freertos/adds.h"

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "error.h"
#include "sensor.h"
#include "sys.h"
#include "modules.h"

#include <time.h>
#include <stdio.h>
#include <string.h>

#include <drivers/owire.h>
#include <drivers/sensor.h>

extern TM_One_Wire_Devices_t ow_devices[MAX_ONEWIRE_PINS];

// This variables are defined at linker time
extern const sensor_t sensors[];

static void callback_func(int id, sensor_instance_t *instance, sensor_value_t *data, sensor_latch_t *latch) {
    int idx;
    int changed;
    const sensor_t *csensor = instance->sensor;

    lua_callback_t *callback = (lua_callback_t *)id;

    // Count properties
    int cnt = 0;
    for(idx=0; idx < SENSOR_MAX_PROPERTIES; idx++) {
            if (csensor->properties[idx].id) {
                cnt++;
            }
    }

    // Create a table with all the properties values
    lua_createtable(luaS_callback_state(callback), 0, cnt);

    for(idx=0; idx < SENSOR_MAX_PROPERTIES; idx++) {
        if (csensor->data[idx].id) {
            lua_pushstring(luaS_callback_state(callback), (char *)csensor->data[idx].id);

            // Test if property has changed
            changed = 0;
            switch (csensor->data[idx].type) {
                case SENSOR_NO_DATA: break;
                case SENSOR_DATA_INT:    changed = (data[idx].integerd.value != latch[idx].value.integerd.value); break;
                case SENSOR_DATA_FLOAT:  changed = (data[idx].floatd.value != latch[idx].value.floatd.value); break;
                case SENSOR_DATA_DOUBLE: changed = (data[idx].doubled.value != latch[idx].value.doubled.value); break;
                case SENSOR_DATA_STRING: changed = (data[idx].stringd.value != latch[idx].value.stringd.value); break;
            }

            if (changed || latch[idx].timeout || latch[idx].repeat) {
                switch (csensor->data[idx].type) {
                    case SENSOR_NO_DATA: break;
                    case SENSOR_DATA_INT:    lua_pushinteger(luaS_callback_state(callback), data[idx].integerd.value); break;
                    case SENSOR_DATA_FLOAT:  lua_pushnumber (luaS_callback_state(callback), data[idx].floatd.value); break;
                    case SENSOR_DATA_DOUBLE: lua_pushnumber (luaS_callback_state(callback), data[idx].doubled.value); break;
                    case SENSOR_DATA_STRING: lua_pushstring (luaS_callback_state(callback), data[idx].stringd.value); break;
                }
            } else {
                lua_pushnil(luaS_callback_state(callback));
            }

            lua_settable(luaS_callback_state(callback), -3);
        }
    }

    luaS_callback_call(callback, 1);
}

static void lsensor_setup_prepare( lua_State* L, const sensor_t *sensor, sensor_setup_t *setup) {
    int i, top;

    // Init setup structure
    memset(setup, 0, SENSOR_MAX_INTERFACES * sizeof(sensor_setup_t));

    // Get the setup argument and put in in the setup structure
    top = 2;

    for(i = 0; i < SENSOR_MAX_INTERFACES;i++) {
        if (sensor->interface[i].type) {
            switch (sensor->interface[i].type) {
                case ADC_INTERFACE:
                    setup[i].adc.unit = luaL_checkinteger(L, top++);
                    setup[i].adc.channel = luaL_checkinteger(L, top++);
                    break;

                case GPIO_INTERFACE:
                    setup[i].gpio.gpio = luaL_checkinteger(L, top++);
                    break;

                case I2C_INTERFACE:
                    setup[i].i2c.id = luaL_checkinteger(L, top++);
                    setup[i].i2c.devid = luaL_checkinteger(L, top++);
                    break;

                case OWIRE_INTERFACE:
                    setup[i].owire.gpio = luaL_checkinteger(L, top++);
                    setup[i].owire.owsensor = luaL_checkinteger(L, top++);
                    break;

                case UART_INTERFACE:
                    setup[i].uart.id = luaL_checkinteger(L, top++);
                    setup[i].uart.speed = luaL_optinteger(L, top++, 9600);
                    setup[i].uart.data_bits = luaL_optinteger(L, top++,8);
                    setup[i].uart.parity = luaL_optinteger(L, top++,0);
                    setup[i].uart.stop_bits = luaL_optinteger(L, top++,1);
                    break;

                default:
                    break;
            }
        }
    }
}

static int lsensor_set_prepare( lua_State* L, const sensor_t *sensor, const char *id, sensor_value_t *property_value ) {
    // Initialize property_value
    memset(property_value, 0, sizeof(sensor_value_t));

    // Get sensor property
    const sensor_data_t *property = sensor_get_property(sensor, id);

    if (!property) {
        return luaL_exception(L, SENSOR_ERR_NOT_FOUND);
    }

    switch (property->type) {
        case SENSOR_DATA_INT:
            property_value->type = SENSOR_DATA_INT;
            property_value->integerd.value = luaL_checkinteger(L, 3);
            break;

        case SENSOR_DATA_FLOAT:
            property_value->type = SENSOR_DATA_FLOAT;
            property_value->floatd.value   = luaL_checknumber(L, 3 );
            break;

        case SENSOR_DATA_DOUBLE:
            property_value->type = SENSOR_DATA_DOUBLE;
            property_value->doubled.value  = luaL_checknumber(L, 3 );
            break;

        default:
            break;
    }

    return 0;
}

static int lsensor_attach( lua_State* L ) {
    driver_error_t *error;
    const sensor_t *sensor;
    sensor_instance_t *instance = NULL;
    sensor_setup_t setup[SENSOR_MAX_INTERFACES];

    const char *id = luaL_checkstring( L, 1 );

    // Get sensor definition
    sensor = get_sensor(id);
    if (!sensor) {
        return luaL_exception(L, SENSOR_ERR_NOT_FOUND);
    }

    // Prepare setup
    lsensor_setup_prepare(L, sensor, setup);

    // Setup sensor
    if ((error = sensor_setup(sensor, setup, &instance))) {
        return luaL_driver_error(L, error);
    }

    // Create user data
    sensor_userdata *data = (sensor_userdata *)lua_newuserdata(L, sizeof(sensor_userdata));
    if (!data) {
            return luaL_exception(L, SENSOR_ERR_NOT_ENOUGH_MEMORY);
    }

    data->instance = instance;
    data->adquired = 0;

    luaL_getmetatable(L, "sensor.ins");
    lua_setmetatable(L, -2);

    return 1;
}

static int lsensor_dettach( lua_State* L ) {
    sensor_userdata *udata =  (sensor_userdata *)luaL_checkudata(L, 1, "sensor.ins");;
    driver_error_t *error;

    if (!udata->instance) return 0;

    // Destroy callbacks
    int i;
    for(i=0; i < SENSOR_MAX_CALLBACKS; i++) {
        if (udata->instance->callbacks[i].callback_id) {
            luaS_callback_destroy((lua_callback_t *)udata->instance->callbacks[i].callback_id);
        }
    }

    // Destroy sensor
    if ((error = sensor_unsetup(udata->instance))) {
        return luaL_driver_error(L, error);
    }

    udata->instance = NULL;

    return 0;
}

static int lsensor_set( lua_State* L ) {
    sensor_userdata *udata = NULL;
    driver_error_t *error;
    sensor_value_t property_value;
    int ret;

    udata = (sensor_userdata *)luaL_checkudata(L, 1, "sensor.ins");
    luaL_argcheck(L, udata, 1, "sensor expected");

    const char *property = luaL_checkstring( L, 2 );

    if (!udata->instance) {
        return luaL_exception(L, SENSOR_ERR_DETACHED);
    }

    // Prepare property value
    if ((ret = lsensor_set_prepare(L, udata->instance->sensor, property, &property_value))) {
        return ret;
    }

    // Set sensor
    if ((error = sensor_set(udata->instance, property, &property_value))) {
            return luaL_driver_error(L, error);
    }

    return 0;
}

static int lsensor_get( lua_State* L ) {
    sensor_userdata *udata = NULL;
    driver_error_t *error;
    sensor_value_t *value;

    udata = (sensor_userdata *)luaL_checkudata(L, 1, "sensor.ins");
    luaL_argcheck(L, udata, 1, "sensor expected");

    const char *property = luaL_checkstring( L, 2 );

    if (!udata->instance) {
        return luaL_exception(L, SENSOR_ERR_DETACHED);
    }

    // Get sensor
    if ((error = sensor_get(udata->instance, property, &value))) {
        return luaL_driver_error(L, error);
    }

    switch (value->type) {
        case SENSOR_NO_DATA:
            lua_pushnil(L);
            return 1;

        case SENSOR_DATA_INT:
            lua_pushinteger(L, value->integerd.value);
            return 1;

        case SENSOR_DATA_FLOAT:
            lua_pushnumber(L, value->floatd.value);
            return 1;

        case SENSOR_DATA_DOUBLE:
            lua_pushnumber(L, value->doubled.value);
            return 1;

        case SENSOR_DATA_STRING:
            lua_pushstring(L, value->stringd.value);
            return 1;

        default:
            return 0;
    }

    return 0;
}

static int lsensor_acquire( lua_State* L ) {
    sensor_userdata *udata = NULL;
    driver_error_t *error;

    udata = (sensor_userdata *)luaL_checkudata(L, 1, "sensor.ins");
    luaL_argcheck(L, udata, 1, "sensor expected");

    if (!udata->instance) {
        return luaL_exception(L, SENSOR_ERR_DETACHED);
    }

    // Acquire data from sensor
    if ((error = sensor_acquire(udata->instance))) {
        return luaL_driver_error(L, error);
    }

    udata->adquired = 1;

    return 0;
}

static int lsensor_read( lua_State* L ) {
    sensor_userdata *udata = NULL;
    driver_error_t *error;
    sensor_value_t *value;

    udata = (sensor_userdata *)luaL_checkudata(L, 1, "sensor.ins");
    luaL_argcheck(L, udata, 1, "sensor expected");

    const char *id = luaL_checkstring( L, 2 );

    if (!udata->instance) {
        return luaL_exception(L, SENSOR_ERR_DETACHED);
    }

    // If data is not acquired acquire data
    if (!udata->adquired) {
        if ((error = sensor_acquire(udata->instance))) {
            return luaL_driver_error(L, error);
        }
    }

    if ((strcmp(id, "all") != 0) && (strcmp(id, "ALL") != 0)) {
        // Read specified data
        if ((error = sensor_read(udata->instance, id, &value))) {
            return luaL_driver_error(L, error);
        }

        udata->adquired = 0;

        switch (value->type) {
            case SENSOR_NO_DATA:
                lua_pushnil(L);
                return 1;
            case SENSOR_DATA_INT:
                lua_pushinteger(L, value->integerd.value);
                return 1;
            case SENSOR_DATA_FLOAT:
                lua_pushnumber(L, value->floatd.value);
                return 1;
            case SENSOR_DATA_DOUBLE:
                lua_pushnumber(L, value->doubled.value);
                return 1;
            default:
                return 0;
        }
    }
    else {
        // Read all sensor data
        int idx, numread=0;
        for(idx=0;idx <  SENSOR_MAX_PROPERTIES;idx++) {
            if (udata->instance->sensor->data[idx].id) {
                *&value = &udata->instance->data[idx];
                switch (value->type) {
                    case SENSOR_NO_DATA:
                        lua_pushnil(L);
                        numread++;
                        break;
                    case SENSOR_DATA_INT:
                        lua_pushinteger(L, value->integerd.value);
                        numread++;
                        break;
                    case SENSOR_DATA_FLOAT:
                        lua_pushnumber(L, value->floatd.value);
                        numread++;
                        break;
                    case SENSOR_DATA_DOUBLE:
                        lua_pushnumber(L, value->doubled.value);
                        numread++;
                        break;
                    default:
                        return luaL_driver_error(L, driver_error(SENSOR_DRIVER, SENSOR_ERR_NOT_FOUND, NULL));
                }
            }
        }
        if (numread == 0) return luaL_driver_error(L, driver_error(SENSOR_DRIVER, SENSOR_ERR_NOT_FOUND, NULL));

        udata->adquired = 0;
        return numread;
    }

    return 0;
}
static int lsensor_list( lua_State* L ) {
    const sensor_t *csensor = sensors;

    uint16_t count = 0, i = 0, j, idx, len;
    uint8_t table = 0, exit;
    char interface[9];
    char type[7];
    char buff[31];
    char interfaces[31];
    char names[31];

    // Check if user wants result as a table, or wants result
    // on the console
    if (lua_gettop(L) == 1) {
        luaL_checktype(L, 1, LUA_TBOOLEAN);
        if (lua_toboolean(L, 1)) {
            table = 1;
        }
    }

    if (!table) {
        printf("SENSOR               INTERFACE                      PROVIDES                            PROPERTIES                 \r\n");
        printf("-------------------------------------------------------------------------------------------------------------------\r\n");
    } else {
        lua_createtable(L, count, 0);
    }

    while (csensor->id) {
        buff[0] = 0x00;
        exit = 0;
        for(j = 0; j < SENSOR_MAX_INTERFACES;j++) {
            if (csensor->interface[j].type) {
                switch (csensor->interface[j].type) {
                    case ADC_INTERFACE:   strcpy(interface, "ADC"); break;
                    case SPI_INTERFACE:   strcpy(interface, "SPI"); break;
                    case I2C_INTERFACE:   strcpy(interface, "I2C"); break;
                    case OWIRE_INTERFACE: strcpy(interface, "1-WIRE"); break;
                    case GPIO_INTERFACE:  strcpy(interface, "GPIO"); break;
                    case UART_INTERFACE:  strcpy(interface, "UART"); break;
                    case INTERNAL_INTERFACE:  strcpy(interface, "INTERNAL"); break;
                    default:
                        break;
                }

                if (strlen(buff) > 0) {
                    if (strlen(buff) + 1 <= 30) {
                        strcat(buff, ",");
                    } else {
                        exit = 1;
                        break;
                    }
                }

                if (strlen(buff) + strlen(interface) <= 30) {
                    strcat(buff, interface);
                } else {
                    exit = 1;
                    break;
                }
            }

            if (exit) {
                break;
            }
        }

        strcpy(interfaces, buff);

        buff[0] = 0x00;
        exit = 0;
        for(j = 0; j < SENSOR_MAX_INTERFACES;j++) {
            if (csensor->interface_name[j]) {
                if (strlen(buff) > 0) {
                    if (strlen(buff) + 1 <= 30) {
                        strcat(buff, ",");
                    } else {
                        exit = 1;
                        break;
                    }
                }

                if (strlen(buff) + strlen(csensor->interface_name[j]) <= 30) {
                    strcat(buff, csensor->interface_name[j]);
                } else {
                    exit = 1;
                    break;
                }
            }

            if (exit) {
                break;
            }
        }

        strcpy(names, buff);

        if (!table) {
            printf("%-20s %-30s ",csensor->id, interfaces);

            len = 0;
            for(idx=0; idx < SENSOR_MAX_PROPERTIES; idx++) {
                if (csensor->data[idx].id) {
                    if (len <= 35) {
                        if (len > 0) {
                            printf(",");
                            len += 1;
                        }

                        printf("%s", csensor->data[idx].id);
                        len += strlen(csensor->data[idx].id);
                    }
                }
            }

            for(;len < 35;len++) printf(" ");

            printf(" ");

            len = 0;
            for(idx=0; idx < SENSOR_MAX_PROPERTIES; idx++) {
                if (csensor->properties[idx].id) {
                    if (len > 0) {
                        printf(",");
                        len += 1;
                    }

                    printf("%s", csensor->properties[idx].id);
                    len += strlen(csensor->properties[idx].id);
                }
            }

            printf("\r\n");
        } else {
            lua_pushinteger(L, i);

            lua_createtable(L, 0, 5);

            lua_pushstring(L, (char *)csensor->id);
            lua_setfield (L, -2, "id");

            int k;
            uint8_t allowed = 0;

            // Sanity checks
            for (k=0;k < SENSOR_MAX_INTERFACES;k++) {
                allowed |= csensor->interface[k].flags & (SENSOR_FLAG_AUTO_ACQ | SENSOR_FLAG_ON_OFF);
            }

            lua_pushboolean(L, allowed != 0);
            lua_setfield (L, -2, "callback");

            lua_pushstring(L, (char *)interfaces);
            lua_setfield (L, -2, "interface");

            lua_pushstring(L, (char *)names);
            lua_setfield (L, -2, "interface_name");

            lua_createtable(L, 0, 0);
            for(idx=0; idx < SENSOR_MAX_PROPERTIES; idx++) {
                if (csensor->data[idx].id) {
                    lua_pushinteger(L, idx);
                    lua_createtable(L, 0, 2);

                    lua_pushstring(L, (char *)csensor->data[idx].id);
                    lua_setfield (L, -2, "id");

                    switch (csensor->data[idx].type) {
                        case SENSOR_DATA_INT: strcpy(type, "int"); break;
                        case SENSOR_DATA_FLOAT: strcpy(type, "float"); break;
                        case SENSOR_DATA_DOUBLE: strcpy(type, "double"); break;

                        default:
                            break;
                    }

                    lua_pushstring(L, type);
                    lua_setfield (L, -2, "type");

                    lua_settable(L,-3);
                }
            }
            lua_setfield (L, -2, "provides");

            lua_createtable(L, 0, 0);
            for(idx=0; idx < SENSOR_MAX_PROPERTIES; idx++) {
                if (csensor->properties[idx].id) {
                    lua_pushinteger(L, idx);
                    lua_createtable(L, 0, 2);

                    lua_pushstring(L, (char *)csensor->properties[idx].id);
                    lua_setfield (L, -2, "id");

                    switch (csensor->properties[idx].type) {
                        case SENSOR_DATA_INT: strcpy(type, "int"); break;
                        case SENSOR_DATA_FLOAT: strcpy(type, "float"); break;
                        case SENSOR_DATA_DOUBLE: strcpy(type, "double"); break;

                        default:
                            break;
                    }

                    lua_pushstring(L, type);
                    lua_setfield (L, -2, "type");

                    lua_settable(L,-3);
                }
            }
            lua_setfield (L, -2, "properties");

            lua_settable(L,-3);
        }

        csensor++;
        i++;
    }

    if (!table) {
        printf("\r\n");
    }

    return table;
}

static int lsensor_enumerate_owire( lua_State* L, uint8_t table, int pin) {
    const sensor_t *csensor = sensors;
    const sensor_t *sensor;
    sensor_instance_t *instance = NULL;
    sensor_setup_t setup;
    uint16_t count = 0;
    unsigned int wh, wl;

    if (!table) {
        printf("SENSOR      DEVICE   ADDRESS             MODEL         \r\n");
        printf("-------------------------------------------------------\r\n");
    } else {
        lua_createtable(L, count, 0);
    }

    // Search for 1-WIRE sensors in build
    while (csensor->id) {
        if (csensor->interface[0].type == OWIRE_INTERFACE) {
            // Get sensor definition
            sensor = get_sensor(csensor->id);
            if (sensor) {
                // Setup this sensor for init bus
                setup.owire.gpio = pin;
                setup.owire.owsensor = 1;
                sensor_setup(sensor, &setup, &instance);
                if (instance) {
                    sensor_value_t *type;
                    char rombuf[17];

                    uint8_t owdev = instance->setup[0].owire.owdevice;
                    sensor_get(instance, "type", &type);
                    if (!type) {
                        continue;
                    }

                    for (int i=0;i<ow_devices[owdev].numdev;i++) {
                        for (int j = 0; j < 8; j++) {
                            sprintf(rombuf+(j*2), "%02x", ow_devices[owdev].roms[i][j]);
                        }

                        if (!table) {
                            printf("%-10s  %02d       %s    %s\r\n",csensor->id, i+1, rombuf, type->stringd.value);
                        } else {
                            lua_pushinteger(L, i);

                            lua_createtable(L, 0, 5);

                            lua_pushstring(L, (char *)csensor->id);
                            lua_setfield (L, -2, "id");

                            lua_pushinteger(L, i+1);
                            lua_setfield (L, -2, "device");

                            sscanf(rombuf, "%08x%08x", &wh,&wl);
                            lua_pushinteger(L, wh);
                            lua_setfield (L, -2, "addressh");

                            lua_pushinteger(L, wl);
                            lua_setfield (L, -2, "addressl");

                            lua_pushstring(L, type->stringd.value);
                            lua_setfield (L, -2, "model");

                            lua_settable(L,-3);
                        }
                    }

                    free(instance);
                }
            }
        }

        csensor++;
    }

    if (!table) {
        printf("\r\n");
    }

    return table;
}

static int lsensor_enumerate( lua_State* L ) {
    uint8_t table = 0;

    int e_type = luaL_checkinteger(L, 1);

    if (e_type == OWIRE_INTERFACE) {
        int pin = luaL_checkinteger(L, 2);

        // Check if user wants result as a table, or wants result
        // on the console
        if (lua_gettop(L) == 3) {
            luaL_checktype(L, 3, LUA_TBOOLEAN);
            if (lua_toboolean(L, 3)) {
                table = 1;
            }
        }

        return lsensor_enumerate_owire(L, table, pin);
    } else {
        return luaL_exception(L, SENSOR_ERR_INTERFACE_NOT_SUPPORTED);
    }

    return table;
}

static int lsensor_callback(lua_State* L) {
    sensor_userdata *udata = NULL;
    driver_error_t *error;

    udata = (sensor_userdata *)luaL_checkudata(L, 1, "sensor.ins");
    luaL_argcheck(L, udata, 1, "sensor expected");

    lua_callback_t *callback = luaS_callback_create(L, 2);

    if ((error = sensor_register_callback(udata->instance, callback_func, (int)callback, 1))) {
        return luaL_driver_error(L, error);
    }

    return 0;
}

// Destructor
static int lsensor_ins_gc (lua_State *L) {
    lsensor_dettach(L);

    return 0;
}

static const LUA_REG_TYPE lsensor_map[] = {
    { LSTRKEY( "attach"       ),    LFUNCVAL( lsensor_attach      ) },
    { LSTRKEY( "list"            ),    LFUNCVAL( lsensor_list         ) },
    { LSTRKEY( "enumerate"    ),    LFUNCVAL( lsensor_enumerate   ) },
    DRIVER_REGISTER_LUA_ERRORS(sensor)
    { LSTRKEY( "OWire"        ), LINTVAL ( OWIRE_INTERFACE     ) },
    { LNILKEY, LNILVAL }
};

static const LUA_REG_TYPE lsensor_ins_map[] = {
    { LSTRKEY( "detach"      ),    LFUNCVAL( lsensor_dettach   ) },
    { LSTRKEY( "acquire"     ),    LFUNCVAL( lsensor_acquire   ) },
      { LSTRKEY( "read"        ),    LFUNCVAL( lsensor_read         ) },
      { LSTRKEY( "set"         ),    LFUNCVAL( lsensor_set         ) },
      { LSTRKEY( "get"         ),    LFUNCVAL( lsensor_get         ) },
      { LSTRKEY( "callback"    ),    LFUNCVAL( lsensor_callback  ) },
    { LSTRKEY( "__metatable" ),    LROVAL  ( lsensor_ins_map   ) },
    { LSTRKEY( "__index"     ),  LROVAL  ( lsensor_ins_map   ) },
    { LSTRKEY( "__gc"        ),  LFUNCVAL( lsensor_ins_gc    ) },
    { LNILKEY, LNILVAL }
};

LUALIB_API int luaopen_sensor( lua_State *L ) {
    luaL_newmetarotable(L,"sensor.ins", (void *)lsensor_ins_map);
    return 0;
}

MODULE_REGISTER_ROM(SENSOR, sensor, lsensor_map, luaopen_sensor, 1);

#endif

/*

s1 = sensor.setup("PING28015", pio.GPIO16)
s1:set("temperature",20)
while true do
    distance = s1:read("distance")
    print("distance "..distance)
    tmr.delayms(500)
end

s1 = sensor.setup("TMP36", adc.ADC1, adc.ADC_CH4, 12)
while true do
    temperature = s1:read("temperature")
    print("temp "..temperature)
    tmr.delayms(500)
end

s1 = sensor.setup("DHT11", pio.GPIO4)
while true do
    temperature = s1:read("temperature")
    humidity = s1:read("humidity")
    print("temp "..temperature..", hum "..humidity)
    tmr.delayms(500)
end

 */
