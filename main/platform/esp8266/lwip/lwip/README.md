# esp-lwip

LwIP library for ESP8266. This work is based on the LwIP code drop done
by Espressif in their SDK 0.9.4. Espressif specific changes where
reviewed, split in small, clean patches and applied on top of official
LwIP git repository.

The Espressif original code was based on LwIP 1.4.0 with some
cherry-picks so initial work for this project was also based on 1.4.0.
After confirming that it's working, all the changes were rebased on
1.4.1.

This project does not contain complete LwIP stack. The code for network
driver was not released by Espressif. Currently we only provide
replacement for `eagle_lwip_if.o` from `libmain.a`. This module is
responsible for setting up `struct netif` and calling `netif_add()` from
LwIP.

## Status

I've done some limited testing of this library - wifi works both in
client and in AP mode and I had no problem with running esphttpd
project. There is a need for more testing, of course. If you find any
bug, please create an Issue or PR. Feel free to contact me by e-mail if
you have any questions or suggestions.

## Building

1. Clone the repo.
2. Make sure your gcc toolchain is in your PATH (I'm using
[esp-open-sdk](https://github.com/pfalcon/esp-open-sdk))
3. Call `make`.
4. Put `liblwip.a` in your libraries path.

## Changes compared to Espressif release

 - The code is based on LwIP 1.4.1.
 - All the options were moved to config/lwipopts.h file which was
   cleaned and simplified.
 - The LwIP code is not marked with `ICACHE_FLASH_ATTR` but all the
   functions are moved to .irom0.text section using objcopy instead.
 - Espressif changes to LwIP code was marked by LWIP_ESP macro.
 - All Espressif specific code (dhcpserver, ping, espconn) are put in
   separate directory
 - Small fixes.

## Features added

 - `ESP_TIMEWAIT_THRESHOLD` - available heap memory is checked on each TCP
   connection accepted. If it's below this threshold, memory is
   reclaimed by killing TCP connections in TIME-WAIT state.
 - Our own `eagle_lwip_if.o`. It should do exactly what the original
   version does but is compiled from our code.
 - `LWIP_NETIF_HOSTNAME_PREFIX` - if `LWIP_NETIF_HOSTNAME` is enabled,
   the device hostname that is raported to DHCP server will be set to
   `LWIP_NETIF_HOSTNAME_PREFIX` with hexadecimal representation of 3
   least significant bytes of MAC address concatenated. This requires
   using our version of `eagle_lwip_if.o`.

## eagle_lwip_if.o story

This object contains `eagle_lwip_if_alloc()` function which is called by
SDK when network interfaces are created. It is responsible for
allocating memory for `struct netif` and calling `netif_add()`. We need
it because some of LwIP features, when enabled, will add additional
fields to `netif` structure, changing its size and breaking SDK
compatibility (Espressif version of `eagle_lwip_if.o` is compiled with
their version of netif struct and will always allocate 56 bytes for it).

If you want to use it, you have to modify your `libmain.a` and either
remove `eagle_lwip_if.o` from it and add our own version to `liblwip.a`
(see `USE_OUR_LWIP_IF` in `Makefile-local.mk`) or replace it with our
own version (see `LIBMAIN_PATH` in `Makefile-local.mk`).

Note that using or version of this module is not enough to freely change
`netif` structure. It seems that it's used by some other parts of SDK
too so we have to ensure that all the fields used by SDK are on expected
offsets. We can freely add new fields at the end of this structure,
though.

## Makefile-local.mk

Use this file to create all the local modifications required to compile
this library in your environment. There are also some options you can
set there, see inline comments.
