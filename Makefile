#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := lua_rtos

all: restore-idf-ld update-idf-ld

flash: restore-idf-ld update-idf-ld

clean: restore-idf-ld

#
# Restore esp-idf esp32.common.ld from git
#
restore-idf-ld:
	@echo "Restoring esp-idf esp32.common.ld ..."
	@cd $(IDF_PATH)/components/esp32/ld && git checkout esp32.common.ld

#
# Update esp-idf esp32.common.ld
#
update-idf-ld:
	@echo "Updating esp-idf esp32.common.ld for Lua RTOS ..."
	@cat ld/lua-rtos.ld > $(IDF_PATH)/components/esp32/ld/esp32.common.ld
	
include $(IDF_PATH)/make/project.mk
		

