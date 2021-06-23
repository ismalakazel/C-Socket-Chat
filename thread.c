#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

/// Convenient way to handle error codes.
#define handle_error(message) \
		do { perror(message); exit(EXIT_FAILURE); } while (0) 


void thread(pthread_t thread_id, void *(*start_routine) (void *), void *args) {
	/// Thread attributes to be given to pthread_create. Remember, it should be destroyed after pthread_create.
	pthread_attr_t thread_attributes;
	int thread_attributes_result = pthread_attr_init(&thread_attributes);
	if (thread_attributes_result != 0) {
		handle_error("create attributes");	
	};

	/// Create new thread.
	int thread = pthread_create(&thread_id, &thread_attributes, start_routine, (void *) args);
	if (thread != 0) {
		handle_error("thread create");
	};
		
	/// Destroy attributes.
	pthread_attr_destroy(&thread_attributes);
}
