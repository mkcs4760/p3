#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h> 
#include <semaphore.h>
#include <fcntl.h> //used for O_CREAT
#include <time.h>
#include "sharedMemory.h"

int randomNum() {
	srand(time(0));
	int num = (rand() % 3) + 1;
	printf("%d\n", num);
	return num;
}

int main(int argc, char *argv[]) {
	int shmid;
    //int n;
    file_entry *entries;
	
	int startIndex = atoi(argv[1]);
	int duration = atoi(argv[2]);
	//printf("Cool! %d and %d!!\n", startIndex, duration);
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
	
	printf("start for child with value %d\n", startIndex);
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
	//sem_t *sem = sem_open("sem", 0);
	/*if (*sem == -1) {
		printf("Error conntecting to semaphore\n");
	}*/
	sem_t *sem = sem_open("mutex", O_RDWR);
	if (sem == SEM_FAILED) {
        perror("sem_open(3) failed");
        exit(EXIT_FAILURE);
    }
	
	
	//read from shared memory
	//printf("\nChild Reading ....\n\n");
	int i, j = 0;
	//sem_wait(entries->mutex);
	sem_wait(sem);
	printf("child with value %d entering critical zone\n", startIndex);
	sleep(randomNum());	
	for (i = startIndex; i < startIndex + duration; i++) { //for each string
		int stringLength = strlen(entries->data[i]);
		char reverseString[80] = {'\0'};
		for (j = stringLength - 1; j >= 0; j--) {
			reverseString[stringLength - j - 1] = entries->data[i][j];
		}
		printf ("%s is the original string\n", entries->data[i]);
		printf("%s is the reverse string\n", reverseString);
		int flag = 1;
		for(j = 0; j < stringLength; j++) {
			if (reverseString[j] != entries->data[i][j]) {
				flag = 0;
			}
		}
		
		
		if (flag == 1) {
			printf("Palindrome!!\n");
			FILE *pOut;
			pOut = fopen("palin.out", "a");
			fprintf(pOut, "%s\n", entries->data[i]);
			fclose(pOut);
		} else {
			printf("Not a palindrome...\n");
			FILE *nOut;
			nOut = fopen("nopalin.out", "a");
			fprintf(nOut, "%s\n", entries->data[i]);
			fclose(nOut);
		}
		
		printf("\n");
		
	}
	sleep(randomNum());
	printf("child with value %d exiting critical zone\n", startIndex);
	sem_post(sem);
	//sem_post(entries->mutex);
	//printf("%d\n", entries->numOfLines);
		
	//putchar('\n');
	//printf("\nDone\n\n");
		
	//destroy shared memory
	/*int ctl_return = shmctl(shmid, IPC_RMID, NULL);
	if (ctl_return == -1) {
		perror("Function scmctl failed. ");
	}*/
	printf("end for child with value %d\n", startIndex);
	
	return 0;
}
