#include "types.h"
#include <limits.h>

#ifndef __CFLP_INSTANCE_HEADER
#define __CFLP_INSTANCE_HEADER

#define CFLP_VAL_INVALID (-1)
#define CFLP_VAL_EMPTY (0)
#define CFLP_VAL_MAX (INT_MAX)
#define CFLP_INSTANCE_DISTANCE_INDEX(facility_idx, customer_idx, num_facilities, num_customers) (customer_idx * num_facilities + facility_idx)

typedef int cflp_val;

struct cflp_instance_s
{
	cflp_val max_bandwith;
	cflp_val distance_costs;
	cflp_val threshold;

	// facilities
	cflp_val* fac_opening_costs;
	cflp_val* fac_max_customers;
	size_t num_customers;

	// customers
	cflp_val* cus_bandwidths;
	size_t num_facilities;

	// facilities [facility_idx] * customers [customer_idx]
	cflp_val* distances; // CFLP_INSTANCE_DISTANCE_INDEX(facility_idx, customer_idx, num_facilities, num_customers)
};

typedef struct cflp_instance_s cflp_instance_t;

cflp_instance_t* cflp_instance_create(cflp_val threshold, cflp_val max_bandwith, cflp_val* fac_max_customers, cflp_val distance_costs, cflp_val* fac_opening_costs, cflp_val* cus_bandwidths, cflp_val* distances, size_t fac_len, size_t cus_len);
cflp_instance_t* cflp_instance_copy(cflp_instance_t* other);
size_t cflp_instance_get_num_customers(cflp_instance_t* instance);
size_t cflp_instance_get_num_facilities(cflp_instance_t* instance);
cflp_val cflp_instance_bandwidth_of(cflp_instance_t* instance, size_t customer_idx);
cflp_val cflp_instance_distance(cflp_instance_t* instance, size_t facility_idx, size_t customer_idx);
cflp_val cflp_instance_opening_costs_for(cflp_instance_t* instance, size_t facility_idx);
cflp_val cflp_instance_max_customers_for(cflp_instance_t* instance, size_t facility_idx);
cflp_val cflp_instance_get_threshold(cflp_instance_t* instance);
cflp_val cflp_instance_calc_objective_value(cflp_instance_t* instance, size_t* solution, size_t solution_len);
void cflp_instance_free(cflp_instance_t* instance);

#endif
