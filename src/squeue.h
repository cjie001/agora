#ifndef _QUEUE_H
#define _QUEUE_H

//队列初始化，size-队列占用的内存大小
void* queue_init(int size);

//释放队列
void queue_release(void *h);

//从队列中取数据,0-成功，-1-没数据，-2-空间不够
int queue_get(void *h, char *data, int *data_len);

//向队列中放数据,0-成功，-1-队列已满
int queue_set(void *h, char *data, int data_len);

//得到队列中的元素个数
int queue_size(void *h);

#endif
