/*
* 
* 48450 - Real-time Operating Systems Autumn 2018
* Author: John Trinidad
* Student ID: 99141145
*
* To make this program run you have to make sure that
* - make sure to have all of the file in one folder/location
* -run "make" to compile both Prg_1 and Prg_2
* -run "make run" to compile both and make them run
* 
* To compile manually:
* gcc -Wall -lm -pthread -o Prg_2.out Prg_2.c -lrt
*
* Run with:
* ./Prg_2.o
*
*/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h> //this has been added or else an error would occur

int main()
{
	const int SIZE = 4096;
	const char *name = "totaltime";

	int shm_fd;
	void *ptr;
	
	if((shm_fd = shm_open(name, O_RDONLY, 0666)) < 0)
	{
		perror("Error opening shared memory. Memory could have been unlinked");
		exit(0);
	}
	if((ptr = mmap(0, SIZE, PROT_READ, MAP_SHARED, shm_fd, 0)) == MAP_FAILED)
	{
		perror("Error reading from memmory");
		exit(0);
	}
	printf("Total time of last Prg_1 execution stored in memory is: %s seconds \n",(char *)ptr);

	shm_unlink(name);

return 0;
}
