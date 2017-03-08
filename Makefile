#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := lua_rtos

all_binaries: configure-idf-lua-rtos configure-idf-lua-rtos-tests

clean: restore-idf

configure-idf-lua-rtos-tests:
	@echo "Configure esp-idf for Lua RTOS tests ..."
	@touch $(PROJECT_PATH)/components/lua_rtos/sys/sys_init.c
	@touch $(PROJECT_PATH)/components/lua_rtos/Lua/src/lbaselib.c
ifeq ("$(wildcard $(IDF_PATH)/components/lua_rtos)","")
	@ln -s $(PROJECT_PATH)/main/test/lua_rtos $(IDF_PATH)/components/lua_rtos
endif

configure-idf-lua-rtos:
	@echo "Configure esp-idf ..."
	@cd $(IDF_PATH)/components/lwip/api && git checkout api_msg.c
	@cd $(IDF_PATH)/components/vfs/include/sys && git checkout dirent.h
	@cd $(IDF_PATH)/components/spi_flash && git checkout flash_ops.c
	@cd $(IDF_PATH)/components/spi_flash/include && git checkout esp_spi_flash.h
	#@cd $(IDF_PATH)/components/freertos/include/freertos && git checkout FreeRTOSConfig.h
	@echo "Configure esp-idf for Lua RTOS ..."
	@touch $(PROJECT_PATH)/components/lua_rtos/lwip/socket.c
	@cd $(IDF_PATH)/components/lwip/api && git checkout api_msg.c
	@patch -f $(IDF_PATH)/components/lwip/api/api_msg.c $(PROJECT_PATH)/main/patches/api_msg.patch
	@patch -f $(IDF_PATH)/components/vfs/include/sys/dirent.h $(PROJECT_PATH)/main/patches/dirent.patch
	@patch -f $(IDF_PATH)/components/spi_flash/flash_ops.c $(PROJECT_PATH)/main/patches/spi_flash.1.patch
	@patch -f $(IDF_PATH)/components/spi_flash/include/esp_spi_flash.h $(PROJECT_PATH)/main/patches/spi_flash.2.patch
	#@patch -f $(IDF_PATH)/components/freertos/include/freertos/FreeRTOSConfig.h $(PROJECT_PATH)/main/patches/free_rtos_config.patch
	
restore-idf:
	@echo "Restoring esp-idf ..."
	@cd $(IDF_PATH)/components/lwip/api && git checkout api_msg.c
	@cd $(IDF_PATH)/components/vfs/include/sys && git checkout dirent.h
	
include $(IDF_PATH)/make/project.mk
