/* Purpose: Instance_set Queries Parsing Routines            */

/* =========================================
 *****************************************
 *            EXTERNAL DEFINITIONS
 *  =========================================
 ***************************************** */
#include "setup.h"

#if INSTANCE_SET_QUERIES

#include <string.h>

#include "classes_kernel.h"
#include "core_environment.h"
#include "parser_expressions.h"
#include "core_functions.h"
#include "classes_instances_query.h"
#include "parser_flow_control.h"
#include "core_utilities.h"
#include "router.h"
#include "core_scanner.h"
#include "router_string.h"

#define __PARSER_CLASS_QUERY_SOURCE__
#include "parser_class_query.h"

/* =========================================
 *****************************************
 *                CONSTANTS
 *  =========================================
 ***************************************** */
#define INSTANCE_SLOT_REF ':'

/* =========================================
 *****************************************
 *   INTERNALLY VISIBLE FUNCTION HEADERS
 *  =========================================
 ***************************************** */

static core_expression_object * ParseQueryRestrictions(void *, core_expression_object *, char *, struct token *);
static BOOLEAN      ReplaceClassNameWithReference(void *, core_expression_object *);
static int          ParseQueryTestExpression(void *, core_expression_object *, char *);
static int          ParseQueryActionExpression(void *, core_expression_object *, char *, core_expression_object *, struct token *);
static void         ReplaceInstanceVariables(void *, core_expression_object *, core_expression_object *, int, int);
static void         ReplaceSlotReference(void *, core_expression_object *, core_expression_object *, struct core_function_definition *, int);
static int          IsQueryFunction(core_expression_object *);

/* =========================================
 *****************************************
 *       EXTERNALLY VISIBLE FUNCTIONS
 *  =========================================
 ***************************************** */

/***********************************************************************
 *  NAME         : ParseQueryNoAction
 *  DESCRIPTION  : Parses the following functions :
 *                (any-instancep)
 *                (find-first-instance)
 *                (find-all-instances)
 *  INPUTS       : 1) The address of the top node of the query function
 *              2) The logical name of the input
 *  RETURNS      : The completed expression chain, or NULL on errors
 *  SIDE EFFECTS : The expression chain is extended, or the "top" node
 *              is deleted on errors
 *  NOTES        : H/L Syntax :
 *
 *              (<function> <query-block>)
 *
 *              <query-block>  :== (<instance-var>+) <query-expression>
 *              <instance-var> :== (<var-name> <class-name>+)
 *
 *              Parses into following form :
 *
 *              <query-function>
 |
 *                   V
 *              <query-expression>  ->
 *
 *              <class-1a> -> <class-1b> -> (QDS) ->
 *
 *              <class-2a> -> <class-2b> -> (QDS) -> ...
 ***********************************************************************/
globle core_expression_object *ParseQueryNoAction(void *theEnv, core_expression_object *top, char *readSource)
{
    core_expression_object *insQuerySetVars;
    struct token queryInputToken;

    insQuerySetVars = ParseQueryRestrictions(theEnv, top, readSource, &queryInputToken);

    if( insQuerySetVars == NULL )
    {
        return(NULL);
    }

    core_increment_pp_buffer_depth(theEnv, 3);
    core_inject_ws_into_pp_buffer(theEnv);

    if( ParseQueryTestExpression(theEnv, top, readSource) == FALSE )
    {
        core_decrement_pp_buffer_depth(theEnv, 3);
        core_return_expression(theEnv, insQuerySetVars);
        return(NULL);
    }

    core_decrement_pp_buffer_depth(theEnv, 3);
    core_get_token(theEnv, readSource, &queryInputToken);

    if( core_get_type(queryInputToken) != RPAREN )
    {
        error_syntax(theEnv, "instance-set query function");
        core_return_expression(theEnv, top);
        core_return_expression(theEnv, insQuerySetVars);
        return(NULL);
    }

    ReplaceInstanceVariables(theEnv, insQuerySetVars, top->args, TRUE, 0);
    core_return_expression(theEnv, insQuerySetVars);
    return(top);
}

/***********************************************************************
 *  NAME         : ParseQueryAction
 *  DESCRIPTION  : Parses the following functions :
 *                (do-for-instance)
 *                (do-for-all-instances)
 *                (delayed-do-for-all-instances)
 *  INPUTS       : 1) The address of the top node of the query function
 *              2) The logical name of the input
 *  RETURNS      : The completed expression chain, or NULL on errors
 *  SIDE EFFECTS : The expression chain is extended, or the "top" node
 *              is deleted on errors
 *  NOTES        : H/L Syntax :
 *
 *              (<function> <query-block> <query-action>)
 *
 *              <query-block>  :== (<instance-var>+) <query-expression>
 *              <instance-var> :== (<var-name> <class-name>+)
 *
 *              Parses into following form :
 *
 *              <query-function>
 |
 *                   V
 *              <query-expression> -> <query-action>  ->
 *
 *              <class-1a> -> <class-1b> -> (QDS) ->
 *
 *              <class-2a> -> <class-2b> -> (QDS) -> ...
 ***********************************************************************/
globle core_expression_object *ParseQueryAction(void *theEnv, core_expression_object *top, char *readSource)
{
    core_expression_object *insQuerySetVars;
    struct token queryInputToken;

    insQuerySetVars = ParseQueryRestrictions(theEnv, top, readSource, &queryInputToken);

    if( insQuerySetVars == NULL )
    {
        return(NULL);
    }

    core_increment_pp_buffer_depth(theEnv, 3);
    core_inject_ws_into_pp_buffer(theEnv);

    if( ParseQueryTestExpression(theEnv, top, readSource) == FALSE )
    {
        core_decrement_pp_buffer_depth(theEnv, 3);
        core_return_expression(theEnv, insQuerySetVars);
        return(NULL);
    }

    core_inject_ws_into_pp_buffer(theEnv);

    if( ParseQueryActionExpression(theEnv, top, readSource, insQuerySetVars, &queryInputToken) == FALSE )
    {
        core_decrement_pp_buffer_depth(theEnv, 3);
        core_return_expression(theEnv, insQuerySetVars);
        return(NULL);
    }

    core_decrement_pp_buffer_depth(theEnv, 3);

    if( core_get_type(queryInputToken) != RPAREN )
    {
        error_syntax(theEnv, "instance-set query function");
        core_return_expression(theEnv, top);
        core_return_expression(theEnv, insQuerySetVars);
        return(NULL);
    }

    ReplaceInstanceVariables(theEnv, insQuerySetVars, top->args, TRUE, 0);
    ReplaceInstanceVariables(theEnv, insQuerySetVars, top->args->next_arg, FALSE, 0);
    core_return_expression(theEnv, insQuerySetVars);
    return(top);
}

/* =========================================
 *****************************************
 *       INTERNALLY VISIBLE FUNCTIONS
 *  =========================================
 ***************************************** */

/***************************************************************
 *  NAME         : ParseQueryRestrictions
 *  DESCRIPTION  : Parses the class restrictions for a query
 *  INPUTS       : 1) The top node of the query expression
 *              2) The logical name of the input
 *              3) Caller's token buffer
 *  RETURNS      : The instance-variable expressions
 *  SIDE EFFECTS : Entire query expression deleted on errors
 *              Nodes allocated for restrictions and instance
 *                variable expressions
 *              Class restrictions attached to query-expression
 *                as arguments
 *  NOTES        : Expects top != NULL
 ***************************************************************/
static core_expression_object *ParseQueryRestrictions(void *theEnv, core_expression_object *top, char *readSource, struct token *queryInputToken)
{
    core_expression_object *insQuerySetVars = NULL, *lastInsQuerySetVars = NULL,
    *classExp = NULL, *lastClassExp,
    *tmp, *lastOne = NULL;
    int error = FALSE;

    core_save_pp_buffer(theEnv, " ");
    core_get_token(theEnv, readSource, queryInputToken);

    if( queryInputToken->type != LPAREN )
    {
        goto ParseQueryRestrictionsError1;
    }

    core_get_token(theEnv, readSource, queryInputToken);

    if( queryInputToken->type != LPAREN )
    {
        goto ParseQueryRestrictionsError1;
    }

    while( queryInputToken->type == LPAREN )
    {
        core_get_token(theEnv, readSource, queryInputToken);

        if( queryInputToken->type != SCALAR_VARIABLE )
        {
            goto ParseQueryRestrictionsError1;
        }

        tmp = insQuerySetVars;

        while( tmp != NULL )
        {
            if( tmp->value == queryInputToken->value )
            {
                error_print_id(theEnv, "INSQYPSR", 1, FALSE);
                print_router(theEnv, WERROR, "Duplicate instance member variable name in function ");
                print_router(theEnv, WERROR, to_string(core_get_expression_function_handle(top)));
                print_router(theEnv, WERROR, ".\n");
                goto ParseQueryRestrictionsError2;
            }

            tmp = tmp->next_arg;
        }

        tmp = core_generate_constant(theEnv, SCALAR_VARIABLE, queryInputToken->value);

        if( insQuerySetVars == NULL )
        {
            insQuerySetVars = tmp;
        }
        else
        {
            lastInsQuerySetVars->next_arg = tmp;
        }

        lastInsQuerySetVars = tmp;
        core_save_pp_buffer(theEnv, " ");
        classExp = parse_args(theEnv, readSource, &error);

        if( error )
        {
            goto ParseQueryRestrictionsError2;
        }

        if( classExp == NULL )
        {
            goto ParseQueryRestrictionsError1;
        }

        if( ReplaceClassNameWithReference(theEnv, classExp) == FALSE )
        {
            goto ParseQueryRestrictionsError2;
        }

        lastClassExp = classExp;
        core_save_pp_buffer(theEnv, " ");

        while((tmp = parse_args(theEnv, readSource, &error)) != NULL )
        {
            if( ReplaceClassNameWithReference(theEnv, tmp) == FALSE )
            {
                goto ParseQueryRestrictionsError2;
            }

            lastClassExp->next_arg = tmp;
            lastClassExp = tmp;
            core_save_pp_buffer(theEnv, " ");
        }

        if( error )
        {
            goto ParseQueryRestrictionsError2;
        }

        core_backup_pp_buffer(theEnv);
        core_backup_pp_buffer(theEnv);
        core_save_pp_buffer(theEnv, ")");
        tmp = core_generate_constant(theEnv, ATOM, (void *)InstanceQueryData(theEnv)->QUERY_DELIMETER_ATOM);
        lastClassExp->next_arg = tmp;
        lastClassExp = tmp;

        if( top->args == NULL )
        {
            top->args = classExp;
        }
        else
        {
            lastOne->next_arg = classExp;
        }

        lastOne = lastClassExp;
        classExp = NULL;
        core_save_pp_buffer(theEnv, " ");
        core_get_token(theEnv, readSource, queryInputToken);
    }

    if( queryInputToken->type != RPAREN )
    {
        goto ParseQueryRestrictionsError1;
    }

    core_backup_pp_buffer(theEnv);
    core_backup_pp_buffer(theEnv);
    core_save_pp_buffer(theEnv, ")");
    return(insQuerySetVars);

ParseQueryRestrictionsError1:
    error_syntax(theEnv, "instance-set query function");

ParseQueryRestrictionsError2:
    core_return_expression(theEnv, classExp);
    core_return_expression(theEnv, top);
    core_return_expression(theEnv, insQuerySetVars);
    return(NULL);
}

/***************************************************
 *  NAME         : ReplaceClassNameWithReference
 *  DESCRIPTION  : In parsing an instance-set query,
 *              this function replaces a constant
 *              class name with an actual pointer
 *              to the class
 *  INPUTS       : The expression
 *  RETURNS      : TRUE if all OK, FALSE
 *              if class cannot be found
 *  SIDE EFFECTS : The expression type and value are
 *              modified if class is found
 *  NOTES        : Searches current and imported
 *              modules for reference
 ***************************************************/
static BOOLEAN ReplaceClassNameWithReference(void *theEnv, core_expression_object *theExp)
{
    char *theClassName;
    void *theDefclass;

    if( theExp->type == ATOM )
    {
        theClassName = to_string(theExp->value);
        theDefclass = (void *)LookupDefclassByMdlOrScope(theEnv, theClassName);

        if( theDefclass == NULL )
        {
            error_lookup(theEnv, "class", theClassName);
            return(FALSE);
        }

        theExp->type = DEFCLASS_PTR;
        theExp->value = theDefclass;
    }

    return(TRUE);
}

/*************************************************************
 *  NAME         : ParseQueryTestExpression
 *  DESCRIPTION  : Parses the test-expression for a query
 *  INPUTS       : 1) The top node of the query expression
 *              2) The logical name of the input
 *  RETURNS      : TRUE if all OK, FALSE otherwise
 *  SIDE EFFECTS : Entire query-expression deleted on errors
 *              Nodes allocated for new expression
 *              Test shoved in front of class-restrictions on
 *                 query argument list
 *  NOTES        : Expects top != NULL
 *************************************************************/
static int ParseQueryTestExpression(void *theEnv, core_expression_object *top, char *readSource)
{
    core_expression_object *qtest;
    int error;
    struct binding *oldBindList;

    error = FALSE;
    oldBindList = get_parsed_bindings(theEnv);
    set_parsed_bindings(theEnv, NULL);
    qtest = parse_args(theEnv, readSource, &error);

    if( error == TRUE )
    {
        set_parsed_bindings(theEnv, oldBindList);
        core_return_expression(theEnv, top);
        return(FALSE);
    }

    if( qtest == NULL )
    {
        set_parsed_bindings(theEnv, oldBindList);
        error_syntax(theEnv, "instance-set query function");
        core_return_expression(theEnv, top);
        return(FALSE);
    }

    qtest->next_arg = top->args;
    top->args = qtest;

    if( are_bindings_empty(theEnv) == FALSE )
    {
        clear_parsed_bindings(theEnv);
        set_parsed_bindings(theEnv, oldBindList);
        error_print_id(theEnv, "INSQYPSR", 2, FALSE);
        print_router(theEnv, WERROR, "Binds are not allowed in instance-set query in function ");
        print_router(theEnv, WERROR, to_string(core_get_expression_function_handle(top)));
        print_router(theEnv, WERROR, ".\n");
        core_return_expression(theEnv, top);
        return(FALSE);
    }

    set_parsed_bindings(theEnv, oldBindList);
    return(TRUE);
}

/*************************************************************
 *  NAME         : ParseQueryActionExpression
 *  DESCRIPTION  : Parses the action-expression for a query
 *  INPUTS       : 1) The top node of the query expression
 *              2) The logical name of the input
 *              3) List of query parameters
 *  RETURNS      : TRUE if all OK, FALSE otherwise
 *  SIDE EFFECTS : Entire query-expression deleted on errors
 *              Nodes allocated for new expression
 *              Action shoved in front of class-restrictions
 *                 and in back of test-expression on query
 *                 argument list
 *  NOTES        : Expects top != NULL && top->args != NULL
 *************************************************************/
static int ParseQueryActionExpression(void *theEnv, core_expression_object *top, char *readSource, core_expression_object *insQuerySetVars, struct token *queryInputToken)
{
    core_expression_object *qaction, *tmpInsSetVars;
    int error;
    struct binding *oldBindList, *newBindList, *prev;

    error = FALSE;
    oldBindList = get_parsed_bindings(theEnv);
    set_parsed_bindings(theEnv, NULL);
    core_get_expression_data(theEnv)->break_context = TRUE;
    core_get_expression_data(theEnv)->return_context = core_get_expression_data(theEnv)->saved_contexts->rtn;

    qaction = group_actions(theEnv, readSource, queryInputToken, TRUE, NULL, FALSE);
    core_backup_pp_buffer(theEnv);
    core_backup_pp_buffer(theEnv);
    core_save_pp_buffer(theEnv, queryInputToken->pp);

    core_get_expression_data(theEnv)->break_context = FALSE;

    if( error == TRUE )
    {
        set_parsed_bindings(theEnv, oldBindList);
        core_return_expression(theEnv, top);
        return(FALSE);
    }

    if( qaction == NULL )
    {
        set_parsed_bindings(theEnv, oldBindList);
        error_syntax(theEnv, "instance-set query function");
        core_return_expression(theEnv, top);
        return(FALSE);
    }

    qaction->next_arg = top->args->next_arg;
    top->args->next_arg = qaction;
    newBindList = get_parsed_bindings(theEnv);
    prev = NULL;

    while( newBindList != NULL )
    {
        tmpInsSetVars = insQuerySetVars;

        while( tmpInsSetVars != NULL )
        {
            if( tmpInsSetVars->value == (void *)newBindList->name )
            {
                clear_parsed_bindings(theEnv);
                set_parsed_bindings(theEnv, oldBindList);
                error_print_id(theEnv, "INSQYPSR", 3, FALSE);
                print_router(theEnv, WERROR, "Cannot rebind instance-set member variable ");
                print_router(theEnv, WERROR, to_string(tmpInsSetVars->value));
                print_router(theEnv, WERROR, " in function ");
                print_router(theEnv, WERROR, to_string(core_get_expression_function_handle(top)));
                print_router(theEnv, WERROR, ".\n");
                core_return_expression(theEnv, top);
                return(FALSE);
            }

            tmpInsSetVars = tmpInsSetVars->next_arg;
        }

        prev = newBindList;
        newBindList = newBindList->next;
    }

    if( prev == NULL )
    {
        set_parsed_bindings(theEnv, oldBindList);
    }
    else
    {
        prev->next = oldBindList;
    }

    return(TRUE);
}

/***********************************************************************************
 *  NAME         : ReplaceInstanceVariables
 *  DESCRIPTION  : Replaces all references to instance-variables within an
 *                instance query-function with function calls to query-instance
 *                (which references the instance array at run-time)
 *  INPUTS       : 1) The instance-variable list
 *              2) A boolean expression containing variable references
 *              3) A flag indicating whether to allow slot references of the type
 *                 <instance-query-variable>:<slot-name> for direct slot access
 *                 or not
 *              4) Nesting depth of query functions
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : If a SCALAR_VARIABLE node is found and is on the list of instance
 *                variables, it is replaced with a query-instance function call.
 *  NOTES        : Other SCALAR_VARIABLE(S) are left alone for replacement by other
 *                parsers.  This implies that a user may use defgeneric,
 *                and defmessage-handler variables within a query-function
 *                where they do not conflict with instance-variable names.
 ***********************************************************************************/
static void ReplaceInstanceVariables(void *theEnv, core_expression_object *vlist, core_expression_object *bexp, int sdirect, int ndepth)
{
    core_expression_object *eptr;
    struct core_function_definition *rindx_func, *rslot_func;
    int posn;

    rindx_func = core_lookup_function(theEnv, "(query-instance)");
    rslot_func = core_lookup_function(theEnv, "(query-instance-slot)");

    while( bexp != NULL )
    {
        if( bexp->type == SCALAR_VARIABLE )
        {
            eptr = vlist;
            posn = 0;

            while((eptr != NULL) ? (eptr->value != bexp->value) : FALSE )
            {
                eptr = eptr->next_arg;
                posn++;
            }

            if( eptr != NULL )
            {
                bexp->type = FCALL;
                bexp->value = (void *)rindx_func;
                eptr = core_generate_constant(theEnv, INTEGER, (void *)store_long(theEnv, (long long)ndepth));
                eptr->next_arg = core_generate_constant(theEnv, INTEGER, (void *)store_long(theEnv, (long long)posn));
                bexp->args = eptr;
            }
            else if( sdirect == TRUE )
            {
                ReplaceSlotReference(theEnv, vlist, bexp, rslot_func, ndepth);
            }
        }

        if( bexp->args != NULL )
        {
            if( IsQueryFunction(bexp))
            {
                ReplaceInstanceVariables(theEnv, vlist, bexp->args, sdirect, ndepth + 1);
            }
            else
            {
                ReplaceInstanceVariables(theEnv, vlist, bexp->args, sdirect, ndepth);
            }
        }

        bexp = bexp->next_arg;
    }
}

/*************************************************************************
 *  NAME         : ReplaceSlotReference
 *  DESCRIPTION  : Replaces instance-set query function variable
 *                references of the form: <instance-variable>:<slot-name>
 *                with function calls to get these instance-slots at run
 *                time
 *  INPUTS       : 1) The instance-set variable list
 *              2) The expression containing the variable
 *              3) The address of the instance slot access function
 *              4) Nesting depth of query functions
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : If the variable is a slot reference, then it is replaced
 *                with the appropriate function-call.
 *  NOTES        : None
 *************************************************************************/
static void ReplaceSlotReference(void *theEnv, core_expression_object *vlist, core_expression_object *theExp, struct core_function_definition *func, int ndepth)
{
    size_t len;
    int posn, oldpp;
    size_t i;
    register char *str;
    core_expression_object *eptr;
    struct token itkn;

    str = to_string(theExp->value);
    len =  strlen(str);

    if( len < 3 )
    {
        return;
    }

    for( i = len - 2 ; i >= 1 ; i-- )
    {
        if((str[i] == INSTANCE_SLOT_REF) ? (i >= 1) : FALSE )
        {
            eptr = vlist;
            posn = 0;

            while( eptr && ((i != strlen(to_string(eptr->value))) ||
                            strncmp(to_string(eptr->value), str,
                                    (STD_SIZE)i)))
            {
                eptr = eptr->next_arg;
                posn++;
            }

            if( eptr != NULL )
            {
                open_string_source(theEnv, "query-var", str + i + 1, 0);
                oldpp = core_get_pp_buffer_status(theEnv);
                core_set_pp_buffer_status(theEnv, OFF);
                core_get_token(theEnv, "query-var", &itkn);
                core_set_pp_buffer_status(theEnv, oldpp);
                close_string_source(theEnv, "query-var");
                theExp->type = FCALL;
                theExp->value = (void *)func;
                theExp->args = core_generate_constant(theEnv, INTEGER, (void *)store_long(theEnv, (long long)ndepth));
                theExp->args->next_arg =
                    core_generate_constant(theEnv, INTEGER, (void *)store_long(theEnv, (long long)posn));
                theExp->args->next_arg->next_arg = core_generate_constant(theEnv, itkn.type, itkn.value);
                break;
            }
        }
    }
}

/********************************************************************
 *  NAME         : IsQueryFunction
 *  DESCRIPTION  : Determines if an expression is a query function call
 *  INPUTS       : The expression
 *  RETURNS      : TRUE if query function call, FALSE otherwise
 *  SIDE EFFECTS : None
 *  NOTES        : None
 ********************************************************************/
static int IsQueryFunction(core_expression_object *theExp)
{
    int (*fptr)(void);

    if( theExp->type != FCALL )
    {
        return(FALSE);
    }

    fptr = (int(*) (void))core_get_expression_function_ptr(theExp);

    if( fptr == (int(*) (void))AnyInstances )
    {
        return(TRUE);
    }

    if( fptr == (int(*) (void))QueryFindInstance )
    {
        return(TRUE);
    }

    if( fptr == (int(*) (void))QueryFindAllInstances )
    {
        return(TRUE);
    }

    if( fptr == (int(*) (void))QueryDoForInstance )
    {
        return(TRUE);
    }

    if( fptr == (int(*) (void))QueryDoForAllInstances )
    {
        return(TRUE);
    }

    if( fptr == (int(*) (void))DelayedQueryDoForAllInstances )
    {
        return(TRUE);
    }

    return(FALSE);
}

#endif

/***************************************************
 *  NAME         :
 *  DESCRIPTION  :
 *  INPUTS       :
 *  RETURNS      :
 *  SIDE EFFECTS :
 *  NOTES        :
 ***************************************************/
