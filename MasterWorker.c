#undef M_DEBUG
#define M_DEBUG 1

#include <MasterWorker.h>
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
#include <sys/un.h>
#include <DebuggerLevel.h>
#include <Collector.h>
#include <dirent.h>


unsigned long long memoryAllocated = 0;
int numberOfMalloc = 0;
int numberOfFree = 0;

typedef struct {
    masterworker *mw;
    char** listOfFiles;

} masterWorkerThreadArgs;

typedef struct {
    queue *q;
    int * mwFlag;

    int * socketChannel;
    int * masterSocketPrio;

	pthread_mutex_t mutex;
	pthread_cond_t cond;

} workerThreadArgs;


int read_file_calculate_sum(char* pathToFile) {
    FILE *fp = fopen(pathToFile, "r");
    if (fp == NULL) {
        DEBUGGER_PRINT_LOW("Unable to open file %s", pathToFile);
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    // Read numbers and calculate sum
    int sum = 0, line_number = 0;
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        int num = atoi(line);
        sum += num * line_number;
        line_number++;
    }
    fclose(fp);
    return sum;
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

    DEBUGGER_PRINT_LOW("[mw] after while cycle rs: %d | socket: %d", rs, socketfd);
    if (rs >= 0) {
        return socketfd;
    }

    return -1;
}


void send_message_to_collector(int socketfd, char * fullPath, char * fileName, int result) {
    int rs;
    char msg[MAX_DATA_SIZE];
    snprintf(msg, MAX_DATA_SIZE, "%s:%s:%d", fullPath, fileName, result);
    rs = send(socketfd, msg, strlen(msg), 0);
    if (rs < 0) {
        perror("send() failed");
    }
}

void master_send_message_to_collector(int socketFd, const char * message, int * flagPrio, pthread_mutex_t * mutex, pthread_cond_t * cond) {
    int rs;
    *flagPrio = 1;
    pthread_mutex_lock(mutex);
    *flagPrio = 0;

    char msg[MAX_DATA_SIZE];

    strcpy(msg, message);
    rs = send(socketFd, msg, strlen(msg), 0);
    if (rs < 0) {
        perror("[Master] send() failed");
    }

    pthread_cond_broadcast(cond);
    pthread_mutex_unlock(mutex);
}


void* worker_thread_function(void * arg) {
    DEBUGGER_PRINT_LOW("[Worker] thread started");
    workerThreadArgs * args = (workerThreadArgs *) arg;
    queue * q = args->q;
    int * masterFlag = args->mwFlag;
    int * masterPrio = args->masterSocketPrio;
    int * socketFd = args->socketChannel;
    pthread_mutex_t mutex = args->mutex;
	pthread_cond_t cond = args->cond;

    char *fileName = NULL, *filePath = NULL;

    while ((*masterFlag) == 0 || q_empty(q) != 0) {
        if (q_pop(q, &fileName, &filePath) == 0) {
            DEBUGGER_PRINT_LOW("[Worker] after pop");
            //pop worked
            char fullPath[255];
            sprintf(fullPath, "%s/%s", filePath, fileName);


            DEBUGGER_PRINT_LOW("[Worker] thread opening file %s", fullPath);
            int sum = read_file_calculate_sum(fullPath);

            pthread_mutex_lock(&mutex);

            while((*masterPrio) > 0) {
                pthread_cond_wait(&cond, &mutex);
            }

            if ((*socketFd) > 0) {
                DEBUGGER_PRINT_LOW("[Worker] sending value %d for %s", sum, fullPath);
                send_message_to_collector(*socketFd, fullPath, fileName, sum);
            } else {
                fprintf(stderr, "[ERROR] [Worker] socket channel <= 0 | %d | this is a bug! \n", * socketFd);
            }

            pthread_cond_broadcast(&cond);
            pthread_mutex_unlock(&mutex);

            MM_FREE(fileName);
            MM_FREE(filePath);
        }
        //pop didn't work check for flags and go back in pop
    }
    if ((*masterFlag) != 0) {
        DEBUGGER_PRINT_LOW("[Worker] master flag != 0 | value: %d", (*masterFlag));
    }

    DEBUGGER_PRINT_LOW("[Worker] thread exit");
    //worker thread has finished and the master want to stop, closing this
    return NULL;

}

void* master_worker_thread_function(void* arg) {

    DEBUGGER_PRINT_LOW("[mw] Started function Master Worker");
    masterWorkerThreadArgs* mwArgs = (masterWorkerThreadArgs*)arg;
    masterworker *mw = mwArgs->mw;
    char ** listOfFiles = mwArgs->listOfFiles;
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

    int socketFd = try_connect_to_collector(&serveraddr, 10, 1);
    DEBUGGER_PRINT_LOW("[mw] result of connection to Collector %d", socketFd);
    //processing master worker
    //we need to create n threads workers and give them the queue Q and the flag

    workerThreadArgs * workerArgs;
    MM_MALLOC(workerArgs, workerThreadArgs);
    workerArgs->q = mw->synchronizedQueue;
    workerArgs->mwFlag = masterWorkerFlag;
    workerArgs->masterSocketPrio = masterWorkerSocketPrio;
    MM_MALLOC(workerArgs->socketChannel, int);
    (* workerArgs->socketChannel) = socketFd;
    pthread_mutex_init(&workerArgs->mutex, NULL);
    pthread_cond_init(&workerArgs->cond, NULL);

    DEBUGGER_PRINT_LOW("[mw] creating workers threads");
    for(int i = 0; i < mw->threadNumber; i++) {
        if (pthread_create(&(mw->workersArray)[i], NULL, &worker_thread_function, workerArgs) != 0) {
            perror("[mw] Couldn't create thread worker");
            return NULL;
        }
    }

    int i = 0;
    int stop = 0;
    DEBUGGER_PRINT_LOW("[mw] pushed value inside queue");
    while (stop == 0) {
        if (listOfFiles[i] != NULL) {
            q_push(mw->synchronizedQueue, listOfFiles[i], ".");
            i++;
        } else if (mw->directoryName != MW_DEFAULT_DIRECTORY_NAME) {
            //explore dir!
            //TODO - only thing missing is the file explorer from mw->directoryName
        }

        if (mw->segnalsHandler == MW_SIGNAL_STAMP_COLLECTOR) {
            master_send_message_to_collector(socketFd, "Master-STAMP", masterWorkerSocketPrio, &workerArgs->mutex, &workerArgs->cond);
        } else if (mw->segnalsHandler == MW_SIGNAL_STOP_READING) {
            DEBUGGER_PRINT_LOW("[mw] recived signal to stop!");
            stop = 1;
            continue;
        }

        sleep(mw->delay); //sleep between master actions

    }



    *masterWorkerFlag = 1;

    for (int i = 0; i < mw->threadNumber; i++) {
        DEBUGGER_PRINT_LOW("[mw] waiting worker i %d of %d | %02lx", i, mw->threadNumber, (mw->workersArray)[i]);
        if (pthread_join((mw->workersArray)[i], NULL) != 0) {
            perror("pthread_join failed");
            return NULL;
        }
    }

    close(socketFd);
    MM_FREE(masterWorkerSocketPrio);
    MM_FREE(workerArgs->socketChannel);
    pthread_mutex_destroy(&workerArgs->mutex);
    pthread_cond_destroy(&workerArgs->cond);
    MM_FREE(workerArgs);
    MM_FREE(masterWorkerFlag);



    DEBUGGER_PRINT_LOW("[mw] return null FINISH!");

    return NULL;
}

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

int mw_start(masterworker *mw, char** listOfFiles) {
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

void mw_destroy(masterworker * mw) {
    fprintf(stderr, "[mw] destroying MW\n");
    q_destroy(mw->synchronizedQueue);

    DEBUGGER_PRINT_MEDIUM("[mw] after destroying queue before workersArray");
    MM_FREE(mw->workersArray);

    MM_FREE(mw);

    DEBUGGER_PRINT_MEDIUM("[mw] mw destroyed");
}


masterworker * gobalMasterWorker;
//i would have prefered not using a global master worker and passing the argument in the signal_handler
//but seams that you can't do that in this

void signal_handler(int signum) {
    //int signum, siginfo_t* info, void* context
    //masterworker* mw = (masterworker*) info->si_value.sival_ptr; //can't use this

    if (signum == SIGUSR1) {
        //mw should send a message to collector to stamp all the infos
        gobalMasterWorker->segnalsHandler = MW_SIGNAL_STAMP_COLLECTOR;
    } else {
        //mw shoudl start
        gobalMasterWorker->segnalsHandler = MW_SIGNAL_STOP_READING;
    }
}


int main(int argc, char* argv[]) {
    DEBUGGER_INFO();

    int nthread = MW_DEFUALT_THREAD_NUMBER;
    int qlen = MW_DEFAULT_QUEUE_LENGHT;
    char* directory_name = MW_DEFAULT_DIRECTORY_NAME;
    int delay = MW_DEFAULT_DELAY;

    char * listOfFiles[argc];    //dump solution since not all argv are files but for the sake of this project leave it be
    int listIndex = 0;

    //i'd love to use (opt = getopt(argc, argv, "n:q:d:t:") to handle the option but from the request the list of files can be input at any time even in the middle of other options.


    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            //reading an option
            switch (argv[i][1]) {
                case 'n':
                    nthread = atoi(argv[i + 1]);
                    DEBUGGER_PRINT_LOW("Custom number of thread -n %d", nthread);
                    break;
                case 'q':
                    qlen = atoi(argv[i + 1]);
                    DEBUGGER_PRINT_LOW("Custom size of queue -q %d", qlen);
                    break;
                case 'd':
                    directory_name = argv[i + 1];
                    DEBUGGER_PRINT_LOW("Custom directory -d %s", directory_name);
                    break;
                case 't':
                    delay = atoi(argv[i + 1]);
                    DEBUGGER_PRINT_LOW("Custom delay -t %d", delay);
                    break;

                default:
                    fprintf(stderr, "Usage: %s [-n nthread] [-q qlen] [-d directory-name] [-t delay]\n", argv[0]);
                    exit(EXIT_FAILURE);
                    break;
            }
            i++; //skip the next value
        } else {
            printf("[MAIN] FILES ? \n");
            listOfFiles[listIndex++] = argv[i];
        }
    }

    listOfFiles[listIndex] = NULL; //last null so we can cicle through the array without knowing the size

    // Validate command line arguments
    if (nthread < 1 || qlen < 1 || delay < 0) {
        fprintf(stderr, "[Main] Invalid command line arguments\n");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork() failed");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        //child aka Collector
        //start the collector
        DEBUGGER_PRINT_LOW("[Main] starting collector");

        struct sigaction sa;
        sa.sa_flags = 0;
        sigemptyset(&sa.sa_mask);


        sigaction(SIGHUP, &sa, NULL);
        sigaction(SIGINT, &sa, NULL);
        sigaction(SIGQUIT, &sa, NULL);
        sigaction(SIGTERM, &sa, NULL);
        sigaction(SIGUSR1, &sa, NULL);
        c_start();
    } else {
        //father aka MasterWorker
        gobalMasterWorker = mw_init(nthread, qlen, directory_name, delay, SOCKET_PATH);

        struct sigaction sa;
        sa.sa_handler = signal_handler;
        sa.sa_flags = SA_SIGINFO;
        sigemptyset(&sa.sa_mask);


        sigaction(SIGHUP, &sa, NULL);
        sigaction(SIGINT, &sa, NULL);
        sigaction(SIGQUIT, &sa, NULL);
        sigaction(SIGTERM, &sa, NULL);
        sigaction(SIGUSR1, &sa, NULL);




        // Start the master-worker system
        if (mw_start(gobalMasterWorker, listOfFiles) != 0) {
            fprintf(stderr, "[Main] Failed to start master-worker system\n");
            exit(EXIT_FAILURE);
        }


        if (mw_wait(gobalMasterWorker) != 0) {
            fprintf(stderr, "[Main] Failed to wait master-worker system\n");
            exit(EXIT_FAILURE);
        }


        mw_destroy(gobalMasterWorker);

        MM_INFO();
    }



    return EXIT_SUCCESS;
}