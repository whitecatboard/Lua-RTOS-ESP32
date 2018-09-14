-- ----------------------------------------------------------------
-- WHITECAT ECOSYSTEM
--
-- Lua RTOS examples
-- ----------------------------------------------------------------
-- This example demonstrates the use of the Lua Wifi module.
-- ----------------------------------------------------------------

-- Wifi scan
net.wf.scan()

-- Configure wifi in Station Mode with SSID from Citilab
net.wf.setup(net.wf.mode.STA, "CITILAB","wifi@citilab")

-- Start wifi
net.wf.start()

-- Get current time from sntp server
net.service.sntp.start()
net.service.sntp.stop()

-- Perform a DNS look up
print("whitecarboard.org: "..net.lookup("whitecatboard.org"))

-- Ping to google
net.ping("8.8.8.8")