#
# Component Makefile
#

ifdef CONFIG_LUA_RTOS_LORA_HW_TYPE_SX1276
  ifdef CONFIG_LUA_RTOS_LORA_STACK_LMIC
  COMPONENT_SRCDIRS := ./gateway/multi_channel/src ./gateway/single_channel ./node/lmic
  COMPONENT_ADD_INCLUDEDIRS := ./gateway/multi_channel/inc ./gateway/single_channel ./node/lmic
  endif
  
  ifdef CONFIG_LUA_RTOS_LORA_STACK_SEMTECH
  COMPONENT_SRCDIRS := ./node/common ./node/semtech/radio ./node/semtech/radio/sx126x ./node/semtech/radio/sx1272 ./node/semtech/radio/sx1276 ./node/semtech/port ./node/semtech/system ./node/semtech/system/crypto ./node/semtech/mac ./node/semtech/mac/region
                       
  COMPONENT_ADD_INCLUDEDIRS := ./node/common ./node/semtech ./node/semtech/radio ./node/semtech/radio/sx126x ./node/semtech/radio/sx1272 ./node/semtech/radio/sx1276 ./node/semtech/port ./node/semtech/system ./node/semtech/system/crypto ./node/semtech/mac ./node/semtech/mac/region
  endif
else
  ifdef CONFIG_LUA_RTOS_LORA_HW_TYPE_SX1272
    COMPONENT_SRCDIRS := ./gateway/multi_channel/src ./gateway/single_channel ./node/lmic
    COMPONENT_ADD_INCLUDEDIRS := ./gateway/multi_channel/inc ./gateway/single_channel ./node/lmic
  else
    ifdef CONFIG_LUA_RTOS_LORA_HW_TYPE_SX1301
      COMPONENT_SRCDIRS := ./gateway/multi_channel/src ./gateway/single_channel ./node/lmic
      COMPONENT_ADD_INCLUDEDIRS := ./gateway/multi_channel/inc ./gateway/single_channel ./node/lmic
    else
      # disable LORA support
      COMPONENT_SRCDIRS :=
      COMPONENT_ADD_INCLUDEDIRS :=
    endif
  endif
endif
