#if 0
#include "unity.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/delay.h>
#include <sys/driver.h>

#include <drivers/i2c.h>

// Write data to EEPROM 24C256D
//
// Memory address = (addr1 << 8) | addr2
static void eeprom_24c256d_write(char addr1, char addr2, char data) {
	driver_error_t *error;
	int transaction = I2C_TRANSACTION_INITIALIZER;

	error = i2c_start(0, &transaction);
	TEST_ASSERT(transaction != I2C_TRANSACTION_INITIALIZER);
	TEST_ASSERT_MESSAGE(error == NULL, driver_get_err_msg(error));

	error = i2c_write_address(0, &transaction, 0x51, 0);
	TEST_ASSERT_MESSAGE(error == NULL, driver_get_err_msg(error));

	error = i2c_write(0, &transaction, &addr1, sizeof(addr1));
	TEST_ASSERT_MESSAGE(error == NULL, driver_get_err_msg(error));

	error = i2c_write(0, &transaction, &addr2, sizeof(addr2));
	TEST_ASSERT_MESSAGE(error == NULL, driver_get_err_msg(error));

	error = i2c_write(0, &transaction, &data, sizeof(data));
	TEST_ASSERT_MESSAGE(error == NULL, driver_get_err_msg(error));

	error = i2c_stop(0, &transaction);
	TEST_ASSERT_MESSAGE(error == NULL, driver_get_err_msg(error));
}

// Write data from EEPROM 24C256D
//
// Memory address = (addr1 << 8) | addr2
static char eeprom_24c256d_read(char addr1, char addr2) {
	driver_error_t *error;
	int transaction = I2C_TRANSACTION_INITIALIZER;
	char data;

	error = i2c_start(0, &transaction);
	TEST_ASSERT(transaction != I2C_TRANSACTION_INITIALIZER);
	TEST_ASSERT_MESSAGE(error == NULL, driver_get_err_msg(error));

	error = i2c_write_address(0, &transaction, 0x51, 0);
	TEST_ASSERT_MESSAGE(error == NULL, driver_get_err_msg(error));

	error = i2c_write(0, &transaction, &addr1, sizeof(addr1));
	TEST_ASSERT_MESSAGE(error == NULL, driver_get_err_msg(error));

	error = i2c_write(0, &transaction, &addr2,sizeof(addr2));
	TEST_ASSERT_MESSAGE(error == NULL, driver_get_err_msg(error));

	error = i2c_start(0, &transaction);
	TEST_ASSERT(transaction != I2C_TRANSACTION_INITIALIZER);
	TEST_ASSERT_MESSAGE(error == NULL, driver_get_err_msg(error));

	error = i2c_write_address(0, &transaction, 0x51, 1);
	TEST_ASSERT_MESSAGE(error == NULL, driver_get_err_msg(error));

	error = i2c_read(0, &transaction, &data, sizeof(data));
	TEST_ASSERT_MESSAGE(error == NULL, driver_get_err_msg(error));

	error = i2c_stop(0, &transaction);
	TEST_ASSERT_MESSAGE(error == NULL, driver_get_err_msg(error));

	return data;
}

TEST_CASE("i2c-master", "[i2c master]") {
	driver_error_t *error;
	int transaction = I2C_TRANSACTION_INITIALIZER;
	char i, data;

	// operations without setup
	error = i2c_start(0, &transaction);
	TEST_ASSERT(transaction == I2C_TRANSACTION_INITIALIZER);
	TEST_ASSERT(error != NULL);

	error = i2c_stop(0, &transaction);
	TEST_ASSERT(transaction == I2C_TRANSACTION_INITIALIZER);
	TEST_ASSERT(error != NULL);

	error = i2c_write_address(0, &transaction, 0x51, 1);
	TEST_ASSERT(transaction == I2C_TRANSACTION_INITIALIZER);
	TEST_ASSERT(error != NULL);

	error = i2c_write(0, &transaction, &data, sizeof(data));
	TEST_ASSERT(transaction == I2C_TRANSACTION_INITIALIZER);
	TEST_ASSERT(error != NULL);

	error = i2c_read(0, &transaction, &data, sizeof(data));
	TEST_ASSERT(transaction == I2C_TRANSACTION_INITIALIZER);
	TEST_ASSERT(error != NULL);

	// Normal operation

	// Setup test
	error = i2c_setup(0, I2C_MASTER, 1, 4, 5, 0, 0);
	TEST_ASSERT(error == NULL);

	// Setup again
	error = i2c_setup(0, I2C_MASTER, 1, 16, 4, 0, 0);
	TEST_ASSERT(error == NULL);

	// Write some data
	for(i=0;i<100;i++) {
		eeprom_24c256d_write(0x00,i,i*2);

		// This is only for test purposes
		// It should be done by ACKNOWLEDGE POLLING
		delay(30);
	}

	// Read write data and verify
	for(i=0;i<100;i++) {
		data = eeprom_24c256d_read(0x00,i);
		TEST_ASSERT_MESSAGE(data == i*2, "invalid read data");
	}
}
#endif
