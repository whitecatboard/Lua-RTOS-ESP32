-- the lua script gets back the ssid and password
-- which is sent from the esp-touch device (e.g. a phone)

-- in this example the ssid and password are stored to nvs
-- where they are read from at startup
net.wf.startsc(function (ssid, password)
	print("SSID: "..ssid)
	print("PASS: "..password)

	nvs.write("wifi","name",ssid)
	nvs.write("wifi","pass",password)
end)

