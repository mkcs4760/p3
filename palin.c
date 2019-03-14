#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h> 
#include "sharedMemory.h"

int main(int argc, char *argv[]) {
	int shmid;
    int n;
    file_entry *entries;
	/*
	int shmid;
	key_t key;

	int *clockSeconds, *clockNano;
	
	//connect to shared memory
	key = 1094;
	shmid = shmget(key, sizeof(int*) + sizeof(long*), 0666);
	if(shmid < 0) {
		perror("Shmget error in user process ");
		exit(1);
	}
	
	//attach ourselves to that shared memory
	clockSeconds = shmat(shmid, NULL, 0); //attempting to store 2 numbers in shared memory
	clockNano = clockSeconds + 1;
	if((clockSeconds == (int *) -1) || (clockNano == (int *) -1)) {
		perror("shmat error in user process");
		exit(1);
	}
	
	int startSeconds = *clockSeconds;
	int startNano = *clockNano;
	int stopSeconds;
	int stopNano;
	int duration = atoi(argv[1]);
	
	stopSeconds = startSeconds;
	stopNano = startNano + duration;
	if (stopNano >= 1000000000) {
		stopSeconds += 1;
		stopNano -= 1000000000;
	}

	while((*clockSeconds < stopSeconds) || ((*clockSeconds == stopSeconds) && (*clockNano < stopNano)));
	//wait for the correct duration
	printf("Child %d - %d:%d - Terminating\n", getpid(), *clockSeconds, *clockNano);
	*/
	
	printf("start for 0\n");
	//connect to shared memory
	if ((shmid = shmget(1094, sizeof(file_entry) + 256, IPC_CREAT | 0666)) == -1) {
        printf("shmget");
        exit(2);
    }
	//attach to shared memory
	entries = (file_entry*) shmat(shmid, 0, 0);
	if (entries == NULL) {
		printf("problem2");
		exit(2);
	}
	//read from shared memory
	//sleep(1);
	printf("\nChild Reading ....\n\n");
	int i, j = 0;
		
	for (i = 0; i < entries->numOfLines; i++) {
		for (j = 0; j < 80; ++j) {
			printf("%c", entries->data[i][j]);
		}
			printf("\n");
	}
		
	printf("%d\n", entries->numOfLines);
		
	putchar('\n');
	printf("\nDone\n\n");
		
	//destroy shared memory
	int ctl_return = shmctl(shmid, IPC_RMID, NULL);
	if (ctl_return == -1) {
		perror("Function scmctl failed. ");
	}
	printf("end for 0\n");	
	
	return 0;
}
