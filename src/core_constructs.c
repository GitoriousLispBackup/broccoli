/* Purpose: Provides basic functionality for creating new
 *   types of constructs, saving constructs to a file, and
 *   adding new functionality to the clear and reset
 *   commands.                                               */

#define __CORE_CONSTRUCTS_SOURCE__

#include <stdio.h>
#define _STDIO_INCLUDED_
#include <string.h>

#include "setup.h"

#include "constant.h"
#include "core_environment.h"
#include "core_memory.h"
#include "router.h"
#include "core_scanner.h"
#include "core_watch.h"
#include "funcs_flow_control.h"
#include "parser_flow_control.h"
#include "core_arguments.h"
#include "parser_expressions.h"
#include "type_list.h"
#include "modules_init.h"
#include "sysdep.h"
#include "core_gc.h"
#include "core_command_prompt.h"

#include "core_constructs.h"

/**************************************
 * LOCAL FUNCTION PROTOTYPES
 ***************************************/

static void    clearConstructs(void *);
static BOOLEAN isClearReady(void *);

/*************************************************
 * core_init_constructs: Allocates environment
 *    data for constructs.
 **************************************************/
void core_init_constructs(void *env)
{
    core_allocate_environment_data(env, CONSTRUCT_DATA_INDEX, sizeof(struct construct_data), clearConstructs);

#if METAOBJECT_PROTOCOL
    core_construct_data(env)->WatchCompilations = ON;
#endif
}

/************************************************
 * core_lookup_construct_by_name: Determines whether a construct
 *   type is in the construct_list.
 *************************************************/
struct construct *core_lookup_construct_by_name(void *env, char *name)
{
    struct construct *currentPtr;

    for( currentPtr = core_construct_data(env)->construct_list;
         currentPtr != NULL;
         currentPtr = currentPtr->next )
    {
        if( strcmp(name, currentPtr->construct_name) == 0 )
        {
            return(currentPtr);
        }
    }

    return(NULL);
}

/**********************************************************
 * core_undefine_construct: Removes a construct and its associated
 *   parsing function from the construct_list. Returns
 *   TRUE if the construct type was removed, otherwise
 *   FALSE.
 ***********************************************************/
int core_undefine_construct(void *env, char *name)
{
    struct construct *currentPtr, *lastPtr = NULL;

    for( currentPtr = core_construct_data(env)->construct_list;
         currentPtr != NULL;
         currentPtr = currentPtr->next )
    {
        if( strcmp(name, currentPtr->construct_name) == 0 )
        {
            if( lastPtr == NULL )
            {
                core_construct_data(env)->construct_list = currentPtr->next;
            }
            else
            {
                lastPtr->next = currentPtr->next;
            }

            core_mem_return_struct(env, construct, currentPtr);
            return(TRUE);
        }

        lastPtr = currentPtr;
    }

    return(FALSE);
}

/***********************************************
 * Save: C access routine for the save command.
 ************************************************/
int core_save(void *env, char *fileName)
{
#if OFF
    struct core_gc_call_function *saveFunction;
    FILE *filePtr;
    void *defmodulePtr;

    /*=====================
     * Open the save file.
     *=====================*/

    if((filePtr = sysdep_open_file(env, fileName, "w")) == NULL )
    {
        return(FALSE);
    }

    /*===========================
     * Bypass the router system.
     *===========================*/

    set_fast_save(env, filePtr);

    /*======================
     * Save the constructs.
     *======================*/

    for( defmodulePtr = get_next_module(env, NULL);
         defmodulePtr != NULL;
         defmodulePtr = get_next_module(env, defmodulePtr))
    {
        for( saveFunction = core_construct_data(env)->saver_list;
             saveFunction != NULL;
             saveFunction = saveFunction->next )
        {
            ((*(void(*) (void *, void *, char *))saveFunction->func))(env, defmodulePtr, (char *)filePtr);
        }
    }

    /*======================
     * Close the save file.
     *======================*/

    sysdep_close_file(env, filePtr);

    /*===========================
     * Remove the router bypass.
     *===========================*/

    set_fast_save(env, NULL);

    /*=========================
     * Return TRUE to indicate
     * successful completion.
     *=========================*/
#endif
    return(TRUE);
}

/******************************************************
 * core_remove_saver: Removes a function from the
 *   saver_list. Returns TRUE if the function
 *   was successfully removed, otherwise FALSE.
 *******************************************************/
BOOLEAN core_remove_saver(void *env, char *name)
{
    int found;

    core_construct_data(env)->saver_list =
        core_gc_remove_call_function(env, name, core_construct_data(env)->saver_list, &found);

    if( found )
    {
        return(TRUE);
    }

    return(FALSE);
}

#if METAOBJECT_PROTOCOL

/*********************************
 * mopSetWatchCompilations: Sets the
 *   value of WatchCompilations.
 **********************************/
void mopSetWatchCompilations(void *env, unsigned value)
{
    core_construct_data(env)->WatchCompilations = value;
}

/************************************
 * mopIsWatchCompilations: Returns the
 *   value of WatchCompilations.
 *************************************/
unsigned mopIsWatchCompilations(void *env)
{
    return(core_construct_data(env)->WatchCompilations);
}

#endif

/*********************************
 * core_set_verbose_loading: Sets the
 *   value of verbose_loading.
 **********************************/
void core_set_verbose_loading(void *env, BOOLEAN value)
{
    core_construct_data(env)->verbose_loading = value;
}

/************************************
 * core_is_verbose_loading: Returns the
 *   value of verbose_loading.
 *************************************/
BOOLEAN core_is_verbose_loading(void *env)
{
    return(core_construct_data(env)->verbose_loading);
}

/************************************
 * core_init_constructs_manager: Initializes
 *   the Construct Manager.
 *************************************/
void core_init_constructs_manager(void *env)
{
    core_define_function(env, FUNC_NAME_CLEAR_ALL, RT_VOID, PTR_FN broccoli_clear, "broccoli_clear", FUNC_CNSTR_CLEAR_ALL);
    core_define_function(env, FUNC_NAME_RESET, RT_VOID, PTR_FN broccoli_reset, "broccoli_clear", FUNC_CNSTR_RESET);

#if DEBUGGING_FUNCTIONS
    AddWatchItem(env, "compilations", 0, &core_construct_data(env)->WatchCompilations, 30, NULL, NULL);
#endif
}

/*************************************
 * ClearCommand: H/L access routine
 *   for the clear command.
 **************************************/
void broccoli_clear(void *env)
{
    if( core_check_arg_count(env, FUNC_NAME_CLEAR_ALL, EXACTLY, 0) == -1 )
    {
        return;
    }

    core_clear(env);
    return;
}

/*************************************
 * ResetCommand: H/L access routine
 *   for the reset command.
 **************************************/
void broccoli_reset(void *env)
{
    if( core_check_arg_count(env, FUNC_NAME_RESET, EXACTLY, 0) == -1 )
    {
        return;
    }

    core_reset(env);
    return;
}

/*****************************
 * core_reset: C access routine
 *   for the reset command.
 ******************************/
void core_reset(void *env)
{
    struct core_gc_call_function *resetPtr;

    /*=====================================
     * The reset command can't be executed
     * while a reset is in progress.
     *=====================================*/

    if( core_construct_data(env)->reset_in_progress )
    {
        return;
    }

    core_construct_data(env)->reset_in_progress = TRUE;
    core_construct_data(env)->reset_ready_in_progress = TRUE;

    /*================================================
     * If the reset is performed from the top level
     * command prompt, reset the halt execution flag.
     *================================================*/

    if( core_get_evaluation_data(env)->eval_depth == 0 )
    {
        core_set_halt_eval(env, FALSE);
    }

    /*=======================================================
     * Call the before reset function to determine if the
     * reset should continue. [Used by the some of the
     * windowed interfaces to query the user whether a
     * reset should proceed with activations on the agenda.]
     *=======================================================*/

    if((core_construct_data(env)->before_reset_hook != NULL) ?
       ((*core_construct_data(env)->before_reset_hook)(env) == FALSE) :
       FALSE )
    {
        core_construct_data(env)->reset_ready_in_progress = FALSE;
        core_construct_data(env)->reset_in_progress = FALSE;
        return;
    }

    core_construct_data(env)->reset_ready_in_progress = FALSE;

    /*===========================
     * Call each reset function.
     *===========================*/

    for( resetPtr = core_construct_data(env)->reset_listeners;
         (resetPtr != NULL) && (core_get_halt_eval(env) == FALSE);
         resetPtr = resetPtr->next )
    {
        if( resetPtr->environment_aware )
        {
            (*resetPtr->func)(env);
        }
        else
        {
            (*(void(*) (void))resetPtr->func)();
        }
    }

    /*============================================
     * Set the current module to the MAIN module.
     *============================================*/

    set_current_module(env, (void *)lookup_module(env, "MAIN"));

    /*===========================================
     * Perform periodic cleanup if the reset was
     * issued from an embedded controller.
     *===========================================*/

    if((core_get_evaluation_data(env)->eval_depth == 0) && (!CommandLineData(env)->EvaluatingTopLevelCommand) &&
       (core_get_evaluation_data(env)->current_expression == NULL))
    {
        core_gc_periodic_cleanup(env, TRUE, FALSE);
    }

    /*===================================
     * A reset is no longer in progress.
     *===================================*/

    core_construct_data(env)->reset_in_progress = FALSE;
}

/***********************************
 * core_set_reset_fn: Sets the
 *  value of before_reset_hook.
 ************************************/
int(*core_set_reset_fn(void *env,
                       int (*func)(void *))) (void *)
{
    int (*tempFunction)(void *);

    tempFunction = core_construct_data(env)->before_reset_hook;
    core_construct_data(env)->before_reset_hook = func;
    return(tempFunction);
}

/***************************************
 * core_add_reset_listener: Adds a function
 *   to reset_listeners.
 ****************************************/
BOOLEAN core_add_reset_listener(void *env, char *name, void (*functionPtr)(void *), int priority)
{
    core_construct_data(env)->reset_listeners = core_gc_add_call_function(env, name, priority,
                                                                          functionPtr,
                                                                          core_construct_data(env)->reset_listeners, TRUE);
    return(TRUE);
}

/*********************************************
 * core_remove_reset_listener: Removes a function
 *   from the reset_listeners.
 **********************************************/
BOOLEAN core_remove_reset_listener(void *env, char *name)
{
    int found;

    core_construct_data(env)->reset_listeners =
        core_gc_remove_call_function(env, name, core_construct_data(env)->reset_listeners, &found);

    if( found )
    {
        return(TRUE);
    }

    return(FALSE);
}

/****************************************************
 * core_clear: C access routine for the clear command.
 *****************************************************/
void core_clear(void *env)
{
    struct core_gc_call_function *func;

    /*==========================================
     * Activate the watch router which captures
     * trace output so that it is not displayed
     * during a clear.
     *==========================================*/

#if DEBUGGING_FUNCTIONS
    activate_router(env, WTRACE);
#endif

    /*===================================
     * Determine if a clear is possible.
     *===================================*/

    core_construct_data(env)->clear_ready_in_progress = TRUE;

    if( isClearReady(env) == FALSE )
    {
        error_print_id(env, "CONSTRUCTS", 1, FALSE);
        print_router(env, WERROR, "Some constructs are still in use. Clear cannot continue.\n");
#if DEBUGGING_FUNCTIONS
        deactivate_router(env, WTRACE);
#endif
        core_construct_data(env)->clear_ready_in_progress = FALSE;
        return;
    }

    core_construct_data(env)->clear_ready_in_progress = FALSE;

    /*===========================
     * Call all clear functions.
     *===========================*/

    core_construct_data(env)->clear_in_progress = TRUE;

    for( func = core_construct_data(env)->clear_listeners;
         func != NULL;
         func = func->next )
    {
        if( func->environment_aware )
        {
            (*func->func)(env);
        }
        else
        {
            (*(void(*) (void))func->func)();
        }
    }

    /*=============================
     * Deactivate the watch router
     * for capturing output.
     *=============================*/

#if DEBUGGING_FUNCTIONS
    deactivate_router(env, WTRACE);
#endif

    /*===========================================
     * Perform periodic cleanup if the clear was
     * issued from an embedded controller.
     *===========================================*/

    if((core_get_evaluation_data(env)->eval_depth == 0) && (!CommandLineData(env)->EvaluatingTopLevelCommand) &&
       (core_get_evaluation_data(env)->current_expression == NULL))
    {
        core_gc_periodic_cleanup(env, TRUE, FALSE);
    }

    /*===========================
     * Clear has been completed.
     *===========================*/

    core_construct_data(env)->clear_in_progress = FALSE;

    /*============================
     * Perform reset after clear.
     *============================*/

    core_reset(env);
}

/*****************************************
 * core_add_clear_listener: Adds a function
 *   to clear_ready_listeners.
 ******************************************/
BOOLEAN core_add_clear_listener(void *env, char *name, int (*functionPtr)(void *), int priority)
{
    core_construct_data(env)->clear_ready_listeners =
        core_gc_add_call_function(env, name, priority,
                                  (void(*) (void *))functionPtr,
                                  core_construct_data(env)->clear_ready_listeners, TRUE);
    return(1);
}

/***********************************************
 * core_remove_clear_listener: Removes a function
 *   from the clear_ready_listeners.
 ************************************************/
BOOLEAN core_remove_clear_listener(void *env, char *name)
{
    int found;

    core_construct_data(env)->clear_ready_listeners =
        core_gc_remove_call_function(env, name, core_construct_data(env)->clear_ready_listeners, &found);

    if( found )
    {
        return(TRUE);
    }

    return(FALSE);
}

/***************************************
 * core_add_clear_fn: Adds a function
 *   to clear_listeners.
 ****************************************/
BOOLEAN core_add_clear_fn(void *env, char *name, void (*functionPtr)(void *), int priority)
{
    core_construct_data(env)->clear_listeners =
        core_gc_add_call_function(env, name, priority,
                                  (void(*) (void *))functionPtr,
                                  core_construct_data(env)->clear_listeners, TRUE);
    return(1);
}

/*********************************************
 * core_remove_clear_fn: Removes a function
 *    from the clear_listeners.
 **********************************************/
BOOLEAN core_remove_clear_fn(void *env, char *name)
{
    int found;

    core_construct_data(env)->clear_listeners =
        core_gc_remove_call_function(env, name, core_construct_data(env)->clear_listeners, &found);

    if( found )
    {
        return(TRUE);
    }

    return(FALSE);
}

/**********************************************
 * core_is_construct_executing: Returns TRUE if a
 *   construct is currently being executed,
 *   otherwise FALSE.
 ***********************************************/
int core_is_construct_executing(void *env)
{
    return(core_construct_data(env)->executing);
}

/*******************************************
 * core_set_construct_executing: Sets the value of
 *   the executing variable indicating that
 *   actions such as reset, clear, etc
 *   should not be performed.
 ********************************************/
void core_set_construct_executing(void *env, int value)
{
    core_construct_data(env)->executing = value;
}

/***********************************************************
 * core_garner_construct_list: Returns a list of all the construct
 *   names in a list value. It doesn't check the
 *   number of arguments. It assumes that the restriction
 *   string in DefineFunction2 call was "00".
 ************************************************************/
void core_garner_construct_list_by_name(void *env, core_data_object_ptr ret, void *(*nextFunction)(void *, void *), char *(*nameFunction)(void *, void *))
{
    void *cons;
    unsigned long count = 0;
    struct list *list;

    /*====================================
     * Determine the number of constructs
     * of the specified type.
     *====================================*/

    for( cons = (*nextFunction)(env, NULL);
         cons != NULL;
         cons = (*nextFunction)(env, cons))
    {
        count++;
    }

    /*===========================
     * Create a list large
     * enough to store the list.
     *===========================*/

    core_set_pointer_type(ret, LIST);
    core_set_data_ptr_start(ret, 1);
    core_set_data_ptr_end(ret, (long)count);
    list = (struct list *)create_list(env, count);
    core_set_pointer_value(ret, (void *)list);

    /*====================================
     * Store the names in the list.
     *====================================*/

    for( cons = (*nextFunction)(env, NULL), count = 1;
         cons != NULL;
         cons = (*nextFunction)(env, cons), count++ )
    {
        if( core_get_evaluation_data(env)->halt == TRUE )
        {
            core_create_error_list(env, ret);
            return;
        }

        set_list_node_type(list, count, ATOM);
        set_list_node_value(list, count, store_atom(env, (*nameFunction)(env, cons)));
    }
}

/*************************************************
 * core_delete_construct_metadata: Frees the pretty print
 *   representation string and user data (both of
 *   which are stored in the generic construct
 *   header).
 **************************************************/
void core_delete_construct_metadata(void *env, struct construct_metadata *theHeader)
{
    if( theHeader->pp != NULL )
    {
        core_mem_release(env, theHeader->pp,
                         sizeof(char) * (strlen(theHeader->pp) + 1));
        theHeader->pp = NULL;
    }

    if( theHeader->ext_data != NULL )
    {
        ext_clear_data(env, theHeader->ext_data);
        theHeader->ext_data = NULL;
    }
}

/****************************************************
 * AddConstruct: Adds a construct and its associated
 *   parsing function to the construct_list.
 *****************************************************/
struct construct *core_define_construct(void *env, char *name, char *pluralName, int (*parseFunction)(void *, char *), void *(*findFunction)(void *, char *), ATOM_HN *(*getConstructNameFunction)(struct construct_metadata *), char *(*getPPFormFunction)(void *, struct construct_metadata *), struct module_header *(*getModuleItemFunction)(struct construct_metadata *), void *(*getNextItemFunction)(void *, void *), void (*setNextItemFunction)(struct construct_metadata *, struct construct_metadata *), BOOLEAN (*isConstructDeletableFunction)(void *, void *), int (*deleteFunction)(void *, void *), void (*freeFunction)(void *, void *))
{
    struct construct *newPtr;

    /*=============================
     * Allocate and initialize the
     * construct data structure.
     *=============================*/

    newPtr = core_mem_get_struct(env, construct);

    newPtr->construct_name = name;
    newPtr->construct_name_plural = pluralName;
    newPtr->parser = parseFunction;
    newPtr->finder = findFunction;
    newPtr->namer = getConstructNameFunction;
    newPtr->pper = getPPFormFunction;
    newPtr->module_header_geter = getModuleItemFunction;
    newPtr->next_item_geter = getNextItemFunction;
    newPtr->next_item_seter = setNextItemFunction;
    newPtr->is_deletable_checker = isConstructDeletableFunction;
    newPtr->deleter = deleteFunction;
    newPtr->freer = freeFunction;

    /*===============================
     * Add the construct to the list
     * of constructs and return it.
     *===============================*/

    newPtr->next = core_construct_data(env)->construct_list;
    core_construct_data(env)->construct_list = newPtr;
    return(newPtr);
}

/***********************************
 * core_add_saver: Adds a function
 *   to the saver_list.
 ************************************/
BOOLEAN core_add_saver(void *env, char *name, void (*functionPtr)(void *, void *, char *), int priority)
{
    core_construct_data(env)->saver_list =
        core_gc_add_call_function(env, name, priority,
                                  (void(*) (void *))functionPtr,
                                  core_construct_data(env)->saver_list, TRUE);

    return(1);
}

/**************************************
 * LOCAL FUNCTION PROTOTYPES
 ***************************************/

/***************************************************
 * DeallocateConstructData: Deallocates environment
 *    data for constructs.
 ****************************************************/
static void clearConstructs(void *env)
{
    struct construct *tmpPtr, *nextPtr;

    core_gc_delete_call_list(env, core_construct_data(env)->saver_list);
    core_gc_delete_call_list(env, core_construct_data(env)->reset_listeners);
    core_gc_delete_call_list(env, core_construct_data(env)->clear_listeners);
    core_gc_delete_call_list(env, core_construct_data(env)->clear_ready_listeners);

    tmpPtr = core_construct_data(env)->construct_list;

    while( tmpPtr != NULL )
    {
        nextPtr = tmpPtr->next;
        core_mem_return_struct(env, construct, tmpPtr);
        tmpPtr = nextPtr;
    }
}

/********************************************************
 * ClearReady: Returns TRUE if a clear can be performed,
 *   otherwise FALSE. Note that this is destructively
 *   determined (e.g. facts will be deleted as part of
 *   the determination).
 *********************************************************/
static BOOLEAN isClearReady(void *env)
{
    struct core_gc_call_function *func;

    int (*tempFunction)(void *);

    for( func = core_construct_data(env)->clear_ready_listeners;
         func != NULL;
         func = func->next )
    {
        tempFunction = (int(*) (void *))func->func;

        if((*tempFunction)(env) == FALSE )
        {
            return(FALSE);
        }
    }

    return(TRUE);
}
