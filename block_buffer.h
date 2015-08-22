#include "types.h"

// #define BLOCK_BUFFER_DEFAULT_SIZE (rand()%100+1) // to test the block buffer
#define BLOCK_BUFFER_DEFAULT_SIZE 128

struct block_buffer_segment_s
{
	char* buffer;
	size_t buffer_size;
	struct block_buffer_segment_s* next;
};

struct block_buffer_s
{
	struct block_buffer_segment_s* head;
	struct block_buffer_segment_s* current;
	size_t position;
	size_t segment_size;
	char* buffer;
	int buffer_changed;
};

typedef struct block_buffer_s block_buffer_t;

block_buffer_t* block_buffer_create();
block_buffer_t* block_buffer_create_len(size_t segment_size);
void block_buffer_append_string(block_buffer_t* buffer, const char* string);
void block_buffer_append_memory(block_buffer_t* buffer, const char* memory, size_t memory_len);
void block_buffer_append_character(block_buffer_t* buffer, const char character);
const char* block_buffer_generate(block_buffer_t* buffer);
void block_buffer_clear(block_buffer_t** buffer);
void block_buffer_free(block_buffer_t* buffer);
