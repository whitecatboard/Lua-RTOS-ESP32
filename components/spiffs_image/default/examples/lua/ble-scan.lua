-- ----------------------------------------------------------------
-- WHITECAT ECOSYSTEM
--
-- Lua RTOS examples
-- ----------------------------------------------------------------
-- This sample performs a ble advertise scan an shows the result
-- on the console
-- ----------------------------------------------------------------

bt.attach(bt.mode.BLE)
bt.scan.start(function(data)
  if (data.type == bt.frameType.EddystoneUID) then
    print("EddystoneUID. RSSI: "..(data.rssi)..". NAMESPACE: "..data.namespace..". INSTANCE: "..data.instance..".")
  elseif (data.type == bt.frameType.EddystoneURL) then
    print("EddystoneURL. RSSI "..(data.rssi)..". URL: "..data.url..".")
  else
    print("RAW. RSSI "..(data.rssi)..". "..data.raw..".")
  end  
end)