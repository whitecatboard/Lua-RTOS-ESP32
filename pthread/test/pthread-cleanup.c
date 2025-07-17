#include "unity.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include <sys/delay.h>

#define NUM_TERMS 4

static int term[NUM_TERMS];

void term_0(void *args) {
	term[0] = 0;
}

void term_1(void *args) {
	term[1] = 1;
}

void term_2(void *args) {
	term[2] = 2;
}

void term_3(void *args) {
	term[3] = 3;
}

// Test that handlers are called in reverse order
static void *thread1(void *args) {
	int count = 0;

	memset(term, -1, sizeof(term));

	pthread_cleanup_push(term_0, NULL);
	pthread_cleanup_push(term_1, NULL);
	pthread_cleanup_push(term_2, NULL);
	pthread_cleanup_push(term_3, NULL);

	for (count = 0; count < 100; count++) {
		// Simulate some work
		usleep(1000);
	}

	pthread_cleanup_pop(1);
	pthread_cleanup_pop(1);
	pthread_cleanup_pop(1);
	pthread_cleanup_pop(1);

	TEST_ASSERT(term[0] == 0);
	TEST_ASSERT(term[1] == 1);
	TEST_ASSERT(term[2] == 2);
	TEST_ASSERT(term[3] == 3);

	return NULL;
}

// Test that only handlers poped with execute argument set to true are
// executed
static void *thread2(void *args) {
	int count = 0;

	memset(term, -1, sizeof(term));

	pthread_cleanup_push(term_0, NULL);
	pthread_cleanup_push(term_1, NULL);
	pthread_cleanup_push(term_2, NULL);
	pthread_cleanup_push(term_3, NULL);

	for (count = 0; count < 100; count++) {
		// Simulate some work
		usleep(1000);
	}

	pthread_cleanup_pop(0);
	pthread_cleanup_pop(1);
	pthread_cleanup_pop(0);
	pthread_cleanup_pop(1);

	TEST_ASSERT(term[0] == 0);
	TEST_ASSERT(term[1] == -1);
	TEST_ASSERT(term[2] == 2);
	TEST_ASSERT(term[3] == -1);

	pthread_exit(NULL);
}

// Test that pthread_exit execute the cleanups
static void *thread3(void *args) {
	int count = 0;

	memset(term, -1, sizeof(term));

	pthread_cleanup_push(term_0, NULL);
	pthread_cleanup_push(term_1, NULL);
	pthread_cleanup_push(term_2, NULL);
	pthread_cleanup_push(term_3, NULL);

	for (count = 0; count < 100; count++) {
		// Simulate some work
		usleep(1000);

		if (count == 50) {
			pthread_exit(NULL);
		}
	}

	pthread_cleanup_pop(1);
	pthread_cleanup_pop(1);
	pthread_cleanup_pop(1);
	pthread_cleanup_pop(1);

	// This point is never reached
	TEST_ASSERT(0);

	pthread_exit(NULL);
}

// Test that when exit from thread with return don't execute the cleanups
static void *thread4(void *args) {
	int count = 0;

	memset(term, -1, sizeof(term));

	pthread_cleanup_push(term_0, NULL);
	pthread_cleanup_push(term_1, NULL);
	pthread_cleanup_push(term_2, NULL);
	pthread_cleanup_push(term_3, NULL);

	for (count = 0; count < 100; count++) {
		// Simulate some work
		usleep(1000);

		if (count == 50) {
			return NULL;
		}
	}

	pthread_cleanup_pop(1);
	pthread_cleanup_pop(1);
	pthread_cleanup_pop(1);
	pthread_cleanup_pop(1);

	// This point is never reached
	TEST_ASSERT(0);

	return NULL;
}

TEST_CASE("pthread cleanup", "[pthread]") {
	pthread_attr_t attr;
	pthread_t th;
	int ret;

	pthread_attr_init(&attr);

	// Test that handlers are called in reverse order
	ret = pthread_create(&th, &attr, thread1, NULL);
	TEST_ASSERT(ret == 0);

	ret = pthread_join(th, NULL);
	TEST_ASSERT(ret == 0);

	// Test that only handlers poped with execute argument set to true are
	// executed
	ret = pthread_create(&th, &attr, thread2, NULL);
	TEST_ASSERT(ret == 0);

	ret = pthread_join(th, NULL);
	TEST_ASSERT(ret == 0);

	// Test that pthread_exit execute the cleanups
	ret = pthread_create(&th, &attr, thread3, NULL);
	TEST_ASSERT(ret == 0);

	ret = pthread_join(th, NULL);
	TEST_ASSERT(ret == 0);

	TEST_ASSERT(term[0] == 0);
	TEST_ASSERT(term[1] == 1);
	TEST_ASSERT(term[2] == 2);
	TEST_ASSERT(term[3] == 3);

	// Test that when exit from thread with return don't execute the cleanups
	ret = pthread_create(&th, &attr, thread4, NULL);
	TEST_ASSERT(ret == 0);

	ret = pthread_join(th, NULL);
	TEST_ASSERT(ret == 0);

	TEST_ASSERT(term[0] == -1);
	TEST_ASSERT(term[1] == -1);
	TEST_ASSERT(term[2] == -1);
	TEST_ASSERT(term[3] == -1);

	ret = pthread_attr_destroy(&attr);
	TEST_ASSERT(ret == 0);
}
