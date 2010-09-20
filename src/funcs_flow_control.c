/* Purpose: Contains the code for several procedural
 *   functions including if, while, loop-for-count, bind,
 *   progn, return, break, and switch                        */

#define __FLOW_CONTROL_SOURCE__

#include <stdio.h>
#define _STDIO_INCLUDED_

#include "setup.h"

#include "core_arguments.h"
#include "constraints_kernel.h"
#include "constraints_query.h"
#include "constraints_operators.h"
#include "core_environment.h"
#include "parser_expressions.h"
#include "core_memory.h"
#include "type_list.h"
#include "parser_flow_control.h"
#include "router.h"
#include "core_scanner.h"
#include "core_gc.h"

#include "funcs_flow_control.h"

/**************************************
 * LOCAL FUNCTION PROTOTYPES
 ***************************************/

static void _delete_flow_control_data(void *);

/*********************************************
 * func_init_flow_control: Initializes
 *   the procedural functions.
 **********************************************/
void func_init_flow_control(void *env)
{
    core_allocate_environment_data(env, FLOW_CONTROL_DATA_INDEX, sizeof(struct flow_control_data), _delete_flow_control_data);

    core_define_function(env, FUNC_NAME_ASSIGNMENT, RT_UNKNOWN, PTR_FN broccoli_bind, "broccoli_bind", FUNC_CNSTR_ASSIGNMENT);
    core_define_function(env, "if", RT_UNKNOWN, PTR_FN broccoli_if, "broccoli_if", NULL);
    core_define_function(env, FUNC_NAME_PROGN, RT_UNKNOWN, PTR_FN broccoli_progn, "broccoli_progn", FUNC_CNSTR_PROGN);
    core_define_function(env, "(get-loop-count)", 'g', PTR_FN broccoli_get_loop_count, "broccoli_get_loop_count", NULL);
    init_flow_control_parsers(env);
    core_set_function_overload(env, FUNC_NAME_PROGN, FALSE, FALSE);
    core_set_function_overload(env, "if", FALSE, FALSE);

    core_add_reset_listener(env, FUNC_NAME_ASSIGNMENT, flush_bindings, 0);
    core_add_clear_fn(env, FUNC_NAME_ASSIGNMENT, flush_bindings, 0);
}

/************************************************************
 * DeallocateProceduralFunctionData: Deallocates environment
 *    data for procedural functions.
 *************************************************************/
static void _delete_flow_control_data(void *env)
{
    core_data_object_ptr nextPtr, garbagePtr;

    garbagePtr = get_flow_control_data(env)->variables;

    while( garbagePtr != NULL )
    {
        nextPtr = garbagePtr->next;
        core_mem_return_struct(env, core_data, garbagePtr);
        garbagePtr = nextPtr;
    }
}

/*************************************
 * broccoli_bind: H/L access routine
 *   for the bind function.
 **************************************/
void broccoli_bind(void *env, core_data_object_ptr ret)
{
    core_data_object *theBind, *lastBind;
    int found = FALSE,
        unbindVar = FALSE;
    ATOM_HN *variableName = NULL;

    /*===============================================
     * Determine the name of the variable to be set.
     *===============================================*/
    core_eval_expression(env, core_get_first_arg(), ret);
    variableName = (ATOM_HN *)core_convert_data_ptr_to_pointer(ret);

    /*===========================================
     * Determine the new value for the variable.
     *===========================================*/

    if( core_get_first_arg()->next_arg == NULL )
    {
        unbindVar = TRUE;
    }
    else if( core_get_first_arg()->next_arg->next_arg == NULL )
    {
        core_eval_expression(env, core_get_first_arg()->next_arg, ret);
    }
    else
    {
        add_to_list(env, ret, core_get_first_arg()->next_arg, TRUE);
    }


    /*===============================================
     * Search for the variable in the list of binds.
     *===============================================*/

    theBind = get_flow_control_data(env)->variables;
    lastBind = NULL;

    while((theBind != NULL) && (found == FALSE))
    {
        if( theBind->metadata == (void *)variableName )
        {
            found = TRUE;
        }
        else
        {
            lastBind = theBind;
            theBind = theBind->next;
        }
    }

    /*========================================================
     * If variable was not in the list of binds, then add it.
     * Make sure that this operation preserves the bind list
     * as a stack.
     *========================================================*/

    if( found == FALSE )
    {
        if( unbindVar == FALSE )
        {
            theBind = core_mem_get_struct(env, core_data);
            theBind->metadata = (void *)variableName;
            inc_atom_count(variableName);
            theBind->next = NULL;

            if( lastBind == NULL )
            {
                get_flow_control_data(env)->variables = theBind;
            }
            else
            {
                lastBind->next = theBind;
            }
        }
        else
        {
            ret->type = ATOM;
            ret->value = get_false(env);
            return;
        }
    }
    else
    {
        core_value_decrement(env, theBind);
    }

    /*================================
     * Set the value of the variable.
     *================================*/

    if( unbindVar == FALSE )
    {
        theBind->type = ret->type;
        theBind->value = ret->value;
        theBind->begin = ret->begin;
        theBind->end = ret->end;
        core_value_increment(env, ret);
    }
    else
    {
        if( lastBind == NULL )
        {
            get_flow_control_data(env)->variables = theBind->next;
        }
        else
        {
            lastBind->next = theBind->next;
        }

        dec_atom_count(env, (struct atom_hash_node *)theBind->metadata);
        core_mem_return_struct(env, core_data, theBind);
        ret->type = ATOM;
        ret->value = get_false(env);
    }
}

/******************************************
 * lookup_binding: Searches the variables
 *   for a specified variable.
 *******************************************/
BOOLEAN lookup_binding(void *env, core_data_object_ptr vPtr, ATOM_HN *varName)
{
    core_data_object_ptr bindPtr;

    for( bindPtr = get_flow_control_data(env)->variables; bindPtr != NULL; bindPtr = bindPtr->next )
    {
        if( bindPtr->metadata == (void *)varName )
        {
            vPtr->type = bindPtr->type;
            vPtr->value = bindPtr->value;
            vPtr->begin = bindPtr->begin;
            vPtr->end = bindPtr->end;
            return(TRUE);
        }
    }

    return(FALSE);
}

/************************************************
 * flush_bindings: Removes all variables from the
 *   list of currently bound local variables.
 *************************************************/
void flush_bindings(void *env)
{
    core_release_data(env, get_flow_control_data(env)->variables, TRUE);
    get_flow_control_data(env)->variables = NULL;
}

/**************************************
 * broccoli_progn: H/L access routine
 *   for the progn function.
 ***************************************/
void broccoli_progn(void *env, core_data_object_ptr ret)
{
    struct core_expression *argPtr;

    argPtr = core_get_evaluation_data(env)->current_expression->args;

    if( argPtr == NULL )
    {
        ret->type = ATOM;
        ret->value = get_false(env);
        return;
    }

    while((argPtr != NULL) && (core_get_halt_eval(env) != TRUE))
    {
        core_eval_expression(env, argPtr, ret);

        if((get_flow_control_data(env)->break_flag == TRUE) || (get_flow_control_data(env)->return_flag == TRUE))
        {
            break;
        }

        argPtr = argPtr->next_arg;
    }

    if( core_get_halt_eval(env) == TRUE )
    {
        ret->type = ATOM;
        ret->value = get_false(env);
        return;
    }

    return;
}

void broccoli_if(void *env, core_data_object_ptr ret)
{
    int numArgs;
    struct core_expression *theExpr;

    /*============================================
     * Check for the correct number of arguments.
     *============================================*/

    if((core_get_evaluation_data(env)->current_expression->args == NULL) ||
       (core_get_evaluation_data(env)->current_expression->args->next_arg == NULL))
    {
        core_check_arg_range(env, "if", 2, 3);
        ret->type = ATOM;
        ret->value = get_false(env);
        return;
    }

    if( core_get_evaluation_data(env)->current_expression->args->next_arg->next_arg == NULL )
    {
        numArgs = 2;
    }
    else if( core_get_evaluation_data(env)->current_expression->args->next_arg->next_arg->next_arg == NULL )
    {
        numArgs = 3;
    }
    else
    {
        core_check_arg_range(env, "if", 2, 3);
        ret->type = ATOM;
        ret->value = get_false(env);
        return;
    }

    /*=========================
     * Evaluate the condition.
     *=========================*/

    core_eval_expression(env, core_get_evaluation_data(env)->current_expression->args, ret);

    if((get_flow_control_data(env)->break_flag == TRUE) || (get_flow_control_data(env)->return_flag == TRUE))
    {
        ret->type = ATOM;
        ret->value = get_false(env);
        return;
    }

    /*=========================================
     * If the condition evaluated to FALSE and
     * an "else" portion exists, evaluate it
     * and return the value.
     *=========================================*/

    if((ret->value == get_false(env)) &&
       (ret->type == ATOM) &&
       (numArgs == 3))
    {
        theExpr = core_get_evaluation_data(env)->current_expression->args->next_arg->next_arg;

        switch( theExpr->type )
        {
        case INTEGER:
        case FLOAT:
        case ATOM:
        case STRING:
#if OBJECT_SYSTEM
        case INSTANCE_NAME:
        case INSTANCE_ADDRESS:
#endif
        case EXTERNAL_ADDRESS:
            ret->type = theExpr->type;
            ret->value = theExpr->value;
            break;

        default:
            core_eval_expression(env, theExpr, ret);
            break;
        }

        return;
    }

    /*===================================================
     * Otherwise if the symbol evaluated to a non-FALSE
     * value, evaluate the "then" portion and return it.
     *===================================================*/

    else if((ret->value != get_false(env)) ||
            (ret->type != ATOM))
    {
        theExpr = core_get_evaluation_data(env)->current_expression->args->next_arg;

        switch( theExpr->type )
        {
        case INTEGER:
        case FLOAT:
        case ATOM:
        case STRING:
#if OBJECT_SYSTEM
        case INSTANCE_NAME:
        case INSTANCE_ADDRESS:
#endif
        case EXTERNAL_ADDRESS:
            ret->type = theExpr->type;
            ret->value = theExpr->value;
            break;

        default:
            core_eval_expression(env, theExpr, ret);
            break;
        }

        return;
    }

    /*=========================================
     * Return FALSE if the condition evaluated
     * to FALSE and there is no "else" portion
     * of the if statement.
     *=========================================*/

    ret->type = ATOM;
    ret->value = get_false(env);
    return;
}

/***********************************************
 * broccoli_get_loop_count
 ************************************************/
long long broccoli_get_loop_count(void *env)
{
    int depth;
    LOOP_COUNT_STACK *tmpCounter;

    depth = to_int(core_get_first_arg()->value);
    tmpCounter = get_flow_control_data(env)->loop_count_stack;

    while( depth > 0 )
    {
        tmpCounter = tmpCounter->nxt;
        depth--;
    }

    return(tmpCounter->count);
}
