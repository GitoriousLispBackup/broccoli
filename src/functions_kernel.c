/* Purpose: Deffunction Execution Routines                   */

/* =========================================
 *****************************************
 *            EXTERNAL DEFINITIONS
 *  =========================================
 ***************************************** */
#include "setup.h"

#if DEFFUNCTION_CONSTRUCT

#ifndef _STDIO_INCLUDED_
#define _STDIO_INCLUDED_
#include <stdio.h>
#endif

#include "core_constructs.h"
#include "core_environment.h"
#include "funcs_function.h"
#include "core_functions_util.h"
#include "funcs_profiling.h"
#include "router.h"
#include "core_gc.h"
#include "core_watch.h"
#include "funcs_flow_control.h"

#define __FUNCTIONS_KERNEL_SOURCE__
#include "functions_kernel.h"

/* =========================================
 *****************************************
 *                CONSTANTS
 *  =========================================
 ***************************************** */
#define BEGIN_TRACE ">> "
#define END_TRACE   "<< "

/* =========================================
 *****************************************
 *   INTERNALLY VISIBLE FUNCTION HEADERS
 *  =========================================
 ***************************************** */

static void _error_unknown_function(void *);

#if DEBUGGING_FUNCTIONS
static void WatchDeffunction(void *, char *);
#endif

/* =========================================
 *****************************************
 *       EXTERNALLY VISIBLE FUNCTIONS
 *  =========================================
 ***************************************** */

/****************************************************
 *  NAME         : function_execute
 *  DESCRIPTION  : Executes the body of a deffunction
 *  INPUTS       : 1) The deffunction
 *              2) Argument expressions
 *              3) Data object buffer to hold result
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Deffunction executed and result
 *              stored in data object buffer
 *  NOTES        : Used in core_eval_expression(env,)
 ****************************************************/
void function_execute(void *env, FUNCTION_DEFINITION *dptr, core_expression_object *args, core_data_object *result)
{
    int oldce;
    FUNCTION_DEFINITION *previouslyExecutingDeffunction;

#if PROFILING_FUNCTIONS
    struct profileFrameInfo profileFrame;
#endif

    result->type = ATOM;
    result->value = get_false(env);
    core_get_evaluation_data(env)->eval_error = FALSE;

    if( core_get_evaluation_data(env)->halt )
    {
        return;
    }

    oldce = core_is_construct_executing(env);
    core_set_construct_executing(env, TRUE);
    previouslyExecutingDeffunction = get_function_data(env)->executing_function;
    get_function_data(env)->executing_function = dptr;
    core_get_evaluation_data(env)->eval_depth++;
    dptr->executing++;
    core_push_function_args(env, args, core_count_args(args), get_function_name(env, (void *)dptr),
                            FUNC_NAME_CREATE_FUNC, _error_unknown_function);

    if( core_get_evaluation_data(env)->eval_error )
    {
        dptr->executing--;
        get_function_data(env)->executing_function = previouslyExecutingDeffunction;
        core_get_evaluation_data(env)->eval_depth--;
        core_gc_periodic_cleanup(env, FALSE, TRUE);
        core_set_construct_executing(env, oldce);
        return;
    }

#if DEBUGGING_FUNCTIONS

    if( dptr->trace )
    {
        WatchDeffunction(env, BEGIN_TRACE);
    }

#endif

#if PROFILING_FUNCTIONS
    StartProfile(env, &profileFrame,
                 &dptr->header.ext_data,
                 ProfileFunctionData(env)->ProfileConstructs);
#endif

    core_eval_function_actions(env, dptr->header.my_module->module_def,
                               dptr->code, dptr->local_variable_count,
                               result, _error_unknown_function);

#if PROFILING_FUNCTIONS
    EndProfile(env, &profileFrame);
#endif

#if DEBUGGING_FUNCTIONS

    if( dptr->trace )
    {
        WatchDeffunction(env, END_TRACE);
    }

#endif
    get_flow_control_data(env)->return_flag = FALSE;

    dptr->executing--;
    core_pop_function_args(env);
    get_function_data(env)->executing_function = previouslyExecutingDeffunction;
    core_get_evaluation_data(env)->eval_depth--;
    core_pass_return_value(env, result);
    core_gc_periodic_cleanup(env, FALSE, TRUE);
    core_set_construct_executing(env, oldce);
}

/* =========================================
 *****************************************
 *       INTERNALLY VISIBLE FUNCTIONS
 *  =========================================
 ***************************************** */

/*******************************************************
 *  NAME         : UnboundDeffunctionErr
 *  DESCRIPTION  : Print out a synopis of the currently
 *                executing deffunction for unbound
 *                variable errors
 *  INPUTS       : None
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Error synopsis printed to WERROR
 *  NOTES        : None
 *******************************************************/
static void _error_unknown_function(void *env)
{
    print_router(env, WERROR, "deffunction ");
    print_router(env, WERROR, get_function_name(env, (void *)get_function_data(env)->executing_function));
    print_router(env, WERROR, ".\n");
}

#if DEBUGGING_FUNCTIONS

/***************************************************
 *  NAME         : WatchDeffunction
 *  DESCRIPTION  : Displays a message indicating when
 *              a deffunction began and ended
 *              execution
 *  INPUTS       : The beginning or end trace string
 *              to print when deffunction starts
 *              or finishes respectively
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Watch message printed
 *  NOTES        : None
 ***************************************************/
static void WatchDeffunction(void *env, char *tstring)
{
    print_router(env, WTRACE, "DFN ");
    print_router(env, WTRACE, tstring);

    if( get_function_data(env)->executing_function->header.my_module->module_def != ((struct module_definition *)get_current_module(env)))
    {
        print_router(env, WTRACE, get_module_name(env, (void *)
                                                  get_function_data(env)->executing_function->header.my_module->module_def));
        print_router(env, WTRACE, "::");
    }

    print_router(env, WTRACE, to_string(get_function_data(env)->executing_function->header.name));
    print_router(env, WTRACE, " ED:");
    core_print_long(env, WTRACE, (long long)core_get_evaluation_data(env)->eval_depth);
    core_print_function_args(env, WTRACE);
}

#endif
#endif
