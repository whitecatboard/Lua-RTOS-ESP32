if(CONFIG_LUA_RTOS_LORA_HW_TYPE_SX1301)
	set(srcs
		"gateway/multi_channel/src/base64.c"
		"gateway/multi_channel/src/jitqueue.c"
		"gateway/multi_channel/src/lora_pkt_fwd.c"
		"gateway/multi_channel/src/loragw_aux.c"
		"gateway/multi_channel/src/loragw_fpga.c"
		"gateway/multi_channel/src/loragw_gps.c"
		"gateway/multi_channel/src/loragw_hal.c"
		"gateway/multi_channel/src/loragw_lbt.c"
		"gateway/multi_channel/src/loragw_radio.c"
		"gateway/multi_channel/src/loragw_reg.c"
		"gateway/multi_channel/src/loragw_spi.native.c"
		"gateway/multi_channel/src/main.c"
		"gateway/multi_channel/src/parson.c"
		"gateway/multi_channel/src/platform.c"
		"gateway/multi_channel/src/timersync.c"
	)
	
	set(include_dirs "gateway/multi_channel/inc")	
elseif(CONFIG_LUA_RTOS_LORA_HW_TYPE_SX1272)
	set(srcs
		"node/lmic/aes.c"
		"node/lmic/lmic_hal.c"
		"node/lmic/lmic.c"
		"node/lmic/lora_lmic.c"
		"node/lmic/oslmic.c"
		"node/lmic/radio.c"
		"gateway/single_channel/gateway.c"
		"gateway/single_channel/sx1276.c"
	)
	
	set(include_dirs
        "gateway/multi_channel/inc"
		"node/lmic"
		"gateway/single_channel"
	)
elseif(CONFIG_LUA_RTOS_LORA_HW_TYPE_SX1276)
	set(srcs
		"node/lmic/aes.c"
		"node/lmic/lmic_hal.c"
		"node/lmic/lmic.c"
		"node/lmic/lora_lmic.c"
		"node/lmic/oslmic.c"
		"node/lmic/radio.c"
		"gateway/single_channel/gateway.c"
		"gateway/single_channel/sx1276.c"
		"gateway/multi_channel/src/base64.c"
	)
	
	set(include_dirs
		"gateway/multi_channel/inc"
		"node/lmic"
		"gateway/single_channel"
	)
else()
	set(srcs
	)
	
	set(include_dirs ".")
endif()

idf_component_register(
	SRCS "${srcs}"
    INCLUDE_DIRS "${include_dirs}"
    REQUIRES sys
)

add_compile_options("-DLIBLORAGW_VERSION=\"4.1.3\"")
add_compile_options("-DEBUG_AUX=0")
add_compile_options("-DEBUG_SPI=0")
add_compile_options("-DEBUG_REG=0")
add_compile_options("-DEBUG_HAL=0")
add_compile_options("-DEBUG_LBT=0")
add_compile_options("-DEBUG_GPS=0")

add_compile_options("-Wno-char-subscripts")
add_compile_options("-Wno-maybe-uninitialized")
