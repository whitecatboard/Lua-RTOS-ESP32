# args for passing into compile rule generation
pthread_SRC_DIR = $(pthread_ROOT)

$(eval $(call component_compile_rules,pthread))
