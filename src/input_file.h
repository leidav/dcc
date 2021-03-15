#ifndef INPUT_FILE
#define INPUT_FILE

#include <stdio.h>

#define INPUT_CHUNK_SIZE (4096 << 2)
#define INPUT_EOF 0x4

struct InputFile {
	const char* name;
	char* buffer;
	size_t read_pos;
	size_t file_size;
	FILE* file;
};

int openInputFile(struct InputFile* file, const char* path, const char* name);
char readChar(struct InputFile* file);
#endif