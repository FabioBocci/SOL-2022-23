#ifndef MASTERWORKER_H
#define MASTERWORKER_H

#include <SynchronizedQueue.h>

#define MW_DEFUALT_THREAD_NUMBER 4
#define MW_DEFAULT_QUEUE_LENGHT 8
#define MW_DEFAULT_DIRECTORY_NAME NULL
#define MW_DEFAULT_DELAY 2

#define MW_SIGNAL_STAMP_COLLECTOR 1
//send a message to the collector to stamp all the info recived

#define MW_SIGNAL_STOP_READING 2
//stop reading new files and exploring the folders, start waiting for the program to end.

//main structure of master worker.
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

//simple dinamic list to store file and its path
typedef struct fileToLoad {
    char * fileName;
    char * dirPath;
    struct fileToLoad * next;
} fileToLoad;

//Allocate and create a new masterworker with the values passed as its values
masterworker* mw_init(int threadNumber, int queueLenght, char* directoryName, int delay, char* pathToSocketFile);

//Start a new thread for the master worker, and also pass the list of files to initialy process
int mw_start(masterworker * mw, struct fileToLoad * listOfFiles);

//Wait that the thread created for this masterworker mw has finished
int mw_wait(masterworker * mw);

//Clear memory and destroy the master worker MUST use after wait.
void mw_destroy(masterworker * mw);

#endif