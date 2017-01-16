SECTIONS {
  .flash.rodata : ALIGN(4)
  {
    _lua_rtos_rodata_start = ABSOLUTE(.);

	/* This is the array for Lua libraries to load at startup */
    . = ALIGN(4);
    lua_libs1 = ABSOLUTE(.);
    KEEP (*(.lua_libs1))
    LONG(0) LONG(0)

	/* This is the array for readonly Lua tables */
    lua_rotable = ABSOLUTE(.);
    KEEP(*(.lua_rotable1))
    LONG(0) LONG(0)
    
    /* This is the array for drivers available in Lua RTOS build */
    drivers = ABSOLUTE(.);
    KEEP(*(.drivers))
    LONG(0) LONG(0)

	/* For each driver we need to declare 2 entries in linker script */
	/* One entry is for the driver messages, and other entry is for */
	/* exception names exported to Lua modules */
	/* TO DO */
	/*   Generate this entries automatically */
    adc_errors = ABSOLUTE(.);
    KEEP(*(.adc_errors))
    LONG(0) LONG(0)

    adc_error_map = ABSOLUTE(.);
    KEEP(*(.adc_error_map))
    LONG(0) LONG(0)

    i2c_errors = ABSOLUTE(.);
    KEEP(*(.i2c_errors))
    LONG(0) LONG(0)

    i2c_error_map = ABSOLUTE(.);
    KEEP(*(.i2c_error_map))
    LONG(0) LONG(0)

    lora_errors = ABSOLUTE(.);
    KEEP(*(.lora_errors))
    LONG(0) LONG(0)

    lora_error_map = ABSOLUTE(.);
    KEEP(*(.lora_error_map))
    LONG(0) LONG(0)

    pwm_errors = ABSOLUTE(.);
    KEEP(*(.pwm_errors))
    LONG(0) LONG(0)

    pwm_error_map = ABSOLUTE(.);
    KEEP(*(.pwm_error_map))
    LONG(0) LONG(0)

    wifi_errors = ABSOLUTE(.);
    KEEP(*(.wifi_errors))
    LONG(0) LONG(0)

    wifi_error_map = ABSOLUTE(.);
    KEEP(*(.wifi_error_map))
    LONG(0) LONG(0)

    gpio_errors = ABSOLUTE(.);
    KEEP(*(.gpio_errors))
    LONG(0) LONG(0)

    gpio_error_map = ABSOLUTE(.);
    KEEP(*(.gpio_error_map))
    LONG(0) LONG(0)

    net_errors = ABSOLUTE(.);
    KEEP(*(.net_errors))
    LONG(0) LONG(0)

    net_error_map = ABSOLUTE(.);
    KEEP(*(.net_error_map))
    LONG(0) LONG(0)

    _lua_rtos_rodata_end = ABSOLUTE(.);
  } >drom0_0_seg
}