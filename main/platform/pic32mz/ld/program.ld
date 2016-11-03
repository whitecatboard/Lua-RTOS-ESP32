/*
 * Linker script for PIC32 firmware using chipKIT bootloader.
 * Flash area for user starts at 9d000000.
 * Start address for user program is 9d001000.
 */
OUTPUT_FORMAT("elf32-tradlittlemips", "elf32-tradbigmips",
	      "elf32-tradlittlemips")
OUTPUT_ARCH(mips)
ENTRY(start)
MEMORY
{
  /* Required by Microchip C32 linker */
  kseg0_program_mem (rx)    : ORIGIN = 0x9D000000, LENGTH = 2048K
  exception_mem     (rx)    : ORIGIN = 0x9D000000, LENGTH = 0x1000
  kseg0_boot_mem    (rx)    : ORIGIN = 0x9FC00000, LENGTH = 64K-64
  kseg1_boot_mem    (rx)    : ORIGIN = 0xBFC00000, LENGTH = 0
  kseg1_data_mem    (rw!x)  : ORIGIN = 0xA0000000, LENGTH = 512K
}

_malloc_r = __malloc_r;
_free_r = __free_r;
_realloc_r = __realloc_r;
_calloc_r = __calloc_r;

_write_r = __write_r;
_read_r = __read_r;
_fstat_r = __fstat_r;
_stat_r = __stat_r;
_close_r = __close_r;
_lseek_r = __lseek_r;
_open_r = __open_r;
_unlink_r = __unlink_r;
_remove_r = __remove_r;
_rename_r = __rename_r;
_times_r = __times_r;
_getpid_r = __getpid_r;
_newTick = __newTick;
_times_r = __times_r;
_gettimeofday_r = __gettimeofday_r;

_lock_acquire = vTaskEnterCritical;
_lock_acquire_recursive = vTaskEnterCritical;
_lock_try_acquire = vTaskEnterCritical;
_lock_try_acquire_recursive = vTaskEnterCritical;
_lock_release = vTaskExitCritical;
_lock_release_recursive = vTaskExitCritical;

_eram = ORIGIN(kseg1_data_mem) + LENGTH(kseg1_data_mem);

SECTIONS
{
  .text           :
  {
    /* Exception handlers. */
    KEEP(*(.exception))
    . = 0x1000;
    /* Execution starts here. */
    KEEP(*(.startup))
    *(.text .stub .text.* .gnu.linkonce.t.*)
    /* .gnu.warning sections are handled specially by elf32.em.  */
    *(.gnu.warning)
    *(.glue_7t) *(.glue_7)
    __rodata_start = . ;
    *(.rodata .rodata.* .gnu.linkonce.r.* .rel.dyn)
    *(.dinit)

    . = ALIGN (32 / 8);
    /* Lua enabled modules. KEEP ensures that will not be removed */
    lua_libs1 = ABSOLUTE(.);
	KEEP (*(.lua_libs1))
    LONG(0) LONG(0) /* Null-terminate the array */

    . = ALIGN (32 / 8);
    lua_rotable = ABSOLUTE(.);
    KEEP(*(.lua_rotable1))
    LONG(0) LONG(0) /* Null-terminate the array */

    /* Align here to ensure that the .text section ends on word boundary. */
    . = ALIGN (32 / 8);
    _etext = .;
  } > kseg0_program_mem

  /* Start data (internal SRAM).  */
  .data		  : AT (ADDR (.text) + SIZEOF (.text))
  {
    __data_start = . ;
    _gp = .; 	/* We use only 32k RAM for kernel, so no need for 0x8000 offset. */
    /* We want the small data sections together, so single-instruction offsets
       can access them all, and initialized data all before uninitialized, so
       we can shorten the on-disk segment size.  */
    *(.sdata .sdata.* .gnu.linkonce.s.*)
    *(.data .data.* .gnu.linkonce.d.*)
    *(.eh_frame)
    _edata = .;
  } > kseg1_data_mem

  .bss ADDR (.data) + SIZEOF (.data) (NOLOAD) :
  {
   __bss_start = .;
   *(.dynbss)
   *(.sbss .sbss.*)
   *(.scommon)
   *(.bss .bss.* .gnu.linkonce.b.*)
   *(COMMON)
   /* Align here to ensure that the .bss section occupies space up to
      _end.  Align after .bss to ensure correct alignment even if the
      .bss section disappears because there are no input sections.  */
   . = ALIGN (32 / 8);
  } > kseg1_data_mem
  __bss_end = . ;
  _heap_start = ABSOLUTE(.);
  _end = .;

  /* Stabs debugging sections.  */
  .stab          0 : { *(.stab) }
  .stabstr       0 : { *(.stabstr) }
  .stab.excl     0 : { *(.stab.excl) }
  .stab.exclstr  0 : { *(.stab.exclstr) }
  .stab.index    0 : { *(.stab.index) }
  .stab.indexstr 0 : { *(.stab.indexstr) }
  .comment       0 : { *(.comment) }
  /* DWARF debug sections.
     Symbols in the DWARF debugging sections are relative to the beginning
     of the section so we begin them at 0.  */
  /* DWARF 1 */
  .debug          0 : { *(.debug) }
  .line           0 : { *(.line) }
  /* GNU DWARF 1 extensions */
  .debug_srcinfo  0 : { *(.debug_srcinfo) }
  .debug_sfnames  0 : { *(.debug_sfnames) }
  /* DWARF 1.1 and DWARF 2 */
  .debug_aranges  0 : { *(.debug_aranges) }
  .debug_pubnames 0 : { *(.debug_pubnames) }
  /* DWARF 2 */
  .debug_info     0 : { *(.debug_info .gnu.linkonce.wi.*) }
  .debug_abbrev   0 : { *(.debug_abbrev) }
  .debug_line     0 : { *(.debug_line) }
  .debug_frame    0 : { *(.debug_frame) }
  .debug_str      0 : { *(.debug_str) }
  .debug_loc      0 : { *(.debug_loc) }
  .debug_macinfo  0 : { *(.debug_macinfo) }
  /* SGI/MIPS DWARF 2 extensions */
  .debug_weaknames 0 : { *(.debug_weaknames) }
  .debug_funcnames 0 : { *(.debug_funcnames) }
  .debug_typenames 0 : { *(.debug_typenames) }
  .debug_varnames  0 : { *(.debug_varnames) }
  .debug_pubtypes 0 : { *(.debug_pubtypes) }
  .debug_ranges   0 : { *(.debug_ranges) }
  .gnu.attributes 0 : { KEEP (*(.gnu.attributes)) }
}
