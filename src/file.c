/*******************************************
   Author        : Jun Zhang
   Email         : ewalker.zj@gmail.com
   Last Modified : 2017-06-20 19:26
   Filename      : file.c
   Description   : 
*******************************************/
#include "file.h"


//ȡ���ļ�������
int get_file_name(char *type, struct tm *tm_now, char *dir, char *filename, int size){
	int i, flag;
	struct stat st;
	char logfile[MAX_PATH_LEN];

	//����һ���ļ�
	sprintf(logfile, "%s/%s_%04d%02d%02d.log", dir, type, tm_now->tm_year + 1900, tm_now->tm_mon + 1, tm_now->tm_mday);
	if(stat(logfile, &st)){	//�ļ�������
		strcpy(filename, logfile);
		return 0;
	}

	//��������ļ�
	flag = st.st_size > size * _M;
	strcpy(filename, logfile);
	i = 1;
	while(1){
		sprintf(logfile, "%s/%s_%04d%02d%02d_%d.log", dir, type, tm_now->tm_year + 1900, tm_now->tm_mon + 1, tm_now->tm_mday, i);
		if(stat(logfile, &st)){	//�ļ�������
			if(flag){
				strcpy(filename, logfile);
			}
			return 0;
		}
		flag = st.st_size > size * _M;
		strcpy(filename, logfile);
		i ++;
	}
	return 0;
}

//�õ��ļ����
FILE* get_type_file(Log *log, char *type, struct tm *tm_now){
	uint32_t h;
	int len = strlen(type);
	LogInfo *info;

	HASH(type, len, h);
	HASHS_SEEK(log->table, h, info);
	if(info == NULL){
		HASHS_INSERT(log->table, h, info);
		strcpy(info->type, type);
	}

	//����ļ��Ƿ����
	if(access(info->filename, F_OK)){
		if(info->fp){
			fclose(info->fp);
			info->fp = NULL;
			memset(&info->tm_open, 0, sizeof(struct tm));
		}
	}

	//����Ƿ���Ҫ�������ֻ�
	if(tm_now->tm_mday != info->tm_open.tm_mday){
		if(info->fp){
			fclose(info->fp);
			info->fp = NULL;
		}
		get_file_name(type, tm_now, log->logpath, info->filename, log->file_size);
	}else if(info->fp && ftell(info->fp) > log->file_size * _M){
		fclose(info->fp);
		info->fp = NULL;
		get_file_name(type, tm_now, log->logpath, info->filename, log->file_size);
	}
	if(info->fp == NULL){
		info->fp = fopen(info->filename, "ab");
		if(info->fp == NULL){
			return NULL;
		}
		info->tm_open = *tm_now;
	}
	return info->fp;
}
