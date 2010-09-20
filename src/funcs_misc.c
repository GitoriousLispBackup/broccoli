#define _META_SOURCE_

#include <stdio.h>
#define _STDIO_INCLUDED_
#include <string.h>

#include "setup.h"

#include "core_arguments.h"
#include "core_environment.h"
#include "parser_expressions.h"
#include "core_memory.h"
#include "type_list.h"
#include "router.h"
#include "sysdep.h"
#include "core_gc.h"

#if DEFFUNCTION_CONSTRUCT
#include "funcs_function.h"
#endif

#include "funcs_misc.h"

#define MISC_FUNCTION_DATA_INDEX 9

struct misc_function_data
{
    long long sequence_number;
};

#define get_misc_function_data(env) ((struct misc_function_data *)core_get_environment_data(env, MISC_FUNCTION_DATA_INDEX))

/**************************************
 * LOCAL FUNCTION PROTOTYPES
 ***************************************/
static void _expand_function_list(void *, core_data_object *, core_expression_object *, core_expression_object **, void *);
static void _dummy_expand(void *, core_data_object *);


/****************************************************************
 * init_misc_functions: Initializes miscellaneous functions.
 *****************************************************************/
void init_misc_functions(void *env)
{
    core_allocate_environment_data(env, MISC_FUNCTION_DATA_INDEX, sizeof(struct misc_function_data), NULL);
    get_misc_function_data(env)->sequence_number = 1;

    core_define_function(env, "bench", 'd', PTR_FN broccoli_bench, "TimerFunction", "**");
    core_define_function(env, FUNC_NAME_EXPAND_META, RT_UNKNOWN, PTR_FN broccoli_expand, "ExpandFuncCall", FUNC_CNSTR_EXPAND_META);
    core_define_function(env, FUNC_NAME_EXPAND, RT_UNKNOWN, PTR_FN _dummy_expand, "DummyExpandFuncList", FUNC_CNSTR_EXPAND);
    core_set_function_overload(env, FUNC_NAME_EXPAND, FALSE, FALSE);
}

/********************************************************************
 * NAME         : broccoli_expand
 * DESCRIPTION  : This function is a wrap-around for a normal
 *               function call.  It preexamines the argument
 *               expression list and expands any references to the
 *               sequence operator.  It builds a copy of the
 *               function call expression with these new arguments
 *               inserted and evaluates the function call.
 * INPUTS       : A data object buffer
 * RETURNS      : Nothing useful
 * SIDE EFFECTS : Expressions alloctaed/deallocated
 *             Function called and arguments evaluated
 *             eval_error set on errors
 * NOTES        : None
 *******************************************************************/
void broccoli_expand(void *env, core_data_object *result)
{
    core_expression_object *newargexp, *fcallexp;
    struct core_function_definition *func;

    /* ======================================================================
     *  Copy the original function call's argument expression list.
     *  Look for expand$ function callsexpressions and replace those
     *   with the equivalent expressions of the expansions of evaluations
     *   of the arguments.
     *  ====================================================================== */
    newargexp = core_copy_expression(env, core_get_first_arg()->args);
    _expand_function_list(env, result, newargexp, &newargexp,
                          (void *)core_lookup_function(env, FUNC_NAME_EXPAND));

    /* ===================================================================
     *  Build the new function call expression with the expanded arguments.
     *  Check the number of arguments, if necessary, and call the thing.
     *  =================================================================== */
    fcallexp = core_mem_get_struct(env, core_expression);
    fcallexp->type = core_get_first_arg()->type;
    fcallexp->value = core_get_first_arg()->value;
    fcallexp->next_arg = NULL;
    fcallexp->args = newargexp;

    if( fcallexp->type == FCALL )
    {
        func = (struct core_function_definition *)fcallexp->value;

        if( core_check_function_arg_count(env, to_string(func->function_handle),
                                          func->restrictions, core_count_args(newargexp)) == FALSE )
        {
            result->type = ATOM;
            result->value = get_false(env);
            core_return_expression(env, fcallexp);
            return;
        }
    }

#if DEFFUNCTION_CONSTRUCT
    else if( fcallexp->type == PCALL )
    {
        if( verify_function_call(env, fcallexp->value,
                                 core_count_args(fcallexp->args)) == FALSE )
        {
            result->type = ATOM;
            result->value = get_false(env);
            core_return_expression(env, fcallexp);
            core_set_eval_error(env, TRUE);
            return;
        }
    }
#endif

    core_eval_expression(env, fcallexp, result);
    core_return_expression(env, fcallexp);
}

/***********************************************************************
 * NAME         : DummyExpandFuncList
 * DESCRIPTION  : The expansion of list arguments is valid only
 *             when done for a function call.  All these expansions
 *             are handled by the H/L wrap-around function
 *             (expansion-call) - see broccoli_expand.  If the H/L
 *             function, epand-list is ever called directly,
 *             it is an error.
 * INPUTS       : Data object buffer
 * RETURNS      : Nothing useful
 * SIDE EFFECTS : eval_error set
 * NOTES        : None
 **********************************************************************/
void _dummy_expand(void *env, core_data_object *result)
{
    result->type = ATOM;
    result->value = get_false(env);
    core_set_eval_error(env, TRUE);
    error_print_id(env, "META", 1, FALSE);
    print_router(env, WERROR, "expand$ must be used in the argument list of a function call.\n");
}

/***********************************************************************
 * NAME         : ExpandFuncList
 * DESCRIPTION  : Recursively examines an expression and replaces
 *               PROC_EXPAND_LIST expressions with the expanded
 *               evaluation expression of its argument
 * INPUTS       : 1) A data object result buffer
 *             2) The expression to modify
 *             3) The address of the expression, in case it is
 *                deleted entirely
 *             4) The address of the H/L function expand$
 * RETURNS      : Nothing useful
 * SIDE EFFECTS : Expressions allocated/deallocated as necessary
 *             Evaluations performed
 *             On errors, argument expression set to call a function
 *               which causes an evaluation error when evaluated
 *               a second time by actual caller.
 * NOTES        : THIS ROUTINE MODIFIES EXPRESSIONS AT RUNTIME!!  MAKE
 *             SURE THAT THE core_expression_object PASSED IS SAFE TO CHANGE!!
 **********************************************************************/
static void _expand_function_list(void *env, core_data_object *result, core_expression_object *theExp, core_expression_object **sto, void *expmult)
{
    core_expression_object *newexp, *top, *bot;
    register long i; /* 6.04 Bug Fix */

    while( theExp != NULL )
    {
        if( theExp->value == expmult )
        {
            core_eval_expression(env, theExp->args, result);
            core_return_expression(env, theExp->args);

            if((core_get_evaluation_data(env)->eval_error) || (result->type != LIST))
            {
                theExp->args = NULL;

                if((core_get_evaluation_data(env)->eval_error == FALSE) && (result->type != LIST))
                {
                    report_implicit_type_error(env, FUNC_NAME_EXPAND, 1);
                }

                theExp->value = (void *)core_lookup_function(env, "(set-evaluation-error)");
                core_get_evaluation_data(env)->eval_error = FALSE;
                core_get_evaluation_data(env)->halt = FALSE;
                return;
            }

            top = bot = NULL;

            for( i = core_get_data_ptr_start(result) ; i <= core_get_data_ptr_end(result) ; i++ )
            {
                newexp = core_mem_get_struct(env, core_expression);
                newexp->type = get_list_node_type(result->value, i);
                newexp->value = get_list_node_value(result->value, i);
                newexp->args = NULL;
                newexp->next_arg = NULL;

                if( top == NULL )
                {
                    top = newexp;
                }
                else
                {
                    bot->next_arg = newexp;
                }

                bot = newexp;
            }

            if( top == NULL )
            {
                *sto = theExp->next_arg;
                core_mem_return_struct(env, core_expression, theExp);
                theExp = *sto;
            }
            else
            {
                bot->next_arg = theExp->next_arg;
                *sto = top;
                core_mem_return_struct(env, core_expression, theExp);
                sto = &bot->next_arg;
                theExp = bot->next_arg;
            }
        }
        else
        {
            if( theExp->args != NULL )
            {
                _expand_function_list(env, result, theExp->args, &theExp->args, expmult);
            }

            sto = &theExp->next_arg;
            theExp = theExp->next_arg;
        }
    }
}

double broccoli_bench(void *env)
{
    int numa, i;
    double startTime;
    core_data_object ret;

    startTime = sysdep_time();

    numa = core_get_arg_count(env);

    i = 1;

    while((i <= numa) && (core_get_halt_eval(env) != TRUE))
    {
        core_get_arg_at(env, i, &ret);
        i++;
    }

    return(sysdep_time() - startTime);
}
