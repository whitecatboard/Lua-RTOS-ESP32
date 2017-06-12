-- ----------------------------------------------------------------
-- WHITECAT ECOSYSTEM
--
-- Lua RTOS examples
-- ----------------------------------------------------------------
-- This example demonstrates the use of the Lua PWM module
-- for control led's brightness
-- ----------------------------------------------------------------

thread.start(function()
	-- Led is connected to this GPIO
	led_pwm_gpio = pio.GPIO26

	-- Setup pwm at 10 Khz, initial duty is 0
	led = pwm.attach(led_pwm_gpio, 10000, 0)

	-- Start led
	led:start()

	initial_duty = 0
	final_duty = 100
	step = 1

	while true do
	  for duty = initial_duty, final_duty, step do
	    led:setduty(duty / 100)
	    tmr.delayms(10);
	  end
  
	  initial_duty = final_duty
	  if (initial_duty == 0) then
		  final_duty = 100
		  step = 1
	  else
		  final_duty = 0
		  step = -1
	  end
	end
end
)
