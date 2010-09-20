#define _METAFUN_SOURCE_

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
#include "router_string.h"
#include "parser_flow_control.h"
#include "core_command_prompt.h"
#include "funcs_function.h"

#include "funcs_meta.h"

#if META_SYSTEM

static int _eval(void *, char *, core_data_object_ptr);

void init_meta_functions(void *env)
{
    core_define_function(env, "help", RT_LIST, PTR_FN broccoli_help, "broccoli_help", "00");
    core_define_function(env, "eval", RT_UNKNOWN, PTR_FN broccoli_eval, "broccoli_eval", "11k");
    core_define_function(env, "call", RT_UNKNOWN, PTR_FN broccoli_call, "broccoli_call", "1**k");
}

/**************************************
 * broccoli_call: H/L access routine
 *   for the funcall function.
 ***************************************/
void broccoli_call(void *env, core_data_object *ret)
{
    int argCount, i, j;
    core_data_object val;
    FUNCTION_REFERENCE ref;
    char *name;
    struct list *list;
    struct core_expression *lastAdd = NULL, *nextAdd, *multiAdd;
    struct core_function_definition *func;

    /*==================================
     * Set up the default return value.
     *==================================*/

    core_set_pointer_type(ret, ATOM);
    core_set_pointer_value(ret, get_false(env));

    /*=================================================
     * The funcall function has at least one argument:
     * the name of the function being called.
     *=================================================*/

    if((argCount = core_check_arg_count(env, "funcall", AT_LEAST, 1)) == -1 )
    {
        return;
    }

    /*============================================
     * Get the name of the function to be called.
     *============================================*/

    if( core_check_arg_type(env, "funcall", 1, ATOM_OR_STRING, &val) == FALSE )
    {
        return;
    }

    /*====================
     * Find the function.
     *====================*/

    name = core_convert_data_to_string(val);

    if( !core_get_function_referrence(env, name, &ref))
    {
        report_explicit_type_error(env, "funcall", 1, "function, deffunction, or generic function name");
        return;
    }

    /*====================================
     * Functions with specialized parsers
     * cannot be used with funcall.
     *====================================*/

    if( ref.type == FCALL )
    {
        func = core_lookup_function(env, name);

        if( func->parser != NULL )
        {
            report_explicit_type_error(env, "funcall", 1, "function without specialized parser");
            return;
        }
    }

    /*======================================
     * Add the arguments to the expression.
     *======================================*/

    core_increment_expression(env, &ref);

    for( i = 2; i <= argCount; i++ )
    {
        core_get_arg_at(env, i, &val);

        if( core_get_eval_error(env))
        {
            core_decrement_expression(env, &ref);
            return;
        }

        switch( core_get_type(val))
        {
        case LIST:
            nextAdd = core_generate_constant(env, FCALL, (void *)core_lookup_function(env, FUNC_NAME_CREATE_LIST));

            if( lastAdd == NULL )
            {
                ref.args = nextAdd;
            }
            else
            {
                lastAdd->next_arg = nextAdd;
            }

            lastAdd = nextAdd;

            multiAdd = NULL;
            list = (struct list *)core_get_value(val);

            for( j = core_get_data_start(val); j <= core_get_data_end(val); j++ )
            {
                nextAdd = core_generate_constant(env, get_list_node_type(list, j), get_list_node_value(list, j));

                if( multiAdd == NULL )
                {
                    lastAdd->args = nextAdd;
                }
                else
                {
                    multiAdd->next_arg = nextAdd;
                }

                multiAdd = nextAdd;
            }

            core_increment_expression(env, lastAdd);
            break;

        default:
            nextAdd = core_generate_constant(env, core_get_type(val), core_get_value(val));

            if( lastAdd == NULL )
            {
                ref.args = nextAdd;
            }
            else
            {
                lastAdd->next_arg = nextAdd;
            }

            lastAdd = nextAdd;
            core_increment_expression(env, lastAdd);
            break;
        }
    }

    /*===========================================================
     * Verify a deffunction has the correct number of arguments.
     *===========================================================*/

#if DEFFUNCTION_CONSTRUCT

    if( ref.type == PCALL )
    {
        if( verify_function_call(env, ref.value, core_count_args(ref.args)) == FALSE )
        {
            error_print_id(env, "META", 4, FALSE);
            print_router(env, WERROR, "Function funcall called with the wrong number of arguments for deffunction ");
            print_router(env, WERROR, get_function_name(env, ref.value));
            print_router(env, WERROR, "\n");
            core_decrement_expression(env, &ref);
            core_return_expression(env, ref.args);
            return;
        }
    }

#endif

    /*======================
     * Call the expression.
     *======================*/

    core_eval_expression(env, &ref, ret);

    /*========================================
     * Return the expression data structures.
     *========================================*/

    core_decrement_expression(env, &ref);
    core_return_expression(env, ref.args);
}

/************************************************
 * broccoli_help: H/L access routine
 *   for the get-function-list function.
 *************************************************/
void broccoli_help(void *env, core_data_object *ret)
{
    struct core_function_definition *func;
    struct list *list;
    unsigned long functionCount = 0;

    if( core_check_arg_count(env, "help", EXACTLY, 0) == -1 )
    {
        core_create_error_list(env, ret);
        return;
    }

    /***
     * Count only the environment available functions.  I.E. non-parenthesized
     * internal functions.
     **/
    for( func = core_get_function_list(env); func != NULL; func = func->next )
    {
        if( func->function_handle->contents[0] != '(' )
        {
            functionCount++;
        }
    }

    core_set_pointer_type(ret, LIST);
    core_set_data_ptr_start(ret, 1);
    core_set_data_ptr_end(ret, functionCount);
    list = (struct list *)create_list(env, functionCount);
    core_set_pointer_value(ret, (void *)list);

    /***
     * Only return the non-internal functions
     **/
    for( func = core_get_function_list(env), functionCount = 1; func != NULL; func = func->next, functionCount++ )
    {
        if( func->function_handle->contents[0] != '(' )
        {
            set_list_node_type(list, functionCount, ATOM);
            set_list_node_value(list, functionCount, func->function_handle);
        }
        else
        {
            functionCount--;
        }
    }
}

/*************************************
 * broccoli_eval: H/L access routine
 *   for the eval function.
 **************************************/
void broccoli_eval(void *env, core_data_object_ptr ret)
{
    core_data_object theArg;

    /*=============================================
     * Function eval expects exactly one argument.
     *=============================================*/

    if( core_check_arg_count(env, "eval", EXACTLY, 1) == -1 )
    {
        core_set_pointer_type(ret, ATOM);
        core_set_pointer_value(ret, get_false(env));
        return;
    }

    /*==================================================
     * The argument should be of type ATOM or STRING.
     *==================================================*/

    if( core_check_arg_type(env, "eval", 1, ATOM_OR_STRING, &theArg) == FALSE )
    {
        core_set_pointer_type(ret, ATOM);
        core_set_pointer_value(ret, get_false(env));
        return;
    }

    /*======================
     * Evaluate the string.
     *======================*/

    _eval(env, core_convert_data_to_string(theArg), ret);
}

/****************************
 * EnvEval: C access routine
 *   for the eval function.
 *****************************/
int _eval(void *env, char *theString, core_data_object_ptr ret)
{
    struct core_expression *top;
    int ov;
    static int depth = 0;
    char logicalNameBuffer[20];
    struct binding *oldBinds;

    /*======================================================
     * Evaluate the string. Create a different logical name
     * for use each time the eval function is called.
     *======================================================*/

    depth++;
    sysdep_sprintf(logicalNameBuffer, "Eval-%d", depth);

    if( open_string_source(env, logicalNameBuffer, theString, 0) == 0 )
    {
        core_set_pointer_type(ret, ATOM);
        core_set_pointer_value(ret, get_false(env));
        depth--;
        return(FALSE);
    }

    /*================================================
     * Save the current parsing state before routines
     * are called to parse the eval string.
     *================================================*/

    ov = core_get_pp_buffer_status(env);
    core_set_pp_buffer_status(env, FALSE);
    oldBinds = get_parsed_bindings(env);
    set_parsed_bindings(env, NULL);

    /*========================================================
     * Parse the string argument passed to the eval function.
     *========================================================*/

    top = parse_atom_or_expression(env, logicalNameBuffer, NULL);

    /*============================
     * Restore the parsing state.
     *============================*/

    core_set_pp_buffer_status(env, ov);
    clear_parsed_bindings(env);
    set_parsed_bindings(env, oldBinds);

    /*===========================================
     * Return if an error occured while parsing.
     *===========================================*/

    if( top == NULL )
    {
        core_set_eval_error(env, TRUE);
        close_string_source(env, logicalNameBuffer);
        core_set_pointer_type(ret, ATOM);
        core_set_pointer_value(ret, get_false(env));
        depth--;
        return(FALSE);
    }

    /*==============================================
     * The sequence expansion operator must be used
     * within the argument list of a function call.
     *==============================================*/

    if((top->type == LIST_VARIABLE))
    {
        error_print_id(env, "META", 1, FALSE);
        print_router(env, WERROR, "expand$ must be used in the argument list of a function call.\n");
        core_set_eval_error(env, TRUE);
        close_string_source(env, logicalNameBuffer);
        core_set_pointer_type(ret, ATOM);
        core_set_pointer_value(ret, get_false(env));
        core_return_expression(env, top);
        depth--;
        return(FALSE);
    }

    /*=======================================
     * The expression to be evaluated cannot
     * contain any local variables.
     *=======================================*/

    if( core_does_expression_have_variables(top, FALSE))
    {
        error_print_id(env, "STRINGS", 2, FALSE);
        print_router(env, WERROR, "Some variables could not be accessed by the eval function.\n");
        core_set_eval_error(env, TRUE);
        close_string_source(env, logicalNameBuffer);
        core_set_pointer_type(ret, ATOM);
        core_set_pointer_value(ret, get_false(env));
        core_return_expression(env, top);
        depth--;
        return(FALSE);
    }

    /*====================================
     * Evaluate the expression and return
     * the memory used to parse it.
     *====================================*/

    core_increment_expression(env, top);
    core_eval_expression(env, top, ret);
    core_decrement_expression(env, top);

    depth--;
    core_return_expression(env, top);
    close_string_source(env, logicalNameBuffer);

    /*==========================================
     * Perform periodic cleanup if the eval was
     * issued from an embedded controller.
     *==========================================*/

    if((core_get_evaluation_data(env)->eval_depth == 0) && (!CommandLineData(env)->EvaluatingTopLevelCommand) &&
       (core_get_evaluation_data(env)->current_expression == NULL))
    {
        core_value_increment(env, ret);
        core_gc_periodic_cleanup(env, TRUE, FALSE);
        core_value_decrement(env, ret);
    }

    if( core_get_eval_error(env))
    {
        return(FALSE);
    }

    return(TRUE);
}

#endif
