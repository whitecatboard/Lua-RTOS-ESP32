-- ----------------------------------------------------------------
-- WHITECAT ECOSYSTEM
--
-- Lua RTOS examples
-- ----------------------------------------------------------------
-- Send a frame via LoRa WAN using ABP
-- ----------------------------------------------------------------
os.loglevel(os.LOG_ALL)

-- Register received callbacks
lora.whenReceived(function(port, payload)
  print("port "..port)
  print("payload "..payload)
end)

-- Setup LoRa WAN
lora.attach(lora.BAND868)
lora.setDevAddr("26011728")
lora.setNwksKey("5DD8013B282A725F51B1227F311EEFB1")
lora.setAppsKey("9F09A4C74BAEDCB06669A4AC1D7ACB68")
lora.setDr(5)
lora.setReTx(0)

-- Send frame
lora.tx(false,1,pack.pack(25.3))