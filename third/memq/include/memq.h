#pragma once

#include <stdint.h>
#include <libmemcached/memcached.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MEMQ MEMQ;

#define MEMQ_OPTION_DNS_TIME	0x1000	//用于设置dns缓存时间
#define MAX_QUEUE_ITEM_SIZE	(5 * 1024 * 1024)	//单个队列数据的最大大小

MEMQ * memq_new(const char *qstr, int retry, double timeout);

//free a MEMQ
void memq_free(MEMQ *p);

//get a doc, when finished free(*valp)
//when valp has value return value is length
//otherwise return value is memcached_return_t
int memq_get(MEMQ *p, char **valp);

//set a doc, key is needed to select a server
int memq_set(MEMQ *p, char *val, int size);

int memq_set_key(MEMQ *p, char *val, int size, char *key);

void memq_set_key64(MEMQ *p, uint64_t key);

//设置数据过期时间
void memq_set_exptime(MEMQ *p, int exptime);

//delete all queue data
int memq_del(MEMQ *q);

//得到错误描述
char* memq_error(MEMQ *q, memcached_return_t rc);

int memq_set_option(MEMQ *p, int flag, uint64_t val);

//启动读队列线程
void* memq_run(char *qstr, int thread_num, int queue_size_mb);

//停止服务
void memq_stop(void *h);

//取一条数据
//timeout 0-非阻塞 -1-阻塞 其他为超时时间
int memq_get_request(void *h, char *str, int max_len, double timeout);

//取一条数据，需要手动调用free释放内存
//timeout 超时时间 0-非阻塞，-1-阻塞，>0为超时时间
//memq_request(h) = memq_request2(h, -1)
void* memq_request2(void *h, double timeout);
void* memq_request(void *h);

#ifdef __cplusplus
}
#endif
