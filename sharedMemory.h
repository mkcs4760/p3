#ifndef SHAREDMEMORY_H
#define SHAREDmEMROY_H

#include <semaphore.h>

//my structure to attach to shared memory
typedef struct file {
	char data[100][80];
} file_entry;

#endif