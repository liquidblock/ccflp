#include <stdio.h>
#include "block_buffer.h"

//4096
#define BUFFERED_READER_DEFAULT_LEN (rand()%100+1)

struct buffered_reader_s
{
	FILE* file;

	block_buffer_t* line_buffer;

	char* buffer;
	size_t buffer_len;
	size_t default_len;
	size_t buffer_pos;

	int skip_lf; // line feed 0x0A
};

typedef struct buffered_reader_s buffered_reader_t;

buffered_reader_t* buffered_reader_create_path(const char* filename);
buffered_reader_t* buffered_reader_create_file(FILE* file);
buffered_reader_t* buffered_reader_create_path_len(const char* filename, size_t buffer_len);
buffered_reader_t* buffered_reader_create_file_len(FILE* file, size_t buffer_len);
const char* buffered_reader_read_line(buffered_reader_t* reader);
void buffered_reader_close(buffered_reader_t* reader);
void buffered_reader_free(buffered_reader_t* reader);