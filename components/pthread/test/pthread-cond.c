#include "unity.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <pthread.h>

#include <sys/delay.h>

#define NUM_THREADS  3
#define COUNT_LIMIT 12
#define TCOUNT 	    10

static int count = 0;
static pthread_t threads[NUM_THREADS];
static pthread_mutex_t count_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t count_threshold_cv = PTHREAD_COND_INITIALIZER;

static void *inc_count(void *args) {
	int i;
	int ret;

	for (i=0; i< TCOUNT; i++) {
		ret = pthread_mutex_lock(&count_mutex);
		TEST_ASSERT(ret == 0);

		count++;

		// If condition is reached signal condition
		if (count == COUNT_LIMIT) {
			ret = pthread_cond_signal(&count_threshold_cv);
			TEST_ASSERT(ret == 0);
		}

		ret = pthread_mutex_unlock(&count_mutex);
		TEST_ASSERT(ret == 0);

		// Simulate some work
		usleep(1000);
	}

	return NULL;
 }

static void *watch_count(void *args) {
	int ret;

	ret = pthread_mutex_lock(&count_mutex);
	TEST_ASSERT(ret == 0);

	while (count < COUNT_LIMIT) {
		ret = pthread_cond_wait(&count_threshold_cv, &count_mutex);
		TEST_ASSERT(ret == 0);
	}

	count += 125;
	TEST_ASSERT (count == 125 + COUNT_LIMIT);

	ret = pthread_mutex_unlock(&count_mutex);
	TEST_ASSERT(ret == 0);

	return NULL;
}

TEST_CASE("pthread conditions", "[pthread]") {
	pthread_attr_t attr;
	int i, ret;

	// Initialize mutex and condition variable objects
	ret = pthread_mutex_init(&count_mutex, NULL);
	TEST_ASSERT(ret == 0);

	ret = pthread_cond_init(&count_threshold_cv, NULL);
	TEST_ASSERT(ret == 0);

	ret = pthread_attr_init(&attr);
	TEST_ASSERT(ret == 0);

	ret = pthread_attr_setstacksize(&attr, 10240);
	TEST_ASSERT(ret == 0);

	ret = pthread_create(&threads[0], &attr, watch_count, NULL);
	TEST_ASSERT(ret == 0);

	ret = pthread_create(&threads[1], &attr, inc_count, NULL);
	TEST_ASSERT(ret == 0);

	ret = pthread_create(&threads[2], &attr, inc_count, NULL);
	TEST_ASSERT(ret == 0);

	// Wait for all threads completion
	for (i=0; i< NUM_THREADS; i++) {
		ret = pthread_join(threads[i], NULL);
		TEST_ASSERT(ret == 0);
	}

	// Clean up
	ret = pthread_attr_destroy(&attr);
	TEST_ASSERT(ret == 0);

	ret = pthread_mutex_destroy(&count_mutex);
	TEST_ASSERT(ret == 0);

	ret = pthread_cond_destroy(&count_threshold_cv);
	TEST_ASSERT(ret == 0);
}
