#include <stdio.h>
#include "block_buffer.h"

// #define BUFFERED_READER_DEFAULT_LEN (rand()%100+1) // to test the buffered reader
#define BUFFERED_READER_DEFAULT_LEN 4096

struct buffered_reader_s
{
	FILE* file;

	block_buffer *line_buffer;

	char* buffer;
	size_t buffer_len;
	size_t default_len;
	size_t buffer_pos;

	int skip_lf; // skip line feed '\n'
};

typedef struct buffered_reader_s buffered_reader;

buffered_reader *buffered_reader_create_path(const char *filename);

buffered_reader *buffered_reader_create_file(FILE *file);

buffered_reader *buffered_reader_create_path_len(const char *filename, size_t buffer_len);

buffered_reader *buffered_reader_create_file_len(FILE *file, size_t buffer_len);

const char *buffered_reader_read_line(buffered_reader *reader);

void buffered_reader_close(buffered_reader *reader);

void buffered_reader_free(buffered_reader *reader);