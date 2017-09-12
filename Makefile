#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := lua_rtos
LUA_RTOS_PATCHES := $(abspath $(wildcard main/patches/*.patch))

all_binaries: configure-idf-lua-rtos configure-idf-lua-rtos-tests

clean: restore-idf

configure-idf-lua-rtos-tests:
	@echo "Configure esp-idf for Lua RTOS tests ..."
	@touch $(PROJECT_PATH)/components/lua_rtos/sys/sys_init.c
	@touch $(PROJECT_PATH)/components/lua_rtos/Lua/src/lbaselib.c
ifeq ("$(wildcard $(IDF_PATH)/components/lua_rtos)","")
	@ln -s $(PROJECT_PATH)/main/test/lua_rtos $(IDF_PATH)/components/lua_rtos
endif

configure-idf-lua-rtos: $(LUA_RTOS_PATCHES)
ifeq ("$(wildcard $(IDF_PATH)/lua_rtos_patches)","")
	@echo "Reverting previous Lua RTOS esp-idf patches ..."
	@cd $(IDF_PATH) && git checkout .
	@echo "Applying Lua RTOS esp-idf patches ..."
	@cd $(IDF_PATH) && git apply --whitespace=warn $^
	@touch $(IDF_PATH)/lua_rtos_patches
endif

restore-idf:
	@echo "Revert previous Lua RTOS esp-idf patches ..."
	@cd $(IDF_PATH) && git checkout .
ifneq ("$(wildcard $(IDF_PATH)/lua_rtos_patches)","")
	@rm $(IDF_PATH)/lua_rtos_patches
endif
	
flash-args:
	@echo $(subst --port $(ESPPORT),, \
			$(subst python /components/esptool_py/esptool/esptool.py,, \
				$(subst $(IDF_PATH),, $(ESPTOOLPY_WRITE_FLASH))\
			)\
	 	  ) \
	 $(subst /build/, , $(subst /build/bootloader/,, $(subst $(PROJECT_PATH), , $(ESPTOOL_ALL_FLASH_ARGS))))
	
include $(IDF_PATH)/make/project.mk
