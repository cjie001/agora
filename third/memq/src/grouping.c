#include <stdint.h>
#include <stdio.h>
#include <string.h>

int grouping(const char *str, unsigned gnum)
{
	if (!str || !*str || !gnum) return -1;

	uint64_t h = 0;
	do {
		h = (h << 7) - h + *((unsigned char *)str) + 987654321L;
		h &= 0x0000ffffffffL;
	} while (*(++str));

	return h % gnum;
}

#ifdef GROUPING_TEST
#include <stdlib.h>
int main(int argc, char **argv)
{
	printf("%s, g %d\n", argv[1], grouping(argv[1], atoi(argv[2])));
	return 0;
}
#endif
