/*******************************************
   Author        : Jun Zhang
   Email         : ewalker.zj@gmail.com
   Last Modified : 2017-06-20 17:59
   Filename      : protocol.h
   Description   : 
*******************************************/
#ifndef _PROTOCOL_H
#define _PROTOCOL_H

#include "types.h"
#include "global.h"

typedef struct LogHead LogHead;

struct LogHead {
	uint32 size;
	char type[MAX_TYPE_LEN];
	char log[0];
};


#endif  // end _PROTOCOL_H
