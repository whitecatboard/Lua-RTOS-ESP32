#
# Lua RTOS build system
#
# This Makefile contains rules related to ESP32 partitions management
#

.PHONY: gen-part erase-ota-data
NOTPARALLEL: check-space

CONFIG_LUA_RTOS_USE_FAT ?=
CONFIG_LUA_RTOS_USE_SPIFFS ?=
CONFIG_LUA_RTOS_USE_LFS ?=
CONFIG_ESP32_PHY_INIT_DATA_IN_PARTITION ?=
OTA_PARTITION_OFFSET :=
OTA_PARTITION_SIZE :=

# Command used to get the size of a file
ifeq ("$(UNAME)", "Darwin")
  FILE_SIZE_CMD := stat -L -f %z
else
  FILE_SIZE_CMD := stat -L -c %s
endif
  
# Command used to generate the partition table
LUA_RTOS_GEN_PART_CMD := $(PYTHON) $(PROJECT_PATH)/boards/gen_part.py

# Get the flash size
CONFIG_LUA_RTOS_FLASH_SIZE := 4194304

ifeq ("$(CONFIG_ESPTOOLPY_FLASHSIZE_1MB)", "y")
  CONFIG_LUA_RTOS_FLASH_SIZE := 1048576
endif

ifeq ("$(CONFIG_ESPTOOLPY_FLASHSIZE_2MB)", "y")
  CONFIG_LUA_RTOS_FLASH_SIZE := 2097152
endif

ifeq ("$(CONFIG_ESPTOOLPY_FLASHSIZE_4MB)", "y")
  CONFIG_LUA_RTOS_FLASH_SIZE := 4194304
endif

ifeq ("$(CONFIG_ESPTOOLPY_FLASHSIZE_8MB)", "y")
  CONFIG_LUA_RTOS_FLASH_SIZE := 8388608
endif

ifeq ("$(CONFIG_ESPTOOLPY_FLASHSIZE_16MB)", "y")
  CONFIG_LUA_RTOS_FLASH_SIZE := 16777216
endif

# Generate the Lua RTOS partition table
$(PARTITION_TABLE_BIN): | gen-part

gen-part:
	$(info Generating Lua RTOS partition table...)
	$(LUA_RTOS_GEN_PART_CMD) -LUA_RTOS_PARTITION_TABLE_OFFSET=$(CONFIG_PARTITION_TABLE_OFFSET)\
							-LUA_RTOS_FLASH_SIZE=$(CONFIG_LUA_RTOS_FLASH_SIZE)\
							-LUA_RTOS_USE_OTA=$(CONFIG_LUA_RTOS_USE_OTA)\
							-LUA_RTOS_USE_FACTORY_PARTITION=$(CONFIG_LUA_RTOS_USE_FACTORY_PARTITION)\
							-LUA_RTOS_PART_STORAGE_SIZE=$(CONFIG_LUA_RTOS_PART_STORAGE_SIZE)\
							-LUA_RTOS_PART_NVS_SIZE=$(CONFIG_LUA_RTOS_PART_NVS_SIZE)\
							-LUA_RTOS_USE_FAT=$(CONFIG_LUA_RTOS_USE_FAT)\
							-LUA_RTOS_USE_SPIFFS=$(CONFIG_LUA_RTOS_USE_SPIFFS)\
							-LUA_RTOS_USE_LFS=$(CONFIG_LUA_RTOS_USE_LFS)\
							-LUA_RTOS_PARTION_CSV=$(PARTITION_TABLE_CSV_PATH)\
							-LUA_RTOS_PHY_INIT_DATA_IN_PARTITION=$(CONFIG_ESP32_PHY_INIT_DATA_IN_PARTITION)
	$(GEN_ESP32PART) $(PARTITION_TABLE_CSV_PATH) $(PARTITION_TABLE_BIN)

# Erases the OTA data partition
erase-ota-data:
	$(if $(filter "foo","foo$(OTA_DATA_OFFSET)"),,\
		$(info Erasing OTA partition...)\
		$(ESPTOOLPY) --chip esp32 --port $(ESPPORT) --baud $(ESPBAUD) erase_region $(OTA_DATA_OFFSET) $(OTA_DATA_SIZE)\
	)
	
# Check that FLASH have enough space to fit the app
app-check-space:
	$(eval APP_SIZE := $(shell $(FILE_SIZE_CMD) $(APP_BIN)))
	$(eval APP_PART_SIZE := $(shell $(GET_PART_INFO) -q --partition-table-file $(PARTITION_TABLE_BIN) --partition-name factory get_partition_info --info size))
	$(if $(filter "foo","foo$(APP_PART_SIZE)"),\
	    $(eval APP_PART_SIZE := $(shell $(GET_PART_INFO) -q --partition-table-file $(PARTITION_TABLE_BIN) --partition-name ota_0 get_partition_info --info size))\
	)
	$(if $(filter "foo","foo$(APP_PART_SIZE)"),,\
		$(eval APP_PART_SIZE := $(shell python -c 'print(int("$(APP_PART_SIZE)", 16))'))\
		$(info App size is $(APP_SIZE) bytes, max size for app in FLASH is $(APP_PART_SIZE) bytes)\
		$(if $(filter "1","$(shell expr $(APP_SIZE) \> $(APP_PART_SIZE))"),\
			$(error Not enough space for app in FLASH)\
		)\
		$(info $(shell expr $(APP_PART_SIZE) - $(APP_SIZE)) bytes remainign in FLASH for app)\
	)	
	
# When flashing app, check that there is enough space en FLASH, and erase ota data partition
flash: | app-check-space erase-ota-data