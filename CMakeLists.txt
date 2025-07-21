cmake_minimum_required(VERSION 3.16)

set(EXTRA_COMPONENT_DIRS
  "idf-replacements/newlib"
  "idf-replacements/esp_system"
  "idf-replacements/mbedtls"
  "sys"
  "pthread" 
  "lua"
  "libcrypt"
  "ramfs"
  "romfs"
  "spiffs"
  "lfs"
  "motion"
  "nmea"
  "gdisplay"
  "sdisplay"
  "sound"
  "lora"
  "openssl"
  "compat"
  "http"
  "captivedns"
  "mqtt"
  "telnet"
  "zlib"
)

#set(EXTRA_COMPONENT_DIRS "components")

include_directories("sys/newlib")
include_directories("main")
include_directories("sys")

#include_directories("components")
#include_directories("components/pthread")
#include_directories("components/sys")
#include_directories("components/lua/common")
#include_directories("components/lua/src")
#include_directories("components/compat/include")

# Do not use toolchain pthread definitions
#add_compile_options("-D_SYS_SCHED_H_")
#add_compile_options("-D_SYS__PTHREADTYPES_H_")
add_compile_options("-D_POSIX_THREADS")

#add_compile_options("-D__PTHREAD_h")
#add_compile_options("-D_SYS_SYSLIMITS_H_")

add_compile_options("-DINCLUDE_xTaskAbortDelay=1")
add_compile_options("-DNEWLIB_HAVE_SLEEP=0")
add_compile_options("-DNEWLIB_HAVE_USLEEP=0")

add_compile_options("-DCPU_HZ=240000000L")                   # CPU frequency in hertz
add_compile_options("-DCORE_TIMER_HZ=240000000L")            # CPU core timer frequency
add_compile_options("-D_CLOCKS_PER_SEC_=configTICK_RATE_HZ") # Number of interrupt ticks for reach 1 second

#add_compile_options("-DPATH_MAX=64")
#add_compile_options("-DMAXNAMLEN=64")
#add_compile_options("-D__FILENAME_MAX__=64")

add_compile_options("-DCONFIG_MAX_FD_BITS=4")

add_compile_options("-DLWIP_IPV6=1")
add_compile_options("-DLWIP_RAW=1")
#add_compile_options("-DLWIP_SO_LINGER=1")
#add_compile_options("-DLWIP_DNS=1")
#add_compile_options("-DLWIP_UDP=1")
#add_compile_options("-DLWIP_SOCKET=1")
#add_compile_options("-DLWIP_DNS_API_DEFINE_ERRORS=1")

add_compile_options("-DDEBUG_FREE_MEM=0")           # Enable LUA free mem debug utility (only for debug purposes)

add_compile_options("-DBUILD_TIME=0")

add_compile_options("-DLUA_RTOS_INCLUDE_LUA=1")
add_compile_options("-DLUA_RTOS_INCLUDE_IDF_REPLACEMENTS=1")
add_compile_options("-DBUILD_COMMIT=\"sss\"")

#if NEWLIB_HAVE_SLEEP == 0 || NEWLIB_HAVE_USLEEP == 0

add_compile_options("--warn-no-multistatement-macros")
	
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(Lua-RTOS)

include(cmake/part.cmake)
