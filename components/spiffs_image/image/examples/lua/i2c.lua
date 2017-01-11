-- Lua RTOS sample code
--
-- This sample demonstrates how to communicate with i2c devices.
-- The sample writes values into an EEPROM (4c256d) and then
-- read from the EEPROM and test that readed values are the writed
-- values. 

-- Setup i2c
i2c.setup(i2c.I2C0, i2c.MASTER, 1, pio.GPIO16, pio.GPIO4)

-- Write
for i=0,100 do
	i2c.start(i2c.I2C0)
	i2c.address(i2c.I2C0, 0x51, false)
	i2c.write(i2c.I2C0, 0x00)
	i2c.write(i2c.I2C0, i)
	i2c.write(i2c.I2C0, i)
	i2c.stop(i2c.I2C0)

	-- This is only for test purposes
	-- It should be done by ACKNOWLEDGE POLLING
	tmr.delayms(20)
end

-- Read and test
for i=0,100 do
	i2c.start(i2c.I2C0)
	i2c.address(i2c.I2C0, 0x51, false)
	i2c.write(i2c.I2C0, 0x00)
	i2c.write(i2c.I2C0, i)
	i2c.start(i2c.I2C0)
	i2c.address(i2c.I2C0, 0x51, true)
	if (i2c.read(i2c.I2C0) ~= i) then
		print("Error for "..i)
	end
	i2c.stop(i2c.I2C0)
end

