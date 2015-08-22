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
	customer_tuple_t* array;
	customer_tuple_t* buffer;
	size_t length;
} mergesort_customer_tuple_t;

typedef struct
{
	facility_tuple_t* array;
	facility_tuple_t* buffer;
	size_t length;
} mergesort_facility_tuple_t;

void merge_facility_tuple(mergesort_facility_tuple_t sort, size_t l, size_t m, size_t r, int order)
{
	memcpy(sort.buffer + l, sort.array + l, (m - l + 1) * sizeof(facility_tuple_t));
	memcpy(sort.buffer + m + 1, sort.array + m + 1, (r - m) * sizeof(facility_tuple_t));
	size_t p = l;
	size_t q = r;
	for (size_t i = l; i <= r; i++) {
		if (order ? sort.buffer[p].key <= sort.buffer[q].key : sort.buffer[p].key >= sort.buffer[q].key) {
			sort.array[i] = sort.buffer[p++];
		}
		else {
			sort.array[i] = sort.buffer[q--];
		}
	}
}

void merge_sort_rec_facility_tuple(mergesort_facility_tuple_t sort, size_t l, size_t r, int order)
{
	if (l < r)
	{
		int m = (l + r) / 2;
		merge_sort_rec_facility_tuple(sort, l, m, order);
		merge_sort_rec_facility_tuple(sort, m + 1, r, !order);
		merge_facility_tuple(sort, l, m, r, order);
	}
}

void merge_customer(mergesort_customer_t sort, size_t l, size_t m, size_t r, int order)
{
	memcpy(sort.buffer + l, sort.array + l, (m - l + 1) * sizeof(customer_t));
	memcpy(sort.buffer + m + 1, sort.array + m + 1, (r - m) * sizeof(customer_t));
	size_t p = l;
	size_t q = r;
	for (size_t i = l; i <= r; i++) {
		if (order ? sort.buffer[p].key <= sort.buffer[q].key : sort.buffer[p].key >= sort.buffer[q].key) {
			sort.array[i] = sort.buffer[p++];
		}
		else {
			sort.array[i] = sort.buffer[q--];
		}
	}
}

void merge_sort_rec_customer(mergesort_customer_t sort, size_t l, size_t r, int order)
{
	if (l < r)
	{
		int m = (l + r) / 2;
		merge_sort_rec_customer(sort, l, m, order);
		merge_sort_rec_customer(sort, m + 1, r, !order);
		merge_customer(sort, l, m, r, order);
	}
}

void merge_customer_tuple(mergesort_customer_tuple_t sort, size_t l, size_t m, size_t r, int order)
{
	memcpy(sort.buffer + l, sort.array + l, (m - l + 1) * sizeof(customer_tuple_t));
	memcpy(sort.buffer + m + 1, sort.array + m + 1, (r - m) * sizeof(customer_tuple_t));
	size_t p = l;
	size_t q = r;
	for (size_t i = l; i <= r; i++) {
		if (order ? sort.buffer[p].key <= sort.buffer[q].key : sort.buffer[p].key >= sort.buffer[q].key) {
			sort.array[i] = sort.buffer[p++];
		}
		else {
			sort.array[i] = sort.buffer[q--];
		}
	}
}

void merge_sort_rec_customer_tuple(mergesort_customer_tuple_t sort, size_t l, size_t r, int order)
{
	if (l < r)
	{
		int m = (l + r) / 2;
		merge_sort_rec_customer_tuple(sort, l, m, order);
		merge_sort_rec_customer_tuple(sort, m + 1, r, !order);
		merge_customer_tuple(sort, l, m, r, order);
	}
}


void merge_sort_asc_facility_tuple(facility_tuple_t* array, size_t length)
{
	mergesort_facility_tuple_t sort;
	sort.array = array;
	sort.length = length;
	sort.buffer = (facility_tuple_t*)malloc(sizeof(facility_tuple_t)* length);
	memcpy(sort.buffer, sort.array, sizeof(facility_tuple_t)* length);
	merge_sort_rec_facility_tuple(sort, 0, length - 1, 1);
	free(sort.buffer);
}

void merge_sort_dsc_customer(customer_t* array, size_t length)
{
	mergesort_customer_t sort;
	sort.array = array;
	sort.length = length;
	sort.buffer = (customer_t*)malloc(sizeof(customer_t)* length);
	memcpy(sort.buffer, sort.array, sizeof(customer_t)* length);
	merge_sort_rec_customer(sort, 0, length - 1, 0);
	free(sort.buffer);
}

void merge_sort_dsc_customer_tuple(customer_tuple_t* array, size_t length)
{
	mergesort_customer_tuple_t sort;
	sort.array = array;
	sort.length = length;
	sort.buffer = (customer_tuple_t*)malloc(sizeof(customer_tuple_t)* length);
	memcpy(sort.buffer, sort.array, sizeof(customer_tuple_t)* length);
	merge_sort_rec_customer_tuple(sort, 0, length - 1, 0);
	free(sort.buffer);
}

cflp_val recentCost(facility_t* facility)
{
	return facility->user == 0 ? facility->opening_costs : 0;
}

int canAddUser(facility_t* facility, cflp_val maxBandwidth, cflp_val bandwidth)
{
	return facility->user < facility->max_user && facility->bandwidth + bandwidth <= maxBandwidth;
}

void addUser(facility_t* facility, cflp_val bandwidth)
{
	facility->user++;
	facility->bandwidth += bandwidth;
}

void removeUser(facility_t* facility, cflp_val bandwidth)
{
	facility->user--;
	facility->bandwidth -= bandwidth;
}

void reset(facility_t* facility)
{
	facility->user = 0;
	facility->bandwidth = 0;
}

typedef struct {
	void* context;
	size_t* solution;
	cflp_val* UpperBoundInclusive;
	cflp_val maxBandwidth;
	size_t solution_len;
} branch_t;

void branch(branch_t* args, customer_t* customer)
{
	for (facility_tuple_t* facilityTuple = customer->nearest; facilityTuple != NULL; facilityTuple = facilityTuple->next) {
		facility_t* facility = facilityTuple->value;
		cflp_val newCost = customer->cost + recentCost(facility) + facilityTuple->key;
		if (newCost + customer->lower <= *args->UpperBoundInclusive) { // L < U Bounding
			int bandwidth = customer->key;
			if (canAddUser(facility, args->maxBandwidth, bandwidth)) { // check if valid solution
				args->solution[customer->num] = facility->num;
				addUser(facility, bandwidth);
				if (customer->next == NULL) {
					*args->UpperBoundInclusive = newCost - 1;
					bnb_set_solution(args->context, newCost, args->solution, args->solution_len);
				}
				else {
					customer->next->cost = newCost;
					branch(args, customer->next);
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
		merge_sort_asc_facility_tuple(nearest, instance->num_facilities);
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
	merge_sort_dsc_customer(customersBandwidth, instance->num_customers);
	merge_sort_dsc_customer_tuple(customerMaxMin, instance->num_customers);
	customer_t* begin = &customersBandwidth[0];
	customer_t* ptr = begin;
	for (size_t i = 1; i < instance->num_customers; i++) {
		ptr->next = &customersBandwidth[i];
		ptr = ptr->next;
	}
	// calculateLowerBound
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
	branch_t args;
	args.context = context;
	args.maxBandwidth = instance->max_bandwith;
	args.solution = solution;
	args.solution_len = instance->num_customers;
	args.UpperBoundInclusive = &upper_bound_inc;
	branch(&args, begin);

	for (size_t i = 0; i < instance->num_customers; i++)
	{
		free(customersBandwidth[i].nearest);
		customersBandwidth[i].nearest = NULL;
	}
	free(customersBandwidth);
	free(facilities);
	free(solution);
}
