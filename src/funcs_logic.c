#define __FUNCS_LOGIC_SOURCE__

#include <stdio.h>
#define _STDIO_INCLUDED_

#include "setup.h"

#include "core_environment.h"
#include "parser_expressions.h"
#include "core_arguments.h"
#include "type_list.h"
#include "router.h"

#include "funcs_logic.h"

void init_logic_functions(void *env)
{
    core_define_function(env, FUNC_NAME_NOT, RT_BOOL, broccoli_not, "broccoli_not", FUNC_CNSTR_NOT);
    core_define_function(env, FUNC_NAME_AND, RT_BOOL, broccoli_and, "broccoli_and", FUNC_CNSTR_AND);
    core_define_function(env, FUNC_NAME_OR, RT_BOOL, broccoli_or, "broccoli_or", FUNC_CNSTR_OR);
}

/************************************
 * broccoli_not: H/L access routine
 *   for the not function.
 *************************************/
BOOLEAN broccoli_not(void *env)
{
    core_expression_object *arg = core_get_first_arg();
    core_data_object result;

    if( arg == NULL )
    {
        return(FALSE);
    }

    if( core_eval_expression(env, arg, &result))
    {
        return(FALSE);
    }

    if( (result.value == get_false(env)) && (result.type == ATOM))
    {
        return(TRUE);
    }

    return(FALSE);
}

/************************************
 * broccoli_and: H/L access routine
 *   for the and function.
 *************************************/
BOOLEAN broccoli_and(void *env)
{
    core_expression_object *arg;
    core_data_object result;

    for( arg = core_get_first_arg(); arg != NULL; arg = core_get_next_arg(arg))
    {
        if( core_eval_expression(env, arg, &result))
        {
            return(FALSE);
        }

        if( (result.value == get_false(env)) && (result.type == ATOM))
        {
            return(FALSE);
        }
    }

    return(TRUE);
}

/***********************************
 * broccoli_or: H/L access routine
 *   for the or function.
 ************************************/
BOOLEAN broccoli_or(void *env)
{
    core_expression_object *arg;
    core_data_object result;

    for( arg = core_get_first_arg(); arg != NULL; arg = core_get_next_arg(arg))
    {
        if( core_eval_expression(env, arg, &result))
        {
            return(FALSE);
        }

        if( (result.value != get_false(env)) || (result.type != ATOM))
        {
            return(TRUE);
        }
    }

    return(FALSE);
}
