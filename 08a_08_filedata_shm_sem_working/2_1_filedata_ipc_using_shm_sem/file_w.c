/** 
	Writer is reading from the file line by line and writing to shared memory
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

#define SHM_SIZE 1024
#define SHM_OPEN_FLAGS_MODE	IPC_CREAT|IPC_EXCL|S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP
#define SHM_OPEN_FLAGS_MODE_FOR_RM	S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP

#define SEM_OPEN_FLAGS	O_CREAT|O_EXCL 
#define SEM_OPEN_MODE	S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP	
#define SEM_INIT_VAL_WR_SEM			1
#define SEM_INIT_VAL_RD_SEM			0

//handler declaration
void handler(int);

//variable used to open & remove shared memory and semaphores
int id;
char *shm_ptr;
char *semW_name;
char *semR_name;

int unlink_prev_shm(key_t key) {

	int id_tmp = shmget(key, SHM_SIZE, SHM_OPEN_FLAGS_MODE_FOR_RM);
	if(id_tmp == -1) 
	{
		perror("shmget() failed inside unlink_prev_shm()!");
		exit(EXIT_FAILURE);
	}

	//removing shared memory
	if(shmctl(id_tmp, IPC_RMID, NULL) == -1)
	{
		perror("shm remove failed!");
		exit(EXIT_FAILURE);
	}

	return 0;
}

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

	//changing string to integer
	key_t key = atoi(argv[1]);

	//creating shared memory; ensure previous shm is unlinked first
create_shm:
	id = shmget(key, SHM_SIZE, SHM_OPEN_FLAGS_MODE);
	if(id == -1) 
	{
		perror("shmget() failed!");
		//exit(EXIT_FAILURE);
		if(errno == EEXIST) 
		{	//handle error due to IPC_CREAT | IPC_EXCL flag
			if(unlink_prev_shm(key) == -1) 
			{
				printf("unlink_prev_shm() failed!!!\n");
				exit(EXIT_FAILURE);
			} else 
			{
				goto create_shm;
			}
		} else 
		{
			exit(EXIT_FAILURE);
		}
 	} else 
 	{
 		// printf("shmget() id:%d\n",id);
 	}

 	//attaching shm to writer's address space
	shm_ptr = (char*)shmat(id, NULL, 0);
	if(shm_ptr == (char *)-1) 
	{
		perror("shmat() failed!");
		exit(EXIT_FAILURE);
 	} else 
 	{
 		// printf("shmget() id:%d\n",id);
 	}	
	printf("shm attached\n");

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

	//opening file
	FILE * fp = fopen("svn_git_cmds.txt", "r");

	//clear SHM before transfer begins; NOT NEEDED since shmget() initializes to zero
	// memset(shm_ptr, 0, SHM_SIZE);

	//writing begins 
	while(!feof(fp))
	{
		//waiting on writer_sem 
		if(sem_wait(writer_sem)==-1)
		{
			perror("sem_wait() failed!");
			exit(EXIT_FAILURE);
		} 

		// if(shm_ptr[0] == '\0') 	//condition not needed in 2 semaphore problem
		{
			if(fgets(shm_ptr, SHM_SIZE, fp) == NULL)
			{ 
				// fprintf(stdout,"%s",shm_ptr);
				perror("fgets() failed!");			
			} 
		}

		// printf("press any key to write next line ...\n");
		// getchar();

		//reader_sem is released
		if(sem_post(reader_sem) == -1)
		{
			perror("sem_post() failed!");
			exit(EXIT_FAILURE);
		}
	}	
	
	//detaching shm from writer's address space
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
		perror("shmdt() failed!");
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