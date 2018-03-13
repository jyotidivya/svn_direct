/**
	header file to include library files and declare structure for circular buffer
 */

#include <stdio.h>					
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>

// #define BUFFER_SIZE 1024
#define SHM_SIZE 32

typedef struct 
{
	void *shmStart;
	void *shmEnd;
	void *write;
	void *read;
	int count;
} CircularBufferMetadata;


int SHM_Buffer_Init_Write(CircularBufferMetadata *, char *);
int Write_Into_SHM_Buffer(CircularBufferMetadata *, char*, int);
int Read_From_Buffer(CircularBufferMetadata *, char *, int);
int Clear_Buffer(CircularBufferMetadata *);
int Present_Valid_Data(CircularBufferMetadata *);
int Free_Space_Present(CircularBufferMetadata *);
int SHM_Metadata_Buffer_Remap(CircularBufferMetadata *, char *);

//handler declaration
void handler(int);





