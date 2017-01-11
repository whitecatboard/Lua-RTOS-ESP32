-- Lua RTOS sample code
--
-- This sample demonstrates how to communicate with i2c devices.
-- The sample writes values into an EEPROM (4c256d) and then
-- read from the EEPROM and test that readed values are the writed
-- values. 

-- Setup i2c
eeprom = i2c.setup(i2c.I2C0, i2c.MASTER, 1, pio.GPIO16, pio.GPIO4)

-- Write
for i=0,100 do
	eeprom:start()
	eeprom:address(0x51, false)
	eeprom:write(0x00)
	eeprom:write(i)
	eeprom:write(i)
	eeprom:stop()

	-- This is only for test purposes
	-- It should be done by ACKNOWLEDGE POLLING
	tmr.delayms(20)
end

-- Read and test
for i=0,100 do
	eeprom:start()
	eeprom:address(0x51, false)
	eeprom:write(0x00)
	eeprom:write(i)
	eeprom:start()
	eeprom:address(0x51, true)
	if (eeprom:read() ~= i) then
		print("Error for "..i)
	end
	eeprom:stop()
end

