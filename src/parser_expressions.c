/* Purpose: Provides routines for parsing expressions.       */

#define __PARSER_EXPRESSIONS_SOURCE__

#include "setup.h"

#include <stdio.h>
#define _STDIO_INCLUDED_
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "constant.h"
#include "core_environment.h"
#include "router.h"
#include "router_string.h"
#include "core_scanner.h"
#include "core_memory.h"
#include "core_arguments.h"
#include "core_utilities.h"
#include "constraints_query.h"
#include "core_functions.h"
#include "parser_expressions.h"
#include "modules_query.h"
#include "funcs_flow_control.h"


#if DEFGENERIC_CONSTRUCT
#include "generics_kernel.h"
#endif

#if DEFFUNCTION_CONSTRUCT
#include "funcs_function.h"
#endif

static BOOLEAN _replace_list_args(void *, struct core_expression *, struct core_expression *, void *, void *);
static int     _validate_expression_against_restrictions(void *, struct core_expression *, char *, char *);

/******************************************************
 * parse_virgin_function: Parses a function. Assumes that the
 *   opening left parenthesis has already been parsed.
 *******************************************************/
struct core_expression *parse_virgin_function(void *env, char *logicalName)
{
    struct token theToken;
    struct core_expression *top;

    /*========================
     * Get the function name.
     *========================*/

    core_get_token(env, logicalName, &theToken);

    if( theToken.type != ATOM )
    {
        error_print_id(env, ERROR_TAG_EXPRESSION_PARSER, 1, TRUE);
        print_router(env, WERROR, ERROR_MSG_INVALID_FUNC_NAME);
        print_router(env, WERROR, ".\n");
        return(NULL);
    }

    /*=================================
     * Parse the rest of the function.
     *=================================*/

    top = parse_function_body(env, logicalName, to_string(theToken.value));
    return(top);
}

/***************************************************
 * parse_function_body: Parses a function. Assumes that
 *   the opening left parenthesis and function name
 *   have already been parsed.
 ****************************************************/
struct core_expression *parse_function_body(void *env, char *logicalName, char *name)
{
    struct core_function_definition *func;
    struct core_expression *top;
    int moduleSpecified = FALSE;
    unsigned position;
    struct atom_hash_node *moduleName = NULL, *constructName = NULL;

#if DEFGENERIC_CONSTRUCT
    void *gfunc;
#endif
#if DEFFUNCTION_CONSTRUCT
    void *dptr;
#endif

    /*=========================================================
     * Module specification cannot be used in a function call.
     *=========================================================*/

    if((position = find_module_separator(name)) != FALSE )
    {
        moduleName = garner_module_name(env, position, name);
        constructName = garner_construct_name(env, position, name);
        moduleSpecified = TRUE;
    }

    /*================================
     * Has the function been defined?
     *================================*/

    func = core_lookup_function(env, name);

#if DEFGENERIC_CONSTRUCT

    if( moduleSpecified )
    {
        if( is_construct_exported(env, "defgeneric", parent_module, construct_name) ||
            (get_current_module(env) == lookup_module(env, to_string(parent_module))))
        {
            gfunc = (void *)EnvFindDefgeneric(env, name);
        }
        else
        {
            gfunc = NULL;
        }
    }
    else
    {
        gfunc = (void *)LookupDefgenericInScope(env, name);
    }

#endif

#if DEFFUNCTION_CONSTRUCT

    if((func == NULL)
#if DEFGENERIC_CONSTRUCT
       && (gfunc == NULL)
#endif
       )
    {
        if( moduleSpecified )
        {
            if( is_construct_exported(env, FUNC_NAME_CREATE_FUNC, moduleName, constructName) ||
                (get_current_module(env) == lookup_module(env, to_string(moduleName))))
            {
                dptr = (void *)lookup_function(env, name);
            }
            else
            {
                dptr = NULL;
            }
        }
        else
        {
            dptr = (void *)lookup_function_in_scope(env, name);
        }
    }
    else
    {
        dptr = NULL;
    }

#endif

    /*=============================
     * Define top level structure.
     *=============================*/

#if DEFFUNCTION_CONSTRUCT

    if( dptr != NULL )
    {
        top = core_generate_constant(env, PCALL, dptr);
    }
    else
#endif
#if DEFGENERIC_CONSTRUCT

    if( gfunc != NULL )
    {
        top = core_generate_constant(env, GCALL, gfunc);
    }
    else
#endif

    if( func != NULL )
    {
        top = core_generate_constant(env, FCALL, func);
    }
    else
    {
        error_print_id(env, ERROR_TAG_EXPRESSION_PARSER, 3, TRUE);
        print_router(env, WERROR, ERROR_MSG_MISSIN_FUNC_DECL);
        print_router(env, WERROR, name);
        print_router(env, WERROR, ".\n");
        return(NULL);
    }

    /*=======================================================
     * Check to see if function has its own parsing routine.
     *=======================================================*/

    push_break_contexts(env);
    core_get_expression_data(env)->return_context = FALSE;
    core_get_expression_data(env)->break_context = FALSE;

#if DEFGENERIC_CONSTRUCT || DEFFUNCTION_CONSTRUCT

    if( top->type == FCALL )
#endif
    {
        if( func->parser != NULL )
        {
            top = (*func->parser)(env, top, logicalName);
            pop_break_contexts(env);

            if( top == NULL ) {return(NULL);}

            if( _replace_list_args(env, top->args, top, core_lookup_function(env, FUNC_NAME_EXPAND_META),
                                   core_lookup_function(env, FUNC_NAME_EXPAND)))
            {
                core_return_expression(env, top);
                return(NULL);
            }

            return(top);
        }
    }

    /*========================================
     * Default parsing routine for functions.
     *========================================*/

    top = collect_args(env, top, logicalName);
    pop_break_contexts(env);

    if( top == NULL ) {return(NULL);}

    if( _replace_list_args(env, top->args, top, core_lookup_function(env, FUNC_NAME_EXPAND_META),
                           core_lookup_function(env, FUNC_NAME_EXPAND)))
    {
        core_return_expression(env, top);
        return(NULL);
    }

    /*============================================================
     * If the function call uses the sequence expansion operator,
     * its arguments cannot be checked until runtime.
     *============================================================*/

    if( top->value == (void *)core_lookup_function(env, FUNC_NAME_EXPAND_META))
    {
        return(top);
    }

    /*============================
     * Check for argument errors.
     *============================*/

    if((top->type == FCALL) && get_constraints(env))
    {
        if( _validate_expression_against_restrictions(env, top, func->restrictions, name))
        {
            core_return_expression(env, top);
            return(NULL);
        }
    }

#if DEFFUNCTION_CONSTRUCT
    else if( top->type == PCALL )
    {
        if( verify_function_call(env, top->value, core_count_args(top->args)) == FALSE )
        {
            core_return_expression(env, top);
            return(NULL);
        }
    }
#endif

    /*========================
     * Return the expression.
     *========================*/

    return(top);
}

/***********************************************************************
 * NAME         : replace_list_args
 * DESCRIPTION  : Replaces function calls which have list
 *               references as arguments into a call to a
 *               special function which expands the list
 *               into single arguments at run-time.
 *             List references which are not function
 *               arguments are errors
 * INPUTS       : 1) The expression
 *             2) The current function call
 *             3) The address of the internal H/L function
 *                (expansion-call)
 *             4) The address of the H/L function expand$
 * RETURNS      : FALSE if OK, TRUE on errors
 * SIDE EFFECTS : Function call expressions modified, if necessary
 * NOTES        : Function calls which truly want a list
 *               to be passed need use only a single-field
 *               refernce (i.e. ? instead of $? - the $ is
 *               being treated as a special expansion operator)
 **********************************************************************/
BOOLEAN _replace_list_args(void *env, core_expression_object *actions, core_expression_object *fcallexp, void *expcall, void *expmult)
{
    core_expression_object *theExp;

    while( actions != NULL )
    {
        if((core_get_expression_data(env)->sequential_operation == FALSE) && (actions->type == LIST_VARIABLE))
        {
            actions->type = SCALAR_VARIABLE;
        }

        if((actions->type == LIST_VARIABLE) || (actions->value == expmult))
        {
            if((fcallexp->type != FCALL) ? FALSE : (((struct core_function_definition *)fcallexp->value)->sequential_usage_allowed == FALSE))
            {
                error_print_id(env, ERROR_TAG_EXPRESSION_PARSER, 4, FALSE);
                print_router(env, WERROR, ERROR_MSG_LIST_INVALID_ARG);
                print_router(env, WERROR, to_string(((struct core_function_definition *)fcallexp->value)->function_handle));
                print_router(env, WERROR, ".\n");
                return(TRUE);
            }

            if( fcallexp->value != expcall )
            {
                theExp = core_generate_constant(env, fcallexp->type, fcallexp->value);
                theExp->args = fcallexp->args;
                theExp->next_arg = NULL;
                fcallexp->type = FCALL;
                fcallexp->value = expcall;
                fcallexp->args = theExp;
            }

            if( actions->value != expmult )
            {
                theExp = core_generate_constant(env, SCALAR_VARIABLE, actions->value);
                actions->args = theExp;
                actions->type = FCALL;
                actions->value = expmult;
            }
        }

        if( actions->args != NULL )
        {
            if((actions->type == GCALL) ||
               (actions->type == PCALL) ||
               (actions->type == FCALL))
            {
                theExp = actions;
            }
            else
            {
                theExp = fcallexp;
            }

            if( _replace_list_args(env, actions->args, theExp, expcall, expmult))
            {
                return(TRUE);
            }
        }

        actions = actions->next_arg;
    }

    return(FALSE);
}

/************************************************
 * push_break_contexts: Saves the current context
 *   for the break/return functions.
 *************************************************/
void push_break_contexts(void *env)
{
    SAVED_CONTEXTS *svtmp;

    svtmp = core_mem_get_struct(env, saved_contexts);
    svtmp->rtn = core_get_expression_data(env)->return_context;
    svtmp->brk = core_get_expression_data(env)->break_context;
    svtmp->nxt = core_get_expression_data(env)->saved_contexts;
    core_get_expression_data(env)->saved_contexts = svtmp;
}

/**************************************************
 * pop_break_contexts: Restores the current context
 *   for the break/return functions.
 ***************************************************/
void pop_break_contexts(void *env)
{
    SAVED_CONTEXTS *svtmp;

    core_get_expression_data(env)->return_context = core_get_expression_data(env)->saved_contexts->rtn;
    core_get_expression_data(env)->break_context = core_get_expression_data(env)->saved_contexts->brk;
    svtmp = core_get_expression_data(env)->saved_contexts;
    core_get_expression_data(env)->saved_contexts = core_get_expression_data(env)->saved_contexts->nxt;
    core_mem_return_struct(env, saved_contexts, svtmp);
}

/****************************************************************
 * validate_expression_against_restrictions: Compares the arguments to
 *   a function to the set of restrictions for that function to
 *   determine if any incompatibilities exist. If so, the value
 *   TRUE is returned, otherwise FALSE is returned.
 *****************************************************************/
int _validate_expression_against_restrictions(void *env, struct core_expression *expr, char *restrictions, char *functionName)
{
    char theChar[2];
    int i = 0, j = 1;
    int number1, number2;
    int argCount;
    char defaultRestriction, argRestriction;
    struct core_expression *argPtr;
    int theRestriction;

    theChar[0] = '0';
    theChar[1] = '\0';

    /*============================================
     * If there are no restrictions, then there's
     * no need to check the function.
     *============================================*/

    if( restrictions == NULL )
    {
        return(FALSE);
    }

    /*=========================================
     * Count the number of function arguments.
     *=========================================*/

    argCount = core_count_args(expr->args);

    /*======================================
     * Get the minimum number of arguments.
     *======================================*/

    theChar[0] = restrictions[i++];

    if( isdigit(theChar[0]))
    {
        number1 = atoi(theChar);
    }
    else if( theChar[0] == '*' )
    {
        number1 = -1;
    }
    else
    {
        return(FALSE);
    }

    /*======================================
     * Get the maximum number of arguments.
     *======================================*/

    theChar[0] = restrictions[i++];

    if( isdigit(theChar[0]))
    {
        number2 = atoi(theChar);
    }
    else if( theChar[0] == '*' )
    {
        number2 = 10000;
    }
    else
    {
        return(FALSE);
    }

    /*============================================
     * Check for the correct number of arguments.
     *============================================*/

    if( number1 == number2 )
    {
        if( argCount != number1 )
        {
            report_arg_count_error(env, functionName, EXACTLY, number1);
            return(TRUE);
        }
    }
    else if( argCount < number1 )
    {
        report_arg_count_error(env, functionName, AT_LEAST, number1);
        return(TRUE);
    }
    else if( argCount > number2 )
    {
        report_arg_count_error(env, functionName, NO_MORE_THAN, number2);
        return(TRUE);
    }

    /*=======================================
     * Check for the default argument types.
     *=======================================*/

    defaultRestriction = restrictions[i];

    if( defaultRestriction == '\0' )
    {
        defaultRestriction = 'u';
    }
    else if( defaultRestriction == '*' )
    {
        defaultRestriction = 'u';
        i++;
    }
    else
    {
        i++;
    }

    /*======================
     * Check each argument.
     *======================*/

    for( argPtr = expr->args;
         argPtr != NULL;
         argPtr = argPtr->next_arg )
    {
        argRestriction = restrictions[i];

        if( argRestriction == '\0' )
        {
            argRestriction = defaultRestriction;
        }
        else
        {
            i++;
        }

        if( argRestriction != '*' )
        {
            theRestriction = (int)argRestriction;
        }
        else
        {
            theRestriction = (int)defaultRestriction;
        }

        if( core_check_against_restrictions(env, argPtr, theRestriction))
        {
            report_explicit_type_error(env, functionName, j, core_arg_type_of(theRestriction));
            return(TRUE);
        }

        j++;
    }

    return(FALSE);
}

/******************************************************
 * collect_args: Parses and groups together all of
 *   the arguments for a function call expression.
 *******************************************************/
struct core_expression *collect_args(void *env, struct core_expression *top, char *logicalName)
{
    int errorFlag;
    struct core_expression *lastOne, *nextOne;

    /*========================================
     * Default parsing routine for functions.
     *========================================*/

    lastOne = NULL;

    while( TRUE )
    {
        core_save_pp_buffer(env, " ");

        errorFlag = FALSE;
        nextOne = parse_args(env, logicalName, &errorFlag);

        if( errorFlag == TRUE )
        {
            core_return_expression(env, top);
            return(NULL);
        }

        if( nextOne == NULL )
        {
            core_backup_pp_buffer(env);
            core_backup_pp_buffer(env);
            core_save_pp_buffer(env, ")");
            return(top);
        }

        if( lastOne == NULL )
        {
            top->args = nextOne;
        }
        else
        {
            lastOne->next_arg = nextOne;
        }

        lastOne = nextOne;
    }
}

/*******************************************
 * parse_args: Parses an argument within
 *   a function call expression.
 ********************************************/
struct core_expression *parse_args(void *env, char *logicalName, int *errorFlag)
{
    struct core_expression *top;
    struct token theToken;

    /*===============
     * Grab a token.
     *===============*/

    core_get_token(env, logicalName, &theToken);

    /*============================
     * ')' counts as no argument.
     *============================*/

    if( theToken.type == RPAREN )
    {
        return(NULL);
    }

    /*================================
     * Parse constants and variables.
     *================================*/

    if((theToken.type == SCALAR_VARIABLE) || (theToken.type == LIST_VARIABLE) ||
       (theToken.type == ATOM) || (theToken.type == STRING) ||
#if OBJECT_SYSTEM
       (theToken.type == INSTANCE_NAME) ||
#endif
       (theToken.type == FLOAT) || (theToken.type == INTEGER))
    {
        return(core_generate_constant(env, theToken.type, theToken.value));
    }

    /*======================
     * Parse function call.
     *======================*/

    if( theToken.type != LPAREN )
    {
        error_print_id(env, ERROR_TAG_EXPRESSION_PARSER, 2, TRUE);
        print_router(env, WERROR, ERROR_MSG_INVALID_ARG);
        print_router(env, WERROR, ".\n");
        *errorFlag = TRUE;
        return(NULL);
    }

    top = parse_virgin_function(env, logicalName);

    if( top == NULL ) {*errorFlag = TRUE;}

    return(top);
}

/***********************************************************
 * parse_atom_or_expression: Parses an expression which may be
 *   a function call, atomic value (string, symbol, etc.),
 *   or variable (local or global).
 ************************************************************/
struct core_expression *parse_atom_or_expression(void *env, char *logicalName, struct token *useToken)
{
    struct token theToken, *thisToken;
    struct core_expression *rv;

    if( useToken == NULL )
    {
        thisToken = &theToken;
        core_get_token(env, logicalName, thisToken);
    }
    else {thisToken = useToken;}

    if((thisToken->type == ATOM) || (thisToken->type == STRING) ||
       (thisToken->type == INTEGER) || (thisToken->type == FLOAT) ||
#if OBJECT_SYSTEM
       (thisToken->type == INSTANCE_NAME) ||
#endif
       (thisToken->type == SCALAR_VARIABLE) || (thisToken->type == LIST_VARIABLE))
    {
        rv = core_generate_constant(env, thisToken->type, thisToken->value);
    }
    else if( thisToken->type == LPAREN )
    {
        rv = parse_virgin_function(env, logicalName);

        if( rv == NULL ) {return(NULL);}
    }
    else
    {
        error_print_id(env, ERROR_TAG_EXPRESSION_PARSER, 2, TRUE);
        print_router(env, WERROR, ERROR_MSG_INVALID_ARG);
        return(NULL);
    }

    return(rv);
}

/********************************************
 * group_actions: Groups together a series of
 *   actions within a progn expression. Used
 *   for example to parse the RHS of a rule.
 *********************************************/
struct core_expression *group_actions(void *env, char *logicalName, struct token *theToken, int readFirstToken, char *endWord, int functionNameParsed)
{
    struct core_expression *top, *nextOne, *lastOne = NULL;

    /*=============================
     * Create the enclosing progn.
     *=============================*/

    top = core_generate_constant(env, FCALL, core_lookup_function(env, FUNC_NAME_PROGN));

    /*========================================================
     * Continue until all appropriate commands are processed.
     *========================================================*/

    while( TRUE )
    {
        /*================================================
         * Skip reading in the token if this is the first
         * pass and the initial token was already read
         * before calling this function.
         *================================================*/

        if( readFirstToken )
        {
            core_get_token(env, logicalName, theToken);
        }
        else
        {
            readFirstToken = TRUE;
        }

        /*=================================================
         * Look to see if a type_symbol.has terminated the list
         * of actions (such as "else" in an if function).
         *=================================================*/

        if((theToken->type == ATOM) &&
           (endWord != NULL) &&
           (!functionNameParsed))
        {
            if( strcmp(to_string(theToken->value), endWord) == 0 )
            {
                return(top);
            }
        }

        /*====================================
         * Process a function if the function
         * name has already been read.
         *====================================*/

        if( functionNameParsed )
        {
            nextOne = parse_function_body(env, logicalName, to_string(theToken->value));
            functionNameParsed = FALSE;
        }

        /*========================================
         * Process a constant or global variable.
         *========================================*/

        else if((theToken->type == ATOM) || (theToken->type == STRING) ||
                (theToken->type == INTEGER) || (theToken->type == FLOAT) ||
#if OBJECT_SYSTEM
                (theToken->type == INSTANCE_NAME) ||
#endif
                (theToken->type == SCALAR_VARIABLE) || (theToken->type == LIST_VARIABLE))
        {
            nextOne = core_generate_constant(env, theToken->type, theToken->value);
        }

        /*=============================
         * Otherwise parse a function.
         *=============================*/

        else if( theToken->type == LPAREN )
        {
            nextOne = parse_virgin_function(env, logicalName);
        }

        /*======================================
         * Otherwise replace sequence expansion
         * variables and return the expression.
         *======================================*/

        else
        {
            if( _replace_list_args(env, top, NULL,
                                   core_lookup_function(env, FUNC_NAME_EXPAND_META),
                                   core_lookup_function(env, FUNC_NAME_EXPAND)))
            {
                core_return_expression(env, top);
                return(NULL);
            }

            return(top);
        }

        /*===========================
         * Add the new action to the
         * list of progn arguments.
         *===========================*/

        if( nextOne == NULL )
        {
            theToken->type = UNKNOWN_VALUE;
            core_return_expression(env, top);
            return(NULL);
        }

        if( lastOne == NULL )
        {
            top->args = nextOne;
        }
        else
        {
            lastOne->next_arg = nextOne;
        }

        lastOne = nextOne;

        core_inject_ws_into_pp_buffer(env);
    }
}

/******************************************
 * parse_constants: Parses a string
 *    into a set of constant expressions.
 *******************************************/
core_expression_object *parse_constants(void *env, char *argstr, int *error)
{
    core_expression_object *top = NULL, *bot = NULL, *tmp;
    char *router = "***FNXARGS***";
    struct token tkn;

    *error = FALSE;

    if( argstr == NULL )
    {
        return(NULL);
    }

    /*=====================================
     * Open the string as an input source.
     *=====================================*/

    if( open_string_source(env, router, argstr, 0) == 0 )
    {
        error_print_id(env, ERROR_TAG_EXPRESSION_PARSER, 6, FALSE);
        print_router(env, WERROR, ERROR_MSG_EXT_CALL_ARGS);
        print_router(env, WERROR, ".\n");
        *error = TRUE;
        return(NULL);
    }

    /*======================
     * Parse the constants.
     *======================*/

    core_get_token(env, router, &tkn);

    while( tkn.type != STOP )
    {
        if((tkn.type != ATOM) && (tkn.type != STRING) &&
           (tkn.type != FLOAT) && (tkn.type != INTEGER) &&
           (tkn.type != INSTANCE_NAME))
        {
            error_print_id(env, ERROR_TAG_EXPRESSION_PARSER, 7, FALSE);
            print_router(env, WERROR, ERROR_MSG_EXT_CALL_CONST);
            print_router(env, WERROR, ".\n");
            core_return_expression(env, top);
            *error = TRUE;
            close_string_source(env, router);
            return(NULL);
        }

        tmp = core_generate_constant(env, tkn.type, tkn.value);

        if( top == NULL )
        {
            top = tmp;
        }
        else
        {
            bot->next_arg = tmp;
        }

        bot = tmp;
        core_get_token(env, router, &tkn);
    }

    /*================================
     * Close the string input source.
     *================================*/

    close_string_source(env, router);

    /*=======================
     * Return the arguments.
     *=======================*/

    return(top);
}

/********************************************
 * strip_frivolous_progn:
 *********************************************/
struct core_expression *strip_frivolous_progn(void *env, struct core_expression *expr)
{
    struct core_function_definition *fptr;
    struct core_expression *temp;

    if( expr == NULL ) {return(expr);}

    if( expr->type != FCALL ) {return(expr);}

    fptr = (struct core_function_definition *)expr->value;

    if( fptr->functionPointer != VOID_FN broccoli_progn )
    {
        return(expr);
    }

    if((expr->args != NULL) &&
       (expr->args->next_arg == NULL))
    {
        temp = expr;
        expr = expr->args;
        temp->args = NULL;
        temp->next_arg = NULL;
        core_return_expression(env, temp);
    }

    return(expr);
}
