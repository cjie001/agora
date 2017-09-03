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

//��ϣ�ڵ�
HASH_DEF_BEGIN(LogInfo)
	char type[MAX_TYPE_LEN];	//��������
	char filename[MAX_PATH_LEN];	//�ļ���
	FILE *fp;	//�ļ����
	struct tm tm_open;	//���ļ���ʱ��
	char queue_info[256];	//queue
HASH_DEF_END

typedef struct Log Log;

typedef HASHS_DEF(LogInfo, MAX_TYPE_NUM) LOGTABLE;

////�����߳�
//typedef struct{
//	int	id;	//�̺߳�
//	pthread_t	pid;
//	oop_source *oop;	//�¼�
//	void	*queue;	//�������
//	int pipes[2];		//�ܵ�
//	Log	*loginfo;
//	char	data[MAX_DATA_LEN];		//������
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


////��־
//struct Log{
//	int		port;					//�˿ں�
//	int		reload_time;			//���¼���ʱ��
//	char	conf[MAX_PATH_LEN];		//��־�ļ���
//	char	logpath[MAX_PATH_LEN];	//��־·��
//	int		file_size;				//��־�ֻ���С
//	time_t	last_time;				//�ϴ��޸�ʱ��
//	char	data[MAX_DATA_LEN];		//������
//	oop_source *oop;				//�¼�ģ��
//	int		sock;					//����socket
//	HASHS_DEF(LogInfo, MAX_TYPE_NUM) table;	//��ϣ��
//
//	int		thread_num;				//�߳���
//	Thread	threads[MAX_THREAD_NUM];	//�����߳�
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
