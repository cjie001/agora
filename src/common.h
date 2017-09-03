/*******************************************
   Author        : Jun Zhang
   Email         : ewalker.zj@gmail.com
   Last Modified : 2017-06-20 19:40
   Filename      : common.h
   Description   : 
*******************************************/

#ifndef _COMMON_H
#define _COMMON_H

#include<stdio.h>
#include<sys/time.h>
#include<pthread.h>
#include<fcntl.h>
#include<sys/prctl.h>
#include<errno.h>
#include<sys/stat.h>
#include<unistd.h>

#include<event2/event.h>


#include "global.h"
#include "hash.h"
#include "types.h"
#include "slog.h"
#include "profile.h"

//哈希节点
HASH_DEF_BEGIN(LogInfo)
	char type[MAX_TYPE_LEN];	//类型名字
	char filename[MAX_PATH_LEN];	//文件名
	FILE *fp;	//文件句柄
	struct tm tm_open;	//打开文件的时间
	char queue_info[256];	//queue
HASH_DEF_END

typedef struct Log Log;

typedef HASHS_DEF(LogInfo, MAX_TYPE_NUM) LOGTABLE;

////工作线程
//typedef struct{
//	int	id;	//线程号
//	pthread_t	pid;
//	oop_source *oop;	//事件
//	void	*queue;	//缓冲队列
//	int pipes[2];		//管道
//	Log	*loginfo;
//	char	data[MAX_DATA_LEN];		//缓冲区
//	int lock;
//	int mem_num;
//	Memcache mems[MAX_MEMCACHE_NUM];	//memcache
//}Thread;


//typedef struct conf{
//	int reload_time;
//	char conf_name[MAX_PATH_LEN];
//	char logpath[MAX_PATH_LEN];
//	int file_size;
//	time_t last_time;
//	int thread_num;
//	int conn_num;
//	int port;
//	char program_name[50];
//	int argc;
//	char **argv;
//} conf;


////日志
//struct Log{
//	int		port;					//端口号
//	int		reload_time;			//重新加载时间
//	char	conf[MAX_PATH_LEN];		//日志文件名
//	char	logpath[MAX_PATH_LEN];	//日志路径
//	int		file_size;				//日志轮换大小
//	time_t	last_time;				//上次修改时间
//	char	data[MAX_DATA_LEN];		//缓冲区
//	oop_source *oop;				//事件模型
//	int		sock;					//服务socket
//	HASHS_DEF(LogInfo, MAX_TYPE_NUM) table;	//哈希表
//
//	int		thread_num;				//线程数
//	Thread	threads[MAX_THREAD_NUM];	//工作线程
//	char	program_name[50];
//	int	argc;
//	char **argv;
//};

#define ERR_EXIT(s) \
do \
{ \ 
	perror(s); \
	exit(EXIT_FAILURE); \
} \
while(0);


#endif // end _COMMON_H
