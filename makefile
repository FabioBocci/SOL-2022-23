CC=gcc
CFLAGS=-Wall -Wextra -pedantic -I. -pthread -lpthread


all: main clean

main: main.o worker.o masterworker.o synchronizedqueue.o collector.o
	$(CC) $(CFLAGS) -o farm main.o masterworker.o worker.o synchronizedqueue.o collector.o

main.o :
	$(CC) $(CFLAGS) -c Main.c

masterworker.o: masterworker.c masterworker.h synchronizedqueue.h debuggerlevel.h collector.h worker.h
	$(CC) $(CFLAGS) -c Masterworker.c

worker.o : worker.c worker.h
	$(CC) $(CFLAGS) -c Worker.c

synchronizedqueue.o: synchronizedqueue.c synchronizedqueue.h
	$(CC) $(CFLAGS) -c SynchronizedQueue.c

collector.o: Collector.c collector.h
	$(CC) $(CFLAGS) -c Collector.c

clean:
	rm -f *.o collector
