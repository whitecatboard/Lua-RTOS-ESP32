if(CONFIG_LUA_RTOS_LORA_MODE_SINGLE_CHANNEL_GATEWAY)
	set(stack_srcs
		"gateway/multi_channel/src/base64.c"
		"gateway/single_channel/lora.c"
		"gateway/single_channel/gateway.c"
		"gateway/single_channel/sx1276.c"
	)
	
	set(pub_incs "gateway/single_channel")	
	set(stack_incs "gateway/multi_channel/inc")	
elseif(CONFIG_LUA_RTOS_LORA_MODE_MULTICHANNEL_GATEWAY)
	set(stack_srcs
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
	
	set(stack_incs "gateway/multi_channel/inc")	
elseif(CONFIG_LUA_RTOS_LORA_MODE_NODE)
	set(stack_srcs
		"node/semtech/port/lora.c"
		"node/semtech/port/board.c"
		"node/semtech/port/aes.c"
		"node/semtech/port/delay-board.c"
		"node/semtech/port/gpio-board.c"
		"node/semtech/port/rtc-board.c"
		"node/semtech/port/spi-board.c"
		"node/semtech/port/eeprom-board.c"
		"node/semtech/port/cmac.c"
		"node/semtech/port/soft-se.c"
		"node/semtech/port/LmHandler.c"
		"node/semtech/loramac-node/src/boards/mcu/utilities.c"
		"node/semtech/loramac-node/src/peripherals/soft-se/soft-se-hal.c"
		"node/semtech/loramac-node/src/mac/LoRaMac.c"
		"node/semtech/loramac-node/src/mac/LoRaMacAdr.c"
		"node/semtech/loramac-node/src/mac/LoRaMacClassB.c"
		"node/semtech/loramac-node/src/mac/LoRaMacCommands.c"
		"node/semtech/loramac-node/src/mac/LoRaMacConfirmQueue.c"
		"node/semtech/loramac-node/src/mac/LoRaMacCrypto.c"
		"node/semtech/loramac-node/src/mac/LoRaMacParser.c"
		"node/semtech/loramac-node/src/mac/LoRaMacSerializer.c"
		"node/semtech/loramac-node/src/mac/region/Region.c"
		"node/semtech/loramac-node/src/mac/region/RegionCommon.c"
		"node/semtech/loramac-node/src/mac/region/RegionEU868.c"
		"node/semtech/loramac-node/src/system/delay.c"
		"node/semtech/loramac-node/src/system/gpio.c"
		"node/semtech/loramac-node/src/system/nvmm.c"
		"node/semtech/loramac-node/src/system/systime.c"
		"node/semtech/loramac-node/src/system/timer.c"
		"node/semtech/loramac-node/src/apps/LoRaMac/common/NvmDataMgmt.c"
		"node/semtech/loramac-node/src/apps/LoRaMac/common/LmHandlerMsgDisplay.c"
		"node/semtech/loramac-node/src/apps/LoRaMac/common/LmHandler/packages/FragDecoder.c"
		"node/semtech/loramac-node/src/apps/LoRaMac/common/LmHandler/packages/LmhpClockSync.c"
		"node/semtech/loramac-node/src/apps/LoRaMac/common/LmHandler/packages/LmhpCompliance.c"
		"node/semtech/loramac-node/src/apps/LoRaMac/common/LmHandler/packages/LmhpFragmentation.c"
		"node/semtech/loramac-node/src/apps/LoRaMac/common/LmHandler/packages/LmhpRemoteMcastSetup.c"
	)
		
	set(stack_incs
		"node/semtech/port"
	    "node/semtech/loramac-node/src/boards"
	    "node/semtech/loramac-node/src/system"
		"node/semtech/loramac-node/src/peripherals/soft-se"
	    "node/semtech/loramac-node/src/radio"
		"node/semtech/loramac-node/src/mac"
		"node/semtech/loramac-node/src/mac/region"
		"node/semtech/loramac-node/src/apps/LoRaMac/common"
		"node/semtech/loramac-node/src/apps/LoRaMac/common/LmHandler"
		"node/semtech/loramac-node/src/apps/LoRaMac/common/LmHandler/packages"
	)

	set(pub_incs
		"node/semtech/port"
	)
	
	if(CONFIG_LUA_RTOS_LORA_HW_TYPE_SX1272)
		set(radio_srcs
		    "node/semtech/loramac-node/src/radio/sx1272/sx1272.c"
			"node/semtech/port/sx1272-board.c"
		)
			
		set(radio_incs
		    "node/semtech/loramac-node/src/radio/sx1272"
		)	
	elseif(CONFIG_LUA_RTOS_LORA_HW_TYPE_SX1276)		
		set(radio_srcs
		    "node/semtech/loramac-node/src/radio/sx1276/sx1276.c"
			"node/semtech/port/sx1276-board.c"
		)
			
		set(radio_incs
		    "node/semtech/loramac-node/src/radio/sx1276"
		)	
	endif()
endif()

idf_component_register(
	SRCS "${stack_srcs}" "${radio_srcs}" 
    INCLUDE_DIRS "${pub_incs}"
    PRIV_INCLUDE_DIRS "${stack_incs}" "${radio_incs}"
    PRIV_REQUIRES sys esp_timer
)

if(CONFIG_LUA_RTOS_LORA_MODE_MULTICHANNEL_GATEWAY)
  target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-format)
  target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-char-subscripts)
  target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-maybe-uninitialized)
  target_compile_options(${COMPONENT_LIB} PRIVATE -DLIBLORAGW_VERSION=\"4.1.3\")
  target_compile_options(${COMPONENT_LIB} PRIVATE -DEBUG_AUX=0)
  target_compile_options(${COMPONENT_LIB} PRIVATE -DEBUG_SPI=0)  
  target_compile_options(${COMPONENT_LIB} PRIVATE -DEBUG_REG=0)  
  target_compile_options(${COMPONENT_LIB} PRIVATE -DEBUG_HAL=0)  
  target_compile_options(${COMPONENT_LIB} PRIVATE -DEBUG_LBT=0)  
  target_compile_options(${COMPONENT_LIB} PRIVATE -DEBUG_GPS=0)  
elseif(CONFIG_LUA_RTOS_LORA_MODE_NODE)
  target_compile_options(${COMPONENT_LIB} PRIVATE -DSOFT_SE)
  target_compile_options(${COMPONENT_LIB} PRIVATE -DREGION_EU868)
  
  if(CONFIG_LUA_RTOS_LORA_HW_TYPE_SX1276)
    target_compile_options(${COMPONENT_LIB} PRIVATE -DSX1276MB1LAS)
  elseif(CONFIG_LUA_RTOS_LORA_HW_TYPE_SX1272)
    target_compile_options(${COMPONENT_LIB} PRIVATE -DSX1272MB2DAS)
  endif()
endif()


