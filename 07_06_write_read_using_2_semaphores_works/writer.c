/** Writer is taking input from user and writing to shared memory
 */

#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>

//handler declaration
void handler(int);

//variable used to open shared memory and remove shared memory
int id;

int main(int argc , char* argv[])
{
	if(argc<2)
	{
		printf("enter key\n");
		exit(1);
	}

	//unlinking semaphore
	int r = sem_unlink(argv[2]);
	int q = sem_unlink(argv[3]);
	if(r==0 && q==0)
	{
		printf("semaphore removed\n");
	}

	//catching ^C signal and changing its deposition to user defined handler
	if(signal(SIGINT,handler) == SIG_ERR)
	{
		perror("signal");
		exit(1);
	}

	//changing string to integer
	key_t key = atoi(argv[1]);

	//creating shared memory
	id = shmget(key, 100, IPC_CREAT|0660);
	printf("id:%d\n",id);
 	
 	//attaching shm to writer's address space
	char *str = (char*) shmat(id, NULL, 0);
	
	//creating and opening first semaphore
	sem_t *write = sem_open(argv[2], O_CREAT, 0660, 1);
	if(write == NULL)
	{
		perror("open");
		exit(1);
	}
	printf("semaphore1 created\n");

	//creating and opening second semaphore
	sem_t *read = sem_open(argv[3], O_CREAT, 0660, 1);
	if(read == NULL)
	{
		perror("open");
		exit(1);
	}
	printf("semaphore2 created\n");	

	//writing begins 
	while(1)
	{
		//semaphore1 is waiting
		if(sem_wait(write) == -1)
		{
			perror("wait");
			exit(1);
		} 

		printf("Write Data : \n");
		gets(str); 
		printf("Data written in memory: %s\n",str);

		//semaphore2 is released
		if(sem_post(read) == -1)
		{
			perror("post");
			exit(1);
		} 
	}	
	
	//detaching shm from writer's address space
	shmdt(str);
 
	return 0;
}

//handler definition
void handler(int s)
{
	//removing shared memory
	if(shmctl(id, IPC_RMID, NULL)==-1)
	{
		perror("remove");
		exit(1);
	}

	//terminate the program
	exit(1);
}