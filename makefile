CC=gcc
CFLAGS=-Wall -Wextra -pedantic -I. -pthread -lpthread


all: Main clean

Main: Main.o Worker.o Masterworker.o SynchronizedQueue.o Collector.o
	$(CC) $(CFLAGS) -o farm Main.o Masterworker.o Worker.o SynchronizedQueue.o Collector.o

Main.o : Main.c Masterworker.h
	$(CC) $(CFLAGS) -c Main.c

Masterworker.o: Masterworker.c Masterworker.h SynchronizedQueue.h debuggerlevel.h Collector.h Worker.h
	$(CC) $(CFLAGS) -c Masterworker.c

Worker.o : Worker.c Worker.h
	$(CC) $(CFLAGS) -c Worker.c

SynchronizedQueue.o: SynchronizedQueue.c SynchronizedQueue.h
	$(CC) $(CFLAGS) -c SynchronizedQueue.c

Collector.o: Collector.c Collector.h
	$(CC) $(CFLAGS) -c Collector.c

clean:
	rm -f *.o Collector
