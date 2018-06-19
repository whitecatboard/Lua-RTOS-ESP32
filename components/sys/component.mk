#
# Component Makefile
#

ifdef TEST_COMPONENTS
TEST_LIBS := $(addprefix -l,$(addsuffix _test,$(TEST_COMPONENTS)))
else
TEST_LIBS := 
endif

COMPONENT_ADD_LDFLAGS = -Wl,--whole-archive -l$(COMPONENT_NAME) -llua -lluasocket -llua-cjson $(TEST_LIBS) -Wl,--no-whole-archive -L$(COMPONENT_PATH)/ld -T lua_rtos.ld

COMPONENT_SRCDIRS := . luartos_build.h freertos vfs editor sys unix syscalls math drivers lmic sensors \
					   sys/machine lwip/netif \
					   lwip shell misc

COMPONENT_ADD_INCLUDEDIRS := . .. ./lwip/include misc
							   
COMPONENT_PRIV_INCLUDEDIRS :=
