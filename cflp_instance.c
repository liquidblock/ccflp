#include "cflp_instance.h"
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#ifndef min
#define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })
#endif

cflp_instance_t* cflp_instance_create(cflp_val threshold, cflp_val max_bandwith, cflp_val* fac_max_customers, cflp_val distance_costs, cflp_val* fac_opening_costs, cflp_val* cus_bandwidths, cflp_val* distances, size_t fac_len, size_t cus_len)
{
	cflp_instance_t* instance = (cflp_instance_t*)malloc(sizeof(cflp_instance_t));

	instance->threshold = threshold;
	instance->max_bandwith = max_bandwith;
	instance->distance_costs = distance_costs;
	instance->num_customers = cus_len;
	instance->num_facilities = fac_len;

	instance->fac_max_customers = (cflp_val*)malloc(fac_len * sizeof(cflp_val));
	memcpy(instance->fac_max_customers, fac_max_customers, fac_len * sizeof(cflp_val));

	instance->fac_opening_costs = (cflp_val*)malloc(fac_len * sizeof(cflp_val));
	memcpy(instance->fac_opening_costs, fac_opening_costs, fac_len * sizeof(cflp_val));

	instance->cus_bandwidths = (cflp_val*)malloc(cus_len * sizeof(cflp_val));
	memcpy(instance->cus_bandwidths, cus_bandwidths, cus_len * sizeof(cflp_val));

	instance->distances = (cflp_val*)malloc(fac_len * cus_len * sizeof(cflp_val));
	memcpy(instance->distances, distances, fac_len * cus_len * sizeof(cflp_val));

	return instance;
}

cflp_instance_t* cflp_instance_copy(cflp_instance_t* other)
{
	return cflp_instance_create(other->threshold, other->max_bandwith, other->fac_max_customers, other->distance_costs, other->fac_opening_costs, other->cus_bandwidths, other->distances, other->num_facilities, other->num_customers);
}

size_t cflp_instance_get_num_customers(cflp_instance_t* instance)
{
	return instance->num_customers;
}

size_t cflp_instance_get_num_facilities(cflp_instance_t* instance)
{
	return instance->num_facilities;
}

cflp_val cflp_instance_bandwidth_of(cflp_instance_t* instance, size_t customer_idx)
{
	return instance->cus_bandwidths[customer_idx];
}

cflp_val cflp_instance_distance(cflp_instance_t* instance, size_t facility_idx, size_t customer_idx)
{
	return instance->distances[CFLP_INSTANCE_DISTANCE_INDEX(facility_idx, customer_idx, instance->num_facilities, instance->num_customers)];
}

cflp_val cflp_instance_opening_costs_for(cflp_instance_t* instance, size_t facility_idx)
{
	return instance->fac_opening_costs[facility_idx];
}

cflp_val cflp_instance_max_customers_for(cflp_instance_t* instance, size_t facility_idx)
{
	return instance->fac_max_customers[facility_idx];
}

cflp_val cflp_instance_get_threshold(cflp_instance_t* instance)
{
	return instance->threshold;
}

cflp_val cflp_instance_calc_objective_value(cflp_instance_t* instance, size_t* solution, size_t solution_len)
{
	size_t opened_facilities_len = cflp_instance_get_num_facilities(instance) * sizeof(cflp_val);
	size_t* opened_facilities = (size_t*)malloc(opened_facilities_len * sizeof(size_t));
	memset(opened_facilities, 0, opened_facilities_len * sizeof(size_t));

	if (solution_len != cflp_instance_get_num_customers(instance))
	{
		errno = EINVAL;
		return CFLP_VAL_INVALID;
	}

	cflp_val sum_costs = CFLP_VAL_EMPTY;
	for (size_t i = 0; i < min(solution_len, cflp_instance_get_num_customers(instance)); i++)
	{
		if (solution[i] < 0)
			continue;

		if (!opened_facilities[solution[i]])
		{
			sum_costs += instance->fac_opening_costs[solution[i]];
			opened_facilities[solution[i]] = 1;
		}
		sum_costs += instance->distance_costs * cflp_instance_distance(instance, solution[i], i);
	}

	free(opened_facilities);
	opened_facilities = NULL;

	return sum_costs;
}

void cflp_instance_free(cflp_instance_t* instance)
{
	free(instance->fac_max_customers);
	instance->fac_max_customers = NULL;

	free(instance->fac_opening_costs);
	instance->fac_opening_costs = NULL;

	free(instance->cus_bandwidths);
	instance->cus_bandwidths = NULL;

	free(instance->distances);
	instance->distances = NULL;

	free(instance);
}
