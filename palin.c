#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>


#define SHSIZE 100


int main(int argc, char *argv[]) {
	printf("plain checkpoint 1\n");
	int shmid;
	key_t key;
	char * shm;
	char *s;
	printf("plain checkpoint 2\n");
	
	key = 1094;
	shmid = shmget(key, SHSIZE, 0666);
	printf("plain checkpoint 3\n");
	
	if(shmid < 0) {
		perror("shmget");
		exit(1);
	}
	printf("plain checkpoint 4\n");
	
	shm = shmat(shmid, NULL, 0); //attach to shared memory
	printf("plain checkpoint 5\n");

	if(shm == (char *) -1) {
		perror("shmat");
		exit(1);
	}
	printf("plain checkpoint 6\n");
	
	for(s = shm; *s != 0; s++) {
		printf("%c", *s);
	}	
	printf("plain checkpoint 7\n");
	
	printf("\n");
	*shm = '*';
	printf("plain checkpoint 8\n");
	
	return 0;
}