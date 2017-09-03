/*******************************************
   Author        : Jun Zhang
   Email         : ewalker.zj@gmail.com
   Last Modified : 2017-04-27 07:14
   Filename      : timer.h
   Description   : 
*******************************************/
#ifndef _TIMER_H
#define _TIMER_H

#include<time.h>


struct timespec diff(struct timespec start, struct timespec end){
	struct timespec diffval;
	if((end.tv_nsec - start.tv_nsec) < 0){
		diffval.tv_sec = end.tv_sec - start.tv_sec - 1;
		diffval.tv_nsec = end.tv_nsec - start.tv_nsec + 1000000000;
	}
 	else{
		diffval.tv_sec = end.tv_sec - start.tv_sec;
		diffval.tv_nsec = end.tv_nsec - start.tv_nsec;
	 }
	return diffval;
}

void curr_time(char* nowtime, int size){
	time_t now_time = time(NULL);
	struct tm* local;
	local = localtime(&now_time);
	char buf[128] = {0};
	strftime(buf, 64, "%Y-%m-%d %H:%M:%S", local);
	sprintf(nowtime, buf, size);
}

#endif   // end _TIMER_H
