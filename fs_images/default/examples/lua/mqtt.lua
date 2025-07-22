-- ----------------------------------------------------------------
-- WHITECAT ECOSYSTEM
--
-- Lua RTOS examples
-- ----------------------------------------------------------------
-- This example demonstrates the use of the Lua MQTT module.
-- ----------------------------------------------------------------

-- Configure wifi in Station Mode with SSID from Citilab
net.wf.setup(net.wf.mode.STA, "CITILAB","wifi@citilab")

-- Start wifi
net.wf.start()

-- Connect to MQTT server from cssiberica.com
client = mqtt.client("100", "cssiberica.com", 1883, false)
client:connect("","")

-- Publis to queue
client:publish("/100","hola",mqtt.QOS0) 