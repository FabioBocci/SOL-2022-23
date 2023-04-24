#include <pthread.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <SynchronizedQueue.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <MemoryManager.h>

#undef M_DEBUG
#define M_DEBUG 0

#define GET_QUEUE_SIZE(q) (q -> arraySize)
#define GET_PUSH_INDEX(q) ((q -> indexPush) %  GET_QUEUE_SIZE(q))
#define GET_POP_INDEX(q) ((q -> indexPop) % GET_QUEUE_SIZE(q))
#define IS_EMPTY(q) ((q -> count) == 0)
#define CAN_PUSH(q) ((q -> count) < GET_QUEUE_SIZE(q))

queue* q_init(int size) {
	queue * q;
	MM_MALLOC(q, queue);
	MM_MALLOC_N(q->arr, element, size);
	q->arraySize = size;
	q->indexPush = size;
	q->indexPop = 0;
	q->count = 0;

	pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->cond_push, NULL);
    pthread_cond_init(&q->cond_pop, NULL);

	return q;
}


int q_push(queue* q, const char* fileNamePush, const char* filePathPush) {
	pthread_mutex_lock(&(q->mutex));
	while(!CAN_PUSH(q)) {
		//wait signal from pop
		struct timespec timeout;
		clock_gettime(CLOCK_REALTIME, &timeout);
    	timeout.tv_sec += 5; // wait for 5 sec
		time_t currentTime = time(&currentTime);
		if (pthread_cond_timedwait(&(q->cond_push), &(q->mutex), &timeout) != 0) {
			pthread_mutex_unlock(&(q->mutex)); //pthread_cond_timedwait ALWAYS take the mutex back even in timeout happen so we need to unlock it before returning
			DEBUGGER_PRINT_MEDIUM("push pthread_cond_timedwait timeout for thread %d", getpid());
			//timeout
			return -1;
		}
	}

	element* elementToPush = &(q->arr)[GET_PUSH_INDEX(q)];
	elementToPush->fileName = (char*) malloc((strlen(fileNamePush) + 1) * sizeof(char));
	strcpy(elementToPush->fileName, fileNamePush);
	elementToPush->filePath = (char*) malloc((strlen(filePathPush) + 1) * sizeof(char));
	strcpy(elementToPush->filePath, filePathPush);


	DEBUGGER_PRINT_MEDIUM("[Q-push] fN: %s filePath: %s", elementToPush->fileName, elementToPush->filePath);

	q->indexPush++;
	q->count++;
	//send signal for pop
	pthread_cond_signal(&(q->cond_pop));
	pthread_mutex_unlock(&(q->mutex));
	return 0;

}

int q_pop(queue* q, char** outputFileName, char** outputFilePath) {
	pthread_mutex_lock(&(q->mutex));
	while(IS_EMPTY(q)) {
		//wait signal from push
		struct timespec timeout;
		clock_gettime(CLOCK_REALTIME, &timeout);
    	timeout.tv_sec += 3; // wait for 3 sec
		time_t currentTime = time(&currentTime);
		int res = pthread_cond_timedwait(&(q->cond_pop), &(q->mutex), &timeout);
		if (res != 0) {
			pthread_mutex_unlock(&(q->mutex)); //pthread_cond_timedwait ALWAYS take the mutex back even in timeout happen so we need to unlock it before returning
			DEBUGGER_PRINT_MEDIUM("POP pthread_cond_timedwait timeout");
			//timeout
			*outputFileName = NULL;
			*outputFilePath = NULL;
			return -1;
		}
	}

	element* elementToPop = &(q->arr)[GET_POP_INDEX(q)];
	q->indexPop++;
	q->count--;

	DEBUGGER_PRINT_MEDIUM("[Q-pop] fN: %s filePath: %s", elementToPop->fileName, elementToPop->filePath);
	*outputFileName = elementToPop->fileName;
	*outputFilePath = elementToPop->filePath;


	//send signal to push
	pthread_cond_signal(&(q->cond_push));
	pthread_mutex_unlock(&(q->mutex));

	return 0;
}


int q_empty(queue* q) {
	int result = -1;
	pthread_mutex_lock(&(q->mutex));
	result = q->count;
	pthread_mutex_unlock(&(q->mutex));
	return result;
}


void q_destroy(queue * q) {
	// for this project we only exit if the queue is empty so if is not when destroied its a bug
	if (q->count != 0) {
		fprintf(stderr, "[Q] destroying queue not empty! this is a bug! \n");
		abort();
	}

    pthread_mutex_destroy(&q->mutex);
    pthread_cond_destroy(&q->cond_push);
    pthread_cond_destroy(&q->cond_pop);

    // The array is empty so we don't need to unload each element
	MM_FREE(q->arr);
	MM_FREE(q);

	DEBUGGER_PRINT_MEDIUM("[Q] queue destroyed");
}