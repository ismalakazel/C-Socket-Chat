#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

void thread(pthread_t thread_id, void *(*start_routine) (void *), void *args);
