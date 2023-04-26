#undef M_DEBUG
#define M_DEBUG 0

#include <MasterWorker.h>
#include <Worker.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <SynchronizedQueue.h>
#include <MemoryManager.h>
#include <Collector.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <DebuggerLevel.h>
#include <Collector.h>
#include <dirent.h>

//Externs used from MemoryManager.h
unsigned long long memoryAllocated = 0;
int numberOfMalloc = 0;
int numberOfFree = 0;
//END externs

// Utility structs to pass args to threads functions
typedef struct {
	masterworker *mw;
	fileToLoad * listOfFiles;

} masterWorkerThreadArgs;



//Utility function to check if the string passed end with .dat (is one of the file to read)
//return 0 if the string end with .dat, -1 otherwise
int end_with_dat_string(char * str) {
	char *endWithStr = ".dat";

	if (strstr(str, endWithStr) != NULL) {
		size_t len = strlen(str);
		size_t endWithStr_len = strlen(endWithStr);
		if (len >= endWithStr_len && !strcmp(str + len - endWithStr_len, endWithStr)) {
			return 0;
		}
	}

	return -1;
}

//try to create a connection to the server, for a max times of attemps
// - Return socketfd if success -1 otherwise
int try_connect_to_collector(struct sockaddr_un * serveraddr, int attempts, float sleepTime) {
	int socketfd, rs = -1;
	int iteration = 0;

	socketfd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (socketfd < 0) {

		DEBUGGER_PRINT_LOW("[mw] socket() < 0");
		return -1;
	}

	while (rs < 0 && iteration < attempts) {
		iteration++;
		rs = connect(socketfd, (struct sockaddr *) serveraddr, SUN_LEN(serveraddr));
		if (rs < 0) {
		   sleep(sleepTime);
		}
	}

	if (rs >= 0) {
		return socketfd;
	}

	return -1;
}



//function used from MW to send a message signal to the socket, it also use internaly mutex cond and flag to get prio.
void master_send_message_to_collector(int socketFd, const char * message, int * flagPrio, pthread_mutex_t * mutex, pthread_cond_t * cond) {
	int rs;
	*flagPrio = 1;
	pthread_mutex_lock(mutex);
	*flagPrio = 0;

	char msg[MAX_DATA_SIZE];
	snprintf(msg, MAX_DATA_SIZE, "%s", message);
	DEBUGGER_PRINT_LOW("Master Message to server: %s \n", msg);
	rs = send(socketFd, msg, strlen(msg), 0);
	if (rs < 0) {
		perror("[Master] send() failed");
	}
	pthread_cond_broadcast(cond);
	pthread_mutex_unlock(mutex);
}

//main function of thread master worker
void* master_worker_thread_function(void* arg) {

	DEBUGGER_PRINT_LOW("[mw] Started function Master Worker");
	masterWorkerThreadArgs* mwArgs = (masterWorkerThreadArgs*)arg;
	masterworker *mw = mwArgs->mw;
	fileToLoad * listOfFiles = mwArgs->listOfFiles;
	int * masterWorkerFlag;
	int * masterWorkerSocketPrio;
	MM_MALLOC(masterWorkerFlag, int);
	*masterWorkerFlag = 0;
	MM_MALLOC(masterWorkerSocketPrio, int);
	*masterWorkerSocketPrio = 0;

	MM_FREE(mwArgs);

	struct sockaddr_un serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));

	serveraddr.sun_family = AF_UNIX;
	strcpy(serveraddr.sun_path, SOCKET_PATH);

	//try to connect to the server (Collector)
	int socketFd = try_connect_to_collector(&serveraddr, 10, 1);

	int val = 1;
	setsockopt(socketFd, SOL_SOCKET, SO_KEEPALIVE, &val, sizeof(val));

	DEBUGGER_PRINT_LOW("[mw] result of connection to Collector %d", socketFd);
	//processing master worker
	//we need to create n threads workers and give them the queue Q and the flag



	pthread_mutex_t mutex;
	pthread_cond_t cond;
	pthread_mutex_init(&mutex, NULL);
	pthread_cond_init(&cond, NULL);

	DEBUGGER_PRINT_LOW("[mw] creating workers threads");
	//Create n thread workers
	for(int i = 0; i < mw->threadNumber; i++) {
		if (worker_create(&(mw->workersArray)[i], mw->synchronizedQueue, masterWorkerFlag, masterWorkerSocketPrio, socketFd, &mutex, &cond) != 0) {
			perror("[mw] Couldn't create thread worker");
			return NULL;
		}
	}

	fileToLoad * currentFileHead = listOfFiles;
	fileToLoad * currentFileTail = NULL; //NB: this is only used when loading the file from a dir!
	fileToLoad * dirToExplore = NULL;
	fileToLoad * dirToExploreTail = NULL;
	if (mw->directoryName != MW_DEFAULT_DIRECTORY_NAME) {
		MM_MALLOC(dirToExplore, fileToLoad);
		dirToExplore->dirPath = malloc(strlen(mw->directoryName) + 1);
		strcpy(dirToExplore->dirPath, mw->directoryName);
		dirToExplore->next = NULL;
		dirToExploreTail = dirToExplore;
	}

	int stop = 0;

	//processing files and dir
	while (stop == 0) {
		if (currentFileHead != NULL) {
			while(q_push(mw->synchronizedQueue, currentFileHead->fileName, currentFileHead->dirPath) == -1) {
				//happen timeout check for condition to stop first then go back in push
				if (stop != 0 || mw->segnalsHandler != 0) {
					//something happens exit this while and going to handle that
					break;
				}
			}
			if (stop == 0 && mw->segnalsHandler == 0) {
				DEBUGGER_PRINT_LOW("[mw] pushed value inside queue");
				//we didn't exit the while for a signal so we can move to the next and free the old head;
				fileToLoad * tmp = currentFileHead;
				currentFileHead = currentFileHead -> next;

				MM_FREE(tmp->dirPath);
				MM_FREE(tmp->fileName);
				MM_FREE(tmp);
			}
			currentFileTail = NULL;
			//this is not null if we just pushed a new file inside the list when exploring a dir, we need to reset this to null
			//no memory lost since we have clear it up from the head
		} else if (dirToExplore != NULL) {
			DIR * currentDir = opendir(dirToExplore->dirPath);
			if (currentDir != NULL) {
				fprintf(stdout, "[mw] Exploring Dir: %s \n ", dirToExplore->dirPath);
				struct dirent * entry;
				while ((entry = readdir(currentDir)) != NULL) {
					if (entry->d_type == DT_REG) {
						if (end_with_dat_string(entry->d_name) == 0) {
							DEBUGGER_PRINT_LOW("[mw] Entry was DAT file: %s \n ", entry->d_name);
							//NB. currentFileHead == NULL at this point!
							if (currentFileTail == NULL) {
								//first time we load a new file!
								MM_MALLOC(currentFileHead, fileToLoad);
								currentFileTail = currentFileHead;
							} else {
								MM_MALLOC(currentFileTail->next, fileToLoad);
								currentFileTail = currentFileTail->next;
							}
							currentFileTail->next = NULL;
							currentFileTail->fileName = malloc(strlen(entry->d_name) + 1);
							strcpy(currentFileTail->fileName, entry->d_name);
							currentFileTail->dirPath = malloc(strlen(dirToExplore->dirPath) + 1);
							strcpy(currentFileTail->dirPath, dirToExplore->dirPath);
						}
					} else if (entry->d_type == DT_DIR) {
						if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
							//skip . and ..
							continue;
						}
						MM_MALLOC(dirToExploreTail->next, fileToLoad);

						dirToExploreTail = dirToExploreTail->next;
						dirToExploreTail -> next = NULL;
						dirToExploreTail->dirPath = malloc(strlen(dirToExplore->dirPath) + strlen(entry->d_name) + 2); //+2 since one for end char and one for /
						sprintf(dirToExploreTail->dirPath, "%s/%s", dirToExplore->dirPath, entry->d_name);

						DEBUGGER_PRINT_LOW("[mw] Entry was a Dir: %s | Added dir to explore: %s \n ", entry->d_name, dirToExploreTail->dirPath);
					} else {
						fprintf(stdout, "[mw] Entry was not a file or dir: %s \n ", entry->d_name);
					}
				}
				closedir(currentDir);
			} else {
				fprintf(stderr, "[mw] invalid Dir for %s \n", dirToExplore->dirPath);
			}
			fileToLoad * tmp = dirToExplore;
			dirToExplore = dirToExplore->next;
			MM_FREE(tmp->dirPath);
			MM_FREE(tmp);

		} else {
			stop = 1;
			continue;
		}

		//check if we recived a signal
		if (mw->segnalsHandler == MW_SIGNAL_STAMP_COLLECTOR) {
			master_send_message_to_collector(socketFd, "MASTER:STAMP", masterWorkerSocketPrio, &mutex, &cond);
		} else if (mw->segnalsHandler == MW_SIGNAL_STOP_READING) {
			DEBUGGER_PRINT_LOW("[mw] recived signal to stop!");
			stop = 1;
			continue;
		}

		usleep(mw->delay * 1000); //sleep between master actions

	}


	//set the flag for workers to stop
	*masterWorkerFlag = 1;

	//Wait all the workers to end their thread
	for (int i = 0; i < mw->threadNumber; i++) {
		DEBUGGER_PRINT_LOW("[mw] waiting worker i %d of %d | %02lx", i, mw->threadNumber, (mw->workersArray)[i]);
		if (worker_wait((mw->workersArray)[i]) != 0) {
			perror("worker_wait failed");
			return NULL;
		}
	}

	//final message to collector so he know we are closing
	master_send_message_to_collector(socketFd, "MASTER:CLOSE", masterWorkerSocketPrio, &mutex, &cond);

	//Free memory used for workers and masterworker thread
	close(socketFd);

	if (currentFileHead != NULL) {
		//this can happen if we exit the program before has finished!
		fileToLoad * tmp;
		while (currentFileHead != NULL)
		{
			tmp = currentFileHead;
			currentFileHead = currentFileHead->next;
			MM_FREE(tmp->dirPath);
			MM_FREE(tmp->fileName);
			MM_FREE(tmp);
		}
	}

	if (dirToExplore != NULL) {
		//this can happen if we exit before the program ends
		fileToLoad * tmp;
		while (dirToExplore != NULL)
		{
			tmp = dirToExplore;
			dirToExplore = dirToExplore->next;
			MM_FREE(tmp->dirPath);
			MM_FREE(tmp);
		}
	}

	MM_FREE(masterWorkerSocketPrio);
	pthread_mutex_destroy(&mutex);
	pthread_cond_destroy(&cond);
	MM_FREE(masterWorkerFlag);



	DEBUGGER_PRINT_LOW("[mw] FINISH!");

	return NULL;
}

//init a new master worker
masterworker* mw_init(int threadNumber, int queueLenght, char* directoryName, int delay, char * pathToSocketFile) {

	DEBUGGER_PRINT_MEDIUM("[mw] Creating new MasterWorker");
	DEBUGGER_PRINT_MEDIUM("[mw] Values applied:");
	DEBUGGER_PRINT_MEDIUM("[mw]  - Thread Number: %d", threadNumber);
	DEBUGGER_PRINT_MEDIUM("[mw]  - Queue Lenght: %d", queueLenght);
	DEBUGGER_PRINT_MEDIUM("[mw]  - Directory Name: %s", directoryName);
	DEBUGGER_PRINT_MEDIUM("[mw]  - Path To SocketFile: %s", pathToSocketFile);

	masterworker *mw;
	MM_MALLOC(mw, masterworker);
	mw->synchronizedQueue = q_init(queueLenght);
	mw->delay = delay;
	mw->segnalsHandler = 0;
	mw->directoryName = directoryName;
	mw->threadNumber = threadNumber;

	MM_MALLOC_N(mw->workersArray, pthread_t, threadNumber);
	mw->pathToSocketFile = pathToSocketFile;

	return mw;
}

//start new thead for master worker
int mw_start(masterworker *mw, fileToLoad * listOfFiles) {
	masterWorkerThreadArgs* mwArgs;
	MM_MALLOC(mwArgs, masterWorkerThreadArgs);
	mwArgs->mw = mw;
	mwArgs->listOfFiles = listOfFiles;

	if (pthread_create(&(mw->masterWorkerMainThread), NULL, master_worker_thread_function, mwArgs) != 0) {
		perror("[mw] pthread_create failed");
		return 1;
	}

	DEBUGGER_PRINT_LOW("[mw] Started new thread for masterworker main");
	return 0;
}

//wait master worker thread
int mw_wait(masterworker *mw) {
	DEBUGGER_PRINT_LOW("[Main] thread waiting master worker");
	// Wait for the master-worker main thread to finish
	if (pthread_join(mw->masterWorkerMainThread, NULL) != 0) {
		perror("pthread_join failed");
		return 1;
	}

	DEBUGGER_PRINT_LOW("[Main] Master-worker main thread finished");
	return 0;
}

//clear memory
void mw_destroy(masterworker * mw) {
	DEBUGGER_PRINT_LOW("[mw] destroying MW\n");
	q_destroy(mw->synchronizedQueue);

	DEBUGGER_PRINT_MEDIUM("[mw] after destroying queue before workersArray");
	MM_FREE(mw->workersArray);

	MM_FREE(mw);

	DEBUGGER_PRINT_MEDIUM("[mw] mw destroyed");
}


