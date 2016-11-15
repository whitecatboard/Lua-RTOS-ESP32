deps_config := \
	/Users/jaumeolivepetrus/esp-idf/components/log/Kconfig \
	/Users/jaumeolivepetrus/esp-idf/components/spi_flash/Kconfig \
	/Users/jaumeolivepetrus/esp-idf/components/esptool_py/Kconfig.projbuild \
	/Users/jaumeolivepetrus/esp-idf/components/bootloader/Kconfig.projbuild \
	/Users/jaumeolivepetrus/esp-idf/Kconfig

include/config/auto.conf: \
	$(deps_config)


$(deps_config): ;
