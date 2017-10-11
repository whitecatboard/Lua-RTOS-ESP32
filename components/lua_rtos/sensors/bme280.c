/*
****************************************************************************
* Copyright (C) 2015 - 2016 Bosch Sensortec GmbH
*
* bme280.c
* Date: 2016/07/04
* Revision: 2.0.5(Pressure and Temperature compensation code revision is 1.1
*               and Humidity compensation code revision is 1.0)
*
* Usage: Sensor Driver file for BME280 sensor
*
****************************************************************************
* License:
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
*   Redistributions of source code must retain the above copyright
*   notice, this list of conditions and the following disclaimer.
*
*   Redistributions in binary form must reproduce the above copyright
*   notice, this list of conditions and the following disclaimer in the
*   documentation and/or other materials provided with the distribution.
*
*   Neither the name of the copyright holder nor the names of the
*   contributors may be used to endorse or promote products derived from
*   this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
* CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
* IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDER
* OR CONTRIBUTORS BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
* OR CONSEQUENTIAL DAMAGES(INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
* ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE
*
* The information provided is believed to be accurate and reliable.
* The copyright holder assumes no responsibility
* for the consequences of use
* of such information nor for any infringement of patents or
* other rights of third parties which may result from its use.
* No license is granted by implication or otherwise under any patent or
* patent rights of the copyright holder.
**************************************************************************/

#include "sdkconfig.h"

#if CONFIG_LUA_RTOS_LUA_USE_SENSOR
#if CONFIG_LUA_RTOS_USE_SENSOR_BME280

#include "bme280.h"

#include <stdio.h>
#include <string.h>

#include <sys/driver.h>

#include <drivers/i2c.h>


// Sensor specification and registration
const sensor_t __attribute__((used,unused,section(".sensors"))) bme280_sensor = {
	.id = "BME280",
	.interface = {
		{.type = I2C_INTERFACE},
	},
	.data = {
		{.id = "temperature", .type = SENSOR_DATA_DOUBLE},
		{.id = "humidity", .type = SENSOR_DATA_DOUBLE},
		{.id = "pressure", .type = SENSOR_DATA_DOUBLE},
	},
	.properties = {
		{.id = "mode", .type = SENSOR_DATA_INT},
		{.id = "standbytime", .type = SENSOR_DATA_INT},
//		{.id = "address", .type = SENSOR_DATA_INT},
		{.id = "smode", .type = SENSOR_DATA_STRING},
	},
	.presetup = bme280_presetup,
	.setup = bme280_setup,
	.acquire = bme280_acquire,
	.set = bme280_set,
	.get = bme280_get
};


struct bme280_user_data_t *p_bme280 = ((void *)0); /**< pointer to BME280 */

/*!
 *	@brief This function is used for initialize
 *	the bus read and bus write functions
 *  and assign the chip id and I2C address of the BME280 sensor
 *	chip id is read in the register 0xD0 bit from 0 to 7
 *
 *	 @param bme280 structure pointer.
 *
 *	@note While changing the parameter of the bme280_user_data_t
 *	@note consider the following point:
 *	Changing the reference value of the parameter
 *	will changes the local copy or local reference
 *	make sure your changes will not
 *	affect the reference value of the parameter
 *	(Better case don't change the reference value of the parameter)
 *
 *
 *
 *
 *	@return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 *
 *
*/
BME280_RETURN_FUNCTION_TYPE bme280_init()
{
	/* used to return the communication result*/
	BME280_RETURN_FUNCTION_TYPE com_rslt = ERROR;
	u8 v_data_u8 = BME280_INIT_VALUE;
	u8 v_chip_id_read_count = BME280_CHIP_ID_READ_COUNT;

	/* assign BME280 ptr */
	//p_bme280 = bme280;

	while (v_chip_id_read_count > 0) {

		/* read Chip Id */
		com_rslt = p_bme280->BME280_BUS_READ_FUNC(p_bme280->dev_addr,
				BME280_CHIP_ID_REG, &v_data_u8,
				BME280_GEN_READ_WRITE_DATA_LENGTH);

		/* Check for the correct chip id */
		if (v_data_u8 == BME280_CHIP_ID)
			break;
		v_chip_id_read_count--;
		/* Delay added concerning the low speed of power up system to
		facilitate the proper reading of the chip ID */
		p_bme280->delay_msec(BME280_REGISTER_READ_DELAY);
	}
	/*assign chip ID to the global structure*/
	p_bme280->chip_id = v_data_u8;
	/*com_rslt status of chip ID read*/
	com_rslt = (v_chip_id_read_count == BME280_INIT_VALUE) ?
			BME280_CHIP_ID_READ_FAIL : BME280_CHIP_ID_READ_SUCCESS;

	if (com_rslt == BME280_CHIP_ID_READ_SUCCESS) {
		/* readout bme280 calibparam structure */
		com_rslt += bme280_get_calib_param();
	}
	return com_rslt;
}
/*!
 *	@brief This API is used to read uncompensated temperature
 *	in the registers 0xFA, 0xFB and 0xFC
 *	@note 0xFA -> MSB -> bit from 0 to 7
 *	@note 0xFB -> LSB -> bit from 0 to 7
 *	@note 0xFC -> LSB -> bit from 4 to 7
 *
 * @param v_uncomp_temperature_s32 : The value of uncompensated temperature
 *
 *
 *
 *	@return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 *
 *
*/
BME280_RETURN_FUNCTION_TYPE bme280_read_uncomp_temperature(
s32 *v_uncomp_temperature_s32)
{
	/* used to return the communication result*/
	BME280_RETURN_FUNCTION_TYPE com_rslt = ERROR;
	/* Array holding the MSB and LSb value
	a_data_u8r[0] - Temperature MSB
	a_data_u8r[1] - Temperature LSB
	a_data_u8r[2] - Temperature XLSB
	*/
	u8 a_data_u8r[BME280_TEMPERATURE_DATA_SIZE] = {
	BME280_INIT_VALUE, BME280_INIT_VALUE, BME280_INIT_VALUE};
	/* check the p_bme280 structure pointer as NULL*/
	if (p_bme280 == BME280_NULL) {
		return E_BME280_NULL_PTR;
		} else {
			com_rslt = p_bme280->BME280_BUS_READ_FUNC(
			p_bme280->dev_addr,
			BME280_TEMPERATURE_MSB_REG,
			a_data_u8r,
			BME280_TEMPERATURE_DATA_LENGTH);
			*v_uncomp_temperature_s32 = (s32)(((
			(u32) (a_data_u8r[BME280_TEMPERATURE_MSB_DATA]))
			<< BME280_SHIFT_BIT_POSITION_BY_12_BITS) |
			(((u32)(a_data_u8r[BME280_TEMPERATURE_LSB_DATA]))
			<< BME280_SHIFT_BIT_POSITION_BY_04_BITS)
			| ((u32)a_data_u8r[BME280_TEMPERATURE_XLSB_DATA] >>
			BME280_SHIFT_BIT_POSITION_BY_04_BITS));
		}
	return com_rslt;
}
/*!
 * @brief Reads actual temperature from uncompensated temperature
 * @note Returns the value in 0.01 degree Centigrade
 * Output value of "5123" equals 51.23 DegC.
 *
 *
 *
 *  @param  v_uncomp_temperature_s32 : value of uncompensated temperature
 *
 *
 *  @return Returns the actual temperature
 *
*/
s32 bme280_compensate_temperature_int32(s32 v_uncomp_temperature_s32)
{
	s32 v_x1_u32r = BME280_INIT_VALUE;
	s32 v_x2_u32r = BME280_INIT_VALUE;
	s32 temperature = BME280_INIT_VALUE;

	/* calculate x1*/
	v_x1_u32r  =
	((((v_uncomp_temperature_s32
	>> BME280_SHIFT_BIT_POSITION_BY_03_BITS) -
	((s32)p_bme280->cal_param.dig_T1
	<< BME280_SHIFT_BIT_POSITION_BY_01_BIT))) *
	((s32)p_bme280->cal_param.dig_T2)) >>
	BME280_SHIFT_BIT_POSITION_BY_11_BITS;
	/* calculate x2*/
	v_x2_u32r  = (((((v_uncomp_temperature_s32
	>> BME280_SHIFT_BIT_POSITION_BY_04_BITS) -
	((s32)p_bme280->cal_param.dig_T1))
	* ((v_uncomp_temperature_s32 >> BME280_SHIFT_BIT_POSITION_BY_04_BITS) -
	((s32)p_bme280->cal_param.dig_T1)))
	>> BME280_SHIFT_BIT_POSITION_BY_12_BITS) *
	((s32)p_bme280->cal_param.dig_T3))
	>> BME280_SHIFT_BIT_POSITION_BY_14_BITS;
	/* calculate t_fine*/
	p_bme280->cal_param.t_fine = v_x1_u32r + v_x2_u32r;
	/* calculate temperature*/
	temperature  = (p_bme280->cal_param.t_fine * 5 + 128)
	>> BME280_SHIFT_BIT_POSITION_BY_08_BITS;
	return temperature;
}
/*!
 * @brief Reads actual temperature from uncompensated temperature
 * @note Returns the value with 500LSB/DegC centred around 24 DegC
 * output value of "5123" equals(5123/500)+24 = 34.246DegC
 *
 *
 *  @param v_uncomp_temperature_s32: value of uncompensated temperature
 *
 *
 *
 *  @return Return the actual temperature as s16 output
 *
*/
s16 bme280_compensate_temperature_int32_sixteen_bit_output(
s32 v_uncomp_temperature_s32)
{
	s16 temperature = BME280_INIT_VALUE;

	bme280_compensate_temperature_int32(
	v_uncomp_temperature_s32);
	temperature  = (s16)((((
	p_bme280->cal_param.t_fine - 122880) * 25) + 128)
	>> BME280_SHIFT_BIT_POSITION_BY_08_BITS);

	return temperature;
}
/*!
 *	@brief This API is used to read uncompensated pressure.
 *	in the registers 0xF7, 0xF8 and 0xF9
 *	@note 0xF7 -> MSB -> bit from 0 to 7
 *	@note 0xF8 -> LSB -> bit from 0 to 7
 *	@note 0xF9 -> LSB -> bit from 4 to 7
 *
 *
 *
 *	@param v_uncomp_pressure_s32 : The value of uncompensated pressure
 *
 *
 *
 *	@return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 *
 *
*/
BME280_RETURN_FUNCTION_TYPE bme280_read_uncomp_pressure(
s32 *v_uncomp_pressure_s32)
{
	/* used to return the communication result*/
	BME280_RETURN_FUNCTION_TYPE com_rslt = ERROR;
	/* Array holding the MSB and LSb value
	a_data_u8[0] - Pressure MSB
	a_data_u8[1] - Pressure LSB
	a_data_u8[2] - Pressure XLSB
	*/
	u8 a_data_u8[BME280_PRESSURE_DATA_SIZE] = {
	BME280_INIT_VALUE, BME280_INIT_VALUE, BME280_INIT_VALUE};
	/* check the p_bme280 structure pointer as NULL*/
	if (p_bme280 == BME280_NULL) {
		return E_BME280_NULL_PTR;
		} else {
			com_rslt = p_bme280->BME280_BUS_READ_FUNC(
			p_bme280->dev_addr,
			BME280_PRESSURE_MSB_REG,
			a_data_u8, BME280_PRESSURE_DATA_LENGTH);
			*v_uncomp_pressure_s32 = (s32)((
			((u32)(a_data_u8[BME280_PRESSURE_MSB_DATA]))
			<< BME280_SHIFT_BIT_POSITION_BY_12_BITS) |
			(((u32)(a_data_u8[BME280_PRESSURE_LSB_DATA]))
			<< BME280_SHIFT_BIT_POSITION_BY_04_BITS) |
			((u32)a_data_u8[BME280_PRESSURE_XLSB_DATA] >>
			BME280_SHIFT_BIT_POSITION_BY_04_BITS));
		}
	return com_rslt;
}
/*!
 * @brief Reads actual pressure from uncompensated pressure
 * @note Returns the value in Pascal(Pa)
 * Output value of "96386" equals 96386 Pa =
 * 963.86 hPa = 963.86 millibar
 *
 *
 *
 *  @param v_uncomp_pressure_s32 : value of uncompensated pressure
 *
 *
 *
 *  @return Return the actual pressure output as u32
 *
*/
u32 bme280_compensate_pressure_int32(s32 v_uncomp_pressure_s32)
{
	s32 v_x1_u32 = BME280_INIT_VALUE;
	s32 v_x2_u32 = BME280_INIT_VALUE;
	u32 v_pressure_u32 = BME280_INIT_VALUE;

	/* calculate x1*/
	v_x1_u32 = (((s32)p_bme280->cal_param.t_fine)
	>> BME280_SHIFT_BIT_POSITION_BY_01_BIT) - (s32)64000;
	/* calculate x2*/
	v_x2_u32 = (((v_x1_u32 >> BME280_SHIFT_BIT_POSITION_BY_02_BITS)
	* (v_x1_u32 >> BME280_SHIFT_BIT_POSITION_BY_02_BITS)
	) >> BME280_SHIFT_BIT_POSITION_BY_11_BITS)
	* ((s32)p_bme280->cal_param.dig_P6);
	/* calculate x2*/
	v_x2_u32 = v_x2_u32 + ((v_x1_u32 *
	((s32)p_bme280->cal_param.dig_P5))
	<< BME280_SHIFT_BIT_POSITION_BY_01_BIT);
	/* calculate x2*/
	v_x2_u32 = (v_x2_u32 >> BME280_SHIFT_BIT_POSITION_BY_02_BITS) +
	(((s32)p_bme280->cal_param.dig_P4)
	<< BME280_SHIFT_BIT_POSITION_BY_16_BITS);
	/* calculate x1*/
	v_x1_u32 = (((p_bme280->cal_param.dig_P3 *
	(((v_x1_u32 >> BME280_SHIFT_BIT_POSITION_BY_02_BITS) *
	(v_x1_u32 >> BME280_SHIFT_BIT_POSITION_BY_02_BITS))
	>> BME280_SHIFT_BIT_POSITION_BY_13_BITS))
	>> BME280_SHIFT_BIT_POSITION_BY_03_BITS) +
	((((s32)p_bme280->cal_param.dig_P2) *
	v_x1_u32) >> BME280_SHIFT_BIT_POSITION_BY_01_BIT))
	>> BME280_SHIFT_BIT_POSITION_BY_18_BITS;
	/* calculate x1*/
	v_x1_u32 = ((((32768 + v_x1_u32)) *
	((s32)p_bme280->cal_param.dig_P1))
	>> BME280_SHIFT_BIT_POSITION_BY_15_BITS);
	/* calculate pressure*/
	v_pressure_u32 =
	(((u32)(((s32)1048576) - v_uncomp_pressure_s32)
	- (v_x2_u32 >> BME280_SHIFT_BIT_POSITION_BY_12_BITS))) * 3125;
	if (v_pressure_u32
	< 0x80000000)
		/* Avoid exception caused by division by zero */
		if (v_x1_u32 != BME280_INIT_VALUE)
			v_pressure_u32 =
			(v_pressure_u32
			<< BME280_SHIFT_BIT_POSITION_BY_01_BIT) /
			((u32)v_x1_u32);
		else
			return BME280_INVALID_DATA;
	else
		/* Avoid exception caused by division by zero */
		if (v_x1_u32 != BME280_INIT_VALUE)
			v_pressure_u32 = (v_pressure_u32
			/ (u32)v_x1_u32) * 2;
		else
			return BME280_INVALID_DATA;

		v_x1_u32 = (((s32)p_bme280->cal_param.dig_P9) *
		((s32)(((v_pressure_u32 >> BME280_SHIFT_BIT_POSITION_BY_03_BITS)
		* (v_pressure_u32 >> BME280_SHIFT_BIT_POSITION_BY_03_BITS))
		>> BME280_SHIFT_BIT_POSITION_BY_13_BITS)))
		>> BME280_SHIFT_BIT_POSITION_BY_12_BITS;
		v_x2_u32 = (((s32)(v_pressure_u32
		>> BME280_SHIFT_BIT_POSITION_BY_02_BITS)) *
		((s32)p_bme280->cal_param.dig_P8))
		>> BME280_SHIFT_BIT_POSITION_BY_13_BITS;
		v_pressure_u32 = (u32)((s32)v_pressure_u32 +
		((v_x1_u32 + v_x2_u32 + p_bme280->cal_param.dig_P7)
		>> BME280_SHIFT_BIT_POSITION_BY_04_BITS));

	return v_pressure_u32;
}
/*!
 *	@brief This API is used to read uncompensated humidity.
 *	in the registers 0xF7, 0xF8 and 0xF9
 *	@note 0xFD -> MSB -> bit from 0 to 7
 *	@note 0xFE -> LSB -> bit from 0 to 7
 *
 *
 *
 *	@param v_uncomp_humidity_s32 : The value of uncompensated humidity
 *
 *
 *
 *	@return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 *
 *
*/
BME280_RETURN_FUNCTION_TYPE bme280_read_uncomp_humidity(
s32 *v_uncomp_humidity_s32)
{
	/* used to return the communication result*/
	BME280_RETURN_FUNCTION_TYPE com_rslt = ERROR;
	/* Array holding the MSB and LSb value
	a_data_u8[0] - Humidity MSB
	a_data_u8[1] - Humidity LSB
	*/
	u8 a_data_u8[BME280_HUMIDITY_DATA_SIZE] = {
	BME280_INIT_VALUE, BME280_INIT_VALUE};
	/* check the p_bme280 structure pointer as NULL*/
	if (p_bme280 == BME280_NULL) {
		return E_BME280_NULL_PTR;
		} else {
			com_rslt = p_bme280->BME280_BUS_READ_FUNC(
			p_bme280->dev_addr,
			BME280_HUMIDITY_MSB_REG, a_data_u8,
			BME280_HUMIDITY_DATA_LENGTH);
			*v_uncomp_humidity_s32 = (s32)(
			(((u32)(a_data_u8[BME280_HUMIDITY_MSB_DATA]))
			<< BME280_SHIFT_BIT_POSITION_BY_08_BITS)|
			((u32)(a_data_u8[BME280_HUMIDITY_LSB_DATA])));
		}
	return com_rslt;
}
/*!
 * @brief Reads actual humidity from uncompensated humidity
 * @note Returns the value in %rH as unsigned 32bit integer
 * in Q22.10 format(22 integer 10 fractional bits).
 * @note An output value of 42313
 * represents 42313 / 1024 = 41.321 %rH
 *
 *
 *
 *  @param  v_uncomp_humidity_s32: value of uncompensated humidity
 *
 *  @return Return the actual relative humidity output as u32
 *
*/
u32 bme280_compensate_humidity_int32(s32 v_uncomp_humidity_s32)
{
	s32 v_x1_u32 = BME280_INIT_VALUE;

	/* calculate x1*/
	v_x1_u32 = (p_bme280->cal_param.t_fine - ((s32)76800));
	/* calculate x1*/
	v_x1_u32 = (((((v_uncomp_humidity_s32
	<< BME280_SHIFT_BIT_POSITION_BY_14_BITS) -
	(((s32)p_bme280->cal_param.dig_H4)
	<< BME280_SHIFT_BIT_POSITION_BY_20_BITS) -
	(((s32)p_bme280->cal_param.dig_H5) * v_x1_u32)) +
	((s32)16384)) >> BME280_SHIFT_BIT_POSITION_BY_15_BITS)
	* (((((((v_x1_u32 *
	((s32)p_bme280->cal_param.dig_H6))
	>> BME280_SHIFT_BIT_POSITION_BY_10_BITS) *
	(((v_x1_u32 * ((s32)p_bme280->cal_param.dig_H3))
	>> BME280_SHIFT_BIT_POSITION_BY_11_BITS) + ((s32)32768)))
	>> BME280_SHIFT_BIT_POSITION_BY_10_BITS) + ((s32)2097152)) *
	((s32)p_bme280->cal_param.dig_H2) + 8192) >> 14));
	v_x1_u32 = (v_x1_u32 - (((((v_x1_u32
	>> BME280_SHIFT_BIT_POSITION_BY_15_BITS) *
	(v_x1_u32 >> BME280_SHIFT_BIT_POSITION_BY_15_BITS))
	>> BME280_SHIFT_BIT_POSITION_BY_07_BITS) *
	((s32)p_bme280->cal_param.dig_H1))
	>> BME280_SHIFT_BIT_POSITION_BY_04_BITS));
	v_x1_u32 = (v_x1_u32 < 0 ? 0 : v_x1_u32);
	v_x1_u32 = (v_x1_u32 > 419430400 ?
	419430400 : v_x1_u32);
	return (u32)(v_x1_u32 >> BME280_SHIFT_BIT_POSITION_BY_12_BITS);
}
/*!
 * @brief Reads actual humidity from uncompensated humidity
 * @note Returns the value in %rH as unsigned 16bit integer
 * @note An output value of 42313
 * represents 42313/512 = 82.643 %rH
 *
 *
 *
 *  @param v_uncomp_humidity_s32: value of uncompensated humidity
 *
 *
 *  @return Return the actual relative humidity output as u16
 *
*/
u16 bme280_compensate_humidity_int32_sixteen_bit_output(
s32 v_uncomp_humidity_s32)
{
	u32 v_x1_u32 = BME280_INIT_VALUE;
	u16 v_x2_u32 = BME280_INIT_VALUE;

	v_x1_u32 =  bme280_compensate_humidity_int32(v_uncomp_humidity_s32);
	v_x2_u32 = (u16)(v_x1_u32 >> BME280_SHIFT_BIT_POSITION_BY_01_BIT);
	return v_x2_u32;
}
/*!
 * @brief This API used to read uncompensated
 * pressure,temperature and humidity
 *
 *
 *
 *
 *  @param  v_uncomp_pressure_s32: The value of uncompensated pressure.
 *  @param  v_uncomp_temperature_s32: The value of uncompensated temperature
 *  @param  v_uncomp_humidity_s32: The value of uncompensated humidity.
 *
 *
 *
 *	@return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 *
 *
*/
BME280_RETURN_FUNCTION_TYPE bme280_read_uncomp_pressure_temperature_humidity(
s32 *v_uncomp_pressure_s32,
s32 *v_uncomp_temperature_s32, s32 *v_uncomp_humidity_s32)
{
	/* used to return the communication result*/
	BME280_RETURN_FUNCTION_TYPE com_rslt = ERROR;
	/* Array holding the MSB and LSb value of
	a_data_u8[0] - Pressure MSB
	a_data_u8[1] - Pressure LSB
	a_data_u8[1] - Pressure LSB
	a_data_u8[1] - Temperature MSB
	a_data_u8[1] - Temperature LSB
	a_data_u8[1] - Temperature LSB
	a_data_u8[1] - Humidity MSB
	a_data_u8[1] - Humidity LSB
	*/
	u8 a_data_u8[BME280_DATA_FRAME_SIZE] = {
	BME280_INIT_VALUE, BME280_INIT_VALUE,
	BME280_INIT_VALUE, BME280_INIT_VALUE,
	BME280_INIT_VALUE, BME280_INIT_VALUE,
	BME280_INIT_VALUE, BME280_INIT_VALUE};
	/* check the p_bme280 structure pointer as NULL*/
	if (p_bme280 == BME280_NULL) {
		return E_BME280_NULL_PTR;
		} else {
			com_rslt = p_bme280->BME280_BUS_READ_FUNC(p_bme280->dev_addr, BME280_PRESSURE_MSB_REG, a_data_u8, BME280_ALL_DATA_FRAME_LENGTH);
			/*Pressure*/
			*v_uncomp_pressure_s32 = (s32)((
			((u32)(a_data_u8[
			BME280_DATA_FRAME_PRESSURE_MSB_BYTE]))
			<< BME280_SHIFT_BIT_POSITION_BY_12_BITS) |
			(((u32)(a_data_u8[
			BME280_DATA_FRAME_PRESSURE_LSB_BYTE]))
			<< BME280_SHIFT_BIT_POSITION_BY_04_BITS) |
			((u32)a_data_u8[
			BME280_DATA_FRAME_PRESSURE_XLSB_BYTE] >>
			BME280_SHIFT_BIT_POSITION_BY_04_BITS));

			/* Temperature */
			*v_uncomp_temperature_s32 = (s32)(((
			(u32) (a_data_u8[
			BME280_DATA_FRAME_TEMPERATURE_MSB_BYTE]))
			<< BME280_SHIFT_BIT_POSITION_BY_12_BITS) |
			(((u32)(a_data_u8[
			BME280_DATA_FRAME_TEMPERATURE_LSB_BYTE]))
			<< BME280_SHIFT_BIT_POSITION_BY_04_BITS)
			| ((u32)a_data_u8[
			BME280_DATA_FRAME_TEMPERATURE_XLSB_BYTE]
			>> BME280_SHIFT_BIT_POSITION_BY_04_BITS));

			/*Humidity*/
			*v_uncomp_humidity_s32 = (s32)((
			((u32)(a_data_u8[
			BME280_DATA_FRAME_HUMIDITY_MSB_BYTE]))
			<< BME280_SHIFT_BIT_POSITION_BY_08_BITS)|
			((u32)(a_data_u8[
			BME280_DATA_FRAME_HUMIDITY_LSB_BYTE])));
		}
	return com_rslt;
}
/*!
 * @brief This API used to read true pressure, temperature and humidity
 *
 *
 *
 *
 *	@param  v_pressure_u32 : The value of compensated pressure.
 *	@param  v_temperature_s32 : The value of compensated temperature.
 *	@param  v_humidity_u32 : The value of compensated humidity.
 *
 *
 *	@return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 *
 *
*/
//--------------------------------------------------------------------
BME280_RETURN_FUNCTION_TYPE bme280_read_pressure_temperature_humidity(
		u32 *v_pressure_u32, s32 *v_temperature_s32, u32 *v_humidity_u32)
{
	// used to return the communication result
	BME280_RETURN_FUNCTION_TYPE com_rslt = ERROR;
	s32 v_uncomp_pressure_s32 = BME280_INIT_VALUE;
	s32 v_uncom_temperature_s32 = BME280_INIT_VALUE;
	s32 v_uncom_humidity_s32 = BME280_INIT_VALUE;
	// check the p_bme280 structure pointer as NULL
	if (p_bme280 == BME280_NULL) {
		return E_BME280_NULL_PTR;
		} else {
			// read the uncompensated pressure,	temperature and humidity
			com_rslt = bme280_read_uncomp_pressure_temperature_humidity(
					&v_uncomp_pressure_s32, &v_uncom_temperature_s32, &v_uncom_humidity_s32);
			// read the true pressure, temperature and humidity
			*v_temperature_s32 = bme280_compensate_temperature_int32(v_uncom_temperature_s32);
			*v_pressure_u32 = bme280_compensate_pressure_int32(v_uncomp_pressure_s32);
			*v_humidity_u32 = bme280_compensate_humidity_int32(v_uncom_humidity_s32);
		}
	return com_rslt;
}
/*!
 *	@brief This API is used to
 *	calibration parameters used for calculation in the registers
 *
 *  parameter | Register address |   bit
 *------------|------------------|----------------
 *	dig_T1    |  0x88 and 0x89   | from 0 : 7 to 8: 15
 *	dig_T2    |  0x8A and 0x8B   | from 0 : 7 to 8: 15
 *	dig_T3    |  0x8C and 0x8D   | from 0 : 7 to 8: 15
 *	dig_P1    |  0x8E and 0x8F   | from 0 : 7 to 8: 15
 *	dig_P2    |  0x90 and 0x91   | from 0 : 7 to 8: 15
 *	dig_P3    |  0x92 and 0x93   | from 0 : 7 to 8: 15
 *	dig_P4    |  0x94 and 0x95   | from 0 : 7 to 8: 15
 *	dig_P5    |  0x96 and 0x97   | from 0 : 7 to 8: 15
 *	dig_P6    |  0x98 and 0x99   | from 0 : 7 to 8: 15
 *	dig_P7    |  0x9A and 0x9B   | from 0 : 7 to 8: 15
 *	dig_P8    |  0x9C and 0x9D   | from 0 : 7 to 8: 15
 *	dig_P9    |  0x9E and 0x9F   | from 0 : 7 to 8: 15
 *	dig_H1    |         0xA1     | from 0 to 7
 *	dig_H2    |  0xE1 and 0xE2   | from 0 : 7 to 8: 15
 *	dig_H3    |         0xE3     | from 0 to 7
 *
 *	@return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 *
 *
*/
BME280_RETURN_FUNCTION_TYPE bme280_get_calib_param(void)
{
	/* used to return the communication result*/
	BME280_RETURN_FUNCTION_TYPE com_rslt = ERROR;
	u8 a_data_u8[BME280_CALIB_DATA_SIZE] = {
	BME280_INIT_VALUE, BME280_INIT_VALUE,
	BME280_INIT_VALUE, BME280_INIT_VALUE, BME280_INIT_VALUE,
	BME280_INIT_VALUE, BME280_INIT_VALUE, BME280_INIT_VALUE,
	BME280_INIT_VALUE, BME280_INIT_VALUE, BME280_INIT_VALUE,
	BME280_INIT_VALUE, BME280_INIT_VALUE, BME280_INIT_VALUE,
	BME280_INIT_VALUE, BME280_INIT_VALUE, BME280_INIT_VALUE,
	BME280_INIT_VALUE, BME280_INIT_VALUE, BME280_INIT_VALUE,
	BME280_INIT_VALUE, BME280_INIT_VALUE, BME280_INIT_VALUE,
	BME280_INIT_VALUE, BME280_INIT_VALUE, BME280_INIT_VALUE};
	/* check the p_bme280 structure pointer as NULL*/
	if (p_bme280 == BME280_NULL) {
		return E_BME280_NULL_PTR;
		} else {
			com_rslt = p_bme280->BME280_BUS_READ_FUNC(
			p_bme280->dev_addr,
			BME280_TEMPERATURE_CALIB_DIG_T1_LSB_REG,
			a_data_u8,
			BME280_PRESSURE_TEMPERATURE_CALIB_DATA_LENGTH);

			p_bme280->cal_param.dig_T1 = (u16)(((
			(u16)((u8)a_data_u8[
			BME280_TEMPERATURE_CALIB_DIG_T1_MSB])) <<
			BME280_SHIFT_BIT_POSITION_BY_08_BITS)
			| a_data_u8[BME280_TEMPERATURE_CALIB_DIG_T1_LSB]);
			p_bme280->cal_param.dig_T2 = (s16)(((
			(s16)((s8)a_data_u8[
			BME280_TEMPERATURE_CALIB_DIG_T2_MSB])) <<
			BME280_SHIFT_BIT_POSITION_BY_08_BITS)
			| a_data_u8[BME280_TEMPERATURE_CALIB_DIG_T2_LSB]);
			p_bme280->cal_param.dig_T3 = (s16)(((
			(s16)((s8)a_data_u8[
			BME280_TEMPERATURE_CALIB_DIG_T3_MSB])) <<
			BME280_SHIFT_BIT_POSITION_BY_08_BITS)
			| a_data_u8[BME280_TEMPERATURE_CALIB_DIG_T3_LSB]);
			p_bme280->cal_param.dig_P1 = (u16)(((
			(u16)((u8)a_data_u8[
			BME280_PRESSURE_CALIB_DIG_P1_MSB])) <<
			BME280_SHIFT_BIT_POSITION_BY_08_BITS)
			| a_data_u8[BME280_PRESSURE_CALIB_DIG_P1_LSB]);
			p_bme280->cal_param.dig_P2 = (s16)(((
			(s16)((s8)a_data_u8[
			BME280_PRESSURE_CALIB_DIG_P2_MSB])) <<
			BME280_SHIFT_BIT_POSITION_BY_08_BITS)
			| a_data_u8[BME280_PRESSURE_CALIB_DIG_P2_LSB]);
			p_bme280->cal_param.dig_P3 = (s16)(((
			(s16)((s8)a_data_u8[
			BME280_PRESSURE_CALIB_DIG_P3_MSB])) <<
			BME280_SHIFT_BIT_POSITION_BY_08_BITS)
			| a_data_u8[
			BME280_PRESSURE_CALIB_DIG_P3_LSB]);
			p_bme280->cal_param.dig_P4 = (s16)(((
			(s16)((s8)a_data_u8[
			BME280_PRESSURE_CALIB_DIG_P4_MSB])) <<
			BME280_SHIFT_BIT_POSITION_BY_08_BITS)
			| a_data_u8[BME280_PRESSURE_CALIB_DIG_P4_LSB]);
			p_bme280->cal_param.dig_P5 = (s16)(((
			(s16)((s8)a_data_u8[
			BME280_PRESSURE_CALIB_DIG_P5_MSB])) <<
			BME280_SHIFT_BIT_POSITION_BY_08_BITS)
			| a_data_u8[BME280_PRESSURE_CALIB_DIG_P5_LSB]);
			p_bme280->cal_param.dig_P6 = (s16)(((
			(s16)((s8)a_data_u8[
			BME280_PRESSURE_CALIB_DIG_P6_MSB])) <<
			BME280_SHIFT_BIT_POSITION_BY_08_BITS)
			| a_data_u8[BME280_PRESSURE_CALIB_DIG_P6_LSB]);
			p_bme280->cal_param.dig_P7 = (s16)(((
			(s16)((s8)a_data_u8[
			BME280_PRESSURE_CALIB_DIG_P7_MSB])) <<
			BME280_SHIFT_BIT_POSITION_BY_08_BITS)
			| a_data_u8[BME280_PRESSURE_CALIB_DIG_P7_LSB]);
			p_bme280->cal_param.dig_P8 = (s16)(((
			(s16)((s8)a_data_u8[
			BME280_PRESSURE_CALIB_DIG_P8_MSB])) <<
			BME280_SHIFT_BIT_POSITION_BY_08_BITS)
			| a_data_u8[BME280_PRESSURE_CALIB_DIG_P8_LSB]);
			p_bme280->cal_param.dig_P9 = (s16)(((
			(s16)((s8)a_data_u8[
			BME280_PRESSURE_CALIB_DIG_P9_MSB])) <<
			BME280_SHIFT_BIT_POSITION_BY_08_BITS)
			| a_data_u8[BME280_PRESSURE_CALIB_DIG_P9_LSB]);
			p_bme280->cal_param.dig_H1 =
			a_data_u8[BME280_HUMIDITY_CALIB_DIG_H1];

			com_rslt += p_bme280->BME280_BUS_READ_FUNC(
			p_bme280->dev_addr,
			BME280_HUMIDITY_CALIB_DIG_H2_LSB_REG, a_data_u8,
			BME280_HUMIDITY_CALIB_DATA_LENGTH);

			p_bme280->cal_param.dig_H2 = (s16)(((
			(s16)((s8)a_data_u8[
			BME280_HUMIDITY_CALIB_DIG_H2_MSB])) <<
			BME280_SHIFT_BIT_POSITION_BY_08_BITS)
			| a_data_u8[BME280_HUMIDITY_CALIB_DIG_H2_LSB]);
			p_bme280->cal_param.dig_H3 =
			a_data_u8[BME280_HUMIDITY_CALIB_DIG_H3];
			p_bme280->cal_param.dig_H4 = (s16)(((
			(s16)((s8)a_data_u8[
			BME280_HUMIDITY_CALIB_DIG_H4_MSB])) <<
			BME280_SHIFT_BIT_POSITION_BY_04_BITS) |
			(((u8)BME280_MASK_DIG_H4) &
			a_data_u8[BME280_HUMIDITY_CALIB_DIG_H4_LSB]));
			p_bme280->cal_param.dig_H5 = (s16)(((
			(s16)((s8)a_data_u8[
			BME280_HUMIDITY_CALIB_DIG_H5_MSB])) <<
			BME280_SHIFT_BIT_POSITION_BY_04_BITS) |
			(a_data_u8[BME280_HUMIDITY_CALIB_DIG_H4_LSB] >>
			BME280_SHIFT_BIT_POSITION_BY_04_BITS));
			p_bme280->cal_param.dig_H6 =
			(s8)a_data_u8[BME280_HUMIDITY_CALIB_DIG_H6];
		}
	return com_rslt;
}
/*!
 *	@brief This API is used to get
 *	the temperature oversampling setting in the register 0xF4
 *	bits from 5 to 7
 *
 *	value               |   Temperature oversampling
 * ---------------------|---------------------------------
 *	0x00                | Skipped
 *	0x01                | BME280_OVERSAMP_1X
 *	0x02                | BME280_OVERSAMP_2X
 *	0x03                | BME280_OVERSAMP_4X
 *	0x04                | BME280_OVERSAMP_8X
 *	0x05,0x06 and 0x07  | BME280_OVERSAMP_16X
 *
 *
 *  @param v_value_u8 : The value of temperature over sampling
 *
 *
 *
 *	@return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 *
 *
*/
BME280_RETURN_FUNCTION_TYPE bme280_get_oversamp_temperature(
u8 *v_value_u8)
{
	/* used to return the communication result*/
	BME280_RETURN_FUNCTION_TYPE com_rslt = ERROR;
	u8 v_data_u8 = BME280_INIT_VALUE;
	/* check the p_bme280 structure pointer as NULL*/
	if (p_bme280 == BME280_NULL) {
		return E_BME280_NULL_PTR;
		} else {
			com_rslt = p_bme280->BME280_BUS_READ_FUNC(
			p_bme280->dev_addr,
			BME280_CTRL_MEAS_REG_OVERSAMP_TEMPERATURE__REG,
			&v_data_u8, BME280_GEN_READ_WRITE_DATA_LENGTH);
			*v_value_u8 = BME280_GET_BITSLICE(v_data_u8,
			BME280_CTRL_MEAS_REG_OVERSAMP_TEMPERATURE);

			p_bme280->oversamp_temperature = *v_value_u8;
		}
	return com_rslt;
}
/*!
 *	@brief This API is used to set
 *	the temperature oversampling setting in the register 0xF4
 *	bits from 5 to 7
 *
 *	value               |   Temperature oversampling
 * ---------------------|---------------------------------
 *	0x00                | Skipped
 *	0x01                | BME280_OVERSAMP_1X
 *	0x02                | BME280_OVERSAMP_2X
 *	0x03                | BME280_OVERSAMP_4X
 *	0x04                | BME280_OVERSAMP_8X
 *	0x05,0x06 and 0x07  | BME280_OVERSAMP_16X
 *
 *
 *  @param v_value_u8 : The value of temperature over sampling
 *
 *
 *
 *	@return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 *
 *
*/
BME280_RETURN_FUNCTION_TYPE bme280_set_oversamp_temperature(
u8 v_value_u8)
{
	/* used to return the communication result*/
	BME280_RETURN_FUNCTION_TYPE com_rslt = ERROR;
	u8 v_data_u8 = BME280_INIT_VALUE;
	u8 v_prev_pow_mode_u8 = BME280_INIT_VALUE;
	u8 v_pre_ctrl_hum_value_u8 = BME280_INIT_VALUE;
	u8 v_pre_config_value_u8 = BME280_INIT_VALUE;
	/* check the p_bme280 structure pointer as NULL*/
	if (p_bme280 == BME280_NULL) {
		return E_BME280_NULL_PTR;
		} else {
			v_data_u8 = p_bme280->ctrl_meas_reg;
			v_data_u8 =
			BME280_SET_BITSLICE(v_data_u8,
			BME280_CTRL_MEAS_REG_OVERSAMP_TEMPERATURE, v_value_u8);
			com_rslt = bme280_get_power_mode(&v_prev_pow_mode_u8);
			if (v_prev_pow_mode_u8 != BME280_SLEEP_MODE) {
				com_rslt += bme280_set_soft_rst();
				p_bme280->delay_msec(BME280_3MS_DELAY);
				/* write previous value
				of configuration register*/
				v_pre_config_value_u8 = p_bme280->config_reg;
				com_rslt += bme280_write_register(
					BME280_CONFIG_REG,
				&v_pre_config_value_u8,
				BME280_GEN_READ_WRITE_DATA_LENGTH);
				/* write previous value
				of humidity oversampling*/
				v_pre_ctrl_hum_value_u8 =
				p_bme280->ctrl_hum_reg;
				com_rslt += bme280_write_register(
					BME280_CTRL_HUMIDITY_REG,
				&v_pre_ctrl_hum_value_u8,
				BME280_GEN_READ_WRITE_DATA_LENGTH);
				/* write previous and updated value
				of configuration register*/
				com_rslt += bme280_write_register(
					BME280_CTRL_MEAS_REG,
				&v_data_u8, BME280_GEN_READ_WRITE_DATA_LENGTH);
			} else {
				com_rslt = p_bme280->BME280_BUS_WRITE_FUNC(
				p_bme280->dev_addr,
				BME280_CTRL_MEAS_REG_OVERSAMP_TEMPERATURE__REG,
				&v_data_u8, BME280_GEN_READ_WRITE_DATA_LENGTH);
			}
				p_bme280->oversamp_temperature = v_value_u8;
				/* read the control measurement register value*/
				com_rslt = bme280_read_register(
					BME280_CTRL_MEAS_REG,
				&v_data_u8, BME280_GEN_READ_WRITE_DATA_LENGTH);
				p_bme280->ctrl_meas_reg = v_data_u8;
				/* read the control humidity register value*/
				com_rslt += bme280_read_register(
					BME280_CTRL_HUMIDITY_REG,
				&v_data_u8, BME280_GEN_READ_WRITE_DATA_LENGTH);
				p_bme280->ctrl_hum_reg = v_data_u8;
				/* read the control
				configuration register value*/
				com_rslt += bme280_read_register(
					BME280_CONFIG_REG,
				&v_data_u8, BME280_GEN_READ_WRITE_DATA_LENGTH);
				p_bme280->config_reg = v_data_u8;
		}
	return com_rslt;
}
/*!
 *	@brief This API is used to get
 *	the pressure oversampling setting in the register 0xF4
 *	bits from 2 to 4
 *
 *	value              | Pressure oversampling
 * --------------------|--------------------------
 *	0x00               | Skipped
 *	0x01               | BME280_OVERSAMP_1X
 *	0x02               | BME280_OVERSAMP_2X
 *	0x03               | BME280_OVERSAMP_4X
 *	0x04               | BME280_OVERSAMP_8X
 *	0x05,0x06 and 0x07 | BME280_OVERSAMP_16X
 *
 *
 *  @param v_value_u8 : The value of pressure oversampling
 *
 *
 *
 *	@return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 *
 *
*/
BME280_RETURN_FUNCTION_TYPE bme280_get_oversamp_pressure(
u8 *v_value_u8)
{
	/* used to return the communication result*/
	BME280_RETURN_FUNCTION_TYPE com_rslt = ERROR;
	u8 v_data_u8 = BME280_INIT_VALUE;
	/* check the p_bme280 structure pointer as NULL*/
	if (p_bme280 == BME280_NULL) {
		return E_BME280_NULL_PTR;
		} else {
			com_rslt = p_bme280->BME280_BUS_READ_FUNC(
			p_bme280->dev_addr,
			BME280_CTRL_MEAS_REG_OVERSAMP_PRESSURE__REG,
			&v_data_u8, BME280_GEN_READ_WRITE_DATA_LENGTH);
			*v_value_u8 = BME280_GET_BITSLICE(
			v_data_u8,
			BME280_CTRL_MEAS_REG_OVERSAMP_PRESSURE);

			p_bme280->oversamp_pressure = *v_value_u8;
		}
	return com_rslt;
}
/*!
 *	@brief This API is used to set
 *	the pressure oversampling setting in the register 0xF4
 *	bits from 2 to 4
 *
 *	value              | Pressure oversampling
 * --------------------|--------------------------
 *	0x00               | Skipped
 *	0x01               | BME280_OVERSAMP_1X
 *	0x02               | BME280_OVERSAMP_2X
 *	0x03               | BME280_OVERSAMP_4X
 *	0x04               | BME280_OVERSAMP_8X
 *	0x05,0x06 and 0x07 | BME280_OVERSAMP_16X
 *
 *
 *  @param v_value_u8 : The value of pressure oversampling
 *
 *
 *
 *	@return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 *
 *
*/
BME280_RETURN_FUNCTION_TYPE bme280_set_oversamp_pressure(
u8 v_value_u8)
{
	/* used to return the communication result*/
	BME280_RETURN_FUNCTION_TYPE com_rslt = ERROR;
	u8 v_data_u8 = BME280_INIT_VALUE;
	u8 v_prev_pow_mode_u8 = BME280_INIT_VALUE;
	u8 v_pre_ctrl_hum_value_u8 = BME280_INIT_VALUE;
	u8 v_pre_config_value_u8 = BME280_INIT_VALUE;
	/* check the p_bme280 structure pointer as NULL*/
	if (p_bme280 == BME280_NULL) {
		return E_BME280_NULL_PTR;
		} else {
			v_data_u8 = p_bme280->ctrl_meas_reg;
			v_data_u8 =
			BME280_SET_BITSLICE(v_data_u8,
			BME280_CTRL_MEAS_REG_OVERSAMP_PRESSURE, v_value_u8);
			com_rslt = bme280_get_power_mode(&v_prev_pow_mode_u8);
			if (v_prev_pow_mode_u8 != BME280_SLEEP_MODE) {
				com_rslt += bme280_set_soft_rst();
				p_bme280->delay_msec(BME280_3MS_DELAY);
				/* write previous value of
				configuration register*/
				v_pre_config_value_u8 = p_bme280->config_reg;
				com_rslt = bme280_write_register(
					BME280_CONFIG_REG,
				&v_pre_config_value_u8,
				BME280_GEN_READ_WRITE_DATA_LENGTH);
				/* write previous value of
				humidity oversampling*/
				v_pre_ctrl_hum_value_u8 =
				p_bme280->ctrl_hum_reg;
				com_rslt += bme280_write_register(
					BME280_CTRL_HUMIDITY_REG,
				&v_pre_ctrl_hum_value_u8,
				BME280_GEN_READ_WRITE_DATA_LENGTH);
				/* write previous and updated value of
				control measurement register*/
				bme280_write_register(
					BME280_CTRL_MEAS_REG,
				&v_data_u8, BME280_GEN_READ_WRITE_DATA_LENGTH);
			} else {
				com_rslt = p_bme280->BME280_BUS_WRITE_FUNC(
				p_bme280->dev_addr,
				BME280_CTRL_MEAS_REG_OVERSAMP_PRESSURE__REG,
				&v_data_u8, BME280_GEN_READ_WRITE_DATA_LENGTH);
			}
				p_bme280->oversamp_pressure = v_value_u8;
				/* read the control measurement register value*/
				com_rslt = bme280_read_register(
					BME280_CTRL_MEAS_REG,
				&v_data_u8, BME280_GEN_READ_WRITE_DATA_LENGTH);
				p_bme280->ctrl_meas_reg = v_data_u8;
				/* read the control humidity register value*/
				com_rslt += bme280_read_register(
					BME280_CTRL_HUMIDITY_REG,
				&v_data_u8, BME280_GEN_READ_WRITE_DATA_LENGTH);
				p_bme280->ctrl_hum_reg = v_data_u8;
				/* read the control
				configuration register value*/
				com_rslt += bme280_read_register(
					BME280_CONFIG_REG,
				&v_data_u8, BME280_GEN_READ_WRITE_DATA_LENGTH);
				p_bme280->config_reg = v_data_u8;
		}
	return com_rslt;
}
/*!
 *	@brief This API is used to get
 *	the humidity oversampling setting in the register 0xF2
 *	bits from 0 to 2
 *
 *	value               | Humidity oversampling
 * ---------------------|-------------------------
 *	0x00                | Skipped
 *	0x01                | BME280_OVERSAMP_1X
 *	0x02                | BME280_OVERSAMP_2X
 *	0x03                | BME280_OVERSAMP_4X
 *	0x04                | BME280_OVERSAMP_8X
 *	0x05,0x06 and 0x07  | BME280_OVERSAMP_16X
 *
 *
 *  @param  v_value_u8 : The value of humidity over sampling
 *
 *
 *
 *	@return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 *
 *
*/
BME280_RETURN_FUNCTION_TYPE bme280_get_oversamp_humidity(
u8 *v_value_u8)
{
	/* used to return the communication result*/
	BME280_RETURN_FUNCTION_TYPE com_rslt = ERROR;
	u8 v_data_u8 = BME280_INIT_VALUE;
	/* check the p_bme280 structure pointer as NULL*/
	if (p_bme280 == BME280_NULL) {
		return E_BME280_NULL_PTR;
		} else {
			com_rslt = p_bme280->BME280_BUS_READ_FUNC(
			p_bme280->dev_addr,
			BME280_CTRL_HUMIDITY_REG_OVERSAMP_HUMIDITY__REG,
			&v_data_u8, BME280_GEN_READ_WRITE_DATA_LENGTH);
			*v_value_u8 = BME280_GET_BITSLICE(
			v_data_u8,
			BME280_CTRL_HUMIDITY_REG_OVERSAMP_HUMIDITY);

			p_bme280->oversamp_humidity = *v_value_u8;
		}
	return com_rslt;
}
/*!
 *	@brief This API is used to set
 *	the humidity oversampling setting in the register 0xF2
 *	bits from 0 to 2
 *
 *	value               | Humidity oversampling
 * ---------------------|-------------------------
 *	0x00                | Skipped
 *	0x01                | BME280_OVERSAMP_1X
 *	0x02                | BME280_OVERSAMP_2X
 *	0x03                | BME280_OVERSAMP_4X
 *	0x04                | BME280_OVERSAMP_8X
 *	0x05,0x06 and 0x07  | BME280_OVERSAMP_16X
 *
 *
 *  @param  v_value_u8 : The value of humidity over sampling
 *
 *
 *
 * @note The "BME280_CTRL_HUMIDITY_REG_OVERSAMP_HUMIDITY"
 * register sets the humidity
 * data acquisition options of the device.
 * @note changes to this registers only become
 * effective after a write operation to
 * "BME280_CTRL_MEAS_REG" register.
 * @note In the code automated reading and writing of
 *	"BME280_CTRL_HUMIDITY_REG_OVERSAMP_HUMIDITY"
 * @note register first set the
 * "BME280_CTRL_HUMIDITY_REG_OVERSAMP_HUMIDITY"
 *  and then read and write
 *  the "BME280_CTRL_MEAS_REG" register in the function.
 *
 *
 *	@return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 *
 *
*/
BME280_RETURN_FUNCTION_TYPE bme280_set_oversamp_humidity(
u8 v_value_u8)
{
	/* used to return the communication result*/
	BME280_RETURN_FUNCTION_TYPE com_rslt = ERROR;
	u8 v_data_u8 = BME280_INIT_VALUE;
	u8 pre_ctrl_meas_value = BME280_INIT_VALUE;
	u8 v_pre_config_value_u8 = BME280_INIT_VALUE;
	u8 v_prev_pow_mode_u8 = BME280_INIT_VALUE;
	/* check the p_bme280 structure pointer as NULL*/
	if (p_bme280 == BME280_NULL) {
		return E_BME280_NULL_PTR;
		} else {
			/* write humidity oversampling*/
			v_data_u8 = p_bme280->ctrl_hum_reg;
			v_data_u8 =
			BME280_SET_BITSLICE(v_data_u8,
			BME280_CTRL_HUMIDITY_REG_OVERSAMP_HUMIDITY, v_value_u8);
			com_rslt = bme280_get_power_mode(&v_prev_pow_mode_u8);
			if (v_prev_pow_mode_u8 != BME280_SLEEP_MODE) {
				com_rslt += bme280_set_soft_rst();
				p_bme280->delay_msec(BME280_3MS_DELAY);
				/* write previous value of
				configuration register*/
				v_pre_config_value_u8 = p_bme280->config_reg;
				com_rslt += bme280_write_register(
					BME280_CONFIG_REG,
				&v_pre_config_value_u8,
				BME280_GEN_READ_WRITE_DATA_LENGTH);
				/* write the value of control humidity*/
				com_rslt += bme280_write_register(
					BME280_CTRL_HUMIDITY_REG,
				&v_data_u8, BME280_GEN_READ_WRITE_DATA_LENGTH);
				/* write previous value of
				control measurement register*/
				pre_ctrl_meas_value =
				p_bme280->ctrl_meas_reg;
				com_rslt += bme280_write_register(
					BME280_CTRL_MEAS_REG,
				&pre_ctrl_meas_value,
				BME280_GEN_READ_WRITE_DATA_LENGTH);
			} else {
				com_rslt +=
				p_bme280->BME280_BUS_WRITE_FUNC(
				p_bme280->dev_addr,
				BME280_CTRL_HUMIDITY_REG_OVERSAMP_HUMIDITY__REG,
				&v_data_u8, BME280_GEN_READ_WRITE_DATA_LENGTH);
				/* Control humidity write will effective only
				after the control measurement register*/
				pre_ctrl_meas_value =
				p_bme280->ctrl_meas_reg;
				com_rslt += bme280_write_register(
					BME280_CTRL_MEAS_REG,
				&pre_ctrl_meas_value,
				BME280_GEN_READ_WRITE_DATA_LENGTH);
			}
			p_bme280->oversamp_humidity = v_value_u8;
			/* read the control measurement register value*/
			com_rslt += bme280_read_register(BME280_CTRL_MEAS_REG,
			&v_data_u8, BME280_GEN_READ_WRITE_DATA_LENGTH);
			p_bme280->ctrl_meas_reg = v_data_u8;
			/* read the control humidity register value*/
			com_rslt += bme280_read_register(
			BME280_CTRL_HUMIDITY_REG,
			&v_data_u8, BME280_GEN_READ_WRITE_DATA_LENGTH);
			p_bme280->ctrl_hum_reg = v_data_u8;
			/* read the control configuration register value*/
			com_rslt += bme280_read_register(BME280_CONFIG_REG,
			&v_data_u8, BME280_GEN_READ_WRITE_DATA_LENGTH);
			p_bme280->config_reg = v_data_u8;
		}
	return com_rslt;
}
/*!
 *	@brief This API used to get the
 *	Operational Mode from the sensor in the register 0xF4 bit 0 and 1
 *
 *
 *
 *	@param v_power_mode_u8 : The value of power mode
 *  value           |    mode
 * -----------------|------------------
 *	0x00            | BME280_SLEEP_MODE
 *	0x01 and 0x02   | BME280_FORCED_MODE
 *	0x03            | BME280_NORMAL_MODE
 *
 *	@return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 *
 *
*/
BME280_RETURN_FUNCTION_TYPE bme280_get_power_mode(u8 *v_power_mode_u8)
{
	/* used to return the communication result*/
	BME280_RETURN_FUNCTION_TYPE com_rslt = ERROR;
	u8 v_mode_u8r = BME280_INIT_VALUE;
	/* check the p_bme280 structure pointer as NULL*/
	if (p_bme280 == BME280_NULL) {
		return E_BME280_NULL_PTR;
		} else {
			com_rslt = p_bme280->BME280_BUS_READ_FUNC(
			p_bme280->dev_addr,
			BME280_CTRL_MEAS_REG_POWER_MODE__REG,
			&v_mode_u8r, BME280_GEN_READ_WRITE_DATA_LENGTH);
			*v_power_mode_u8 = BME280_GET_BITSLICE(v_mode_u8r,
			BME280_CTRL_MEAS_REG_POWER_MODE);
		}
	return com_rslt;
}
/*!
 *	@brief This API used to set the
 *	Operational Mode from the sensor in the register 0xF4 bit 0 and 1
 *
 *
 *
 *	@param v_power_mode_u8 : The value of power mode
 *  value           |    mode
 * -----------------|------------------
 *	0x00            | BME280_SLEEP_MODE
 *	0x01 and 0x02   | BME280_FORCED_MODE
 *	0x03            | BME280_NORMAL_MODE
 *
 *	@return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 *
 *
*/
BME280_RETURN_FUNCTION_TYPE bme280_set_power_mode(u8 v_power_mode_u8)
{
	/* used to return the communication result*/
	BME280_RETURN_FUNCTION_TYPE com_rslt = ERROR;
	u8 v_mode_u8r = BME280_INIT_VALUE;
	u8 v_prev_pow_mode_u8 = BME280_INIT_VALUE;
	u8 v_pre_ctrl_hum_value_u8 = BME280_INIT_VALUE;
	u8 v_pre_config_value_u8 = BME280_INIT_VALUE;
	u8 v_data_u8 = BME280_INIT_VALUE;

	/* check the p_bme280 structure pointer as NULL*/
	if (p_bme280 == BME280_NULL) {
		return E_BME280_NULL_PTR;
		}
		else {
			if (v_power_mode_u8 <= BME280_NORMAL_MODE) {
				v_mode_u8r = p_bme280->ctrl_meas_reg;
				v_mode_u8r = BME280_SET_BITSLICE(v_mode_u8r, BME280_CTRL_MEAS_REG_POWER_MODE, v_power_mode_u8);
				com_rslt = bme280_get_power_mode(&v_prev_pow_mode_u8);
				if (v_prev_pow_mode_u8 != BME280_SLEEP_MODE) {
					com_rslt += bme280_set_soft_rst();
					p_bme280->delay_msec(BME280_3MS_DELAY);
					/* write previous value of configuration register*/
					v_pre_config_value_u8 =	p_bme280->config_reg;
					com_rslt = bme280_write_register(BME280_CONFIG_REG, &v_pre_config_value_u8, BME280_GEN_READ_WRITE_DATA_LENGTH);
					/* write previous value of humidity oversampling*/
					v_pre_ctrl_hum_value_u8 =
					p_bme280->ctrl_hum_reg;
					com_rslt += bme280_write_register(BME280_CTRL_HUMIDITY_REG, &v_pre_ctrl_hum_value_u8, BME280_GEN_READ_WRITE_DATA_LENGTH);
					/* write previous and updated value of control measurement register*/
					com_rslt += bme280_write_register(BME280_CTRL_MEAS_REG, &v_mode_u8r, BME280_GEN_READ_WRITE_DATA_LENGTH);
				}
				else {
					com_rslt = p_bme280->BME280_BUS_WRITE_FUNC( p_bme280->dev_addr, BME280_CTRL_MEAS_REG_POWER_MODE__REG,
							&v_mode_u8r, BME280_GEN_READ_WRITE_DATA_LENGTH);
				}
				/* read the control measurement register value*/
				com_rslt = bme280_read_register(BME280_CTRL_MEAS_REG, &v_data_u8, BME280_GEN_READ_WRITE_DATA_LENGTH);
				p_bme280->ctrl_meas_reg = v_data_u8;
				/* read the control humidity register value*/
				com_rslt += bme280_read_register(BME280_CTRL_HUMIDITY_REG, &v_data_u8, BME280_GEN_READ_WRITE_DATA_LENGTH);
				p_bme280->ctrl_hum_reg = v_data_u8;
				/* read the config register value*/
				com_rslt += bme280_read_register(BME280_CONFIG_REG, &v_data_u8, BME280_GEN_READ_WRITE_DATA_LENGTH);
				p_bme280->config_reg = v_data_u8;
			}
			else {
				com_rslt = E_BME280_OUT_OF_RANGE;
			}
		}
	return com_rslt;
}
/*!
 * @brief Used to reset the sensor
 * The value 0xB6 is written to the 0xE0
 * register the device is reset using the
 * complete power-on-reset procedure.
 * @note Soft reset can be easily set using bme280_set_softreset().
 * @note Usage Hint : bme280_set_softreset()
 *
 *
 *	@return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 *
 *
*/
BME280_RETURN_FUNCTION_TYPE bme280_set_soft_rst(void)
{
	/* used to return the communication result*/
	BME280_RETURN_FUNCTION_TYPE com_rslt = ERROR;
	u8 v_data_u8 = BME280_SOFT_RESET_CODE;
	/* check the p_bme280 structure pointer as NULL*/
	if (p_bme280 == BME280_NULL) {
		return E_BME280_NULL_PTR;
		} else {
			com_rslt = p_bme280->BME280_BUS_WRITE_FUNC(
			p_bme280->dev_addr,
			BME280_RST_REG, &v_data_u8,
			BME280_GEN_READ_WRITE_DATA_LENGTH);
		}
	return com_rslt;
}

#ifdef BME280_USE_SPI
/*!
 *	@brief This API used to get the sensor
 *	SPI mode(communication type) in the register 0xF5 bit 0
 *
 *
 *
 *	@param v_enable_disable_u8 : The value of SPI enable
 *	value  | Description
 * --------|--------------
 *   0     | Disable
 *   1     | Enable
 *
 *
 *
 *	@return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 *
 *
*/
BME280_RETURN_FUNCTION_TYPE bme280_get_spi3(u8 *v_enable_disable_u8)
{
	/* used to return the communication result*/
	BME280_RETURN_FUNCTION_TYPE com_rslt = ERROR;
	u8 v_data_u8 = BME280_INIT_VALUE;
	/* check the p_bme280 structure pointer as NULL*/
	if (p_bme280 == BME280_NULL) {
		return E_BME280_NULL_PTR;
		} else {
			com_rslt = p_bme280->BME280_BUS_READ_FUNC(
			p_bme280->dev_addr,
			BME280_CONFIG_REG_SPI3_ENABLE__REG,
			&v_data_u8, BME280_GEN_READ_WRITE_DATA_LENGTH);
			*v_enable_disable_u8 = BME280_GET_BITSLICE(
			v_data_u8,
			BME280_CONFIG_REG_SPI3_ENABLE);
		}
	return com_rslt;
}
/*!
 *	@brief This API used to set the sensor
 *	SPI mode(communication type) in the register 0xF5 bit 0
 *
 *
 *
 *	@param v_enable_disable_u8 : The value of SPI enable
 *	value  | Description
 * --------|--------------
 *   0     | Disable
 *   1     | Enable
 *
 *
 *
 *	@return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 *
 *
*/
BME280_RETURN_FUNCTION_TYPE bme280_set_spi3(u8 v_enable_disable_u8)
{
	/* used to return the communication result*/
	BME280_RETURN_FUNCTION_TYPE com_rslt = ERROR;
	u8 v_data_u8 = BME280_INIT_VALUE;
	u8 pre_ctrl_meas_value = BME280_INIT_VALUE;
	u8 v_prev_pow_mode_u8 = BME280_INIT_VALUE;
	u8 v_pre_ctrl_hum_value_u8 =  BME280_INIT_VALUE;
	/* check the p_bme280 structure pointer as NULL*/
	if (p_bme280 == BME280_NULL) {
		return E_BME280_NULL_PTR;
		} else {
			v_data_u8 = p_bme280->config_reg;
			v_data_u8 =
			BME280_SET_BITSLICE(v_data_u8,
			BME280_CONFIG_REG_SPI3_ENABLE, v_enable_disable_u8);
			com_rslt = bme280_get_power_mode(&v_prev_pow_mode_u8);
			if (v_prev_pow_mode_u8 != BME280_SLEEP_MODE) {
				com_rslt += bme280_set_soft_rst();
				p_bme280->delay_msec(BME280_3MS_DELAY);
				/* write previous and updated value of
				configuration register*/
				com_rslt += bme280_write_register(
					BME280_CONFIG_REG,
				&v_data_u8, BME280_GEN_READ_WRITE_DATA_LENGTH);
				/* write previous value of
				humidity oversampling*/
				v_pre_ctrl_hum_value_u8 =
				p_bme280->ctrl_hum_reg;
				com_rslt +=  bme280_write_register(
					BME280_CTRL_HUMIDITY_REG,
				&v_pre_ctrl_hum_value_u8,
				BME280_GEN_READ_WRITE_DATA_LENGTH);
				/* write previous value of
				control measurement register*/
				pre_ctrl_meas_value =
				p_bme280->ctrl_meas_reg;
				com_rslt += bme280_write_register(
					BME280_CTRL_MEAS_REG,
				&pre_ctrl_meas_value,
				BME280_GEN_READ_WRITE_DATA_LENGTH);
			} else {
				com_rslt =
				p_bme280->BME280_BUS_WRITE_FUNC(
				p_bme280->dev_addr,
				BME280_CONFIG_REG_SPI3_ENABLE__REG,
				&v_data_u8, BME280_GEN_READ_WRITE_DATA_LENGTH);
			}
			/* read the control measurement register value*/
			com_rslt += bme280_read_register(
				BME280_CTRL_MEAS_REG,
			&v_data_u8, BME280_GEN_READ_WRITE_DATA_LENGTH);
			p_bme280->ctrl_meas_reg = v_data_u8;
			/* read the control humidity register value*/
			com_rslt += bme280_read_register(
				BME280_CTRL_HUMIDITY_REG,
			&v_data_u8, BME280_GEN_READ_WRITE_DATA_LENGTH);
			p_bme280->ctrl_hum_reg = v_data_u8;
			/* read the control configuration register value*/
			com_rslt += bme280_read_register(
				BME280_CONFIG_REG,
			&v_data_u8, BME280_GEN_READ_WRITE_DATA_LENGTH);
			p_bme280->config_reg = v_data_u8;
		}
	return com_rslt;
}
#endif

/*!
 *	@brief This API is used to reads filter setting
 *	in the register 0xF5 bit 3 and 4
 *
 *
 *
 *	@param v_value_u8 : The value of IIR filter coefficient
 *
 *	value	    |	Filter coefficient
 * -------------|-------------------------
 *	0x00        | BME280_FILTER_COEFF_OFF
 *	0x01        | BME280_FILTER_COEFF_2
 *	0x02        | BME280_FILTER_COEFF_4
 *	0x03        | BME280_FILTER_COEFF_8
 *	0x04        | BME280_FILTER_COEFF_16
 *
 *	@return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 *
 *
*/
BME280_RETURN_FUNCTION_TYPE bme280_get_filter(u8 *v_value_u8)
{
	/* used to return the communication result*/
	BME280_RETURN_FUNCTION_TYPE com_rslt = ERROR;
	u8 v_data_u8 = BME280_INIT_VALUE;
	/* check the p_bme280 structure pointer as NULL*/
	if (p_bme280 == BME280_NULL) {
		return E_BME280_NULL_PTR;
		} else {
			com_rslt = p_bme280->BME280_BUS_READ_FUNC(
			p_bme280->dev_addr,
			BME280_CONFIG_REG_FILTER__REG,
			&v_data_u8, BME280_GEN_READ_WRITE_DATA_LENGTH);
			*v_value_u8 = BME280_GET_BITSLICE(v_data_u8,
			BME280_CONFIG_REG_FILTER);
		}
	return com_rslt;
}
/*!
 *	@brief This API is used to write filter setting
 *	in the register 0xF5 bit 3 and 4
 *
 *
 *
 *	@param v_value_u8 : The value of IIR filter coefficient
 *
 *	value	    |	Filter coefficient
 * -------------|-------------------------
 *	0x00        | BME280_FILTER_COEFF_OFF
 *	0x01        | BME280_FILTER_COEFF_2
 *	0x02        | BME280_FILTER_COEFF_4
 *	0x03        | BME280_FILTER_COEFF_8
 *	0x04        | BME280_FILTER_COEFF_16
 *
 *	@return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 *
 *
*/
BME280_RETURN_FUNCTION_TYPE bme280_set_filter(u8 v_value_u8)
{
	/* used to return the communication result*/
	BME280_RETURN_FUNCTION_TYPE com_rslt = ERROR;
	u8 v_data_u8 = BME280_INIT_VALUE;
	u8 pre_ctrl_meas_value = BME280_INIT_VALUE;
	u8 v_prev_pow_mode_u8 = BME280_INIT_VALUE;
	u8 v_pre_ctrl_hum_value_u8 =  BME280_INIT_VALUE;
	/* check the p_bme280 structure pointer as NULL*/
	if (p_bme280 == BME280_NULL) {
		return E_BME280_NULL_PTR;
		} else {
			v_data_u8 = p_bme280->config_reg;
			v_data_u8 =
			BME280_SET_BITSLICE(v_data_u8,
			BME280_CONFIG_REG_FILTER, v_value_u8);
			com_rslt = bme280_get_power_mode(&v_prev_pow_mode_u8);
			if (v_prev_pow_mode_u8 != BME280_SLEEP_MODE) {
				com_rslt += bme280_set_soft_rst();
				p_bme280->delay_msec(BME280_3MS_DELAY);
				/* write previous and updated value of
				configuration register*/
				com_rslt += bme280_write_register(
					BME280_CONFIG_REG,
				&v_data_u8, BME280_GEN_READ_WRITE_DATA_LENGTH);
				/* write previous value of
				humidity oversampling*/
				v_pre_ctrl_hum_value_u8 =
				p_bme280->ctrl_hum_reg;
				com_rslt += bme280_write_register(
					BME280_CTRL_HUMIDITY_REG,
				&v_pre_ctrl_hum_value_u8,
				BME280_GEN_READ_WRITE_DATA_LENGTH);
				/* write previous value of
				control measurement register*/
				pre_ctrl_meas_value =
				p_bme280->ctrl_meas_reg;
				com_rslt += bme280_write_register(
					BME280_CTRL_MEAS_REG,
				&pre_ctrl_meas_value,
				BME280_GEN_READ_WRITE_DATA_LENGTH);
			} else {
				com_rslt =
				p_bme280->BME280_BUS_WRITE_FUNC(
				p_bme280->dev_addr,
				BME280_CONFIG_REG_FILTER__REG,
				&v_data_u8, BME280_GEN_READ_WRITE_DATA_LENGTH);
			}
			/* read the control measurement register value*/
			com_rslt += bme280_read_register(BME280_CTRL_MEAS_REG,
			&v_data_u8, BME280_GEN_READ_WRITE_DATA_LENGTH);
			p_bme280->ctrl_meas_reg = v_data_u8;
			/* read the control humidity register value*/
			com_rslt += bme280_read_register(
			BME280_CTRL_HUMIDITY_REG,
			&v_data_u8, BME280_GEN_READ_WRITE_DATA_LENGTH);
			p_bme280->ctrl_hum_reg = v_data_u8;
			/* read the configuration register value*/
			com_rslt += bme280_read_register(BME280_CONFIG_REG,
			&v_data_u8, BME280_GEN_READ_WRITE_DATA_LENGTH);
			p_bme280->config_reg = v_data_u8;
		}
	return com_rslt;
}
/*!
 *	@brief This API used to Read the
 *	standby duration time from the sensor in the register 0xF5 bit 5 to 7
 *
 *	@param v_standby_durn_u8 : The value of standby duration time value.
 *  value       | standby duration
 * -------------|-----------------------
 *    0x00      | BME280_STANDBY_TIME_1_MS
 *    0x01      | BME280_STANDBY_TIME_63_MS
 *    0x02      | BME280_STANDBY_TIME_125_MS
 *    0x03      | BME280_STANDBY_TIME_250_MS
 *    0x04      | BME280_STANDBY_TIME_500_MS
 *    0x05      | BME280_STANDBY_TIME_1000_MS
 *    0x06      | BME280_STANDBY_TIME_2000_MS
 *    0x07      | BME280_STANDBY_TIME_4000_MS
 *
 *
 *	@return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 *
 *
*/
BME280_RETURN_FUNCTION_TYPE bme280_get_standby_durn(u8 *v_standby_durn_u8)
{
	/* used to return the communication result*/
	BME280_RETURN_FUNCTION_TYPE com_rslt = ERROR;
	u8 v_data_u8 = BME280_INIT_VALUE;
	/* check the p_bme280 structure pointer as NULL*/
	if (p_bme280 == BME280_NULL) {
		return E_BME280_NULL_PTR;
		} else {
			com_rslt = p_bme280->BME280_BUS_READ_FUNC(
			p_bme280->dev_addr,
			BME280_CONFIG_REG_TSB__REG,
			&v_data_u8, BME280_GEN_READ_WRITE_DATA_LENGTH);
			*v_standby_durn_u8 = BME280_GET_BITSLICE(
			v_data_u8, BME280_CONFIG_REG_TSB);
		}
	return com_rslt;
}
/*!
 *	@brief This API used to write the
 *	standby duration time from the sensor in the register 0xF5 bit 5 to 7
 *
 *	@param v_standby_durn_u8 : The value of standby duration time value.
 *  value       | standby duration
 * -------------|-----------------------
 *    0x00      | BME280_STANDBY_TIME_1_MS
 *    0x01      | BME280_STANDBY_TIME_63_MS
 *    0x02      | BME280_STANDBY_TIME_125_MS
 *    0x03      | BME280_STANDBY_TIME_250_MS
 *    0x04      | BME280_STANDBY_TIME_500_MS
 *    0x05      | BME280_STANDBY_TIME_1000_MS
 *    0x06      | BME280_STANDBY_TIME_2000_MS
 *    0x07      | BME280_STANDBY_TIME_4000_MS
 *
 *	@note Normal mode comprises an automated perpetual
 *	cycling between an (active)
 *	Measurement period and an (inactive) standby period.
 *	@note The standby time is determined by
 *	the contents of the register t_sb.
 *	Standby time can be set using BME280_STANDBY_TIME_125_MS.
 *
 *	@note Usage Hint : bme280_set_standby_durn(BME280_STANDBY_TIME_125_MS)
 *
 *
 *
 *	@return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 *
 *
*/
BME280_RETURN_FUNCTION_TYPE bme280_set_standby_durn(u8 v_standby_durn_u8)
{
	/* used to return the communication result*/
	BME280_RETURN_FUNCTION_TYPE com_rslt = ERROR;
	u8 v_data_u8 = BME280_INIT_VALUE;
	u8 pre_ctrl_meas_value = BME280_INIT_VALUE;
	u8 v_prev_pow_mode_u8 = BME280_INIT_VALUE;
	u8 v_pre_ctrl_hum_value_u8 = BME280_INIT_VALUE;

	/* check the p_bme280 structure pointer as NULL*/
	if (p_bme280 == BME280_NULL) {
		return E_BME280_NULL_PTR;
		} else {
			v_data_u8 = p_bme280->config_reg;
			v_data_u8 = BME280_SET_BITSLICE(v_data_u8, BME280_CONFIG_REG_TSB, v_standby_durn_u8);
			com_rslt = bme280_get_power_mode(&v_prev_pow_mode_u8);
			if (v_prev_pow_mode_u8 != BME280_SLEEP_MODE) {
				com_rslt += bme280_set_soft_rst();
				p_bme280->delay_msec(BME280_3MS_DELAY);
				/* write previous and updated value of configuration register*/
				com_rslt += bme280_write_register(BME280_CONFIG_REG, &v_data_u8, BME280_GEN_READ_WRITE_DATA_LENGTH);
				/* write previous value of humidity oversampling*/
				v_pre_ctrl_hum_value_u8 = p_bme280->ctrl_hum_reg;
				com_rslt += bme280_write_register(BME280_CTRL_HUMIDITY_REG, &v_pre_ctrl_hum_value_u8, BME280_GEN_READ_WRITE_DATA_LENGTH);
				/* write previous value of control measurement register*/
				pre_ctrl_meas_value = p_bme280->ctrl_meas_reg;
				com_rslt += bme280_write_register(BME280_CTRL_MEAS_REG, &pre_ctrl_meas_value, BME280_GEN_READ_WRITE_DATA_LENGTH);
			}
			else {
				com_rslt =
				p_bme280->BME280_BUS_WRITE_FUNC(p_bme280->dev_addr, BME280_CONFIG_REG_TSB__REG, &v_data_u8, BME280_GEN_READ_WRITE_DATA_LENGTH);
			}
			/* read the control measurement register value*/
			com_rslt += bme280_read_register(BME280_CTRL_MEAS_REG, &v_data_u8, BME280_GEN_READ_WRITE_DATA_LENGTH);
			p_bme280->ctrl_meas_reg = v_data_u8;
			/* read the control humidity register value*/
			com_rslt += bme280_read_register(BME280_CTRL_HUMIDITY_REG, &v_data_u8, BME280_GEN_READ_WRITE_DATA_LENGTH);
			p_bme280->ctrl_hum_reg = v_data_u8;
			/* read the configuration register value*/
			com_rslt += bme280_read_register(BME280_CONFIG_REG, &v_data_u8, BME280_GEN_READ_WRITE_DATA_LENGTH);
			p_bme280->config_reg = v_data_u8;
		}
	return com_rslt;
}
/*
 * @brief Writes the working mode to the sensor
 *
 *
 *
 *
 *  @param v_work_mode_u8 : Mode to be set
 *  value    | Working mode
 * ----------|--------------------
 *   0       | BME280_ULTRALOWPOWER_MODE
 *   1       | BME280_LOWPOWER_MODE
 *   2       | BME280_STANDARDRESOLUTION_MODE
 *   3       | BME280_HIGHRESOLUTION_MODE
 *   4       | BME280_ULTRAHIGHRESOLUTION_MODE
 *
 *	@return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 *
 *
*/
/*BME280_RETURN_FUNCTION_TYPE bme280_set_work_mode(u8 v_work_mode_u8)
{
BME280_RETURN_FUNCTION_TYPE com_rslt = ERROR;
u8 v_data_u8 = BME280_INIT_VALUE;
if (p_bme280 == BME280_NULL) {
	return E_BME280_NULL_PTR;
} else {
	if (v_work_mode_u8 <= BME280_ULTRAHIGHRESOLUTION_MODE) {
		com_rslt = p_bme280->BME280_BUS_READ_FUNC(
			p_bme280->dev_addr,	BME280_CTRL_MEAS_REG,
			&v_data_u8, BME280_GEN_READ_WRITE_DATA_LENGTH);
		if (com_rslt == SUCCESS) {
			switch (v_work_mode_u8) {
			case BME280_ULTRALOWPOWER_MODE:
				p_bme280->oversamp_temperature =
				BME280_ULTRALOWPOWER_OSRS_T;
				p_bme280->osrs_p =
				BME280_ULTRALOWPOWER_OSRS_P;
				break;
			case BME280_LOWPOWER_MODE:
				p_bme280->oversamp_temperature =
				BME280_LOWPOWER_OSRS_T;
				p_bme280->osrs_p = BME280_LOWPOWER_OSRS_P;
				break;
			case BME280_STANDARDRESOLUTION_MODE:
				p_bme280->oversamp_temperature =
				BME280_STANDARDRESOLUTION_OSRS_T;
				p_bme280->osrs_p =
				BME280_STANDARDRESOLUTION_OSRS_P;
				break;
			case BME280_HIGHRESOLUTION_MODE:
				p_bme280->oversamp_temperature =
				BME280_HIGHRESOLUTION_OSRS_T;
				p_bme280->osrs_p = BME280_HIGHRESOLUTION_OSRS_P;
				break;
			case BME280_ULTRAHIGHRESOLUTION_MODE:
				p_bme280->oversamp_temperature =
				BME280_ULTRAHIGHRESOLUTION_OSRS_T;
				p_bme280->osrs_p =
				BME280_ULTRAHIGHRESOLUTION_OSRS_P;
				break;
			}
			v_data_u8 = BME280_SET_BITSLICE(v_data_u8,
				BME280_CTRL_MEAS_REG_OVERSAMP_TEMPERATURE,
				p_bme280->oversamp_temperature);
			v_data_u8 = BME280_SET_BITSLICE(v_data_u8,
				BME280_CTRL_MEAS_REG_OVERSAMP_PRESSURE,
				p_bme280->osrs_p);
			com_rslt += p_bme280->BME280_BUS_WRITE_FUNC(
				p_bme280->dev_addr,	BME280_CTRL_MEAS_REG,
				&v_data_u8, BME280_GEN_READ_WRITE_DATA_LENGTH);
		}
	} else {
		com_rslt = E_BME280_OUT_OF_RANGE;
	}
}
return com_rslt;
}*/
/*!
 * @brief This API used to read uncompensated
 * temperature,pressure and humidity in forced mode
 *
 *
 *	@param v_uncom_pressure_s32: The value of uncompensated pressure
 *	@param v_uncom_temperature_s32: The value of uncompensated temperature
 *	@param v_uncom_humidity_s32: The value of uncompensated humidity
 *
 *
 *	@return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 *
 *
*/
//---------------------------------------------------------------------------------
BME280_RETURN_FUNCTION_TYPE bme280_get_forced_uncomp_pressure_temperature_humidity(
		s32 *v_uncom_pressure_s32, s32 *v_uncom_temperature_s32, s32 *v_uncom_humidity_s32)
{
	/* used to return the communication result*/
	BME280_RETURN_FUNCTION_TYPE com_rslt = ERROR;
	u8 v_data_u8 = BME280_INIT_VALUE;
	u8 v_waittime_u8 = BME280_INIT_VALUE;
	u8 v_prev_pow_mode_u8 = BME280_INIT_VALUE;
	u8 v_mode_u8r = BME280_INIT_VALUE;
	u8 pre_ctrl_config_value = BME280_INIT_VALUE;
	u8 v_pre_ctrl_hum_value_u8 = BME280_INIT_VALUE;

	// check the p_bme280 structure pointer as NULL
	if (p_bme280 == BME280_NULL) return E_BME280_NULL_PTR;
	else {
		v_mode_u8r = p_bme280->ctrl_meas_reg;
		v_mode_u8r = BME280_SET_BITSLICE(v_mode_u8r, BME280_CTRL_MEAS_REG_POWER_MODE, BME280_FORCED_MODE);

		com_rslt = bme280_get_power_mode(&v_prev_pow_mode_u8);
		if (v_prev_pow_mode_u8 != BME280_SLEEP_MODE) {
			com_rslt += bme280_set_soft_rst();
			p_bme280->delay_msec(BME280_3MS_DELAY);
			// write previous and updated value of configuration register
			pre_ctrl_config_value = p_bme280->config_reg;
			com_rslt += bme280_write_register(BME280_CONFIG_REG, &pre_ctrl_config_value, BME280_GEN_READ_WRITE_DATA_LENGTH);
			// write previous value of humidity oversampling
			v_pre_ctrl_hum_value_u8 =
			p_bme280->ctrl_hum_reg;
			com_rslt += bme280_write_register(BME280_CTRL_HUMIDITY_REG,	&v_pre_ctrl_hum_value_u8, BME280_GEN_READ_WRITE_DATA_LENGTH);
			// write the force mode
			com_rslt += bme280_write_register(BME280_CTRL_MEAS_REG, &v_mode_u8r, BME280_GEN_READ_WRITE_DATA_LENGTH);
		}
		else {
			// write previous value of humidity oversampling
			v_pre_ctrl_hum_value_u8 = p_bme280->ctrl_hum_reg;
			com_rslt += bme280_write_register(BME280_CTRL_HUMIDITY_REG,	&v_pre_ctrl_hum_value_u8, BME280_GEN_READ_WRITE_DATA_LENGTH);
			// write the force mode
			com_rslt += bme280_write_register( BME280_CTRL_MEAS_REG, &v_mode_u8r, BME280_GEN_READ_WRITE_DATA_LENGTH);
		}

		bme280_compute_wait_time(&v_waittime_u8);
		p_bme280->delay_msec(v_waittime_u8);
		// read the force-mode value of pressure temperature and humidity
		com_rslt += bme280_read_uncomp_pressure_temperature_humidity(v_uncom_pressure_s32, v_uncom_temperature_s32,	v_uncom_humidity_s32);

		// read the control humidity register value
		com_rslt += bme280_read_register(BME280_CTRL_HUMIDITY_REG, &v_data_u8, BME280_GEN_READ_WRITE_DATA_LENGTH);
		p_bme280->ctrl_hum_reg = v_data_u8;
		// read the configuration register value
		com_rslt += bme280_read_register(BME280_CONFIG_REG,	&v_data_u8, BME280_GEN_READ_WRITE_DATA_LENGTH);
		p_bme280->config_reg = v_data_u8;

		// read the control measurement register value
		com_rslt += bme280_read_register(BME280_CTRL_MEAS_REG, &v_data_u8, BME280_GEN_READ_WRITE_DATA_LENGTH);
		p_bme280->ctrl_meas_reg = v_data_u8;
	}
	return com_rslt;
}
/*!
 * @brief
 *	This API write the data to
 *	the given register
 *
 *
 *	@param v_addr_u8 -> Address of the register
 *	@param v_data_u8 -> The data from the register
 *	@param v_len_u8 -> no of bytes to read
 *
 *
 *	@return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 *
 *
 */
BME280_RETURN_FUNCTION_TYPE bme280_write_register(u8 v_addr_u8,
u8 *v_data_u8, u8 v_len_u8)
{
	/* used to return the communication result*/
	BME280_RETURN_FUNCTION_TYPE com_rslt = ERROR;
	/* check the p_bme280 structure pointer as NULL*/
	if (p_bme280 == BME280_NULL) {
		return E_BME280_NULL_PTR;
		} else {
			com_rslt = p_bme280->BME280_BUS_WRITE_FUNC(
			p_bme280->dev_addr,
			v_addr_u8, v_data_u8, v_len_u8);
		}
	return com_rslt;
}
/*!
 * @brief
 *	This API reads the data from
 *	the given register
 *
 *
 *	@param v_addr_u8 -> Address of the register
 *	@param v_data_u8 -> The data from the register
 *	@param v_len_u8 -> no of bytes to read
 *
 *
 *	@return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 *
 *
 */
BME280_RETURN_FUNCTION_TYPE bme280_read_register(u8 v_addr_u8,
u8 *v_data_u8, u8 v_len_u8)
{
	/* used to return the communication result*/
	BME280_RETURN_FUNCTION_TYPE com_rslt = ERROR;
	/* check the p_bme280 structure pointer as NULL*/
	if (p_bme280 == BME280_NULL) {
		return E_BME280_NULL_PTR;
		} else {
			com_rslt = p_bme280->BME280_BUS_READ_FUNC(
			p_bme280->dev_addr,
			v_addr_u8, v_data_u8, v_len_u8);
		}
	return com_rslt;
}
#ifdef BME280_ENABLE_FLOAT

/*!
 * @brief Reads actual temperature from uncompensated temperature
 * @note returns the value in Degree centigrade
 * @note Output value of "51.23" equals 51.23 DegC.
 *
 *
 *
 *  @param v_uncom_temperature_s32 : value of uncompensated temperature
 *
 *
 *
 *  @return  Return the actual temperature in floating point
 *
*/
double bme280_compensate_temperature_double(s32 v_uncom_temperature_s32)
{
	double v_x1_u32 = BME280_INIT_VALUE;
	double v_x2_u32 = BME280_INIT_VALUE;
	double temperature = BME280_INIT_VALUE;

	/*
	var1 = (t / 16384.0 - T[1] / 1024.0) * T[2]
	var2 = ((t / 131072.0 - T[1] / 8192.0) * (t / 131072.0 - T[1] / 8192.0)) * T[3]
	tfine = var1 + var2
	t = tfine / 5120.0
	*/
	v_x1_u32  = ((((double)v_uncom_temperature_s32) / 16384.0) - ((double)p_bme280->cal_param.dig_T1) / 1024.0) * ((double)p_bme280->cal_param.dig_T2);

	v_x2_u32  = ((((double)v_uncom_temperature_s32) / 131072.0 - ((double)p_bme280->cal_param.dig_T1) / 8192.0) *
			((((double)v_uncom_temperature_s32) / 131072.0) - ((double)p_bme280->cal_param.dig_T1) / 8192.0)) *
			((double)p_bme280->cal_param.dig_T3);

	p_bme280->cal_param.t_fine = (s32)(v_x1_u32 + v_x2_u32);
	p_bme280->cal_param.tfine = v_x1_u32 + v_x2_u32;
	temperature  = p_bme280->cal_param.tfine / 5120.0;


	return temperature;
}
/*!
 * @brief Reads actual pressure from uncompensated pressure
 * @note Returns pressure in Pa as double.
 * @note Output value of "96386.2"
 * equals 96386.2 Pa = 963.862 hPa.
 *
 *
 *  @param v_uncom_pressure_s32 : value of uncompensated pressure
 *
 *
 *  @return  Return the actual pressure in floating point
 *
*/
double bme280_compensate_pressure_double(s32 v_uncom_pressure_s32)
{
	double v_x1_u32 = BME280_INIT_VALUE;
	double v_x2_u32 = BME280_INIT_VALUE;
	double pressure = BME280_INIT_VALUE;

	/*
	var1 = tfine / 2.0 - 64000.0
	var2 = var1 * var1 * P[6] / 32768.0
	var2 = var2 + var1 * P[5] * 2.0
	var2 = var2 / 4.0 + P[4] * 65536.0
	var1 = (P[3] * var1 * var1 / 524288.0 + P[2] * var1) / 524288.0
	var1 = (1.0 + var1 / 32768.0) * P[1]
	if var1 == 0 then
	p=nil
	else
	p = 1048576.0 - p
	p = ((p - var2 / 4096.0) * 6250.0) / var1
	var1 = P[9] * p * p / 2147483648.0
	var2 = p * P[8] / 32768.0
	p = p + (var1 + var2 + P[7]) / 16.0
	p=p/100.0
	end
	*/
	//v_x1_u32 = ((double)p_bme280->cal_param.t_fine / 2.0) - 64000.0;
	v_x1_u32 = (p_bme280->cal_param.tfine / 2.0) - 64000.0;
	v_x2_u32 = v_x1_u32 * v_x1_u32 * ((double)p_bme280->cal_param.dig_P6) / 32768.0;
	v_x2_u32 = v_x2_u32 + (v_x1_u32 * ((double)p_bme280->cal_param.dig_P5) * 2.0);

	v_x2_u32 = (v_x2_u32 / 4.0) + (((double)p_bme280->cal_param.dig_P4) * 65536.0);

	v_x1_u32 = (((double)p_bme280->cal_param.dig_P3) * v_x1_u32 * v_x1_u32 / 524288.0 +
			(((double)p_bme280->cal_param.dig_P2) * v_x1_u32)) / 524288.0;

	v_x1_u32 = (1.0 + (v_x1_u32 / 32768.0)) * ((double)p_bme280->cal_param.dig_P1);

	// Avoid exception caused by division by zero
	if (v_x1_u32 == 0) return BME280_INVALID_DATA;

	pressure = 1048576.0 - (double)v_uncom_pressure_s32;
	pressure = ((pressure - (v_x2_u32 / 4096.0)) * 6250.0) / v_x1_u32;

	v_x1_u32 = ((double)p_bme280->cal_param.dig_P9) * pressure * pressure / 2147483648.0;
	v_x2_u32 = pressure * ((double)p_bme280->cal_param.dig_P8) / 32768.0;
	pressure = pressure + (v_x1_u32 + v_x2_u32 + ((double)p_bme280->cal_param.dig_P7)) / 16.0;

	return pressure;
}
/*!
 * @brief Reads actual humidity from uncompensated humidity
 * @note returns the value in relative humidity (%rH)
 * @note Output value of "42.12" equals 42.12 %rH
 *
 *  @param v_uncom_humidity_s32 : value of uncompensated humidity
 *
 *
 *
 *  @return Return the actual humidity in floating point
 *
*/
double bme280_compensate_humidity_double(s32 v_uncom_humidity_s32)
{
	double var_h = BME280_INIT_VALUE;

	//var_h = (((double)p_bme280->cal_param.t_fine) - 76800.0);
	var_h = p_bme280->cal_param.tfine - 76800.0;
	if ((var_h > 0) || (var_h < 0))
		var_h = (v_uncom_humidity_s32 -
		(((double)p_bme280->cal_param.dig_H4) * 64.0 +
		((double)p_bme280->cal_param.dig_H5) / 16384.0 * var_h))*
		(((double)p_bme280->cal_param.dig_H2) / 65536.0 *
		(1.0 + ((double) p_bme280->cal_param.dig_H6)
		/ 67108864.0 * var_h * (1.0 + ((double)
		p_bme280->cal_param.dig_H3) / 67108864.0 * var_h)));
	else return BME280_INVALID_DATA;

	var_h = var_h * (1.0 - ((double)
	p_bme280->cal_param.dig_H1)*var_h / 524288.0);
	if (var_h > 100.0)
		var_h = 100.0;
	else if (var_h < 0.0)
		var_h = 0.0;
	return var_h;

}
#endif
#if defined(BME280_ENABLE_INT64) && defined(BME280_64BITSUPPORT_PRESENT)
/*!
 * @brief Reads actual pressure from uncompensated pressure
 * @note Returns the value in Pa as unsigned 32 bit
 * integer in Q24.8 format (24 integer bits and
 * 8 fractional bits).
 * @note Output value of "24674867"
 * represents 24674867 / 256 = 96386.2 Pa = 963.862 hPa
 *
 *
 *
 *  @param  v_uncom_pressure_s32 : value of uncompensated temperature
 *
 *
 *  @return Return the actual pressure in u32
 *
*/
u32 bme280_compensate_pressure_int64(s32 v_uncom_pressure_s32)
{
	s64 v_x1_s64r = BME280_INIT_VALUE;
	s64 v_x2_s64r = BME280_INIT_VALUE;
	s64 pressure = BME280_INIT_VALUE;

	v_x1_s64r = ((s64)p_bme280->cal_param.t_fine) - 128000;
	v_x2_s64r = v_x1_s64r * v_x1_s64r *	(s64)p_bme280->cal_param.dig_P6;
	v_x2_s64r = v_x2_s64r + ((v_x1_s64r * (s64)p_bme280->cal_param.dig_P5) << BME280_SHIFT_BIT_POSITION_BY_17_BITS);
	v_x2_s64r = v_x2_s64r +	(((s64)p_bme280->cal_param.dig_P4) << BME280_SHIFT_BIT_POSITION_BY_35_BITS);

	v_x1_s64r = ((v_x1_s64r * v_x1_s64r * (s64)p_bme280->cal_param.dig_P3) >> BME280_SHIFT_BIT_POSITION_BY_08_BITS) +
			((v_x1_s64r * (s64)p_bme280->cal_param.dig_P2) << BME280_SHIFT_BIT_POSITION_BY_12_BITS);

	v_x1_s64r = (((((s64)1)	<< BME280_SHIFT_BIT_POSITION_BY_47_BITS) + v_x1_s64r)) *
			((s64)p_bme280->cal_param.dig_P1) >> BME280_SHIFT_BIT_POSITION_BY_33_BITS;

	// Avoid exception caused by division by zero
	if (v_x1_s64r == 0) return BME280_INVALID_DATA;

	pressure = 1048576 - v_uncom_pressure_s32;
	pressure = (((pressure << BME280_SHIFT_BIT_POSITION_BY_31_BITS) - v_x2_s64r) * 3125) / v_x1_s64r;
	v_x1_s64r = (((s64)p_bme280->cal_param.dig_P9) * (pressure >> BME280_SHIFT_BIT_POSITION_BY_13_BITS) *
			(pressure >> BME280_SHIFT_BIT_POSITION_BY_13_BITS))	>> BME280_SHIFT_BIT_POSITION_BY_25_BITS;
	v_x2_s64r = (((s64)p_bme280->cal_param.dig_P8) * pressure) >> BME280_SHIFT_BIT_POSITION_BY_19_BITS;
	pressure = (((pressure + v_x1_s64r + v_x2_s64r) >> BME280_SHIFT_BIT_POSITION_BY_08_BITS) +
			(((s64)p_bme280->cal_param.dig_P7) << BME280_SHIFT_BIT_POSITION_BY_04_BITS));

	return (u32)pressure;
}
/*!
 * @brief Reads actual pressure from uncompensated pressure
 * @note Returns the value in Pa.
 * @note Output value of "12337434"
 * @note represents 12337434 / 128 = 96386.2 Pa = 963.862 hPa
 *
 *
 *
 *  @param v_uncom_pressure_s32 : value of uncompensated pressure
 *
 *
 *  @return the actual pressure in u32
 *
*/
u32 bme280_compensate_pressure_int64_twentyfour_bit_output(s32 v_uncom_pressure_s32)
{
	u32 pressure = BME280_INIT_VALUE;

	pressure = bme280_compensate_pressure_int64( v_uncom_pressure_s32);
	pressure = (u32)(pressure >> BME280_SHIFT_BIT_POSITION_BY_01_BIT);
	return pressure;
}
#endif
/*!
 * @brief Computing waiting time for sensor data read
 *
 *
 *
 *
 *  @param v_delaytime_u8 : The value of delay time for force mode
 *
 *
 *	@retval 0 -> Success
 *
 *
 */
BME280_RETURN_FUNCTION_TYPE bme280_compute_wait_time(u8 *v_delaytime_u8)
{
	/* used to return the communication result*/
	BME280_RETURN_FUNCTION_TYPE com_rslt = SUCCESS;

	*v_delaytime_u8 = (T_INIT_MAX +	T_MEASURE_PER_OSRS_MAX *
			(((1 << p_bme280->oversamp_temperature) >> BME280_SHIFT_BIT_POSITION_BY_01_BIT)	+
			((1 << p_bme280->oversamp_pressure)	>> BME280_SHIFT_BIT_POSITION_BY_01_BIT) +
			((1 << p_bme280->oversamp_humidity)	>> BME280_SHIFT_BIT_POSITION_BY_01_BIT)) +
			((p_bme280->oversamp_pressure > 0)
			? T_SETUP_PRESSURE_MAX : 0) + ((p_bme280->oversamp_humidity > 0) ? T_SETUP_HUMIDITY_MAX : 0) + 15) / 16;
	return com_rslt;
}

// ================================ BME280 ===========================================

//--------------------------------------------------------------
static void print_driver_error(driver_error_t *error, int err) {
	//printf(" DRIVER ERROR [%d]: type: %d, unit: %d, exc: %d\r\n", err, error->type, error->unit, error->exception);

    free(error);
}

/*	\Brief          : The function is used as I2C bus write
 *	\Return         : Status of the I2C write
 *	\param dev_addr : The device address of the sensor
 *	\param reg_addr : Address of the register to which data is going to be written
 *	\param reg_data : Register data array, will be used for write the values into the register
 *	\param cnt      : The number of bytes to be written
 */

//---------------------------------------------------------------------
s8 BME280_I2C_bus_write(u8 dev_addr, u8 reg_addr, u8 *reg_data, u8 cnt)
{
	driver_error_t *error;

	/*printf("[bme280 wr] (%02x) %02x:", dev_addr, reg_addr);
	for (int i=0; i<cnt; i++) {
		printf(" %02x", reg_data[i]);
	}*/
    if ((error = i2c_start(p_bme280->unit, &p_bme280->transaction))) {
    	print_driver_error(error, -1);
    	return -1;
    }
	if ((error = i2c_write_address(p_bme280->unit, &p_bme280->transaction, dev_addr, 0))) {
    	print_driver_error(error, -2);
    	return -2;
    }
    if ((error = i2c_write(p_bme280->unit, &p_bme280->transaction, (char *)&reg_addr, 1))) {
    	print_driver_error(error, -3);
    	return -3;
    }
    if ((error = i2c_write(p_bme280->unit, &p_bme280->transaction, (char *)reg_data, cnt))) {
    	print_driver_error(error, -4);
    	return -4;
    }
    if ((error = i2c_stop(p_bme280->unit, &p_bme280->transaction))) {
    	print_driver_error(error, -5);
    	return -5;
    }

	//printf("\r\n");
	return 0;
}

 /*	\Brief          : The function is used as I2C bus write&read
 *	\Return         : Status of the I2C write&read
 *	\param dev_addr : The device address of the sensor
 *	\param reg_addr : Address of the first register, from where data is going to be read
 *	\param reg_data : Array of data read from the sensor
 *	\param cnt      : The number of data bytes to be read
 */
//--------------------------------------------------------------------
s8 BME280_I2C_bus_read(u8 dev_addr, u8 reg_addr, u8 *reg_data, u8 cnt)
{
	driver_error_t *error;

	//printf("[bme280 rd] (%02x) [%d] %02x:", dev_addr, cnt, reg_addr);
    if ((error = i2c_start(p_bme280->unit, &p_bme280->transaction))) {
    	print_driver_error(error, -1);
    	return -1;
    }
	if ((error = i2c_write_address(p_bme280->unit, &p_bme280->transaction, dev_addr, 0))) {
    	print_driver_error(error, -2);
    	return -2;
    }
    if ((error = i2c_write(p_bme280->unit, &p_bme280->transaction, (char *)&reg_addr, 1))) {
    	print_driver_error(error, -3);
    	return -3;
    }
	// read reg data
    if ((error = i2c_start(p_bme280->unit, &p_bme280->transaction))) {
    	print_driver_error(error, -5);
    	return -6;
    }
	if ((error = i2c_write_address(p_bme280->unit, &p_bme280->transaction, dev_addr, 1))) {
    	print_driver_error(error, -7);
    	return -7;
    }
	if ((error = i2c_read(p_bme280->unit, &p_bme280->transaction, (char *)reg_data, cnt))) {
    	print_driver_error(error, -8);
    	return -8;
    }
    if ((error = i2c_stop(p_bme280->unit, &p_bme280->transaction))) {
    	print_driver_error(error, -10);
    	return -10;
    }

	/*for (int i=0; i<cnt; i++) {
		printf(" %02x", reg_data[i]);
	}
	printf("\r\n");*/
    return 0;
}

/*	Brief : The delay routine
 *	\param : delay in ms
*/
//------------------------------
void BME280_delay_msek(u32 msek)
{
    vTaskDelay(msek / portTICK_RATE_MS);
}

//--------------------------------------------------------------
static void _bme280_get(double *temp, double *hum, double *pres)
{
	// The variable used to read uncompensated temperature
	s32 v_data_uncomp_temp_s32 = BME280_INIT_VALUE;
	// The variable used to read uncompensated pressure
	s32 v_data_uncomp_pres_s32 = BME280_INIT_VALUE;
	// The variable used to read uncompensated humidity
	s32 v_data_uncomp_hum_s32 = BME280_INIT_VALUE;

	s32 com_rslt = ERROR;

	if (p_bme280->chip_id == BME280_CHIP_ID) {
		if (p_bme280->mode != BME280_NORMAL_MODE)
			com_rslt = bme280_get_forced_uncomp_pressure_temperature_humidity(
					&v_data_uncomp_pres_s32, &v_data_uncomp_temp_s32, &v_data_uncomp_hum_s32);
		else com_rslt = bme280_read_uncomp_pressure_temperature_humidity(
				&v_data_uncomp_pres_s32, &v_data_uncomp_temp_s32, &v_data_uncomp_hum_s32);
		if (com_rslt == 0) {
			//printf("BME280 READ uncomp %d, data: %d  %d  %d\r\n", com_rslt, v_data_uncomp_pres_s32, v_data_uncomp_temp_s32, v_data_uncomp_hum_s32);
			*temp = bme280_compensate_temperature_double(v_data_uncomp_temp_s32);
			*pres = bme280_compensate_pressure_double(v_data_uncomp_pres_s32);
			*hum = bme280_compensate_humidity_double(v_data_uncomp_hum_s32);
		}
	}

}

//------------------------------------
static uint8_t bme280_getsby(int sb) {
	uint8_t sby;
	if (sb < 10) sby = BME280_STANDBY_TIME_1_MS;
	else if (sb < 20) sby = BME280_STANDBY_TIME_10_MS;
	else if (sb < 62) sby = BME280_STANDBY_TIME_20_MS;
	else if (sb < 125) sby = BME280_STANDBY_TIME_63_MS;
	else if (sb < 250) sby = BME280_STANDBY_TIME_125_MS;
	else if (sb < 500) sby = BME280_STANDBY_TIME_250_MS;
	else if (sb < 1000) sby = BME280_STANDBY_TIME_500_MS;
	else sby = BME280_STANDBY_TIME_1000_MS;

	return sby;
}

//----------------------------------------
static int bme280_getsby_ms(int sby) {
	int sb = 1000;
	if (sby == BME280_STANDBY_TIME_1_MS) sb = 1;
	else if (sby == BME280_STANDBY_TIME_10_MS) sb = 10;
	else if (sby == BME280_STANDBY_TIME_20_MS) sb = 20;
	else if (sby == BME280_STANDBY_TIME_63_MS) sb = 63;
	else if (sby == BME280_STANDBY_TIME_125_MS) sb = 125;
	else if (sby == BME280_STANDBY_TIME_250_MS) sb = 250;
	else if (sby == BME280_STANDBY_TIME_500_MS) sb = 500;
	return sb;
}

//------------------------------------------------------
int bm280_get_mode(sensor_instance_t *unit, char *buf) {
    p_bme280 = unit->setup[0].i2c.userdata;
	u8 mode = 255;
	u8 sby = 255;
	s32 com_rslt = ERROR;
	int sb = 1000;
	char smode[16];

	if (p_bme280->chip_id == BME280_CHIP_ID) {
		com_rslt = bme280_get_power_mode(&mode);
		if (com_rslt == 0) {
			com_rslt += bme280_get_standby_durn(&sby);
			if (com_rslt != 0) sby = 255;
		}
		else mode = 255;
	}

	if (sby < 255) {
		if (sby == BME280_STANDBY_TIME_1_MS) sb = 1;
		else if (sby == BME280_STANDBY_TIME_10_MS) sb = 10;
		else if (sby == BME280_STANDBY_TIME_20_MS) sb = 20;
		else if (sby == BME280_STANDBY_TIME_63_MS) sb = 63;
		else if (sby == BME280_STANDBY_TIME_125_MS) sb = 125;
		else if (sby == BME280_STANDBY_TIME_250_MS) sb = 250;
		else if (sby == BME280_STANDBY_TIME_500_MS) sb = 500;
	}
	switch (mode) {
		case 0:
			sprintf(smode, "SLEEP");
			break;
		case 1:
			sprintf(smode, "FORCED");
			break;
		case 3:
			sprintf(smode, "NORMAL");
			break;
		default:
			sprintf(smode, "unknown");
			break;
	}
	if (buf != NULL) {
		if (mode == BME280_NORMAL_MODE) sprintf(buf, "%s STANDBY=%d", smode, sb);
		else sprintf(buf, "%s", smode);
	}

	return mode;
}

/*
 * Operation functions
 */

/*
 * Initialize BME280 sensor
 * Mode is set to sleep mode
 * Optional parameters can be given: addres is detected automaticaly
 */
//-----------------------------------------------------
driver_error_t *bme280_presetup(sensor_instance_t *unit) {
	// Set default values, if not provided
	if (unit->setup[0].i2c.devid == 0) {
		unit->setup[0].i2c.devid = BME280_I2C_ADDRESS1;
	}

	if (unit->setup[0].i2c.speed == 0) {
		unit->setup[0].i2c.speed = 400000;
	}

	return NULL;
}

driver_error_t *bme280_setup(sensor_instance_t *unit) {
    s32 com_rslt = ERROR;

    // Sanity checks
	if ((unit->setup[0].i2c.devid != BME280_I2C_ADDRESS1) && (unit->setup[0].i2c.devid != BME280_I2C_ADDRESS2)) {
		return driver_error(SENSOR_DRIVER, SENSOR_ERR_INVALID_ADDRESS, NULL);
	}

    p_bme280 = calloc(sizeof(struct bme280_user_data_t), sizeof(char));
    if (!p_bme280) {
		return driver_error(SENSOR_DRIVER, SENSOR_ERR_NOT_ENOUGH_MEMORY, "NULL");
    }

	// Set default mode & standby time & address
	unit->properties[0].integerd.value = BME280_SLEEP_MODE;
	unit->properties[1].integerd.value = BME280_STANDBY_TIME_125_MS;
	unit->properties[2].integerd.value = unit->setup[0].i2c.devid;

	// Allocate space for buffer
	char *buffer = (char *)calloc(32, 1);
	if (!buffer) {
		return driver_error(SENSOR_DRIVER, SENSOR_ERR_NOT_ENOUGH_MEMORY, NULL);
	}
	sprintf(buffer, "SLEEP");
	unit->properties[3].stringd.value = buffer;

	p_bme280->unit = unit->setup[0].i2c.id;
	p_bme280->transaction = I2C_TRANSACTION_INITIALIZER;
	unit->setup[0].i2c.userdata = p_bme280;

    p_bme280->chip_id = 0;
	p_bme280->mode = 255;
	p_bme280->bus_write = BME280_I2C_bus_write;
	p_bme280->bus_read = BME280_I2C_bus_read;
	p_bme280->dev_addr = unit->properties[2].integerd.value;
	p_bme280->delay_msec = BME280_delay_msek;

	/*--------------------------------------------------------------------------*
	 *  This function used to assign the value/reference of
	 *	the following parameters
	 *	I2C address
	 *	Bus Write
	 *	Bus read
	 *	Chip id
	*-------------------------------------------------------------------------*/
	com_rslt = bme280_init();
	/* I2C auto detect
	if (com_rslt != BME280_CHIP_ID_READ_SUCCESS) {
		(p_bme280->dev_addr == BME280_I2C_ADDRESS1) ? p_bme280->dev_addr == BME280_I2C_ADDRESS2:p_bme280->dev_addr == BME280_I2C_ADDRESS1;
		p_bme280->dev_addr = BME280_I2C_ADDRESS2;
		com_rslt = bme280_init();
	}
	*/
	if (com_rslt == BME280_CHIP_ID_READ_SUCCESS) {
		p_bme280->mode = BME280_SLEEP_MODE;

		com_rslt += bme280_set_soft_rst();
		BME280_delay_msek(5);

		//	For reading the pressure, humidity and temperature data it is required to
		//	set the OSS setting of humidity, pressure and temperature
		// set the humidity oversampling
		com_rslt += bme280_set_oversamp_humidity(BME280_OVERSAMP_1X);
		// set the pressure oversampling
		com_rslt += bme280_set_oversamp_pressure(BME280_OVERSAMP_2X);
		// set the temperature oversampling
		com_rslt += bme280_set_oversamp_temperature(BME280_OVERSAMP_4X);
		// set standby time
		com_rslt += bme280_set_standby_durn(BME280_STANDBY_TIME_125_MS);

		com_rslt += bme280_set_power_mode(BME280_SLEEP_MODE);
	}

	// **************** END INITIALIZATION ****************
	if (com_rslt == 0) {
		// read measured values, first read after init returns wrong values
		BME280_delay_msek(BME280_STANDBY_TIME_125_MS);
		double temp = 0.0;
		double pres = 0.0;
		double hum = 0.0;
		_bme280_get(&temp, &hum, &pres);
	}
	else {
		return driver_error(SENSOR_DRIVER, SENSOR_ERR_CANT_INIT, "cannot detect device");
	}

	return NULL;
}

//-------------------------------------------------------------------------------
driver_error_t *bme280_acquire(sensor_instance_t *unit, sensor_value_t *values) {
    if (!unit->setup[0].i2c.userdata) {
		return driver_error(SENSOR_DRIVER, SENSOR_ERR_NOT_SETUP, NULL);
    }
    p_bme280 = unit->setup[0].i2c.userdata;

	double temp = 0.0;
	double pres = 0.0;
	double hum = 0.0;
	_bme280_get(&temp, &hum, &pres);

	values[0].doubled.value = temp;
	values[1].doubled.value = hum;
	values[2].doubled.value = pres / 100.0;

	return NULL;
}

//---------------------------------------------------------------------------------------------
driver_error_t *bme280_get(sensor_instance_t *unit, const char *id, sensor_value_t *property) {
    if (!unit->setup[0].i2c.userdata) {
		return driver_error(SENSOR_DRIVER, SENSOR_ERR_NOT_SETUP, NULL);
    }
    p_bme280 = unit->setup[0].i2c.userdata;

    if (strcmp(id,"mode") == 0) {
		property->integerd.value  = bm280_get_mode(unit, NULL);
	}
	else if (strcmp(id,"standbytime") == 0) {
		property->integerd.value  = bme280_getsby_ms(unit->properties[1].integerd.value);
	}
	else if (strcmp(id,"address") == 0) {
		property->integerd.value  = unit->properties[2].integerd.value;
	}
	else if (strcmp(id,"smode") == 0) {
		// Free previous value, if needed
		if (!property->stringd.value) {
			// Allocate space for buffer
			property->stringd.value = (char *)calloc(32, 1);
			if (!property->stringd.value) {
				return driver_error(SENSOR_DRIVER, SENSOR_ERR_NOT_ENOUGH_MEMORY, NULL);
			}
		}
		bm280_get_mode(unit, property->stringd.value);
	}

	return NULL;
}

/*
 * Set BME280 operating mode
 * Parameters:
 * mode: operating mode
 *       0: sleep mode, no operation, lowest power
 *       1: forced mode, perform one measurement, return to sleep mode
 *       3: normal mode, continuous measurements with inactive periods between
 */
//--------------------------------------------------------------------------------------------
driver_error_t *bme280_set(sensor_instance_t *unit, const char *id, sensor_value_t *property) {
    if (!unit->setup[0].i2c.userdata) {
		return driver_error(SENSOR_DRIVER, SENSOR_ERR_NOT_SETUP, NULL);
    }
    p_bme280 = unit->setup[0].i2c.userdata;

    if (strcmp(id,"mode") == 0) {
		u8 mode, md;
		s32 com_rslt = ERROR;

		if (p_bme280->chip_id == BME280_CHIP_ID) {
			md = property->integerd.value;
			if ((md == BME280_SLEEP_MODE) || (md == BME280_FORCED_MODE) || (md == BME280_NORMAL_MODE)) {
				com_rslt = bme280_get_power_mode(&mode);
				if (com_rslt == 0) {
					if (mode != md) {
						com_rslt = bme280_set_power_mode(md);
						if (com_rslt == 0) {
							p_bme280->mode = md;
							unit->properties[0].integerd.value = md;
							if (md == BME280_NORMAL_MODE) {
								// set standby time for normal mode
								u8 sby = unit->properties[1].integerd.value;
								if (sby < 8) com_rslt = bme280_set_standby_durn(sby);
							}
							// set mode string
							if (!unit->properties[3].stringd.value) {
								// Allocate space for buffer
								property->stringd.value = (char *)calloc(32, 1);
								if (!property->stringd.value) {
									return driver_error(SENSOR_DRIVER, SENSOR_ERR_NOT_ENOUGH_MEMORY, NULL);
								}
							}
							bm280_get_mode(unit, unit->properties[3].stringd.value);
						}
					}
				}
			}
			else {
				return driver_error(SENSOR_DRIVER, SENSOR_ERR_CANT_INIT, "mode: 0 (sleep), 1 (forced), 3 (normal)");
			}
		}
	}
    else if (strcmp(id,"standbytime") == 0) {
		if (unit->properties[0].integerd.value == BME280_NORMAL_MODE) {
			u8 sby = bme280_getsby(property->integerd.value);
			bme280_set_standby_durn(sby);
			unit->properties[1].integerd.value = bme280_getsby(property->integerd.value);
		}
    }
    else if (strcmp(id,"address") == 0) {
    	unit->properties[2].integerd.value =property->integerd.value;
    }

	return NULL;
}

#endif
#endif
