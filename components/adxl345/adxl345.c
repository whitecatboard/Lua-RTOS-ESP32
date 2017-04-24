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

static const uint32_t adxl345_i2c_id = 0;
static const uint8_t adxl345_i2c_addr = 0x53;

static int adxl345_init(lua_State* L) {

    uint8_t devid;

    int id = luaL_checkinteger(L, 1);
    int mode = luaL_checkinteger(L, 2);
    int speed = luaL_checkinteger(L, 3);
    int sda = luaL_checkinteger(L, 4);
    int scl = luaL_checkinteger(L, 5);

    if ((error = i2c_setup(adxl345_i2c_id, mode, speed, sda, scl, 0, 0))) {
        return luaL_driver_error(L, error);
    }

    // Enable sensor
    i2c_start(adxl345_i2c_id , I2C_TRANSACTION_INITIALIZER);
    i2c_write_address(adxl345_i2c_id, I2C_TRANSACTION_INITIALIZER , adxl345_i2c_addr, false);
    i2c_write(adxl345_i2c_id , I2C_TRANSACTION_INITIALIZER , 0x2D , sizeof(uint8_t));
    i2c_write(adxl345_i2c_id , I2C_TRANSACTION_INITIALIZER , 0x08 , sizeof(uint8_t));
    i2c_stop(adxl345_i2c_id , I2C_TRANSACTION_INITIALIZER);

    return 1;
}

static int adxl345_read(lua_State* L) {

    uint8_t data[6];
    int x,y,z;
    int i;
    char data;
    uint8_t start_addr = 0x32;

    i2c_start(adxl345_i2c_id , I2C_TRANSACTION_INITIALIZER);
    i2c_write_address(adxl345_i2c_id, I2C_TRANSACTION_INITIALIZER , adxl345_i2c_addr, false);
    i2c_write(adxl345_i2c_id , I2C_TRANSACTION_INITIALIZER , start_addr , sizeof(uint8_t));
    i2c_start(adxl345_i2c_id , I2C_TRANSACTION_INITIALIZER);
    i2c_write_address(adxl345_i2c_id, I2C_TRANSACTION_INITIALIZER , adxl345_i2c_addr, true);

    i2c_read(adxl345_i2c_id, I2C_TRANSACTION_INITIALIZER , &data, 6);

    i2c_stop(adxl345_i2c_id , I2C_TRANSACTION_INITIALIZER);

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
    luaL_newlib(L, adxl345_map);
    return 1;
}

MODULE_REGISTER_MAPPED(ADXL345, adxl345, adxl345_map, luaopen_adxl345);

#endif