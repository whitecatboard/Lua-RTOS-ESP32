-- system.lua
--
-- This script is executed after a system boot or a system reset and is intended
-- for setup the system.

---------------------------------------------------
-- Main setups
---------------------------------------------------
os.loglevel(os.LOG_ERR)    -- Log only errors
os.logcons(true)           -- Enable/disable syslog messages to console
os.history(false)          -- Enable/disable history
