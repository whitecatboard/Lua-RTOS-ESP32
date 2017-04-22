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
#include <pthread.h>
#include "vl53l0x_platform.h"
#include "vl53l0x_api.h"

// calls read_i2c_block_data(address, reg, length)
static int (*i2c_read_func)(uint8_t address, uint8_t reg,
                    uint8_t *list, uint8_t length) = NULL;

// calls write_i2c_block_data(address, reg, list)
static int (*i2c_write_func)(uint8_t address, uint8_t reg,
                    uint8_t *list, uint8_t length) = NULL;

static pthread_mutex_t i2c_mutex = PTHREAD_MUTEX_INITIALIZER; 

void VL53L0X_init(VL53L0X_DEV Dev)
{
}

void VL53L0X_set_i2c(void *read_func, void *write_func)
{
    i2c_read_func = read_func;
    i2c_write_func = write_func;
}

static int i2c_write(VL53L0X_DEV Dev, uint8_t cmd,
                    uint8_t *data, uint8_t len)
{
    int result = VL53L0X_ERROR_NONE;

    if (i2c_write_func != NULL)
    {
        if (Dev->TCA9548A_Device < 8)
        {
            // Make sure that the call to set the bus on the TCA9548A is
            // synchronized with the write to the VL53L0X device to allow
            // to make the transaction thread-safe
            pthread_mutex_lock(&i2c_mutex);

            // If the value is < 8 then a TCA9548A I2C Multiplexer
            // is being used so prefix each call with a call to 
            // set the device at the multiplexer to the number
            // specified
            if (i2c_write_func(Dev->TCA9548A_Address, (1 << Dev->TCA9548A_Device), NULL, 0) < 0)
            {
                printf("TCA9548A write error\n");
                result = VL53L0X_ERROR_CONTROL_INTERFACE;
            }
        }

        if (result == VL53L0X_ERROR_NONE)
        {
            if (i2c_write_func(Dev->I2cDevAddr, cmd, data, len) < 0)
            {
                result = VL53L0X_ERROR_CONTROL_INTERFACE;
            }
        }

        if (Dev->TCA9548A_Device < 8)
        {
            pthread_mutex_unlock(&i2c_mutex);
        }
    }
    else
    {
        printf("i2c bus write not set.\n");
        result = VL53L0X_ERROR_CONTROL_INTERFACE;
    }
    
    return result;
}

static int i2c_read(VL53L0X_DEV Dev, uint8_t cmd,
                    uint8_t * data, uint8_t len)
{
    int result = VL53L0X_ERROR_NONE;

    if (i2c_read_func != NULL)
    {
        if (Dev->TCA9548A_Device < 8)
        {
            // Make sure that the call to set the bus on the TCA9548A is
            // synchronized with the read of the VL53L0X device to allow
            // to make the transaction thread-safe
            pthread_mutex_lock(&i2c_mutex);

            // If the value is < 8 then a TCA9548A I2C Multiplexer
            // is being used so prefix each call with a call to 
            // set the device at the multiplexer to the number
            // specified
            if (i2c_write_func(Dev->TCA9548A_Address, (1 << Dev->TCA9548A_Device), NULL, 0) < 0)
            {
                printf("TCA9548A read error\n");
                result =  VL53L0X_ERROR_CONTROL_INTERFACE;
            }
        }

        if (result == VL53L0X_ERROR_NONE)
        {
            if (i2c_read_func(Dev->I2cDevAddr, cmd, data, len) < 0)
            {
                result =  VL53L0X_ERROR_CONTROL_INTERFACE;
            }
        }

        if (Dev->TCA9548A_Device < 8)
        {
            pthread_mutex_unlock(&i2c_mutex);
        }
    }
    else
    {
        printf("i2c bus read not set.\n");
        result =  VL53L0X_ERROR_CONTROL_INTERFACE;
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
    return i2c_write(Dev, index, pdata, count);
}

VL53L0X_Error VL53L0X_ReadMulti(VL53L0X_DEV Dev, uint8_t index,
                                uint8_t *pdata, uint32_t count)
{
    return i2c_read(Dev, index, pdata, count);
}

VL53L0X_Error VL53L0X_WrByte(VL53L0X_DEV Dev, uint8_t index, uint8_t data)
{
	return i2c_write(Dev, index, &data, 1);
}

VL53L0X_Error VL53L0X_WrWord(VL53L0X_DEV Dev, uint8_t index, uint16_t data)
{
    uint8_t buf[4];
    buf[1] = data>>0&0xFF;
    buf[0] = data>>8&0xFF;
    return i2c_write(Dev, index, buf, 2);
}

VL53L0X_Error VL53L0X_WrDWord(VL53L0X_DEV Dev, uint8_t index, uint32_t data)
{
    uint8_t buf[4];
    buf[3] = data>>0&0xFF;
    buf[2] = data>>8&0xFF;
    buf[1] = data>>16&0xFF;
    buf[0] = data>>24&0xFF;
    return i2c_write(Dev, index, buf, 4);
}

VL53L0X_Error VL53L0X_UpdateByte(VL53L0X_DEV Dev, uint8_t index,
                                uint8_t AndData, uint8_t OrData)
{

    int32_t status_int;
    uint8_t data;

    status_int = i2c_read(Dev, index, &data, 1);

    if (status_int != 0)
    {
        return  status_int;
    }

    data = (data & AndData) | OrData;
    return i2c_write(Dev, index, &data, 1);
}

VL53L0X_Error VL53L0X_RdByte(VL53L0X_DEV Dev, uint8_t index, uint8_t *data)
{
    uint8_t tmp = 0;
    int ret = i2c_read(Dev, index, &tmp, 1);
    *data = tmp;
    // printf("%u\n", tmp);
    return ret;
}

VL53L0X_Error VL53L0X_RdWord(VL53L0X_DEV Dev, uint8_t index, uint16_t *data)
{
    uint8_t buf[2];
    int ret = i2c_read(Dev, index, buf, 2);
    uint16_t tmp = 0;
    tmp |= buf[1]<<0;
    tmp |= buf[0]<<8;
    // printf("%u\n", tmp);
    *data = tmp;
    return ret;
}

VL53L0X_Error  VL53L0X_RdDWord(VL53L0X_DEV Dev, uint8_t index, uint32_t *data)
{
    uint8_t buf[4];
    int ret = i2c_read(Dev, index, buf, 4);
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
