CC=gcc
CFLAGS=-Wall -Wextra -I. -pthread -lpthread -std=gnu99
COMMON = MemoryManager.h DebuggerLevel.h
.PHONY = clean test

default: ./utility/farm

./utility/farm: Main.o Worker.o MasterWorker.o SynchronizedQueue.o Collector.o $(COMMON)
	$(CC) $(CFLAGS) $^ -o $@

./utility/generafile: ./utility/generafile.c
	$(CC) $^ -o $@ -std=gnu99

Main.o : Main.c MasterWorker.h Collector.h $(COMMON)

MasterWorker.o: MasterWorker.c MasterWorker.h SynchronizedQueue.h Worker.h Collector.h $(COMMON)

Worker.o : Worker.c Worker.h $(COMMON)

SynchronizedQueue.o: SynchronizedQueue.c SynchronizedQueue.h $(COMMON)

Collector.o: Collector.c Collector.h $(COMMON)


	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o Collector


test: ./utility/farm ./utility/generafile
	cd ./utility && chmod +x test.sh && ./test.sh
	cd ..