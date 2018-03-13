/** writing to shm with condition
 */

#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <semaphore.h>
#include <unistd.h>

int main(int argc , char* argv[])
{
	if(argc<2)
	{
		printf("enter key\n");
		exit(1);
	}

	int r = sem_unlink(argv[2]);
	if(r==0)
	{
		printf("semaphore removed\n");
	}

	key_t key=atoi(argv[1]);

	int id=shmget(key,100,IPC_CREAT|0660);
	printf("id:%d\n",id);
 
	char *str = (char*) shmat(id,NULL,0);
	printf("shm attcahed\n");

	sem_t *sptr=sem_open(argv[2],O_CREAT,0660,1);
	if(sptr==NULL)
	{
		perror("open");
		exit(1);
	}
	printf("semaphore opened\n");

	while(1)
	{
		if(sem_wait(sptr)==-1)
		{
			perror("wait");
			exit(1);
		}

		if(*str=='\0')
		{
			printf("enter string\n");
			gets(str);
		}

		if(sem_post(sptr)==-1)
		{
			perror("post");
			exit(1);
		}
		usleep(10);

	}
	shmdt(str);
 
	return 0;
}
