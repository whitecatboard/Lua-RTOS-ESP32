#
# Lua RTOS build system
#
# This Makefile contains rules related to the creation of a file system, that can be flashed into the FLASH memory.
# This allows the Lua RTOS users to create a file system, with the required files to run an application, without
# the need to use an external storage device. In this way, when the user builds Lua RTOS, the file system can be
# builded and flashed to the device.
#
# The creation of the file system is controlled by the following variables:
#
# FS_ROOT_PATH: Is the path (on the host) that contains the common file system content. All the files and
#               directories below this path will be copied into the file system. By default it is set to the
#               file system defined in the boards.json file.
#
# COMPONENT_ADD_FS: Is a variable that can be defined in any Lua RTOS component.mk file that adds specific content
#                   to the file system, if the component is included in the Lua RTOS build. This variable is the
#                   path on the host that contains the specific content.
#
# FS_SEARCH_PATH: Contains the path on the host to look for Lua RTOS components that adds specific content to the
#                 file system. By default, the searching starts into the components and components/lua/modules
#                 directories located under the project path.
#

.PHONY: fs-info fs-prepare fs-spiffs fs-lfs flashfs fs flashfs-args
.NOTPARALLEL: fs-info fs-prepare fs-spiffs fs-lfs flashfs fs flashfs-args

FS_ROOT_PATH ?=
FS_SEARCH_PATH ?=
COMPONENT_ADD_FS :=
COMPONENT_FS :=

FS_PARTITION := storage

# Don't include this components
FS_EXCLUDE_COMPONENTS := romfs_image

# Set the root path
ifeq ("foo$(FS_ROOT_PATH)", "foo")
  ifneq ("foo$(BOARDN)", "foo")
    FS_ROOT_PATH := $(PROJECT_PATH)/components/fs_images/$(shell $(PYTHON) $(PROJECT_PATH)/boards/boards.py $(BOARDN) filesystem)
  else
    FS_ROOT_PATH := $(PROJECT_PATH)/components/fs_images/default
  endif
endif

ifneq ("$(shell test -e $(FS_ROOT_PATH) && echo ex)","ex")
  $(error $(FS_ROOT_PATH) doesn't exist)
endif

# Set the search path
ifeq ("foo$(FS_SEARCH_PATH)", "foo")
  FS_SEARCH_PATH := $(PROJECT_PATH)/components $(PROJECT_PATH)/components/lua/modules
endif

$(foreach cd,$(FS_SEARCH_PATH), \
  $(if $(filter ex,$(shell test -e $(cd) && echo ex)),,$(error $(cd) doesn't exist))\
)

# Get the path for all the component located in FS_SEARCH_PATH
FS_COMPONENTS := $(dir $(foreach cd,$(FS_SEARCH_PATH), \
					$(wildcard $(cd)/*/component.mk) $(wildcard $(cd)/component.mk) \
				 ))
				   
FS_COMPONENTS := $(sort $(foreach comp,$(FS_COMPONENTS),$(lastword $(subst /, ,$(comp)))))

FS_COMPONENTS_PATHS := $(foreach comp,$(FS_COMPONENTS),$(firstword $(foreach cd,$(FS_SEARCH_PATH),$(wildcard $(dir $(cd))$(comp) $(cd)/$(comp)))))

# For a given component path, this macro adds the specific content (if any) defined by the component
define includeComponentFS
  $(if $(filter "$(shell test -e $(1)/component.mk && echo ex)","ex"),
    $(if $(filter $(FS_EXCLUDE_COMPONENTS), $(notdir $(1))),,\
      COMPONENT_ADD_FS :=\
      $(eval include $(1)/component.mk)\
      $(if $(filter "$(COMPONENT_ADD_FS)",""),,      
        COMPONENT_FS += $(addsuffix /*,$(addprefix $(1)/, $(COMPONENT_ADD_FS)))\
      )\
    )
  )
endef

# Determine the file system type: SPIFFS or LFS
ifeq ("$(CONFIG_LUA_RTOS_USE_SPIFFS)", "y")
  FS_TYPE := spiffs
endif

ifeq ("$(CONFIG_LUA_RTOS_USE_LFS)", "y")
  FS_TYPE := lfs
endif

# Gets required information from the file system's storage partition
fs-info:
	$(eval FS_BASE_ADDR:=$(shell $(GET_PART_INFO) --partition-name $(FS_PARTITION) --offset $(PARTITION_TABLE_BIN)))
	$(eval FS_SIZE:=$(shell $(GET_PART_INFO) --partition-name $(FS_PARTITION) --size $(PARTITION_TABLE_BIN)))
	
# Copy all the file system content into a temporal directory, which is used in other rules
# to create the file system
fs-prepare:
	$(foreach componentpath,$(FS_COMPONENTS_PATHS), \
		$(eval $(call includeComponentFS,$(componentpath))))
	@rm -f -r $(PROJECT_PATH)/build/tmp-fs
	@mkdir -p $(PROJECT_PATH)/build/tmp-fs
	$(if $(filter "foo","foo$(COMPONENT_FS)"),,\
		@cp -f -r $(COMPONENT_FS) $(PROJECT_PATH)/build/tmp-fs\
	)
	@cp -f -r $(FS_ROOT_PATH)/* $(PROJECT_PATH)/build/tmp-fs

# Make spiffs file system
fs-spiffs: mkspiffs fs-prepare fs-info | gen-part
	@echo "Making spiffs image..."
	$(MKSPIFFS_COMPONENT_PATH)/../mkspiffs/src/mkspiffs -c $(PROJECT_PATH)/build/tmp-fs -b $(CONFIG_LUA_RTOS_SPIFFS_LOG_BLOCK_SIZE) -p $(CONFIG_LUA_RTOS_SPIFFS_LOG_PAGE_SIZE) -s $(FS_SIZE) $(BUILD_DIR_BASE)/spiffs_image.img

# Make lfs file system
fs-lfs: mklfs fs-prepare fs-info | gen-part
	@echo "Making lfs image..."
	$(MKLFS_COMPONENT_PATH)/../mklfs/src/mklfs -c $(PROJECT_PATH)/build/tmp-fs -b $(CONFIG_LUA_RTOS_LFS_BLOCK_SIZE) -p $(CONFIG_LUA_RTOS_LFS_PROG_SIZE) -r $(CONFIG_LUA_RTOS_LFS_READ_SIZE) -s $(FS_SIZE) -i $(BUILD_DIR_BASE)/lfs_image.img

# Make file system
fs: fs-$(FS_TYPE) 

# Flash the file system
flashfs: fs
	$(info Flasing $(shell basename $(FS_ROOT_PATH)) $(FS_TYPE) file system)
	$(ESPTOOLPY_WRITE_FLASH) $(FS_BASE_ADDR) $(BUILD_DIR_BASE)/$(FS_TYPE)_image.img

# Print the arguments required by the esptool.py tool to flash the file system 
flashfs-args: fs-info | gen-part
	@echo $(subst $(PROJECT_PATH)/build/,, \
			$(subst --port $(ESPPORT),, \
				$(subst python /components/esptool_py/esptool/esptool.py,, \
					$(subst $(IDF_PATH),, $(ESPTOOLPY_WRITE_FLASH) $(FS_BASE_ADDR) $(BUILD_DIR_BASE)/$(FS_TYPE)_image.img)\
				)\
	 	  	) \
	 	  )