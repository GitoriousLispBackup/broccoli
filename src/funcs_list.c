/* Purpose: Contains the code for several list
 *   functions including first$, rest$, subseq$, delete$,
 *   delete-member$, replace-member$
 *   replace$, insert$, explode$, implode$, nth$, member$,
 *   subsetp, progn$, str-implode, str-explode, subset, nth,
 *   mv-replace, member, mv-subseq, and mv-delete.           */

#define __FUNCS_LIST_SOURCE__

#include "setup.h"

#include <stdio.h>
#define _STDIO_INCLUDED_
#include <string.h>

#include "core_arguments.h"
#include "core_environment.h"
#include "parser_expressions.h"
#include "core_memory.h"
#include "type_list.h"
#include "funcs_list.h"
#include "parser_flow_control.h"
#include "funcs_flow_control.h"
#include "router.h"
#include "core_scanner.h"
#include "core_gc.h"

#if OBJECT_SYSTEM
#include "classes.h"
#endif

/*************
 * STRUCTURES
 **************/

typedef struct _loop_variable_stack
{
    unsigned short type;
    void *         value;
    long           index;
    struct _loop_variable_stack *nxt;
} LOOP_VARIABLE_STACK;

/**************************************
 * LOCAL FUNCTION PROTOTYPES
 ***************************************/
static struct core_expression            *_foreach_parser(void *, struct core_expression *, char *);
static void _progn_driver(void *, core_data_object_ptr, char *);
static void _replace_loop_variables(void *, ATOM_HN *, struct core_expression *, int);

/**************************************
 * LOCAL INTERNAL VARIABLE DEFINITIONS
 ***************************************/

#define LIST_FUNCTION_DATA_INDEX 10

struct list_function_data
{
    LOOP_VARIABLE_STACK *variable_stack;
};

#define get_list_function_data(env) ((struct list_function_data *)core_get_environment_data(env, LIST_FUNCTION_DATA_INDEX))

/*********************************************
 * init_list_functions: Initializes
 *   the list functions.
 **********************************************/
void init_list_functions(void *env)
{
    core_allocate_environment_data(env, LIST_FUNCTION_DATA_INDEX, sizeof(struct list_function_data), NULL);

    core_define_function(env, FUNC_NAME_CREATE_LIST, RT_LIST, PTR_FN brocolli_create_list, "brocolli_create_list", FUNC_CNSTR_CREATE_LIST);
    core_define_function(env, "cat", RT_LIST, PTR_FN broccoli_concatenate, "broccoli_concatenate", FUNC_CNSTR_CREATE_LIST);
    core_define_function(env, FUNC_NAME_CAR, RT_UNKNOWN, PTR_FN broccoli_car, "broccoli_car", FUNC_CNSTR_CAR);
    core_define_function(env, FUNC_NAME_CDR, RT_LIST, PTR_FN broccoli_cdr, "broccoli_cdr", FUNC_CNSTR_CDR);
    core_define_function(env, FUNC_NAME_FOREACH, RT_UNKNOWN, PTR_FN broccoli_foreach, "broccoli_foreach", FUNC_CNSTR_PROGN);
    core_define_function(env, FUNC_NAME_SLICE, RT_LIST, PTR_FN broccoli_slice, "broccoli_slice", FUNC_CNSTR_SLICE);
    core_define_function(env, FUNC_NAME_PROGN_VAR, RT_UNKNOWN, PTR_FN broccoli_get_loop_variable, "broccoli_get_loop_variable", FUNC_CNSTR_PROGN_VAR);
    core_define_function(env, FUNC_NAME_PROGN_INDEX, RT_LONG, PTR_FN broccoli_get_loop_index, "broccoli_get_loop_index", FUNC_CNSTR_PROGN_INDEX);
    core_define_function(env, "range", RT_LIST, PTR_FN broccoli_range, "broccoli_range", "22n");
    core_define_function(env, "len",          'g', PTR_FN broccoli_length,      "broccoli_length", "11q");

    core_add_function_parser(env, FUNC_NAME_FOREACH, _foreach_parser);
}

/*****************************************************************
 * brocolli_create_list: H/L access routine for the create$ function.
 ******************************************************************/
void brocolli_create_list(void *env, core_data_object_ptr ret)
{
    add_to_list(env, ret, core_get_first_arg(), TRUE);
}

void broccoli_concatenate(void *env, core_data_object_ptr ret)
{
    concatenate_lists(env, ret, core_get_first_arg(), TRUE);
}

/**************************************
 * broccoli_car: H/L access routine
 *   for the first$ function.
 ***************************************/
void broccoli_car(void *env, core_data_object_ptr ret)
{
    core_data_object val;
    struct list *list;
    long long idx = 1l;
    short type;

    /*===================================
     * Get the segment to be subdivided.
     *===================================*/

    if( core_check_arg_type(env, "first", 1, LIST, &val) == FALSE )
    {
        core_create_error_list(env, ret);
        return;
    }

    if( (idx > core_get_data_length(val)) )
    {
        core_set_pointer_type(ret, ATOM);
        core_set_pointer_value(ret, (void *)store_atom(env, "nil"));

        return;
    }

    list = (struct list *)core_get_value(val);

    type = get_list_node_type(list, ((long)idx) + core_get_data_start(val) - 1);
    core_set_pointer_type(ret, type);

    if( type == LIST )
    {
        struct list *list_ptr = get_list_node_value(list, ((long)idx) + core_get_data_start(val) - 1);

        core_set_pointer_value(ret, list_ptr);
        core_set_data_ptr_start(ret, 1L);
        core_set_data_ptr_end(ret, list_ptr->length);
    }
    else
    {
        core_set_pointer_value(ret, get_list_node_value(list, ((long)idx) + core_get_data_start(val) - 1));
    }
}

/*************************************
 * broccoli_cdr: H/L access routine
 *   for the rest$ function.
 **************************************/
void broccoli_cdr(void *env, core_data_object_ptr ret)
{
    core_data_object val;
    struct list *list;

    /*===================================
     * Get the segment to be subdivided.
     *===================================*/

    if( core_check_arg_type(env, "rest", 1, LIST, &val) == FALSE )
    {
        core_create_error_list(env, ret);
        return;
    }

    list = (struct list *)core_convert_data_to_pointe(val);

    /*=========================
     * Return the new segment.
     *=========================*/

    core_set_pointer_type(ret, LIST);
    core_set_pointer_value(ret, list);

    if( core_get_data_start(val) > core_get_data_end(val))
    {
        core_set_data_ptr_start(ret, core_get_data_start(val));
    }
    else
    {
        core_set_data_ptr_start(ret, core_get_data_start(val) + 1);
    }

    core_set_data_ptr_end(ret, core_get_data_end(val));
}

/**************************************
 * broccoli_foreach: H/L access routine
 *   for the foreach function.
 ***************************************/
void broccoli_foreach(void *env, core_data_object_ptr result)
{
    _progn_driver(env, result, FUNC_NAME_FOREACH);
}

/******************************************
 * ListPrognDriver: Driver routine
 *   for the progn$ and foreach functions.
 ******************************************/
static void _progn_driver(void *env, core_data_object_ptr result, char *functionName)
{
    core_expression_object *theExp;
    core_data_object argval;
    long i, end; /* 6.04 Bug Fix */
    LOOP_VARIABLE_STACK *temp;

    temp = core_mem_get_struct(env, _loop_variable_stack);
    temp->type = ATOM;
    temp->value = get_false(env);
    temp->nxt = get_list_function_data(env)->variable_stack;

    get_list_function_data(env)->variable_stack = temp;
    result->type = ATOM;
    result->value = get_false(env);

    if( core_check_arg_type(env, functionName, 1, LIST, &argval) == FALSE )
    {
        get_list_function_data(env)->variable_stack = temp->nxt;
        core_mem_return_struct(env, _loop_variable_stack, temp);
        return;
    }

    core_value_increment(env, &argval);
    end = core_get_data_end(argval);

    for( i = core_get_data_start(argval) ; i <= end ; i++ )
    {
        temp->type = get_list_node_type(argval.value, i);
        temp->value = get_list_node_value(argval.value, i);
        /* temp->index = i; */
        temp->index = (i - core_get_data_start(argval)) + 1;

        for( theExp = core_get_first_arg()->next_arg ; theExp != NULL ; theExp = theExp->next_arg )
        {
            core_get_evaluation_data(env)->eval_depth++;
            core_eval_expression(env, theExp, result);
            core_get_evaluation_data(env)->eval_depth--;

            if( get_flow_control_data(env)->return_flag == TRUE )
            {
                core_pass_return_value(env, result);
            }

            core_gc_periodic_cleanup(env, FALSE, TRUE);

            if( core_get_evaluation_data(env)->halt || get_flow_control_data(env)->break_flag || get_flow_control_data(env)->return_flag )
            {
                core_value_decrement(env, &argval);
                get_flow_control_data(env)->break_flag = FALSE;

                if( core_get_evaluation_data(env)->halt )
                {
                    result->type = ATOM;
                    result->value = get_false(env);
                }

                get_list_function_data(env)->variable_stack = temp->nxt;
                core_mem_return_struct(env, _loop_variable_stack, temp);
                return;
            }
        }
    }

    core_value_decrement(env, &argval);
    get_flow_control_data(env)->break_flag = FALSE;
    get_list_function_data(env)->variable_stack = temp->nxt;
    core_mem_return_struct(env, _loop_variable_stack, temp);
}

/*****************************************************
 * ForeachParser: Parses the foreach function.
 ******************************************************/
static struct core_expression *_foreach_parser(void *env, struct core_expression *top, char *infile)
{
    struct binding *oldBindList, *newBindList, *prev;
    struct token tkn;
    struct core_expression *tmp;
    ATOM_HN *fieldVar = NULL;

    core_save_pp_buffer(env, " ");
    core_get_token(env, infile, &tkn);

    if( tkn.type != SCALAR_VARIABLE )
    {
        goto ForeachParseError;
    }

    fieldVar = (ATOM_HN *)tkn.value;
    core_save_pp_buffer(env, " ");

    core_get_token(env, infile, &tkn);

    if((tkn.type != ATOM) || (strcmp(to_string(tkn.value), "in") != 0))
    {
        error_syntax(env, "function for, symbol 'in' required in list looping mode");
        core_return_expression(env, top);
        return(NULL);
    }

    top->args = parse_atom_or_expression(env, infile, NULL);

    if( top->args == NULL )
    {
        core_return_expression(env, top);
        return(NULL);
    }

    if( core_check_against_restrictions(env, top->args, (int)'m'))
    {
        goto ForeachParseError;
    }

    oldBindList = get_parsed_bindings(env);
    set_parsed_bindings(env, NULL);
    core_increment_pp_buffer_depth(env, 3);
    core_get_expression_data(env)->break_context = TRUE;
    core_get_expression_data(env)->return_context = core_get_expression_data(env)->saved_contexts->rtn;
    core_inject_ws_into_pp_buffer(env);
    top->args->next_arg = group_actions(env, infile, &tkn, TRUE, NULL, FALSE);
    core_decrement_pp_buffer_depth(env, 3);
    core_backup_pp_buffer(env);
    core_backup_pp_buffer(env);
    core_save_pp_buffer(env, tkn.pp);

    if( top->args->next_arg == NULL )
    {
        clear_parsed_bindings(env);
        set_parsed_bindings(env, oldBindList);
        core_return_expression(env, top);
        return(NULL);
    }

    tmp = top->args->next_arg;
    top->args->next_arg = tmp->args;
    tmp->args = NULL;
    core_return_expression(env, tmp);
    newBindList = get_parsed_bindings(env);
    prev = NULL;

    while( newBindList != NULL )
    {
        if((fieldVar == NULL) ? FALSE :
           (strcmp(to_string(newBindList->name), to_string(fieldVar)) == 0))
        {
            clear_parsed_bindings(env);
            set_parsed_bindings(env, oldBindList);
            error_print_id(env, "FLOW", 2, FALSE);
            print_router(env, WERROR, "Cannot rebind variable in `for` body.\n");
            core_return_expression(env, top);
            return(NULL);
        }

        prev = newBindList;
        newBindList = newBindList->next;
    }

    if( prev == NULL )
    {
        set_parsed_bindings(env, oldBindList);
    }
    else
    {
        prev->next = oldBindList;
    }

    if( fieldVar != NULL )
    {
        _replace_loop_variables(env, fieldVar, top->args->next_arg, 0);
    }

    return(top);

ForeachParseError:
    error_syntax(env, FUNC_NAME_FOREACH);
    core_return_expression(env, top);
    return(NULL);
}

/*********************************************
 * ReplaceMvPrognFieldVars: Replaces variable
 *   references found in the progn$ function.
 **********************************************/
static void _replace_loop_variables(void *env, ATOM_HN *fieldVar, struct core_expression *theExp, int depth)
{
    size_t flen;

    flen = strlen(to_string(fieldVar));

    while( theExp != NULL )
    {
        if((theExp->type != SCALAR_VARIABLE) ? FALSE :
           (strncmp(to_string(theExp->value), to_string(fieldVar),
                    (STD_SIZE)flen) == 0))
        {
            if( to_string(theExp->value)[flen] == '\0' )
            {
                theExp->type = FCALL;
                theExp->value = (void *)core_lookup_function(env, FUNC_NAME_PROGN_VAR);
                theExp->args = core_generate_constant(env, INTEGER, store_long(env, (long long)depth));
            }
            else if( strcmp(to_string(theExp->value) + flen, "-index") == 0 )
            {
                theExp->type = FCALL;
                theExp->value = (void *)core_lookup_function(env, FUNC_NAME_PROGN_INDEX);
                theExp->args = core_generate_constant(env, INTEGER, store_long(env, (long long)depth));
            }
        }
        else if( theExp->args != NULL )
        {
            if((theExp->type == FCALL) && (theExp->value == (void *)core_lookup_function(env, "progn$")))
            {
                _replace_loop_variables(env, fieldVar, theExp->args, depth + 1);
            }
            else
            {
                _replace_loop_variables(env, fieldVar, theExp->args, depth);
            }
        }

        theExp = theExp->next_arg;
    }
}

/**************************************************
 * broccoli_get_loop_variable
 ***************************************************/
void broccoli_get_loop_variable(void *env, core_data_object_ptr result)
{
    int depth;
    LOOP_VARIABLE_STACK *temp;

    depth = to_int(core_get_first_arg()->value);
    temp = get_list_function_data(env)->variable_stack;

    while( depth > 0 )
    {
        temp = temp->nxt;
        depth--;
    }

    result->type = temp->type;
    result->value = temp->value;
}

/**************************************************
 * broccoli_get_loop_index
 ***************************************************/
long broccoli_get_loop_index(void *env)
{
    int depth;
    LOOP_VARIABLE_STACK *temp;

    depth = to_int(core_get_first_arg()->value);
    temp = get_list_function_data(env)->variable_stack;

    while( depth > 0 )
    {
        temp = temp->nxt;
        depth--;
    }

    return(temp->index);
}

/**************************************************
 * broccoli_slice
 ***************************************************/
void broccoli_slice(void *env, core_data_object_ptr sub_value)
{
    core_data_object value;
    struct list *list;
    long long offset, start, end, length;

    /*===================================
     * Get the segment to be subdivided.
     *===================================*/

    if( core_check_arg_type(env, FUNC_NAME_SLICE, 1, LIST, &value) == FALSE )
    {
        core_create_error_list(env, sub_value);
        return;
    }

    list = (struct list *)core_convert_data_to_pointe(value);
    offset = core_get_data_start(value);
    length = core_get_data_length(value);

    /*=============================================
     * Get range arguments. If they are not within
     * appropriate ranges, return a null segment.
     *=============================================*/

    if( core_check_arg_type(env, FUNC_NAME_SLICE, 2, INTEGER, &value) == FALSE )
    {
        core_create_error_list(env, sub_value);
        return;
    }

    start = core_convert_data_to_long(value);

    if( core_check_arg_type(env, FUNC_NAME_SLICE, 3, INTEGER, &value) == FALSE )
    {
        core_create_error_list(env, sub_value);
        return;
    }

    end = core_convert_data_to_long(value);

    if((end < 1) || (end < start))
    {
        core_create_error_list(env, sub_value);
        return;
    }

    /*===================================================
     * Adjust lengths  to conform to segment boundaries.
     *===================================================*/

    if( start > length )
    {
        core_create_error_list(env, sub_value);
        return;
    }

    if( end > length )
    {
        end = length;
    }

    if( start < 0 )
    {
        start = 0;
    }

    /*=========================
     * Return the new segment.
     *=========================*/

    core_set_pointer_type(sub_value, LIST);
    core_set_pointer_value(sub_value, list);
    core_set_data_ptr_end(sub_value, offset + end - 1);
    core_set_data_ptr_start(sub_value, offset + start);
}

void broccoli_range(void *env, core_data_object_ptr ret)
{
    core_data_object value;
    struct list *list;
    long long length = 0, start, end;
    int direction = 0;
    long long val;
    int i = 1;

    /*=============================================
     * Get range arguments. If they are not within
     * appropriate ranges, return a null segment.
     *=============================================*/

    if( core_check_arg_type(env, "range", 1, INTEGER, &value) == FALSE )
    {
        core_create_error_list(env, ret);
        return;
    }

    start = core_convert_data_to_long(value);

    if( core_check_arg_type(env, "range", 2, INTEGER, &value) == FALSE )
    {
        core_create_error_list(env, ret);
        return;
    }

    end = core_convert_data_to_long(value);

    if( start < end )
    {
        direction = 1;
    }
    else if( start > end )
    {
        direction = -1;
    }

    if( start < 0 )
    {
        if( end < 0 )
        {
            length = abs(start - end);
        }
        else
        {
            length = abs(start) + end;
        }
    }
    else if( end < 0 )
    {
        length = start + (direction * end);
    }
    else
    {
        length = abs(end - start);
    }

    length++;

    list = create_list(env, length);

    for( val = start; i <= length; i++, val++ )
    {
        set_list_node_type(list, i, INTEGER);
        set_list_node_value(list, i, (void *)store_long(env, val));
    }

    core_set_pointer_type(ret, LIST);
    core_set_data_ptr_start(ret, 1);
    core_set_data_ptr_end(ret, get_list_length(list));
    core_set_pointer_value(ret, (void*)list);

    return;
}

long long broccoli_length(void *env)
{
    core_data_object item;

    /*====================================================
     * The length$ function expects exactly one argument.
     *====================================================*/

    if( core_check_arg_count(env, "len", EXACTLY, 1) == -1 )
    {
        return(-1L);
    }

    core_get_arg_at(env, 1, &item);

    /*====================================================
     * If the argument is a string or symbol, then return
     * the number of characters in the argument.
     *====================================================*/

    if((core_get_type(item) == STRING) || (core_get_type(item) == ATOM))
    {
        return( (long)strlen(core_convert_data_to_string(item)));
    }

    /*====================================================
     * If the argument is a list value, then return
     * the number of fields in the argument.
     *====================================================*/

    if( core_get_type(item) == LIST )
    {
        return( (long)core_get_data_length(item));
    }

    /*=============================================
     * If the argument wasn't a string, symbol, or
     * list value, then generate an error.
     *=============================================*/

    core_set_eval_error(env, TRUE);
    report_implicit_type_error(env, "len", 1);
    return(-1L);
}
