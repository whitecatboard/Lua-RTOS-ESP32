-- ----------------------------------------------------------------
-- WHITECAT ECOSYSTEM
--
-- Lua RTOS examples
-- ----------------------------------------------------------------
-- Read blink a led in a thread. While led is blinking Lua
-- interpreter are available, you cant start new threads for
-- doing other things.
-- ----------------------------------------------------------------

thread.start(function()
  pio.pin.setdir(pio.OUTPUT, pio.GPIO26)
  pio.pin.setpull(pio.NOPULL, pio.GPIO26)
  while true do
    pio.pin.setval(1, pio.GPIO26)
    tmr.delayms(100)
    pio.pin.setval(0, pio.GPIO26)
    tmr.delayms(100)
  end
end)


function test(tag, f, expected)
	status, err = pcall(f)
	if (not (status == expected)) then
		print(tag..": FAILED")
	else
		print(tag..": PASS")
	end
end

-- Invalid thread id
test("THREAD1", function() thread.suspend(0) end, false)
test("THREAD2", function() thread.suspend(-2) end, false)

-- Suspend all threads
test("THREAD3", function() thread.suspend() end, true)

-- Suspend main thread
test("THREAD4", function() thread.suspend(1) end, false)

-- Suspend an inexisting thread
test("THREAD5", function() thread.suspend(2) end, false)

-- Suspend a task
test("THREAD6", function() thread.suspend(0) end, false)

-- Suspend an existing thread
t = thread.start(function()
	while true do
		tmr.delayms(4)
	end
end)

test("THREAD7", function() thread.suspend(t) end, true)
test("THREAD8", function() thread.suspend(t) end, true)
