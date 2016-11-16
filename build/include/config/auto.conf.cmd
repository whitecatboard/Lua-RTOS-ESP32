deps_config := \
	/Users/jaumeolivepetrus/esp-idf/components/bt/Kconfig \
	/Users/jaumeolivepetrus/esp-idf/components/esp32/Kconfig \
	/Users/jaumeolivepetrus/esp-idf/components/freertos/Kconfig \
	/Users/jaumeolivepetrus/esp-idf/components/log/Kconfig \
	/Users/jaumeolivepetrus/Lua-RTOS-ESP32/components/lua_rtos/Kconfig \
	/Users/jaumeolivepetrus/esp-idf/components/lwip/Kconfig \
	/Users/jaumeolivepetrus/esp-idf/components/mbedtls/Kconfig \
	/Users/jaumeolivepetrus/esp-idf/components/spi_flash/Kconfig \
	/Users/jaumeolivepetrus/esp-idf/components/bootloader/Kconfig.projbuild \
	/Users/jaumeolivepetrus/esp-idf/components/esptool_py/Kconfig.projbuild \
	/Users/jaumeolivepetrus/esp-idf/components/partition_table/Kconfig.projbuild \
	/Users/jaumeolivepetrus/esp-idf/Kconfig

include/config/auto.conf: \
	$(deps_config)


$(deps_config): ;
