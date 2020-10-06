#include "unity.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <pthread.h>

#include <sys/delay.h>

#define NUM_THREADS  2

static pthread_t threads[NUM_THREADS];

static void *thread1(void *args) {
	int count = 0;

	for (count = 0; count < 100; count++) {
		// Simulate some work
		usleep(1000);
	}

	int *ret = malloc(sizeof(int));
	*ret = count;

	pthread_exit(ret);
 }

static void *thread2(void *args) {
	int count;

	for (count = 0; count < 50; count++) {
		// Simulate some work
		usleep(1000);
	}

	int *ret = malloc(sizeof(int));
	*ret = count;

	pthread_exit(ret);
 }


TEST_CASE("pthread join", "[pthread]") {
	pthread_attr_t attr;
	int i, ret;
	int *res;

	ret = pthread_attr_init(&attr);
	TEST_ASSERT(ret == 0);

	ret = pthread_create(&threads[0], &attr, thread1, NULL);
	TEST_ASSERT(ret == 0);

	ret = pthread_create(&threads[1], &attr, thread2, NULL);
	TEST_ASSERT(ret == 0);

	// Wait for all threads completion
	for (i=0; i< NUM_THREADS; i++) {
		ret = pthread_join(threads[i], (void **)&res);
		TEST_ASSERT(ret == 0);

		if (i == 0) {
			TEST_ASSERT(*res == 100);
		} else if (i == 1) {
			TEST_ASSERT(*res == 50);
		}

		free(res);
	}

	// Check that a detached thread is not joinable
	ret = pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
	TEST_ASSERT(ret == 0);

	ret = pthread_create(&threads[0], &attr, thread1, NULL);
	TEST_ASSERT(ret == 0);

	ret = pthread_join(threads[0], (void **)&res);
	TEST_ASSERT(ret == EINVAL);

	// Clean up
	ret = pthread_attr_destroy(&attr);
	TEST_ASSERT(ret == 0);
}
