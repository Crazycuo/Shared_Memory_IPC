/*************************************************************************
	> File Name: shm_buffer.h
	> Author: Crazycuo
	> Mail: 740094202@qq.com 
	> Created Time: 2016年03月03日 星期四 13时10分44秒
 ************************************************************************/
#ifndef _SHM_BUFFER_H
#define _SHM_BUFFER_H

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include "CycleQueue.h"

#define BUFFER_SPACE 1024 //related to the CycleQueue
#define ARRAY_SHM_BUFFERS 1024//related to the BLOCKS

#define PER_BUFFER_SIZE 1*1024*1024 
#define BLOCKS 8 

#define SIZE (((BLOCKS)*(PER_BUFFER_SIZE))+BUFFER_SPACE+ARRAY_SHM_BUFFERS)
#define BLOCKS_PER_CLIENT 4

typedef struct _shm_buffer shm_buffer;
struct _shm_buffer{
	long size;
	int  addr;
	int offset;//offset to the baseaddr
	int num_of_readers;
	int num_of_finished_readers;
};

/*
 * the _shm_buffer_head should be on the top of a shm;
 */
typedef struct _shm_buffer_head shm_buffer_head;
struct _shm_buffer_head{
	pthread_mutex_t shm_buffer_head_mutex;
	int per_buffer_size;
	int num_of_buffers;
	
	int shmaddr;
	int allocated_buffer_start_addr;//the shm_buffer_head takes some place

	long size;//the size of the shm
	int allocated_size;//the size of shm - the size of the shm_buffer_head

	CycleQueue writequeue;
	CycleQueue readqueue[4];
};

//return the addr which the shm_buffer refer to
shm_buffer* get_empty_block(shm_buffer_head *head, char *baseaddr, int i, int rw)
{
	int p;

	if(rw == 0) {//写申请: 获取空闲的项给发送方
		if(OutQueue(&head->writequeue,&p) == -1)
			return NULL;
	}else{//读申请： 每个接收方从自己的读循环队列中获取发送方发送的数据的索引
		if(OutQueue(&head->readqueue[i],&p) == -1) 
			return NULL;
	}
	return (shm_buffer*)(p+baseaddr);
}

int split_shm_to_buffers(struct _shm_buffer_head *buffer_head, int per_buffer_size, char * baseaddr)
{
	int i;
	int buffer_nums = buffer_head->allocated_size/per_buffer_size;

	printf("the num of the buffers is %d\n",buffer_nums);
	printf("the beginning of the shared Memory is %d\n\n",buffer_head->shmaddr);
	
	printf("/********************************\n");
	InitQueue(&((shm_buffer_head*)baseaddr)->writequeue);

	for(i = 0; i < buffer_nums; i++) {
		shm_buffer *buffer=(shm_buffer*)malloc(sizeof(shm_buffer));

		if(buffer==NULL)
		{
			printf("malloc error！");
			return -1;
		}
	
		buffer->addr = buffer_head->allocated_buffer_start_addr + i*per_buffer_size;		
		buffer->size = per_buffer_size;
		buffer->offset = BUFFER_SPACE+i*sizeof(shm_buffer);

		printf("mapping the buffer %d to the shm\n",i);
		//shm_buffer_head *head = (shm_buffer_head *)baseaddr;
		InQueue(&buffer_head->writequeue,BUFFER_SPACE+i*sizeof(shm_buffer), 0);
		memcpy(baseaddr+BUFFER_SPACE+i*sizeof(shm_buffer),buffer,sizeof(shm_buffer));
		free(buffer);
	}
	printf("mapping the shm_buffer_head to the shm\n");
	memcpy(baseaddr, (char*)buffer_head, sizeof(shm_buffer_head));

	for(i=0;i<4;i++)
	{
		//InitQueue(&buffer_head->readqueue[i]);
		InitQueue(&((shm_buffer_head*)baseaddr)->readqueue[i]);
	}

	printf("********************************/\n\n");
	printf("the size of shm_buffer_head is %lu\n",sizeof(shm_buffer_head));
	printf("the size of shm_buffer %lu\n",sizeof(shm_buffer));
	return 0;
}

void do_something(){

}
#endif
