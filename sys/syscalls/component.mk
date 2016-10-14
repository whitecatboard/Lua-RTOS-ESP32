INC_DIRS += $(syscalls_ROOT)..
CFLAGS += -DKERNEL

# args for passing into compile rule generation
syscalls_SRC_DIR = $(syscalls_ROOT)
syscalls_INC_DIR = $(ROOT) $(syscalls_ROOT)

$(eval $(call component_compile_rules,syscalls))
