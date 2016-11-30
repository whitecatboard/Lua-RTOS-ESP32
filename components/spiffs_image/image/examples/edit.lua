-- WHITECAT sample code
--
-- UART terminal
function terminal()
pio.pin.setdir(pio.OUTPUT,pio.PE_2)                                         
pio.pin.setlow(pio.PE_2)
tmr.delayms(50)
pio.pin.sethigh(pio.PE_2)

line = ""
console = 1
uartid = 2

-- Setup UART, 57600 bps, 8N1
uart.setup(uartid, 57600, 8, uart.PARNONE, uart.STOP1)

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
    c = uart.read(uartid, "*c", 0)
    if (c) then
        uart.write(console, c)
    end
end
end
terminal()
