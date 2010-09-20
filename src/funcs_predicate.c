/************************************************************
 * Purpose: Contains the code for several predicate
 *   functions including not, and, or, eq, neq, <=, >=, <,
 *   >, =, <>, symbolp, stringp, lexemep, numberp, integerp,
 *   floatp, oddp, evenp, listp, sequencep, and
 *   pointerp.                                               */

#define _PRDCTFUN_SOURCE_

#include <stdio.h>
#define _STDIO_INCLUDED_

#include "setup.h"

#include "core_environment.h"
#include "parser_expressions.h"
#include "core_arguments.h"
#include "type_list.h"
#include "router.h"

#include "funcs_predicate.h"

/*************************************************
 * init_predicate_functions: Defines standard
 *   math and predicate functions.
 **************************************************/
void init_predicate_functions(void *env)
{
}
