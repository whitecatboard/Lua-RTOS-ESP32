-- ----------------------------------------------------------------
-- WHITECAT ECOSYSTEM
--
-- Lua RTOS examples
-- ----------------------------------------------------------------
-- This example demonstrates the use of the Lua stepper module.
-- ----------------------------------------------------------------

dir = 1

-- Attach 2 steppers
--
--   stepper1: dir pin at GPIO26, step pin at GPIO14
--   stepper2: dir pin at GPIO12, step pin at GPIO13
--
-- Steppers are configured by default:
--
--   steps per unit (revolutions): 200
--   min speed: 60 rpm
--   max speed: 800 rpm
--   acceleration: 2 revolutions / secs^2
s1 = stepper.attach(pio.GPIO26, pio.GPIO14)
s2 = stepper.attach(pio.GPIO12, pio.GPIO13)

-- Move the steppers (10 revolutions), in bucle, changing the
-- direction
while true do
	s1:move(dir * 10)
	s2:move(dir * 10)

	stepper.start(s1,s2)

	if (dir == 1) then
		dir = -1
	else
		dir = 1
	end
end