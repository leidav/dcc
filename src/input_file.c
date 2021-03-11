#include "input_file.h"

#include <stdlib.h>

static size_t getFileSize(FILE* file)
{
	fseek(file, 0, SEEK_END);
	size_t size = ftell(file);
	fseek(file, 0, SEEK_SET);
	return size;
}
static size_t readChunk(struct InputFile* file)
{
	return fread(file->buffer, INPUT_CHUNK_SIZE, 1, file->file);
}

int openInputFile(struct InputFile* file, const char* path, const char* name)
{
	FILE* f = fopen(path, "r");
	if (!f) {
		return -1;
	}
	file->name = name;
	file->file = f;
	char* buffer = malloc(INPUT_CHUNK_SIZE);
	if (!buffer) {
		fclose(f);
		readChunk(file);
	}
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
