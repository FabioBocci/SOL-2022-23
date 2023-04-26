#undef M_DEBUG
#define M_DEBUG 0


#include <Worker.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <DebuggerLevel.h>
#include <dirent.h>
#include <Collector.h>
#include <MemoryManager.h>

//Utility struct to store worker thread args
typedef struct {
	queue *q;
	int * mwFlag;

	int socketChannel;
	int * masterSocketPrio;

	pthread_mutex_t * mutex;
	pthread_cond_t * cond;

} workerThreadArgs;
// END Utility structs


//simple function who open and read the values from the file passed as argument
long read_file_calculate_sum(char* pathToFile) {
	FILE *fp = fopen(pathToFile, "rb");
	if (fp == NULL) {
		DEBUGGER_PRINT_LOW("Unable to open file %s", pathToFile);
		perror("fopen");
		exit(EXIT_FAILURE);
	}
	// Read numbers and calculate sum
	long sum = 0, line_number = 0, buffer;
	while(fread(&buffer, sizeof(buffer), 1, fp) == 1) {
		sum += buffer * line_number;
		//fprintf(stderr, "file: %s | linechar: %ld | line: %ld | num: %ld | num * line: %ld | sum: %ld \n", pathToFile, buffer, line_number, buffer, buffer * line_number, sum);
		line_number++;
		buffer = 0;
	}

	fclose(fp);
	return sum;
}


//function used from worker to send a message to the socket
void send_message_to_collector(int socketfd, char * fullPath, char * fileName, long result) {
	int rs;
	char msg[MAX_DATA_SIZE];
	snprintf(msg, MAX_DATA_SIZE, "%s:%s:%ld", fullPath, fileName, result);
	rs = send(socketfd, msg, strlen(msg), 0);
	if (rs < 0) {
		perror("send() failed");
	}
}


//main function of thread worker
void* worker_thread_function(void * arg) {
	DEBUGGER_PRINT_LOW("[Worker] thread started");
	workerThreadArgs * args = (workerThreadArgs *) arg;
	queue * q = args->q;
	int * masterFlag = args->mwFlag;
	int * masterPrio = args->masterSocketPrio;
	pthread_mutex_t * mutex = args->mutex;
	pthread_cond_t * cond = args->cond;
	int socketFd = args->socketChannel;
	MM_FREE(args);
	char *fileName = NULL, *filePath = NULL;

	while ((*masterFlag) == 0 || q_empty(q) != 0) {
		if (q_pop(q, &fileName, &filePath) == 0) {
			DEBUGGER_PRINT_LOW("[Worker] after pop");
			//pop worked
			char fullPath[255];
			sprintf(fullPath, "%s/%s", filePath, fileName);

			DEBUGGER_PRINT_LOW("[Worker] thread opening file %s", fullPath);
			long sum = read_file_calculate_sum(fullPath);

			pthread_mutex_lock(mutex);

			while((*masterPrio) > 0) {
				pthread_cond_wait(cond, mutex);
			}

			if (socketFd > 0) {
				DEBUGGER_PRINT_LOW("[Worker] sending value %ld for %s", sum, fullPath);
				send_message_to_collector(socketFd, fullPath, fileName, sum);
			} else {
				fprintf(stderr, "[ERROR] [Worker] socket channel <= 0 | %d | this is a bug! \n",  socketFd);
			}

			pthread_cond_broadcast(cond);
			pthread_mutex_unlock(mutex);

			MM_FREE(fileName);
			MM_FREE(filePath);
		}
		//pop didn't work check for flags and go back in pop
	}

	DEBUGGER_PRINT_LOW("[Worker] thread exit");
	//worker thread has finished and the master want to stop, closing
	return NULL;
}


//return 0 if success to create a new worker thread, -1 otherwise.
int worker_create(pthread_t * thread, queue * q, int * masterWorkerFlag, int * masterWorkerSocketPrio, int socketFd, pthread_mutex_t * mutex, pthread_cond_t * cond) {
	workerThreadArgs * workerArgs;
	MM_MALLOC(workerArgs, workerThreadArgs);
	workerArgs->q = q;
	workerArgs->mwFlag = masterWorkerFlag;
	workerArgs->masterSocketPrio = masterWorkerSocketPrio;
	workerArgs->socketChannel = socketFd;
	workerArgs->mutex = mutex;
	workerArgs->cond = cond;

	if (pthread_create(thread, NULL, &worker_thread_function, workerArgs) != 0) {
		return -1;
	}
	return 0;
}

//wait the worker to end, return 0 if success, -1 otherwise.
int worker_wait(pthread_t thread) {
	if (pthread_join(thread, NULL) != 0) {
		return -1;
	}
	return 0;
}