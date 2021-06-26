#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

/// Convenient way to handle error codes.
#define handle_error(message) \
		do { perror(message); exit(EXIT_FAILURE); } while (0) 


/// Creates a thread that runs a user specified function.
/// - Parameter t
void thread(pthread_t tid, void *(*f) (void *), void **args) {
	
	/// Thread attributes are given to pthread_create. Must be destroyed after pthread_create.
	pthread_attr_t attributes;
	pthread_attr_init(&attributes);

	/// Create new thread.
	int thread = pthread_create(&tid, &attributes, *f, (void *) *args);
		
	/// Destroy attributes.
	pthread_attr_destroy(&attributes);
}
