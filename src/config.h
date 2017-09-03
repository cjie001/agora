/*******************************************
   Author        : Jun Zhang
   Email         : ewalker.zj@gmail.com
   Last Modified : 2017-07-10 16:14
   Filename      : config.h
   Description   : 
*******************************************/
#ifndef _CONFIG_H
#define _CONFIG_H

#include "common.h"

#pragma pack(1)
typedef struct _CONF {
	int ip;
	int port;
	int backdog;
	uint32 worker_num;
	uint32 conn_num;
	uint32 queue_worker_num;
	char conf_name[MAX_PATH_LEN];
	char logpath[MAX_PATH_LEN];
	int file_size;
	time_t last_time;
	int reload_time;
	char program_name[50];
	int argc;
	char **argv;
} CONF;
#pragma pack()


int load_conf(CONF *conf, const char *conf_file, LOGTABLE *table);
int dump_conf(CONF *conf, char *data);


#endif // end _CONFIG_H
