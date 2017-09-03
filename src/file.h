/*******************************************
   Author        : Jun Zhang
   Email         : ewalker.zj@gmail.com
   Last Modified : 2017-06-20 19:26
   Filename      : file.h
   Description   : 
*******************************************/
#ifndef _FILE_H
#define _FILE_H

#include "common.h"

//取得文件的名字
int get_file_name(char *type, struct tm *tm_now, char *dir, char *filename, int size);

//得到文件句柄
FILE* get_type_file(Log *log, char *type, struct tm *tm_now);

#endif  // end _FILE_H
