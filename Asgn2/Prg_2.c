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
* gcc -Wall -lm -std=c99 -pthread -o Prg_2.out Prg_2.c -lrt
*
* Run with:
* ./Prg_2.out or "make run2"
* 
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
#include <ctype.h>
#include <signal.h>

//defines
#define INPUTFILE "Topic2_Prg_2.txt"
#define OUTPUTFILE "output_topic2.txt"
#define LINELENGTH 256

//structs
typedef struct
{
	int maxProcess, maxResources;
	int processes[100], allot[100][100], need[100][100], available[100], currentavailable[100], max[100][100];

}arrays_t;

//boolean definitions
#define FALSE 0
#define TRUE 1

//MY GLOBAL VARS
bool debugmode = FALSE; //flag for logging of extra information

/*Declarations*/
void fileReader(arrays_t *filearray);
void signalHandler(int signalNum) ;



int main(int argc, char*argv[])
{

	arrays_t myarrays;
	fileReader(&myarrays); 

	int maxProcess = myarrays.maxProcess;
	int maxResources = myarrays.maxResources;

	//testing by displaying the values of an array
	if(debugmode)
	{
		for(int i=0; i<maxProcess;i++)
		{
			for(int j=0; j<maxResources;j++)
			{
				printf("%i ", myarrays.need[i][j]);
			}
			printf("\n");
		}
		printf("\n");
	}

	//process for copying available resources array into current availableresources array
	for(int j=0; j<maxResources;j++)
	{
		myarrays.currentavailable[j] = myarrays.available[j];
	}

	//actual processing
	int processCount = 0;
	int safeSequence[maxProcess];
	int finished[maxProcess];
	bool isSafe = FALSE;
	for(int i=0; i<maxProcess; i++)
	{
		finished[i] = 0;
	} 

	while(processCount < maxProcess)
	{
		bool found = FALSE; 
		for(int i=0; i<maxProcess;i++)//go through each row in the need. If a pass goes through and find nothing, unsafe
		{
			if(finished[i] == FALSE)
			{

				int j;
				for(j=0; j<maxResources;j++)
				{
					if(myarrays.need[i][j] > myarrays.currentavailable[j])
					{
						break;
					}
				}

				if(j==maxResources)
				{
					if(debugmode)printf("PROCESS SAFE SO FAR: %i \n", i);
					for(int k = 0; k < maxResources; k++)
					{
						myarrays.currentavailable[k] = myarrays.currentavailable[k] + myarrays.allot[i][k];
					}
					safeSequence[processCount++] = i;
					finished[i] = TRUE;
					found = TRUE;

					if(debugmode)
					{
						for(int z = 0; z < maxResources; z++)
						{
							printf("%i ", myarrays.currentavailable[z]);
						}
						printf("\n");
					}

				}
			}
		}
		if(found == FALSE)
		{
			isSafe = FALSE;
			printf("SEQUENCE UNSAFE:\n");
			for(int i = 0; i<maxProcess;i++)
			{
				printf("PROCESS ORDER: %i \n", safeSequence[i]);
			}
			return -1;
		}
	}

	isSafe = TRUE;

	printf("SUCCESS! SAFE SEQUENCE IS: \n");
	for(int i = 0; i<maxProcess;i++)
	{
		printf("PROCESS ORDER: %i \n", safeSequence[i]);
	}


	//Output to file
	FILE* file; 
	file = fopen(OUTPUTFILE, "w");

	if(isSafe)
	{
		fprintf(file, "SUCCESS! \nSafe process sequence is: < ");
	}
	else
	{
		fprintf(file, "FAILED! \nUnsafe process sequence is: < ");
	}

	for(int i = 0; i<maxProcess;i++)
	{
		fprintf(file, "P%i", safeSequence[i]);
		if(i<maxProcess-1)
		{
			fprintf(file, ",");
		}
	}
	fprintf(file, " > \n");


	/* Raise a signal to mark completion of writing to output file */
	signal(SIGUSR1, signalHandler);
	raise(SIGUSR1);
	return 0;
}


void fileReader(arrays_t *filearray)
{

	filearray->maxProcess = 0;
	filearray->maxResources = 0;

	FILE* file;
	file = fopen(INPUTFILE, "r");
	int lineCount = 0;

	while(!feof(file))
	{
		char *temparray[100];
		char oneLineString[LINELENGTH] = "";
		char actualString[LINELENGTH] = "";
		fgets(oneLineString,LINELENGTH,file);
		int i=0;
		int j=0;
		lineCount++;
		
		while(oneLineString[i])
		{
			if(isspace(oneLineString[i]))
			{
				i++;
			}
			else
			{
				if(isspace(oneLineString[i-1]) && j>0)
				{
					actualString[j] = ',';
					j++;
				}
				actualString[j] = oneLineString[i];
				j++;
				i++;
			}
		}
		if(debugmode)printf("%s \n", actualString);

		//check for how many resources
		if(lineCount == 2)
		{
			char *token;
			int counter=0;
			int z =1;
			token = strtok(actualString, ",");

			while(token != NULL)
			{
				temparray[counter++] = token;
				if(debugmode)printf("token: %s \n", token);
				token = strtok(NULL, ",");
			}
			if(debugmode)printf("temparray %s \n", temparray[0]);

			while(strcmp(temparray[0], temparray[z]) != 0)
			{
				if(debugmode)printf("comparing %s %s \n", temparray[0], temparray[z]);
				z++;
			}
			filearray->maxResources = z;
			if(debugmode)printf("max resource %i \n", z);
		}

		//allocate stuff into array
		if(lineCount > 2)
		{
			char *token;
			int counter=lineCount-3;
			token = strtok(actualString, ",");
			while(token != NULL)
			{
				if(debugmode)printf("COUNTER: %i\n", counter);

				//processes array
				filearray->processes[counter] = atoi(&token[1]);
				if(debugmode)
				{
					printf("%i \n", atoi(&token[1]));
					printf("token process %i \n", filearray->processes[counter]);
				}
				
				//allot array
				for(int z =0; z<filearray->maxResources; z++)
				{
					token = strtok(NULL, ",");
					filearray->allot[counter][z] = atoi(token);
					if(debugmode)printf("token allot %i \n", filearray->allot[counter][z]);
				}

				//need array
				for(int z =0; z<filearray->maxResources; z++)
				{
					token = strtok(NULL, ",");
					filearray->need[counter][z] = atoi(token);
					if(debugmode)printf("token need %i \n", filearray->need[counter][z]);
				}

				//available reources array
				if(lineCount == 3)
				{
					for(int z =0; z<filearray->maxResources; z++)
					{
						token = strtok(NULL, ",");
						filearray->available[z] = atoi(token);
						if(debugmode)printf("token available %i \n", filearray->available[z]);
					}
				}

				token = strtok(NULL, ",");
				counter++;
			}
		}
	}

	fclose(file);

	filearray->maxProcess = lineCount-3;

	if(debugmode)
	{
		printf("%i %i \n", filearray->maxProcess, filearray->maxResources);
		for(int i =0; i<filearray->maxProcess; i++)
		{
			printf("inside process array %i \n", filearray->processes[i]);
			for(int j =0; j<filearray->maxResources; j++)
			{
				printf("inside allot array %i \n", filearray->allot[i][j]);
				printf("inside need array %i \n", filearray->need[i][j]);
				printf("inside available array %i \n", filearray->available[j]);
			}
		}
		printf("\n");
	}
}

void signalHandler(int signalNum) 
{
	if (signalNum == SIGUSR1)
		printf("Writing to output_topic2.txt has finished!\n");
	else
	{
		printf("Incorrect signal received. Exiting!\n");
		exit(1);
	}
}