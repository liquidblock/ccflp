#include "buffered_reader.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>

buffered_reader *buffered_reader_create_path(const char *filename)
{
	return buffered_reader_create_path_len(filename, BUFFERED_READER_DEFAULT_LEN);
}

buffered_reader *buffered_reader_create_file(FILE *file)
{
	return buffered_reader_create_file_len(file, BUFFERED_READER_DEFAULT_LEN);
}

buffered_reader *buffered_reader_create_path_len(const char *filename, size_t buffer_len)
{
	FILE* file = fopen(filename, "r");
	if (file == NULL)
	{
		return NULL;
	}
	return buffered_reader_create_file_len(file, buffer_len);
}

buffered_reader *buffered_reader_create_file_len(FILE *file, size_t buffer_len)
{
	if (buffer_len <= 0)
	{
		errno = EINVAL;
		return NULL;
	}
	if (file == NULL)
	{
		errno = EINVAL;
		return NULL;
	}
	buffered_reader *reader = (buffered_reader *) malloc(sizeof(buffered_reader));
	reader->buffer = (char*)malloc(buffer_len);
	reader->buffer_len = 0;
	reader->file = file;
	reader->buffer_pos = 0;
	reader->default_len = buffer_len;
	reader->line_buffer = block_buffer_create();
	reader->skip_lf = 0;
	return reader;
}

int burred_reader_empty(buffered_reader *reader)
{
	return reader->buffer_pos >= reader->buffer_len;
}

void buffered_reader_read_char(buffered_reader *reader)
{
	if (burred_reader_empty(reader))
	{
		//reader->buffer_len = fread(reader->buffer, sizeof(char), reader->default_len, reader->file);
		fgets(reader->buffer, reader->default_len, reader->file);
		reader->buffer_len = strlen(reader->buffer);
		reader->buffer_pos = 0;
	}
}

const char *buffered_reader_read_line(buffered_reader *reader)
{
	block_buffer_clear(&reader->line_buffer);
	int empty = 1;
	while (1)
	{
		buffered_reader_read_char(reader);
		if (burred_reader_empty(reader))
		{
			if (empty)
			{
				return NULL;
			}
			else
			{
				return block_buffer_generate(reader->line_buffer);
			}
		}
		size_t len = 0;
		int eol = 0;
		if (reader->buffer[reader->buffer_pos] == '\n' && reader->skip_lf)
		{
			reader->skip_lf = 0;
			reader->buffer_pos++;
		}
		for (size_t i = reader->buffer_pos; i < reader->buffer_len; i++)
		{
			if (reader->buffer[i] == '\r')
			{
				eol = 1;
				reader->skip_lf = 1;
				break;
			}
			else if (reader->buffer[i] == '\n')
			{
				eol = 1;
				break;
			}
			empty = 0;
			len++;
		}
		if (len != 0)
		{
			block_buffer_append_memory(reader->line_buffer, reader->buffer + reader->buffer_pos, len);
		}
		reader->buffer_pos += len + (eol ? 1 : 0);
		if (eol)
		{
			return block_buffer_generate(reader->line_buffer);
		}
	}
}

void buffered_reader_close(buffered_reader *reader)
{
	if(reader->file != NULL) fclose(reader->file);
	reader->file = NULL;
}

void buffered_reader_free(buffered_reader *reader)
{
	buffered_reader_close(reader);

	if (reader->buffer != NULL)
	{
		free(reader->buffer);
		reader->buffer = NULL;
	}

	if (reader->line_buffer != NULL)
	{
		block_buffer_free(reader->line_buffer);
		reader->line_buffer = NULL;
	}

	free(reader);
}
