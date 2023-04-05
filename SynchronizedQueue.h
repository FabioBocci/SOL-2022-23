#ifndef SYNCHRONIZED__Q_H
#define SYNCHRONIZED__Q_H

typedef struct Element {
	char* fileName;
	char* filePath;
} element;

typedef struct {
	element** arr;
	int arraySize;
	int indexPop;
	int indexPush;
	int count;


	pthread_mutex_t mutex;

	pthread_cond_t cond_push;
	pthread_cond_t cond_pop;

} queue;



queue* q_init(int size);

void q_push(queue* q, char* fileNamePush, char* filePathPush);

void q_pop(queue* q, char** outputFileName, char** outputFilePath);

#endif