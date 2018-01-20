-- config.lua
--
-- Lua RTOS configuration file

-- config.shell = false    -- Uncomment to disable shell / comment to enable shell
-- config.history = false  -- Uncomment to disable history / comment to enable history

-- Uncomment one of this to set the loglevel
-- config.loglevel = os.LOG_INFO
-- config.loglevel = os.LOG_EMERG
-- config.loglevel = os.LOG_ALERT
-- config.loglevel = os.LOG_CRIT
-- config.loglevel = os.LOG_ERR
-- config.loglevel = os.LOG_WARNING
-- config.loglevel = os.LOG_DEBUG
-- config.loglevel = os.LOG_ALL

-- config.logcons = false  -- Uncomment to disable syslog messages on the console

-- config.wifi = true      -- Uncomment to enable wifi / comment to disable it
-- config.ethernet = true  -- Uncomment to enable ethernet / comment to disable it

-- config.sntp = true -- Uncomment to enable sntp client / comment to disable it
-- config.http = true -- Uncomment to enable http server / comment to disable it
-- config.openvpn = true -- Uncomment to enable OpenVpn client / comment to disable it
-- config.ssh = true -- Uncomment to enable the ssh server / comment to disable it

-- config.can.gw = true -- Uncomment to enable can gateway

-- Wifi configuration in station mode
config.data.wifi = {
	-- Put the ssid / password needed to connect to the network
	ssid = "",
	pass = "",

	-- Uncomment ip, mask, gw to enable static ip
	-- ip = net.packip(192, 168, 1, 100),
	-- mask = net.packip(255, 255, 255, 0),
	-- gw = net.packip(192, 168, 1, 1),
	
	-- Uncomment to set dns servers (only for static ip)
	-- dns1 = net.packip(8, 8, 8, 8),
	-- dns2 = net.packip(8, 8, 4, 4),
}

-- Ethernet configuration
config.data.ethernet = {
	-- Uncomment ip, mask, gw to enable static ip
	-- ip = net.packip(192, 168, 1, 100),
	-- mask = net.packip(255, 255, 255, 0),
	-- gw = net.packip(192, 168, 1, 1),
	
	-- Uncomment to set dns servers (only for static ip)
	-- dns1 = net.packip(8, 8, 8, 8),
	-- dns2 = net.packip(8, 8, 4, 4),
}

-- HTTP server configuration
config.data.http = {
	-- Put the http port for unsecured connections. Use 0 to disable unsecured
	-- http connectiond
	httpPort = 80,

	-- Put the http port for secured connections. Use 0 to disable secured
	-- http connectiond
	--
	-- If secured connections are enabled fill caFile / keyFile
	httpsPort = 0, 
	caFile = "",
	keyFile = "",
}

-- CAN gateway configuration
config.data.can.gw = {
	port = 1333,
	speed = 1000
}