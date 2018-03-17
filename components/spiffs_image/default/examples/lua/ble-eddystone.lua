-- ----------------------------------------------------------------
-- WHITECAT ECOSYSTEM
--
-- Lua RTOS examples
-- ----------------------------------------------------------------
-- This sample run 2 eddystone beacons
--
-- First beacon is an uid beacon
-- Second beacon is an url beacon
-- ----------------------------------------------------------------

b1 = bt.service.eddystone.uid("808182838485", -42, "3F3B51B60B88EF9949E5", "CF484185AE0B")
b2 = bt.service.eddystone.url("808182838485", -42, "https://whitecatboard.org")

b1:start()
b2:start()