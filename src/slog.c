#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>
#include "slog.h"

#ifdef DLOG
#include "dlog.h"
#endif

#define MAX_PATH_LEN            256             /* 最大路径长度 */
#define DEF_MAX_LOG_LEN 64000
#define MAX_LOG_MSG_LEN 65536

typedef struct
{
	TLogLevel minLevel;
	int maxLen;
	TLogSpec spec;
	char pathName[5][MAX_PATH_LEN];
	FILE *fp[5];
}TProcessLogConf;

static TProcessLogConf logConf;
static pthread_mutex_t logMutex = PTHREAD_MUTEX_INITIALIZER;
static int openFlag = 0;
static pthread_key_t logThrTagKey;
static int keyFlag = 0;
int slog_set_level(int level) {
    logConf.minLevel = level;
    return level;
}

int slog_open(const char *logDir,const char *processName, TLogConf *conf)
{
	time_t tt;
	char tmpBuf[MAX_LINE_LEN];

	if (!logDir || !processName)
		return -1;

	// set log
    int i = 0;
    if (conf)
	{
		logConf.minLevel = conf->minLevel;
		logConf.maxLen = conf->maxLen;
		logConf.spec = conf->spec;

		if (logConf.maxLen <= 0)
			logConf.maxLen = DEF_MAX_LOG_LEN;
	}
	else
	{
		logConf.minLevel = LL_ALL;
		logConf.maxLen = 64000;
		memset(&(logConf.spec), 0, sizeof(TLogSpec));
		logConf.spec.log2TTY = 1;
	}

        // open the log file
	snprintf(logConf.pathName[0], MAX_PATH_LEN, "%s/%slog.debug", logDir, processName);
    snprintf(logConf.pathName[1], MAX_PATH_LEN, "%s/%slog.trace", logDir, processName);
    snprintf(logConf.pathName[2], MAX_PATH_LEN, "%s/%slog.notice", logDir, processName);
    snprintf(logConf.pathName[3], MAX_PATH_LEN, "%s/%slog.warning", logDir, processName);
    snprintf(logConf.pathName[4], MAX_PATH_LEN, "%s/%slog.fatal", logDir, processName);
    for (i = 0; i < 5; i++) {
         if (!(logConf.fp[i]=fopen(logConf.pathName[i],"a+"))) {
              return -1;
         }
    }
	openFlag=1;

	// 生成线程私有数据(日志标志)
	if (pthread_key_create(&logThrTagKey, free) != 0)
	{
         for (i = 0; i < 5; i++) {
              fclose(logConf.fp[i]);
              return -1;
         }
	}
	keyFlag = 1;


	time(&tt);
	ctime_r(&tt,tmpBuf);

    for (i = 0; i < 5; i++) {
         fprintf(logConf.fp[i],"===============================================\n");
         fprintf(logConf.fp[i],"Open process log at %s",tmpBuf);
         fprintf(logConf.fp[i],"===============================================\n");
         fflush(logConf.fp[i]);
    }
    
	fprintf(stderr,"===============================================\n");
	fprintf(stderr,"Open process log at %s",tmpBuf);
	fprintf(stderr,"===============================================\n");
	return 0;
}

// 返回各自线程的日志标志指针
const char **slog_thr_tag(void)
{
	const char **thrTag = pthread_getspecific(logThrTagKey);
	if (thrTag == NULL)
	{
		thrTag = malloc(sizeof(const char **));
		*thrTag = NULL;
		pthread_setspecific(logThrTagKey, thrTag);
	}
	return thrTag;
}


int slog_write_with_postion(const char * file, const char * function, 
			    int line, int level, const char *format, ... )
{
	va_list args;
	char msg[MAX_LOG_MSG_LEN];
	char *p;
	time_t tt;
	struct tm ttm;
	pthread_t tid;
	int flen;
	int len;
	const char *thrTag;

    int level_index = level - 1;
	if (level<logConf.minLevel)
		return 0;

	p = msg;
  
	// log等级
	switch (level)
	{
	case LL_DEBUG:
		strcpy(p, "DEBUG ");
		p += 6;
		break;
	case LL_TRACE:
		strcpy(p, "TRACE ");
		p += 6;
		break;
	case LL_NOTICE:
		strcpy(p, "NOTICE ");
		p += 7;
		break;
	case LL_WARNING:
		strcpy(p, "WARNING ");
		p += 8;
		break;
	case LL_FATAL:
		strcpy(p, "FATAL ");
		p += 6;
		break;
	default:
		fprintf(stderr, "Invalid log level '%d'.\n", level);
		return -1;
	}
	
	// time
	time(&tt);
	localtime_r(&tt, &ttm);
	p += sprintf(p, "%d-%d-%d %02d:%02d:%02d ", ttm.tm_year + 1900, ttm.tm_mon + 1, 
				 ttm.tm_mday, ttm.tm_hour, ttm.tm_min, ttm.tm_sec);
	if (file && function && (line > 0)) {
	    p+= sprintf(p, "%s:%s:%d ",file,  function, line);
	}

	// thread
	if (keyFlag && (thrTag = slogThrTag))
	{
	    // max lenght of thrTag is 30
		*(p + 30) = 0;
		strncpy(p, thrTag, 30);
		p += strlen(p);
		strcpy(p, "# ");
		p += 2;
	}
	else
	{
		tid = pthread_self();
		p += sprintf(p, "%lu# ", tid);
	}

	va_start(args,format);
	len=vsnprintf(p,MAX_LOG_MSG_LEN-(p-msg)-1,format,args);
#ifdef DLOG
	if (level == LL_FATAL)
	{
		vdlog("FATAL", format, args);
	}
#endif
	va_end(args);

	if (len>=MAX_LOG_MSG_LEN-(p-msg)-1)
		len=MAX_LOG_MSG_LEN-(p-msg)-2;
	p+=len;

	if (*(p-1)!='\n')
		*p++='\n';
	*p=0;

  
	if (openFlag)
	{
		pthread_mutex_lock(&logMutex);

		if (fseek(logConf.fp[level_index],0,SEEK_END)==-1
			|| (flen=ftell(logConf.fp[level_index]))==-1)
		{
			fprintf(stderr,"Fail to get length of file '%s'.\n",logConf.pathName[level_index]);
			return -1;
		}

		if (flen>=logConf.maxLen)
		{
			fclose(logConf.fp[level_index]);
			openFlag=0;

			if (truncate(logConf.pathName[level_index],0)==-1)
			{
				fprintf(stderr,"Fail to truncate file '%s'.\n",logConf.pathName[level_index]);
				return -1;
			}
			if (!(logConf.fp[level_index]=fopen(logConf.pathName[level_index],"a+")))
				return -1;
			openFlag=1;
		}

		fprintf(logConf.fp[level_index],"%s",msg);
		fflush(logConf.fp[level_index]);

		pthread_mutex_unlock(&logMutex);

		if (logConf.spec.log2TTY==1)
			fprintf(stderr,"%s",msg);
	}
	else
		fprintf(stderr,"%s",msg);

	return 0;
}

int slog_write(int level, const char *format, ... )
{
	va_list args;
	char msg[MAX_LOG_MSG_LEN];
	char *p;
	time_t tt;
	struct tm ttm;
	pthread_t tid;
	int flen;
	int len;
	const char *thrTag;
    int level_index = level - 1;
	if (level<logConf.minLevel)
		return 0;

	p = msg;


	switch (level)
	{
	case LL_DEBUG:
		strcpy(p, "DEBUG ");
		p += 6;
		break;
	case LL_TRACE:
		strcpy(p, "TRACE ");
		p += 6;
		break;
	case LL_NOTICE:
		strcpy(p, "NOTICE ");
		p += 7;
		break;
	case LL_WARNING:
		strcpy(p, "WARNING ");
		p += 8;
		break;
	case LL_FATAL:
		strcpy(p, "FATAL ");
		p += 6;
		break;
	default:
		fprintf(stderr, "Invalid log level '%d'.\n", level);
		return -1;
	}
	time(&tt);
	localtime_r(&tt, &ttm);
	p += sprintf(p, "%d-%d-%d %02d:%02d:%02d ", ttm.tm_year + 1900, ttm.tm_mon + 1, 
				 ttm.tm_mday, ttm.tm_hour, ttm.tm_min, ttm.tm_sec);


	if (keyFlag && (thrTag = slogThrTag))
	{
	
		*(p + 30) = 0;
		strncpy(p, thrTag, 30);
		p += strlen(p);
		strcpy(p, "# ");
		p += 2;
	}
	else
	{
		tid = pthread_self();
		p += sprintf(p, "%lu# ", tid);
	}

	va_start(args,format);
	len=vsnprintf(p,MAX_LOG_MSG_LEN-(p-msg)-1,format,args);
#ifdef DLOG
	if (level == LL_FATAL)
	{
		vdlog("FATAL", format, args);
	}
#endif
	va_end(args);

	if (len>=MAX_LOG_MSG_LEN-(p-msg)-1)
		len=MAX_LOG_MSG_LEN-(p-msg)-2;
	p+=len;

	if (*(p-1)!='\n')
		*p++='\n';
	*p=0;

  
	if (openFlag)
	{
		pthread_mutex_lock(&logMutex);

		if (fseek(logConf.fp[level_index],0,SEEK_END)==-1
			|| (flen=ftell(logConf.fp[level_index]))==-1)
		{
			fprintf(stderr,"Fail to get length of file '%s'.\n",logConf.pathName[level_index]);
			return -1;
		}

		if (flen>=logConf.maxLen)
		{
			fclose(logConf.fp[level_index]);
			openFlag=0;

			if (truncate(logConf.pathName[level_index],0)==-1)
			{
				fprintf(stderr,"Fail to truncate file '%s'.\n",logConf.pathName[level_index]);
				return -1;
			}
			if (!(logConf.fp[level_index]=fopen(logConf.pathName[level_index],"a+")))
				return -1;
			openFlag=1;
		}

		fprintf(logConf.fp[level_index],"%s",msg);
		fflush(logConf.fp[level_index]);

		pthread_mutex_unlock(&logMutex);

		if (logConf.spec.log2TTY==1)
			fprintf(stderr,"%s",msg);
	}
	else
		fprintf(stderr,"%s",msg);

	return 0;
}




int slog_close(int isErr)
{
	time_t tt;
	char tmpBuf[MAX_LINE_LEN];

	if (isErr)
		snprintf(tmpBuf,MAX_LINE_LEN,"Abnormally close process log at ");
	else
		snprintf(tmpBuf,MAX_LINE_LEN,"Close process log at ");

	time(&tt);
	ctime_r(&tt,tmpBuf+strlen(tmpBuf));
    int i ;
    for (i = 0; i < 5; i++) {
         fprintf(logConf.fp[i],"===============================================\n");
         fprintf(logConf.fp[i],"%s",tmpBuf);
         fprintf(logConf.fp[i],"===============================================\n");
         fflush(logConf.fp[i]);
    }
	fprintf(stderr,"===============================================\n");
	fprintf(stderr,"%s",tmpBuf);
	fprintf(stderr,"===============================================\n");

	openFlag=0;

    for (i = 0; i < 5; i++) {
         fclose(logConf.fp[i]);
         logConf.fp[i]=NULL;
    }
	pthread_key_delete(logThrTagKey);

	return 0;
}
