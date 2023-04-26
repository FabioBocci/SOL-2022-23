CC=gcc
CFLAGS=-Wall -Wextra -pedantic -I. -pthread -lpthread


all: main clean

main: main.o worker.o masterworker.o synchronizedqueue.o collector.o
	$(CC) $(CFLAGS) -o farm main.o masterworker.o worker.o synchronizedqueue.o collector.o

main.o :
	$(CC) $(CFLAGS) -c Main.c

masterworker.o:
	$(CC) $(CFLAGS) -c MasterWorker.c

worker.o :
	$(CC) $(CFLAGS) -c Worker.c

synchronizedqueue.o:
	$(CC) $(CFLAGS) -c SynchronizedQueue.c

collector.o:
	$(CC) $(CFLAGS) -c Collector.c

clean:
	rm -f *.o collector
