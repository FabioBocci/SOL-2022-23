#ifndef MASTERWORKER_H
#define MASTERWORKER_H

#include <SynchronizedQueue.h>

#define MW_DEFUALT_THREAD_NUMBER 4
#define MW_DEFAULT_QUEUE_LENGHT 8
#define MW_DEFAULT_DIRECTORY_NAME NULL
#define MW_DEFAULT_DELAY 0

typedef struct {
	queue* synchronizedQueue;

	int threadNumber;
	int delay;
	char* directoryName;

} masterworker;


masterworker* mw_init(int threadNumber, int queueLenght, char* directoryName, int delay);


#endif