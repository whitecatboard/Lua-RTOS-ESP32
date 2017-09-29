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

    KEEP(*(.driver_error))
    KEEP(*(.driver_error_map))

    /* This is the array for sensors available in Lua RTOS build */
    sensors = ABSOLUTE(.);
    KEEP(*(.sensors))
    LONG(0) LONG(0)

    _lua_rtos_rodata_end = ABSOLUTE(.);
  } >drom0_0_seg
}
