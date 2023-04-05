#include <pthread.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <SynchronizedQueue.h>
#include <string.h>

#define GET_QUEUE_SIZE(q) (q -> arraySize)
#define GET_PUSH_INDEX(q) ((q -> indexPush) %  GET_QUEUE_SIZE(q))
#define GET_POP_INDEX(q) ((q -> indexPop) % GET_QUEUE_SIZE(q))
#define IS_EMPTY(q) ((q -> count) == 0)
#define CAN_PUSH(q) ((q -> count) < GET_QUEUE_SIZE(q))

queue* q_init(int size) {
	queue * q = (queue *) malloc(sizeof(queue));

	q->arr = (element**) malloc(size * sizeof(element*));
	q->arraySize = size;
	q->indexPush = size;
	q->indexPop = 0;
	q->count = 0;

	pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->cond_push, NULL);
    pthread_cond_init(&q->cond_pop, NULL);

	return q;
}


void q_push(queue* q, char* fileNamePush, char* filePathPush) {
	pthread_mutex_lock(&(q->mutex));
	while(!CAN_PUSH(q)) {
		//wait signal from pop
		pthread_cond_wait(&(q->cond_push), &(q->mutex));
	}

	element* elementToPush = (element*) malloc(sizeof(element));
	(q->arr)[GET_PUSH_INDEX(q)] = elementToPush;
	elementToPush->fileName = (char*) malloc((strlen(fileNamePush) + 1) * sizeof(char));
	strcpy(elementToPush->fileName, fileNamePush);
	elementToPush->filePath = (char*) malloc((strlen(filePathPush) + 1) * sizeof(char));
	strcpy(elementToPush->filePath, filePathPush);


	q->indexPush++;
	q->count++;
	//send signal for pop
	pthread_cond_signal(&(q->cond_pop));
	pthread_mutex_unlock(&(q->mutex));

}

void q_pop(queue* q, char** outputFileName, char** outputFilePath) {
	pthread_mutex_lock(&(q->mutex));
	while(IS_EMPTY(q)) {
		//wait signal from push
		pthread_cond_wait(&(q->cond_pop), &(q->mutex));
	}

	element* elementToPop = (q->arr)[GET_POP_INDEX(q)];
	q->indexPop++;
	q->count--;

	*outputFileName = elementToPop->fileName;
	*outputFilePath = elementToPop->filePath;

    free(elementToPop);

	//send signal to push
	pthread_cond_signal(&(q->cond_push));
	pthread_mutex_unlock(&(q->mutex));
}
