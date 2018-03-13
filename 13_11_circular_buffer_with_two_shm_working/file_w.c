/** 
	Writer is reading user inputted data and writing to shared memory

	Build command :- gcc -o file_w file_w.c shm_definition.c -lpthread
 */

#include "shm_header.h"

#define BUFFER_SIZE 1024

#define SHM_OPEN_FLAGS_MODE		IPC_CREAT|IPC_EXCL|S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP
#define SHM_OPEN_FLAGS_MODE_FOR_RM	S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP

#define SEM_OPEN_FLAGS	O_CREAT|O_EXCL 
#define SEM_OPEN_MODE	S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP	
#define SEM_INIT_VAL_WR_SEM			1
#define SEM_INIT_VAL_RD_SEM			0



int unlink_prev_shm_circularBuffer(key_t key, size_t size) 
{

	int id_tmp = shmget(key, size, SHM_OPEN_FLAGS_MODE_FOR_RM);
	if(id_tmp == -1) 
	{
		perror("shmget() failed inside unlink_prev_shm_circularBuffer()!");
		exit(EXIT_FAILURE);
	}
	
	if(shmctl(id_tmp, IPC_RMID, NULL) == -1)
	{
		perror("shm remove failed!");
		exit(EXIT_FAILURE);
	}

	return 0;
}


int idCB, idMetadata;
char *pSHMCircularBuffer;
char *pSHMMetadata;
char *semW_name;
char *semR_name;
	
int main(int argc, char* argv[])
{
	CircularBufferMetadata buffer;
	CircularBufferMetadata *pBufferMetadata = &buffer;
	char writeData[BUFFER_SIZE];
	
	if(argc < 4)
	{
		printf("error:usage %s <shm key> <writer_sem name> <reader_sem name>\n\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	
	char *shm_key = argv[1];
	semW_name = argv[2];
	semR_name = argv[3];

	//unlinking previously created semaphore
	int r = sem_unlink(semW_name);
	int q = sem_unlink(semR_name);
	if(r==0 && q==0)
	{
		printf("semaphores removed\n");
	}

	//catching ^C signal and changing its deposition to user defined handler
	if(signal(SIGINT, handler) == SIG_ERR)
	{
		perror("signal() failed!");
		exit(EXIT_FAILURE);
	}

	// shared memory to share the circular buffer and metadata

	//changing string to integer
	key_t key = atoi(argv[1]);

	
	//creating shared memory; ensure previous shm is unlinked first
create_shm:
	idCB = shmget(key, SHM_SIZE, SHM_OPEN_FLAGS_MODE);
	if(idCB == -1) 
	{
		perror("shmget() failed!");
		//exit(EXIT_FAILURE);
		if(errno == EEXIST) 
		{	//handle error due to IPC_CREAT | IPC_EXCL flag
			if(unlink_prev_shm_circularBuffer(key, SHM_SIZE) == -1) 
			{
				printf("unlink_prev_shm_circularBuffer() failed!!!\n");
				exit(EXIT_FAILURE);
			} else 
			{
				goto create_shm;
			}
		} 
		else 
		{
			exit(EXIT_FAILURE);
		}
 	} 

 	// shared memory to share the metadata 
 	key_t key_metadata = key + 10;

 	// creating shared memory
 	create_shm2:
		idMetadata = shmget(key_metadata, sizeof(CircularBufferMetadata), SHM_OPEN_FLAGS_MODE);
		if(idMetadata == -1) 
		{
			perror("shmget() failed!");
			//exit(EXIT_FAILURE);
			if(errno == EEXIST) 
			{	//handle error due to IPC_CREAT | IPC_EXCL flag
				if(unlink_prev_shm_circularBuffer(key_metadata, sizeof(CircularBufferMetadata)) == -1) 
				{
					printf("unlink_prev_shm_circularBuffer() failed!!!\n");
					exit(EXIT_FAILURE);
				} 
				else 
				{
					goto create_shm2;
				}
			} 
			else 
			{
				exit(EXIT_FAILURE);
			}

		}

 	//attaching circular buffer shared memory to writer's address space
	pSHMCircularBuffer = (char*)shmat(idCB, NULL, 0);
	if(pSHMCircularBuffer == (char *)-1) 
	{
		perror("shmat() failed!");
		exit(EXIT_FAILURE);
 	} 
	printf("SHM Circular Buffer attached\n");


	// attaching metadata shared memory to writer's address space
	pSHMMetadata = (char*)shmat(idMetadata, NULL, 0);
	if(pSHMMetadata == (char *)-1) 
	{
		perror("shmat() failed!");
		exit(EXIT_FAILURE);
 	} 
	printf("SHM Metadata attached\n");


	//creating and opening semaphore on which writer waits
	sem_t *writer_sem = sem_open(semW_name, SEM_OPEN_FLAGS, SEM_OPEN_MODE, SEM_INIT_VAL_WR_SEM);
	if(writer_sem == SEM_FAILED)
	{
		perror("sem_open for writer_sem");
		exit(EXIT_FAILURE);
	}
	printf("writer_sem created\n");

	
	//creating and opening semaphore on which reader waits
	sem_t *reader_sem = sem_open(semR_name, SEM_OPEN_FLAGS, SEM_OPEN_MODE, SEM_INIT_VAL_RD_SEM);
	if(reader_sem == SEM_FAILED)
	{
		perror("sem_open for reader_sem");
		exit(EXIT_FAILURE);
	}
	printf("reader_sem created\n");	

	// typecasting shared memory to type CircularBufferMetadata to hold the structure pointed by pBufferMetadata
	pBufferMetadata = (CircularBufferMetadata *)pSHMMetadata;

	SHM_Buffer_Initialize(pBufferMetadata, pSHMCircularBuffer);
	printf("initialized\n");
	
	
	while(1)
	{	
		if(sem_wait(writer_sem) == -1)
		{
			perror("sem_wait() failed!");
			exit(EXIT_FAILURE);
		}

		// remapping metadata to process's virtual address
		SHM_Metadata_Buffer_Remap(pBufferMetadata, pSHMCircularBuffer);

		// user inputted data
		printf("enter the data:\n");
		gets(writeData);
		int inputDataSize = (strlen(writeData) + 1);	//plus 1 for including '\0' introduced by gets but ignored by strlen 
		// printf("%d\n", inputDataSize);
		
		if(Free_Space_Present(pBufferMetadata) >= inputDataSize)
		{
			//writing begins 
			Write_Into_SHM_Buffer(pBufferMetadata, writeData, inputDataSize);
		}


		//reader_sem is released
		if(sem_post(reader_sem) == -1)
		{
			perror("sem_post() failed!");
			exit(EXIT_FAILURE);
		}
	}	
	
	//detaching shm_ptr from writer's address space
	if(shmdt(pSHMCircularBuffer) == -1)
	{
		perror("shmdt() failed!");
		exit(EXIT_FAILURE);		
	}

	//detaching shm_metadata from writer's address space
	if(shmdt(pSHMMetadata) == -1)
	{
		perror("shmdt() failed!");
		exit(EXIT_FAILURE);		
	}
 
	return EXIT_SUCCESS;
}

//SIGINT handler definition
void handler(int s)
{
	//detaching shm_ptr from writer's address space
	if(shmdt(pSHMCircularBuffer) == -1)
	{
		perror("shmdt() failed!");
		exit(EXIT_FAILURE);		
	}

	//detaching shm_metadata from writer's address space
	if(shmdt(pSHMMetadata) == -1)
	{
		perror("shmdt() failed!");
		exit(EXIT_FAILURE);		
	}

	//removing shm_ptr
	if(shmctl(idCB, IPC_RMID, NULL) == -1)
	{
		perror("shm remove failed!");
		exit(EXIT_FAILURE);
	}

	//removing shm_metadata
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

	//terminate the program
	exit(EXIT_SUCCESS);
}


