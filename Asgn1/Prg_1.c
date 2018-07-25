/*
* 
*48450 - Real-time Operating Systems Autumn 2018
*Author: John Trinidad
*Student ID: 99141145
*
* To make this program run you have to make sure that
* - make sure to have all of the file in one folder/location
* -run "make" to compile both Prg_1 and Prg_2
* -run "make run" to compile both and make them run
* 
* To compile manually:
* gcc -Wall -lm -pthread -o Prg_1.out Prg_1.c -lrt
*
* Run with:
* ./Prg_1.o or ./Prg_1.o inputfilename.txt
*
*/
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h> //added for boolean functionality
#include <sys/types.h>
#include <unistd.h> // for the read and write and fork function
#include <string.h> //for strlen
#include <time.h>
#include <sys/mman.h> //this has been added for mmap
#include <fcntl.h>
#include <sys/shm.h>

//mex length for each line
#define LINELENGTH 256

//boolean definitions
#define FALSE 0
#define TRUE 1

//status definitions
#define INIT 1
#define PROCESSING 2
#define FINALISING 3
#define FINISH 4

//billion definition to calculate time
#define BILLION 1E9

#define HEADEREND "end_header"
#define INPUTFILE "data.txt"
#define OUTPUTFILE "src.txt"

/********************************************/
//MY GLOBAL VARS
bool debugmode = TRUE; //flag for logging of extra information

struct ThreadAttr
{
	int status;
	int pipefd[2];
	bool isEOF;
	char pipeOutput[LINELENGTH];
	pthread_mutex_t mutex;
	sem_t write, read, justify;
	pthread_t threadA, threadB, threadC; 
	char *filename;
};

/********************************************/


/*DECLARATIONS*/
void *threadA(void *param);/*threads call this function*/
void *threadB(void *param);/*threads call this function*/
void *threadC(void *param);/*threads call this function*/
void writeTime(struct ThreadAttr *threadAttr, double totaltime);
void initializeData(struct ThreadAttr *threadAttr);
void printStatus(int status);
void clearResources(struct ThreadAttr *threadAttr);

int main(int argc, char*argv[])
{
	struct ThreadAttr threadAttr;
	pthread_attr_t attr; 

	//clock_t starttime, endtime;
	struct timespec timestart;
	struct timespec timeend;
	double totaltime;
	if(argc == 2)
	{
		printf("Custom input filename provided. Using this file.\n");
		threadAttr.filename = argv[1];
	}
	else if (argc == 1)
	{
		printf("No custom input filename provided. Using default data.txt.\n");
		threadAttr.filename = INPUTFILE;
	}
	else
	{
		printf("Too many arguments.\n");
		printf("Please enter input file ame or leave blank to use default filename.\n");
		exit(1);
	}	
	
	//starttime = clock();
	clock_gettime(CLOCK_REALTIME, &timestart);

	initializeData(&threadAttr);
	pthread_attr_init(&attr);

	if ( pipe(threadAttr.pipefd) < 0 )
	{
    	perror("Error creating pipe");
    	clearResources(&threadAttr);
		exit(1);
	}

	if (pthread_create(&threadAttr.threadA,&attr,threadA,&threadAttr) == 0)
	{
		printf("Thread A created\n");
	}
	else
	{
		perror("Error in Thread A");
		clearResources(&threadAttr);
		exit(1);
	}

	if (pthread_create(&threadAttr.threadB,&attr,threadB,&threadAttr) == 0)
	{
		printf("Thread B created\n");
	}
	else
	{
		perror("Error in Thread B");
		clearResources(&threadAttr);
		exit(1);
	}

	if (pthread_create(&threadAttr.threadC,&attr,threadC,&threadAttr) == 0)
	{
		printf("Thread C created\n");
	}
	else
	{
		perror("Error in Thread C");
		clearResources(&threadAttr);
		exit(1);
	}

	
	pthread_join (threadAttr.threadA,NULL);
	if(debugmode) printf("thread A done\n");
	pthread_join (threadAttr.threadB,NULL);
	if(debugmode) printf("thread B done\n");
	pthread_join (threadAttr.threadC,NULL);
	if(debugmode) printf("thread C done\n");

	printStatus(FINALISING);

	//free resources
	clearResources(&threadAttr);

	//do clock calcs
	//endtime = clock();
	clock_gettime(CLOCK_REALTIME , &timeend);
	//printf("start:");
	//totaltime = ((double)(endtime - starttime)) / CLOCKS_PER_SEC;
	totaltime = ((double)(timeend.tv_nsec - timestart.tv_nsec)) / BILLION;
	writeTime(&threadAttr, totaltime);
	printStatus(FINISH);
	printf("Program runtime %f seconds\n", totaltime);
return 0;
}


void *threadA(void *param)
{
	struct ThreadAttr *threadAttr = param;
	FILE* file;
	char pipeInput[LINELENGTH];

	//file = fopen(inputFile, "r");
	file = fopen(INPUTFILE, "r");

	if(file != NULL)
	{
		printStatus(PROCESSING);
		// while((fgets(pipeInput,LINELENGTH,file)) != NULL)
		while(!feof(file))
		{
			sem_wait(&threadAttr->read);
			pthread_mutex_lock(&threadAttr->mutex);
			fgets(pipeInput,LINELENGTH,file);
			// if(feof(file)){
			// 	break;
			// }
			if(debugmode) printf("Thread A string: %s",pipeInput); 
			write(threadAttr->pipefd[1], pipeInput, LINELENGTH);
			pthread_mutex_unlock(&threadAttr->mutex); 
			sem_post(&threadAttr->write);
		}
		threadAttr->isEOF = TRUE;
	}
	else
	{
		perror("Error opening data.txt file. Exiting");
		clearResources(threadAttr);
		exit(1);
	}
	fclose(file);

	pthread_exit(NULL);
}

void *threadB(void *param)
{
	struct ThreadAttr *threadAttr = param;
	// while(1)
	while(!(threadAttr->isEOF))
	{
		sem_wait(&threadAttr->write);
		pthread_mutex_lock(&threadAttr->mutex);
		//if(!isEOF) activate this "if" when using strcmp
		//{
			read(threadAttr->pipefd[0], threadAttr->pipeOutput, LINELENGTH);
			if(debugmode) printf("Thread B string %s", threadAttr->pipeOutput);
		//}
		pthread_mutex_unlock(&threadAttr->mutex); 
		sem_post(&threadAttr->justify);
	}
	pthread_exit(NULL);
}

void *threadC(void *param)
{
	struct ThreadAttr *threadAttr = param;
	bool pastHeader = FALSE;
	FILE* file; 
	file = fopen(OUTPUTFILE, "w");
	

	if(file != NULL)
	{
		// while(1)
		while(!(threadAttr->isEOF))
		{
			sem_wait(&threadAttr->justify);
			pthread_mutex_lock(&threadAttr->mutex);
			//if(!isEOF) activate this "if" when using strcmp
			//{
				
				if(debugmode) printf("Thread C string: %s \n",threadAttr->pipeOutput); 
				if(pastHeader)
				{
					//printf(threadAttr->pipeOutput);
					fprintf(file, "%s", threadAttr->pipeOutput);
				}
				if (strstr(threadAttr->pipeOutput, HEADEREND) != NULL)
				//if (strcmp(threadAttr->pipeOutput, headerEnd) == 0)
				{
					pastHeader = TRUE;
				}
			//}
			
			pthread_mutex_unlock(&threadAttr->mutex); 
			sem_post(&threadAttr->read);
		}
	}
	else
	{
		printf("Error creating src.txt file. Exiting");
		clearResources(threadAttr);
		exit(1);
	}
	fclose(file);
	pthread_exit(NULL);
}

void writeTime(struct ThreadAttr *threadAttr, double totaltime)
{
	const int SIZE = 4096;
	const char *name = "totaltime"; //same as making an array e.g. name[3] = "OS"
	double timevalue = totaltime;

	int shm_fd;
	void *ptr;
	
	if((shm_fd = shm_open(name, O_CREAT | O_RDWR, 0666)) < 0)
	{
		perror("Error opening shared memory");
    	clearResources(threadAttr);
		exit(1);
	}
	
	if(ftruncate(shm_fd, SIZE) != 0)
	{
		perror("Error truncating memory");
    	clearResources(threadAttr);
		exit(1);
	}
	
	if((ptr = mmap(0, SIZE, PROT_WRITE, MAP_SHARED, shm_fd, 0)) == MAP_FAILED)
	{
		perror("Error writing to memory");
    	clearResources(threadAttr);
		exit(1);
	}

	sprintf(ptr, "%f", timevalue);
}

void initializeData(struct ThreadAttr *threadAttr) 
{
	printStatus(INIT);
	threadAttr->isEOF = FALSE;
	pthread_mutex_init(&threadAttr->mutex, NULL);
	sem_init(&threadAttr->read, 0, 1);
	sem_init(&threadAttr->write, 0, 0);
	sem_init(&threadAttr->justify, 0, 0);
}

void printStatus(int status)
{
	switch(status)
	{
		case INIT:
		printf("Initializing...\n");
		break;
		case PROCESSING:
		printf("Executing tasks...\n");
		break;
		case FINALISING:
		printf("Finishing tasks...\n");
		break;
		case FINISH:
		printf("Program finished\n");
		break;
	}
}

void clearResources(struct ThreadAttr *threadAttr)
{
	close(threadAttr->pipefd[0]);
	close(threadAttr->pipefd[1]);
	sem_destroy(&threadAttr->read);
	sem_destroy(&threadAttr->write);
	sem_destroy(&threadAttr->justify);
	pthread_mutex_destroy(&threadAttr->mutex);
}