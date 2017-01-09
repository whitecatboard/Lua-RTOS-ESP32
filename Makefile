#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := lua_rtos

all_binaries: configure-idf-lua-rtos

clean: restore-idf

configure-idf-lua-rtos:
	@echo "Configure esp-idf for Lua RTOS ..."
ifeq ("$(wildcard $(IDF_PATH)/components/lua_rtos)","")
	@ln -s $(PROJECT_PATH)/main/test/lua_rtos $(IDF_PATH)/components/lua_rtos
endif
	@touch $(PROJECT_PATH)/components/lua_rtos/sys/sys_init.c
	@touch $(PROJECT_PATH)/components/lua_rtos/lwip/socket.c
	@cd $(IDF_PATH)/components/esp32/ld && git checkout esp32.common.ld
	@cd $(IDF_PATH)/components/lwip/api && git checkout api_msg.c
	@patch -f $(IDF_PATH)/components/esp32/ld/esp32.common.ld $(PROJECT_PATH)/main/patches/ld.patch
	@patch -f $(IDF_PATH)/components/lwip/api/api_msg.c $(PROJECT_PATH)/main/patches/api_msg.patch

restore-idf:
	@echo "Restoring esp-idf ..."
ifeq ("$(wildcard $(IDF_PATH)/components/lua_rtos)","$(IDF_PATH)/components/lua_rtos")
	@rm $(IDF_PATH)/components/lua_rtos
endif
	@cd $(IDF_PATH)/components/esp32/ld && git checkout esp32.common.ld
	@cd $(IDF_PATH)/components/lwip/api && git checkout api_msg.c
	
include $(IDF_PATH)/make/project.mk