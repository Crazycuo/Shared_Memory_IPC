/*************************************************************************
	> File Name: shmwrite.c
	> Author: Crazycuo
	> Mail: 740094202@qq.com 
	> Created Time: 2016年03月02日 星期三 19时57分12秒
 ************************************************************************/

#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h> //open
#include <errno.h>
#include <time.h>
#include <stdlib.h>

#include "../shm_buffer.h"

long j;
int main(int argc, char **argv)
{
	int shm_id,i;
	key_t key;
	char pathname[30];
	char *shmaddr;
	shm_buffer_head *buffer_head;

	strcpy(pathname,"/home/autolab");
	key=ftok(pathname,0x03);
	if(key==-1) {
		perror("ftok error");
		return -1;
	}
	printf("key=%d\n",key);

	//shm_id=shmget(key,SIZE,IPC_CREAT|IPC_EXCL|0600);
	shm_id=shmget(key,SIZE,IPC_CREAT|0777);
	if(shm_id == -1) {
		perror("shmget error\n");
		return -1;
	}
	printf("shm_id = %d\n",shm_id);

	shmaddr=(char*)shmat(shm_id,NULL,0);
	if((int)shmaddr == -1) {
		perror("shmat addr error!\n");
		return -1;
	}

	/*
	 * initialize the buffer_head;
	 */
	buffer_head=(shm_buffer_head*)malloc(sizeof(shm_buffer_head));
	buffer_head->per_buffer_size = PER_BUFFER_SIZE;
	buffer_head->shmaddr=0;
	buffer_head->allocated_buffer_start_addr = buffer_head->shmaddr +BUFFER_SPACE+ARRAY_SHM_BUFFERS;
	buffer_head->size = SIZE;
	buffer_head->allocated_size = SIZE-BUFFER_SPACE-ARRAY_SHM_BUFFERS;//1024 is left for the shm_buffer[]


	split_shm_to_buffers(buffer_head,PER_BUFFER_SIZE,shmaddr);
	
	shm_buffer_head *head;
	head=(shm_buffer_head*)shmaddr;
	


	int fd;
	int bytes_read,bytes_write;
	char *ptr;

	fd = open("test.txt",O_RDONLY);
	if(fd == -1) {
		fprintf(stderr, "open file error1\n");
		return -1;
	}

	int count = 0;

	sleep(10);
	//char buff[PER_BUFFER_SIZE];
	char *buff=(char *)malloc(sizeof(char)*PER_BUFFER_SIZE);
	printf("begin\n");


	long lostcount=0;
	int index= atoi(argv[1]);

	//while(1)
	int num_of_msg = 100000;
	int j;
	for(j=0; j<num_of_msg; j++)
	{
		shm_buffer *buffer = get_empty_block(head,shmaddr,i,0);
		int i=0;
		if(buffer) {
			struct timeval time;

			memset(buff,'0',PER_BUFFER_SIZE);
			gettimeofday(&time,NULL);
			sprintf(buff,"%ld %ld",time.tv_sec,time.tv_usec);

			memcpy(buffer->addr+shmaddr,buff,PER_BUFFER_SIZE);
			buffer->size = PER_BUFFER_SIZE;
			buffer->num_of_readers = index;
			buffer->num_of_finished_readers=0;
			for(i=0; i<index; i++) {
				InQueue(&head->readqueue[i],(char *)buffer -shmaddr, 1);
			}
		}
		else printf("lostcount:%ld\n",++lostcount); 
		usleep(1*10);
		count++;
	}

	close(fd);
	printf("end\n");

	shmdt(shmaddr);
	shmctl(shm_id,IPC_RMID,NULL);
	return 0;
}

