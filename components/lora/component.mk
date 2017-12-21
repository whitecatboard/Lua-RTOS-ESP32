#
# Component Makefile
#

ifdef CONFIG_LUA_RTOS_LORA_HW_TYPE_SX1276
  COMPONENT_SRCDIRS := ./gateway/multi_channel/src ./gateway/single_channel ./node/lmic ./common
  COMPONENT_ADD_INCLUDEDIRS := ./gateway/multi_channel/inc ./gateway/single_channel ./node/lmic ./common
else
  ifdef CONFIG_LUA_RTOS_LORA_HW_TYPE_SX1272
    COMPONENT_SRCDIRS := ./gateway/multi_channel/src ./gateway/single_channel ./node/lmic ./common
    COMPONENT_ADD_INCLUDEDIRS := ./gateway/multi_channel/inc ./gateway/single_channel ./node/lmic ./common
  else
    ifdef CONFIG_LUA_RTOS_LORA_HW_TYPE_SX1301
      COMPONENT_SRCDIRS := ./gateway/multi_channel/src ./gateway/single_channel ./node/lmic ./common
      COMPONENT_ADD_INCLUDEDIRS := ./gateway/multi_channel/inc ./gateway/single_channel ./node/lmic ./common
    else
      # disable LORA support
      COMPONENT_SRCDIRS :=
      COMPONENT_ADD_INCLUDEDIRS :=
    endif
  endif
endif
