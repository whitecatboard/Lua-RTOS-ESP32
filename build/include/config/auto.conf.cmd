deps_config := \
	/Users/jolive/esp-idf/components/bt/Kconfig \
	/Users/jolive/esp-idf/components/esp32/Kconfig \
	/Users/jolive/esp-idf/components/freertos/Kconfig \
	/Users/jolive/esp-idf/components/log/Kconfig \
	/Users/jolive/Lua-RTOS-ESP32/components/lua_rtos/Kconfig \
	/Users/jolive/esp-idf/components/lwip/Kconfig \
	/Users/jolive/esp-idf/components/mbedtls/Kconfig \
	/Users/jolive/esp-idf/components/spi_flash/Kconfig \
	/Users/jolive/esp-idf/components/bootloader/Kconfig.projbuild \
	/Users/jolive/esp-idf/components/esptool_py/Kconfig.projbuild \
	/Users/jolive/esp-idf/components/partition_table/Kconfig.projbuild \
	/Users/jolive/esp-idf/Kconfig

include/config/auto.conf: \
	$(deps_config)


$(deps_config): ;
