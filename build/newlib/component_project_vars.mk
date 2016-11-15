# Automatically generated build file. Do not edit.
COMPONENT_INCLUDES += $(IDF_PATH)/components/newlib/include $(IDF_PATH)/components/newlib/platform_include
COMPONENT_LDFLAGS += $(IDF_PATH)/components/newlib/lib/libc.a $(IDF_PATH)/components/newlib/lib/libm.a -lnewlib
newlib-build: 
