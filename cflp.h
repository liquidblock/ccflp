#include "cflp_instance.h"

void bnb_set_solution(void* context, cflp_val new_upper_bound, size_t* new_solution, size_t new_solution_length);
void bnb_run(void* context, cflp_instance_t* instance);
