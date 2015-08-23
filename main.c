#include <stdio.h>
#include "cflp_instance_reader.h"
#include "block_buffer.h"
#include "cflp.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/timeb.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#ifdef WIN32
#define PTW32_STATIC_LIB

#endif
#include <pthread.h>

typedef long long millisec;

void bailOut(const char* msg)
{
	printf("\nERR %s\n", msg);
}

millisec currentTimeMillis()
{
	struct timeb tmb;
	ftime(&tmb);
	return tmb.time * 1000 + tmb.millitm;
}

typedef struct
{
	cflp_instance_t* instance;
	pthread_mutex_t mutex;
	cflp_val upper_bound;
	size_t* solution;
	size_t solution_length;
	pthread_cond_t cond;
} bnb_args;

void set_solution(bnb_args* args, int new_upper_bound, size_t* new_solution, int new_solution_length)
{
	int res = pthread_mutex_lock(&args->mutex);
	if (res == 0)
	{
		if (args->solution != NULL)
		{
			free(args->solution);
		}
		args->solution = (size_t*)malloc(sizeof(size_t)* new_solution_length);
		memcpy(args->solution, new_solution, sizeof(size_t)*  new_solution_length);
		args->solution_length = new_solution_length;
		args->upper_bound = new_upper_bound;
		pthread_mutex_unlock(&args->mutex);
	}
}

void bnb_set_solution(void* context, cflp_val new_upper_bound, size_t* new_solution, size_t new_solution_length)
{
	set_solution((bnb_args*)context, new_upper_bound, new_solution, new_solution_length);
}

void* run_thread(void* param)
{
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	bnb_args* args = (bnb_args*)param;
	pthread_mutex_lock(&args->mutex);
	pthread_cond_signal(&args->cond);
	pthread_mutex_unlock(&args->mutex);
	cflp_instance_t* instance = args->instance;
	bnb_run(param, instance);
	return NULL;
}

void run(cflp_instance_t* instance, int dontStop, int test, int debug, const char* choppedFileName)
{
	cflp_instance_t* original = cflp_instance_copy(instance);
	
	millisec start = currentTimeMillis();
	millisec end = currentTimeMillis();
	millisec offs = end - start;
	millisec timeout = 30000; // 30 secounds

	pthread_t thread;
	bnb_args args;
	args.instance = instance;
	pthread_mutex_init(&args.mutex, NULL);
	pthread_cond_init(&args.cond, NULL);
	args.solution = NULL;
	args.solution_length = 0;
	args.upper_bound = CFLP_VAL_INVALID;

	

	pthread_create(&thread, NULL, run_thread, &args);
	pthread_mutex_lock(&args.mutex);
	pthread_cond_wait(&args.cond, &args.mutex);
	pthread_mutex_unlock(&args.mutex);

	struct timeb tmb;
	ftime(&tmb);
	struct timespec abstime;
	abstime.tv_nsec = ((timeout % 1000) + tmb.millitm) * 1000000;
	abstime.tv_sec = tmb.time + timeout / 1000;
	pthread_timedjoin_np(thread, NULL, &abstime);
	pthread_mutex_lock(&args.mutex);
	pthread_cancel(thread);
	pthread_mutex_unlock(&args.mutex);
	pthread_join(thread, NULL);
	pthread_mutex_destroy(&args.mutex);
	pthread_cond_destroy(&args.cond);

	end = currentTimeMillis();

	millisec sum123 = end - start - offs;

	int* used_bandwidths = NULL;
	int* connected_customers = NULL;
	block_buffer_t* msg = block_buffer_create();
	do
	{
		if (args.solution == NULL)
		{
			bailOut("Keine gueltige Loesung!");
			break;
		}

		cflp_val upper_bound = args.upper_bound;
		size_t* solution = args.solution;
		size_t solution_length = args.solution_length;

		if (solution_length != original->num_customers)
		{
			bailOut("Ihre Loesung hat zu wenige/viele Kunden!");
			break;
		}

		used_bandwidths = (int*)malloc(sizeof(int)* cflp_instance_get_num_facilities(original));
		memset(used_bandwidths, 0, sizeof(int)* cflp_instance_get_num_facilities(original));
		connected_customers = (int*)malloc(sizeof(int)* cflp_instance_get_num_facilities(original));
		memset(connected_customers, 0, sizeof(int)* cflp_instance_get_num_facilities(original));

		size_t fIdx;
		int error = 0;
		for (size_t i = 0; i < original->num_customers; ++i) {
			fIdx = solution[i];
			if (fIdx < 0 || fIdx >= cflp_instance_get_num_facilities(original))
			{
				bailOut("Ungueltiger Facility Index!");
				error = 1;
				break;
			}

			used_bandwidths[fIdx] += cflp_instance_bandwidth_of(original, i);
			if (used_bandwidths[fIdx] > original->max_bandwith)
			{
				bailOut("Eine Facility verbraucht zu viel Bandbreite!");
				error = 1;
				break;
			}
				

			connected_customers[fIdx] += 1;
			if (connected_customers[fIdx] > cflp_instance_max_customers_for(original, fIdx))
			{
				bailOut("Eine Facility hat zu viele Kunden!");
				error = 1;
				break;
			}
		}
		if (error)
		{
			break;
		}

		cflp_val objectiveValue = cflp_instance_calc_objective_value(original, solution, solution_length);

		if (abs(objectiveValue - upper_bound) > 0) {
			bailOut("Die obere Schranke muss immer gleich der aktuell besten Loesung sein!");
			break;
		}

		if (test)
		{
			block_buffer_append_string(msg, choppedFileName);
			block_buffer_append_string(msg, ": ");
		}
		

		millisec sum = end - start - offs;
		if (debug)
		{
			printf("%s: DBG %s", choppedFileName, "Loesung: ");
		}

		if (upper_bound > original->threshold)
		{
			printf("\nERR zu schlechte Loesung: Ihr Ergebnis %d liegt ueber dem Schwellwert (%d)\n", upper_bound, original->threshold);
			break;
		}
		
		block_buffer_append_string(msg, "Schwellwert = ");
		block_buffer_append_int(msg, original->threshold);
		block_buffer_append_string(msg, ". Ihr Ergebnis ist OK mit \n");
		block_buffer_append_int(msg, upper_bound);

		if (test)
		{
			if (sum > 1000)
			{
				block_buffer_append_string(msg, ", Zeit: ");
				block_buffer_append_int(msg, sum / 1000);
				block_buffer_append_string(msg, "s");
			}
			else
			{
				block_buffer_append_string(msg, ", Zeit: ");
				block_buffer_append_int(msg, sum);
				block_buffer_append_string(msg, "ms");
			}
			
		}
		printf("\n%s\n", block_buffer_generate(msg));
	} while (0);
	
	block_buffer_free(msg);

	if (args.solution != NULL)
	{
		free(args.solution);
		args.solution = NULL;
	}
	if (used_bandwidths != NULL)
	{
		free(used_bandwidths);
		used_bandwidths = NULL;
	}
	if (connected_customers != NULL)
	{
		free(connected_customers);
		connected_customers = NULL;
	}

	cflp_instance_free(original);
}

int main(int argv, char** argc)
{
	const char* fileName = NULL;
	const char* choppedFileName = ""; // TODO: assign choppedFileName
	int dontStop = 1;
	int test = 1;
	int debug = 0;

	for (int i = 1; i < argv; i++)
	{
		if (strcmp(argc[i], "-s") == 0)
		{
			dontStop = 1;
		}
		else if (strcmp(argc[i], "-t") == 0)
		{
			test = 1;
		}
		else if (strcmp(argc[i], "-d") == 0)
		{
			debug = test = 1;
		}
		else
		{
			fileName = argc[i];
		}
	}
	
	cflp_instance_t* instance = cflp_instance_reader_read_instance(fileName);
	if (instance != NULL)
	{
		run(instance, dontStop, test, debug, choppedFileName);
		cflp_instance_free(instance);
	}
	else
	{
		perror("Could not load instance!");
	}
	instance = NULL;
}
