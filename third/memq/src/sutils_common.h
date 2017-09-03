#ifndef SUTILSCOMMON_H
#define SUTILSCOMMON_H

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <ctime>
#include <string>
#include <vector>
#include <stdexcept>
#include <sstream>  
#include <stdint.h>

using namespace std;

#include <time.h>
#include <sys/time.h>

string serialize32(uint32_t t);
string serialize8(uint8_t t);
string serialize64(uint64_t t);

#endif
