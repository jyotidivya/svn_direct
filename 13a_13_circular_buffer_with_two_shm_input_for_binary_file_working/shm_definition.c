/**
 	Definitions for functions to initialize the buffer , write to the buffer, read from it, checking free space, clearing buffer
	and size of data present 
 */

#include "shm_header.h"

// function definition to initialize the circular buffer 
int SHM_Buffer_Initialize(CircularBufferMetadata *pBufferMetadata, char *pSHMCircularBuffer)
{
	pBufferMetadata -> shmStart = pSHMCircularBuffer;

	pBufferMetadata -> shmEnd = pSHMCircularBuffer + (SHM_SIZE - 1);

	pBufferMetadata -> write = pSHMCircularBuffer; 

	pBufferMetadata -> read = pSHMCircularBuffer;

	pBufferMetadata -> count = 0;

	return(EXIT_SUCCESS);
}


// function definition to write to the buffer
int Write_Into_SHM_Buffer(CircularBufferMetadata *pBufferMetadata, char *writeData, int numberOfBytesRead)
{

	while(numberOfBytesRead)
	{
		
		memcpy(pBufferMetadata -> write, writeData, 1);
		
		pBufferMetadata -> write = pBufferMetadata -> write + 1;

		if(pBufferMetadata -> write == (pBufferMetadata -> shmEnd +1))
		{
			pBufferMetadata -> write = pBufferMetadata -> shmStart;
		}
		
		pBufferMetadata -> count++;
		writeData++;
		numberOfBytesRead--;
	}
	
	return(EXIT_SUCCESS);
}


// function definition to read from the buffer
int Read_From_Buffer(CircularBufferMetadata *pBufferMetadata, char * readData, int validData)
{
	
	while(validData)
	{
	
		if(memcpy(readData, pBufferMetadata -> read, 1) == NULL)
		{
			perror("memcpy failed()\n");
			exit(EXIT_FAILURE);
		}

		pBufferMetadata -> read = pBufferMetadata -> read + 1;

		if(pBufferMetadata -> read == (pBufferMetadata -> shmEnd + 1))
		{
			pBufferMetadata -> read = pBufferMetadata -> shmStart;
		}

		pBufferMetadata -> count--;
		readData++;
		validData--;

	}

	return(EXIT_SUCCESS);
}


// check the present data
int Present_Valid_Data(CircularBufferMetadata *pBufferMetadata)
{
	return(pBufferMetadata -> count);
}


//checking free space available in the buffer
int Free_Space_Present(CircularBufferMetadata *pBufferMetadata)
{
	return(SHM_SIZE - pBufferMetadata -> count);
}


//remapping metadata to its own address space
int SHM_Metadata_Buffer_Remap(CircularBufferMetadata *pBufferMetadata, char *pSHMCircularBuffer) 
{
	if(pSHMCircularBuffer == pBufferMetadata -> shmStart)
	{
		printf("Already mapped to process's virtual address\n");
		return(EXIT_SUCCESS);
	}

	int writerUpdatedOffset = pBufferMetadata -> write - pBufferMetadata -> shmStart;
	int readerUpdatedOffset = pBufferMetadata -> read - pBufferMetadata -> shmStart;

	pBufferMetadata -> shmStart = pSHMCircularBuffer;
	pBufferMetadata -> shmEnd = pSHMCircularBuffer + (SHM_SIZE - 1);
	pBufferMetadata -> write = pBufferMetadata -> shmStart + writerUpdatedOffset;
	pBufferMetadata -> read = pBufferMetadata -> shmStart + readerUpdatedOffset;

	return(EXIT_SUCCESS);
}
