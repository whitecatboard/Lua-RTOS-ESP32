if(CONFIG_LUA_RTOS_LORA_HW_TYPE_SX1301)
	set(srcs
		"gateway/multi_channel/srcbase64.c"
		"gateway/multi_channel/srcjitqueue.c"
		"gateway/multi_channel/srclora_pkt_fwd.c"
		"gateway/multi_channel/srcloragw_aux.c"
		"gateway/multi_channel/srcloragw_fpga.c"
		"gateway/multi_channel/srcloragw_gps.c"
		"gateway/multi_channel/srcloragw_hal.c"
		"gateway/multi_channel/srcloragw_lbt.c"
		"gateway/multi_channel/srcloragw_radio.c"
		"gateway/multi_channel/srcloragw_reg.c"
		"gateway/multi_channel/srcloragw_spi.native.c"
		"gateway/multi_channel/srcmain.c"
		"gateway/multi_channel/srcparson.c"
		"gateway/multi_channel/srcplatform.c"
		"gateway/multi_channel/srctimersync.c"
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
