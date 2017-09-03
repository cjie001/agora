/*******************************************
   Author        : Jun Zhang
   Email         : ewalker.zj@gmail.com
   Last Modified : 2017-06-20 19:36
   Filename      : mq.c
   Description   : 
*******************************************/
#include "mq.h"

static Memcache* get_mcq(QUEUE_WORKER *thread, LogInfo *info){
	int i;
	Memcache *mem;
	for(i = 0; i < thread->mem_num; i ++){
		mem = thread->mems + i;
		if(strcmp(mem->queue_info, info->queue_info) == 0){
			break;
		}
	}
	if(i == thread->mem_num){
		mem = thread->mems + thread->mem_num ++;
		strcpy(mem->queue_info, info->queue_info);
		mem->memq = NULL;
	}
	if(mem->memq == NULL){
		mem->memq = memq_new(info->queue_info, INT_MAX, 10);
		if(mem->memq == NULL){
			return NULL;
		}
	}
	return mem;
}


