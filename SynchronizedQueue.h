#ifndef SYNCHRONIZED__Q_H
#define SYNCHRONIZED__Q_H

#include <pthread.h>

typedef struct Element {
	char* fileName;
	char* filePath;
} element;

typedef struct {
	element* arr;
	int arraySize;
	int indexPop;
	int indexPush;
	int count;


	pthread_mutex_t mutex;

	pthread_cond_t cond_push;
	pthread_cond_t cond_pop;

} queue;



queue* q_init(int size);

//return 0 if the queue now has the new element, -1 otherwise
int q_push(queue* q, const char* fileNamePush, const char* filePathPush);

//return 0 was able to remove the element from the queue and set outFileName and outputFilePath to the corrispective values, -1 otherwise
int q_pop(queue* q, char** outputFileName, char** outputFilePath);

//return 0 if the queue is empty, the number of elements inside otherwise
int q_empty(queue* q);

void q_destroy(queue *q);

#endif