#ifndef _H_bmathfun

#define _H_bmathfun

#ifndef __CORE_EVALUATION_H__
#include "core_evaluation.h"
#endif

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef __FUNCS_MATH_BASIC_SOURCE__
#define LOCALE
#else
#define LOCALE extern
#endif

LOCALE void init_basic_math_functions(void *);
LOCALE void broccoli_addition(void *, core_data_object_ptr);
LOCALE void broccoli_multiply(void *, core_data_object_ptr);
LOCALE void broccoli_subtraction(void *, core_data_object_ptr);
LOCALE void broccoli_division(void *, core_data_object_ptr);
LOCALE long long broccoli_convert_to_integer(void *);
LOCALE double    broccoli_covert_to_float(void *);

/***
 * Function names and constraints
 **/
#define FUNC_NAME_ADD               "+"
#define FUNC_NAME_SUB               "-"
#define FUNC_NAME_MULT              "*"
#define FUNC_NAME_DIV               "/"
#define FUNC_NAME_CAST_INT          "int"
#define FUNC_NAME_CAST_FLOAT        "float"

#define FUNC_CNSTR_ADD              "0*n"
#define FUNC_CNSTR_SUB              "1*n"
#define FUNC_CNSTR_MULT             "0*n"
#define FUNC_CNSTR_DIV              "1*n"
#define FUNC_CNSTR_CAST_INT         "11n"
#define FUNC_CNSTR_CAST_FLOAT       "11n"

#endif
