/** Reader is reading from the shared memory and is synchronized with writer with the help of semaphore 
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

//handler declaration
void handler(int);

int id;
 
int main(int argc,char* argv[])
{
	if(argc<2)
	{
		printf("key & semaphore name is absent\n");
		exit(1);
	}

	//catching ^C signal and changing its deposition to user defined handler
	if(signal(SIGINT,handler) == SIG_ERR)
	{
		perror("signal");
		exit(1);
	}

	//changing string to integer
	key_t key=atoi(argv[1]);

	//get the id of shared memory
	id = shmget(key,0,0);
	printf("id:%d\n",id); 
	
	//attaching shared memory to reader's address space
	char *str = (char*) shmat(id,0,0);
	if(str==NULL)
	{
		perror("shmat");
		exit(1);
	}
	printf("shm attached\n");

	//opening first semaphore
	sem_t *write = sem_open(argv[2],0,0,1);
	if(write == NULL)
	{
		perror("open");
		exit(1);
	}
	printf("semaphore1 opened\n");

	//opening second semaphore
	sem_t *read = sem_open(argv[3],0,0,1);
	if(read==NULL)
	{
		perror("open");
		exit(1);
	}
	printf("semaphore2 opened\n");

	//reading begins
	while(1)
	{	
		//semaphore 2 is waiting
		if(sem_wait(read)==-1)
		{
			perror("wait");
			exit(1);
		}
		
		//reading data from shm and printing to stdout
		printf("%s\n",str);

		//semaphore 1 is released
		if(sem_post(write)==-1)
		{
			perror("post");
			exit(1);
		}
	}

	//detaching shm from reader's address space
	shmdt(str);
	
	return 0;
}

//handler definition
void handler(int s)
{
	//removing shared memory
	if(shmctl(id,IPC_RMID,NULL)==-1)
	{
		perror("remove");
		exit(1);
	}
	
	exit(1);
}