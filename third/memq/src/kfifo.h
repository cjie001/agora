#pragma once
#include <stdint.h>

//循环队列

//内存分配函数
typedef void* (*malloc_func_t)(size_t);

//变长队列的创建，适用于数据、日志
void* dfifo_alloc(int size, malloc_func_t mfunc);

//向队列中插入一条数据
//timeout 0-非阻塞，-1 阻塞，其他值为超时时间
int dfifo_put(void *h, uint32_t cmd, uint32_t len, void *data, double timeout);

//从队列中读取一条数据，返回值-2代表内存不够
int dfifo_get(void *h, uint32_t *cmd, uint32_t *len, void *data, double timeout);

//重新初始化队列
void dfifo_reset(void *h);

//相队列写入结束标志
int dfifo_put_end(void *h);
