#define _FUNC_COMPARISON_SOURCE_

#include <stdio.h>
#define _STDIO_INCLUDED_

#include "setup.h"

#include "core_environment.h"
#include "parser_expressions.h"
#include "core_arguments.h"
#include "type_list.h"
#include "router.h"

#include "funcs_comparison.h"

/*************************************************
 * init_predicate_functions: Defines standard
 *   math and predicate functions.
 **************************************************/
void func_init_comparisons(void *env)
{
    core_define_function(env, FUNC_NAME_EQUAL, RT_BOOL, broccoli_equal, "broccoli_equal", FUNC_CNSTR_EQUAL);
    core_define_function(env, FUNC_NAME_NOT_EQUAL, RT_BOOL, broccoli_not_equal, "broccoli_not_equal", FUNC_CNSTR_NOT_EQUAL);

    core_define_function(env, "<=", 'b', broccoli_less_than_or_equal, "broccoli_less_than_or_equal", "2*n");
    core_define_function(env, ">=", 'b', broccoli_greater_than_or_equal, "broccoli_greater_than_or_equal", "2*n");
    core_define_function(env, "<", 'b', broccoli_less_than, "broccoli_less_than", "2*n");
    core_define_function(env, ">", 'b', broccoli_greater_than, "broccoli_greater_than", "2*n");
}

/***********************************
 * EqFunction: H/L access routine
 *   for the eq function.
 ************************************/
BOOLEAN broccoli_equal(void *env)
{
    core_data_object item, nextItem;
    int numArgs, i;
    struct core_expression *expr;

    /*====================================
     * Determine the number of arguments.
     *====================================*/
    numArgs = core_get_arg_count(env);

    if( numArgs == 1 )
    {
        return(TRUE);
    }

    /*==============================================
     * Get the value of the first argument against
     * which subsequent arguments will be compared.
     *==============================================*/

    expr = core_get_first_arg();
    core_eval_expression(env, expr, &item);

    /*=====================================
     * Compare all arguments to the first.
     * If any are the same, return FALSE.
     *=====================================*/

    expr = core_get_next_arg(expr);

    for( i = 2 ; i <= numArgs ; i++ )
    {
        core_eval_expression(env, expr, &nextItem);

        if( core_get_type(nextItem) != core_get_type(item))
        {
            return(FALSE);
        }

        if( core_get_type(nextItem) == LIST )
        {
            if( are_list_segments_equal(&nextItem, &item) == FALSE )
            {
                return(FALSE);
            }
        }
        else if( nextItem.value != item.value )
        {
            return(FALSE);
        }

        expr = core_get_next_arg(expr);
    }

    /*=====================================
     * All of the arguments were different
     * from the first. Return TRUE.
     *=====================================*/

    return(TRUE);
}

/************************************
 * NeqFunction: H/L access routine
 *   for the neq function.
 *************************************/
BOOLEAN broccoli_not_equal(void *env)
{
    core_data_object item, nextItem;
    int numArgs, i;
    struct core_expression *expr;

    /*====================================
     * Determine the number of arguments.
     *====================================*/

    numArgs = core_get_arg_count(env);

    if( numArgs == 1 )
    {
        return(FALSE);
    }

    /*==============================================
     * Get the value of the first argument against
     * which subsequent arguments will be compared.
     *==============================================*/

    expr = core_get_first_arg();
    core_eval_expression(env, expr, &item);

    /*=====================================
     * Compare all arguments to the first.
     * If any are different, return FALSE.
     *=====================================*/

    for( i = 2, expr = core_get_next_arg(expr); i <= numArgs; i++, expr = core_get_next_arg(expr))
    {
        core_eval_expression(env, expr, &nextItem);

        if( core_get_type(nextItem) != core_get_type(item))
        {
            continue;
        }
        else if( nextItem.type == LIST )
        {
            if( are_list_segments_equal(&nextItem, &item) == TRUE )
            {
                return(FALSE);
            }
        }
        else if( nextItem.value == item.value )
        {
            return(FALSE);
        }
    }

    /*=====================================
     * All of the arguments were identical
     * to the first. Return TRUE.
     *=====================================*/

    return(TRUE);
}

/****************************************
 * broccoli_less_than_or_equal: H/L access
 *   routine for the <= function.
 *****************************************/
BOOLEAN broccoli_less_than_or_equal(void *env)
{
    core_expression_object *arg;
    core_data_object rv1, rv2;
    int pos = 1;

    /*=========================
     * Get the first argument.
     *=========================*/

    arg = core_get_first_arg();

    if( arg == NULL )
    {
        return(TRUE);
    }

    if( !core_get_numeric_arg(env, arg, "<=", &rv1, FALSE, pos))
    {
        return(FALSE);
    }

    pos++;

    /*====================================================
     * Compare each of the subsequent arguments to its
     * predecessor. If any is greater, then return FALSE.
     *====================================================*/

    for( arg = core_get_next_arg(arg);
         arg != NULL;
         arg = core_get_next_arg(arg), pos++ )
    {
        if( !core_get_numeric_arg(env, arg, "<=", &rv2, FALSE, pos))
        {
            return(FALSE);
        }

        if( rv1.type == INTEGER )
        {
            if( rv2.type == INTEGER )
            {
                if( to_long(rv1.value) > to_long(rv2.value))
                {
                    return(FALSE);
                }
            }
            else
            {
                if((double)to_long(rv1.value) > to_double(rv2.value))
                {
                    return(FALSE);
                }
            }
        }
        else
        {
            if( rv2.type == INTEGER )
            {
                if( to_double(rv1.value) > (double)to_long(rv2.value))
                {
                    return(FALSE);
                }
            }
            else
            {
                if( to_double(rv1.value) > to_double(rv2.value))
                {
                    return(FALSE);
                }
            }
        }

        rv1.type = rv2.type;
        rv1.value = rv2.value;
    }

    /*======================================
     * Each argument was less than or equal
     * to it predecessor. Return TRUE.
     *======================================*/

    return(TRUE);
}

/*******************************************
 * broccoli_greater_than_or_equal: H/L access
 *   routine for the >= function.
 ********************************************/
BOOLEAN broccoli_greater_than_or_equal(void *env)
{
    core_expression_object *arg;
    core_data_object rv1, rv2;
    int pos = 1;

    /*=========================
     * Get the first argument.
     *=========================*/

    arg = core_get_first_arg();

    if( arg == NULL )
    {
        return(TRUE);
    }

    if( !core_get_numeric_arg(env, arg, ">=", &rv1, FALSE, pos))
    {
        return(FALSE);
    }

    pos++;

    /*===================================================
     * Compare each of the subsequent arguments to its
     * predecessor. If any is lesser, then return FALSE.
     *===================================================*/

    for( arg = core_get_next_arg(arg);
         arg != NULL;
         arg = core_get_next_arg(arg), pos++ )
    {
        if( !core_get_numeric_arg(env, arg, ">=", &rv2, FALSE, pos))
        {
            return(FALSE);
        }

        if( rv1.type == INTEGER )
        {
            if( rv2.type == INTEGER )
            {
                if( to_long(rv1.value) < to_long(rv2.value))
                {
                    return(FALSE);
                }
            }
            else
            {
                if((double)to_long(rv1.value) < to_double(rv2.value))
                {
                    return(FALSE);
                }
            }
        }
        else
        {
            if( rv2.type == INTEGER )
            {
                if( to_double(rv1.value) < (double)to_long(rv2.value))
                {
                    return(FALSE);
                }
            }
            else
            {
                if( to_double(rv1.value) < to_double(rv2.value))
                {
                    return(FALSE);
                }
            }
        }

        rv1.type = rv2.type;
        rv1.value = rv2.value;
    }

    /*=========================================
     * Each argument was greater than or equal
     * to its predecessor. Return TRUE.
     *=========================================*/

    return(TRUE);
}

/*********************************
 * broccoli_less_than: H/L access
 *   routine for the < function.
 **********************************/
BOOLEAN broccoli_less_than(void *env)
{
    core_expression_object *arg;
    core_data_object rv1, rv2;
    int pos = 1;

    /*=========================
     * Get the first argument.
     *=========================*/

    arg = core_get_first_arg();

    if( arg == NULL )
    {
        return(TRUE);
    }

    if( !core_get_numeric_arg(env, arg, "<", &rv1, FALSE, pos))
    {
        return(FALSE);
    }

    pos++;

    /*==========================================
     * Compare each of the subsequent arguments
     * to its predecessor. If any is greater or
     * equal, then return FALSE.
     *==========================================*/

    for( arg = core_get_next_arg(arg);
         arg != NULL;
         arg = core_get_next_arg(arg), pos++ )
    {
        if( !core_get_numeric_arg(env, arg, "<", &rv2, FALSE, pos))
        {
            return(FALSE);
        }

        if( rv1.type == INTEGER )
        {
            if( rv2.type == INTEGER )
            {
                if( to_long(rv1.value) >= to_long(rv2.value))
                {
                    return(FALSE);
                }
            }
            else
            {
                if((double)to_long(rv1.value) >= to_double(rv2.value))
                {
                    return(FALSE);
                }
            }
        }
        else
        {
            if( rv2.type == INTEGER )
            {
                if( to_double(rv1.value) >= (double)to_long(rv2.value))
                {
                    return(FALSE);
                }
            }
            else
            {
                if( to_double(rv1.value) >= to_double(rv2.value))
                {
                    return(FALSE);
                }
            }
        }

        rv1.type = rv2.type;
        rv1.value = rv2.value;
    }

    /*=================================
     * Each argument was less than its
     * predecessor. Return TRUE.
     *=================================*/

    return(TRUE);
}

/************************************
 * broccoli_greater_than: H/L access
 *   routine for the > function.
 *************************************/
BOOLEAN broccoli_greater_than(void *env)
{
    core_expression_object *arg;
    core_data_object rv1, rv2;
    int pos = 1;

    /*=========================
     * Get the first argument.
     *=========================*/

    arg = core_get_first_arg();

    if( arg == NULL )
    {
        return(TRUE);
    }

    if( !core_get_numeric_arg(env, arg, ">", &rv1, FALSE, pos))
    {
        return(FALSE);
    }

    pos++;

    /*==========================================
     * Compare each of the subsequent arguments
     * to its predecessor. If any is lesser or
     * equal, then return FALSE.
     *==========================================*/

    for( arg = core_get_next_arg(arg);
         arg != NULL;
         arg = core_get_next_arg(arg), pos++ )
    {
        if( !core_get_numeric_arg(env, arg, ">", &rv2, FALSE, pos))
        {
            return(FALSE);
        }

        if( rv1.type == INTEGER )
        {
            if( rv2.type == INTEGER )
            {
                if( to_long(rv1.value) <= to_long(rv2.value))
                {
                    return(FALSE);
                }
            }
            else
            {
                if((double)to_long(rv1.value) <= to_double(rv2.value))
                {
                    return(FALSE);
                }
            }
        }
        else
        {
            if( rv2.type == INTEGER )
            {
                if( to_double(rv1.value) <= (double)to_long(rv2.value))
                {
                    return(FALSE);
                }
            }
            else
            {
                if( to_double(rv1.value) <= to_double(rv2.value))
                {
                    return(FALSE);
                }
            }
        }

        rv1.type = rv2.type;
        rv1.value = rv2.value;
    }

    /*================================
     * Each argument was greater than
     * its predecessor. Return TRUE.
     *================================*/

    return(TRUE);
}
