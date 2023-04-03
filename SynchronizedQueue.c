#include <pthread.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <SynchronizedQueue.h>
#include <string.h>

#define GET_PUSH_INDEX (indexPush % arraySize)
#define GET_POP_INDEX (indexPop % arraySize)
#define IS_EMPTY (count == 0)
#define CAN_PUSH (count < arraySize)
#define IS_INITIALIZED(str) {\
	if (arraySize < 0) {\
		fprintf(stderr, "SynchronizedQueue is not INITIALIZED - %s ", str); \
		exit(EXIT_FAILURE);\
	}\
}

typedef struct Element {
	char* fileName;
	char* filePath;
} element;

element** arr;
int arraySize = -1;
int indexPop = 0;
int indexPush = -1;
int count = 0;


pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t cond_push = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_pop = PTHREAD_COND_INITIALIZER;


void q_init(int size) {
	arr = (element**) malloc(size * sizeof(element*));
	arraySize = size;
	indexPush = size;
}


void q_push(char* fileNamePush, char* filePathPush) {
	pthread_mutex_lock(&mutex);
	IS_INITIALIZED("push");
	while(!CAN_PUSH) {
		//wait signal from pop
		pthread_cond_wait(&cond_push, &mutex);
	}

	element* elementToPush = (element*) malloc(sizeof(element));
	arr[GET_PUSH_INDEX] = elementToPush;
	elementToPush->fileName = (char*) malloc((strlen(fileNamePush) + 1) * sizeof(char));
	strcpy(elementToPush->fileName, fileNamePush);
	elementToPush->filePath = (char*) malloc((strlen(filePathPush) + 1) * sizeof(char));
	strcpy(elementToPush->filePath, filePathPush);


	indexPush++;
	count++;
	//send signal for pop
	pthread_cond_signal(&cond_pop);
	pthread_mutex_unlock(&mutex);

}

void q_pop(char** outputFileName, char** outputFilePath) {
	pthread_mutex_lock(&mutex);
	IS_INITIALIZED("pop");
	while(IS_EMPTY) {
		//wait signal from push
		pthread_cond_wait(&cond_push, &mutex);
	}

	element* elementToPop = arr[GET_POP_INDEX];
	indexPop++;
	count--;

	*outputFileName = elementToPop->fileName;
	*outputFilePath = elementToPop->filePath;

    free(elementToPop);

	//send signal to push
	pthread_cond_signal(&cond_push);
	pthread_mutex_unlock(&mutex);
}





