#undef M_DEBUG
#define M_DEBUG 0

#include <MasterWorker.h>
#include <MemoryManager.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <Collector.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <DebuggerLevel.h>
#include <dirent.h>



masterworker * gobalMasterWorker;
//i would have prefered not using a global master worker and passing the argument in the signal_handler
//but seams that you can't do that in this

void signal_handler(int signum) {
	//int signum, siginfo_t* info, void* context
	//masterworker* mw = (masterworker*) info->si_value.sival_ptr; //can't use this

	if (signum == SIGPIPE) {
		fprintf(stderr, "DIO CANEEEE SI è ROTTA LA PIPE! \n");
	}


	if (signum == SIGUSR1) {
		//mw should send a message to collector to stamp all the infos
		gobalMasterWorker->segnalsHandler = MW_SIGNAL_STAMP_COLLECTOR;
	} else {
		//mw shoudl start
		gobalMasterWorker->segnalsHandler = MW_SIGNAL_STOP_READING;
	}
}

void signal_handler_empty(int signum) {
}


int main(int argc, char* argv[]) {
	DEBUGGER_INFO();

	int nthread = MW_DEFUALT_THREAD_NUMBER;
	int qlen = MW_DEFAULT_QUEUE_LENGHT;
	char* directory_name = MW_DEFAULT_DIRECTORY_NAME;
	int delay = MW_DEFAULT_DELAY;

	//i'd love to use (opt = getopt(argc, argv, "n:q:d:t:") to handle the option but from the request the list of files can be input at any time even in the middle of other options.

	//used to read and store info about the file we read
	fileToLoad * head = NULL;
	fileToLoad * tail = head;

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
			DEBUGGER_PRINT_LOW("[MAIN] FILE | %s \n", argv[i]);
			if (head == NULL || tail == NULL) {
				MM_MALLOC(head, fileToLoad);
				tail = head;

			} else {
				MM_MALLOC(tail->next, fileToLoad);
				tail = tail->next;
			}
			tail->fileName = malloc(strlen(argv[i]) + 1);
			strcpy(tail->fileName, argv[i]);
			tail->dirPath = malloc(strlen(".") + 1);
			strcpy(tail->dirPath, ".");
			tail->next = NULL;

		}
	}


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

		signal(SIGHUP, signal_handler_empty);
		signal(SIGINT, signal_handler_empty);
		signal(SIGQUIT, signal_handler_empty);
		signal(SIGTERM, signal_handler_empty);
		signal(SIGUSR1, signal_handler_empty);
		signal(SIGPIPE, signal_handler_empty);
		c_start();
	} else {
		//father aka MasterWorker
		gobalMasterWorker = mw_init(nthread, qlen, directory_name, delay, SOCKET_PATH);

		signal(SIGHUP, signal_handler);
		signal(SIGINT, signal_handler);
		signal(SIGQUIT, signal_handler);
		signal(SIGTERM, signal_handler);
		signal(SIGUSR1, signal_handler);
		signal(SIGPIPE, signal_handler);



		// Start the master-worker system
		if (mw_start(gobalMasterWorker, head) != 0) {
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