#include "cflp.h"
#include <string.h>
#include <stdlib.h>

typedef struct
{
	size_t num;
	size_t user;
	cflp_val bandwidth;
	size_t max_user;
	cflp_val opening_costs;
} facility_st;

typedef struct facility_tuple_s
{
	cflp_val key;
	facility_st *value;
	struct facility_tuple_s* next;
} facility_tuple_st;

typedef struct customer_s
{
	cflp_val key;
	size_t num;
	facility_tuple_st *nearest;
	struct customer_s* next;
	cflp_val lower;
	cflp_val cost;
} customer_st;

typedef struct
{
	cflp_val key;
	customer_st *customer;
} customer_tuple_st;

typedef struct
{
	facility_tuple_st *array;
	facility_tuple_st *buffer;
	size_t length;
} mergesort_st;

void merge(mergesort_st sort, size_t l, size_t m, size_t r, int order)
{
	memcpy(sort.buffer + l, sort.array + l, (m - l + 1) * sizeof(facility_tuple_st));
	memcpy(sort.buffer + m + 1, sort.array + m + 1, (r - m) * sizeof(facility_tuple_st));
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

void merge_sort_rec(mergesort_st sort, size_t l, size_t r, int order)
{
	if (l < r)
	{
		int m = (l + r) / 2;
		merge_sort_rec(sort, l, m, order);
		merge_sort_rec(sort, m + 1, r, !order);
		merge(sort, l, m, r, order);
	}
}

void merge_sort_asc(facility_tuple_st *array, size_t length)
{
	mergesort_st sort;
	sort.array = array;
	sort.length = length;
	sort.buffer = (facility_tuple_st *) malloc(sizeof(facility_tuple_st) * length);
	memcpy(sort.buffer, sort.array, sizeof(facility_tuple_st) * length);
	merge_sort_rec(sort, 0, length - 1, 1);
	free(sort.buffer);
}

void merge_sort_dsc(facility_tuple_st *array, size_t length)
{
	mergesort_st sort;
	sort.array = array;
	sort.length = length;
	sort.buffer = (facility_tuple_st *) malloc(sizeof(facility_tuple_st) * length);
	memcpy(sort.buffer, sort.array, sizeof(facility_tuple_st) * length);
	merge_sort_rec(sort, 0, length - 1, 1);
	free(sort.buffer);
}

cflp_val recentCost(facility_st *facility)
{
	return facility->user == 0 ? facility->opening_costs : 0;
}

int canAddUser(facility_st *facility, cflp_val maxBandwidth, cflp_val bandwidth)
{
	return facility->user < facility->max_user && facility->bandwidth + bandwidth <= maxBandwidth;
}

void addUser(facility_st *facility, cflp_val bandwidth)
{
	facility->user++;
	facility->bandwidth += bandwidth;
}

void removeUser(facility_st *facility, cflp_val bandwidth)
{
	facility->user--;
	facility->bandwidth -= bandwidth;
}

void branch(void *context, customer_st *customer, size_t *solution, cflp_val *UpperBoundInclusive,
			cflp_val maxBandwidth, size_t solution_len)
{
	for (facility_tuple_st *facilityTuple = customer->nearest;
		 facilityTuple != NULL; facilityTuple = facilityTuple->next) {
		facility_st *facility = facilityTuple->value;
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

void bnb_run(void *context, cflp_instance *instance)
{
	// calculateBestCustomers
	customer_st *customersBandwidth = (customer_st *) malloc(sizeof(customer_st) * instance->num_customers);
	facility_st *facilities = (facility_st *) malloc(sizeof(facility_st) * instance->num_facilities);
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
		facility_tuple_st *nearest = (facility_tuple_st *) malloc(sizeof(facility_tuple_st) * instance->num_facilities);
		for (size_t k = 0; k < instance->num_facilities; k++)
		{
			nearest[k].key = instance->distances[CFLP_INSTANCE_DISTANCE_INDEX(k, i, instance->num_facilities, instance->num_customers)] * instance->distance_costs;
			nearest[k].value = &facilities[k];
			nearest[k].next = NULL;
		}
		merge_sort_asc(nearest, instance->num_facilities);
		facility_tuple_st *ptr = &nearest[0];
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
	}
	// TODO: sort customersBandwidth here
	customer_st *begin = &customersBandwidth[0];
	customer_st *ptr = begin;
	for (size_t i = 1; i < instance->num_customers; i++) {
		ptr->next = &customersBandwidth[i];
		ptr = ptr->next;
	}
	// calculateLowerBound
	cflp_val cost = 0;
	customersBandwidth[instance->num_customers - 1].lower = cost;
	for (size_t i = instance->num_customers - 1; i > 0; i--)
	{
		customer_st *customer = &customersBandwidth[i];
		cflp_val minDistance = customer->nearest->key;
		cost += minDistance;
		customersBandwidth[i - 1].lower = cost;
	}
	// TODO: use Upper Bound Heuristic here

	size_t* solution = (size_t*)malloc(sizeof(size_t)* instance->num_customers);
	
	cflp_val upper_bound_inc = CFLP_VAL_MAX;
	branch(context, begin, solution, &upper_bound_inc, instance->max_bandwith, instance->num_customers);
	
	free(solution);
	solution = NULL;
	
	for (size_t i = 0; i < instance->num_customers; i++)
	{
		free(customersBandwidth[i].nearest);
		customersBandwidth[i].nearest = NULL;
	}
	free(customersBandwidth);
	free(facilities);
}
