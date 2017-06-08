-- ----------------------------------------------------------------
-- WHITECAT ECOSYSTEM
--
-- Lua RTOS examples
-- ----------------------------------------------------------------
-- Read ADC value from a TMP36 temperature sensor and shows 
-- current temperature value on the screen.
--
-- ADC is the internal ADC.
-- ----------------------------------------------------------------

tmp36_adc = adc.ADC1     -- Sensor is connected to ADC1 module
tmp36_chan = adc.ADC_CH4 -- Sensor is connected to ADC1 chan 4 (GPIO32)
tmp36_chan_res = 12      -- Channel resolution

-- Setup ADC module
tmp36 = adc.attach(tmp36_adc, tmp36_chan, tmp36_chan_res)

-- Sample
while true do
    -- Read from adc
    val, mval = tmp36:read()

    -- Computes temperature
    temp = (mval - 500) / 10

    -- Show temperature
    print("raw: "..val.."\t mvolts: "..mval.."\t temp: "..string.format("%.2f", temp))

    -- Wait
    tmr.delay(1)
end