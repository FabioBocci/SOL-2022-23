#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include <DebuggerLevel.h>
#include <stdlib.h>
#include <stdio.h>

extern unsigned long long memoryAllocated;
extern int numberOfMalloc;
extern int numberOfFree;


#define MM_MALLOC(pointer, type)					\
	pointer = (type*) malloc(sizeof(type));			\
	numberOfMalloc++;								\
	memoryAllocated += sizeof(type);				\
	DEBUGGER_PRINT_HIGH("Allocated memory for %s of type %s | size %ld | Memory Used %lld | in file %s | row %d", # pointer, # type, sizeof(type), memoryAllocated, __FILE__, __LINE__);\

#define MM_MALLOC_N(pointer, type, size)				\
	pointer = (type*) malloc(sizeof(type) * (size));	\
	numberOfMalloc++;									\
	memoryAllocated += (sizeof(type) * (size));			\
	DEBUGGER_PRINT_HIGH("Allocated memory for %s of type %s | size %ld | Memory Used %lld | in file %s | row %d", # pointer, # type, (sizeof(type) * (size)), memoryAllocated, __FILE__, __LINE__);\


#define MM_FREE(pointer)							\
	memoryAllocated -= sizeof(pointer);				\
	numberOfFree++;									\
	DEBUGGER_PRINT_HIGH("Free memory for %s | size %ld | Memory Used %lld | in file %s | row %d", # pointer, sizeof(pointer), memoryAllocated, __FILE__, __LINE__);\
	free(pointer);

#define MM_SIZED_MALLOC(pointer, size)					\
	pointer = malloc(size);								\
	memoryAllocated += (size);							\
	numberOfMalloc++;									\
	DEBUGGER_PRINT_HIGH("Allocated memory for %s | size %ld | Memory Used %lld | in file %s | row %d", # pointer, (size), memoryAllocated, __FILE__, __LINE__);\

#define MM_INFO() 														\
	DEBUGGER_PRINT_LOW("------Memory manager------"); 					\
	DEBUGGER_PRINT_LOW("Total allocated memory %lld", memoryAllocated);	\
	DEBUGGER_PRINT_LOW("  total heap usage: %d malloc, %d frees", numberOfMalloc, numberOfFree);	\
	DEBUGGER_PRINT_LOW("-------------------------");

#endif