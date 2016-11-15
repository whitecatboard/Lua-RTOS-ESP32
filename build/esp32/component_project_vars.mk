# Automatically generated build file. Do not edit.
COMPONENT_INCLUDES += $(IDF_PATH)/components/esp32/include
COMPONENT_LDFLAGS += -lesp32 $(IDF_PATH)/components/esp32/libhal.a -L$(IDF_PATH)/components/esp32/lib -lcore -lnet80211 -lphy -lrtc -lpp -lwpa -lsmartconfig -lcoexist -lwps -L $(IDF_PATH)/components/esp32/ld -T esp32_out.ld -T esp32.common.ld -T esp32.rom.ld -T esp32.peripherals.ld
esp32-build: 
