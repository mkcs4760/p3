#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h> 

int main(int argc, char *argv[]) {
	
	printf("child lives\n");
	
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
	
	return 0;
}
