#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <math.h>
#include "block_buffer.h"
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#ifndef max
#define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })
#endif

struct block_buffer_segment_s* block_buffer_segment_create(size_t segment_size)
{
	struct block_buffer_segment_s* segment = (struct block_buffer_segment_s*)malloc(sizeof(struct block_buffer_segment_s));
	segment->buffer_size = segment_size;
	segment->buffer = (char*)malloc(segment_size * sizeof(char));
	segment->next = NULL;
	return segment;
}

void block_buffer_add_segment(block_buffer_t* buffer, size_t segment_size)
{
	buffer->current->next = block_buffer_segment_create(segment_size);
	buffer->current = buffer->current->next;
	buffer->position = 0;
}

void block_buffer_free_segment(struct block_buffer_segment_s* segment)
{
	segment->next = NULL;
	free(segment->buffer);
	segment->buffer = NULL;

	free(segment);
}

block_buffer_t* block_buffer_create()
{
	return block_buffer_create_len(BLOCK_BUFFER_DEFAULT_SIZE);
}

block_buffer_t* block_buffer_create_len(size_t segment_size)
{
	if (segment_size <= 0)
	{
		errno = EINVAL;
		return NULL;
	}
	block_buffer_t* buffer = (block_buffer_t*)malloc(sizeof(block_buffer_t));
	buffer->buffer = NULL;
	buffer->buffer_changed = 1;
	buffer->position = 0;
	buffer->segment_size = segment_size;
	buffer->head = block_buffer_segment_create(segment_size);
	buffer->current = buffer->head;
	return buffer;
}

void block_buffer_append_string(block_buffer_t* buffer, const char* string)
{
	block_buffer_append_memory(buffer, string, strlen(string));
}

void block_buffer_append_memory(block_buffer_t* buffer, const char* memory, size_t memory_len)
{
	if (memory_len <= 0)
	{
		return;
	}
	buffer->buffer_changed = 1;
	if (buffer->position + memory_len <= buffer->current->buffer_size)
	{
		memcpy(buffer->current->buffer + buffer->position, memory, memory_len);
		buffer->position += memory_len;
	}
	else
	{
		size_t copy_len = buffer->current->buffer_size - buffer->position;
		if (copy_len > 0)
		{
			memcpy(buffer->current->buffer + buffer->position, memory, copy_len);
			buffer->position += copy_len;
			memory += copy_len; // add copy_len bytes to the memory pointer
			memory_len -= copy_len;
		}
		block_buffer_add_segment(buffer, max(buffer->segment_size, memory_len));
		block_buffer_append_memory(buffer, memory, memory_len);
	}
}

void block_buffer_append_character(block_buffer_t* buffer, const char character)
{
	buffer->buffer_changed = 1;
	if (buffer->position < buffer->current->buffer_size)
	{
		buffer->current->buffer[buffer->position++] = character;
	}
	else
	{
		block_buffer_add_segment(buffer, buffer->segment_size);
		block_buffer_append_character(buffer, character);
	}
}

const char* block_buffer_generate(block_buffer_t* buffer)
{
	if (buffer->buffer_changed)
	{
		if (buffer->buffer != NULL)
		{
			free(buffer->buffer);
			buffer->buffer = NULL;
		}
	}

	if (buffer->buffer == NULL)
	{
		size_t buffer_len = 0;
		struct block_buffer_segment_s* pointer = buffer->head;
		while (pointer != NULL)
		{
			if (pointer->next == NULL)
			{
				buffer_len += buffer->position;
			}
			else
			{
				buffer_len += pointer->buffer_size;
			}
			pointer = pointer->next;
		}
		buffer->buffer = (char*)malloc(buffer_len + 1); // buffer_len + escape char '\0'

		pointer = buffer->head;
		char* memory = buffer->buffer;
		while (pointer != NULL)
		{
			if (pointer->next == NULL)
			{
				memcpy(memory, pointer->buffer, buffer->position);
				memory += buffer->position;
			}
			else
			{
				memcpy(memory, pointer->buffer, pointer->buffer_size);
				memory += pointer->buffer_size;
			}
			pointer = pointer->next;
		}
		*memory = '\0';
	}

	buffer->buffer_changed = 0;
	return buffer->buffer;
}

void block_buffer_clear(block_buffer_t** buffer)
{
	block_buffer_free(*buffer);
	*buffer = block_buffer_create();
}

void block_buffer_free(block_buffer_t* buffer)
{
	while (buffer->head != NULL)
	{
		struct block_buffer_segment_s* next = buffer->head->next;
		block_buffer_free_segment(buffer->head);
		buffer->head = next;
	}
	buffer->current = NULL;

	if (buffer->buffer != NULL)
	{
		free(buffer->buffer);
		buffer->buffer = NULL;
	}

	free(buffer);
}
