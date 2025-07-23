set(srcs
	"common/blocks.c"
	"common/cache.c"
	"common/hex.c"
	"common/linenoise.c"
	"common/lrotable.c"
	"common/shell.c"
	"common/sys.c"
#	"modules/bluetooth/bluetooth.c"
	"modules/hw/adc.c"
	"modules/hw/can.c"
	"modules/hw/cpu.c"
	"modules/hw/encoder.c"
	"modules/hw/gdisplay.c"
	"modules/hw/i2c.c"
	"modules/hw/neopixel.c"
	"modules/hw/pio.c"
 	"modules/hw/pwm.c"
	"modules/hw/rcswitch.c"
	"modules/hw/rtc.c"
	"modules/hw/sdisplay.c"
	"modules/hw/sensor.c"
	"modules/hw/servo.c"
	"modules/hw/spi.c"
#	"modules/hw/stepper.c"
	"modules/hw/touch.c"
	"modules/hw/uart.c"
#	"modules/hw/ulp.c"
	"modules/lua-cjson/fpconv.c"
	"modules/lua-cjson/lua_cjson.c"
	"modules/lua-cjson/strbuf.c"
#	"modules/luasocket/auxiliar.c"
#	"modules/luasocket/buffer.c"
#	"modules/luasocket/compat.c"
#	"modules/luasocket/except.c"
#	"modules/luasocket/inet.c"
#	"modules/luasocket/io.c"
#	"modules/luasocket/luasocket.c"
#	"modules/luasocket/mime.c"
#	"modules/luasocket/options.c"
#	"modules/luasocket/select.c"
#	"modules/luasocket/serial.c"
#	"modules/luasocket/tcp.c"
#	"modules/luasocket/timeout.c"
#	"modules/luasocket/udp.c"
#	"modules/luasocket/unix.c"
#	"modules/luasocket/unixdgram.c"
#	"modules/luasocket/unixstream.c"
#	"modules/luasocket/usocket.c"
	"modules/middleware/lora.c"
	"modules/middleware/mqtt.c"
	"modules/misc/lsound.c"
	"modules/net/net.c"
	"modules/sys/crc.c"
	"modules/sys/error.c"
	"modules/sys/event.c"
	"modules/sys/fs.c"
	"modules/sys/lpack.c"
	"modules/sys/md5.c"
	"modules/sys/nvs.c"
	"modules/sys/thread.c"
	"modules/sys/tmr.c"
	"modules/sys/vm.c"
	"src/lapi.c"
	"src/lauxlib.c"
	"src/lbaselib.c"
	"src/lbitlib.c"
	"src/lcode.c"
	"src/lcorolib.c"
	"src/lctype.c"
	"src/ldblib.c"
	"src/ldebug.c"
	"src/ldo.c"
	"src/ldump.c"
	"src/lfunc.c"
	"src/lgc.c"
	"src/linit.c"
	"src/liolib.c"
	"src/llex.c"
	"src/lmathlib.c"
	"src/lmem.c"
	"src/loadlib.c"
	"src/lobject.c"
	"src/lopcodes.c"
	"src/loslib.c"
	"src/lparser.c"
	"src/lstate.c"
	"src/lstring.c"
	"src/lstrlib.c"
	"src/ltable.c"
	"src/ltablib.c"
	"src/ltm.c"
	"src/lua.c"
	"src/luac.c"
	"src/lundump.c"
	"src/lutf8lib.c"
	"src/lvm.c"
	"src/lzio.c"
)

set(include_dirs
	"common" 
	"src"
	"modules/sys"
)

set(fragments
    "iram.lf"
)	   

idf_component_register(
	SRCS "${srcs}"
	INCLUDE_DIRS "${include_dirs}"
	LDFRAGMENTS "${fragments}"
	PRIV_REQUIRES bt wpa_supplicant ulp app_update pthread libcrypt vfs nvs_flash lora sound sys mqtt esp_rom rc-switch esp_hw_support
)

# Force linker to link lua modules enabled in build
set(extra_link_flags "-u luaopen_adc")
list(APPEND extra_link_flags "-u luaopen_can")
list(APPEND extra_link_flags "-u luaopen_cpu")
list(APPEND extra_link_flags "-u luaopen_encoder")
list(APPEND extra_link_flags "-u luaopen_gdisplay")
list(APPEND extra_link_flags "-u luaopen_i2c")
list(APPEND extra_link_flags "-u luaopen_neopixel")
list(APPEND extra_link_flags "-u luaopen_pio")
list(APPEND extra_link_flags "-u luaopen_pwm")
list(APPEND extra_link_flags "-u luaopen_rcswitch")
list(APPEND extra_link_flags "-u luaopen_rtc")
list(APPEND extra_link_flags "-u luaopen_sdisplay")
list(APPEND extra_link_flags "-u luaopen_sensor")
list(APPEND extra_link_flags "-u luaopen_servo")
list(APPEND extra_link_flags "-u luaopen_spi")
#list(APPEND extra_link_flags "-u luaopen_stepper")
list(APPEND extra_link_flags "-u luaopen_touch")
list(APPEND extra_link_flags "-u luaopen_uart")
#list(APPEND extra_link_flags "-u luaopen_ulp")
#list(APPEND extra_link_flags "-u luaopen_bt")
list(APPEND extra_link_flags "-u luaopen_lora")
list(APPEND extra_link_flags "-u luaopen_mqtt")
list(APPEND extra_link_flags "-u luaopen_sound")
list(APPEND extra_link_flags "-u luaopen_net")
list(APPEND extra_link_flags "-u luaopen_crc")
list(APPEND extra_link_flags "-u luaopen_event")
list(APPEND extra_link_flags "-u luaopen_fs")
list(APPEND extra_link_flags "-u luaopen_pack")
list(APPEND extra_link_flags "-u luaopen_md5")
list(APPEND extra_link_flags "-u luaopen_nvs")
list(APPEND extra_link_flags "-u luaopen_thread")
list(APPEND extra_link_flags "-u luaopen_tmr")
list(APPEND extra_link_flags "-u luaopen_vm")

list(APPEND extra_link_flags "-u luaopen_coroutine")
list(APPEND extra_link_flags "-u luaopen_debug")
list(APPEND extra_link_flags "-u luaopen_math")
list(APPEND extra_link_flags "-u luaopen_os")
list(APPEND extra_link_flags "-u luaopen_string")
list(APPEND extra_link_flags "-u luaopen_table")
list(APPEND extra_link_flags "-u luaopen_utf8")

target_link_libraries(${COMPONENT_LIB} INTERFACE "${extra_link_flags}")

target_compile_options(${COMPONENT_LIB} PRIVATE -DSOCKET_SELECT)