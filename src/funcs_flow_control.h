#ifndef __FUNCS_FUNCTION_H__
#define __FUNCS_FUNCTION_H__

#ifndef __CORE_EVALUATION_H__
#include "core_evaluation.h"
#endif

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef __FLOW_CONTROL_SOURCE__
#define LOCALE
#else
#define LOCALE extern
#endif

typedef struct loop_count_stack
{
    long long count;
    struct loop_count_stack *nxt;
} LOOP_COUNT_STACK;

#define FLOW_CONTROL_DATA_INDEX 13

struct flow_control_data
{
    int               return_flag;
    int               break_flag;
    LOOP_COUNT_STACK *loop_count_stack;
    struct core_data *variables;
};

#define get_flow_control_data(env) ((struct flow_control_data *)core_get_environment_data(env, FLOW_CONTROL_DATA_INDEX))

LOCALE void func_init_flow_control(void *);
LOCALE void broccoli_bind(void *, core_data_object_ptr);
LOCALE void broccoli_if(void *, core_data_object_ptr);
LOCALE void broccoli_progn(void *, core_data_object_ptr);
LOCALE BOOLEAN   lookup_binding(void *, struct core_data *, struct atom_hash_node *);
LOCALE long long broccoli_get_loop_count(void *);
LOCALE void      flush_bindings(void *);

#endif
