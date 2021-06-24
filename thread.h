#include <pthread.h>

void thread(pthread_t thread_id, void *(*start_routine) (void *), void *args);
