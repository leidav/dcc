#include "helper.h"

const char* fileName(const char* path)
{
	int last_seperator = -1;
	int i = 0;
	const char* ptr = path;
	while (*ptr) {
		if (*ptr == '/') {
			last_seperator = i;
		}
		ptr++;
		i++;
	}
	return &path[last_seperator + 1];
}
