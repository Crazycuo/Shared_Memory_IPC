//循环队列，以数组作为存储结构，C语言实现
#include <stdlib.h>
#include <stdio.h>
#include "shm_buffer.h"
#include <pthread.h>

//定义队列数组大小
#define QUEUESIZE 16

//定义循环队列结构体
typedef struct _CycleQueue
{
	pthread_cond_t cond;
	pthread_condattr_t condattr;

	pthread_mutex_t mutex;
	pthread_mutexattr_t mattr;
	int front;//队首
	int rear;//队尾
	long count;//队列中元素个数
	int data[QUEUESIZE];//存储队列元素
} CycleQueue;

void InitQueue(CycleQueue *queue);//初始化队列
int InQueue(CycleQueue *queue,int data, int flag);//元素data入队
int OutQueue(CycleQueue *queue,int *pdata);//元素出队，并存放在pdata指向区域
int IsQueueEmpty(CycleQueue *queue);//判断队列是否为空
int IsQueueFull(CycleQueue *queue);//判断队列是否已满
void TraverseQueue(CycleQueue *queue);

void InitQueue(CycleQueue *queue)
{
	queue->front=0;
	queue->rear=0;
	queue->count=0;
	
	pthread_condattr_init(&queue->condattr);
	pthread_condattr_setpshared(&queue->condattr, PTHREAD_PROCESS_SHARED);
	pthread_cond_init(&queue->cond,&queue->condattr);

	pthread_mutexattr_init(&queue->mattr);
	pthread_mutexattr_setpshared(&queue->mattr,PTHREAD_PROCESS_SHARED);
	pthread_mutex_init(&queue->mutex,&queue->mattr);
}

int InQueue(CycleQueue *queue,int data, int flag)
{
	if (IsQueueFull(queue))
	{
		return 0;
	}
	else
	{
		queue->data[queue->rear]=data;
		__sync_add_and_fetch(&queue->count,1);
		queue->rear=(queue->rear+1)%QUEUESIZE;

		//pthread_mutex_lock(&queue->mutex);
		if(flag == 1) {
			pthread_cond_signal(&queue->cond);
			//printf("send signal\n");
		}
		//pthread_mutex_unlock(&queue->mutex);
		return 1;
	}
	
}

int OutQueue(CycleQueue *queue,int *pdata)
{
	if (IsQueueEmpty(queue))
	{
		return -1;
	}
	else
	{
		*pdata=queue->data[queue->front];
		__sync_sub_and_fetch(&queue->count,1);
		queue->front=(queue->front+1)%QUEUESIZE;
		return 1;
	}
}

int IsQueueEmpty(CycleQueue *queue)
{
	return
		__sync_sub_and_fetch(&queue->count,0)==0;
}
int IsQueueFull(CycleQueue *queue)
{
		return __sync_sub_and_fetch(&queue->count,0)==QUEUESIZE;
}

