#include "cflp_instance_reader.h"
#include "buffered_reader.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int cflp_instance_reader_whitespace(char c)
{
	return c == ' ' || c == '\t';
}

int cflp_instance_reader_ignore_line(const char* line)
{
	size_t line_len = strlen(line);
	size_t ptr = 0;
	for (ptr = 0; ptr < line_len; ptr++)
	{
		if (line[ptr] == '#')
		{
			break;
		}
		else if (!cflp_instance_reader_whitespace(line[ptr]))
		{
			return 0;
		}
	}
	return 1;
}

const char* cflp_instance_reader_read_line(buffered_reader_t* reader)
{
	const char* line = NULL;
	do
	{
		line = buffered_reader_read_line(reader);
	} while (line != NULL && cflp_instance_reader_ignore_line(line));
	return line;
}

int cflp_instance_reader_read_int(buffered_reader_t* reader, const char* prefix)
{
	const char* line = cflp_instance_reader_read_line(reader);
	if (line == NULL)
	{
		return -1;
	}
	size_t line_len = strlen(line);
	size_t prefix_len = strlen(prefix);
	if (line_len < prefix_len)
	{
		return -1;
	}
	if (memcmp(line, prefix, prefix_len) != 0)
	{
		return -1;
	}
	return atoi(line + prefix_len);
}


int* cflp_instance_reader_fill_int_list(const char* line, size_t line_len, int* array, size_t num)
{
	block_buffer_t* buffer = block_buffer_create();
	int number = 0;
	size_t number_pos = 0;
	int error = 0;
	for (size_t i = 0; i < line_len + 1; i++)
	{
		if (cflp_instance_reader_whitespace(line[i]) || line[i] == '\0')
		{
			if (number)
			{
				const char* num_c = block_buffer_generate(buffer);
				if (number_pos >= num)
				{
					error = 1;
					break;
				}
				array[number_pos++] = atoi(num_c);
				block_buffer_clear(&buffer);
				number = 0;
			}
		}
		else
		{
			if (!number)
			{
				number = 1;
			}
			block_buffer_append_character(buffer, line[i]);
		}
	}
	if (number_pos != num)
	{
		error = 1;
	}
	block_buffer_free(buffer);
	buffer = NULL;
	if (error)
	{
		return NULL;
	}
	return array;
}

int* cflp_instance_reader_read_int_list(buffered_reader_t* reader, const char* prefix, size_t num)
{
	const char* line = cflp_instance_reader_read_line(reader);
	if (line == NULL)
	{
		return NULL;
	}
	size_t line_len = strlen(line);
	size_t prefix_len = strlen(prefix);
	if (line_len < prefix_len)
	{
		return NULL;
	}
	if (memcmp(line, prefix, prefix_len) != 0)
	{
		return NULL;
	}
	line += prefix_len;
	line_len -= prefix_len;
	int* array = (int*)malloc(sizeof(int) * num);
	int* result = cflp_instance_reader_fill_int_list(line, line_len, array, num);
	if (result == NULL)
	{
		free(array);
		array = NULL;
	}
	return result;
}


int cflp_instance_reader_read_int_array(buffered_reader_t* reader, int* bandwidths, int* distances, int num_facilities, int num_customers)
{
	for (int i = 0; i < num_customers; i++)
	{
		const char* line = cflp_instance_reader_read_line(reader);
		if (line == NULL)
		{
			return 0;
		}
		size_t linelen = strlen(line);
		block_buffer_t* buffer = block_buffer_create();
		int error = 1;
		for (size_t k = 0; k < linelen; k++)
		{
			if (line[k] == ';')
			{
				const char* num = block_buffer_generate(buffer);
				bandwidths[i] = atoi(num);

				int* res = cflp_instance_reader_fill_int_list(line + k + 1, linelen - k - 1, distances + num_facilities * i, num_facilities);

				if (res != NULL)
				{
					error = 0;
				}
				error = 0;
				break;
			}
			else
			{
				block_buffer_append_character(buffer, line[k]);
			}
		}
		block_buffer_free(buffer);
		buffer = NULL;
		if (error)
		{
			return 0;
		}
	}
	return 1;
}

cflp_instance_t* cflp_instance_reader_read_instance(const char* path)
{
	buffered_reader_t* reader = NULL;
	if (path == NULL)
	{
		reader = buffered_reader_create_file(stdin);
	}
	else
	{
		reader = buffered_reader_create_path(path);
	}

	if (reader == NULL)
	{
		return NULL;
	}

	int* max_customers = NULL;
	int* opening_costs = NULL;
	int* bandwidths = NULL;
	int* distances = NULL;
	cflp_instance_t* result = NULL;

	do
	{
		int threshold = cflp_instance_reader_read_int(reader, "THRESHOLD:");
		if (threshold <= 0)
		{
			break;
		}
		int num_facilities = cflp_instance_reader_read_int(reader, "FACILITIES:");
		if (num_facilities <= 0)
		{
			break;
		}
		int num_customers = cflp_instance_reader_read_int(reader, "CUSTOMERS:");
		if (num_customers <= 0)
		{
			break;
		}
		int max_bandwidth = cflp_instance_reader_read_int(reader, "MAXBANDWIDTH:");
		if (max_bandwidth <= 0)
		{
			break;
		}
		max_customers = cflp_instance_reader_read_int_list(reader, "MAXCUSTOMERS:", num_facilities);
		if (max_customers == NULL)
		{
			break;
		}
		int distance_costs = cflp_instance_reader_read_int(reader, "DISTANCECOSTS:");
		if (distance_costs <= 0)
		{
			break;
		}
		opening_costs = cflp_instance_reader_read_int_list(reader, "OPENINGCOSTS:", num_facilities);
		if (opening_costs == NULL)
		{
			break;
		}

		distances = (cflp_val*)malloc(sizeof(cflp_val) * num_facilities * num_customers);
		bandwidths = (cflp_val*)malloc(sizeof(cflp_val) * num_customers);
		
		int res = cflp_instance_reader_read_int_array(reader, bandwidths, distances, num_facilities, num_customers);
		if (!res)
		{
			break;
		}

		result = cflp_instance_create(threshold, max_bandwidth, max_customers, distance_costs, opening_costs, bandwidths, distances, num_facilities, num_customers);

	} while (0);

	if (max_customers != NULL) free(max_customers);
	max_customers = NULL;
	if (opening_costs != NULL) free(opening_costs);
	opening_costs = NULL;
	if (bandwidths != NULL) free(bandwidths);
	bandwidths = NULL;
	if (distances != NULL) free(distances);
	distances = NULL;

	buffered_reader_free(reader);
	
	return result;
}
