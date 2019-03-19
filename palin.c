#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h> 
#include <semaphore.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include "sharedMemory.h"

//generates a random number between 1 and 3
int randomNum() {
	srand(time(0));
	int num = (rand() % 3) + 1;
	return num;
}

//takes in program name and error string, and runs error message procedure
void errorMessage(char programName[100], char errorString[100]){
	char errorFinal[200];
	sprintf(errorFinal, "%s : Error : %s", programName, errorString);
	perror(errorFinal);
	exit(1);
}

//function obtained at url https://stackoverflow.com/questions/3930363/implement-time-delay-in-c
//waits for a given number of seconds
void waitFor (unsigned int secs) {
	unsigned int retTime = time(0) + secs;
	while (time(0) < retTime);
}

//sends out alerts whenever we enter/exit a critical zone
void criticalAlert(int direction, char whichFile[15], time_t time) {
	long long ourTime = (long long)time;
	char alert[200];
	if (direction)
		sprintf(alert, "%d entering %s critical section at system time %lld", getpid(), whichFile, ourTime);
	else
		sprintf(alert, "%d exiting %s critical section at system time %lld", getpid(), whichFile, ourTime);
	perror(alert);
}

int main(int argc, char *argv[]) {
	//this section of code allows us to print the program name in error messages
	char programName[100];
	strcpy(programName, argv[0]);
	if (programName[0] == '.' && programName[1] == '/') {
		memmove(programName, programName + 2, strlen(programName));
	}
	
	int shmid;
    file_entry *entries; //allows us to request shared memory data
	
	int startIndex = atoi(argv[1]); //get our arguments
	int duration = atoi(argv[2]);
	
	//connect to shared memory
	if ((shmid = shmget(1094, sizeof(file_entry) + 256, IPC_CREAT | 0666)) == -1) {
        errorMessage(programName, "Function shmget failed. ");
    }
	//attach to shared memory
	entries = (file_entry*) shmat(shmid, 0, 0);
	if (entries == NULL) {
		errorMessage(programName, "Function shmat failed. ");
	}

	//connect to semaphores
	sem_t *semP = sem_open("mutexP", O_RDWR);
	if (semP == SEM_FAILED) {
        errorMessage(programName, "Unable to open semaphore ");
    }
	sem_t *semN = sem_open("mutexN", O_RDWR);
	if (semN == SEM_FAILED) {
        errorMessage(programName, "Unable to open semaphore ");
    }
		
	
	//read from shared memory
	int i, j = 0;
	for (i = startIndex; i < startIndex + duration; i++) { //for each string within our range
		int stringLength = strlen(entries->data[i]);
		char reverseString[80] = {'\0'}; //create a reverse string
		for (j = stringLength - 1; j >= 0; j--) {
			reverseString[stringLength - j - 1] = entries->data[i][j];
		}
		int flag = 1;
		for(j = 0; j < stringLength; j++) {
			if (reverseString[j] != entries->data[i][j]) { //compare original and reverse strings
				flag = 0; //if we find a difference between original and reverse string, set flag to 0
			}
		}
		
		int pid = getpid();
		if (flag == 1) { //if no difference, we have a palindrome
			sem_wait(semP); //critical zone starts
			criticalAlert(1, "palindrome", time(0)); //send alert via stderr
			waitFor(randomNum()); //required wait before file processing
			FILE *pOut;
			pOut = fopen("palin.out", "a");
			fprintf(pOut, "%d\t%d\t%s\n", pid, i, entries->data[i]);
			fclose(pOut);
			waitFor(randomNum()); //required wait after file processing
			criticalAlert(0, "palindrome", time(0)); //send alert via stderr
			sem_post(semP); //critical zone ends
		
		} else { //if there is a diference, we do not have a palindrome
			sem_wait(semN); //critical zone starts
			criticalAlert(1, "non-palindrome", time(0)); //send alert via stderr
			waitFor(randomNum()); //required wait before file processing
			FILE *nOut;
			nOut = fopen("nopalin.out", "a");
			fprintf(nOut, "%d\t%d\t%s\n", pid, i, entries->data[i]);
			fclose(nOut);
			waitFor(randomNum()); //required wait after file processing
			criticalAlert(0, "non-palindrome", time(0)); //send alert via stderr
			sem_post(semN); //critical zone ends			
		}
	}

	return 0;
}
