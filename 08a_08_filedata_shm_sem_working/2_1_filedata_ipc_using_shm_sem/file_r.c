/** 
	Reader is reading from the shared memory and is synchronized with writer with the help of semaphore 
 */

#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <semaphore.h>
#include <unistd.h>
#include <signal.h>

#define SHM_SIZE 0	//since opener of SHM
#define SHM_OPEN_FLAGS_MODE	S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP

#define SEM_OPEN_FLAGS	0 

//handler declaration
void handler(int);

//variable used to open & remove shared memory and semaphores
int id;
char *shm_ptr;
char *semW_name;
char *semR_name;
 
int main(int argc, char* argv[])
{
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

	//get the id of existing shared memory
	id = shmget(key, SHM_SIZE, SHM_OPEN_FLAGS_MODE);
	if(id == -1) 
	{
		perror("shmget() failed!");
		exit(EXIT_FAILURE);
 	} else 
 	{
 		// printf("shmget() id:%d\n",id);
 	}
	
	//attaching shared memory to reader's address space
	shm_ptr = (char*) shmat(id, NULL, 0);
	if(shm_ptr == (char *)-1) 
	{
		perror("shmat() failed!");
		exit(EXIT_FAILURE);
 	} else 
 	{
 		// printf("shmget() id:%d\n",id);
 	}	
	printf("shm attached\n");

	//opening semaphore on which writer waits
	sem_t *writer_sem = sem_open(semW_name, SEM_OPEN_FLAGS);
	if(writer_sem == SEM_FAILED)
	{
		perror("sem_open for writer_sem");
		exit(EXIT_FAILURE);
	}
	printf("writer_sem created\n");

	//opening semaphore on which reader waits
	sem_t *reader_sem = sem_open(semR_name, SEM_OPEN_FLAGS);
	if(reader_sem == SEM_FAILED)
	{
		perror("sem_open for reader_sem");
		exit(EXIT_FAILURE);
	}
	printf("reader_sem created\n");	

	//reading begins
	while(1)
	{	
		//waiting on reader_sem 
		if(sem_wait(reader_sem) == -1)
		{
			perror("sem_wait() failed!");
			exit(EXIT_FAILURE);
		}
		
		//reading data from shm and printing to stdout
		// puts(shm_ptr);	//also puts trailing newline which is already present from fgets() input from writer app
		printf("%s", shm_ptr);
		fflush(stdout);	//needed else final few lines will not output
		shm_ptr[0] = '\0';	//tell writer that reading is done

		//writer_sem is released
		if(sem_post(writer_sem) == -1)
		{
			perror("sem_post() failed!");
			exit(EXIT_FAILURE);
		}
	}

	//detaching shm from reader's address space
	if(shmdt(shm_ptr) == -1)
	{
		perror("shmdt() failed!");
		exit(EXIT_FAILURE);		
	}
 	
	return EXIT_SUCCESS;
}

//SIGINT handler definition
void handler(int s)
{
	//detaching shm from writer's address space
	if(shmdt(shm_ptr) == -1)
	{
		perror("shmdt() failed in SIGINT handler!");
		exit(EXIT_FAILURE);		
	}

	//removing shared memory
	if(shmctl(id, IPC_RMID, NULL) == -1)
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