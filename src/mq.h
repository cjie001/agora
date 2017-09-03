/*******************************************
   Author        : Jun Zhang
   Email         : ewalker.zj@gmail.com
   Last Modified : 2017-06-20 19:14
   Filename      : mq.h
   Description   : 
*******************************************/
#ifndef _MQ_H
#define _MQ_H

//#include "common.h"
//#include "protocol.h"
#include "memq.h"
#include "tcp_server.h"

typedef struct MQ Memcache;

struct {
	char queue_info[256];
	MEMQ* memq;
} Memcache;


//取得mcq
//static Memcache* get_mcq(WORKER* worker, LogInfo* info);

//д�뵽mcq
//int send_queue_log(WORKER *worker, LogHead *head, LogInfo *info);

#endif  // end _MQ_H
