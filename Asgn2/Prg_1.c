/*
* 
*48450 - Real-time Operating Systems Autumn 2018
*Author: John Trinidad
*Student ID: 99141145
*
* To make this program run you have to make sure that
* - make sure to have all of the file in one folder/location
* -run "make" to compile both Prg_1 and Prg_2
* 
* 
* To compile manually:
* gcc -Wall -lm -std=c99 -pthread-o Prg_1.out Prg_1.c -lrt
*
* Run with:
* ./Prg_1.out or "make run"
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
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>

//defines
#define OUTPUTFILE "output.txt"
#define FIFONAME "/tmp/myfifo"
#define LINELENGTH 256

//structs
typedef struct
{
	int PID;
	float arriveTime, burstTime, startTime, remainingBurst, waitTime, turnaroundTime;
}process_t;

typedef struct 
{
	int processNumber;
	pthread_t threadA, threadB; 
	pthread_mutex_t mutex;
	sem_t semA, semB;
	process_t *processArray;
	int breaker;
}thread_t;


//boolean definitions
#define FALSE 0
#define TRUE 1

//MY GLOBAL VARS
bool debugmode = FALSE; //flag for logging of extra information

/*Declarations*/
void *threadA(void *param);/*threads call this function*/
void *threadB(void *param);/*threads call this function*/
void initialiseProcesses(process_t *processArray);
void initializeData(thread_t *threadAttr);
void clearResources(thread_t *threadAttr);


int main(int argc, char*argv[])
{
	thread_t threadAttr;
	pthread_attr_t attr; 

	threadAttr.processNumber = 7;
	threadAttr.processArray = malloc(sizeof(process_t)*threadAttr.processNumber);
	initialiseProcesses(threadAttr.processArray);
	if(debugmode)printf("process array access check: %i %f %f \n", threadAttr.processArray[0].PID, threadAttr.processArray[0].arriveTime, threadAttr.processArray[0].burstTime);
	
	initializeData(&threadAttr);
	pthread_attr_init(&attr);

	//thread and fifo creation
	if (mkfifo(FIFONAME, 0666) == 0)
	{
		printf("FIFO created\n");
	}
	else
	{
		perror("Error in FIFO");
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

	pthread_join (threadAttr.threadA,NULL);
	if(debugmode) printf("thread A done\n");
	pthread_join (threadAttr.threadB,NULL);
	if(debugmode) printf("thread B done\n");

	clearResources(&threadAttr);

	return 0;
}



void *threadA(void *param)
{
	thread_t *threadAttr = param;
	int PIDcurrent, CPUcounter, fifofd;
	int PIDcounter = 0;
	float avgTurnaroundTime = 0;
	float avgWaitTime = 0;

	sem_wait(&threadAttr->semA);

	if(debugmode)printf("this is thread A \n");

	//this is the SRTF
	for(CPUcounter = 0; PIDcounter < threadAttr->processNumber; CPUcounter++)
	{
		float lowestArriveBurst = 99999.0; //high temp value
		PIDcurrent = -1; //negative value until process hits

		for(int i = 0; i < threadAttr->processNumber; i++)
		{
			float nextArriveTime = threadAttr->processArray[i].arriveTime;
			float nextArriveBurst = threadAttr->processArray[i].remainingBurst;
			if(debugmode)printf("%f \n", nextArriveBurst);
			if(nextArriveTime <= CPUcounter && nextArriveBurst < lowestArriveBurst && nextArriveBurst > 0)
			{
				lowestArriveBurst = nextArriveBurst;
				PIDcurrent = threadAttr->processArray[i].PID;
				if(debugmode)printf("picked PID: %i\n", PIDcurrent);
			}
		}

		if(PIDcurrent != -1)
		{
			threadAttr->processArray[PIDcurrent-1].remainingBurst--;

			if(threadAttr->processArray[PIDcurrent-1].remainingBurst == 0)
			{
				PIDcounter++;

				float tempBeginTime = threadAttr->processArray[PIDcurrent-1].arriveTime;
				float tempBurstTime = threadAttr->processArray[PIDcurrent-1].burstTime;
				avgWaitTime = avgWaitTime + (CPUcounter+1) - tempBeginTime -tempBurstTime;
				avgTurnaroundTime = avgTurnaroundTime + (CPUcounter+1) - tempBeginTime;
			}
		}
		
		
		if(debugmode)printf("%i %i %i \n\n", CPUcounter, PIDcurrent, PIDcounter);
		//CPUcounter++;
	}
	avgWaitTime = avgWaitTime/threadAttr->processNumber;
	avgTurnaroundTime = avgTurnaroundTime/threadAttr->processNumber;
	printf("CPU counter time: %i \n", CPUcounter);
	printf("Average Wait Time: %f \n", avgWaitTime);
	printf("Average Turnaround Time: %f \n", avgTurnaroundTime);

	//print to fifo
	char strAvgWaitTime[LINELENGTH], strAvgTurnaroundTime[LINELENGTH]; 
	sprintf(strAvgWaitTime, "%f", avgWaitTime);
	sprintf(strAvgTurnaroundTime, "%f", avgTurnaroundTime);

	sem_post(&threadAttr->semB); //init thread B
	if(debugmode)printf("begin step 1\n");
	fifofd = open(FIFONAME, O_WRONLY);

	write(fifofd, strAvgWaitTime, strlen(strAvgWaitTime)+1);
	close(fifofd);
	
	sem_wait(&threadAttr->semA);

	if(debugmode)printf("begin step 2\n");
	sem_post(&threadAttr->semB);
	fifofd = open(FIFONAME, O_WRONLY);
	write(fifofd, strAvgTurnaroundTime, strlen(strAvgTurnaroundTime)+1);
	
	close(fifofd);
	threadAttr->breaker = TRUE;
	sem_post(&threadAttr->semB);

	pthread_exit(NULL);
}


void *threadB(void *param)
{
	thread_t *threadAttr = param;
	int fifofd;
	char strOut[80];
	int tempcounter = 1;
	FILE* file; 
	file = fopen(OUTPUTFILE, "w");
	threadAttr->breaker = FALSE;

	if(file != NULL)
	{
		while(threadAttr->breaker == FALSE)
		{
			sem_wait(&threadAttr->semB);
			if(threadAttr->breaker == FALSE)
			{
				fifofd = open(FIFONAME, O_RDONLY);
				read(fifofd, strOut, sizeof(strOut)+1);
				if(debugmode)printf("TIME OUTPUT: %s \n", strOut);
				if(tempcounter == 1)
				{
					fprintf(file, "Average Wait Time: ");
				}
				else if(tempcounter == 2)
				{
					fprintf(file, "Average Turnaround Time: ");
				}
				fprintf(file, "%s\n", strOut);
				tempcounter++;
				close(fifofd);
			}
			sem_post(&threadAttr->semA);

		}
	}
	else
	{
		printf("Error creating src.txt file. Exiting \n");
		clearResources(threadAttr);
		exit(1);
	}
	printf("Successfully written times in output file! \n");
	fclose(file);
	pthread_exit(NULL);
}


void initialiseProcesses(process_t *processArray)
{
	processArray[0].PID=1; processArray[0].arriveTime=8; processArray[0].burstTime=10; processArray[0].remainingBurst=10;
	processArray[1].PID=2; processArray[1].arriveTime=10; processArray[1].burstTime=3; processArray[1].remainingBurst=3;
	processArray[2].PID=3; processArray[2].arriveTime=14; processArray[2].burstTime=7; processArray[2].remainingBurst=7;
	processArray[3].PID=4; processArray[3].arriveTime=9; processArray[3].burstTime=5; processArray[3].remainingBurst=5;
	processArray[4].PID=5; processArray[4].arriveTime=16; processArray[4].burstTime=4; processArray[4].remainingBurst=4;
	processArray[5].PID=6; processArray[5].arriveTime=21; processArray[5].burstTime=6; processArray[5].remainingBurst=6;
	processArray[6].PID=7; processArray[6].arriveTime=26; processArray[6].burstTime=2; processArray[6].remainingBurst=2;
	if(debugmode)
	{
		printf("%i %f %f %f \n", processArray[0].PID,processArray[0].arriveTime,processArray[0].burstTime, processArray[0].remainingBurst);
		printf("%i %f %f %f \n", processArray[1].PID,processArray[1].arriveTime,processArray[1].burstTime, processArray[1].remainingBurst);
		printf("%i %f %f %f \n", processArray[2].PID,processArray[2].arriveTime,processArray[2].burstTime, processArray[2].remainingBurst);
		printf("%i %f %f %f \n", processArray[3].PID,processArray[3].arriveTime,processArray[3].burstTime, processArray[3].remainingBurst);
		printf("%i %f %f %f \n", processArray[4].PID,processArray[4].arriveTime,processArray[4].burstTime, processArray[4].remainingBurst);
		printf("%i %f %f %f \n", processArray[5].PID,processArray[5].arriveTime,processArray[5].burstTime, processArray[5].remainingBurst);
		printf("%i %f %f %f \n", processArray[6].PID,processArray[6].arriveTime,processArray[6].burstTime, processArray[6].remainingBurst);
	}
}


void initializeData(thread_t *threadAttr)
{
	sem_init(&threadAttr->semA, 0, 1);
	sem_init(&threadAttr->semB, 0, 0);
}


void clearResources(thread_t *threadAttr)
{
	sem_destroy(&threadAttr->semA);
	sem_destroy(&threadAttr->semB);;
	pthread_mutex_destroy(&threadAttr->mutex);
	free(threadAttr->processArray);
	unlink(FIFONAME);
}