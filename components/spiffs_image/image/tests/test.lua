_soft = true

tests = {}

-- table.insert(tests, function() dofile('main.lua') end)
table.insert(tests, function() dofile("gc.lua") end)
-- table.insert(tests, function() dofile('db.lua') end)
table.insert(tests, function() assert(dofile('calls.lua') == deep and deep) end)
table.insert(tests, function() dofile('strings.lua') end)
table.insert(tests, function() dofile('literals.lua') end)
table.insert(tests, function() dofile('tpack.lua') end)
--table.insert(tests, function() assert(dofile('attrib.lua') == 27) end)
table.insert(tests, function() assert(dofile('locals.lua') == 5) end)
table.insert(tests, function() dofile('constructs.lua') end)
--table.insert(tests, function() dofile('code.lua', true) end)
--table.insert(tests, function() dofile('nextvar.lua') end)
table.insert(tests, function() dofile('pm.lua') end)
table.insert(tests, function() dofile('utf8.lua') end)
--table.insert(tests, function() dofile('api.lua') end)
table.insert(tests, function() assert(dofile('events.lua') == 12) end)
table.insert(tests, function() dofile('vararg.lua') end)
table.insert(tests, function() dofile('closure.lua') end)
table.insert(tests, function() dofile('coroutine.lua') end)
table.insert(tests, function() dofile('goto.lua', true) end)
--table.insert(tests, function() dofile('errors.lua') end)
table.insert(tests, function() dofile('math.lua') end)
table.insert(tests, function() dofile('sort.lua', true) end)
table.insert(tests, function() dofile('bitwise.lua') end)
--table.insert(tests, function() assert(dofile('verybig.lua', true) == 10) end)
--table.insert(tests, function() dofile('files.lua') end)
table.insert(tests, function() dofile('bitwise.lua') end)

if os.bootcount() == 1 then
	os.remove("/tests/test.log")
end

if os.bootcount() < #tests then
	os.stdout("/tests/test.log")
	tests[os.bootcount()]()
	os.stdout()
	os.sleep(0)
end
