/* Purpose: Provides utility routines for manipulating and
 *   examining expressions.                                  */

#ifndef __CORE_EXPRESSIONS_OPERATORS_H__

#define __CORE_EXPRESSIONS_OPERATORS_H__

#ifndef __EXPRESSIONS_H__
#include "core_expressions.h"
#endif

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef __CORE_EXPRESSIONS_OPERATORS_SOURCE__
#define LOCALE
#else
#define LOCALE extern
#endif

LOCALE void                            core_print_expression(void *, char *, struct core_expression *);
LOCALE long                            core_calculate_expression_size(struct core_expression *);
LOCALE int                             core_count_args(struct core_expression *);
LOCALE struct core_expression        * core_copy_expression(void *, struct core_expression *);
LOCALE BOOLEAN                         core_does_expression_have_variables(struct core_expression *, int);
LOCALE BOOLEAN                         core_are_expressions_equivalent(struct core_expression *, struct core_expression *);
LOCALE struct core_expression        * core_generate_constant(void *, unsigned short, void *);
LOCALE int                             core_check_against_restrictions(void *, struct core_expression *, int);
LOCALE BOOLEAN                         core_is_constant(int);

#endif
