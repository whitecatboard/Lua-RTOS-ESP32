-- ----------------------------------------------------------------
-- WHITECAT ECOSYSTEM
--
-- Lua RTOS examples
-- ----------------------------------------------------------------
-- This example demonstrates the use of the Lua PWM module
-- for control led's brightness
-- ----------------------------------------------------------------

-- Led brightness is controled by this PWM channel
led_pwm_chan = pwm.PWM_CH0

-- Led is connected to this GPIO
led_pwm_gpio = pio.GPIO27

-- Setup pwm module0
pwm0 = pwm.setup(pwm.PWM0)

-- Setup pwm at 10 Khz, initial duty is 0
led = pwm0:setupchan(led_pwm_chan, led_pwm_gpio, 10000, 0)

-- Start led
led:start()

while true do
  -- Set duty from 0% to 100% at 10ms interval
  for duty = 0, 100 do
    led:setduty(duty / 100)
    tmr.delayms(10);
  end
                            
  -- Set duty from 100% to 0% at 10ms interval
  for duty = 0, 100 do
    led:setduty((100 - duty) / 100)
    tmr.delayms(10);
  end
end
