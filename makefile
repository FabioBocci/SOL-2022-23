CC=gcc
CFLAGS=-Wall -Wextra -pedantic -I. -pthread -lpthread


all: main clean

main: main.o worker.o masterworker.o synchronizedqueue.o collector.o
	$(CC) $(CFLAGS) -o farm main.o masterworker.o worker.o synchronizedqueue.o collector.o

main.o : main.c masterworker.h
	$(CC) $(CFLAGS) -c main.c

masterworker.o: masterworker.c masterworker.h synchronizedqueue.h debuggerlevel.h collector.h worker.h
	$(CC) $(CFLAGS) -c masterworker.c

worker.o : worker.c worker.h
	$(CC) $(CFLAGS) -c worker.c

synchronizedqueue.o: synchronizedqueue.c synchronizedqueue.h
	$(CC) $(CFLAGS) -c synchronizedqueue.c

collector.o: collector.c collector.h
	$(CC) $(CFLAGS) -c collector.c

clean:
	rm -f *.o collector
