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
#include <fcntl.h>
#include <signal.h>
#include "sharedMemory.h"

int shmid;

pid_t pidList[19] = { 0 }; //list of all pids

//called whenever we terminate program. Unlinks semaphores, kills child processes, destroys shared memory,
//and, if resulting from an error, destroys master process
void endAll(int error) {
	//unlink semaphores
	sem_unlink("mutexP");
	sem_unlink("mutexN");
	
	//kill child processes
	int i;
	for(i = 0; i < 19; i++) {
		if (pidList[i] > 0) { //if there is an actual pid stored here
			kill(pidList[i], SIGKILL); //this should kill all children
			waitpid(pidList[i], NULL, WUNTRACED);
		}
	}

    //destroy shared memory
	int ctl_return = shmctl(shmid, IPC_RMID, NULL);
	if (ctl_return == -1) {
		perror(" Error with shmctl command: Could not remove shared memory ");
		exit(1);
	}
	
	//destroy master process
	if (error)
		kill(-1*getpid(), SIGKILL);	
}

//called when interupt signal (^C) is called
void intHandler(int dummy) {
	printf(" Interupt signal received.\n");
	endAll(1);
}

//handles the 2 second timer force stop - based on textbook code as instructed by professor
static void myhandler(int s) {
    char message[46] = "Program reached 2 minute time limit. Program ";
    int errsave;
    errsave = errno;
    write(STDERR_FILENO, &message, 45);
    errno = errsave;
	endAll(1);
}

//function taken from textbook as instructed by professor
static int setupinterrupt(void) { //set up myhandler for  SIGPROF
    struct sigaction act;
    act.sa_handler = myhandler;
    act.sa_flags = 0;
    return (sigemptyset(&act.sa_mask) || sigaction(SIGPROF, &act, NULL));
}

//function taken from textbook as instructed by professor
static int setupitimer(void) { // set ITIMER_PROF for 2-minute intervals
    struct itimerval value;
    value.it_interval.tv_sec = 120;
    value.it_interval.tv_usec = 0;
    value.it_value = value.it_interval;
    return (setitimer(ITIMER_PROF, &value, NULL));
}

//takes in program name and error string, and runs error message procedure
void errorMessage(char programName[100], char errorString[100]){
	char errorFinal[200];
	sprintf(errorFinal, "%s : error : %s", programName, errorString);
	perror(errorFinal);
	endAll(1); //releases resources and terminates all processes
}

//a function to check if string contains ending whitespace, and if so remove it
void removeSpaces(char* s) {
	int length = strlen(s);
	char test = s[length - 1];
	if (isspace(test)) {
		s[strlen(s)-1] = '\0'; //remove ending whitespace
	}
}

int main(int argc, char *argv[]) {
	signal(SIGINT, intHandler); //signal processing
	
	sem_unlink("mutexP"); //just in case something went wrong last run, the semaphores will still be available this time
	sem_unlink("mutexN");		
	
	//this section of code allows us to print the program name in error messages
	char programName[100];
	strcpy(programName, argv[0]);
	if (programName[0] == '.' && programName[1] == '/') {
		memmove(programName, programName + 2, strlen(programName));
	}
	
	//set up 2 minute timer
   if (setupinterrupt()) {
		errno = 125;
		errorMessage(programName, "Failed to set up 2 minute timer. ");
    }
    if (setupitimer() == -1) {
		errno = 125;
		errorMessage(programName, "Failed to set up 2 minute timer. ");
    }
	
	char inputFileName[] = "input.txt";
	int maxKidsTotal = 4;
	int maxKidsAtATime = 19; //we must never have more then 20 processes running, meaning 19 kids + 1 parent
	
	//first we process the getopt arguments
	int option;
	while ((option = getopt(argc, argv, "hn:i:")) != -1) {
		switch (option) {
			case 'h' :	printf("Help page for OS_Klein_project3\n"); //for h, we print helpful information about arguments to the screen
						printf("Consists of the following:\n\tTwo .c files titled master.c and palin.c\n\tOne .h file titled sharedMemory.h\n\tOne Makefile\n\tOne README.md file\n\tOne version control log.\n");
						printf("The command 'make' will run the makefile and compile the program\n");
						printf("Command line arguments for master executable:\n");
						printf("\t-i\t<inputFileName>\t\tdefaults to input.txt\n");
						printf("\t-n\t<maxTotalChildren>\tdefaults to 4\n");
						printf("\t-h\t<NoArgument>\n");
						printf("Version control acomplished using github. Log obtained using command 'git log > versionLog.txt'\n");
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
		errorMessage(programName, "Cannot open input file. ");
	}

    file_entry *entries; //allows us to request shared memory data

	//connect to shared memory
	if ((shmid = shmget(1094, sizeof(file_entry) + 256, IPC_CREAT | 0666)) == -1) {
		errorMessage(programName, "Function shmget failed. ");
    }
	//attach to shared memory
    entries = (file_entry *) shmat(shmid, 0, 0);
    if (entries == NULL ) {
        errorMessage(programName, "Function shmat failed. ");
    }
    int i = 0;
	char buffer[81];
	int numOfLines = 0;
	while (fgets(buffer, 81, input) != NULL) { //read from file...
		//remove new line character
		char *p = buffer;
		if (p[strlen(p)-1] == '\n') {	
			p[strlen(p)-1] = 0;
		}
		sprintf(entries->data[i], "%s", p); //...and save to shared memory
		i++;
		numOfLines++;
	}
	fclose(input);
	
	if (numOfLines == 0) { //if input file is empty, send an error message
		errno = 11;
		errorMessage(programName, "Input file is empty ");
	}

	//initialize semaphore
	sem_t *semP = sem_open("mutexP", O_CREAT | O_EXCL, 0644, 1);
	if (semP == SEM_FAILED) {
		errorMessage(programName, "Error creating semaphore ");
	}
	sem_t *semN = sem_open("mutexN", O_CREAT | O_EXCL, 0644, 1);
	if (semN == SEM_FAILED) {
		errorMessage(programName, "Error creating semaphore ");
	}
	sem_close(semP); //we won't use them in master process, so close for now
	sem_close(semN);
	
	//clear our output files
	FILE *pOut;
	pOut = fopen("palin.out", "w");
	fclose(pOut);
	FILE *nOut;
	nOut = fopen("nopalin.out", "w");
	fclose(nOut);			
	
	int startIndex = -5;
	int duration = 5;
	int done = 0;
	int numKidsRunning = 0;
	int numKidsBorn = 0;
	int returnValue;
	int numKidsDone = 0;
	
	//began the process of creating children
	while ((numKidsDone < maxKidsTotal) && !(done == 1 && numKidsRunning == 0)) {
		if (numKidsDone >= maxKidsTotal) {
			
		}
		returnValue = waitpid(-1, NULL, WNOHANG); //see if any processes have terminated
		if (returnValue > 0) { //a child has ended
			numKidsRunning -= 1;
			for (i = 0; i < 19; i++) { //remove pid from array list
				if (pidList[i] == returnValue) {
					pidList[i] = 0;
					break;
				}
			}			
			numKidsDone += 1;
		}
		
		if (done == 0 && numKidsRunning < maxKidsAtATime && numKidsRunning + numKidsDone < maxKidsTotal) { //if not done && if we can start another process
			startIndex += 5; //increment start index
			char buffer1[11];
			sprintf(buffer1, "%d", startIndex); //save start index to buffer1
			if (startIndex + duration >= numOfLines) {
				duration = numOfLines % 5; //duration is normally 5, unless there are no more lines. Then it is less
				if (duration == 0)
					duration = 5;
				done = 1;
			}
			char buffer2[11];
			sprintf(buffer2, "%d", duration); //save duration to buffer2
			pid_t pid;
			pid = fork();
			if (pid == 0) { //child
				execl ("palin", "plain", buffer1, buffer2, NULL); //send both buffers as arguments to palin
				errorMessage(programName, "execl function failed. ");
			} else { //parrent
				numKidsBorn += 1;
				numKidsRunning += 1;
				for (i = 0; i < 19; i++) { //claim a spot in the array list
					if (pidList[i] == 0) {
						pidList[i] = pid;
						break;
					}
				}
				continue;
			}		
		}
	}
	
	endAll(0); //release resources and terminate child processes
	printf("Program complete. Results written to palin.out and nopalin.out\n");
	return 0;
}