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
#include <signal.h>
#include "sharedMemory.h"


int randomNum() {
	srand(time(0));
	int num = (rand() % 3) + 1;
	printf("%d\n", num);
	return num;
}

//function obtained at url https://stackoverflow.com/questions/3930363/implement-time-delay-in-c
void waitFor (unsigned int secs) {
	unsigned int retTime = time(0) + secs;
	while (time(0) < retTime);
}

int main(int argc, char *argv[]) {
	int shmid;
    //int n;
    file_entry *entries;
	
	int startIndex = atoi(argv[1]);
	int duration = atoi(argv[2]);
	
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

	sem_t *semP = sem_open("mutexP", O_RDWR);
	if (semP == SEM_FAILED) {
        perror("sem_open(3P) failed");
        exit(EXIT_FAILURE);
    }
	sem_t *semN = sem_open("mutexN", O_RDWR);
	if (semN == SEM_FAILED) {
        perror("sem_open(3N) failed");
        exit(EXIT_FAILURE);
    }
		
	
	//read from shared memory
	//printf("\nChild Reading ....\n\n");
	int i, j = 0;
	//sem_wait(entries->mutex);

	for (i = startIndex; i < startIndex + duration; i++) { //for each string
		int stringLength = strlen(entries->data[i]);
		char reverseString[80] = {'\0'};
		for (j = stringLength - 1; j >= 0; j--) {
			reverseString[stringLength - j - 1] = entries->data[i][j];
		}
		//printf ("%s is the original string\n", entries->data[i]);
		//printf("%s is the reverse string\n", reverseString);
		int flag = 1;
		for(j = 0; j < stringLength; j++) {
			if (reverseString[j] != entries->data[i][j]) {
				flag = 0;
			}
		}
		
		int pid = getpid();
		if (flag == 1) {
			sem_wait(semP);
			printf("child with value %d entering critical zone for P\n", startIndex);
			//critical zone starts
			waitFor(randomNum());
			//sleep(randomNum());	
			//printf("Palindrome!!\n");
			FILE *pOut;
			pOut = fopen("palin.out", "a");
			fprintf(pOut, "%d\t%d\t%s\n", pid, i, entries->data[i]);
			fclose(pOut);
			//sleep(randomNum());
			waitFor(randomNum());
			//critical zone ends
			printf("child with value %d exiting critical zone for P\n", startIndex);
			sem_post(semP);
		
		} else {
			sem_wait(semN);
			printf("child with value %d entering critical zone for N\n", startIndex);
			//critical zone starts
			waitFor(randomNum());
			//sleep(randomNum());				
			//printf("Not a palindrome...\n");
			FILE *nOut;
			nOut = fopen("nopalin.out", "a");
			fprintf(nOut, "%d\t%d\t%s\n", pid, i, entries->data[i]);
			fclose(nOut);
			//sleep(randomNum());
			waitFor(randomNum());
			//critical zone ends
			printf("child with value %d exiting critical zone for N\n", startIndex);
			sem_post(semN);			
		}
		printf("\n");	
	}
	
	printf("end for child with value %d\n", startIndex);
	
	return 0;
}
