-- ----------------------------------------------------------------
-- WHITECAT ECOSYSTEM
--
-- Lua RTOS examples
-- ----------------------------------------------------------------
-- NEOPIXEL example
-- ----------------------------------------------------------------

thread.start(function()
	neo = neopixel.setup(neopixel.WS2812B, pio.GPIO14, 6)

	pixel = 0
	direction = 0

	while true do
	  neo:setPixel(pixel, 0, 255, 0)
	  neo:update()
	  tmr.delayms(100)
	  neo:setPixel(pixel, 0, 00, 0)

	  if (direction == 0) then
		  if (pixel == 5) then
			  direction = 1
			  pixel = 4
		  else
			  pixel = pixel + 1
		  end
	  else
		  if (pixel == 0) then
			  direction = 0
			  pixel = 1
		  else
			  pixel = pixel - 1
		  end
	  end
	end
end)