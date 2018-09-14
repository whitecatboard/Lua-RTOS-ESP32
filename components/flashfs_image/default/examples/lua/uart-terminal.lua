-- ----------------------------------------------------------------
-- WHITECAT ECOSYSTEM
--
-- Lua RTOS examples
-- ----------------------------------------------------------------
-- Terminal
--
-- This example opens an uart port. The rx stream is write to the
-- console, and the tx strim from the console is write to the port.
--
-- A good example is to connect an UART GPS to the uart port and
-- show the NMEA sentences.
-- ----------------------------------------------------------------


function terminal(port, speed, bits, parity, stop)
	local line = ""
	local console = uart.CONSOLE

	uart.attach(port, speed, bits, parity, stop)

	while true do
	    -- Read from console
	    c = uart.read(console, "*c", 0)
	    if (c) then
	        -- Echo to console
	        uart.write(console, c)
	        if (string.char(c) == '\r') then
	            uart.write(console, '\n')
	        end

	        -- Write to serial port
	        uart.write(uartid, c)

	        if (string.char(c) == '\r') then
	            uart.write(uartid, '\n')
	        end
	    end

	    -- Read from serial port
	    c = uart.read(port, "*c", 0)
	    if (c) then
	        uart.write(console, c)
	    end
	end
end

terminal(uart.UART1, 9600, 8, uart.PARNONE, uart.STOP1)
