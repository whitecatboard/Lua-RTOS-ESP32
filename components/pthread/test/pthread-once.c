#include "unity.h"

#include <pthread.h>
#include <sys/delay.h>

#define NUM_THREADS  2

static pthread_t threads[NUM_THREADS];

static pthread_once_t once = PTHREAD_ONCE_INIT;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static int execs;

static void init() {
	int ret;

	ret = pthread_mutex_lock(&mutex);
	TEST_ASSERT(ret == 0);

	execs++;

	ret = pthread_mutex_unlock(&mutex);
	TEST_ASSERT(ret == 0);
}

static void *thread1(void *args) {
	int ret;

	ret = pthread_once(&once, init);
	TEST_ASSERT(ret == 0);

	// Simulate some work
	usleep(1000);

	pthread_exit(NULL);
 }

TEST_CASE("pthread once", "[pthread]") {
	pthread_attr_t attr;
	int i, ret;

	ret = pthread_attr_init(&attr);
	TEST_ASSERT(ret == 0);

	ret = pthread_mutex_init(&mutex, NULL);
	TEST_ASSERT(ret == 0);

	ret = pthread_create(&threads[0], &attr, thread1, NULL);
	TEST_ASSERT(ret == 0);

	ret = pthread_create(&threads[1], &attr, thread1, NULL);
	TEST_ASSERT(ret == 0);

	// Wait for all threads completion
	for (i=0; i< NUM_THREADS; i++) {
		ret = pthread_join(threads[i], NULL);
		TEST_ASSERT(ret == 0);
	}

	TEST_ASSERT(execs == 1);

	// Clean up
	ret = pthread_attr_destroy(&attr);
	TEST_ASSERT(ret == 0);

	ret = pthread_mutex_destroy(&mutex);
	TEST_ASSERT(ret == 0);
}
