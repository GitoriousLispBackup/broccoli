/* Purpose: Provides access routines for accessing arguments
 *   passed to user or system functions defined using the
 *   DefineFunction protocol.                                */

#define __CORE_ARGUMENTS_SOURCE__

#include "setup.h"

#include <stdio.h>
#define _STDIO_INCLUDED_
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "core_environment.h"
#include "core_functions.h"
#include "router.h"
#include "constraints_query.h"
#if OBJECT_SYSTEM
#include "funcs_instance.h"
#endif
#include "core_utilities.h"
#include "sysdep.h"

#include "core_arguments.h"

/**************************************
 * LOCAL FUNCTION PROTOTYPES
 ***************************************/

static void reportNonexistantArgError(void *, char *, char *, int);

/*******************************************************************
 * core_get_arg_at: Access function to retrieve the nth argument from
 *   a user or system function defined using the DefineFunction
 *   protocol. The argument retrieved can be of any type. The value
 *   and type of the argument are returned in a core_data_object
 *   structure provided by the calling function.
 ********************************************************************/
core_data_object_ptr core_get_arg_at(void *env, int argumentPosition, core_data_object_ptr ret)
{
    int count = 1;
    struct core_expression *argPtr;

    /*=====================================================
     * Find the appropriate argument in the argument list.
     *=====================================================*/

    for( argPtr = core_get_evaluation_data(env)->current_expression->args;
         (argPtr != NULL) && (count < argumentPosition);
         argPtr = argPtr->next_arg )
    {
        count++;
    }

    if( argPtr == NULL )
    {
        reportNonexistantArgError(env, "RtnUnknown",
                                  to_string(core_get_expression_function_handle(core_get_evaluation_data(env)->current_expression)),
                                  argumentPosition);
        core_set_halt_eval(env, TRUE);
        core_set_eval_error(env, TRUE);
        return(NULL);
    }

    /*=======================================
     * Return the value of the nth argument.
     *=======================================*/

    core_eval_expression(env, argPtr, ret);
    return(ret);
}

/**********************************************************
 * core_get_arg_count: Returns the length of the argument list
 *   for the function call currently being evaluated.
 ***********************************************************/
int core_get_arg_count(void *env)
{
    int count = 0;
    struct core_expression *argPtr;

    for( argPtr = core_get_evaluation_data(env)->current_expression->args;
         argPtr != NULL;
         argPtr = argPtr->next_arg )
    {
        count++;
    }

    return(count);
}

/***********************************************************************
 * core_check_arg_count: Given the expected number of arguments, determines
 *   if the function currently being evaluated has the correct number
 *   of arguments. Three types of argument checking are provided by
 *   this function: 1) The function has exactly the expected number of
 *   arguments; 2) The function has at least the expected number of
 *   arguments; 3) The function has at most the expected number of
 *   arguments. The number of arguments is returned if no error occurs,
 *   otherwise -1 is returned.
 ************************************************************************/
int core_check_arg_count(void *env, char *functionName, int countRelation, int expectedNumber)
{
    int numberOfArguments;

    /*==============================================
     * Get the number of arguments for the function
     * currently being evaluated.
     *==============================================*/

    numberOfArguments = core_get_arg_count(env);

    /*=========================================================
     * If the function satisfies expected number of arguments,
     * constraint, then return the number of arguments found.
     *=========================================================*/

    if( countRelation == EXACTLY )
    {
        if( numberOfArguments == expectedNumber )
        {
            return(numberOfArguments);
        }
    }
    else if( countRelation == AT_LEAST )
    {
        if( numberOfArguments >= expectedNumber )
        {
            return(numberOfArguments);
        }
    }
    else if( countRelation == NO_MORE_THAN )
    {
        if( numberOfArguments <= expectedNumber )
        {
            return(numberOfArguments);
        }
    }

    /*================================================
     * The correct number of arguments was not found.
     * Generate an error message and return -1.
     *================================================*/

    report_arg_count_error(env, functionName, countRelation, expectedNumber);

    core_set_halt_eval(env, TRUE);
    core_set_eval_error(env, TRUE);

    return(-1);
}

/***************************************************************
 * core_check_arg_range: Checks that the number of arguments passed
 *   to a function falls within a specified minimum and maximum
 *   range. The number of arguments passed to the function is
 *   returned if no error occurs, otherwise -1 is returned.
 ****************************************************************/
int core_check_arg_range(void *env, char *functionName, int min, int max)
{
    int numberOfArguments;

    numberOfArguments = core_get_arg_count(env);

    if((numberOfArguments < min) || (numberOfArguments > max))
    {
        error_print_id(env, ERROR_TAG_ARGUMENTS, 1, FALSE);
        print_router(env, WERROR, ERROR_MSG_ARG_RANGE);
        print_router(env, WERROR, functionName);
        print_router(env, WERROR, " ");
        core_print_long(env, WERROR, (long)min);
        print_router(env, WERROR, " to ");
        core_print_long(env, WERROR, (long)max);
        print_router(env, WERROR, " expected.\n");
        core_set_halt_eval(env, TRUE);
        core_set_eval_error(env, TRUE);
        return(-1);
    }

    return(numberOfArguments);
}

/************************************************************
 * core_check_arg_type: Retrieves the nth argument passed to the
 *   function call currently being evaluated and determines
 *   if it matches a specified type. Returns TRUE if the
 *   argument was successfully retrieved and is of the
 *   appropriate type, otherwise returns FALSE.
 *************************************************************/
int core_check_arg_type(void *env, char *functionName, int argumentPosition, int expectedType, core_data_object_ptr ret)
{
    /*========================
     * Retrieve the argument.
     *========================*/

    core_get_arg_at(env, argumentPosition, ret);

    if( core_get_evaluation_data(env)->eval_error )
    {
        return(FALSE);
    }

    /*========================================
     * If the argument's type exactly matches
     * the expected type, then return TRUE.
     *========================================*/

    if( ret->type == expectedType )
    {
        return(TRUE);
    }

    /*=============================================================
     * Some expected types encompass more than one primitive type.
     * If the argument's type matches one of the primitive types
     * encompassed by the expected type, then return TRUE.
     *=============================================================*/

    if((expectedType == INTEGER_OR_FLOAT) &&
       ((ret->type == INTEGER) || (ret->type == FLOAT)))
    {
        return(TRUE);
    }

    if((expectedType == ATOM_OR_STRING) &&
       ((ret->type == ATOM) || (ret->type == STRING)))
    {
        return(TRUE);
    }

#if OBJECT_SYSTEM

    if(((expectedType == ATOM_OR_STRING) || (expectedType == ATOM)) &&
       (ret->type == INSTANCE_NAME))
    {
        return(TRUE);
    }

    if((expectedType == INSTANCE_NAME) &&
       ((ret->type == INSTANCE_NAME) || (ret->type == ATOM)))
    {
        return(TRUE);
    }

    if((expectedType == INSTANCE_OR_INSTANCE_NAME) &&
       ((ret->type == INSTANCE_ADDRESS) ||
        (ret->type == INSTANCE_NAME) ||
        (ret->type == ATOM)))
    {
        return(TRUE);
    }

#endif

    /*===========================================================
     * If the expected type is float and the argument's type is
     * integer (or vice versa), then convert the argument's type
     * to match the expected type and then return TRUE.
     *===========================================================*/

    if((ret->type == INTEGER) && (expectedType == FLOAT))
    {
        ret->type = FLOAT;
        ret->value = (void *)store_double(env, (double)to_long(ret->value));
        return(TRUE);
    }

    if((ret->type == FLOAT) && (expectedType == INTEGER))
    {
        ret->type = INTEGER;
        ret->value = (void *)store_long(env, (long long)to_double(ret->value));
        return(TRUE);
    }

    /*=====================================================
     * The argument's type didn't match the expected type.
     * Print an error message and return FALSE.
     *=====================================================*/

    if( expectedType == FLOAT )
    {
        report_explicit_type_error(env, functionName, argumentPosition, TYPE_FLOAT_NAME);
    }
    else if( expectedType == INTEGER )
    {
        report_explicit_type_error(env, functionName, argumentPosition, "integer");
    }
    else if( expectedType == ATOM )
    {
        report_explicit_type_error(env, functionName, argumentPosition, "symbol");
    }
    else if( expectedType == STRING )
    {
        report_explicit_type_error(env, functionName, argumentPosition, "string");
    }
    else if( expectedType == LIST )
    {
        report_explicit_type_error(env, functionName, argumentPosition, TYPE_LIST_NAME);
    }
    else if( expectedType == INTEGER_OR_FLOAT )
    {
        report_explicit_type_error(env, functionName, argumentPosition, "integer or float");
    }
    else if( expectedType == ATOM_OR_STRING )
    {
        report_explicit_type_error(env, functionName, argumentPosition, "symbol or string");
    }

#if OBJECT_SYSTEM
    else if( expectedType == INSTANCE_NAME )
    {
        report_explicit_type_error(env, functionName, argumentPosition, "instance name");
    }
    else if( expectedType == INSTANCE_ADDRESS )
    {
        report_explicit_type_error(env, functionName, argumentPosition, "instance address");
    }
    else if( expectedType == INSTANCE_OR_INSTANCE_NAME )
    {
        report_explicit_type_error(env, functionName, argumentPosition, "instance address or instance name");
    }
#endif

    core_set_halt_eval(env, TRUE);
    core_set_eval_error(env, TRUE);

    return(FALSE);
}

/*****************************************************************
 * core_get_numeric_arg: Evaluates an expression to yield a numeric
 *  argument. This provides quicker retrieval than using some of
 *  the other argument access routines. The numeric argument is
 *  returned in a core_data_object supplied by the calling function.
 *  TRUE is returned if a numeric argument was successfully
 *  retrieved, otherwise FALSE is returned.
 ******************************************************************/
BOOLEAN core_get_numeric_arg(void *env, struct core_expression *arg, char *functionName, core_data_object *result, BOOLEAN convertToFloat, int whichArgument)
{
    unsigned short theType;
    void *val;

    /*==================================================================
     * Evaluate the expression (don't bother calling core_eval_expression
     * if the type is float or integer).
     *==================================================================*/

    switch( arg->type )
    {
    case FLOAT:
    case INTEGER:
        theType = arg->type;
        val = arg->value;
        break;

    default:
        core_eval_expression(env, arg, result);
        theType = result->type;
        val = result->value;
        break;
    }

    /*==========================================
     * If the argument is not float or integer,
     * print an error message and return FALSE.
     *==========================================*/

    if((theType != FLOAT) && (theType != INTEGER))
    {
        report_explicit_type_error(env, functionName, whichArgument, "integer or float");
        core_set_halt_eval(env, TRUE);
        core_set_eval_error(env, TRUE);
        result->type = INTEGER;
        result->value = (void *)store_long(env, 0LL);
        return(FALSE);
    }

    /*==========================================================
     * If the argument is an integer and the "convert to float"
     * flag is TRUE, then convert the integer to a float.
     *==========================================================*/

    if((convertToFloat) && (theType == INTEGER))
    {
        theType = FLOAT;
        val = (void *)store_double(env, (double)to_long(val));
    }

    /*============================================================
     * The numeric argument was successfully retrieved. Store the
     * argument in the user supplied core_data_object and return TRUE.
     *============================================================*/

    result->type = theType;
    result->value = val;

    return(TRUE);
}

/********************************************************************
 * core_lookup_logical_name: Retrieves the nth argument passed to the function
 *   call currently being evaluated and determines if it is a valid
 *   logical name. If valid, the logical name is returned, otherwise
 *   NULL is returned.
 *********************************************************************/
char *core_lookup_logical_name(void *env, int whichArgument, char *defaultLogicalName)
{
    char *logicalName;
    core_data_object result;

    core_get_arg_at(env, whichArgument, &result);

    if((core_get_type(result) == ATOM) ||
       (core_get_type(result) == STRING) ||
       (core_get_type(result) == INSTANCE_NAME))
    {
        logicalName = to_string(result.value);

        if((strcmp(logicalName, "t") == 0) || (strcmp(logicalName, "T") == 0))
        {
            logicalName = defaultLogicalName;
        }
    }
    else if( core_get_type(result) == FLOAT )
    {
        logicalName = to_string(store_atom(env, core_convert_float_to_string(env, core_convert_data_to_double(result))));
    }
    else if( core_get_type(result) == INTEGER )
    {
        logicalName = to_string(store_atom(env, core_conver_long_to_string(env, core_convert_data_to_long(result))));
    }
    else
    {
        logicalName = NULL;
    }

    return(logicalName);
}

/***********************************************************
 * core_get_filename: Retrieves the nth argument passed to the
 *   function call currently being evaluated and determines
 *   if it is a valid file name. If valid, the file name is
 *   returned, otherwise NULL is returned.
 ************************************************************/
char *core_get_filename(void *env, char *functionName, int whichArgument)
{
    core_data_object result;

    core_get_arg_at(env, whichArgument, &result);

    if((core_get_type(result) != STRING) && (core_get_type(result) != ATOM))
    {
        report_explicit_type_error(env, functionName, whichArgument, "file name");
        return(NULL);
    }

    return(core_convert_data_to_string(result));
}

/*****************************************************************
 * report_file_open_error: Generalized error message for opening files.
 ******************************************************************/
void report_file_open_error(void *env, char *functionName, char *fileName)
{
    error_print_id(env, ERROR_TAG_ARGUMENTS, 2, FALSE);
    print_router(env, WERROR, functionName);
    print_router(env, WERROR, ERROR_MSG_FILE_OPEN);
    print_router(env, WERROR, fileName);
    print_router(env, WERROR, ".\n");
}

#if DEFMODULE_CONSTRUCT

/***********************************************************
 * core_get_module_name: Retrieves the nth argument passed to the
 *   function call currently being evaluated and determines
 *   if it is a valid module name. If valid, the module
 *   name is returned or NULL is returned to indicate all
 *   modules.
 ************************************************************/
struct module_definition *core_get_module_name(void *env, char *functionName, int whichArgument, int *error)
{
    core_data_object result;
    struct module_definition *module_def;

    *error = FALSE;

    /*========================
     * Retrieve the argument.
     *========================*/

    core_get_arg_at(env, whichArgument, &result);

    /*=================================
     * A module name must be a symbol.
     *=================================*/

    if( core_get_type(result) != ATOM )
    {
        report_explicit_type_error(env, functionName, whichArgument, "defmodule name");
        *error = TRUE;
        return(NULL);
    }

    /*=======================================
     * Check to see that the symbol actually
     * corresponds to a defined module.
     *=======================================*/

    if((module_def = (struct module_definition *)lookup_module(env, core_convert_data_to_string(result))) == NULL )
    {
        if( strcmp("*", core_convert_data_to_string(result)) != 0 )
        {
            report_explicit_type_error(env, functionName, whichArgument, "defmodule name");
            *error = TRUE;
        }

        return(NULL);
    }

    /*=================================
     * Return a pointer to the module.
     *=================================*/

    return(module_def);
}

#endif

/***************************************************************
 * core_get_atom: Retrieves the 1st argument passed to the
 *   function call currently being evaluated and determines if
 *   it is a valid name for a construct. Also checks that the
 *   function is only passed a single argument. This routine
 *   is used by functions to retrieve the construct name on
 *   which to operate.
 ****************************************************************/
char *core_get_atom(void *env, char *functionName, char *constructType)
{
    core_data_object result;

    if( core_get_arg_count(env) != 1 )
    {
        report_arg_count_error(env, functionName, EXACTLY, 1);
        return(NULL);
    }

    core_get_arg_at(env, 1, &result);

    if( core_get_type(result) != ATOM )
    {
        report_explicit_type_error(env, functionName, 1, constructType);
        return(NULL);
    }

    return(core_convert_data_to_string(result));
}

/********************************************************
 * report_arg_count_error: Prints the error message for an
 *   incorrect number of arguments passed to a function.
 *********************************************************/
void report_arg_count_error(void *env, char *functionName, int countRelation, int expectedNumber)
{
    error_print_id(env, ERROR_TAG_ARGUMENTS, 4, FALSE);
    print_router(env, WERROR, functionName);

    if( countRelation == EXACTLY )
    {
        print_router(env, WERROR, ERROR_MSG_EXACTLY);
    }
    else if( countRelation == AT_LEAST )
    {
        print_router(env, WERROR, ERROR_MSG_AT_LEAST);
    }
    else if( countRelation == NO_MORE_THAN )
    {
        print_router(env, WERROR, ERROR_MSG_NO_MORE);
    }
    else
    {
        print_router(env, WERROR, ERROR_MSG_GENERAL);
    }

    core_print_long(env, WERROR, (long int)expectedNumber);
    print_router(env, WERROR, " args.\n");
}

/************************************************************
 *  NAME         : core_check_function_arg_count
 *  DESCRIPTION  : Checks the number of arguments against
 *                 the system function restriction list
 *  INPUTS       : 1) Name of the calling function
 *                 2) The restriction list can be NULL
 *                 3) The number of arguments
 *  RETURNS      : TRUE if OK, FALSE otherwise
 *  SIDE EFFECTS : eval_error set on errrors
 *  NOTES        : Used to check generic function implicit
 *                 method (system function) calls and system
 *                 function calls which have the sequence
 *                 expansion operator in their argument list
 *************************************************************/
BOOLEAN core_check_function_arg_count(void *env, char *functionName, char *restrictions, int argumentCount)
{
    register int minArguments, maxArguments;
    char theChar[2];

    theChar[0] = '0';
    theChar[1] = EOS;

    /*=====================================================
     * If there are no restrictions, then there is no need
     * to check for the correct number of arguments.
     *=====================================================*/

    if( restrictions == NULL )
    {
        return(TRUE);
    }

    /*===========================================
     * Determine the minimum number of arguments
     * required by the function.
     *===========================================*/

    if( isdigit(restrictions[0]))
    {
        theChar[0] = restrictions[0];
        minArguments = atoi(theChar);
    }
    else
    {
        minArguments = -1;
    }

    /*===========================================
     * Determine the maximum number of arguments
     * required by the function.
     *===========================================*/

    if( isdigit(restrictions[1]))
    {
        theChar[0] = restrictions[1];
        maxArguments = atoi(theChar);
    }
    else
    {
        maxArguments = 10000;
    }

    /*==============================================
     * If the function expects exactly N arguments,
     * then check to see if there are N arguments.
     *==============================================*/

    if( minArguments == maxArguments )
    {
        if( argumentCount != minArguments )
        {
            report_arg_count_error(env, functionName, EXACTLY, minArguments);
            core_set_eval_error(env, TRUE);
            return(FALSE);
        }

        return(TRUE);
    }

    /*==================================
     * Check to see if there were fewer
     * arguments passed than expected.
     *==================================*/

    if( argumentCount < minArguments )
    {
        report_arg_count_error(env, functionName, AT_LEAST, minArguments);
        core_set_eval_error(env, TRUE);
        return(FALSE);
    }

    /*=================================
     * Check to see if there were more
     * arguments passed than expected.
     *=================================*/

    if( argumentCount > maxArguments )
    {
        report_arg_count_error(env, functionName, NO_MORE_THAN, maxArguments);
        core_set_eval_error(env, TRUE);
        return(FALSE);
    }

    /*===============================
     * The number of arguments falls
     * within the expected range.
     *===============================*/

    return(TRUE);
}

/******************************************************************
 * report_explicit_type_error: Prints the error message for the wrong type
 *   of argument passed to a user or system defined function. The
 *   expected type is passed as a string to this function.
 *******************************************************************/
void report_explicit_type_error(void *env, char *functionName, int whichArg, char *expectedType)
{
    error_print_id(env, ERROR_TAG_ARGUMENTS, 5, FALSE);
    print_router(env, WERROR, functionName);
    print_router(env, WERROR, ERROR_MSG_ARG_TYPE);
    core_print_long(env, WERROR, (long int)whichArg);
    print_router(env, WERROR, ", expected ");
    print_router(env, WERROR, expectedType);
    print_router(env, WERROR, ".\n");
}

/*************************************************************
 * report_implicit_type_error: Prints the error message for the wrong
 *   type of argument passed to a user or system defined
 *   function. The expected type is derived by examining the
 *   function's argument restriction list.
 **************************************************************/
void report_implicit_type_error(void *env, char *functionName, int whichArg)
{
    struct core_function_definition *func;
    char *theType;

    func = core_lookup_function(env, functionName);

    if( func == NULL )
    {
        return;
    }

    theType = core_arg_type_of(core_get_function_arg_restriction(func, whichArg));

    report_explicit_type_error(env, functionName, whichArg, theType);
}

/***************************************************
 * report_logical_name_lookup_error: Generic error message
 *   for illegal logical names.
 ****************************************************/
void report_logical_name_lookup_error(void *env, char *func)
{
    error_print_id(env, "IO SYSTEM", 1, FALSE);
    print_router(env, WERROR, "Illegal logical name used for ");
    print_router(env, WERROR, func);
    print_router(env, WERROR, " function.\n");
}

/**********************************
 * LOCAL FUNCTIONS
 *********************************/

/*************************************************************************
 * reportNonexistantArgError: Prints the error message for a nonexistant argument.
 **************************************************************************/
static void reportNonexistantArgError(void *env, char *accessFunction, char *functionName, int argumentPosition)
{
    error_print_id(env, ERROR_TAG_ARGUMENTS, 3, FALSE);
    print_router(env, WERROR, accessFunction);
    print_router(env, WERROR, ERROR_MSG_NO_EXISTS);
    print_router(env, WERROR, functionName);
    print_router(env, WERROR, " for arg == ");
    core_print_long(env, WERROR, (long int)argumentPosition);
}
