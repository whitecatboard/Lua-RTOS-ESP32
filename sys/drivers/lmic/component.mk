INC_DIRS += $(lmic_ROOT)..

CFLAGS += -DCFG_eu868=1
CFLAGS += -DCFG_sx1276_radio=1

lmic_SRC_DIR = $(lmic_ROOT)

$(eval $(call component_compile_rules,lmic))
