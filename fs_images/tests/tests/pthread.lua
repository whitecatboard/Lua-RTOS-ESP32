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