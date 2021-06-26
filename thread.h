#include <pthread.h>

void thread(pthread_t tid, void *(*f) (void *), void **args);
