#
# This Makefile contains rules related to the creation of the ROMFS
#
# The creation of the file system is controlled by the following variables:
#
# ROMFS_ROOT: Is the path (on the host) that contains the file system content. All the files and
#             directories below this path will be copied into the file system. If the path is a
#             relative path firt it is search in COMPONENT_PATH, and then in PROJECT_PATH.
#

ifdef CONFIG_LUA_RTOS_USE_ROM_FS

.PHONY: romfs_image_build

ROMFS_ROOT ?=
ROMFS_ABS_ROOT := $(ROMFS_ROOT)

# Get current working directory into the BUILD_DIR directory
ROMFS_CWD := $(abspath $(dir .))

ifeq ("foo$(ROMFS_ROOT)","foo")
  # Use default empty file system
  ROMFS_ABS_ROOT := $(abspath $(COMPONENT_PATH)/empty)
else
  ifneq ("$(shell test -e $(ROMFS_ROOT) && echo ex)","ex")
    # Not found, search in COMPONENT_PATH
    ifeq ("$(shell test -e $(abspath $(COMPONENT_PATH)/$(ROMFS_ROOT)) && echo ex)","ex")
      # Found 
      ROMFS_ABS_ROOT := $(abspath $(COMPONENT_PATH)/$(ROMFS_ROOT))
    else
      # Not found, search in PROJECT_PATH
      ifeq ("$(shell test -e $(abspath $(PROJECT_PATH)/$(ROMFS_ROOT)) && echo ex)","ex")
        # Found
        ROMFS_ABS_ROOT := $(abspath $(PROJECT_PATH)/$(ROMFS_ROOT))
      else
        # Not found
        $(error $(ROMFS_ROOT) doesn't exist)
      endif
    endif
  endif
endif


ROMFS_SYMBOL_START := _binary_$(subst .,_,$(subst -,_,$(subst /,_,$(ROMFS_CWD)/romfs.img)))_start
ROMFS_SYMBOL_END := _binary_$(subst .,_,$(subst -,_,$(subst /,_,$(ROMFS_CWD)/romfs.img)))_end
ROMFS_SYMBOL_SIZE := _binary_$(subst .,_,$(subst -,_,$(subst /,_,$(ROMFS_CWD)/romfs.img)))_size
  
# Build the file system
build: | romfs_image_build

romfs_image_build: $(ROMFS_ABS_ROOT)
	@$(MAKE) -C $(COMPONENT_PATH)/../mkromfs/src all
	@echo "Compiling ROMFS image..."
	@rm -f -r $(ROMFS_CWD)/root
	@cp -f -r $^ $(ROMFS_CWD)/root
	@rm -f $(ROMFS_CWD)/libromfs_image.a
	@$(COMPONENT_PATH)/../mkromfs/src/mkromfs -c $(ROMFS_CWD)/root -i $(ROMFS_CWD)/romfs.img
	@$(OBJCOPY) -I binary -O elf32-xtensa-le -B xtensa --rename-section .data=.romfs \
		--redefine-sym $(ROMFS_SYMBOL_START)=_romfs_start\
		--redefine-sym $(ROMFS_SYMBOL_END)=_romfs_end\
		--strip-symbol $(ROMFS_SYMBOL_SIZE)\
		$(ROMFS_CWD)/romfs.img $(ROMFS_CWD)/romfs.img.o
	@$(AR) cru $(ROMFS_CWD)/libromfs_image.a $(ROMFS_CWD)/romfs.img.o
	
endif