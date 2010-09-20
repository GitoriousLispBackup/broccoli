#ifndef __FUNCS_LIST_H__
#define __FUNCS_LIST_H__

#ifndef __CORE_EVALUATION_H__
#include "core_evaluation.h"
#endif

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef __FUNCS_LIST_SOURCE__
#define LOCALE
#else
#define LOCALE extern
#endif

LOCALE void init_list_functions(void *);
LOCALE void brocolli_create_list(void *, core_data_object_ptr);
LOCALE void broccoli_concatenate(void *, core_data_object_ptr);
LOCALE void broccoli_car(void *, core_data_object_ptr);
LOCALE void broccoli_cdr(void *, core_data_object_ptr);
LOCALE void broccoli_foreach(void *, core_data_object_ptr);
LOCALE void broccoli_slice(void *, core_data_object_ptr);
LOCALE void broccoli_get_loop_variable(void *, core_data_object_ptr);
LOCALE long      broccoli_get_loop_index(void *);
LOCALE void      broccoli_range(void *env, core_data_object_ptr sub_value);
LOCALE long long broccoli_length(void *);

/***
 * Function names and constraints
 **/
#define FUNC_NAME_CAR               "first"
#define FUNC_NAME_CDR               "rest"
#define FUNC_NAME_FOREACH           "for"
#define FUNC_NAME_PROGN_VAR         "(get-progn$-field)"
#define FUNC_NAME_PROGN_INDEX       "(get-progn$-index)"
#define FUNC_NAME_SLICE             "slice"

#define FUNC_CNSTR_CAR              "11m"
#define FUNC_CNSTR_CDR              "11m"
#define FUNC_CNSTR_PROGN            NULL
#define FUNC_CNSTR_PROGN_VAR        "00"
#define FUNC_CNSTR_PROGN_INDEX      "00"
#define FUNC_CNSTR_SLICE            "33im"

#endif
