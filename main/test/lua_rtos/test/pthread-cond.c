#include "unity.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <pthread/pthread.h>

#include <sys/delay.h>

#define NUM_THREADS  3
#define COUNT_LIMIT 12
#define TCOUNT 	    10

static int count = 0;
static pthread_t threads[NUM_THREADS];
static pthread_mutex_t count_mutex;
static pthread_cond_t count_threshold_cv;
static int running = 0;

static void *inc_count(void *args) {
	int i;

	pthread_mutex_lock(&count_mutex);
	running++;
	pthread_mutex_unlock(&count_mutex);

	for (i=0; i< TCOUNT; i++) {
		pthread_mutex_lock(&count_mutex);

		count++;

		// If condition is reached signal condition
		if (count == COUNT_LIMIT) {
			pthread_cond_signal(&count_threshold_cv);
		}

		pthread_mutex_unlock(&count_mutex);

		// Simulate some work
		udelay(1000);
	}

	pthread_mutex_lock(&count_mutex);
	running--;
	pthread_mutex_unlock(&count_mutex);

	pthread_exit(NULL);
 }

static void *watch_count(void *args) {
	// Lock mutex for signal
	pthread_mutex_lock(&count_mutex);
	running++;

	while (count < COUNT_LIMIT) {
		pthread_cond_wait(&count_threshold_cv, &count_mutex);
		count += 125;
		TEST_ASSERT (count == 125 + COUNT_LIMIT);
	}

	running--;
	pthread_mutex_unlock(&count_mutex);

	pthread_exit(NULL);
}

TEST_CASE("pthread conditions", "[pthread]") {
	pthread_attr_t attr;
	int i, ret;

	running = 0;

	// Initialize mutex and condition variable objects
	pthread_mutex_init(&count_mutex, NULL);
	pthread_cond_init (&count_threshold_cv, NULL);

	pthread_attr_init(&attr);
	ret = pthread_create(&threads[0], &attr, watch_count, NULL);
	TEST_ASSERT(ret == 0);

	ret = pthread_create(&threads[1], &attr, inc_count, NULL);
	TEST_ASSERT(ret == 0);

	ret = pthread_create(&threads[2], &attr, inc_count, NULL);
	TEST_ASSERT(ret == 0);

	// Wait for all threads to complete
	for (i=0; i< NUM_THREADS; i++) {
		pthread_join(threads[i], NULL);
	}

	TEST_ASSERT(running == 0);

	// Clean up
	pthread_attr_destroy(&attr);
	pthread_mutex_destroy(&count_mutex);
	pthread_cond_destroy(&count_threshold_cv);
}
