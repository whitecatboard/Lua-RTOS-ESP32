/*
 * Driver for Analog Devices ADXL345 accelerometer.
 *
 * Code based on BMP085 driver.
 */
#include "luartos.h"

#if CONFIG_LUA_RTOS_LUA_USE_ADXL345

#include "i2c.h"
#include "modules.h"
#include "lauxlib.h"
#include "platform.h"
#include <stdlib.h>
#include <string.h>

static const uint8_t adxl345_i2c_addr = 0x53;

typedef struct {
	int unit;
	int transaction;
} adxl345_user_data_t;

static int adxl345_init(lua_State* L) {

    driver_error_t *error;

    int id = luaL_checkinteger(L, 1);
    int mode = luaL_checkinteger(L, 2);
    int speed = luaL_checkinteger(L, 3);
    int sda = luaL_checkinteger(L, 4);
    int scl = luaL_checkinteger(L, 5);

    if ((error = i2c_setup(id, mode, speed, sda, scl, 0, 0))) {
    	return luaL_driver_error(L, error);
    }

    // Allocate userdata
    adxl345_user_data_t *user_data = (adxl345_user_data_t *)lua_newuserdata(L, sizeof(adxl345_user_data_t));
    if (!user_data) {
       	return luaL_exception(L, I2C_ERR_NOT_ENOUGH_MEMORY);
    }

    user_data->unit = id;
    user_data->transaction = I2C_TRANSACTION_INITIALIZER;

    if ((error = i2c_start(user_data->unit, &user_data->transaction))) {
    	return luaL_driver_error(L, error);
    }

	if ((error = i2c_write_address(user_data->unit, &user_data->transaction, adxl345_i2c_addr, false))) {
    	return luaL_driver_error(L, error);
    }

    if ((error = i2c_write(user_data->unit, &user_data->transaction, 0x2D , sizeof(uint8_t)))) {
    	return luaL_driver_error(L, error);
    }
    if ((error = i2c_write(user_data->unit, &user_data->transaction, 0x08 , sizeof(uint8_t)))) {
    	return luaL_driver_error(L, error);
    }

    if ((error = i2c_stop(user_data->unit, &user_data->transaction))) {
    	return luaL_driver_error(L, error);
    }

    luaL_getmetatable(L, "adxl345.trans");
    lua_setmetatable(L, -2);

    return 1;
}

static int adxl345_read(lua_State* L) {

    driver_error_t *error;
	adxl345_user_data_t *user_data;

	// Get user data
	user_data = (adxl345_user_data_t *)luaL_checkudata(L, 1, "i2c.trans");
    luaL_argcheck(L, user_data, 1, "i2c transaction expected");

    char data[6];
    int x,y,z;
    uint8_t start_addr = 0x32;

    if ((error = i2c_start(user_data->unit, &user_data->transaction))) {
    	return luaL_driver_error(L, error);
    }

	if ((error = i2c_write_address(user_data->unit, &user_data->transaction, adxl345_i2c_addr, false))) {
    	return luaL_driver_error(L, error);
    }

    if ((error = i2c_write(user_data->unit, &user_data->transaction, start_addr , sizeof(uint8_t)))) {
    	return luaL_driver_error(L, error);
    }

    if ((error = i2c_start(user_data->unit, &user_data->transaction))) {
    	return luaL_driver_error(L, error);
    }

	if ((error = i2c_write_address(user_data->unit, &user_data->transaction, adxl345_i2c_addr, true))) {
    	return luaL_driver_error(L, error);
    }

    if ((error = i2c_read(user_data->unit, &user_data->transaction, &data, 6))) {
    	return luaL_driver_error(L, error);
    }

    // We need to flush because we need to return reaad data now
    if ((error = i2c_flush(user_data->unit, &user_data->transaction, 1))) {
    	return luaL_driver_error(L, error);
    }

    if ((error = i2c_stop(user_data->unit, &user_data->transaction))) {
    	return luaL_driver_error(L, error);
    }

    x = (int16_t) ((data[1] << 8) | data[0]);
    y = (int16_t) ((data[3] << 8) | data[2]);
    z = (int16_t) ((data[5] << 8) | data[4]);

    lua_pushinteger(L, x);
    lua_pushinteger(L, y);
    lua_pushinteger(L, z);

    return 3;
}

static const LUA_REG_TYPE adxl345_map[] = {
    { LSTRKEY( "read" ),         LFUNCVAL( adxl345_read )},
    { LSTRKEY( "init" ),         LFUNCVAL( adxl345_init )},
    { LNILKEY, LNILVAL}
};


LUALIB_API int luaopen_adxl345( lua_State *L ) {
    luaL_newmetarotable(L,"adxl345.trans", (void *)adxl345_map);
    return 0;
}

MODULE_REGISTER_MAPPED(ADXL345, adxl345, adxl345_map, luaopen_adxl345);
#endif