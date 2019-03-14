//my structure to attach to shared memory
typedef struct file {
	char data[100][80];
	int numOfLines;
} file_entry;