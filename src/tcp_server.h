/*******************************************
   Author        : Jun Zhang
   Email         : ewalker.zj@gmail.com
   Last Modified : 2017-06-20 18:11
   Filename      : tcp_server.h
   Description   : 
*******************************************/
#ifndef _TCP_SERVER_H
#define _TCP_SERVER_H

#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<string.h>
#include<limits.h>
#include<signal.h>

#include<event2/event.h>
#include<event2/bufferevent.h>
#include<event2/util.h>
#include<event2/buffer.h>
#include<stdbool.h>


#include "global.h"
#include "protocol.h"
#include "common.h"
#include "file.h"
#include "memq.h"
//#include "util.h"
#include "squeue.h"
#include "hash.h"
#include "config.h"


typedef struct _SERVER SERVER;
typedef struct _WORKER WORKER;
typedef struct _QUEUE_WORKER QUEUE_WORKER;


typedef struct conn_item CONN;
typedef struct conn_list CONN_LIST;


typedef struct {
	char queue_info[128];
	MEMQ* memq;
} Memcache;


struct conn_item{
	uint32 id;
	int fd;
	char ip[16];
	uint16 port;
	char buf[MAX_BUF_LEN];
	//int start;
	//int end;
	//bool is_empty;
	//int read_len;
	//int write_len;
	
	struct bufferevent *bufev;

	CONN* next;

};


struct conn_list{
	CONN* head;
	CONN* tail;
	CONN list[0];
};


struct _WORKER {
	pthread_t thread_id;
	int id;
	int sfd;
	struct event_base *base;
	struct event *read_event;
	struct event *write_event;
	int notified_rfd;
	int notified_wfd;
	CONN_LIST *conns;
	SERVER *server;
};

struct _WRITE_WORKER { 
	pthread_t thread_id;
	int id;
	struct event_base *base;
	struct event *read_event;
	Memcache mems[MAX_MEMCACHE_NUM];
};

typedef struct _WORKER_ARRAY {
	uint16_t size;
	SERVER *server;
	WORKER array[0];
} WORKER_ARRAY;

struct _SERVER {
	int server_fd;
	int port;
	struct event_base *base;
	struct event *listen_event;
	WORKER_ARRAY *workers;
	WORKER_ARRAY *write_workers;
	int ret;
	uint16 worker_num;
	uint64 conn_count;
	pthread_mutex_t start_lock;
	pthread_cond_t start_cond;
};

struct _QUEUE_WORKER { 
	pthread_t thread_id;
	int id;
	void *queue;
	struct event_base *base;
	struct event *queue_event;
	int notify_wfd;
	int notify_rfd;
	char data[MAX_LOG_LEN + sizeof(LogHead)];
	//char data[65559];
	Memcache mems[MAX_MEMCACHE_NUM];
	int mem_num;
};

#define PUT_CONN_ITEM(conn, item)	\
	item -> next = NULL;	\
	if( NULL == conn->tail )	\
		conn -> head = item;	\
	else	\
		conn -> tail -> next = item;	\
	conn -> tail = item;

#define GET_CONN_ITEM(conn, item)	\
	item = conn -> head;	\
	if(NULL != item){	\
		conn -> head = item -> next;	\
		if( NULL == conn -> head )	\
			conn -> tail = NULL;	\
	}	\

//	if(conn -> head != conn -> tail){	\
//		item = conn -> head;	\
//		conn -> head = conn -> head -> next;	\
//	}	\
//	else	\
//	{	\
//		item = NULL;	\
//	}

//SERVER* init_server(int port, uint16 workernum, uint32 connnum, int read_timeout, int write_timeout);
//WORKER_ARRAY* init_workers(SERVER *server, uint16 workernum, uint32 connnum, int read_timeout, int write_timeout);

CONN_LIST* init_conn_list(uint32 size);

//void start_server(void* args);
//int start_workers(WORKER_ARRAY* workers);



extern int run_daemon();

#endif   // end _TCP_SERVER_H
