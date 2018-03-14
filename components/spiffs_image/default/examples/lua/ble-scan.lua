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
    print("EddystoneUID. RSSI: "..(data.rssi)..". NAMESPACE: "..data.namespace..". INSTANCE: "..data.instance..". DISTANCE: "..data.distance..".")
  elseif (data.type == bt.frameType.EddystoneURL) then
    print("EddystoneURL. RSSI "..(data.rssi)..". URL: "..data.url..". DISTANCE: "..data.distance..".")
  end  
end)