/*******************************************
   Author        : Jun Zhang
   Email         : ewalker.zj@gmail.com
   Last Modified : 2017-07-10 16:26
   conf_file      : config.c
   Description   : 
*******************************************/
#include "config.h"


int change_program_name(int argc, char **argv, char *name)
{
	char tmp[20];
	int i;
	sprintf(tmp, "%s", name);
	for(i = 1; i < argc; i++)
	{
		memset(argv[i], 0, strlen(argv[i]));
	}

	memset(*argv, 0, strlen(*argv));
	strcpy(*argv, tmp);
	prctl(PR_SET_NAME, name);

	return 0;
}


int load_conf(CONF *conf, const char *conf_file, LOGTABLE *table)
{
	if(!conf || !conf_file)
	{
		//slog(LL_FATAL, "load conf, malloc conf failed");
		return -1;
	}
	struct stat st = {0};

	if(*conf -> conf_name == 0)
	{
		strcpy(conf -> conf_name, conf_file);
		char tmp[MAX_PATH_LEN];
		GetProfileString(conf_file, "global", "slog", "run", tmp, MAX_PATH_LEN);
		TLogConf conf;
		conf.minLevel = LL_ALL;
		conf.maxLen = 256 * 1024 * 1024;
		conf.spec.log2TTY = 0;
		slog_open(".", tmp, &conf);
	}

	if(stat(conf_file, &st))
	{
		return 0;
	}

	if(st.st_mtime == conf -> last_time)
	{
		return 0;
	}

	conf -> last_time = st.st_mtime;
	conf -> port = GetProfileInt(conf_file, "global", "port", 50000);
	conf -> backdog = GetProfileInt(conf_file, "global", "backdog", 1024);
	conf -> worker_num = GetProfileInt(conf_file, "global", "worker_num", 32);
	conf -> conn_num = GetProfileInt(conf_file, "global", "conn_num", 10);
	conf -> queue_worker_num = GetProfileInt(conf_file, "global", "queue_worker_num", 32);
	conf -> reload_time = GetProfileInt(conf_file, "global", "reload_time", 2);
	conf -> file_size = GetProfileInt(conf_file, "global", "file_size", 1024);
	GetProfileString(conf_file, "global", "logpath", "log", conf -> logpath, MAX_PATH_LEN);
	//GetProfileString(conf_file, "global", "ip", "", conf -> ip, 16);

	if(access(conf -> logpath, F_OK))
	{
		mkdirs(conf -> logpath);
	}

	
	int i = 0;
	while(1){
		char tmp[256];
		char type_name[256];
		char type_queue[256];
		sprintf(tmp, "type%d_name", i);
		GetProfileString(conf_file, "global", tmp, "", type_name, 256);
		if(*type_name == 0){
			break;
		}
		sprintf(tmp, "type%d_queue", i);
		GetProfileString(conf_file, "global", tmp, "", type_queue, 256);
		LogInfo *node;
		int len = strlen(type_name);
		uint32_t h;
		HASH(type_name, len, h);
		HASHS_INSERT(*table, h, node);
		if(node){
			strcpy(node->type, type_name);
			strcpy(node->queue_info, type_queue);
			slog_write(LL_WARNING, "%s\t%s", type_name, node -> queue_info);
		}
		i ++;
	}

	//得到程序的名字
	GetProfileString(conf_file, "global", "program_name", "", conf -> program_name, 50);
	if(*conf -> program_name){
		change_program_name(conf -> argc, conf -> argv, conf -> program_name);
	}
	return 0;
}


int dump_conf(CONF *conf, char *data){
	return sprintf("%s\n", conf->logpath);
}
