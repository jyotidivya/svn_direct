/** 
	Reader is reading from the shared memory and is synchronized with writer with the help of semaphore 

	Build command :-gcc -o file_r file_r.c shm_definition.c -lpthread
 */

#include "shm_header.h"

#define BUFFER_SIZE 1024

#define SHM_OPEN_SIZE 0	//since opener of SHM
#define SHM_OPEN_FLAGS_MODE	S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP

#define SEM_OPEN_FLAGS	0 

#define FILE_PATHNAME 			"/home/divya/Desktop/test.h264"
#define FILE_OPEN_FLAGS			 O_RDWR		
#define SIZE_TO_WRITE_TO_FILE	 512

int idCB, idMetadata, fd;
char *pSHMCircularBuffer;
char *pSHMMetadata;
char *semW_name;
char *semR_name;

 
int main(int argc, char* argv[])
{
	char readData[BUFFER_SIZE];
	char c;
	CircularBufferMetadata buffer;
	CircularBufferMetadata *pBufferMetadata = &buffer;
	int numberOfBytesWritten;

	if(argc < 4)
	{
		printf("error:usage %s <shm key> <writer_sem name> <reader_sem name>\n\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	char *shm_key = argv[1];
	semW_name = argv[2];
	semR_name = argv[3];

	//catching ^C signal and changing its deposition to user defined handler
	if(signal(SIGINT, handler) == SIG_ERR)
	{
		perror("signal() failed!");
		exit(EXIT_FAILURE);
	}

	//changing string to integer
	key_t key = atoi(shm_key);

	//get the idCB of circular buffer shared memory
	idCB = shmget(key, SHM_OPEN_SIZE, SHM_OPEN_FLAGS_MODE);
	if(idCB == -1) 
	{
		perror("shmget() failed!");
		exit(EXIT_FAILURE);
 	} 
	
 	key_t key_metadata = key + 10;

 	// get the idCB of circular buffer metadata shared memory
 	idMetadata = shmget(key_metadata, SHM_OPEN_SIZE, SHM_OPEN_FLAGS_MODE);
	if(idMetadata == -1) 
	{
		perror("shmget() failed!");
		exit(EXIT_FAILURE);
 	} 


	//attaching circular buffer shared memory to reader's address space
	pSHMCircularBuffer = (char*) shmat(idCB, NULL, 0);
	if(pSHMCircularBuffer == (char *)-1) 
	{
		perror("shmat() failed!");
		exit(EXIT_FAILURE);
 	} 	
	printf("SHM Circular Buffer attached\n");


	// attaching circular buffer shared memory to reader's address space
	pSHMMetadata = (char*) shmat(idMetadata, NULL, 0);
	if(pSHMMetadata == (char*)-1) 
	{
		perror("shmat() failed!");
		exit(EXIT_FAILURE);
 	}
 	printf("SHM Metadata attached\n");


	//opening semaphore on which writer waits
	sem_t *writer_sem = sem_open(semW_name, SEM_OPEN_FLAGS);
	if(writer_sem == SEM_FAILED)
	{
		perror("sem_open for writer_sem");
		exit(EXIT_FAILURE);
	}
	printf("writer_sem opened\n");

	//opening semaphore on which reader waits
	sem_t *reader_sem = sem_open(semR_name, SEM_OPEN_FLAGS);
	if(reader_sem == SEM_FAILED)
	{
		perror("sem_open for reader_sem");
		exit(EXIT_FAILURE);
	}
	printf("reader_sem opened\n");	
	
	// typecasting shared memory to type CircularBufferMetadata to hold the structure pointed by struct_ptr
	pBufferMetadata = (CircularBufferMetadata *)pSHMMetadata;

	// open the file
	fd = open(FILE_PATHNAME, FILE_OPEN_FLAGS);
	if(fd == -1)
	{
		perror("open()failed!");
		exit(EXIT_FAILURE);
	}

	do
	{	
		//waiting on reader_sem 
		if(sem_wait(reader_sem) == -1)
		{
			perror("sem_wait() failed!");
			exit(EXIT_FAILURE);
		}

		//remapping circular buffer metadata to procees's virtual address
		SHM_Metadata_Buffer_Remap(pBufferMetadata, pSHMCircularBuffer);

		int validData = Present_Valid_Data(pBufferMetadata);
		// printf("Valid data present:%d\n", validData);

		//reading data from shm and printing to stdout
		Read_From_Buffer(pBufferMetadata, readData, validData);
		
		if(SIZE_TO_WRITE_TO_FILE > validData)
		{	
			numberOfBytesWritten = write(fd, readData, validData);
			printf("no. of bytes written:%d\n", numberOfBytesWritten);
		}
		else
		{
			numberOfBytesWritten = write(fd, readData, SIZE_TO_WRITE_TO_FILE);
			printf("no. of bytes written:%d\n", numberOfBytesWritten);
		}

		//writer_sem is released
		if(sem_post(writer_sem) == -1)
		{
			perror("sem_post() failed!");
			exit(EXIT_FAILURE);
		}
	} while(numberOfBytesWritten != 0);

	// close the file descriptor
	if(close(fd) == -1)
	{
		perror("close()!");
		exit(EXIT_FAILURE);
	}

	//detaching shm from reader's address space
	if(shmdt(pSHMCircularBuffer) == -1)
	{
		perror("shmdt() failed!");
		exit(EXIT_FAILURE);		
	}
 	
	return EXIT_SUCCESS;
}

//SIGINT handler definition
void handler(int s)
{
	//detaching shmCircular buffer from reader's address space
	if(shmdt(pSHMCircularBuffer) == -1)
	{
		perror("shmdt() failed in SIGINT handler!");
		exit(EXIT_FAILURE);		
	}

	//detaching shmbuffer metadata from reader's address space
	if(shmdt(pSHMMetadata) == -1)
	{
		perror("shmdt() failed in SIGINT handler!");
		exit(EXIT_FAILURE);		
	}

	//removing shared memory
	if(shmctl(idCB, IPC_RMID, NULL) == -1)
	{
		perror("shm remove failed!");
		exit(EXIT_FAILURE);
	}

	//removing shared memory
	if(shmctl(idMetadata, IPC_RMID, NULL) == -1)
	{
		perror("shm remove failed!");
		exit(EXIT_FAILURE);
	}

	//removing writer semaphore
	if(sem_unlink(semW_name) == -1)
	{
		perror("semW_name remove failed!");
		exit(EXIT_FAILURE);
	}

	//removing reader semaphore
	if(sem_unlink(semR_name) == -1)
	{
		perror("semR_name remove failed!");
		exit(EXIT_FAILURE);
	}

	// close the file descriptor
	if(close(fd) == -1)
	{
		perror("close()!");
		exit(EXIT_FAILURE);
	}

	//terminate the program
	exit(EXIT_SUCCESS);
}
