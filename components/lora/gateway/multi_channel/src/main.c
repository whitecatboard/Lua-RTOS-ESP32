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

    if (pthread_create(&thread, &attr, lora_gw, NULL)) {
    	return;
	}
}
