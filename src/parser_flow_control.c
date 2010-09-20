#define __PARSER_FLOW_CONTROL_SOURCE__

#include <stdio.h>
#define _STDIO_INCLUDED_

#include "setup.h"

#include "core_arguments.h"
#include "constraints_kernel.h"
#include "constraints_query.h"
#include "constraints_operators.h"
#include "constraints_operators.h"
#include "core_environment.h"
#include "parser_expressions.h"
#include "core_memory.h"
#include "modules_query.h"
#include "type_list.h"
#include "router.h"
#include "core_scanner.h"
#include "core_gc.h"

#include "parser_flow_control.h"

#define PRCDRPSR_DATA 12

struct flow_control_data
{
    struct binding *binding_names;
};

#define get_flow_control_data(env) ((struct flow_control_data *)core_get_environment_data(env, PRCDRPSR_DATA))

/**************************************
 * LOCAL FUNCTION PROTOTYPES
 ***************************************/

static struct core_expression * _parse_binding(void *, struct core_expression *, char *);
static int                      _add_binding_name(void *, struct atom_hash_node *, CONSTRAINT_META *);
static void                     _delete_flow_control_data(void *);
static struct core_expression * _parse_if(void *, struct core_expression *, char *);
static void                     _replace_loop_count_variables(void *, ATOM_HN *, core_expression_object *, int);

/******************************************
 * init_flow_control_parsers
 *******************************************/
void init_flow_control_parsers(void *env)
{
    core_allocate_environment_data(env, PRCDRPSR_DATA, sizeof(struct flow_control_data), _delete_flow_control_data);

    core_add_function_parser(env, FUNC_NAME_ASSIGNMENT, _parse_binding);
    core_add_function_parser(env, "if", _parse_if);
}

/************************************************************
 * DeallocateProceduralFunctionData: Deallocates environment
 *    data for procedural functions.
 *************************************************************/
static void _delete_flow_control_data(void *env)
{
    struct binding *temp_bind;

    while( get_flow_control_data(env)->binding_names != NULL )
    {
        temp_bind = get_flow_control_data(env)->binding_names->next;
        core_mem_return_struct(env, binding, get_flow_control_data(env)->binding_names);
        get_flow_control_data(env)->binding_names = temp_bind;
    }
}

/*******************************************************
 * get_parsed_bindings:
 ********************************************************/
struct binding *get_parsed_bindings(void *env)
{
    return(get_flow_control_data(env)->binding_names);
}

/*******************************************************
 * set_parsed_bindings:
 ********************************************************/
void set_parsed_bindings(void *env, struct binding *newValue)
{
    get_flow_control_data(env)->binding_names = newValue;
}

/*******************************************************
 * clear_parsed_bindings:
 ********************************************************/
void clear_parsed_bindings(void *env)
{
    struct binding *temp_bind;

    while( get_flow_control_data(env)->binding_names != NULL )
    {
        temp_bind = get_flow_control_data(env)->binding_names->next;
        remove_constraint(env, get_flow_control_data(env)->binding_names->constraints);
        core_mem_return_struct(env, binding, get_flow_control_data(env)->binding_names);
        get_flow_control_data(env)->binding_names = temp_bind;
    }
}

/*******************************************************
 * are_bindings_empty:
 ********************************************************/
BOOLEAN are_bindings_empty(void *env)
{
    if( get_flow_control_data(env)->binding_names != NULL )
    {
        return(FALSE);
    }

    return(TRUE);
}

/**********************************************************
 * BindParse: purpose is to parse the bind statement. The
 *   parse of the statement is the return value.
 *   Syntax:  (bind ?var <expression>)
 ***********************************************************/
static struct core_expression *_parse_binding(void *env, struct core_expression *top, char *infile)
{
    struct token theToken;
    ATOM_HN *variableName;
    struct core_expression *texp;
    CONSTRAINT_META *theConstraint = NULL;

    core_save_pp_buffer(env, " ");

    /*=============================================
     * Next token must be the name of the variable
     * to be bound.
     *=============================================*/

    core_get_token(env, infile, &theToken);

    if((theToken.type != SCALAR_VARIABLE))
    {
        if((theToken.type != LIST_VARIABLE) || core_get_expression_data(env)->sequential_operation )
        {
            error_syntax(env, "bind function");
            core_return_expression(env, top);
            return(NULL);
        }
    }

    /*==============================
     * Process the bind expression.
     *==============================*/

    top->args = core_generate_constant(env, ATOM, theToken.value);
    variableName = (ATOM_HN *)theToken.value;

    texp = core_mem_get_struct(env, core_expression);
    texp->args = texp->next_arg = NULL;

    if( collect_args(env, texp, infile) == NULL )
    {
        core_return_expression(env, top);
        return(NULL);
    }

    top->args->next_arg = texp->args;
    core_mem_return_struct(env, core_expression, texp);

    if( top->args->next_arg != NULL )
    {
        theConstraint = convert_expression_to_constraint(env, top->args->next_arg);
    }

    _add_binding_name(env, variableName, theConstraint);

    return(top);
}

/*******************************************************
 * find_parsed_binding:
 ********************************************************/
int find_parsed_binding(void *env, ATOM_HN *name_sought)
{
    struct binding *var_ptr;
    int theIndex = 1;

    var_ptr = get_flow_control_data(env)->binding_names;

    while( var_ptr != NULL )
    {
        if( var_ptr->name == name_sought )
        {
            return(theIndex);
        }

        var_ptr = var_ptr->next;
        theIndex++;
    }

    return(0);
}

/*******************************************************
 * get_binding_count_in_current_context: Counts the number of variables
 *   names that have been bound using the bind function
 *   in the current context (e.g. the RHS of a rule).
 ********************************************************/
int get_binding_count_in_current_context(void *env)
{
    struct binding *theVariable;
    int theIndex = 0;

    theVariable = get_flow_control_data(env)->binding_names;

    while( theVariable != NULL )
    {
        theVariable = theVariable->next;
        theIndex++;
    }

    return(theIndex);
}

/***************************************************************
 * AddBindName: Adds a variable name used as the first argument
 *   of the bind function to the list of variable names parsed
 *   within the current semantic context (e.g. RHS of a rule).
 ****************************************************************/
static int _add_binding_name(void *env, ATOM_HN *variableName, CONSTRAINT_META *theConstraint)
{
    CONSTRAINT_META *tmpConstraint;
    struct binding *currentBind, *lastBind;
    int theIndex = 1;

    /*=========================================================
     * Look for the variable name in the list of bind variable
     * names already parsed. If it is found, then return the
     * index to the variable and union the new constraint
     * information with the old constraint information.
     *=========================================================*/

    lastBind = NULL;
    currentBind = get_flow_control_data(env)->binding_names;

    while( currentBind != NULL )
    {
        if( currentBind->name == variableName )
        {
            if( theConstraint != NULL )
            {
                tmpConstraint = currentBind->constraints;
                currentBind->constraints = constraint_union(env, theConstraint, currentBind->constraints);
                remove_constraint(env, tmpConstraint);
                remove_constraint(env, theConstraint);
            }

            return(theIndex);
        }

        lastBind = currentBind;
        currentBind = currentBind->next;
        theIndex++;
    }

    /*===============================================================
     * If the variable name wasn't found, then add it to the list of
     * variable names and store the constraint information with it.
     *===============================================================*/

    currentBind = core_mem_get_struct(env, binding);
    currentBind->name = variableName;
    currentBind->constraints = theConstraint;
    currentBind->next = NULL;

    if( lastBind == NULL )
    {
        get_flow_control_data(env)->binding_names = currentBind;
    }
    else
    {
        lastBind->next = currentBind;
    }

    return(theIndex);
}

/*******************************************************
 * remove_binding:
 ********************************************************/
void remove_binding(void *env, struct atom_hash_node *bname)
{
    struct binding *prv, *tmp;

    prv = NULL;
    tmp = get_flow_control_data(env)->binding_names;

    while((tmp != NULL) ? (tmp->name != bname) : FALSE )
    {
        prv = tmp;
        tmp = tmp->next;
    }

    if( tmp != NULL )
    {
        if( prv == NULL )
        {
            get_flow_control_data(env)->binding_names = tmp->next;
        }
        else
        {
            prv->next = tmp->next;
        }

        remove_constraint(env, tmp->constraints);
        core_mem_return_struct(env, binding, tmp);
    }
}

static struct core_expression *_parse_if(void *env, struct core_expression *top, char *infile)
{
    struct token theToken;

    /*============================
     * Process the if expression.
     *============================*/

    core_save_pp_buffer(env, " ");

    top->args = parse_atom_or_expression(env, infile, NULL);

    if( top->args == NULL )
    {
        core_return_expression(env, top);
        return(NULL);
    }

    core_increment_pp_buffer_depth(env, 3);

    /*==============================
     * Process the if then actions.
     *==============================*/

    core_inject_ws_into_pp_buffer(env);

    if( core_get_expression_data(env)->saved_contexts->rtn == TRUE )
    {
        core_get_expression_data(env)->return_context = TRUE;
    }

    if( core_get_expression_data(env)->saved_contexts->brk == TRUE )
    {
        core_get_expression_data(env)->break_context = TRUE;
    }

    top->args->next_arg = group_actions(env, infile, &theToken, TRUE, "else", FALSE);

    if( top->args->next_arg == NULL )
    {
        core_return_expression(env, top);
        return(NULL);
    }

    top->args->next_arg = strip_frivolous_progn(env, top->args->next_arg);

    /*===========================================
     * A ')' signals an if then without an else.
     *===========================================*/

    if( theToken.type == RPAREN )
    {
        core_decrement_pp_buffer_depth(env, 3);
        core_backup_pp_buffer(env);
        core_backup_pp_buffer(env);
        core_save_pp_buffer(env, theToken.pp);
        return(top);
    }

    /*=============================================
     * Keyword 'else' must follow if then actions.
     *=============================================*/

    if((theToken.type != ATOM) || (strcmp(to_string(theToken.value), "else") != 0))
    {
        error_syntax(env, "if function");
        core_return_expression(env, top);
        return(NULL);
    }

    /*==============================
     * Process the if else actions.
     *==============================*/

    core_inject_ws_into_pp_buffer(env);
    top->args->next_arg->next_arg = group_actions(env, infile, &theToken, TRUE, NULL, FALSE);

    if( top->args->next_arg->next_arg == NULL )
    {
        core_return_expression(env, top);
        return(NULL);
    }

    top->args->next_arg->next_arg = strip_frivolous_progn(env, top->args->next_arg->next_arg);

    /*======================================================
     * Check for the closing right parenthesis of the if.
     *======================================================*/

    if( theToken.type != RPAREN )
    {
        error_syntax(env, "if function");
        core_return_expression(env, top);
        return(NULL);
    }

    /*===========================================
     * A ')' signals an if then without an else.
     *===========================================*/

    core_backup_pp_buffer(env);
    core_backup_pp_buffer(env);
    core_save_pp_buffer(env, ")");
    core_decrement_pp_buffer_depth(env, 3);
    return(top);
}

/**************************************************
 * ReplaceLoopCountVars
 ***************************************************/
static void _replace_loop_count_variables(void *env, ATOM_HN *loopVar, core_expression_object *theExp, int depth)
{
    while( theExp != NULL )
    {
        if((theExp->type != SCALAR_VARIABLE) ? FALSE :
           (strcmp(to_string(theExp->value), to_string(loopVar)) == 0))
        {
            theExp->type = FCALL;
            theExp->value = (void *)core_lookup_function(env, "(get-loop-count)");
            theExp->args = core_generate_constant(env, INTEGER, store_long(env, (long long)depth));
        }
        else if( theExp->args != NULL )
        {
            if((theExp->type != FCALL) ? FALSE :
               (theExp->value == (void *)core_lookup_function(env, "for#")))
            {
                _replace_loop_count_variables(env, loopVar, theExp->args, depth + 1);
            }
            else
            {
                _replace_loop_count_variables(env, loopVar, theExp->args, depth);
            }
        }

        theExp = theExp->next_arg;
    }
}
