/*
MIT License

Copyright (c) 2017 John Bryan Moore

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "i2c.h"
#include "vl53l0x_platform.h"
#include "vl53l0x_api.h"

void VL53L0X_init(VL53L0X_DEV Dev){}

static int VL53L0X_i2c_write(VL53L0X_DEV Dev, uint8_t cmd, uint8_t *data, uint8_t len)
{
    driver_error_t *error;

    int result = VL53L0X_ERROR_NONE;
    char reg = (char)cmd;

    if ((error = i2c_start(Dev->unit, &Dev->tran))) {
    	return VL53L0X_ERROR_CONTROL_INTERFACE;
    }

    if(i2c_write_address(Dev->unit, &Dev->tran , Dev->I2cDevAddr, false) != NULL){
        return VL53L0X_ERROR_CONTROL_INTERFACE;
    }
    if(i2c_write(Dev->unit , &Dev->tran , &reg , 1) != NULL){
        return VL53L0X_ERROR_CONTROL_INTERFACE;
    }
    if(i2c_write(Dev->unit , &Dev->tran , (char*)data , (int)len) != NULL){
        return VL53L0X_ERROR_CONTROL_INTERFACE;
    }
    if(i2c_stop(Dev->unit , &Dev->tran) != NULL){
        return VL53L0X_ERROR_CONTROL_INTERFACE;
    }
    
    return result;
}

static int VL53L0X_i2c_read(VL53L0X_DEV Dev, int8_t cmd, uint8_t *data, int8_t len)
{
    int result = VL53L0X_ERROR_NONE;
    char reg = (char)cmd;

    if(i2c_start(Dev->unit, &Dev->tran) != NULL){
        return VL53L0X_ERROR_CONTROL_INTERFACE;
    }

    if(i2c_write_address(Dev->unit, &Dev->tran , Dev->I2cDevAddr, false) != NULL){
        return VL53L0X_ERROR_CONTROL_INTERFACE;
    }
    if(i2c_write(Dev->unit, &Dev->tran , &reg , 1) != NULL){
        return VL53L0X_ERROR_CONTROL_INTERFACE;
    }

    if(i2c_start(Dev->unit, &Dev->tran) != NULL){
        return VL53L0X_ERROR_CONTROL_INTERFACE;
    }

    if(i2c_write_address(Dev->unit, &Dev->tran , Dev->I2cDevAddr, true) != NULL){
        return VL53L0X_ERROR_CONTROL_INTERFACE;
    }

    if (i2c_read(Dev->unit, &Dev->tran , (char*)data, (int)len) != NULL){ 
        return VL53L0X_ERROR_CONTROL_INTERFACE;
    }

    if(i2c_stop(Dev->unit, &Dev->tran) != NULL){
        return VL53L0X_ERROR_CONTROL_INTERFACE;
    }
    
    return result;
}

VL53L0X_Error VL53L0X_LockSequenceAccess(VL53L0X_DEV Dev)
{
    VL53L0X_Error Status = VL53L0X_ERROR_NONE;
    return Status;
}

VL53L0X_Error VL53L0X_UnlockSequenceAccess(VL53L0X_DEV Dev)
{
    VL53L0X_Error Status = VL53L0X_ERROR_NONE;
    return Status;
}

VL53L0X_Error VL53L0X_WriteMulti(VL53L0X_DEV Dev, uint8_t index,
                                uint8_t *pdata, uint32_t count)
{
    return VL53L0X_i2c_write(Dev, index, pdata, count);
}

VL53L0X_Error VL53L0X_ReadMulti(VL53L0X_DEV Dev, uint8_t index,
                                uint8_t *pdata, uint32_t count)
{
    return VL53L0X_i2c_read(Dev, index, pdata, count);
}

VL53L0X_Error VL53L0X_WrByte(VL53L0X_DEV Dev, uint8_t index, uint8_t data)
{
	return VL53L0X_i2c_write(Dev, index, &data, 1);
}

VL53L0X_Error VL53L0X_WrWord(VL53L0X_DEV Dev, uint8_t index, uint16_t data)
{
    uint8_t buf[4];
    buf[1] = data>>0&0xFF;
    buf[0] = data>>8&0xFF;
    return VL53L0X_i2c_write(Dev, index, buf, 2);
}

VL53L0X_Error VL53L0X_WrDWord(VL53L0X_DEV Dev, uint8_t index, uint32_t data)
{
    uint8_t buf[4];
    buf[3] = data>>0&0xFF;
    buf[2] = data>>8&0xFF;
    buf[1] = data>>16&0xFF;
    buf[0] = data>>24&0xFF;
    return VL53L0X_i2c_write(Dev, index, buf, 4);
}

VL53L0X_Error VL53L0X_UpdateByte(VL53L0X_DEV Dev, uint8_t index,
                                uint8_t AndData, uint8_t OrData)
{

    int32_t status_int;
    uint8_t data;

    status_int = VL53L0X_i2c_read(Dev, index, &data, 1);

    if (status_int != 0)
    {
        return  status_int;
    }

    data = (data & AndData) | OrData;
    return VL53L0X_i2c_write(Dev, index, &data, 1);
}

VL53L0X_Error VL53L0X_RdByte(VL53L0X_DEV Dev, uint8_t index, uint8_t *data)
{
    uint8_t tmp = 0;
    int ret = VL53L0X_i2c_read(Dev, index, &tmp, 1);
    *data = tmp;
    // printf("%u\n", tmp);
    return ret;
}

VL53L0X_Error VL53L0X_RdWord(VL53L0X_DEV Dev, uint8_t index, uint16_t *data)
{
    uint8_t buf[2];
    int ret = VL53L0X_i2c_read(Dev, index, buf, 2);
    uint16_t tmp = 0;
    tmp |= buf[1]<<0;
    tmp |= buf[0]<<8;
    // printf("%u\n", tmp);
    *data = tmp;
    return ret;
}

VL53L0X_Error VL53L0X_RdDWord(VL53L0X_DEV Dev, uint8_t index, uint32_t *data)
{
    uint8_t buf[4];
    int ret = VL53L0X_i2c_read(Dev, index, buf, 4);
    uint32_t tmp = 0;
    tmp |= buf[3]<<0;
    tmp |= buf[2]<<8;
    tmp |= buf[1]<<16;
    tmp |= buf[0]<<24;
    *data = tmp;
    // printf("%zu\n", tmp);
    return ret;
}

VL53L0X_Error VL53L0X_PollingDelay(VL53L0X_DEV Dev)
{
    usleep(5000);
    return VL53L0X_ERROR_NONE;
}