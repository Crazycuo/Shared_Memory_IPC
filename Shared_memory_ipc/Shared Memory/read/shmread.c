/*************************************************************************
	> File Name: shmwrite.c
	> Author: Crazycuo
	> Mail: 740094202@qq.com 
	> Created Time: 2016年03月02日 星期三 19时57分12秒
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include "../shm_buffer.h"

struct timeval starttime;
int flag=0;
char* shmaddr;
shm_buffer_head *head;

long count=0;
double sum=0;
double max=0;
double min=100000;
double aver=0;
double passedtime=0;
long recount=0;
pthread_mutex_t mutex;

void * timer(void *pdata)
{
	while(1)
	{
		sleep(10);
		printf("count:%ld recount:%ld\n",count,recount);
		printf("\n");
	}
}

void* handle(void *pdata)
{
	pthread_detach(pthread_self());
	double cost;
	char sec[11];
	char usec[7];
	struct timeval time;
	shm_buffer *temp=(shm_buffer *)pdata;
	int p=(char*)temp-shmaddr;

	memcpy(sec,shmaddr+temp->addr,10);
	memcpy(usec,shmaddr+temp->addr+11,6);
	sec[10]='\0';
	usec[10]='\0';

	gettimeofday(&time,NULL);
	if(flag==0){
		starttime.tv_sec=time.tv_sec;
		starttime.tv_usec=time.tv_usec;
		flag++;
	}
	
	cost=(time.tv_usec-atol(usec))*1.0/1000+1000*(time.tv_sec-atol(sec));
	if(cost>1000) {
		printf("context:%s %s\n",sec,usec);
		printf("now time:%ld %ld\n",time.tv_sec,time.tv_usec);
	}
	//printf("per cost: %fms\n\n",cost);

	if(cost>max) max=cost;
	if(cost<min) min=cost;

	int icount;
	pthread_mutex_lock(&mutex);
		sum+=cost;
		icount=++count;
		aver=sum/icount;
	pthread_mutex_unlock(&mutex);
	//printf("max: %f\t min: %f\taver:%fms\n",max,min,aver);
	passedtime=(time.tv_usec-starttime.tv_usec)*1.0/1000+1000*(time.tv_sec-starttime.tv_sec);
	printf("total time:%fms count:%d\n",passedtime,icount);	

	int res=__sync_add_and_fetch(&(temp->num_of_finished_readers),1);	

	if(res == temp->num_of_readers) {
		//memset(shmaddr+temp->addr,0,PER_BUFFER_SIZE);
		temp->size = 0;
		InQueue(&head->writequeue,p,0);
		__sync_add_and_fetch(&recount,1);
	}
}
int main(int argc, char **argv)
{
	int shm_id,i;
	key_t key;
	char temp[8];
	char pathname[30];

	//系统建立IPC通讯必须指定一个ID值，通常情况下，该ID值通过ftok函数得到
	strcpy(pathname,"/home/autolab");
	key=ftok(pathname,0x03);
	if(key==-1) {
		perror("ftok error");
		return -1;
	}
	printf("key=%d\n",key);

	shm_id=shmget(key,0,0);
	if(shm_id == -1) {
		perror("shmget error\n");
		return -1;
	}
	printf("shm_id = %d\n",shm_id);

	shmaddr=(char*)shmat(shm_id,NULL,0);
	if((int)shmaddr ==-1) {
		perror("shmat addr error!\n");
		return -1;
	}

	/*
	 * here is the daemon
	 */
	pthread_mutex_init(&mutex,NULL);

	head=(shm_buffer_head*)shmaddr;
	char *buffer = (char*)malloc(PER_BUFFER_SIZE*sizeof(char));
	int fd;

	int shm_fd =atoi(argv[1]);
	i=0;
	struct timeval starttime;
	pthread_t pthread_timer;
	
	//pthread_create(&pthread_timer,NULL,timer,NULL);
	while(1){
		while(IsQueueEmpty(&head->readqueue[shm_fd])) {
			pthread_cond_wait(&head->readqueue[shm_fd].cond,&head->readqueue[shm_fd].mutex);
		}
		int p;
		while(OutQueue(&head->readqueue[shm_fd],&p)==1) {
			shm_buffer *temp;
			temp = (shm_buffer*)(shmaddr+p);

			strncpy(buffer,shmaddr+temp->addr,temp->size);
			pthread_t pthread_handle;
			pthread_create(&pthread_handle,NULL,handle,temp);

		}
	}
	if(shmdt(shmaddr)==-1) {
		perror("detach error!\n");
		return -1;
	}
	shmctl(shm_id,IPC_RMID,NULL);
	return 0;
}

