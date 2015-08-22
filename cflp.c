#include "cflp.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct
{
	size_t num;
	size_t user;
	cflp_val bandwidth;
	size_t max_user;
	cflp_val opening_costs;
} facility_t;

typedef struct facility_tuple_s
{
	cflp_val key;
	facility_t* value;
	struct facility_tuple_s* next;
} facility_tuple_t;

typedef struct customer_s
{
	cflp_val key;
	size_t num;
	facility_tuple_t* nearest;
	struct customer_s* next;
	cflp_val lower;
	cflp_val cost;
} customer_t;

typedef struct
{
	cflp_val key;
	customer_t* customer;
} customer_tuple_t;

typedef struct
{
	customer_t* array;
	customer_t* buffer;
	size_t length;
} mergesort_customer_t;

typedef struct
{
	facility_tuple_t* array;
	facility_tuple_t* buffer;
	size_t length;
} mergesort_facility_tuple_t;

typedef struct
{
	const char* array;
	char* buffer;
	size_t size;
	size_t length;
} mergesort_t;

int key_customer(const void* obj)
{
	return ((customer_t*)obj)->key;
}

int key_customer_tuple(const void* obj)
{
	return ((customer_tuple_t*)obj)->key;
}

int key_facility_tuple(const void* obj)
{
	return ((facility_tuple_t*)obj)->key;
}

typedef int(*key_func)(const void* obj);

void merge(mergesort_t sort, size_t l, size_t m, size_t r, int order, key_func func)
{
	size_t index = (l)* sort.size;
	size_t length = (m - l + 1)* sort.size;
	memcpy(sort.buffer + index, sort.array + index, length);
	index = (m + 1)* sort.size;
	length = (r - m)* sort.size;
	memcpy(sort.buffer + index, sort.array + index, length);
	size_t p = l;
	size_t q = r;
	for (size_t i = l; i <= r; i++) {
		int pkey = func(sort.buffer + p * sort.size);
		int qkey = func(sort.buffer + q * sort.size);
		if (order ? pkey <= qkey : pkey >= qkey) {
			memcpy(sort.array + i * sort.size, sort.buffer + (p++) * sort.size, sort.size);
		}
		else {
			memcpy(sort.array + i * sort.size, sort.buffer + (q--) * sort.size, sort.size);
		}
	}
}

void merge_sort_rec(mergesort_t sort, size_t l, size_t r, int order, key_func func)
{
	if (l < r)
	{
		int m = (l + r) / 2;
		merge_sort_rec(sort, l, m, order, func);
		merge_sort_rec(sort, m + 1, r, !order, func);
		merge(sort, l, m, r, order, func);
	}
}

void merge_sort_asc(void* array, size_t length, size_t size, key_func func)
{
	mergesort_t sort;
	sort.array = (char*)array;
	sort.length = length;
	sort.size = size;
	sort.buffer = (char*)malloc(size * length);
	memcpy(sort.buffer, sort.array, size * length);
	merge_sort_rec(sort, 0, length - 1, 1, func);
	free(sort.buffer);
}

void merge_sort_dsc(void* array, size_t length, size_t size, key_func func)
{
	mergesort_t sort;
	sort.array = (char*)array;
	sort.length = length;
	sort.size = size;
	sort.buffer = (char*)malloc(size * length);
	memcpy(sort.buffer, sort.array, size * length);
	merge_sort_rec(sort, 0, length - 1, 0, func);
	free(sort.buffer);
}

static __inline cflp_val recentCost(facility_t* facility)
{
	return facility->user == 0 ? facility->opening_costs : 0;
}

static __inline int canAddUser(facility_t* facility, cflp_val maxBandwidth, cflp_val bandwidth)
{
	return facility->user < facility->max_user && facility->bandwidth + bandwidth <= maxBandwidth;
}

static __inline void addUser(facility_t* facility, cflp_val bandwidth)
{
	facility->user++;
	facility->bandwidth += bandwidth;
}

static __inline void removeUser(facility_t* facility, cflp_val bandwidth)
{
	facility->user--;
	facility->bandwidth -= bandwidth;
}

static __inline void reset(facility_t* facility)
{
	facility->user = 0;
	facility->bandwidth = 0;
}

void branch(void* context, customer_t* customer, size_t* solution, cflp_val* UpperBoundInclusive, cflp_val maxBandwidth, size_t solution_len)
{
	for (facility_tuple_t* facilityTuple = customer->nearest; facilityTuple != NULL; facilityTuple = facilityTuple->next) {
		facility_t* facility = facilityTuple->value;
		cflp_val newCost = customer->cost + recentCost(facility) + facilityTuple->key;
		if (newCost + customer->lower <= *UpperBoundInclusive) { // L < U Bounding
			int bandwidth = customer->key;
			if (canAddUser(facility, maxBandwidth, bandwidth)) { // check if valid solution
				solution[customer->num] = facility->num;
				addUser(facility, bandwidth);
				if (customer->next == NULL) {
					*UpperBoundInclusive = newCost - 1;
					bnb_set_solution(context, newCost, solution, solution_len);
				}
				else {
					customer->next->cost = newCost;
					branch(context, customer->next, solution, UpperBoundInclusive, maxBandwidth, solution_len);
				}
				removeUser(facility, bandwidth);
			}
		}
		else if (facility->user > 0) { // Theo's improvement
			return;
		}
	}
}

void bnb_run(void* context, cflp_instance_t* instance)
{
	size_t* solution = (size_t*)malloc(sizeof(size_t)* instance->num_customers);
	cflp_val upper_bound_inc = CFLP_VAL_MAX;

	// calculateBestCustomers
	customer_t* customersBandwidth = (customer_t*)malloc(sizeof(customer_t)* instance->num_customers);
	customer_tuple_t* customerMaxMin = (customer_tuple_t*)malloc(sizeof(customer_tuple_t)* instance->num_customers);
	facility_t* facilities = (facility_t*)malloc(sizeof(facility_t)* instance->num_facilities);
	for (size_t k = 0; k < instance->num_facilities; k++)
	{
		facilities[k].bandwidth = 0;
		facilities[k].max_user = instance->fac_max_customers[k];
		facilities[k].num = k;
		facilities[k].opening_costs = instance->fac_opening_costs[k];
		facilities[k].user = 0;
	}
	for (size_t i = 0; i < instance->num_customers; i++)
	{
		facility_tuple_t* nearest = (facility_tuple_t*)malloc(sizeof(facility_tuple_t)* instance->num_facilities);
		for (size_t k = 0; k < instance->num_facilities; k++)
		{
			nearest[k].key = instance->distances[CFLP_INSTANCE_DISTANCE_INDEX(k, i, instance->num_facilities, instance->num_customers)] * instance->distance_costs;
			nearest[k].value = &facilities[k];
			nearest[k].next = NULL;
		}
		merge_sort_asc(nearest, instance->num_facilities, sizeof(facility_tuple_t), key_facility_tuple);
		facility_tuple_t* ptr = &nearest[0];
		for (size_t k = 1; k < instance->num_facilities; k++)
		{
			ptr->next = &nearest[k];
			ptr = ptr->next;
		}
		customersBandwidth[i].key = instance->cus_bandwidths[i];
		customersBandwidth[i].num = i;
		customersBandwidth[i].nearest = nearest;
		customersBandwidth[i].lower = 0;
		customersBandwidth[i].next = NULL;
		customersBandwidth[i].cost = 0;

		customerMaxMin[i].key = nearest[instance->num_facilities - 1].key - nearest[0].key;
		customerMaxMin[i].customer = &customersBandwidth[i];
	}
	merge_sort_dsc(customersBandwidth, instance->num_customers, sizeof(customer_t), key_customer);
	merge_sort_dsc(customerMaxMin, instance->num_customers, sizeof(customer_tuple_t), key_customer_tuple);
	customer_t* begin = &customersBandwidth[0];
	customer_t* ptr = begin;
	for (size_t i = 1; i < instance->num_customers; i++) {
		ptr->next = &customersBandwidth[i];
		ptr = ptr->next;
	}

	// calculateLowerBound - Dual Heuristic
	cflp_val cost = 0;
	customersBandwidth[instance->num_customers - 1].lower = cost;
	for (size_t i = instance->num_customers - 1; i > 0; i--)
	{
		customer_t* customer = &customersBandwidth[i];
		cflp_val minDistance = customer->nearest->key;
		cost += minDistance;
		customersBandwidth[i - 1].lower = cost;
	}

	// calculateUpperBound - Primal Heuristic
	cost = 0;
	int valid_solution = 0;
	for (size_t i = 0; i < instance->num_customers; i++) {
		customer_t* customer = customerMaxMin[i].customer;
		valid_solution = 0;
		for (facility_tuple_t* facilityTuple = customer->nearest; facilityTuple != NULL; facilityTuple = facilityTuple->next) {
			facility_t* facility = facilityTuple->value;
			if (canAddUser(facility, instance->max_bandwith, customer->key)) {
				cost += recentCost(facility) + facilityTuple->key;
				addUser(facility, customer->key);
				solution[customer->num] = facility->num;
				valid_solution = 1;
				break;
			}
		}
		if (!valid_solution)
		{
			break;
		}
	}
	if (valid_solution)
	{
		upper_bound_inc = cost - 1;
		bnb_set_solution(context, cost, solution, instance->num_customers);
	}
	for (int i = 0; i < instance->num_facilities; i++) {
		reset(&facilities[i]);
	}

	// run
	branch(context, begin, solution, &upper_bound_inc, instance->max_bandwith, instance->num_customers);

	for (size_t i = 0; i < instance->num_customers; i++)
	{
		free(customersBandwidth[i].nearest);
		customersBandwidth[i].nearest = NULL;
	}
	free(customersBandwidth);
	free(customerMaxMin);
	free(facilities);
	free(solution);
}
