#ifndef SHAREDMEMORY_H
#define SHAREDmEMROY_H

//my structure to attach to shared memory
#include <semaphore.h>
typedef struct file {
	char data[100][80];
	union sem_t *mutex;
} file_entry;

#endif