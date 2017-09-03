#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>
#include "kfifo.h"

#define MAX_THREAD_NUM 100

#define M_LOCK(l) pthread_mutex_lock(&l)
#define M_UNLOCK(l) pthread_mutex_unlock(&l)

#define RD_LOCK(l, p) while(sem_timedwait(&l, p) < 0);
#define R_LOCK(l) while(sem_wait(&l) < 0);
#define R_UNLOCK(l) {int v;sem_getvalue(&l,&v);if(v<MAX_THREAD_NUM)sem_post(&l);}

//循环队列结构体
typedef struct {
	uint32_t size;		//data的大小
	uint32_t in;		//写入的总长度
	uint32_t out;		//读取的总长度
	uint32_t esize;		//一个元素的大小
	uint32_t is_end;	//是否结束了
	uint32_t reserved;
	pthread_mutex_t put_lock;	//put操作的互斥锁
	pthread_mutex_t get_lock;	//get操作的互斥锁
	sem_t get_sem;	//get信号量
	sem_t put_sem;	//put信号量
	char data[0];		//数据缓冲区
} kfifo_t;

//变长队列的头部
typedef struct{
	uint32_t cmd;
	uint32_t len;
}Head;

//初始化进程锁
static void init_lock(pthread_mutex_t *lock){
	pthread_mutexattr_t mattr;
	pthread_mutexattr_init(&mattr);
	pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
	pthread_mutex_init(lock, &mattr);
	pthread_mutexattr_destroy(&mattr);
}

//初始化变长循环队列
void* dfifo_alloc(int size, malloc_func_t mfunc){
	if(size <= 0){
		return NULL;
	}
	if(mfunc == NULL){
		mfunc = malloc;
	}
	kfifo_t *fifo = mfunc(size + sizeof(kfifo_t));
	if(fifo == NULL)
		return NULL;
	if(mfunc == malloc)	//清空内存
		memset(fifo, 0, sizeof(*fifo));
	if(fifo->size == 0){
		fifo->size = size;
		init_lock(&fifo->get_lock);
		init_lock(&fifo->put_lock);
		sem_init(&fifo->get_sem, 1, 0);
		sem_init(&fifo->put_sem, 1, 0);
	}else{
		int get_retry = 100;
		while(get_retry-- && pthread_mutex_trylock(&fifo->get_lock))usleep(100);
		pthread_mutex_unlock(&fifo->get_lock);

		int put_retry = 100;
		while(put_retry-- && pthread_mutex_trylock(&fifo->put_lock))usleep(100);
		pthread_mutex_unlock(&fifo->put_lock);
		if(get_retry <= 0 || put_retry <= 0){
			fifo->in = fifo->out = 0;
			init_lock(&fifo->get_lock);
			init_lock(&fifo->put_lock);
			sem_init(&fifo->get_sem, 1, 0);
			sem_init(&fifo->put_sem, 1, 0);
		}
	}
	
	return fifo;
}

//写操作结束了
int dfifo_put_end(void *h){
	if(h == NULL)
		return -1;
	kfifo_t *fifo = (kfifo_t*)h;
	fifo->is_end = 1;
	int v;
	sem_getvalue(&fifo->get_sem, &v);
	if(!v){
		int i;
		for(i = 0; i < MAX_THREAD_NUM; i ++){
			sem_post(&fifo->get_sem);
		}
	}
	return 0;
}

//写队列
int dfifo_put(void *h, uint32_t cmd, uint32_t len, void *data, double timeout){
	if(h == NULL){
		return -1;
	}
	kfifo_t *fifo = (kfifo_t*)h;
	uint32_t total_len = sizeof(Head) + len;
	uint32_t space = fifo->size - total_len;
	if(fifo->in - fifo->out > space && !timeout){ //队列已满
		return -1;
	}
	M_LOCK(fifo->put_lock);
	if(fifo->in - fifo->out > space && !timeout){
		M_UNLOCK(fifo->put_lock);
		return -1;
	}
	struct timespec spec;
	gettimeofday((void*)&spec, NULL);
	if(timeout > 0){
		int sec = (int)timeout;
		int usec = (timeout - sec) * 1000000;
		spec.tv_sec += sec;
		spec.tv_nsec += usec;
	}
	spec.tv_nsec *= 1000;
	
	while(fifo->in - fifo->out > space){
		M_UNLOCK(fifo->put_lock);
		if(timeout == -1){
			R_LOCK(fifo->put_sem);
		}else{
			RD_LOCK(fifo->put_sem, &spec);
		}
		M_LOCK(fifo->put_lock);
	}
	Head head = {cmd, len};
	uint32_t cur = fifo->in % fifo->size;
	uint32_t left = fifo->size - cur;

	if(left >= total_len){
		memcpy(fifo->data + cur, &head, sizeof(Head));
		cur += sizeof(Head);
		memcpy(fifo->data + cur, data, len);
	}else if(left >= sizeof(Head)){
		memcpy(fifo->data + cur, &head, sizeof(Head));
		cur += sizeof(Head);
		left -= sizeof(Head);
		memcpy(fifo->data + cur, data, left);
		memcpy(fifo->data, data + left, len - left);
	}else{
		memcpy(fifo->data + cur, &head, left);
		memcpy(fifo->data, (void*)&head + left, sizeof(Head) - left);
		memcpy(fifo->data + sizeof(Head) - left, data, len);
	}

	fifo->in += total_len;
	M_UNLOCK(fifo->put_lock);
	R_UNLOCK(fifo->get_sem);
	return 0;
}

//读队列
int dfifo_get(void *h, uint32_t *cmd, uint32_t *len, void *data, double timeout){
	if(h == NULL){
		return -1;
	}
	uint32_t max_len = *len;
	*len = 0;
	kfifo_t *fifo = (kfifo_t*)h;
	if(fifo->in == fifo->out && (!timeout || fifo->is_end))
		return -1;
	M_LOCK(fifo->get_lock);
	if(fifo->in == fifo->out && (!timeout || fifo->is_end)){
		M_UNLOCK(fifo->get_lock);
		return -1;
	}
	struct timespec spec;
	gettimeofday((void*)&spec, NULL);
	if(timeout > 0){
		int sec = (int)timeout;
		int usec = (timeout - sec) * 1000000;
		spec.tv_sec += sec;
		spec.tv_nsec += usec;
	}
	spec.tv_nsec *= 1000;
	while(fifo->in == fifo->out){
		M_UNLOCK(fifo->get_lock);
		if(timeout == -1){
			R_LOCK(fifo->get_sem);
		}else{
			RD_LOCK(fifo->get_sem, &spec);
		}
		if(fifo->is_end)
			return -1;
		M_LOCK(fifo->get_lock);
	}
	uint32_t cur = fifo->out % fifo->size;
	uint32_t left = fifo->size - cur;
	Head head;
	if(left >= sizeof(Head)){
		memmove(&head, fifo->data + cur, sizeof(Head));
		cur += sizeof(Head);
		left -= sizeof(Head);
	}else{
		memmove(&head, fifo->data + cur, left);
		memmove((void*)&head + left, fifo->data, sizeof(Head) - left);
		cur = sizeof(Head) - left;
		left = fifo->size - cur;
	}
	*cmd = head.cmd;
	*len = head.len;
	if(head.len > max_len){ //内存不够
		M_UNLOCK(fifo->get_lock);
		return -2;
	}
	uint32_t total_len = sizeof(Head) + *len;
	if(left >= *len){
		memmove(data, fifo->data + cur, *len);
	}else{
		memmove(data, fifo->data + cur, left);
		memmove(data + left, fifo->data, *len - left);
	}
	fifo->out += total_len;
	M_UNLOCK(fifo->get_lock);
	R_UNLOCK(fifo->put_sem);
	return 0;
}

//重置dfifo
void dfifo_reset(void *h){
	kfifo_t *fifo = (kfifo_t*)h;
	M_LOCK(fifo->get_lock);
	M_LOCK(fifo->put_lock);
	fifo->in = fifo->out = 0;
	sem_init(&fifo->get_sem, 1, 0);
	sem_init(&fifo->put_sem, 1, 0);
	M_UNLOCK(fifo->put_lock);
	M_UNLOCK(fifo->get_lock);
}
