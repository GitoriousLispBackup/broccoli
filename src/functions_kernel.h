#ifndef __FUNCTIONS_KERNEL_H__
#define __FUNCTIONS_KERNEL_H__

#include "funcs_function.h"
#ifndef __EXPRESSIONS_H__
#include "core_expressions.h"
#endif
#ifndef __CORE_EVALUATION_H__
#include "core_evaluation.h"
#endif

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef __FUNCTIONS_KERNEL_SOURCE__
#define LOCALE
#else
#define LOCALE extern
#endif

LOCALE void function_execute(void *, FUNCTION_DEFINITION *, core_expression_object *, core_data_object *);

#endif
