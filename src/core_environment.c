/* Purpose: Routines for supporting multiple environments.   */

#define __CORE_ENVIRONMENT_SOURCE__

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "setup.h"

#include "core_memory.h"
#include "core_utilities.h"
#include "router.h"
#include "sysdep.h"
#include "core_gc.h"

#include "core_environment.h"

#define SIZE_ENVIRONMENT_HASH  131

/**************************************
 * LOCAL FUNCTION PROTOTYPES
 ***************************************/

static void   _remove_cleaners(struct core_environment *);
static void * _create_driver(struct atom_hash_node **, struct float_hash_node **, struct integer_hash_node **, struct bitmap_hash_node **, struct external_address_hash_node **);

/**************************************
 * LOCAL INTERNAL VARIABLE DEFINITIONS
 ***************************************/


/******************************************************
 * core_allocate_environment_data: Allocates environment data
 *    for the specified environment data record.
 *******************************************************/
BOOLEAN core_allocate_environment_data(void *venvironment, unsigned int position, unsigned long size, void (*cleanupFunction)(void *))
{
    struct core_environment *environment = (struct core_environment *)venvironment;

    /*===========================================
     * Environment data can't be of length zero.
     *===========================================*/

    if( size <= 0 )
    {
        printf("\n[ENVRNMNT1] Environment data position %d allocated with size of 0 or less.\n", position);
        return(FALSE);
    }

    /*================================================================
     * Check to see if the data position exceeds the maximum allowed.
     *================================================================*/

    if( position >= MAXIMUM_ENVIRONMENT_POSITIONS )
    {
        printf("\n[ENVRNMNT2] Environment data position %d exceeds the maximum allowed.\n", position);
        return(FALSE);
    }

    /*============================================================
     * Check if the environment data has already been registered.
     *============================================================*/

    if( environment->data[position] != NULL )
    {
        printf("\n[ENVRNMNT3] Environment data position %d already allocated.\n", position);
        return(FALSE);
    }

    /*====================
     * Allocate the data.
     *====================*/

    environment->data[position] = malloc(size);

    if( environment->data[position] == NULL )
    {
        printf("\n[ENVRNMNT4] Environment data position %d could not be allocated.\n", position);
        return(FALSE);
    }

    memset(environment->data[position], 0, size);

    /*=============================
     * Store the cleanup function.
     *=============================*/

    environment->cleaners[position] = cleanupFunction;

    /*===============================
     * Data successfully registered.
     *===============================*/

    return(TRUE);
}

/**************************************************************
 * core_delete_environment_data: Deallocates all environments
 *   stored in the environment hash table and then deallocates
 *   the environment hash table.
 ***************************************************************/
BOOLEAN core_delete_environment_data()
{
    return(FALSE);
}

/***********************************************************
 * core_create_environment: Creates an environment data structure
 *   and initializes its content to zero/null.
 ************************************************************/
void *core_create_environment()
{
    return(_create_driver(NULL, NULL, NULL, NULL, NULL));
}

/********************************************************
 * CreateEnvironmentDriver: Creates an environment data
 *   structure and initializes its content to zero/null.
 *********************************************************/
void *_create_driver(struct atom_hash_node **symbolTable, struct float_hash_node **floatTable, struct integer_hash_node **integerTable, struct bitmap_hash_node **bitmapTable, struct external_address_hash_node **externalAddressTable)
{
    struct core_environment *environment;
    void *theData;

    environment = (struct core_environment *)malloc(sizeof(struct core_environment));

    if( environment == NULL )
    {
        printf("\n[ENVRNMNT5] Unable to create new environment.\n");
        return(NULL);
    }

    theData = malloc(sizeof(void *) * MAXIMUM_ENVIRONMENT_POSITIONS);

    if( theData == NULL )
    {
        free(environment);
        printf("\n[ENVRNMNT6] Unable to create environment data.\n");
        return(NULL);
    }

    memset(theData, 0, sizeof(void *) * MAXIMUM_ENVIRONMENT_POSITIONS);

    environment->initialized = FALSE;
    environment->data = (void **)theData;
    environment->next = NULL;
    environment->environment_cleaner_list = NULL;
    environment->environment_index = 0;
    environment->context = NULL;
    environment->router_context = NULL;
    environment->function_context = NULL;
    environment->callback_context = NULL;

    /*=============================================
     * Allocate storage for the cleanup functions.
     *=============================================*/

    theData = malloc(sizeof(void(*) (struct core_environment *)) * MAXIMUM_ENVIRONMENT_POSITIONS);

    if( theData == NULL )
    {
        free(environment->data);
        free(environment);
        printf("\n[ENVRNMNT7] Unable to create environment data.\n");
        return(NULL);
    }

    memset(theData, 0, sizeof(void(*) (struct core_environment *)) * MAXIMUM_ENVIRONMENT_POSITIONS);
    environment->cleaners = (void (**)(void *))theData;

    init_system(environment, symbolTable, floatTable, integerTable, bitmapTable, externalAddressTable);

    return(environment);
}

/*********************************************
 * core_get_environment_context: Returns the context
 *   of the specified environment.
 **********************************************/
void *core_get_environment_context(void *environment)
{
    return(((struct core_environment *)environment)->context);
}

/******************************************
 * core_set_environment_context: Sets the context
 *   of the specified environment.
 *******************************************/
void *core_set_environment_context(void *environment, void *theContext)
{
    void *oldContext;

    oldContext = ((struct core_environment *)environment)->context;

    ((struct core_environment *)environment)->context = theContext;

    return(oldContext);
}

/**************************************************
 * core_get_environment_router_context: Returns the router
 *   context of the specified environment.
 ***************************************************/
void *core_get_environment_router_context(void *environment)
{
    return(((struct core_environment *)environment)->router_context);
}

/***********************************************
 * core_set_environment_router_context: Sets the router
 *   context of the specified environment.
 ************************************************/
void *core_set_environment_router_context(void *environment, void *theRouterContext)
{
    void *oldRouterContext;

    oldRouterContext = ((struct core_environment *)environment)->router_context;

    ((struct core_environment *)environment)->router_context = theRouterContext;

    return(oldRouterContext);
}

/******************************************************
 * core_get_environment_function_context: Returns the function
 *   context of the specified environment.
 *******************************************************/
void *core_get_environment_function_context(void *environment)
{
    return(((struct core_environment *)environment)->function_context);
}

/*************************************************
 * core_set_environment_function_context: Sets the router
 *   context of the specified environment.
 **************************************************/
void *core_set_environment_function_context(void *environment, void *funcContext)
{
    void *oldFunctionContext;

    oldFunctionContext = ((struct core_environment *)environment)->function_context;

    ((struct core_environment *)environment)->function_context = funcContext;

    return(oldFunctionContext);
}

/******************************************************
 * core_get_environment_callback_context: Returns the callback
 *   context of the specified environment.
 *******************************************************/
void *core_get_environment_callback_context(void *environment)
{
    return(((struct core_environment *)environment)->callback_context);
}

/***************************************************
 * core_set_environment_callback_context: Sets the callback
 *   context of the specified environment.
 ****************************************************/
void *core_set_environment_callback_context(void *environment, void *theCallbackContext)
{
    void *oldCallbackContext;

    oldCallbackContext = ((struct core_environment *)environment)->callback_context;

    ((struct core_environment *)environment)->callback_context = theCallbackContext;

    return(oldCallbackContext);
}

/*********************************************
 * core_delete_environment: Destroys the specified
 *   environment returning all of its memory.
 **********************************************/
BOOLEAN core_delete_environment(void *venvironment)
{
    struct environment_cleaner *cleanupPtr;
    int i;
    struct core_mem_memory *theMemData;
    BOOLEAN rv = TRUE;
    struct core_environment *environment = (struct core_environment *)venvironment;

    theMemData = core_mem_get_memory_data(environment);

    core_mem_release_count(environment, -1, FALSE);

    for( i = 0; i < MAXIMUM_ENVIRONMENT_POSITIONS; i++ )
    {
        if( environment->cleaners[i] != NULL )
        {
            (*environment->cleaners[i])(environment);
        }
    }

    free(environment->cleaners);

    for( cleanupPtr = environment->environment_cleaner_list;
         cleanupPtr != NULL;
         cleanupPtr = cleanupPtr->next )
    {
        (*cleanupPtr->func)(environment);
    }

    _remove_cleaners(environment);

    core_mem_release_count(environment, -1, FALSE);

    if((theMemData->amount != 0) || (theMemData->calls != 0))
    {
        printf("\n[ENVRNMNT8] Environment data not fully deallocated.\n");
        printf("\n[ENVRNMNT8] MemoryAmount = %ld.\n", (long)theMemData->amount);
        printf("\n[ENVRNMNT8] MemoryCalls = %ld.\n", (long)theMemData->calls);
        rv = FALSE;
    }

    free(theMemData->memory_table);

#if BLOCK_MEMORY
    ReturnAllBlocks(environment);
#endif

    for( i = 0; i < MAXIMUM_ENVIRONMENT_POSITIONS; i++ )
    {
        if( environment->data[i] != NULL )
        {
            free(environment->data[i]);
            environment->data[i] = NULL;
        }
    }

    free(environment->data);
    free(environment);

    return(rv);
}

/*************************************************
 * core_add_environment_cleaner: Adds a function
 *   to the ListOfCleanupEnvironmentFunctions.
 **************************************************/
BOOLEAN core_add_environment_cleaner(void *venv, char *name, void (*functionPtr)(void *), int priority)
{
    struct environment_cleaner *newPtr, *currentPtr, *lastPtr = NULL;
    struct core_environment *env = (struct core_environment *)venv;

    newPtr = (struct environment_cleaner *)malloc(sizeof(struct environment_cleaner));

    if( newPtr == NULL )
    {
        return(FALSE);
    }

    newPtr->name = name;
    newPtr->func = functionPtr;
    newPtr->priority = priority;

    if( env->environment_cleaner_list == NULL )
    {
        newPtr->next = NULL;
        env->environment_cleaner_list = newPtr;
        return(TRUE);
    }

    currentPtr = env->environment_cleaner_list;

    while((currentPtr != NULL) ? (priority < currentPtr->priority) : FALSE )
    {
        lastPtr = currentPtr;
        currentPtr = currentPtr->next;
    }

    if( lastPtr == NULL )
    {
        newPtr->next = env->environment_cleaner_list;
        env->environment_cleaner_list = newPtr;
    }
    else
    {
        newPtr->next = currentPtr;
        lastPtr->next = newPtr;
    }

    return(TRUE);
}

/*************************************************
 * RemoveEnvironmentCleanupFunctions: Removes the
 *   list of environment cleanup functions.
 **************************************************/
static void _remove_cleaners(struct core_environment *env)
{
    struct environment_cleaner *nextPtr;

    while( env->environment_cleaner_list != NULL )
    {
        nextPtr = env->environment_cleaner_list->next;
        free(env->environment_cleaner_list);
        env->environment_cleaner_list = nextPtr;
    }
}
