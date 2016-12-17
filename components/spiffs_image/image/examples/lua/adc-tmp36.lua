-- Read adc value from a TMP36 temperature sensor and shows current temperature
-- value on the screen.

tmp36_adc = adc.ADC1     -- Sensor is connected to ADC1 module
tmp36_chan = adc.ADC_CH6 -- Sensor is connected to ADC1 chan 6
tmp36_chan_res = 12      -- Channel resolution

-- Setup ADC module
adc1 = adc.setup(tmp36_adc)

-- Setup ADC channel
tmp36 = adc1:setupchan(tmp36_chan_res, tmp36_chan)

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
