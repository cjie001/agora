#ifndef SLOG_H
#define SLOG_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MAX_LFATALINE_LEN
#define MAX_LINE_LEN            256    /*the max length of line*/
#endif

#define SLOG_DEBUG(format, args...) slog_write_with_postion(__FILE__, __FUNCTION__, __LINE__, LL_DEBUG, format, ## args)
#define SLOG_TRACE(format, args...) slog_write_with_postion(__FILE__, __FUNCTION__, __LINE__, LL_TRACE, format, ## args )
#define SLOG_NOTICE(format, args...) slog_write_with_postion(NULL,    NULL,          -1,      LL_NOTICE,format, ## args )
#define SLOG_WARNING(format, args...) slog_write_with_postion(__FILE__, __FUNCTION__, __LINE__, LL_WARNING, format, ## args )
#define SLOG_FATAL(format, args...) slog_write_with_postion(__FILE__, __FUNCTION__, __LINE__, LL_FATAL, format, ## args )

//#define slog_write(level, format, args...) slog_write_with_postion(NULL, NULL, -1, (level), format,  ## args )

extern const char ** slog_thr_tag(void);


/* the thread tag. CAN NOT be accessed before slog_open
 * it was assigned at the begin of given thread, the value assigned must can be acess with the whole
 * lift cycle of the thread util resigned by another value.
 * the default value is the thread id.
 * for example: slogThrTag = "WORK_THR", then the log is :
 * NOTICE 2005-12-30 10:20:30 WORK_THR#
 */
#define slogThrTag	(*slog_thr_tag())


/* the log level*/
typedef enum
{
    LL_ALL,  /*all event*/    
    LL_DEBUG, /*debug level*/
    LL_TRACE, /*trace level*/
    LL_NOTICE,/*notice level*/
    LL_WARNING, /*warning level, not crash error*/
    LL_FATAL,  /*fatal level*/
    LL_NONE  /*no log*/
}TLogLevel;


/*sepcial configure*/
typedef struct
{
    unsigned log2TTY : 1;  /*is log to stderr at the same time?*/	
    unsigned other : 31;   /*reverve*/
}TLogSpec;

typedef struct
{ 
    TLogLevel minLevel;  /*the mini level loged*/
    int maxLen;     /*the max lenght of log (cut of is overflow)*/
    TLogSpec spec;  /* special configure */
}TLogConf; 

/*
 * open the process log, slog_open need the version of configure file
 * need HOME/conf/common.conf
 * kill the process if fail
 *
 */
extern void open_log(char *homeDir,char *procName);

/**
 * set the level of log
 */
extern int slog_set_level(int level);

/*
 * open the process log, need inited at the begin of process(often begin of main)
 * @return 0  - succ
 *         -1 - fail
 * @param logDir      - the dir of log file
 * @param processName - the name of process(used to prefix the log file)
 * @param logConf     - the configure of log, default configure if NULL
 */
extern int slog_open(const char *logDir,const char *processName, TLogConf *logConf);

/*
 * write to log(suit for both process and thread)
 * @return 0  - succ
 * @return -1 - fail
 * @param level  - the log level
 * @param format ..., the param 
 */

extern int slog_write_with_postion(const char * file, const char * function, int line,
				       int level, const char * format, ...);


/*
 * @deprecated use SLOG_XXXX instead 
 **/
extern int slog_write(int level, const char *format, ... );

extern int slog_close(int isErr); 

#ifdef __cplusplus
}
#endif

#endif

