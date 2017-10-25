-- system.lua
--
-- This script is executed after a system boot or a system reset and is intended
-- for setup the system.

---------------------------------------------------
-- Main setups
---------------------------------------------------
os.loglevel(os.LOG_INFO)   -- Log level to info
os.logcons(true)           -- Enable/disable sys log messages to console
os.shell(true)             -- Enable/disable shell
os.history(true)           -- Enable/disable history

-- Network setup
do
	local useWifi = true   -- Use wifi?
	
	local ssid = "CITILAB"          -- Wifi SSID
	local passwd = "wifi@citilab"   -- Wifi password
	
	if (useWifi) then
		print("Starting wifi ...")
		net.wf.setup(net.wf.mode.STA, ssid, passwd)
		net.wf.start()
	end

	print("Updating time ...")
	net.service.sntp.start()
end
