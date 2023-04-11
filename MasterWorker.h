#ifndef MASTERWORKER_H
#define MASTERWORKER_H

#include <SynchronizedQueue.h>

#define MW_DEFUALT_THREAD_NUMBER 4
#define MW_DEFAULT_QUEUE_LENGHT 8
#define MW_DEFAULT_DIRECTORY_NAME NULL
#define MW_DEFAULT_DELAY 0

#define MW_SIGNAL_STAMP_COLLECTOR 1
//send a message to the collector to stamp all the info recived

#define MW_SIGNAL_STOP_READING 2
//stop reading new files and exploring the folders, start waiting for the program to end.

typedef struct {

	int threadNumber;
	int delay;
	char* directoryName;

	queue* synchronizedQueue;
	pthread_t masterWorkerMainThread;
	pthread_t * workersArray;

	char * pathToSocketFile;
	int segnalsHandler; //set this int to MW_SIGNAL_STAMP_COLLECTOR or MW_SIGNAL_STOP_READING to handle the signals

} masterworker;


masterworker* mw_init(int threadNumber, int queueLenght, char* directoryName, int delay, char* pathToSocketFile);

int mw_start(masterworker * mw, char** listOfFiles);

int mw_wait(masterworker * mw);

void mw_destroy(masterworker * mw);

#endif