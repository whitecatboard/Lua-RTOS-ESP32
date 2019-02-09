/**
 * ONE WIRE driver for Lua-RTOS-ESP32
 * author: LoBo (loboris@gmail.com)
 * based on TM_ONEWIRE (author  Tilen Majerle)
 */

#include "luartos.h"

#include <sys/syslog.h>
#include <string.h>

#include <drivers/cpu.h>
#include <drivers/owire.h>
#include <drivers/gpio.h>
#include <stdio.h>

#define OWIRE_FIRST_PIN	1
#define OWIRE_LAST_PIN	31

TM_One_Wire_Devices_t ow_devices[MAX_ONEWIRE_PINS];

// Convert address to device
int8_t owire_addess_to_dev(uint8_t sensor, uint64_t address) {
	if (address < 255) {
		return address;
	}

	for (uint8_t i=0;i<MAX_ONEWIRE_SENSORS;i++) {
		if ((uint64_t)(ow_devices[sensor].roms[i][0]) == address) return i;
	}

	return -1;
}

// Check if owire pin is already setup
int owire_checkpin(uint8_t pin) {
	for (uint8_t i=0;i<MAX_ONEWIRE_PINS;i++) {
		if (ow_devices[i].device.pin == pin) return i;
	}
	return -1;
}

TM_One_Wire_Devices_t *ow_getdevice(uint8_t dev) {
	if (ow_devices[dev].device.pin == 0) return NULL;
	else return &ow_devices[dev];
}

void ow_devices_init(uint8_t dev) {
	ow_devices[dev].device.LastDeviceFlag = 0;
	ow_devices[dev].device.LastDiscrepancy = 0;
	ow_devices[dev].device.LastFamilyDiscrepancy = 0;
	memset(ow_devices[dev].device.ROM_NO, 0, sizeof(ow_devices[dev].device.ROM_NO));
	ow_devices[dev].numdev = 0;
	memset(ow_devices[dev].roms, 0, sizeof(ow_devices[dev].roms));
}

// Register driver and messages
void owire_init();

DRIVER_REGISTER_BEGIN(OWIRE,owire,CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS * (CPU_LAST_GPIO + 1),owire_init,NULL);
	DRIVER_REGISTER_ERROR(OWIRE, owire, CannotSetup, "can't setup", OWIRE_ERR_CANT_INIT);
	DRIVER_REGISTER_ERROR(OWIRE, owire, InvalidChannel, "invalid channel", OWIRE_ERR_INVALID_CHANNEL);
DRIVER_REGISTER_END(OWIRE,owire,CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS * (CPU_LAST_GPIO + 1),owire_init,NULL);

// Get the pins used by an ONE WIRE channel
void owire_pins(int8_t owpin, uint8_t *pin) {
	if ((owpin >= OWIRE_FIRST_PIN) && (owpin <= OWIRE_LAST_PIN)) *pin = owpin;
}

// Lock resources needed by ONE WIRE
driver_error_t *owire_lock_resources(int8_t pin, void *resources) {
	// Check if pin already setup for owire
    int owdev = owire_checkpin(pin);
	if (owdev >= 0) return NULL;

    owdev = owire_checkpin(0);
	if (owdev == -1) {
		return driver_error(OWIRE_DRIVER, OWIRE_ERR_CANT_INIT, "max devices reached");
	}

#if CONFIG_LUA_RTOS_USE_HARDWARE_LOCKS
	owire_resources_t tmp_owire_resources;

	if (!resources) {
		resources = &tmp_owire_resources;
	}

	owire_resources_t *owire_resources = (owire_resources_t *)resources;
    driver_unit_lock_error_t *lock_error = NULL;

    owire_pins(pin, &owire_resources->pin);

    // Lock owire pin
    if ((lock_error = driver_lock(OWIRE_DRIVER, pin, GPIO_DRIVER, owire_resources->pin, DRIVER_ALL_FLAGS, NULL))) {
    	// Revoked lock on pin
    	return driver_lock_error(OWIRE_DRIVER, lock_error);
    }
#endif

	ow_devices[owdev].device.pin = pin;

	return NULL;
}

// Setup an ONE WIRE channel
driver_error_t *owire_setup_pin(int8_t pin) {
	// Check if pin already setup for owire
	if (owire_checkpin(pin) >= 0) return NULL;

	// Sanity checks
	if ((pin < OWIRE_FIRST_PIN) || (pin > OWIRE_LAST_PIN)) {
		return driver_error(OWIRE_DRIVER, OWIRE_ERR_CANT_INIT, "invalid pin");
	}

    // Lock resources
    driver_error_t *error;
    owire_resources_t resources;

    if ((error = owire_lock_resources(pin, &resources))) {
		return error;
	}

    return NULL;
}

void owire_init() {
	memset(ow_devices, 0, sizeof(TM_One_Wire_Devices_t) * MAX_ONEWIRE_PINS);
}

//******************
// ONEWIRE FUNCTIONS
//******************

//--------------------------------
void owdevice_input(uint8_t dev) {
	gpio_set_pull_mode(ow_devices[dev].device.pin, GPIO_PULLUP_ONLY);
	gpio_set_direction(ow_devices[dev].device.pin, GPIO_MODE_INPUT);
}

//-----------------------------------
void owdevice_pinpower(uint8_t dev) {
	gpio_set_direction(ow_devices[dev].device.pin, GPIO_MODE_OUTPUT);
	gpio_set_level(ow_devices[dev].device.pin,1);
}

//-------------------------------------------
unsigned char TM_OneWire_Reset(uint8_t dev) {
	unsigned char bit = 1;
	int i;
	portDISABLE_INTERRUPTS();
	// Set line low and wait ~500 us
	gpio_set_direction(ow_devices[dev].device.pin, GPIO_MODE_OUTPUT);
	gpio_set_level(ow_devices[dev].device.pin,0);
	ets_delay_us(500);

	// Release the line and wait 500 us for line value
	gpio_set_level(ow_devices[dev].device.pin,1);
	gpio_set_direction(ow_devices[dev].device.pin, GPIO_MODE_INPUT);
	i = 500;
	while (i > 0) {
		ets_delay_us(10);
		if (gpio_get_level(ow_devices[dev].device.pin) == 0){
	    	bit = 0;
	    	break;
	    }
	    i -= 10;
	}
	if ((i > 0) && (bit == 0)) {
		// wait up to 500 us
		ets_delay_us(i);
	}
	portENABLE_INTERRUPTS();
    // Return value of presence pulse, 0 = OK, 1 = ERROR
    return bit;
}

// ow WRITE slot
//---------------------------------------------------------------
static void TM_OneWire_WriteBit(uint8_t dev, unsigned char bit) {
  portDISABLE_INTERRUPTS();
  if (bit) {
	// ** Bit high
	// Set line low and wait 8 us
	gpio_set_direction(ow_devices[dev].device.pin, GPIO_MODE_OUTPUT);
	gpio_set_level(ow_devices[dev].device.pin,0);
	ets_delay_us(8);

	// Release the line and wait ~65 us
	gpio_set_level(ow_devices[dev].device.pin,1);
	gpio_set_direction(ow_devices[dev].device.pin, GPIO_MODE_INPUT);
	ets_delay_us(65);
  }
  else {
    // ** Bit low
	// Set line low and wait ~65 us
	gpio_set_direction(ow_devices[dev].device.pin, GPIO_MODE_OUTPUT);
	gpio_set_level(ow_devices[dev].device.pin,0);
	ets_delay_us(65);

	// Release the line and wait 5 us
	gpio_set_level(ow_devices[dev].device.pin,1);
	gpio_set_direction(ow_devices[dev].device.pin, GPIO_MODE_INPUT);
	ets_delay_us(5);
  }
  portENABLE_INTERRUPTS();
}

// ow READ slot
//---------------------------------------------
unsigned char TM_OneWire_ReadBit(uint8_t dev) {
	unsigned char bit = 1;
	int i;

    portDISABLE_INTERRUPTS();
	// Set line low and wait 3 us
	gpio_set_direction(ow_devices[dev].device.pin, GPIO_MODE_OUTPUT);
	gpio_set_level(ow_devices[dev].device.pin,0);
	ets_delay_us(3);

	// Release the line and wait ~65 us for line value
	gpio_set_level(ow_devices[dev].device.pin,1);
	gpio_set_direction(ow_devices[dev].device.pin, GPIO_MODE_INPUT);
	i = 66;
	while (i > 0) {
		ets_delay_us(2);
		if (gpio_get_level(ow_devices[dev].device.pin) == 0){
	    	bit = 0;
	    	break;
	    }
	    i -= 2;
	}
	if ((i > 0) && (bit == 0)) {
		// wait up to 65 us
		ets_delay_us(i);
	}
    portENABLE_INTERRUPTS();
	// Return bit value
	return bit;
}

//----------------------------------------------------------
void TM_OneWire_WriteByte(uint8_t dev, unsigned char byte) {
  unsigned char i = 8;
  // Write 8 bits
  while (i--) {
    // LSB bit is first
    TM_OneWire_WriteBit(dev, byte & 0x01);
    byte >>= 1;
  }
}

//----------------------------------------------
unsigned char TM_OneWire_ReadByte(uint8_t dev) {
  unsigned char i = 8, byte = 0;
  while (i--) {
    byte >>= 1;
    byte |= (TM_OneWire_ReadBit(dev) << 7);
  }
  return byte;
}

//-----------------------------------------------
static void TM_OneWire_ResetSearch(uint8_t dev) {
  // Reset the search state
  ow_devices[dev].device.LastDiscrepancy = 0;
  ow_devices[dev].device.LastDeviceFlag = 0;
  ow_devices[dev].device.LastFamilyDiscrepancy = 0;
}

//-------------------------------------------------------------------
unsigned char TM_OneWire_Search(uint8_t dev, unsigned char command) {
  unsigned char id_bit_number;
  unsigned char last_zero, rom_byte_number, search_result;
  unsigned char id_bit, cmp_id_bit;
  unsigned char rom_byte_mask, search_direction;

  /* Initialize for search */
  id_bit_number = 1;
  last_zero = 0;
  rom_byte_number = 0;
  rom_byte_mask = 1;
  search_result = 0;
  // if the last call was not the last one
  if (!ow_devices[dev].device.LastDeviceFlag) {
    // 1-Wire reset
    if (TM_OneWire_Reset(dev)) {
      /* Reset the search */
      ow_devices[dev].device.LastDiscrepancy = 0;
      ow_devices[dev].device.LastDeviceFlag = 0;
      ow_devices[dev].device.LastFamilyDiscrepancy = 0;
      return 0;
    }
    // issue the search command
    TM_OneWire_WriteByte(dev, command);
    // loop to do the search
    do {
      // read a bit and its complement
      id_bit = TM_OneWire_ReadBit(dev);
      cmp_id_bit = TM_OneWire_ReadBit(dev);
      // check for no devices on 1-wire
      if ((id_bit == 1) && (cmp_id_bit == 1)) {
        break;
      } else {
        // all devices coupled have 0 or 1
        if (id_bit != cmp_id_bit) {
          search_direction = id_bit;  // bit write value for search
        } else {
          // if this discrepancy if before the Last Discrepancy
          // on a previous next then pick the same as last time
          if (id_bit_number < ow_devices[dev].device.LastDiscrepancy) {
            search_direction = ((ow_devices[dev].device.ROM_NO[rom_byte_number] & rom_byte_mask) > 0);
          } else {
            // if equal to last pick 1, if not then pick 0
            search_direction = (id_bit_number == ow_devices[dev].device.LastDiscrepancy);
          }
          // if 0 was picked then record its position in LastZero
          if (search_direction == 0) {
            last_zero = id_bit_number;
            // check for Last discrepancy in family
            if (last_zero < 9) {
              ow_devices[dev].device.LastFamilyDiscrepancy = last_zero;
            }
          }
        }
        // set or clear the bit in the ROM byte rom_byte_number
        // with mask rom_byte_mask
        if (search_direction == 1) {
          ow_devices[dev].device.ROM_NO[rom_byte_number] |= rom_byte_mask;
        } else {
          ow_devices[dev].device.ROM_NO[rom_byte_number] &= ~rom_byte_mask;
        }
        // serial number search direction write bit
        TM_OneWire_WriteBit(dev, search_direction);
        // increment the byte counter id_bit_number
        // and shift the mask rom_byte_mask
        id_bit_number++;
        rom_byte_mask <<= 1;
        // if the mask is 0 then go to new SerialNum byte rom_byte_number and reset mask
        if (rom_byte_mask == 0) {
          //docrc8(ROM_NO[rom_byte_number]);  // accumulate the CRC
          rom_byte_number++;
          rom_byte_mask = 1;
        }
      }
    } while (rom_byte_number < 8);  // loop until through all ROM bytes 0-7

    // if the search was successful then
    if (!(id_bit_number < 65)) {
      // search successful so set LastDiscrepancy,LastDeviceFlag,search_result
      ow_devices[dev].device.LastDiscrepancy = last_zero;
      // check for last device
      if (ow_devices[dev].device.LastDiscrepancy == 0) {
        ow_devices[dev].device.LastDeviceFlag = 1;
      }
      search_result = 1;
    }
  }

  // if no device found then reset counters so next 'search' will be like a first
  if (!search_result || !ow_devices[dev].device.ROM_NO[0]) {
    ow_devices[dev].device.LastDiscrepancy = 0;
    ow_devices[dev].device.LastDeviceFlag = 0;
    ow_devices[dev].device.LastFamilyDiscrepancy = 0;
    search_result = 0;
  }

  return search_result;
}

//-------------------------------------------
unsigned char TM_OneWire_First(uint8_t dev) {
  // Reset search values
  TM_OneWire_ResetSearch(dev);
  // Start with searching
  return TM_OneWire_Search(dev, ONEWIRE_CMD_SEARCHROM);
}

//------------------------------------------
unsigned char TM_OneWire_Next(uint8_t dev) {
  // Leave the search state alone
  return TM_OneWire_Search(dev, ONEWIRE_CMD_SEARCHROM);
}

/*
//------------------------------
static int TM_OneWire_Verify() {
  unsigned char rom_backup[8];
  int i,rslt,ld_backup,ldf_backup,lfd_backup;
  // keep a backup copy of the current state
  for (i = 0; i < 8; i++)
    rom_backup[i] = ow_devices[dev].device.ROM_NO[i];
  ld_backup = ow_devices[dev].device.LastDiscrepancy;
  ldf_backup = ow_devices[dev].device.LastDeviceFlag;
  lfd_backup = ow_devices[dev].device.LastFamilyDiscrepancy;
  // set search to find the same device
  ow_devices[dev].device.LastDiscrepancy = 64;
  ow_devices[dev].device.LastDeviceFlag = 0;
  if (TM_OneWire_Search(ONEWIRE_CMD_SEARCHROM)) {
    // check if same device found
    rslt = 1;
    for (i = 0; i < 8; i++) {
      if (rom_backup[i] != ow_devices[dev].device.ROM_NO[i]) {
        rslt = 1;
        break;
      }
    }
  } else {
    rslt = 0;
  }
  // restore the search state
  for (i = 0; i < 8; i++) {
    ow_devices[dev].device.ROM_NO[i] = rom_backup[i];
  }
  ow_devices[dev].device.LastDiscrepancy = ld_backup;
  ow_devices[dev].device.LastDeviceFlag = ldf_backup;
  ow_devices[dev].device.LastFamilyDiscrepancy = lfd_backup;
  // return the result of the verify
  return rslt;
}
//-------------------------------------------------------
static void TM_OneWire_TargetSetup(unsigned char family_code) {
  unsigned char i;
  // set the search state to find SearchFamily type devices
  ow_devices[dev].device.ROM_NO[0] = family_code;
  for (i = 1; i < 8; i++) {
    ow_devices[dev].device.ROM_NO[i] = 0;
  }
  ow_devices[dev].device.LastDiscrepancy = 64;
  ow_devices[dev].device.LastFamilyDiscrepancy = 0;
  ow_devices[dev].device.LastDeviceFlag = 0;
}
//----------------------------------------
static void TM_OneWire_FamilySkipSetup() {
  // set the Last discrepancy to last family discrepancy
  ow_devices[dev].device.LastDiscrepancy = ow_devices[dev].device.LastFamilyDiscrepancy;
  ow_devices[dev].device.LastFamilyDiscrepancy = 0;
  // check for end of list
  if (ow_devices[dev].device.LastDiscrepancy == 0) {
    ow_devices[dev].device.LastDeviceFlag = 1;
  }
}
//-----------------------------------------------
static unsigned char TM_OneWire_GetROM(unsigned char index) {
  return ow_devices[dev].device.ROM_NO[index];
}
//--------------------------------------------
static void TM_OneWire_Select(unsigned char* addr) {
  unsigned char i;
  TM_OneWire_WriteByte(ONEWIRE_CMD_MATCHROM);
  for (i = 0; i < 8; i++) {
    TM_OneWire_WriteByte(*(addr + i));
  }
}
*/

//------------------------------------------------------------------
void TM_OneWire_SelectWithPointer(uint8_t dev, unsigned char *ROM) {
  unsigned char i;
  TM_OneWire_WriteByte(dev, ONEWIRE_CMD_MATCHROM);
  for (i = 0; i < 8; i++) {
    TM_OneWire_WriteByte(dev, *(ROM + i));
  }
}

//------------------------------------------------------------------
void TM_OneWire_GetFullROM(uint8_t dev, unsigned char *firstIndex) {
  unsigned char i;
  for (i = 0; i < 8; i++) {
    *(firstIndex + i) = ow_devices[dev].device.ROM_NO[i];
  }
}

//---------------------------------------------------------------------
unsigned char TM_OneWire_CRC8(unsigned char *addr, unsigned char len) {
  unsigned char crc = 0, inbyte, i, mix;

  while (len--) {
    inbyte = *addr++;
    for (i = 8; i; i--) {
      mix = (crc ^ inbyte) & 0x01;
      crc >>= 1;
      if (mix) {
        crc ^= 0x8C;
      }
      inbyte >>= 1;
    }
  }
  /* Return calculated CRC */
  return crc;
}

//----------------------------------------
uint8_t TM_OneWire_Dosearch(uint8_t dev) {
	// Search for devices on owire bus
	uint8_t count = 0;
	uint8_t owdev = 0;
	owdev = TM_OneWire_First(dev);
	while (owdev) {
		count++;  // Increase device counter
		// Get full ROM value, 8 bytes, give location of first byte where to save
		TM_OneWire_GetFullROM(dev, ow_devices[dev].roms[count - 1]);
		// Get next device
		owdev = TM_OneWire_Next(dev);
		if (count >= MAX_ONEWIRE_SENSORS) break;
	}

	ow_devices[dev].numdev = count;

	return count;
}
