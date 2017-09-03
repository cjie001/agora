#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>
#include <pthread.h>
#include <assert.h>
#include "string.h"

#define MAGIC_NUM	0xf13fae0c

#define offset(a,b) (int)(((a*)0)->b)

//���н��
typedef struct{
	int len;	//���ݳ���
	char data[0];	//����
}QueueNode;

//����
typedef struct{
	int	size;	//�������ܴ�С
	int head;	//����ͷ
	int tail;	//����β
	int num;	//�����еĽ�����
	pthread_mutex_t lock;
	char buffer[0];	//������
}Queue;

//��ʼ��һ������
Queue* queue_init( int size){
	Queue *p;
	p = (Queue*)malloc(size);
	memset(p, 0, sizeof(Queue));
	size -= offset(Queue, buffer);
	p->size = size;
	p->num = p->head = p->tail = 0;
	pthread_mutex_init(&p->lock, NULL);
	return p;
}

//�ͷŶ���
void queue_release(void *h){
	Queue *p = (Queue*)h;
	pthread_mutex_destroy(&p->lock);
}

//�Ӷ�����ȡ����,0-�ɹ���-1-û���ݣ�-2-�ռ䲻��
int queue_get(void *h, char *data, int *data_len){
	Queue *queue = (Queue*)h;
	int ret = 0;
	assert(queue && queue->size);
	pthread_mutex_lock(&queue->lock);
	if(queue->num > 0){	//���зǿ�
		int head = queue->head;
		int maxlen = *data_len;
		QueueNode *node;
		if(queue->size - head < 4 || *(int*)(queue->buffer + head) == 0){
			head = 0;
		}
		node = (QueueNode*)(queue->buffer + head);
		*data_len = node->len;
		if(maxlen < node->len || data == NULL){
			ret = -2;
		}else{
			memmove(data, node->data, node->len);
			head += offset(QueueNode, data) + node->len;
			if(head == queue->size){
				head = 0;
			}
			queue->head = head;
			queue->num --;
			if(queue->num == 0){
				queue->head = queue->tail = 0;
			}
		}
	}else{
		ret = -1;
	}
	pthread_mutex_unlock(&queue->lock);
	return ret;
}

int queue_fetch(void* h, char* data, int* data_len){
	Queue* queue = (Queue*)h;
	int ret = 0;
	assert(queue && queue->size);
	if (queue->num > 0){
		int head = queue->head;
		int maxlen = *data_len;
		QueueNode* node;
		if(queue->size - head < 4 || *(int*)(queue->buffer + head) == 0){
			head = 0;
		}
		node = (QueueNode*)(queue->buffer + head);
		*data_len = node->len;
		if(maxlen < node->len || data == NULL){
			ret = -2;
		}else{
			memmove(data, node->data, node->len);
		}	
	}else{
		return -1;
	}
	return ret;
}

//������з�����,0-�ɹ���-1-��������
int queue_set(void *h, char *data, int data_len){
	Queue *queue = (Queue*)h;
	int ret = 0;
	assert(queue && queue->size);
	pthread_mutex_lock(&queue->lock);
	if(data_len > 0){
		int size = offset(QueueNode, data) + data_len;
		int tail = queue->tail;
		if(tail >= queue->head){
			if(queue->size - tail < size){
				tail = 0;
				if(queue->head - tail < size){
					ret = -1;	//����������
				}else if(queue->size - queue->tail >= 4){
					*(int*)(queue->buffer + queue->tail) = 0;	//��־Ϊ������
				}
			}
		}else{
			if(queue->head - tail < size){
				ret = -1;
			}
		}
		if(ret == 0){
			QueueNode *node = (QueueNode*)(queue->buffer + tail);
			node->len = data_len;
			memmove(node->data, data, data_len);
			tail += size;
			queue->tail = tail;
			queue->num ++;
		}
	}
	pthread_mutex_unlock(&queue->lock);
	return ret;
}

//�õ������е�Ԫ�ظ���
int queue_size(void *h){
	Queue *queue = (Queue*)h;
	int size;
	pthread_mutex_lock(&queue->lock);
	size = queue->num;
	pthread_mutex_unlock(&queue->lock);
	return size;
}
