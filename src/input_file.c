#include "input_file.h"

#include <stdlib.h>

#define MIN(x, y) ((x) < (y) ? (x) : (y))

static size_t getFileSize(FILE* file)
{
	fseek(file, 0, SEEK_END);
	size_t size = ftell(file);
	fseek(file, 0, SEEK_SET);
	return size;
}
static size_t readChunk(struct InputFile* file)
{
	size_t size = MIN(file->file_size - file->read_pos, INPUT_CHUNK_SIZE);
	return fread(file->buffer, size, 1, file->file);
}

int openInputFile(struct InputFile* file, const char* path, const char* name)
{
	FILE* f = fopen(path, "r");
	if (!f) {
		return -1;
	}
	file->name = name;
	file->file = f;
	file->read_pos = 0;
	file->file_size = getFileSize(f);
	char* buffer = malloc(INPUT_CHUNK_SIZE);
	if (!buffer) {
		fclose(f);
		return -1;
	}
	file->buffer = buffer;
	return 0;
}
char readChar(struct InputFile* file)
{
	size_t chunk_pos = file->read_pos % INPUT_CHUNK_SIZE;
	if (chunk_pos == 0) {
		readChunk(file);
	}
	if (file->read_pos == file->file_size) {
		return INPUT_EOF;
	}
	char c = file->buffer[chunk_pos];
	file->read_pos++;
	return c;
}
