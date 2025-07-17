/**
 * DS1820 Family temperature sensor driver for Lua-RTOS-ESP32
 * author: LoBo (loboris@gmail.com)
 * based on TM_ONEWIRE (author  Tilen Majerle)
 */

#include "luartos.h"

#if CONFIG_LUA_RTOS_LUA_USE_SENSOR
#if CONFIG_LUA_RTOS_USE_SENSOR_DS1820

#include <math.h>
#include <string.h>
#include <stdio.h>

#include "sensors/ds1820.h"
#include "time.h"
#include <drivers/owire.h>
#include <sys/driver.h>

static int ds_parasite_pwr = 0;
extern TM_One_Wire_Devices_t ow_devices[MAX_ONEWIRE_PINS];

#ifdef DS18B20ALARMFUNC
static unsigned char ow_alarm_device [MAX_ONEWIRE_SENSORS][8];
#endif

// Sensor specification and registration
const sensor_t __attribute__((used,unused,section(".sensors"))) ds1820_sensor = {
	.id = "DS1820",
	.interface = {
		{.type = OWIRE_INTERFACE},
	},
	.data = {
		{.id = "temperature", .type = SENSOR_DATA_FLOAT},
	},
	.properties = {
		{.id = "resolution", .type = SENSOR_DATA_INT},
		{.id = "rom", .type = SENSOR_DATA_STRING},
		{.id = "type", .type = SENSOR_DATA_STRING},
		{.id = "numdev", .type = SENSOR_DATA_INT},
	},
	.setup = ds1820_setup,
	.acquire = ds1820_acquire,
	.set = ds1820_set,
	.get = ds1820_get,
};


//*********************
// TM_DS18B20_Functions
//*********************

//-----------------------------------------------
unsigned char TM_DS18B20_Is(unsigned char *ROM) {
  /* Checks if first byte is equal to DS18B20's family code */
  if ((*ROM == DS18B20_FAMILY_CODE) ||
	  (*ROM == DS18S20_FAMILY_CODE) ||
	  (*ROM == DS1822_FAMILY_CODE)  ||
	  (*ROM == DS28EA00_FAMILY_CODE)) {
    return *ROM;
  }
  return 0;
}

//-----------------------------------------------------------------
static void TM_DS18B20_Family(unsigned char code, char *dsfamily) {
  switch (code) {
  	  case DS18B20_FAMILY_CODE:
	  	  sprintf(dsfamily, "DS18B20");
	  	  break;
  	  case DS18S20_FAMILY_CODE:
	  	  sprintf(dsfamily, "DS18S20");
	  	  break;
  	  case DS1822_FAMILY_CODE:
	  	  sprintf(dsfamily, "DS1822");
	  	  break;
  	  case DS28EA00_FAMILY_CODE:
	  	  sprintf(dsfamily, "DS28EA00");
	  	  break;
  	  default:
  		  sprintf(dsfamily, "unknown");
  }
}

//------------------------------------
static int getPowerMode(uint8_t dev) {
	// Reset pulse
	if (TM_OneWire_Reset(dev) != 0) return owError_NoDevice;
	// Skip rom
	TM_OneWire_WriteByte(dev, ONEWIRE_CMD_SKIPROM);
	// Test parasite power
	TM_OneWire_WriteByte(dev, ONEWIRE_CMD_RPWRSUPPLY);
	if (TM_OneWire_ReadBit(dev) == 0) ds_parasite_pwr = 1;
	else ds_parasite_pwr = 0;
	return ow_OK;
}

//------------------------------------------------------------------
static owState_t TM_DS18B20_Start(uint8_t dev, unsigned char *ROM) {
  // Check if device is DS18B20
  if (!TM_DS18B20_Is(ROM)) {
    return owError_Not18b20;
  }
  if (getPowerMode(dev)) return owError_NoDevice;

  // Reset line
  if (TM_OneWire_Reset(dev) != 0) return owError_NoDevice;

  // Select ROM number
  TM_OneWire_SelectWithPointer(dev, ROM);
  // Start temperature conversion
  TM_OneWire_WriteByte(dev, DS18B20_CMD_CONVERTTEMP);

  if (ds_parasite_pwr) owdevice_pinpower(dev);

  return ow_OK;
}

//-------------------------------------------------
#if 0
static owState_t TM_DS18B20_StartAll(uint8_t dev) {
  if (getPowerMode(dev)) return owError_NoDevice;

  // Reset pulse
  if (TM_OneWire_Reset(dev) != 0) return owError_NoDevice;;
  // Skip rom
  TM_OneWire_WriteByte(dev, ONEWIRE_CMD_SKIPROM);
  // Start conversion on all connected devices
  TM_OneWire_WriteByte(dev, DS18B20_CMD_CONVERTTEMP);

  if (ds_parasite_pwr) owdevice_pinpower(dev);

  return ow_OK;
}
#endif

//--------------------------------------------------------------------------------------
static owState_t TM_DS18B20_Read(uint8_t dev, unsigned char *ROM, double *destination) {
  unsigned int temperature;
  unsigned char resolution;
  char digit, minus = 0;
  double decimal;
  unsigned char i = 0;
  unsigned char data[9];
  unsigned char crc;

  /* Check if device is DS18B20 */
  if (!TM_DS18B20_Is(ROM)) {
    return owError_Not18b20;
  }
  /* Check if line is released, if it is, then conversion is complete */
  if (!TM_OneWire_ReadBit(dev)) {
    /* Conversion is not finished yet */
    return owError_NotFinished;
  }
  /* Reset line */
  if (TM_OneWire_Reset(dev) != 0) {
    return owError_NoDevice;
  }
  /* Select ROM number */
  TM_OneWire_SelectWithPointer(dev, ROM);
  /* Read scratchpad command by onewire protocol */
  TM_OneWire_WriteByte(dev, ONEWIRE_CMD_RSCRATCHPAD);

  /* Get data */
  for (i = 0; i < 9; i++) {
    /* Read byte by byte */
    data[i] = TM_OneWire_ReadByte(dev);
  }
  /* Calculate CRC */
  crc = TM_OneWire_CRC8(data, 8);
  /* Check if CRC is ok */
  if (crc != data[8]) {
    /* CRC invalid */
    return owError_BadCRC;
  }

  /* First two bytes of scratchpad are temperature values */
  temperature = data[0] | (data[1] << 8);
  /* Reset line */
  TM_OneWire_Reset(dev);
  if (*ROM != DS18S20_FAMILY_CODE) {
	  /* Check if temperature is negative */
	  if (temperature & 0x8000) {
		/* Two's complement, temperature is negative */
		temperature = ~temperature + 1;
		minus = 1;
	  }
	  /* Get sensor resolution */
	  resolution = ((data[4] & 0x60) >> 5) + 9;
	  /* Store temperature integer digits and decimal digits */
	  digit = temperature >> 4;
	  digit |= ((temperature >> 8) & 0x7) << 4;

	  /* Store decimal digits */
	  switch (resolution) {
		case 9: {
		  decimal = (temperature >> 3) & 0x01;
		  decimal *= (double)DS18B20_DECIMAL_STEPS_9BIT;
		} break;
		case 10: {
		  decimal = (temperature >> 2) & 0x03;
		  decimal *= (double)DS18B20_DECIMAL_STEPS_10BIT;
		} break;
		case 11: {
		  decimal = (temperature >> 1) & 0x07;
		  decimal *= (double)DS18B20_DECIMAL_STEPS_11BIT;
		} break;
		case 12: {
		  decimal = temperature & 0x0F;
		  decimal *= (double)DS18B20_DECIMAL_STEPS_12BIT;
		} break;
		default: {
		  decimal = 0xFF;
		  digit = 0;
		}
	  }

	  /* Check for negative part */
	  decimal = digit + decimal;
	  if (minus) {
		decimal = 0 - decimal;
	  }
	  /* Set to pointer */
	  *destination = decimal;

  }
  else {
	if (!data[7]) {
	    return owError_Convert;
	}
	if (data[1] == 0) {
		temperature = ((int)(data[0] >> 1))*1000;
	}
	else { // negative
		temperature = 1000*(-1*(int)(0x100-data[0]) >> 1);
	}
	temperature -= 250;
	decimal = 1000*((int)(data[7] - data[6]));
	decimal /= (int)data[7];
	temperature += decimal;
    /* Set to pointer */
	*destination = (double)temperature / 1000.0;
  }
  /* Return 1, temperature valid */
  return ow_OK;
}

//------------------------------------------------------------------------------
static unsigned char TM_DS18B20_GetResolution(uint8_t dev, unsigned char *ROM) {
  unsigned char conf;

  if (!TM_DS18B20_Is(ROM)) {
    return owError_Not18b20;
  }

  if (*ROM == DS18S20_FAMILY_CODE) return TM_DS18B20_Resolution_12bits;

  /* Reset line */
  if (TM_OneWire_Reset(dev) != 0) {
    return owError_NoDevice;
  }
  /* Select ROM number */
  TM_OneWire_SelectWithPointer(dev, ROM);
  /* Read scratchpad command by onewire protocol */
  TM_OneWire_WriteByte(dev, ONEWIRE_CMD_RSCRATCHPAD);

  /* Ignore first 4 bytes */
  TM_OneWire_ReadByte(dev);
  TM_OneWire_ReadByte(dev);
  TM_OneWire_ReadByte(dev);
  TM_OneWire_ReadByte(dev);

  /* 5th byte of scratchpad is configuration register */
  conf = TM_OneWire_ReadByte(dev);

  /* Return 9 - 12 value according to number of bits */
  return ((conf & 0x60) >> 5) + 9;
}

//--------------------------------------------------------------------------------------------------------------
static owState_t TM_DS18B20_SetResolution(uint8_t dev, unsigned char *ROM, TM_DS18B20_Resolution_t resolution) {
  unsigned char th, tl, conf;

  if (!TM_DS18B20_Is(ROM)) return owError_Not18b20;
  if (getPowerMode(dev)) return owError_NoDevice;
  if (*ROM == DS18S20_FAMILY_CODE) return ow_OK;

  /* Reset line */
  if (TM_OneWire_Reset(dev) != 0) return owError_NoDevice;

  /* Select ROM number */
  TM_OneWire_SelectWithPointer(dev, ROM);
  /* Read scratchpad command by onewire protocol */
  TM_OneWire_WriteByte(dev, ONEWIRE_CMD_RSCRATCHPAD);

  /* Ignore first 2 bytes */
  TM_OneWire_ReadByte(dev);
  TM_OneWire_ReadByte(dev);

  th = TM_OneWire_ReadByte(dev);
  tl = TM_OneWire_ReadByte(dev);
  conf = TM_OneWire_ReadByte(dev);

  if (resolution == TM_DS18B20_Resolution_9bits) {
    conf &= ~(1 << DS18B20_RESOLUTION_R1);
    conf &= ~(1 << DS18B20_RESOLUTION_R0);
  } else if (resolution == TM_DS18B20_Resolution_10bits) {
    conf &= ~(1 << DS18B20_RESOLUTION_R1);
    conf |= 1 << DS18B20_RESOLUTION_R0;
  } else if (resolution == TM_DS18B20_Resolution_11bits) {
    conf |= 1 << DS18B20_RESOLUTION_R1;
    conf &= ~(1 << DS18B20_RESOLUTION_R0);
  } else if (resolution == TM_DS18B20_Resolution_12bits) {
    conf |= 1 << DS18B20_RESOLUTION_R1;
    conf |= 1 << DS18B20_RESOLUTION_R0;
  }

  /* Reset line */
  if (TM_OneWire_Reset(dev) != 0) return owError_NoDevice;

  /* Select ROM number */
  TM_OneWire_SelectWithPointer(dev, ROM);
  /* Write scratchpad command by onewire protocol, only th, tl and conf register can be written */
  TM_OneWire_WriteByte(dev, ONEWIRE_CMD_WSCRATCHPAD);

  /* Write bytes */
  TM_OneWire_WriteByte(dev, th);
  TM_OneWire_WriteByte(dev, tl);
  TM_OneWire_WriteByte(dev, conf);

  /* Reset line */
  if (TM_OneWire_Reset(dev) != 0) return owError_NoDevice;

  /* Select ROM number */
  TM_OneWire_SelectWithPointer(dev, ROM);
  /* Copy scratchpad to EEPROM of DS18B20 */
  TM_OneWire_WriteByte(dev, ONEWIRE_CMD_CPYSCRATCHPAD);

  if (ds_parasite_pwr) owdevice_pinpower(dev);
  vTaskDelay(20 / portTICK_RATE_MS);
  if (ds_parasite_pwr) owdevice_input(dev);

  return ow_OK;
}

/***************************/
/* DS18B20 Alarm functions */
/***************************/
#ifdef DS18B20ALARMFUNC
//-------------------------------------------------------------------------------------------------
static unsigned char TM_DS18B20_SetAlarmLowTemperature(uint8_t dev, unsigned char *ROM, int temp) {
  unsigned char tl, th, conf;
  if (!TM_DS18B20_Is(ROM)) {
    return owError_Not18b20;
  }
  if (temp > 125) {
    temp = 125;
  }
  if (temp < -55) {
    temp = -55;
  }
  /* Reset line */
  if (TM_OneWire_Reset(dev) != 0) {
    return owError_NoDevice;
  }
  /* Select ROM number */
  TM_OneWire_SelectWithPointer(dev, ROM);
  /* Read scratchpad command by onewire protocol */
  TM_OneWire_WriteByte(dev, ONEWIRE_CMD_RSCRATCHPAD);

  /* Ignore first 2 bytes */
  TM_OneWire_ReadByte(dev);
  TM_OneWire_ReadByte(dev);

  th = TM_OneWire_ReadByte(dev);
  tl = TM_OneWire_ReadByte(dev);
  conf = TM_OneWire_ReadByte(dev);

  tl = (unsigned char)temp;

  /* Reset line */
  if (TM_OneWire_Reset(dev) != 0) {
    return owError_NoDevice;
  }
  /* Select ROM number */
  TM_OneWire_SelectWithPointer(dev, ROM);
  /* Write scratchpad command by onewire protocol, only th, tl and conf register can be written */
  TM_OneWire_WriteByte(dev, ONEWIRE_CMD_WSCRATCHPAD);

  /* Write bytes */
  TM_OneWire_WriteByte(dev, th);
  TM_OneWire_WriteByte(dev, tl);
  TM_OneWire_WriteByte(dev, conf);

  /* Reset line */
  if (TM_OneWire_Reset(dev) != 0) {
    return owError_NoDevice;
  }
  /* Select ROM number */
  TM_OneWire_SelectWithPointer(dev, ROM);
  /* Copy scratchpad to EEPROM of DS18B20 */
  TM_OneWire_WriteByte(dev, ONEWIRE_CMD_CPYSCRATCHPAD);

  return ow_OK;
}

//----------------------------------------------------------------------------------------------
static owState_t TM_DS18B20_SetAlarmHighTemperature(uint8_t dev, unsigned char *ROM, int temp) {
  unsigned char tl, th, conf;
  if (!TM_DS18B20_Is(ROM)) {
    return owError_Not18b20;
  }
  if (temp > 125) {
    temp = 125;
  }
  if (temp < -55) {
    temp = -55;
  }
  /* Reset line */
  if (TM_OneWire_Reset(dev) != 0) {
    return owError_NoDevice;
  }
  /* Select ROM number */
  TM_OneWire_SelectWithPointer(dev, ROM);
  /* Read scratchpad command by onewire protocol */
  TM_OneWire_WriteByte(dev, ONEWIRE_CMD_RSCRATCHPAD);

  /* Ignore first 2 bytes */
  TM_OneWire_ReadByte(dev);
  TM_OneWire_ReadByte(dev);

  th = TM_OneWire_ReadByte(dev);
  tl = TM_OneWire_ReadByte(dev);
  conf = TM_OneWire_ReadByte(dev);

  th = (unsigned char)temp;

  /* Reset line */
  if (TM_OneWire_Reset(dev) != 0) {
    return owError_NoDevice;
  }
  /* Select ROM number */
  TM_OneWire_SelectWithPointer(dev, ROM);
  /* Write scratchpad command by onewire protocol, only th, tl and conf register can be written */
  TM_OneWire_WriteByte(dev, ONEWIRE_CMD_WSCRATCHPAD);

  /* Write bytes */
  TM_OneWire_WriteByte(dev, th);
  TM_OneWire_WriteByte(dev, tl);
  TM_OneWire_WriteByte(dev, conf);

  /* Reset line */
  if (TM_OneWire_Reset(dev) != 0) {
    return owError_NoDevice;
  }
  /* Select ROM number */
  TM_OneWire_SelectWithPointer(dev, ROM);
  /* Copy scratchpad to EEPROM of DS18B20 */
  TM_OneWire_WriteByte(dev, ONEWIRE_CMD_CPYSCRATCHPAD);

  return ow_OK;
}

//------------------------------------------------------------------------------------
static owState_t TM_DS18B20_DisableAlarmTemperature(uint8_t dev, unsigned char *ROM) {
  unsigned char tl, th, conf;
  if (!TM_DS18B20_Is(ROM)) {
    return owError_Not18b20;
  }
  /* Reset line */
  if (TM_OneWire_Reset(dev) != 0) {
    return owError_NoDevice;
  }
  /* Select ROM number */
  TM_OneWire_SelectWithPointer(dev, ROM);
  /* Read scratchpad command by onewire protocol */
  TM_OneWire_WriteByte(dev, ONEWIRE_CMD_RSCRATCHPAD);

  /* Ignore first 2 bytes */
  TM_OneWire_ReadByte(dev);
  TM_OneWire_ReadByte(dev);

  th = TM_OneWire_ReadByte(dev);
  tl = TM_OneWire_ReadByte(dev);
  conf = TM_OneWire_ReadByte(dev);

  th = 125;
  tl = (unsigned char)-55;

  /* Reset line */
  if (TM_OneWire_Reset(dev) != 0) {
    return owError_NoDevice;
  }
  /* Select ROM number */
  TM_OneWire_SelectWithPointer(dev, ROM);
  /* Write scratchpad command by onewire protocol, only th, tl and conf register can be written */
  TM_OneWire_WriteByte(dev, ONEWIRE_CMD_WSCRATCHPAD);

  /* Write bytes */
  TM_OneWire_WriteByte(dev, th);
  TM_OneWire_WriteByte(dev, tl);
  TM_OneWire_WriteByte(dev, conf);

  /* Reset line */
  if (TM_OneWire_Reset(dev) != 0) {
    return owError_NoDevice;
  }
  /* Select ROM number */
  TM_OneWire_SelectWithPointer(dev, ROM);
  /* Copy scratchpad to EEPROM of DS18B20 */
  TM_OneWire_WriteByte(dev, ONEWIRE_CMD_CPYSCRATCHPAD);

  return ow_OK;
}

//--------------------------------------------------------
static unsigned char TM_DS18B20_AlarmSearch(uint8_t dev) {
  /* Start alarm search */
  return TM_OneWire_Search(dev, DS18B20_CMD_ALARMSEARCH);
}

#endif /* DS18B20ALARMFUNC */


//----------------------------------------
static uint8_t numDS1820dev(uint8_t dev) {
	// get number of DS1820 devices on the bus
	uint8_t count = 0;

	for (uint8_t i=0;i<MAX_ONEWIRE_SENSORS;i++) {
		if (TM_DS18B20_Is(&(ow_devices[dev].roms[i][0]))) count++;
	}
	return count;
}

/*
 * DS1820 sensor functions
 */
//------------------------------------------------------
void ds1820_getrom(sensor_instance_t *unit, char *ROM) {
	uint8_t owdev = unit->setup[0].owire.owdevice;
	uint8_t sens = unit->setup[0].owire.owsensor - 1;

	sens = owire_addess_to_dev(owdev, sens);

	int i;
	for (i = 0; i < 8; i++) {
		sprintf(ROM+(i*2), "%02x", ow_devices[owdev].roms[sens][i]);
	}
}

//-------------------------------------------------------
void ds1820_gettype(sensor_instance_t *unit, char *buf) {
	uint8_t owdev = unit->setup[0].owire.owdevice;
	uint8_t sens = unit->setup[0].owire.owsensor - 1;

	sens = owire_addess_to_dev(owdev, sens);

	uint8_t code = TM_DS18B20_Is(&(ow_devices[owdev].roms[sens][0]));
	if (code) TM_DS18B20_Family(code, buf);
	else sprintf(buf, "unknown");
}

//-----------------------------------------------
uint8_t ds1820_get_res(sensor_instance_t *unit) {
	return unit->properties[0].integerd.value;
}

//-------------------------------------
void ow_list(sensor_instance_t *unit) {
	char rombuf[17];
	char family[12];
	uint8_t owdev = unit->setup[0].owire.owdevice;

	for (int i=0;i<ow_devices[owdev].numdev;i++) {
		for (int j = 0; j < 8; j++) {
			sprintf(rombuf+(j*2), "%02x", ow_devices[owdev].roms[i][j]);
		}
		printf("%02d [%s]", i+1, rombuf);
		uint8_t code = TM_DS18B20_Is(&(ow_devices[owdev].roms[i][0]));
		if (code) {
		  TM_DS18B20_Family(code, family);
		  printf(" %s\r\n", family);
		}
		else printf(" unknown\r\n");
	}
}

//----------------------------------------------
uint8_t ds1820_numdev(sensor_instance_t *unit) {
	uint8_t dev = unit->setup[0].owire.owdevice;

	return numDS1820dev(dev);
}

//---------------------------------------------------------------------------
static uint8_t _set_resolution(uint8_t ds_res, uint8_t dev, uint8_t ds_dev) {
	uint8_t res = ds_res;
	if ( res!=TM_DS18B20_Resolution_9bits &&
	   res!=TM_DS18B20_Resolution_10bits &&
	   res!=TM_DS18B20_Resolution_11bits &&
	   res!=TM_DS18B20_Resolution_12bits ) {
		res = TM_DS18B20_Resolution_10bits;
	}
	if (ow_devices[dev].roms[ds_dev-1][0] == DS18S20_FAMILY_CODE) {
		res = TM_DS18B20_Resolution_9bits;
	}
	else {
		res = TM_DS18B20_SetResolution(dev, ow_devices[dev].roms[ds_dev-1], (TM_DS18B20_Resolution_t)res);
		// Get resolution
		res = TM_DS18B20_GetResolution(dev,ow_devices[dev].roms[ds_dev-1]);
		if (res != TM_DS18B20_Resolution_9bits &&
		   res!=TM_DS18B20_Resolution_10bits &&
		   res!=TM_DS18B20_Resolution_11bits &&
		   res!=TM_DS18B20_Resolution_12bits ) {
			res = TM_DS18B20_Resolution_10bits;
		}
	}
	return res;
}

/*
 * Operation functions
 */
//-----------------------------------------------------
driver_error_t *ds1820_setup(sensor_instance_t *unit) {
	// DS1820 specific setup, check for devices on the bus
	uint8_t dev = unit->setup[0].owire.owdevice;
	uint8_t ds_dev = unit->setup[0].owire.owsensor;

	ds_dev = owire_addess_to_dev(dev, ds_dev);

	// Set default resolution to 10 bit
	unit->properties[0].integerd.value = 10;

	uint8_t numDS182 = numDS1820dev(dev);
	if ((numDS182 == 0) || (ds_dev > numDS182) || (ds_dev == 0)) {
		return driver_error(SENSOR_DRIVER, SENSOR_ERR_CANT_INIT, "device not on bus");
	}

	// check for DS1820 device
	if (!TM_DS18B20_Is(&(ow_devices[dev].roms[ds_dev-1][0]))) {
		return driver_error(SENSOR_DRIVER, SENSOR_ERR_CANT_INIT, "not DS1820 device");
	}

	// Set default resolution (10 bits)
	unit->properties[0].integerd.value = _set_resolution(10, dev, ds_dev);

	// Set exfunc values
	//unit->data[1].exfuncd.value = EXFUNC_DS1820_GETROM;
//	unit->data[2].exfuncd.value = EXFUNC_DS1820_GETTYPE;
	//unit->data[3].exfuncd.value = EXFUNC_DS1820_GETRESOLUTION;
//	unit->data[4].exfuncd.value = EXFUNC_OWIRE_LISTDEV;
	//unit->data[5].exfuncd.value = EXFUNC_DS1820_NUMDEV;

	return NULL;
}

//--------------------------------------------------------------------------------------------
driver_error_t *ds1820_set(sensor_instance_t *unit, const char *id, sensor_value_t *property) {
	if (strcmp(id,"resolution") == 0) {
		unsigned char ds_dev = unit->setup[0].owire.owsensor;
		uint8_t dev = unit->setup[0].owire.owdevice;

		ds_dev = owire_addess_to_dev(dev, ds_dev);

		memcpy(&unit->properties[0], property, sizeof(sensor_value_t));

		// Set sensor's resolution
		unit->properties[0].integerd.value = _set_resolution(property->integerd.value, dev, ds_dev);
	}

	return NULL;
}

//-------------------------------------------------------------------------------
driver_error_t *ds1820_acquire(sensor_instance_t *unit, sensor_value_t *values) {
	unsigned char sens = unit->setup[0].owire.owsensor - 1;
	uint8_t dev = unit->setup[0].owire.owdevice;
	int retries = 0;

	sens = owire_addess_to_dev(dev, sens);

	owState_t stat;
	double temper;
	uint16_t measure_time = 900;

	// set measure time, it depends on resolution
	switch (unit->properties[0].integerd.value) {
	case 9:
		measure_time = 150;
		break;
	case 10:
		measure_time = 250;
		break;
	case 11:
		measure_time = 450;
		break;
	case 12:
		measure_time = 850;
		break;
	}

	/* Start temperature conversion on all devices on one bus
	TM_DS18B20_StartAll(dev);
	*/
	// Start temperature conversion on device

retry:
	if (TM_DS18B20_Start(dev, (unsigned char *)&ow_devices[dev].roms[sens]) != ow_OK) {
        retries++;
        if (retries < 3) {
            vTaskDelay(1 / portTICK_RATE_MS);
            goto retry;
        }

		values[0].floatd.value = -9997.0;
		return NULL;
	}

	// Wait until measurement finished
	if (ds_parasite_pwr) {
		vTaskDelay(measure_time / portTICK_RATE_MS);
		// Set owire pin to input mode
		owdevice_input(dev);
	}
	else {
		for (int mtime = 0; mtime < measure_time; mtime += 10) {
			vTaskDelay(10 / portTICK_RATE_MS);
			if (TM_OneWire_ReadBit(dev)) break;
		}
	}
	vTaskDelay(10 / portTICK_RATE_MS);

	if (!TM_OneWire_ReadBit(dev)) {
        retries++;
        if (retries < 3) {
            vTaskDelay(1 / portTICK_RATE_MS);
            goto retry;
        }
        /* Timeout */
		values[0].floatd.value = -9998.0;

		return NULL;
	}

	// Read temperature from selected device
	// Read temperature from ROM address and store it to temper variable
	stat = TM_DS18B20_Read(dev, ow_devices[dev].roms[sens], &temper);
	if ( stat == ow_OK) {
		values[0].floatd.value = temper;
	}
	else {
		// Reading error
        retries++;
        if (retries < 3) {
            vTaskDelay(1 / portTICK_RATE_MS);
            goto retry;
        }

        values[0].floatd.value = -9999.0;
	}

	return NULL;
}

driver_error_t *ds1820_get(sensor_instance_t *unit, const char *id, sensor_value_t *property) {
	if (strcmp(id,"numdev") == 0) {
		property->integerd.value  = ds1820_numdev(unit);
	} else if (strcmp(id,"rom") == 0) {
		// Free previous value, if needed
		if (property->stringd.value) {
			free(property->stringd.value);
		}

		// Allocate space for buffer
		char *buffer = (char *)calloc(1, 32);
		if (!buffer) {
			return driver_error(SENSOR_DRIVER, SENSOR_ERR_NOT_ENOUGH_MEMORY, NULL);
		}

		ds1820_getrom(unit, buffer);

		property->stringd.value = buffer;
	} else if (strcmp(id,"type") == 0) {
		// Free previous value, if needed
		if (property->stringd.value) {
			free(property->stringd.value);
		}

		// Allocate space for buffer
		char *buffer = (char *)calloc(1, 32);
		if (!buffer) {
			return driver_error(SENSOR_DRIVER, SENSOR_ERR_NOT_ENOUGH_MEMORY, NULL);
		}

		ds1820_gettype(unit, buffer);

		property->stringd.value = buffer;
	}

	return NULL;
}

#endif
#endif
