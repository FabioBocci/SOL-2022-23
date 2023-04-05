#include <MasterWorker.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <SynchronizedQueue.h>
#include <DebuggerLevel.h>


masterworker* mw_init(int threadNumber, int queueLenght, char* directoryName, int delay) {

    DEBUGGER_PRINT_HIGH("Creating new MasterWorker \n");
    DEBUGGER_PRINT_HIGH("Values applied: \n");
    DEBUGGER_PRINT_HIGH(" - Thread Number: %d \n", threadNumber);
    DEBUGGER_PRINT_HIGH(" - Queue Lenght: %d \n", queueLenght);
    DEBUGGER_PRINT_HIGH(" - Directory Name: %s \n", directoryName);

    masterworker *mw = (masterworker*) malloc(sizeof(masterworker));
    mw->synchronizedQueue = q_init(queueLenght);
    mw->delay = delay;
    mw->directoryName = directoryName;
    mw->threadNumber = threadNumber;

    return mw;
}




int main(int argc, char* argv[]) {
    DEBUGGER_INFO();

    int nthread = MW_DEFUALT_THREAD_NUMBER;
    int qlen = MW_DEFAULT_QUEUE_LENGHT;
    char* directory_name = MW_DEFAULT_DIRECTORY_NAME;
    int delay = MW_DEFAULT_DELAY;

    // Parse command line arguments
    int opt;
    while ((opt = getopt(argc, argv, "n:q:d:t:")) != -1) {
        switch (opt) {
            case 'n':
                nthread = atoi(optarg);
                break;
            case 'q':
                qlen = atoi(optarg);
                break;
            case 'd':
                directory_name = optarg;
                break;
            case 't':
                delay = atoi(optarg);
                break;
            default:
                fprintf(stderr, "Usage: %s [-n nthread] [-q qlen] [-d directory-name] [-t delay]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    // Validate command line arguments
    if (nthread < 1 || qlen < 1 || delay < 0) {
        fprintf(stderr, "Invalid command line arguments\n");
        exit(EXIT_FAILURE);
    }
    mw_init(nthread, qlen, directory_name, delay);

    return EXIT_SUCCESS;
}