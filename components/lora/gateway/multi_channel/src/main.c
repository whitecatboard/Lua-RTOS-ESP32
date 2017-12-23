#include <pthread.h>

int lora_pkt_fwd(void);

static void *lora_gw(void *arg) {
	lora_pkt_fwd();

	return NULL;
}

void lora_gw_start() {
	pthread_t thread;
	pthread_attr_t attr;

	pthread_attr_init(&attr);

	// Set stack size
    pthread_attr_setstacksize(&attr, CONFIG_LUA_RTOS_LUA_STACK_SIZE);

    if (pthread_create(&thread, &attr, lora_gw, NULL)) {
    	return;
	}

    pthread_setname_np(thread, "lora");
}
