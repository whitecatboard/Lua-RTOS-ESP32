-- ----------------------------------------------------------------
-- WHITECAT ECOSYSTEM
--
-- Lua RTOS examples
-- ----------------------------------------------------------------
-- This sample demonstrates how to communicate with i2c devices.
-- The sample writes values into an EEPROM (4c256d) and then
-- read from the EEPROM and test that readed values are the writed
-- values. 
-- ----------------------------------------------------------------

-- Attach the eeprom to the i2c bus
function eeprom_attach()
	eeprom = i2c.attach(i2c.I2C0, i2c.MASTER, 400000)
end

-- Once the internally-timed write cycle has started and the EEPROM
-- inputs are disabled, acknowledge polling can be initiated.
-- This involves sending a start condition followed by the device address word.
-- If this fails (timeout, or ack not received) we can't write.
function eeprom_poll(devAddress) 
	local done = false
	
	while not done do
		try(
		  function() 
	  		eeprom:start()
			eeprom:address(devAddress, false)
	  		eeprom:stop()
			done = true
		  end
		)
	end
end

-- Write to eeprom (byte)
function eeprom_write(devAddress, address, value)
	eeprom:start()
	eeprom:address(devAddress, false)
	eeprom:write(0x00, address, value)
	eeprom:stop()

	eeprom_poll(devAddress)
end

-- Read from eeprom (byte)
function eeprom_read(devAddress, address)
	local read

	eeprom:start()
	eeprom:address(devAddress, false)
	eeprom:write(0x00, address)
	eeprom:start()
	eeprom:address(devAddress, true)

	read = eeprom:read()
	
	eeprom:stop()

	return read
end

-- Attach eeprom
eeprom_attach()

-- Write test bytes
for i=0,100 do
	eeprom_write(0x51, i, i)
end

-- Check bytes
for i=0,100 do
	if (eeprom_read(0x51, i) ~= i) then
		error("Readed "..read..", expected "..i)
	end
end