/* Purpose: Provides utility routines for manipulating and
 *   examining expressions.                                  */

#define __CORE_EXPRESSIONS_OPERATORS_SOURCE__

#include "setup.h"

#include <stdio.h>
#define _STDIO_INCLUDED_
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "core_memory.h"
#include "core_environment.h"
#include "router.h"
#include "core_functions.h"
#include "constraints_query.h"
#include "core_utilities.h"
#include "constraints_operators.h"
#include "constraints_operators.h"

#include "core_expressions_operators.h"

/*************************************************************
 * core_check_against_restrictions: Compares an argument to a
 *   function to the set of restrictions for that function to
 *   determine if any incompatibilities exist. If so, the
 *   value TRUE is returned, otherwise FALSE is returned.
 *   Restrictions checked are:
 *     a - external address
 *     d - float
 *     e - instance address, instance name, or symbol
 *     f - float
 *     g - integer, float, or symbol
 *     h - instance address, instance name, fact address,
 *         integer, or symbol
 *     i - integer
 *     j - symbol, string, or instance name
 *     k - symbol or string
 *     l - integer
 *     m - list
 *     n - float or integer
 *     o - instance name
 *     p - instance name or symbol
 *     q - string, symbol, or list
 *     s - string
 *     u - unknown (any type allowed)
 *     w - symbol
 *     x - instance address
 *     y - fact address
 *     z - fact address, integer, or symbol (*)
 **************************************************************/
int core_check_against_restrictions(void *env, struct core_expression *expr, int theRestriction)
{
    CONSTRAINT_META *cr1, *cr2, *cr3;

    /*=============================================
     * Generate a constraint record for the actual
     * argument passed to the function.
     *=============================================*/

    cr1 = convert_expression_to_constraint(env, expr);

    /*================================================
     * Generate a constraint record based on the type
     * of argument expected by the function.
     *================================================*/

    cr2 = convert_arg_type_to_constraint(env, theRestriction);

    /*===============================================
     * Intersect the two constraint records and then
     * discard them.
     *===============================================*/

    cr3 = constraint_intersection(env, cr1, cr2);

    remove_constraint(env, cr1);
    remove_constraint(env, cr2);

    /*====================================================
     * If the intersection of the two constraint records
     * is empty, then the argument passed to the function
     * doesn't satisfy the restrictions for the argument.
     *====================================================*/

    if( is_satisfiable(cr3))
    {
        remove_constraint(env, cr3);
        return(TRUE);
    }

    /*===================================================
     * The argument satisfies the function restrictions.
     *===================================================*/

    remove_constraint(env, cr3);
    return(FALSE);
}

/***********************************************
 * core_is_constant: Returns TRUE if the type
 *   is a constant, otherwise FALSE.
 ************************************************/
BOOLEAN core_is_constant(int theType)
{
    switch( theType )
    {
    case ATOM:
    case STRING:
    case INTEGER:
    case FLOAT:
#if OBJECT_SYSTEM
    case INSTANCE_NAME:
    case INSTANCE_ADDRESS:
#endif
        return(TRUE);
    }

    return(FALSE);
}

/****************************************************************************
 * core_are_expressions_equivalent: Determines if two expressions are identical. Returns
 *   TRUE if the expressions are identical, otherwise FALSE is returned.
 *****************************************************************************/
BOOLEAN core_are_expressions_equivalent(struct core_expression *firstList, struct core_expression *secondList)
{
    /*==============================================
     * Compare each argument in both expressions by
     * following the next_arg list.
     *==============================================*/

    for(;
        (firstList != NULL) && (secondList != NULL);
        firstList = firstList->next_arg, secondList = secondList->next_arg )
    {
        /*=========================
         * Compare type and value.
         *=========================*/

        if( firstList->type != secondList->type )
        {
            return(FALSE);
        }

        if( firstList->value != secondList->value )
        {
            return(FALSE);
        }

        /*==============================
         * Compare the arguments lists.
         *==============================*/

        if( core_are_expressions_equivalent(firstList->args, secondList->args) == FALSE )
        {
            return(FALSE);
        }
    }

    /*=====================================================
     * If firstList and secondList aren't both NULL, then
     * one of the lists contains more expressions than the
     * other.
     *=====================================================*/

    if( firstList != secondList )
    {
        return(FALSE);
    }

    /*============================
     * Expressions are identical.
     *============================*/

    return(TRUE);
}

/***************************************************
 * core_count_args: Returns the number of structures
 *   stored in an expression as traversed through
 *   the next_arg pointer but not the args
 *   pointer.
 ****************************************************/
int core_count_args(struct core_expression *testPtr)
{
    int size = 0;

    while( testPtr != NULL )
    {
        size++;
        testPtr = testPtr->next_arg;
    }

    return(size);
}

/*****************************************
 * CopyExpresssion: Copies an expression.
 ******************************************/
struct core_expression *core_copy_expression(void *env, struct core_expression *original)
{
    struct core_expression *topLevel, *next, *last;

    if( original == NULL ) {return(NULL);}

    topLevel = core_generate_constant(env, original->type, original->value);
    topLevel->args = core_copy_expression(env, original->args);

    last = topLevel;
    original = original->next_arg;

    while( original != NULL )
    {
        next = core_generate_constant(env, original->type, original->value);
        next->args = core_copy_expression(env, original->args);

        last->next_arg = next;
        last = next;
        original = original->next_arg;
    }

    return(topLevel);
}

/***********************************************************
 * core_does_expression_have_variables: Determines if an expression
 *   contains any variables. Returns TRUE if the expression
 *   contains any variables, otherwise FALSE is returned.
 ************************************************************/
BOOLEAN core_does_expression_have_variables(struct core_expression *expr, BOOLEAN globalsAreVariables)
{
    while( expr != NULL )
    {
        if( expr->args != NULL )
        {
            if( core_does_expression_have_variables(expr->args, globalsAreVariables))
            {
                return(TRUE);
            }
        }

        if(((expr->type == LIST_VARIABLE) || (expr->type == SCALAR_VARIABLE)) && (globalsAreVariables == TRUE))
        {
            return(TRUE);
        }

        expr = expr->next_arg;
    }

    return(FALSE);
}

/****************************************
 * core_calculate_expression_size: Returns the number of
 *   structures stored in an expression.
 *****************************************/
long core_calculate_expression_size(struct core_expression *testPtr)
{
    long size = 0;

    while( testPtr != NULL )
    {
        size++;

        if( testPtr->args != NULL )
        {
            size += core_calculate_expression_size(testPtr->args);
        }

        testPtr = testPtr->next_arg;
    }

    return(size);
}

/***********************************************
 * core_generate_constant: Generates a constant expression
 *   value of type string, symbol, or number.
 ************************************************/
struct core_expression *core_generate_constant(void *env, unsigned short type, void *value)
{
    struct core_expression *top;

    top = core_mem_get_struct(env, core_expression);
    top->next_arg = NULL;
    top->args = NULL;
    top->type = type;
    top->value = value;

    return(top);
}

/************************************************
 * core_print_expression: Pretty prints an expression.
 *************************************************/
void core_print_expression(void *env, char *fileid, struct core_expression *expr)
{
    struct core_expression *oldExpression;

    if( expr == NULL )
    {
        return;
    }

    while( expr != NULL )
    {
        switch( expr->type )
        {
        case SCALAR_VARIABLE:
            print_router(env, fileid, "$");
            print_router(env, fileid, to_string(expr->value));
            break;

        case LIST_VARIABLE:
            print_router(env, fileid, "@");
            print_router(env, fileid, to_string(expr->value));
            break;

        case FCALL:
            print_router(env, fileid, "(");
            print_router(env, fileid, to_string(core_get_expression_function_handle(expr)));

            if( expr->args != NULL )
            {
                print_router(env, fileid, " ");
            }

            core_print_expression(env, fileid, expr->args);
            print_router(env, fileid, ")");
            break;

        default:
            oldExpression = core_get_evaluation_data(env)->current_expression;
            core_get_evaluation_data(env)->current_expression = expr;
            core_print_atom(env, fileid, expr->type, expr->value);
            core_get_evaluation_data(env)->current_expression = oldExpression;
            break;
        }

        expr = expr->next_arg;

        if( expr != NULL )
        {
            print_router(env, fileid, " ");
        }
    }

    return;
}
