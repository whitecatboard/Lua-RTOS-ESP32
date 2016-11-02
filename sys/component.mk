INC_DIRS += $(sys_ROOT).. $(sys_ROOT)drivers/lmic

CFLAGS += -DKERNEL

CFLAGS += -DCFG_eu868=1
CFLAGS += -DCFG_sx1276_radio=1

sys_SRC_DIR  = $(sys_ROOT) $(sys_ROOT)drivers $(sys_ROOT)drivers/platform/$(PLATFORM)
sys_SRC_DIR += $(sys_ROOT)platform/$(PLATFORM) $(sys_ROOT)list $(sys_ROOT)syscalls $(sys_ROOT)unix
sys_SRC_DIR += $(sys_ROOT)spiffs $(sys_ROOT)spiffs/platform/$(PLATFORM)

#   $(sys_ROOT)unix $(sys_ROOT)fat

$(eval $(call component_compile_rules,sys))