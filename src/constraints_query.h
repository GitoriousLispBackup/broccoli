/* Purpose: Provides functions for constraint checking of
 *   data types.                                             */

#ifndef __CONSTRAINTS_QUERY_H__
#define __CONSTRAINTS_QUERY_H__

#ifndef __CONSTRAINTS_KERNEL_H__
#include "constraints_kernel.h"
#endif
#ifndef __CORE_EVALUATION_H__
#include "core_evaluation.h"
#endif

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef __CONSTRAINTS_QUERY_SOURCE__
#define LOCALE
#else
#define LOCALE extern
#endif

#define NO_VIOLATION                    0
#define TYPE_VIOLATION                  1
#define RANGE_VIOLATION                 2
#define ALLOWED_VALUES_VIOLATION        3
#define FUNCTION_RETURN_TYPE_VIOLATION  4
#define CARDINALITY_VIOLATION           5
#define ALLOWED_CLASSES_VIOLATION       6

LOCALE BOOLEAN is_type_allowed(int, void *, CONSTRAINT_META *);
LOCALE BOOLEAN is_class_allowed(void *, int, void *, CONSTRAINT_META *);
LOCALE BOOLEAN is_satisfiable(struct constraint_metadata *);
LOCALE int     garner_expression_violations(void *, struct core_expression *, CONSTRAINT_META *);
LOCALE int     garner_value_violations(void *, int, void *, CONSTRAINT_META *);
LOCALE int     garner_violations(void *, core_data_object *, CONSTRAINT_META *);
LOCALE void    report_constraint_violation_error(void *, char *, char *, int, int, struct atom_hash_node *, int, int, CONSTRAINT_META *, int);

#endif
