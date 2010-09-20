/* Purpose: Provides functions for performing operations on
 *   constraint records including computing the intersection
 *   and union of constraint records.                        */

#ifndef __CONSTRAINTS_OPERATORS_H__
#define __CONSTRAINTS_OPERATORS_H__


#ifndef __CORE_EVALUATION_H__
#include "core_evaluation.h"
#endif
#ifndef __CONSTRAINTS_KERNEL_H__
#include "constraints_kernel.h"
#endif

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef __CONSTRAINTS_OPERATORS_SOURCE__
#define LOCALE
#else
#define LOCALE extern
#endif

LOCALE struct constraint_metadata       * constraint_intersection(void *, struct constraint_metadata *, struct constraint_metadata *);
LOCALE struct constraint_metadata       * constraint_union(void *, struct constraint_metadata *, struct constraint_metadata *);
LOCALE struct constraint_metadata       * create_constraint(void *);
LOCALE int                                set_constraint_type(int, CONSTRAINT_META *);
LOCALE CONSTRAINT_META                  * convert_arg_type_to_constraint(void *, int);
LOCALE CONSTRAINT_META                  * convert_expression_to_constraint(void *, struct core_expression *);

#endif
