#define __FUNCS_MATH_BASIC_SOURCE__

#include <stdio.h>
#define _STDIO_INCLUDED_

#include "setup.h"

#include "core_arguments.h"
#include "core_environment.h"
#include "parser_expressions.h"
#include "router.h"

#include "funcs_math_basic.h"

#define BASIC_MATH_FUNCTION_DATA_INDEX 6

struct basic_math_function_data
{
    BOOLEAN use_float_dividend;
};

#define get_basic_math_function_data(env) ((struct basic_math_function_data *)core_get_environment_data(env, BASIC_MATH_FUNCTION_DATA_INDEX))

/**************************************************************
 * init_basic_math_functions: Defines basic math functions.
 ***************************************************************/
void init_basic_math_functions(void *env)
{
    core_allocate_environment_data(env, BASIC_MATH_FUNCTION_DATA_INDEX, sizeof(struct basic_math_function_data), NULL);

    get_basic_math_function_data(env)->use_float_dividend = TRUE;

    core_define_function(env, FUNC_NAME_ADD, RT_INT_FLOAT, PTR_FN broccoli_addition, "AdditionFunction", FUNC_CNSTR_ADD);
    core_define_function(env, FUNC_NAME_MULT, RT_INT_FLOAT, PTR_FN broccoli_multiply, "MultiplicationFunction", FUNC_CNSTR_MULT);
    core_define_function(env, FUNC_NAME_SUB, RT_INT_FLOAT, PTR_FN broccoli_subtraction, "SubtractionFunction", FUNC_CNSTR_SUB);
    core_define_function(env, FUNC_NAME_DIV, RT_INT_FLOAT, PTR_FN broccoli_division, "DivisionFunction", FUNC_CNSTR_DIV);
    core_define_function(env, FUNC_NAME_CAST_INT, RT_LONG_LONG, PTR_FN broccoli_convert_to_integer, "IntegerFunction", FUNC_CNSTR_CAST_INT);
    core_define_function(env, FUNC_NAME_CAST_FLOAT, RT_DOUBLE, PTR_FN broccoli_covert_to_float, "FloatFunction", FUNC_CNSTR_CAST_FLOAT);
}

/*********************************
 * broccoli_addition: H/L access
 *   routine for the + function.
 **********************************/
void broccoli_addition(void *env, core_data_object_ptr ret)
{
    double ftotal = 0.0;
    long long ltotal = 0LL;
    BOOLEAN useFloatTotal = FALSE;
    core_expression_object *expr;
    core_data_object arg;
    int pos = 1;

    /*=================================================
     * Loop through each of the arguments adding it to
     * a running total. If a floating point number is
     * encountered, then do all subsequent operations
     * using floating point values.
     *=================================================*/

    expr = core_get_first_arg();

    while( expr != NULL )
    {
        if( !core_get_numeric_arg(env, expr, "+", &arg, useFloatTotal, pos))
        {
            expr = NULL;
        }
        else
        {
            expr = core_get_next_arg(expr);
        }

        if( useFloatTotal )
        {
            ftotal += to_double(arg.value);
        }
        else
        {
            if( arg.type == INTEGER )
            {
                ltotal += to_long(arg.value);
            }
            else
            {
                ftotal = (double)ltotal + to_double(arg.value);
                useFloatTotal = TRUE;
            }
        }

        pos++;
    }

    /*======================================================
     * If a floating point number was in the argument list,
     * then return a float, otherwise return an integer.
     *======================================================*/

    if( useFloatTotal )
    {
        ret->type = FLOAT;
        ret->value = (void *)store_double(env, ftotal);
    }
    else
    {
        ret->type = INTEGER;
        ret->value = (void *)store_long(env, ltotal);
    }
}

/***************************************
 * broccoli_multiply: access
 *   routine for the * function.
 ****************************************/
void broccoli_multiply(void *env, core_data_object_ptr ret)
{
    double ftotal = 1.0;
    long long ltotal = 1LL;
    BOOLEAN useFloatTotal = FALSE;
    core_expression_object *expr;
    core_data_object arg;
    int pos = 1;

    /*===================================================
     * Loop through each of the arguments multiplying it
     * by a running product. If a floating point number
     * is encountered, then do all subsequent operations
     * using floating point values.
     *===================================================*/

    expr = core_get_first_arg();

    while( expr != NULL )
    {
        if( !core_get_numeric_arg(env, expr, "*", &arg, useFloatTotal, pos))
        {
            expr = NULL;
        }
        else
        {
            expr = core_get_next_arg(expr);
        }

        if( useFloatTotal )
        {
            ftotal *= to_double(arg.value);
        }
        else
        {
            if( arg.type == INTEGER )
            {
                ltotal *= to_long(arg.value);
            }
            else
            {
                ftotal = (double)ltotal * to_double(arg.value);
                useFloatTotal = TRUE;
            }
        }

        pos++;
    }

    /*======================================================
     * If a floating point number was in the argument list,
     * then return a float, otherwise return an integer.
     *======================================================*/

    if( useFloatTotal )
    {
        ret->type = FLOAT;
        ret->value = (void *)store_double(env, ftotal);
    }
    else
    {
        ret->type = INTEGER;
        ret->value = (void *)store_long(env, ltotal);
    }
}

/************************************
 * broccoli_subtraction: access
 *   routine for the - function.
 *************************************/
void broccoli_subtraction(void *env, core_data_object_ptr ret)
{
    double ftotal = 0.0;
    long long ltotal = 0LL;
    BOOLEAN useFloatTotal = FALSE;
    core_expression_object *expr;
    core_data_object arg;
    int pos = 1;

    /*=================================================
     * Get the first argument. This number which will
     * be the starting total from which all subsequent
     * arguments will subtracted.
     *=================================================*/

    expr = core_get_first_arg();

    if( expr != NULL )
    {
        if( !core_get_numeric_arg(env, expr, "-", &arg, useFloatTotal, pos))
        {
            expr = NULL;
        }
        else
        {
            expr = core_get_next_arg(expr);
        }

        if( arg.type == INTEGER )
        {
            ltotal = to_long(arg.value);
        }
        else
        {
            ftotal = to_double(arg.value);
            useFloatTotal = TRUE;
        }

        pos++;
    }

    /*===================================================
     * Loop through each of the arguments subtracting it
     * from a running total. If a floating point number
     * is encountered, then do all subsequent operations
     * using floating point values.
     *===================================================*/

    while( expr != NULL )
    {
        if( !core_get_numeric_arg(env, expr, "-", &arg, useFloatTotal, pos))
        {
            expr = NULL;
        }
        else
        {
            expr = core_get_next_arg(expr);
        }

        if( useFloatTotal )
        {
            ftotal -= to_double(arg.value);
        }
        else
        {
            if( arg.type == INTEGER )
            {
                ltotal -= to_long(arg.value);
            }
            else
            {
                ftotal = (double)ltotal - to_double(arg.value);
                useFloatTotal = TRUE;
            }
        }

        pos++;
    }

    if( (pos - 1) == 1 )
    {
        ftotal = -ftotal;
        ltotal = -ltotal;
    }

    /*======================================================
     * If a floating point number was in the argument list,
     * then return a float, otherwise return an integer.
     *======================================================*/

    if( useFloatTotal )
    {
        ret->type = FLOAT;
        ret->value = (void *)store_double(env, ftotal);
    }
    else
    {
        ret->type = INTEGER;
        ret->value = (void *)store_long(env, ltotal);
    }
}

/**********************************
 * broccoli_division:  access
 *   routine for the / function.
 ***********************************/
void broccoli_division(void *env, core_data_object_ptr ret)
{
    double ftotal = 1.0;
    long long ltotal = 1LL;
    BOOLEAN useFloatTotal;
    core_expression_object *expr;
    core_data_object arg;
    int pos = 1;

    useFloatTotal = get_basic_math_function_data(env)->use_float_dividend;

    /*===================================================
     * Get the first argument. This number which will be
     * the starting product from which all subsequent
     * arguments will divide. If the auto float dividend
     * feature is enable, then this number is converted
     * to a float if it is an integer.
     *===================================================*/

    expr = core_get_first_arg();

    if( expr != NULL )
    {
        if( !core_get_numeric_arg(env, expr, "/", &arg, useFloatTotal, pos))
        {
            expr = NULL;
        }
        else
        {
            expr = core_get_next_arg(expr);
        }

        if( arg.type == INTEGER )
        {
            ltotal = to_long(arg.value);
        }
        else
        {
            ftotal = to_double(arg.value);
            useFloatTotal = TRUE;
        }

        pos++;
    }

    /*====================================================
     * Loop through each of the arguments dividing it
     * into a running product. If a floating point number
     * is encountered, then do all subsequent operations
     * using floating point values. Each argument is
     * checked to prevent a divide by zero error.
     *====================================================*/

    while( expr != NULL )
    {
        if( !core_get_numeric_arg(env, expr, "/", &arg, useFloatTotal, pos))
        {
            expr = NULL;
        }
        else
        {
            expr = core_get_next_arg(expr);
        }

        if((arg.type == INTEGER) ? (to_long(arg.value) == 0L) :
           ((arg.type == FLOAT) ? to_double(arg.value) == 0.0 : FALSE))
        {
            error_divide_by_zero(env, "/");
            core_set_halt_eval(env, TRUE);
            core_set_eval_error(env, TRUE);
            ret->type = FLOAT;
            ret->value = (void *)store_double(env, 1.0);
            return;
        }

        if( useFloatTotal )
        {
            ftotal /= to_double(arg.value);
        }
        else
        {
            if( arg.type == INTEGER )
            {
                ltotal /= to_long(arg.value);
            }
            else
            {
                ftotal = (double)ltotal / to_double(arg.value);
                useFloatTotal = TRUE;
            }
        }

        pos++;
    }

    if( (pos - 1) == 1 )
    {
        ftotal = 1.0f / ftotal;
        ltotal = 1 / ltotal;
    }

    /*======================================================
     * If a floating point number was in the argument list,
     * then return a float, otherwise return an integer.
     *======================================================*/

    if( useFloatTotal )
    {
        ret->type = FLOAT;
        ret->value = (void *)store_double(env, ftotal);
    }
    else
    {
        ret->type = INTEGER;
        ret->value = (void *)store_long(env, ltotal);
    }
}

/****************************************
 * broccoli_convert_to_integer: H/L access routine
 *   for the integer function.
 *****************************************/
long long broccoli_convert_to_integer(void *env)
{
    core_data_object valstruct;

    /*============================================
     * Check for the correct number of arguments.
     *============================================*/

    if( core_check_arg_count(env, "int", EXACTLY, 1) == -1 )
    {
        return(0LL);
    }

    /*================================================================
     * Check for the correct type of argument. Note that ArgTypeCheck
     * will convert floats to integers when an integer is requested
     * (which is the purpose of the integer function).
     *================================================================*/

    if( core_check_arg_type(env, "int", 1, INTEGER, &valstruct) == FALSE )
    {
        return(0LL);
    }

    /*===================================================
     * Return the numeric value converted to an integer.
     *===================================================*/

    return(to_long(valstruct.value));
}

/**************************************
 * broccoli_covert_to_float: H/L access routine
 *   for the float function.
 ***************************************/
double broccoli_covert_to_float(void *env)
{
    core_data_object valstruct;

    /*============================================
     * Check for the correct number of arguments.
     *============================================*/

    if( core_check_arg_count(env, FUNC_NAME_CAST_FLOAT, EXACTLY, 1) == -1 )
    {
        return(0.0);
    }

    /*================================================================
     * Check for the correct type of argument. Note that ArgTypeCheck
     * will convert integers to floats when a float is requested
     * (which is the purpose of the float function).
     *================================================================*/

    if( core_check_arg_type(env, FUNC_NAME_CAST_FLOAT, 1, FLOAT, &valstruct) == FALSE )
    {
        return(0.0);
    }

    /*================================================
     * Return the numeric value converted to a float.
     *================================================*/

    return(to_double(valstruct.value));
}
