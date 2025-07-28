set(srcs
	"drivers/MCP23S17.c"
	"drivers/adc.c"
	"drivers/adc_ads1015.c"
	"drivers/adc_ads1115.c"
	"drivers/adc_internal.c"
	"drivers/adc_mcp3008.c"
	"drivers/adc_mcp3208.c"
#	"drivers/bluetooth.c"
#	"drivers/bluetooth_eddystone.c"
#	"drivers/bluetooth_hci.c"
	"drivers/can.c"
	"drivers/clock.c"
	"drivers/cpu.c"
	"drivers/dac.c"
	"drivers/encoder.c"
	"drivers/eth.c"
	"drivers/dgdisplay.c"
	"drivers/gpio.c"
	"drivers/gpio_debouncing.c"
	"drivers/ht16k33.c"
	"drivers/i2c.c"
	"drivers/ili9341.c"
	"drivers/neopixel.c"
	"drivers/net.c"
	"drivers/net_http.c"
	"drivers/nzr.c"
	"drivers/owire.c"
	"drivers/pca9xxx.c"
	"drivers/pcd8544.c"
	"drivers/power_bus.c"
	"drivers/pwm.c"
	"drivers/rmt.c"
	"drivers/rtc.c"
	"drivers/sensor.c"
	"drivers/servo.c"
	"drivers/spi.c"
#	"drivers/spi_eth.c"
	"drivers/ssd1306.c"
	"drivers/st7735.c"
#	"drivers/stepper.c"
	"drivers/system.c"
	"drivers/timer.c"
#	"drivers/tm1637.c"
	"drivers/uart.c"
	"drivers/wifi.c"
	"editor/edit.c"
	"freertos/adds.c"
#	"lwip/netif/spiethernetif.c"
#	"lwip/netif/tunif.c"
#	"lwip/netif/vfs_tun.c"
	"lwip/ping.c"
	"math/e_remainder.c"
	"math/ef_remainder.c"
	"misc/hex_string.c"
	"sensors/2y0a21.c"
	"sensors/BDE0600G.c"
	"sensors/BH1620FVC.c"
	"sensors/BH1721FVC.c"
	"sensors/GUVA-S12SD.c"
	"sensors/ML8511.c"
	"sensors/MPXV6115V.c"
	"sensors/a49e.c"
	"sensors/analog_joystick.c"
	"sensors/bme280.c"
	"sensors/dht11.c"
	"sensors/dht22.c"
	"sensors/dht23.c"
	"sensors/dhtxx.c"
	"sensors/ds1820.c"
	"sensors/flow_meter.c"
	"sensors/gps.c"
	"sensors/hall_flow_sensor.c"
#	"sensors/hall_internal.c"
	"sensors/hall_switch.c"
	"sensors/key_matrix_4_4.c"
	"sensors/ldr.c"
	"sensors/linear_potentiometer.c"
	"sensors/lm35.c"
	"sensors/ping28015.c"
	"sensors/pir_am412.c"
	"sensors/push_switch.c"
	"sensors/relative_rotary_encoder.c"
	"sensors/sds011.c"
	"sensors/thermistor.c"
	"sensors/tilt_switch.c"
	"sensors/tmp36.c"
	"sensors/toggle_switch_2p.c"
	"sensors/toggle_switch_3p.c"
	"sensors/us015.c"
	"sensors/vh400.c"
#	"shell/shell.c"
	"sys/console.c"
	"sys/delay.c"
	"sys/driver.c"
	"sys/history.c"
	"sys/list.c"
	"sys/memory.c"
	"sys/mount.c"
	"sys/mutex.c"
	"sys/nanosleep.c"
	"sys/panic.c"
	"sys/path.c"
	"sys/signal.c"
	"sys/status.c"
	"sys/sys_init.c"
	"sys/tail.c"
	"syscalls/__wrap__access.c"
	"syscalls/__wrap__calloc_r.c"
	"syscalls/__wrap__malloc_r.c"
	"syscalls/__wrap__open_r.c"
	"syscalls/__wrap__realloc_r.c"
	"syscalls/__wrap__rename_r.c"
	"syscalls/__wrap__stat_r.c"
	"syscalls/__wrap__unlink_r.c"
	"syscalls/__wrap_mkdir.c"
	"syscalls/__wrap_opendir.c"
	"syscalls/__wrap_rmdir.c"
	"syscalls/chdir.c"
	"syscalls/common.c"
	"unix/getcwd.c"
	"unix/syslog.c"
#	"vfs/fat.c"
	"vfs/lfs.c"
#	"vfs/pty.c"
	"vfs/ramfs.c"
	"vfs/romfs.c"
	"vfs/spiffs.c"
	"vfs/tty.c"
#	"vfs/urandom.c"
	"vfs/vfs.c"
)

set(include_dirs 
    "."
	"freertos"
	"vfs"
	"editor"
	"sys"
#	"unix"
	"syscalls"
	"math"
	"drivers"
	"sensors"
	"lwip/netif"
	"lwip"
#	"shell"
	"misc"
)
				
set(fragments
    "iram.lf"
)	   

idf_component_register(
	SRCS "${srcs}"
    INCLUDE_DIRS "${include_dirs}"
	LDFRAGMENTS "${fragments}"
    PRIV_REQUIRES app_update driver nvs_flash pthread vfs
    REQUIRES lua esp_adc spiffs ramfs romfs lfs motion nmea gdisplay driver esp_hw_support esp_wifi esp_eth openssl eth_enc424j600
)

#PRIV_REQUIRES driver bt esp_adc_cal app_update openssl mbedtls wpa_supplicant nmea 

set_property(TARGET ${COMPONENT_LIB} APPEND PROPERTY INTERFACE_LINK_LIBRARIES "-Wl,--wrap=_realloc_r")
set_property(TARGET ${COMPONENT_LIB} APPEND PROPERTY INTERFACE_LINK_LIBRARIES "-Wl,--wrap=_malloc_r")
set_property(TARGET ${COMPONENT_LIB} APPEND PROPERTY INTERFACE_LINK_LIBRARIES "-Wl,--wrap=_calloc_r")
set_property(TARGET ${COMPONENT_LIB} APPEND PROPERTY INTERFACE_LINK_LIBRARIES "-Wl,--wrap=_open_r")
set_property(TARGET ${COMPONENT_LIB} APPEND PROPERTY INTERFACE_LINK_LIBRARIES "-Wl,--wrap=_unlink_r")
set_property(TARGET ${COMPONENT_LIB} APPEND PROPERTY INTERFACE_LINK_LIBRARIES "-Wl,--wrap=_rename_r")
set_property(TARGET ${COMPONENT_LIB} APPEND PROPERTY INTERFACE_LINK_LIBRARIES "-Wl,--wrap=_stat_r")
set_property(TARGET ${COMPONENT_LIB} APPEND PROPERTY INTERFACE_LINK_LIBRARIES "-Wl,--wrap=mkdir")
set_property(TARGET ${COMPONENT_LIB} APPEND PROPERTY INTERFACE_LINK_LIBRARIES "-Wl,--wrap=opendir")
set_property(TARGET ${COMPONENT_LIB} APPEND PROPERTY INTERFACE_LINK_LIBRARIES "-Wl,--wrap=rmdir")
set_property(TARGET ${COMPONENT_LIB} APPEND PROPERTY INTERFACE_LINK_LIBRARIES "-Wl,--wrap=access")

#set_property(TARGET ${COMPONENT_LIB} APPEND PROPERTY INTERFACE_LINK_LIBRARIES "-Wl,--no-whole-archive -L ${COMPONENT_DIR}/ld -T lua_rtos.ld")

# Force linker to link wrap functions
set(extra_link_flags "-u _realloc_r")
list(APPEND extra_link_flags "-u _malloc_r")
list(APPEND extra_link_flags "-u _calloc_r")
list(APPEND extra_link_flags "-u _open_r")
list(APPEND extra_link_flags "-u _unlink_r")
list(APPEND extra_link_flags "-u _rename_r")
list(APPEND extra_link_flags "-u _stat_r")
list(APPEND extra_link_flags "-u mkdir")
list(APPEND extra_link_flags "-u opendir")
list(APPEND extra_link_flags "-u rmdir")
list(APPEND extra_link_flags "-u access")
list(APPEND extra_link_flags "-u __wrap__stat_r")
list(APPEND extra_link_flags "-u __wrap__unlink_r")
list(APPEND extra_link_flags "-u __wrap__rename_r")

# Force linker to link drivers. It is required to use one of the public function of the driver
list(APPEND extra_link_flags "-u adc_setup")
#list(APPEND extra_link_flags "-u bt_setup")
list(APPEND extra_link_flags "-u can_setup")
list(APPEND extra_link_flags "-u dac_tone_setup")
list(APPEND extra_link_flags "-u encoder_setup")
list(APPEND extra_link_flags "-u system_setup")
list(APPEND extra_link_flags "-u eth_setup")
list(APPEND extra_link_flags "-u gpio_pin_output")
list(APPEND extra_link_flags "-u neopixel_setup")
list(APPEND extra_link_flags "-u net_init")
list(APPEND extra_link_flags "-u nzr_setup")
list(APPEND extra_link_flags "-u owire_init")
list(APPEND extra_link_flags "-u pwbus_on")
list(APPEND extra_link_flags "-u pwm_setup")
#list(APPEND extra_link_flags "-u rmt_setup_rx")
list(APPEND extra_link_flags "-u rtc_mem_size")
list(APPEND extra_link_flags "-u sensor_setup")
list(APPEND extra_link_flags "-u servo_setup")
#list(APPEND extra_link_flags "-u spi_eth_setup")
list(APPEND extra_link_flags "-u spi_setup")
#list(APPEND extra_link_flags "-u stepper_setup")
list(APPEND extra_link_flags "-u system_init")
list(APPEND extra_link_flags "-u tmr_init")
list(APPEND extra_link_flags "-u uart_init")
list(APPEND extra_link_flags "-u wifi_setup")
#list(APPEND extra_link_flags "-u i2c_attach")
list(APPEND extra_link_flags "-u gdisplay_init")
#list(APPEND extra_link_flags "-u lora_setup")
#list(APPEND extra_link_flags "-u cpu_port_number")
#list(APPEND extra_link_flags "-u luaopen_touch")
#list(APPEND extra_link_flags "-u luaopen_ulp")
#list(APPEND extra_link_flags "-u luaopen_mqtt")
#list(APPEND extra_link_flags "-u http_init")
#list(APPEND extra_link_flags "-u mdns_init")
#list(APPEND extra_link_flags "-u event_init")
#list(APPEND extra_link_flags "-u thread_init")
#list(APPEND extra_link_flags "-u rcswitch_listen")
list(APPEND extra_link_flags "-u sdisplay_setup")
#list(APPEND extra_link_flags "-u rcswitch_listen")
#list(APPEND extra_link_flags "-u sound_music_time_signature")

# Force linker to link sensors. It is required to use one of the public function of the sensor
list(APPEND extra_link_flags "-u s2y0a21_setup")
list(APPEND extra_link_flags "-u a49e_acquire")
list(APPEND extra_link_flags "-u analog_joystick_setup")
list(APPEND extra_link_flags "-u BDE0600G_presetup")
list(APPEND extra_link_flags "-u BH1620FVC_presetup")
list(APPEND extra_link_flags "-u BH1721FVC_presetup")
list(APPEND extra_link_flags "-u bme280_presetup")
list(APPEND extra_link_flags "-u dht11_acquire")
list(APPEND extra_link_flags "-u dht22_acquire")
list(APPEND extra_link_flags "-u dht23_acquire")
list(APPEND extra_link_flags "-u ds1820_setup")
list(APPEND extra_link_flags "-u gps_setup")
list(APPEND extra_link_flags "-u guva_s12sd_presetup")
list(APPEND extra_link_flags "-u hall_flow_setup")
#list(APPEND extra_link_flags "-u internal_hall_setup")
list(APPEND extra_link_flags "-u _hall_switch_setup")
list(APPEND extra_link_flags "-u key_matrix_4_4_setup")
list(APPEND extra_link_flags "-u ldr_setup")
list(APPEND extra_link_flags "-u potentiometer_acquire")
list(APPEND extra_link_flags "-u lm35_presetup")
list(APPEND extra_link_flags "-u ML8511_presetup")
list(APPEND extra_link_flags "-u MPXV6115V_presetup")
list(APPEND extra_link_flags "-u ping28015_setup")
list(APPEND extra_link_flags "-u _push_switch_setup")
list(APPEND extra_link_flags "-u relative_rotary_encoder_setup")
list(APPEND extra_link_flags "-u sds011_presetup")
list(APPEND extra_link_flags "-u thermistor_setup")
list(APPEND extra_link_flags "-u _tilt_switch_setup")
list(APPEND extra_link_flags "-u tmp36_setup")
list(APPEND extra_link_flags "-u _2_pos_switch_setup")
list(APPEND extra_link_flags "-u _3_pos_switch_setup")
list(APPEND extra_link_flags "-u us015_setup")
list(APPEND extra_link_flags "-u vh400_presetup")


target_link_libraries(${COMPONENT_LIB} INTERFACE "${extra_link_flags}")

# Enable / disable LUA free mem debug utility (only for debug purposes)
target_compile_options(${COMPONENT_LIB} PUBLIC -DDEBUG_FREE_MEM=0)