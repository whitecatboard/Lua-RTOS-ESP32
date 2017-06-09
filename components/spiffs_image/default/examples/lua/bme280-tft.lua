-- ----------------------------------------------------------------
-- WHITECAT ECOSYSTEM
--
-- Lua RTOS examples
-- ----------------------------------------------------------------
-- This sample shows the temperature, humidity and pressure values
-- readed from a BME280 sensor, on a TFT display
-- ----------------------------------------------------------------

thread.start(function()
	local line
	local line_spacing

	-- Init TFT
	tft.init(tft.ST7735G, tft.LANDSCAPE_FLIP)
	tft.setfont(tft.UBUNTU16_FONT)
	tft.setcolor(tft.WHITE)
	tft.clear()
	
	line_spacing = tft.getfontheight() + 2
	
	-- Instantiate the sensor
	s1 = sensor.attach("BME280", i2c.I2C0, 400, 0x76)

	while true do
	  -- Read temperature
	  temperature = s1:read("temperature")

	  -- Read humidity
	  humidity = s1:read("humidity")

	  -- Read preassure
	  pressure = s1:read("pressure")

	  -- Print results
	  line = 1
	  tft.write(2,line * line_spacing,"T: "..string.format("%4.2f",temperature))
	  
	  line = line + 1
	  tft.write(2,line * line_spacing,"H: "..string.format("%4.2f",humidity))
	  
	  line = line + 1
	  tft.write(2,line * line_spacing,"P: "..string.format("%4.2f",pressure))
  
	  tmr.delayms(500)
	end
end)