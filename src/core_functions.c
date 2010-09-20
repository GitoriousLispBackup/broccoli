/* Purpose: Routines for adding new user or system defined
 *   functions.                                              */

#define __CORE_FUNCTIONS_SOURCE__

#include "setup.h"

#include <ctype.h>
#include <stdlib.h>

#include "constant.h"
#include "core_environment.h"
#include "router.h"
#include "core_memory.h"
#include "core_evaluation.h"

#include "core_functions.h"

/**************************************
 * LOCAL FUNCTION PROTOTYPES
 ***************************************/

static void _add_hash_function(void *, struct core_function_definition *);
static void _initialize_function_hash_table(void *);
static void _delete_function_data(void *);
static int  _remove_hash_function(void *, struct core_function_definition *);
static int _define_function(void *, char *, int, int(*) (void *), char *, char *, BOOLEAN, void *);

/********************************************************
 * core_init_function_data: Allocates environment
 *    data for external functions.
 *********************************************************/
void core_init_function_data(void *env)
{
    core_allocate_environment_data(env, EXTERNAL_FUNCTION_DATA_INDEX, sizeof(struct core_function_data), _delete_function_data);
}

/**********************************************************
 * DeallocateExternalFunctionData: Deallocates environment
 *    data for external functions.
 ***********************************************************/
static void _delete_function_data(void *env)
{
    struct core_function_hash *fhPtr, *nextFHPtr;
    int i;

    struct core_function_definition *tmpPtr, *nextPtr;

    tmpPtr = core_get_function_data(env)->ListOfFunctions;

    while( tmpPtr != NULL )
    {
        nextPtr = tmpPtr->next;
        core_mem_return_struct(env, core_function_definition, tmpPtr);
        tmpPtr = nextPtr;
    }


    if( core_get_function_data(env)->FunctionHashtable == NULL )
    {
        return;
    }

    for( i = 0; i < FUNCTION_HASH_SZ; i++ )
    {
        fhPtr = core_get_function_data(env)->FunctionHashtable[i];

        while( fhPtr != NULL )
        {
            nextFHPtr = fhPtr->next;
            core_mem_return_struct(env, core_function_hash, fhPtr);
            fhPtr = nextFHPtr;
        }
    }

    core_mem_free(env, core_get_function_data(env)->FunctionHashtable,
                  (int)sizeof(struct core_function_hash *) *FUNCTION_HASH_SZ);
}

/************************************************************
 * core_define_function: Used to define a system or user external
 *   function so that the KB can access it.
 *************************************************************/
int core_define_function(void *env, char *name, int returnType, int (*pointer)(void *), char *actualName, char *restrictions)
{
    return(_define_function(env, name, returnType, pointer, actualName, restrictions, TRUE, NULL));
}

/************************************************************
 * DefineFunction3: Used to define a system or user external
 *   function so that the KB can access it. Allows argument
 *   restrictions to be attached to the function.
 *   Return types are:
 *     a - external address
 *     b - boolean integer (converted to symbol)
 *     c - character (converted to symbol)
 *     d - double precision float
 *     f - single precision float (converted to double)
 *     g - long long integer
 *     i - integer (converted to long long integer)
 *     j - unknown (symbol, string,
 *                  or instance name by convention)
 *     k - unknown (symbol or string by convention)
 *     l - long integer (converted to long long integer)
 *     m - unknown (list by convention)
 *     n - unknown (integer or float by convention)
 *     o - instance name
 *     s - string
 *     u - unknown
 *     v - void
 *     w - symbol
 *     x - instance address
 *************************************************************/
int _define_function(void *env, char *name, int returnType, int (*pointer)(void *), char *actualName, char *restrictions, BOOLEAN environmentAware, void *context)
{
    struct core_function_definition *newFunction;

    if( (returnType != RT_EXT_ADDRESS) &&
        (returnType != RT_BOOL) &&
        (returnType != RT_CHAR) &&
        (returnType != RT_DOUBLE) &&
        (returnType != RT_FLOAT) &&
        (returnType != RT_LONG_LONG) &&
        (returnType != RT_INT) &&
        (returnType != RT_ATOM_STRING_INST) &&
        (returnType != RT_ATOM_STRING) &&
        (returnType != RT_LONG) &&
        (returnType != RT_LIST) &&
        (returnType != RT_INT_FLOAT) &&
#if OBJECT_SYSTEM
        (returnType != RT_INSTANCE) &&
#endif
        (returnType != RT_STRING) &&
        (returnType != RT_UNKNOWN) &&
        (returnType != RT_VOID) &&
#if OBJECT_SYSTEM
        (returnType != RT_INST_ADDRESS) &&
#endif
        (returnType != RT_ATOM) )
    {
        return(0);
    }

    newFunction = core_lookup_function(env, name);

    if( newFunction == NULL )
    {
        newFunction = core_mem_get_struct(env, core_function_definition);
        newFunction->function_handle = (ATOM_HN *)store_atom(env, name);
        inc_atom_count(newFunction->function_handle);
        newFunction->next = core_get_function_list(env);
        core_get_function_data(env)->ListOfFunctions = newFunction;
        _add_hash_function(env, newFunction);
    }

    newFunction->return_type = (char)returnType;
    newFunction->functionPointer = (int(*) (void))pointer;
    newFunction->function_name = actualName;

    if( restrictions != NULL )
    {
        if(((int)(strlen(restrictions)) < 2) ? TRUE :
           ((!isdigit(restrictions[0]) && (restrictions[0] != '*')) ||
            (!isdigit(restrictions[1]) && (restrictions[1] != '*'))))
        {
            restrictions = NULL;
        }
    }

    newFunction->restrictions = restrictions;
    newFunction->parser = NULL;
    newFunction->overloadable = TRUE;
    newFunction->sequential_usage_allowed = TRUE;
    newFunction->environment_aware = (short)environmentAware;
    newFunction->ext_data = NULL;
    newFunction->context = context;

    return(1);
}

/**********************************************
 * core_undefine_function: Used to remove a function
 *   definition from the list of functions.
 ***********************************************/
int core_undefine_function(void *env, char *functionName)
{
    ATOM_HN *findValue;
    struct core_function_definition *fPtr, *lastPtr = NULL;

    findValue = (ATOM_HN *)lookup_atom(env, functionName);

    for( fPtr = core_get_function_data(env)->ListOfFunctions;
         fPtr != NULL;
         fPtr = fPtr->next )
    {
        if( fPtr->function_handle == findValue )
        {
            dec_atom_count(env, fPtr->function_handle);
            _remove_hash_function(env, fPtr);

            if( lastPtr == NULL )
            {
                core_get_function_data(env)->ListOfFunctions = fPtr->next;
            }
            else
            {
                lastPtr->next = fPtr->next;
            }

            ext_clear_data(env, fPtr->ext_data);
            core_mem_return_struct(env, core_function_definition, fPtr);
            return(TRUE);
        }

        lastPtr = fPtr;
    }

    return(FALSE);
}

/*****************************************
 * RemoveHashFunction: Removes a function
 *   from the function hash table.
 ******************************************/
static int _remove_hash_function(void *env, struct core_function_definition *fdPtr)
{
    struct core_function_hash *fhPtr, *lastPtr = NULL;
    unsigned hashValue;

    hashValue = hash_atom(to_string(fdPtr->function_handle), FUNCTION_HASH_SZ);

    for( fhPtr = core_get_function_data(env)->FunctionHashtable[hashValue];
         fhPtr != NULL;
         fhPtr = fhPtr->next )
    {
        if( fhPtr->function_definition == fdPtr )
        {
            if( lastPtr == NULL )
            {
                core_get_function_data(env)->FunctionHashtable[hashValue] = fhPtr->next;
            }
            else
            {
                lastPtr->next = fhPtr->next;
            }

            core_mem_return_struct(env, core_function_hash, fhPtr);
            return(TRUE);
        }

        lastPtr = fhPtr;
    }

    return(FALSE);
}

/**************************************************************************
 * core_add_function_parser: Associates a specialized expression parsing function
 *   with the function entry for a function which was defined using
 *   DefineFunction. When this function is parsed, the specialized parsing
 *   function will be called to parse the arguments of the function. Only
 *   user and system defined functions can have specialized parsing
 *   routines. Generic functions and deffunctions can not have specialized
 *   parsing routines.
 ***************************************************************************/
int core_add_function_parser(void *env, char *functionName, struct core_expression *(*fpPtr)(void *, struct core_expression *, char *))
{
    struct core_function_definition *fdPtr;

    fdPtr = core_lookup_function(env, functionName);

    if( fdPtr == NULL )
    {
        print_router(env, WERROR, "Cannot add a parser for a non-existent function.\n");
        return(0);
    }

    fdPtr->restrictions = NULL;
    fdPtr->parser = fpPtr;
    fdPtr->overloadable = FALSE;

    return(1);
}

/****************************************************************
 * core_set_function_overload: Makes a system function overloadable or not,
 * i.e. can the function be a method for a generic function.
 *****************************************************************/
int core_set_function_overload(void *env, char *functionName, int seqp, int ovlp)
{
    struct core_function_definition *fdPtr;

    fdPtr = core_lookup_function(env, functionName);

    if( fdPtr == NULL )
    {
        print_router(env, WERROR, "Cannot add a sequence expansion for a non-existent function.\n");
        return(FALSE);
    }

    fdPtr->sequential_usage_allowed = (short)(seqp ? TRUE : FALSE);
    fdPtr->overloadable = (short)(ovlp ? TRUE : FALSE);
    return(TRUE);
}

/********************************************************
 * core_arg_type_of: Returns a descriptive string for
 *   a function argument type (used by DefineFunction2).
 *********************************************************/
char *core_arg_type_of(int theRestriction)
{
    switch((char)theRestriction )
    {
    case RT_EXT_ADDRESS:
        return(TYPE_POINTER_NAME);

    case RT_INSTANCE_NAME:
        return("instance address, instance name, or symbol");

    case RT_DOUBLE:
    case RT_FLOAT:
        return(TYPE_FLOAT_NAME);

    case RT_LONG_LONG:
        return("integer, float, or symbol");

    case RT_UNUSED_0:
        return("instance address, instance name, fact address, integer, or symbol");

    case RT_ATOM_STRING_INST:
        return("symbol, string, or instance name");

    case RT_ATOM_STRING:
        return("symbol or string");

    case RT_INT:
    case RT_LONG:
        return(TYPE_INT_NAME);

    case RT_LIST:
        return(TYPE_LIST_NAME);

    case RT_INT_FLOAT:
        return("integer or float");

    case RT_INSTANCE:
        return("instance name");

    case RT_INSTANCE_NAME_ATOM:
        return("instance name or symbol");

    case RT_LIST_ATOM_STRING:
        return("list, symbol, or string");

    case RT_STRING:
        return("string");

    case RT_ATOM:
        return("symbol");

    case RT_INST_ADDRESS:
        return("instance address");

    case RT_UNUSED_3:
        return(TYPE_UNDEFINED);

    case RT_UNUSED_4:
        return(TYPE_UNDEFINED);

    case RT_UNKNOWN:
        return("non-void return value");
    }

    return("unknown argument type");
}

/**************************************************
 * core_get_function_arg_restriction: Returns the restriction type
 *   for the nth parameter of a function.
 ***************************************************/
int core_get_function_arg_restriction(struct core_function_definition *func, int position)
{
    int defaultRestriction = (int)RT_UNKNOWN;
    size_t theLength;
    int i = 2;

    /*===========================================================
     * If no restrictions at all are specified for the function,
     * then return RT_UNKNOWN to indicate that any value is suitable as
     * an argument to the function.
     *===========================================================*/

    if( func == NULL )
    {
        return(defaultRestriction);
    }

    if( func->restrictions == NULL )
    {
        return(defaultRestriction);
    }

    /*===========================================================
     * If no type restrictions are specified for the function,
     * then return RT_UNKNOWN to indicate that any value is suitable as
     * an argument to the function.
     *===========================================================*/

    theLength = strlen(func->restrictions);

    if( theLength < 3 )
    {
        return(defaultRestriction);
    }

    /*==============================================
     * Determine the functions default restriction.
     *==============================================*/

    defaultRestriction = (int)func->restrictions[i];

    if( defaultRestriction == '*' )
    {
        defaultRestriction = (int)RT_UNKNOWN;
    }

    /*=======================================================
     * If the requested position does not have a restriction
     * specified, then return the default restriction.
     *=======================================================*/

    if( theLength < (size_t)(position + 3))
    {
        return(defaultRestriction);
    }

    /*=========================================================
     * Return the restriction specified for the nth parameter.
     *=========================================================*/

    return((int)func->restrictions[position + 2]);
}

/************************************************
 * core_get_function_list: Returns the ListOfFunctions.
 *************************************************/
struct core_function_definition *core_get_function_list(void *env)
{
    return(core_get_function_data(env)->ListOfFunctions);
}

/*******************************************************
 * core_lookup_function: Returns a pointer to the corresponding
 *   core_function_definition structure if a function name is
 *   in the function list, otherwise returns NULL.
 ********************************************************/
struct core_function_definition *core_lookup_function(void *env, char *functionName)
{
    struct core_function_hash *fhPtr;
    unsigned hashValue;
    ATOM_HN *findValue;

    if( core_get_function_data(env)->FunctionHashtable == NULL ) {return(NULL);}

    hashValue = hash_atom(functionName, FUNCTION_HASH_SZ);

    findValue = (ATOM_HN *)lookup_atom(env, functionName);

    for( fhPtr = core_get_function_data(env)->FunctionHashtable[hashValue];
         fhPtr != NULL;
         fhPtr = fhPtr->next )
    {
        if( fhPtr->function_definition->function_handle == findValue )
        {
            return(fhPtr->function_definition);
        }
    }

    return(NULL);
}

/********************************************************
 * InitializeFunctionHashTable: Purpose is to initialize
 *   the function hash table to NULL.
 *********************************************************/
static void _initialize_function_hash_table(void *env)
{
    int i;

    core_get_function_data(env)->FunctionHashtable = (struct core_function_hash **)
                                                     core_mem_alloc_no_init(env, (int)sizeof(struct core_function_hash *) *
                                                                            FUNCTION_HASH_SZ);

    for( i = 0; i < FUNCTION_HASH_SZ; i++ )
    {
        core_get_function_data(env)->FunctionHashtable[i] = NULL;
    }
}

/***************************************************************
 * AddHashFunction: Adds a function to the function hash table.
 ****************************************************************/
static void _add_hash_function(void *env, struct core_function_definition *fdPtr)
{
    struct core_function_hash *newhash, *temp;
    unsigned hashValue;

    if( core_get_function_data(env)->FunctionHashtable == NULL )
    {
        _initialize_function_hash_table(env);
    }

    newhash = core_mem_get_struct(env, core_function_hash);
    newhash->function_definition = fdPtr;

    hashValue = hash_atom(fdPtr->function_handle->contents, FUNCTION_HASH_SZ);

    temp = core_get_function_data(env)->FunctionHashtable[hashValue];
    core_get_function_data(env)->FunctionHashtable[hashValue] = newhash;
    newhash->next = temp;
}

/************************************************
 * core_get_min_args: Returns the minimum number of
 *   arguments expected by an external function.
 *************************************************/
int core_get_min_args(struct core_function_definition *func)
{
    char theChar[2], *restrictions;

    restrictions = func->restrictions;

    if( restrictions == NULL )
    {
        return(-1);
    }

    theChar[0] = restrictions[0];
    theChar[1] = '\0';

    if( isdigit(theChar[0]))
    {
        return(atoi(theChar));
    }
    else if( theChar[0] == '*' )
    {
        return(-1);
    }

    return(-1);
}

/************************************************
 * core_get_max_args: Returns the maximum number of
 *   arguments expected by an external function.
 *************************************************/
int core_get_max_args(struct core_function_definition *func)
{
    char theChar[2], *restrictions;

    restrictions = func->restrictions;

    if( restrictions == NULL )
    {
        return(-1);
    }

    if( restrictions[0] == '\0' )
    {
        return(-1);
    }

    theChar[0] = restrictions[1];
    theChar[1] = '\0';

    if( isdigit(theChar[0]))
    {
        return(atoi(theChar));
    }
    else if( theChar[0] == '*' )
    {
        return(-1);
    }

    return(-1);
}
