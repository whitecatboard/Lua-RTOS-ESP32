INC_DIRS += $(sys_ROOT)..
CFLAGS += -DKERNEL

sys_SRC_DIR = $(sys_ROOT) $(sys_ROOT)platform/$(PLATFORM) $(sys_ROOT)syscalls $(sys_ROOT)list $(sys_ROOT)drivers $(sys_ROOT)drivers/platform/$(PLATFORM) $(sys_ROOT)unix $(sys_ROOT)fat $(sys_ROOT)fat/option $(sys_ROOT)spiffs $(sys_ROOT)spiffs/platform/$(PLATFORM)
# sys_SRC_DIR += $(sys_ROOT)drivers/lmic

$(eval $(call component_compile_rules,sys))
