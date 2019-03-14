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
#include "sharedMemory.h"

#define MAXLINECOUNT = 100
#define MAXLINELENGTH = 80

int shmid;



//handles the 2 second timer force stop - based on textbook code as instructed by professor
static void myhandler(int s) {
    char message[41] = "Program reached 2 second limit. Program ";
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
    value.it_interval.tv_sec = 2;
    value.it_interval.tv_usec = 0;
    value.it_value = value.it_interval;
    return (setitimer(ITIMER_PROF, &value, NULL));
}

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
    if (setupinterrupt()) {
		errno = 125;
		errorMessage(programName, "Failed to set up 2 second timer. ");
    }
    if (setupitimer() == -1) {
		errno = 125;
		errorMessage(programName, "Failed to set up 2 second timer. ");
    }
	
	char inputFileName[] = "input.txt";
	//char outputFileName[] = "output.txt";
	int maxKidsTotal = 4;
	int maxKidsAtATime = 19; //we must never have more then 20 processes running, meaning 19 kids + 1 parent

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
						printf("\t-s\t<maxChildrenAtATime>\tdefaults to 2\n");
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
    
	printf("done attachment\n");
	
	
	entries->numOfLines = 0;	
	
	
	
	//used shared memory in one file now, eventually split into two once it works
    if (fork() == 0) { //child
    	//char buffer[11];
		//sprintf(buffer, "%d", fullLine[2]);
		execl ("palin", "plain", /*buffer,*/ NULL);
		errorMessage(programName, "execl function failed. ");
						
		/*
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
        sleep(1);
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
		*/
    } else {
		int i = 0;
		char buffer[81];
		//perror("checkpoint 1");
		while (fgets(buffer, 81, input) != NULL) {
			//remove EOF character
			char *p = buffer;
			//perror("checkpoint 2");
			if (p[strlen(p)-1] == '\n') {	
				p[strlen(p)-1] = 0;
			}
			//perror("checkpoint 3");
			//strcpy(&temp[i], p);
			//perror("checkpoint 4"); //we never get pasted this checkpoint
			sprintf(entries->data[i], "%s", p); //can't figure out the correct syntax. No examples of 2D char arrays online
			
			//strcpy(entries->data[i],p); //several failed attemps. Always either won't build or results in seg fault
			//sprintf(entries->data, "%s", p);
			//strcpy(entries->data[i][0], p);
			//sprintf(&entries->data[i][0], "%s", p);
			//entries->data[i][0] = temp[i][0];
			//perror("checkpoint 5");
			i++;
			entries->numOfLines++;
		}
		//perror("checkpoint 6");

		
        wait(NULL);
        shmdt(&shmid);
		printf("end for else\n");
    }
	fclose(input);
    return 0;
	
	
	
	
	//int shmid;
	//key_t key;
	//int *clockSeconds, *clockNano;
	//long clockInc = readOneNumber(input, programName);
	//set up NEW STUFF, not the stupid clock thingy
	/*char buffer[81]; //we are told that no line will contain more then 80 characters, plus one for new line char
	int result;
	int i,j = 0;

	
	//char ourStrings[100][80];
	file_entry *entries; //this is my new pointer
	int numOfLines = 0;
	
	while (fgets(buffer, 81, input) != NULL) {
		//remove EOF character
		char *p = buffer;
		if (p[strlen(p)-1] == '\n') {	
			p[strlen(p)-1] = 0;
		}
		strcpy(&ourStrings[i][0], p);
		i++;
		numOfLines++;
	}

	for (i = 0; i < numOfLines; i++) {
		for (j = 0; j < 80; ++j) {
			printf("%c", ourStrings[i][j]);
		}
		printf("\n");
	}
	
	

    fclose(input);*/

	
	
	/*
	key = 1094;
	shmid = shmget(key, sizeof(int*) + sizeof(long*), IPC_CREAT | 0666); //this is where we create shared memory
	if(shmid < 0) {
		errorMessage(programName, "Function shmget failed. ");
	}
	
	//attach ourselves to that shared memory
	clockSeconds = shmat(shmid, NULL, 0); //attempting to store 2 numbers in shared memory
	clockNano = clockSeconds + 1;
	if((clockSeconds == (int *) -1) || (clockNano == (int *) -1)) {
		errorMessage(programName, "Function shmat failed. ");
	}

	*clockSeconds = 0;
	*clockNano = 0;
	int numKidsRunning = 0;
	int numKidsDone = 0;
	bool lineWaiting = false;
	int counter = 0;
	int singleNum;
	int fullLine[3];
	char* token;
	bool endOfFile = false;
	int temp;
	FILE *output;
	output = fopen("output.test", "w");
	//loop until we're done
	while((numKidsDone < maxKidsTotal) && !((numKidsRunning == 0) && endOfFile)) { //simulated clock is incremented by parent
		//increment clock
		//waitpid to see if a child has ended
		//if (child has ended)
		//launch another child
		
		*clockNano += clockInc;
		if (*clockNano >= 1000000000) { //increment the next unit
			*clockSeconds += 1;
			*clockNano -= 1000000000;
		}
		temp = waitpid(-1, NULL, WNOHANG);
		//if a child has ended, return pid
		//if children are running, return 0
		//if no children are running, return -1
		if (temp > 0) { //a child has ended
			//write to output file the time this process ended
			fprintf(output, "Child %d ended at %d:%d\n", temp, *clockSeconds, *clockNano);
			numKidsDone += 1;
			numKidsRunning -= 1;
		}
		//if you still have kids to run and there's room to run them
		if (((numKidsDone + numKidsRunning) < maxKidsTotal) && (numKidsRunning < maxKidsAtATime)) {
			if (lineWaiting == false) { //boolean added to avoid reading same line twice or skipping a line
				char line[100];
				lineWaiting = true;
				counter = 0;
				char *value = fgets(line, 100, input); //get line of numbers
				if (value == NULL) {
					//if there are no more lines, then we reached the EOF
					endOfFile = true;
				}
				else {
					token = strtok(line, " ");
					while (token != NULL && token[0] != '\n' && counter < 3) {
						singleNum = atoi(token);
						fullLine[counter] = singleNum;
						counter++;
						token = strtok(NULL, " ");
					}
				}
			}
			if (endOfFile == false) {
				if ((*clockSeconds > fullLine[0]) || ((*clockSeconds == fullLine[0]) && (*clockNano >= fullLine[1]))) {
					//it's time to make a child
					lineWaiting = false;
					pid_t pid;
					pid = fork();
					
					if (pid == 0) { //child
						char buffer[11];
						sprintf(buffer, "%d", fullLine[2]);
						execl ("palin", "plain", buffer, NULL);
						errorMessage(programName, "execl function failed. ");
					}
					else if (pid > 0) { //parent
						numKidsRunning += 1;
						//write to output file the time this process was launched
						fprintf(output, "Created child %d at %d:%d to last %d\n", pid, *clockSeconds, *clockNano, fullLine[2]);
						continue;
					}
				}
			}
		}
	}
	fclose(output);
	
	//destroy shared memory
	printf("Parent terminating %d:%d\n", *clockSeconds, *clockNano);
	int ctl_return = shmctl(shmid, IPC_RMID, NULL);
	if (ctl_return == -1) {
		errorMessage(programName, "Function scmctl failed. ");
	}
	*/
	return 0;
}