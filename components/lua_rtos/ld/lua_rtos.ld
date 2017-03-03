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

    /* This is the array for sensors available in Lua RTOS build */
    sensors = ABSOLUTE(.);
    KEEP(*(.sensors))
    LONG(0) LONG(0)

	/* For each driver we need to declare 2 entries in linker script */
	/* One entry is for the driver messages, and other entry is for */
	/* exception names exported to Lua modules */
	/* TO DO */
	/*   Generate this entries automatically */
    uart_errors = ABSOLUTE(.);
    KEEP(*(.uart_errors))
    LONG(0) LONG(0)

    uart_error_map = ABSOLUTE(.);
    KEEP(*(.uart_error_map))
    LONG(0) LONG(0)

    pwbus_errors = ABSOLUTE(.);
    KEEP(*(.pwbus_errors))
    LONG(0) LONG(0)

    pwbus_error_map = ABSOLUTE(.);
    KEEP(*(.pwbus_error_map))
    LONG(0) LONG(0)

    thread_errors = ABSOLUTE(.);
    KEEP(*(.thread_errors))
    LONG(0) LONG(0)

    thread_error_map = ABSOLUTE(.);
    KEEP(*(.thread_error_map))
    LONG(0) LONG(0)

    spi_errors = ABSOLUTE(.);
    KEEP(*(.spi_errors))
    LONG(0) LONG(0)

    spi_error_map = ABSOLUTE(.);
    KEEP(*(.spi_error_map))
    LONG(0) LONG(0)

    espi_errors = ABSOLUTE(.);
    KEEP(*(.espi_errors))
    LONG(0) LONG(0)

    espi_error_map = ABSOLUTE(.);
    KEEP(*(.espi_error_map))
    LONG(0) LONG(0)

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

    mqtt_errors = ABSOLUTE(.);
    KEEP(*(.mqtt_errors))
    LONG(0) LONG(0)

    mqtt_error_map = ABSOLUTE(.);
    KEEP(*(.mqtt_error_map))
    LONG(0) LONG(0)

    net_errors = ABSOLUTE(.);
    KEEP(*(.net_errors))
    LONG(0) LONG(0)

    net_error_map = ABSOLUTE(.);
    KEEP(*(.net_error_map))
    LONG(0) LONG(0)

    event_errors = ABSOLUTE(.);
    KEEP(*(.event_errors))
    LONG(0) LONG(0)

    event_error_map = ABSOLUTE(.);
    KEEP(*(.event_error_map))
    LONG(0) LONG(0)

    servo_errors = ABSOLUTE(.);
    KEEP(*(.servo_errors))
    LONG(0) LONG(0)

    servo_error_map = ABSOLUTE(.);
    KEEP(*(.servo_error_map))
    LONG(0) LONG(0)

    owire_errors = ABSOLUTE(.);
    KEEP(*(.owire_errors))
    LONG(0) LONG(0)

    owire_error_map = ABSOLUTE(.);
    KEEP(*(.owire_error_map))
    LONG(0) LONG(0)

    sensor_errors = ABSOLUTE(.);
    KEEP(*(.sensor_errors))
    LONG(0) LONG(0)

    sensor_error_map = ABSOLUTE(.);
    KEEP(*(.sensor_error_map))
    LONG(0) LONG(0)

    _lua_rtos_rodata_end = ABSOLUTE(.);
  } >drom0_0_seg
}