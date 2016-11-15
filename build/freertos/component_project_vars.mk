# Automatically generated build file. Do not edit.
COMPONENT_INCLUDES += $(IDF_PATH)/components/freertos/include
COMPONENT_LDFLAGS += -lfreertos -Wl,--undefined=uxTopUsedPriority
freertos-build: 
