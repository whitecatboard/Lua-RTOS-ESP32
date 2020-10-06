-- ----------------------------------------------------------------
-- WHITECAT ECOSYSTEM
--
-- Lua RTOS examples
-- ----------------------------------------------------------------
-- This sample computes the TX power at 0 m needed to configure an
-- eddystone beacon.
-- 
-- To measure TX power put the emitter and received at 1 m.
--
-- At 1 m rssi = tx power
-- ----------------------------------------------------------------

rssi = 0
measures = 0

bt.attach(bt.mode.BLE)
bt.scan.start(function(data)
  if ((data.type == bt.frameType.EddystoneUID) or (data.type == bt.frameType.EddystoneURL)) then
	  measures = measures + 1
	  rssi = ((measures - 1) * rssi + data.rssi) / measures;
	  print("TX POW: "..(rssi+41))
  end  
end)
