#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <unistd.h> 
#include <errno.h>
#include <signal.h>
#include <sys/time.h>
#include <ctype.h>
#include <stdbool.h>
#include <semaphore.h>
#include <fcntl.h> //used for O_CREAT
#include "sharedMemory.h"

#define MAXLINECOUNT = 100
#define MAXLINELENGTH = 80

int shmid;



//handles the 2 second timer force stop - based on textbook code as instructed by professor
/*
static void myhandler(int s) {
    char message[41] = "Program reached 25 second limit. Program ";
    int errsave;
    errsave = errno;
    write(STDERR_FILENO, &message, 40);
    errno = errsave;
   
    //destroy shared memory
	int ctl_return = shmctl(shmid, IPC_RMID, NULL);
	if (ctl_return == -1) {
		perror("shmctl for removel: ");
		exit(1);
	}
   
    kill(-1*getpid(), SIGKILL); //kills process and all children
}
//function taken from textbook as instructed by professor
static int setupinterrupt(void) { //set up myhandler for  SIGPROF
    struct sigaction act;
    act.sa_handler = myhandler;
    act.sa_flags = 0;
    return (sigemptyset(&act.sa_mask) || sigaction(SIGPROF, &act, NULL));
}
//function taken from textbook as instructed by professor
static int setupitimer(void) { // set ITIMER_PROF for 2-second intervals
    struct itimerval value;
    value.it_interval.tv_sec = 25;
    value.it_interval.tv_usec = 0;
    value.it_value = value.it_interval;
    return (setitimer(ITIMER_PROF, &value, NULL));
}
*/
//takes in program name and error string, and runs error message procedure
void errorMessage(char programName[100], char errorString[100]){
	char errorFinal[200];
	sprintf(errorFinal, "%s : Error : %s", programName, errorString);
	perror(errorFinal);
	
	//destroy shared memory
	int ctl_return = shmctl(shmid, IPC_RMID, NULL);
	if (ctl_return == -1) {
		perror("shmctl for removel: ");
		exit(1);
	}
	kill(-1*getpid(), SIGKILL);
}

//a function to check if string contains ending whitespace, and if so remove it
void removeSpaces(char* s) {
	int length = strlen(s);
	char test = s[length - 1];
	if (isspace(test)) {
		s[strlen(s)-1] = '\0'; //remove ending whitespace
	}
}

//a function designed to read a line containing a single number and process it
int readOneNumber(FILE *input, char programName[100]) {
	char line[100];
	char *token;
	fgets(line, 100, input);
	if (line[0] == '\0') { //if there are no more lines, then we have an error
		errno = 1;
		perror("Error reading in one number");
		//errorMessage(programName, "Invalid input file format. Expected more lines then read. ");
	}
	token = strtok(line, " "); //read in a single numer
	removeSpaces(token); //remove any hanging whitespace
	int ourValue = atoi(token);
	if ((token = strtok(NULL, " ")) != NULL) { //check if there is anything after this number
		if (token[0] == '\n') { //if what remains is just new line character, no problem
			return ourValue;
		}
		else {
			return -1; //else, the function failed and -1 is returned
		}
	}
	else {
		return ourValue;
	}
}


int main(int argc, char *argv[]) {
	//this section of code allows us to print the program name in error messages
	char programName[100];
	strcpy(programName, argv[0]);
	if (programName[0] == '.' && programName[1] == '/') {
		memmove(programName, programName + 2, strlen(programName));
	}
	
	//set up 2 second timer
   /* if (setupinterrupt()) {
		errno = 125;
		errorMessage(programName, "Failed to set up 2 second timer. ");
    }
    if (setupitimer() == -1) {
		errno = 125;
		errorMessage(programName, "Failed to set up 2 second timer. ");
    }*/
	
	char inputFileName[] = "input.txt";
	//char outputFileName[] = "output.txt";
	int maxKidsTotal = 4;
	int maxKidsAtATime = 19; //we must never have more then 20 processes running, meaning 19 kids + 1 parent

	//clear our files
	FILE *pOut;
	pOut = fopen("palin.out", "w");
	fclose(pOut);
	FILE *nOut;
	nOut = fopen("nopalin.out", "w");
	fclose(nOut);			
	
	
	//first we process the getopt arguments
	int option;
	while ((option = getopt(argc, argv, "hn:i:")) != -1) {
		switch (option) {
			case 'h' :	printf("Help page for OS_Klein_project3\n"); //for h, we print data to the screen
						printf("Consists of the following:\n\tTwo .c file titled master.c and palin.c\n\tOne Makefile\n\tOne README.md file\n\tOne version control log.\n");
						printf("The command 'make' will run the makefile and compile the program\n");
						printf("Command line arguments for master executable:\n");
						printf("\t-i\t<inputFileName>\t\tdefaults to input.txt\n");
						//output file names are set to palin.out and nopalin.out
						printf("\t-n\t<maxTotalChildren>\tdefaults to 4\n");
						printf("\t-s\t<maxKidsAtATime>\tdefaults to 2\n");
						printf("\t-h\t<NoArgument>\n");
						printf("Version control acomplished using github. Log obtained using command 'git log > versionLog.txt\n");
						exit(0);
						break;
			case 'n' :	maxKidsTotal = atoi(optarg); //for n, we set the maximum number of children we will fork
						break;
			case 'i' :	strcpy(inputFileName, optarg); //for i, we specify input file name
						break;
			default :	errno = 22; //anything else is an invalid argument
						errorMessage(programName, "You entered an invalid argument. ");
		}
	}
	
	//open input file
	FILE *input;
	input = fopen(inputFileName, "r");
	if (input == NULL) {
		errno = 2;
		errorMessage(programName, "Cannot open desired file. ");
	}

	
	int shmid;
    //int n;
    file_entry *entries;

    printf("start for else\n");
	//connect to shared memory
	if ((shmid = shmget(1094, sizeof(file_entry) + 256, IPC_CREAT | 0666)) == -1) {
		printf("problem3");
        exit(2);
    }
	//attach to shared memory
    entries = (file_entry *) shmat(shmid, 0, 0);
    if (entries == NULL ) {
        printf("problem4");
		exit(2);
    }
    int i = 0;
	char buffer[81];
	int numOfLines = 0;
	while (fgets(buffer, 81, input) != NULL) {
		//remove new line character
		char *p = buffer;
		if (p[strlen(p)-1] == '\n') {	
			p[strlen(p)-1] = 0;
		}

		sprintf(entries->data[i], "%s", p);
		i++;
		numOfLines++;
	}
	fclose(input);
	printf("done attachment\n");
	
	//initialize semaphore
	//sem_init(&entries->mutex, 1, 1);
	//entries->mutex = sem_open ("sem", O_CREAT | O_EXCL, 0644, 1);
	//sem = sem_open ("pSem", O_CREAT | O_EXCL, 0644, value);
	//char *name ="rams";
	sem_t *semP = sem_open("mutexP", O_CREAT | O_EXCL, 0644, 1);
	if (semP == SEM_FAILED) {
		perror("sem_open1 error");
	}
	sem_t *semN = sem_open("mutexN", O_CREAT | O_EXCL, 0644, 1);
	if (semN == SEM_FAILED) {
		perror("sem_open2 error");
	}

	
	printf("This file has %d lines\n", numOfLines);	
	
	int startIndex = -5;
	int duration = 5;
	int done = 0;
	int numKidsRunning = 0;
	int returnValue;
	int numKidsDone = 0;
	
	while ((numKidsDone < maxKidsTotal) && !(done == 1 && numKidsRunning == 0)) {
		//printf("done equals %d and kidsRunning equals %d\n", done, numKidsRunning);
		returnValue = waitpid(-1, NULL, WNOHANG);
		//if a child has ended, return pid
		//if children are running, return 0
		//if no children are running, return -1
		if (returnValue > 0) { //a child has ended
			//write to output file the time this process ended
			//printf("Child %d ended\n", returnValue);
			numKidsRunning -= 1;
			numKidsDone += 1;
		}
		
		if (done == 0 && numKidsRunning < maxKidsAtATime) {
			startIndex += 5;
			char buffer1[11];
			sprintf(buffer1, "%d", startIndex);
			if (startIndex + duration >= numOfLines) {
				duration = numOfLines % 5;
				done = 1;
				//printf("last run\n");
			}
			char buffer2[11];
			sprintf(buffer2, "%d", duration);
			//printf("Let's send in %d and %d\n", startIndex, duration);
			if (fork() == 0) { //child

				execl ("palin", "plain", buffer1, buffer2, NULL);
				errorMessage(programName, "execl function failed. ");
			} else { //parrent
				numKidsRunning += 1;
				
				//wait(NULL);
				//shmdt(&shmid);
				//printf("end for else\n");
				continue;
			}		
		}

	}

	sem_close(semP);
	sem_close(semN);
	/* Close the semaphore as we won't be using it in the parent process */
   /* if (sem_close(sem) < 0) {
        perror("sem_close(3) failed");
        sem_unlink("mutex");
        //exit(EXIT_FAILURE);
    }*/
	//destroy shared memory
	int ctl_return = shmctl(shmid, IPC_RMID, NULL);
	if (ctl_return == -1) {
		errorMessage(programName, "Function scmctl failed. ");
	}
	
	return 0;
}