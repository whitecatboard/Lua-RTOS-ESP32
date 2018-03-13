-- ----------------------------------------------------------------
-- WHITECAT ECOSYSTEM
--
-- Lua RTOS examples
-- ----------------------------------------------------------------
-- Start advertising for 2 eddystone beacons
-- ----------------------------------------------------------------

b1 = bt.service.eddystone.uid("808182838485", 0xed, "3F3B51B60B88EF9949E5", "CF484185AE0B")
b2 = bt.service.eddystone.url("808182838485", 0xed, "https://whitecatboard.org")

b1:start()
b2:start()