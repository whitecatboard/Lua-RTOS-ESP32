INC_DIRS += $(lmic_ROOT)..

lmic_SRC_DIR = $(lmic_ROOT)

$(eval $(call component_compile_rules,lmic))
