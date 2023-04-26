#include <SynchronizedQueue.h>
#include <pthread.h>




//return 0 if success to create a new worker thread, -1 otherwise.
int worker_create(pthread_t * thread, queue * q, int * masterWorkerFlag, int * masterWorkerSocketPrio, int  socketFd, pthread_mutex_t * mutex, pthread_cond_t * cond);

//wait the worker to end, return 0 if success, -1 otherwise.
int worker_wait(pthread_t thread);