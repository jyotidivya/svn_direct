/** reading from shm with condition
 */

#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <string.h>
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

    key_t key = atoi(argv[1]);

    int id = shmget(key,0,0);
    printf("id:%d\n",id); 
    
    char *str = (char*) shmat(id,0,0);
    if(str==NULL)
    {
        perror("shmat");
        exit(1);
    }
    printf("shm attached\n");

    sem_t *sptr=sem_open(argv[2],0,0,1);
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

    	if(*str!='\0')
    	{
    		puts(str);
    		memset(str,'\0',100);
    	}

    	if(sem_post(sptr)==-1)
    	{
    		perror("post");
    		exit(1);
    	}
    }

    shmdt(str);
    shmctl(id,IPC_RMID,NULL);

    return 0;
}