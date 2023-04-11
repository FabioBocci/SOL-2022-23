CC=gcc
CFLAGS=-Wall -Wextra -pedantic -I. -pthread -lpthread


all: main clean

main: masterworker.o synchronizedqueue.o collector.o
	$(CC) $(CFLAGS) -o farm masterworker.o synchronizedqueue.o collector.o

masterworker.o: masterworker.c masterworker.h synchronizedqueue.h debuggerlevel.h collector.h
	$(CC) $(CFLAGS) -c masterworker.c

synchronizedqueue.o: synchronizedqueue.c synchronizedqueue.h
	$(CC) $(CFLAGS) -c synchronizedqueue.c

collector.o: collector.c collector.h
	$(CC) $(CFLAGS) -c collector.c

clean:
	rm -f *.o collector
