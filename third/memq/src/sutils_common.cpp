#include "sutils_common.h"

using namespace std;

#include <time.h>
#include <sys/time.h>

string serialize32(uint32_t t)
{
	unsigned char raw_array[4];
	raw_array[0]= (t >> 24) & 0xff;
	raw_array[1]= (t >> 16) & 0xff;
	raw_array[2]= (t >> 8) & 0xff;
	raw_array[3]= t & 0xff;
	return string(reinterpret_cast<const char *>(raw_array), 4);
}

string serialize8(uint8_t t)
{
	return string(1, t);
}

string serialize64(uint64_t t)
{
	unsigned char raw_array[8];
	raw_array[0]= (t >> 56) & 0xff;
	raw_array[1]= (t >> 48) & 0xff;
	raw_array[2]= (t >> 40) & 0xff;
	raw_array[3]= (t >> 32) & 0xff;
	raw_array[4]= (t >> 24) & 0xff;
	raw_array[5]= (t >> 16) & 0xff;
	raw_array[6]= (t >> 8) & 0xff;
	raw_array[7]= t & 0xff;
	return string(reinterpret_cast<const char *>(raw_array), 8);
}
