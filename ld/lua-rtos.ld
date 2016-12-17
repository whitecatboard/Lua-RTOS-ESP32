SECTIONS {
    .flash.rodata.lua_rodata : {
	    . = ALIGN(4);
	    /* Lua enabled modules. KEEP ensures that will not be removed */
	    lua_libs1 = ABSOLUTE(.);
	        KEEP (*(.lua_libs1))
	    LONG(0) LONG(0) /* Null-terminate the array */

	    lua_rotable = ABSOLUTE(.);
	    KEEP(*(.lua_rotable1))
	    LONG(0) LONG(0) /* Null-terminate the array */
	}
}
INSERT AFTER .flash.rodata;