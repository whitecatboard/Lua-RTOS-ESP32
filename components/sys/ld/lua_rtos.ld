SECTIONS {
  .flash.rodata : ALIGN(4)
  {
    _lua_rtos_rodata_start = ABSOLUTE(.);

	/* This is the array for Lua libraries to load at startup */
    . = ALIGN(8);
    lua_libs1 = ABSOLUTE(.);
    KEEP (*(.lua_libs1))
    LONG(0) LONG(0)

	/* This is the array for readonly Lua tables */
    . = ALIGN(8);
    lua_rotable = ABSOLUTE(.);
    KEEP(*(.lua_rotable1))
    LONG(0) LONG(0)
    LONG(0) LONG(0)

    /* This is the array for drivers available in Lua RTOS build */
    . = ALIGN(4);
    drivers = ABSOLUTE(.);
    KEEP(*(.drivers))
    LONG(0) LONG(0)
    LONG(0) LONG(0)

    KEEP(*(.driver_error))
    KEEP(*(.driver_error_map))

    /* This is the array for sensors available in Lua RTOS build */
    . = ALIGN(4);
    sensors = ABSOLUTE(.);
    KEEP(*(.sensors))
    LONG(0) LONG(0)
    LONG(0) LONG(0)

    . = ALIGN(4);
    KEEP(*(.romfs))
    
    _lua_rtos_rodata_end = ABSOLUTE(.);
  } >drom0_0_seg
}
