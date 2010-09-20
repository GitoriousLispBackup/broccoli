/* Purpose: Support functions for the watch and unwatch
 *   commands.                                               */

#define __CORE_WATCH_SOURCE__

#include "setup.h"

#if DEBUGGING_FUNCTIONS

#include <stdio.h>
#define _STDIO_INCLUDED_
#include <string.h>

#include "constant.h"
#include "core_environment.h"
#include "core_memory.h"
#include "router.h"
#include "core_arguments.h"
#include "core_functions.h"
#include "core_watch.h"

/**************************************
 * LOCAL FUNCTION PROTOTYPES
 ***************************************/

static struct core_watch_item       * ValidWatchItem(void *, char *, int *);
static BOOLEAN                        RecognizeWatchRouters(void *, char *);
static int                            CaptureWatchPrints(void *, char *, char *);
static void                           DeallocateWatchData(void *);

/*********************************************
 * InitializeWatchData: Allocates environment
 *    data for watch items.
 **********************************************/
void InitializeWatchData(void *env)
{
    core_allocate_environment_data(env, WATCH_DATA_INDEX, sizeof(struct core_watch_data), DeallocateWatchData);
}

/***********************************************
 * DeallocateWatchData: Deallocates environment
 *    data for watch items.
 ************************************************/
static void DeallocateWatchData(void *env)
{
    struct core_watch_item *tmpPtr, *nextPtr;

    tmpPtr = core_get_watch_data(env)->items;

    while( tmpPtr != NULL )
    {
        nextPtr = tmpPtr->next;
        core_mem_return_struct(env, core_watch_item, tmpPtr);
        tmpPtr = nextPtr;
    }
}

/************************************************************
 * AddWatchItem: Adds an item to the list of watchable items
 *   that can be set using the watch and unwatch commands.
 *   Returns FALSE if the item is already in the list,
 *   otherwise returns TRUE.
 *************************************************************/
BOOLEAN AddWatchItem(void *env, char *name, int code, unsigned *flag, int priority, unsigned (*fn_access)(void *, int, unsigned, struct core_expression *), unsigned (*fn_print)(void *, char *, int, struct core_expression *))
{
    struct core_watch_item *newPtr, *currentPtr, *lastPtr;

    /*================================================================
     * Find the insertion point in the watchable items list to place
     * the new item. If the item is already in the list return FALSE.
     *================================================================*/

    for( currentPtr = core_get_watch_data(env)->items, lastPtr = NULL;
         currentPtr != NULL;
         currentPtr = currentPtr->next )
    {
        if( strcmp(currentPtr->name, name) == 0 )
        {
            return(FALSE);
        }

        if( priority < currentPtr->priority )
        {
            lastPtr = currentPtr;
        }
    }

    /*============================
     * Create the new watch item.
     *============================*/

    newPtr = core_mem_get_struct(env, core_watch_item);
    newPtr->name = name;
    newPtr->flag = flag;
    newPtr->code = code;
    newPtr->priority = priority;
    newPtr->fn_access = fn_access;
    newPtr->fn_print = fn_print;

    /*=================================================
     * Insert the new item in the list of watch items.
     *=================================================*/

    if( lastPtr == NULL )
    {
        newPtr->next = core_get_watch_data(env)->items;
        core_get_watch_data(env)->items = newPtr;
    }
    else
    {
        newPtr->next = lastPtr->next;
        lastPtr->next = newPtr;
    }

    /*==================================================
     * Return TRUE to indicate the item has been added.
     *==================================================*/

    return(TRUE);
}

/****************************************************
 * core_watch: C access routine for the watch command.
 *****************************************************/
BOOLEAN core_watch(void *env, char *itemName)
{
    return(EnvSetWatchItem(env, itemName, ON, NULL));
}

/********************************************************
 * EnvUnwatch: C access routine for the unwatch command.
 *********************************************************/
BOOLEAN EnvUnwatch(void *env, char *itemName)
{
    return(EnvSetWatchItem(env, itemName, OFF, NULL));
}

/**********************************************************************
 * EnvSetWatchItem: Sets the state of a specified watch item to either
 *   on or off. Returns TRUE if the item was set, otherwise FALSE.
 ***********************************************************************/
int EnvSetWatchItem(void *env, char *itemName, unsigned newState, struct core_expression *argExprs)
{
    struct core_watch_item *wPtr;

    /*======================================================
     * If the new state isn't on or off, then return FALSE.
     *======================================================*/

    if((newState != ON) && (newState != OFF))
    {
        return(FALSE);
    }

    /*===================================================
     * If the name of the watch item to set is all, then
     * all watch items are set to the new state and TRUE
     * is returned.
     *===================================================*/

    if( strcmp(itemName, "all") == 0 )
    {
        for( wPtr = core_get_watch_data(env)->items; wPtr != NULL; wPtr = wPtr->next )
        {
            /*==============================================
             * If no specific arguments are specified, then
             * set the global flag for the watch item.
             *==============================================*/

            if( argExprs == NULL )
            {
                *(wPtr->flag) = newState;
            }

            /*=======================================
             * Set flags for individual watch items.
             *=======================================*/

            if((wPtr->fn_access == NULL) ? FALSE :
               ((*wPtr->fn_access)(env, wPtr->code, newState, argExprs) == FALSE))
            {
                core_set_eval_error(env, TRUE);
                return(FALSE);
            }
        }

        return(TRUE);
    }

    /*=================================================
     * Search for the watch item to be set in the list
     * of watch items. If found, set the watch item to
     * its new state and return TRUE.
     *=================================================*/

    for( wPtr = core_get_watch_data(env)->items; wPtr != NULL; wPtr = wPtr->next )
    {
        if( strcmp(itemName, wPtr->name) == 0 )
        {
            /*==============================================
             * If no specific arguments are specified, then
             * set the global flag for the watch item.
             *==============================================*/

            if( argExprs == NULL )
            {
                *(wPtr->flag) = newState;
            }

            /*=======================================
             * Set flags for individual watch items.
             *=======================================*/

            if((wPtr->fn_access == NULL) ? FALSE :
               ((*wPtr->fn_access)(env, wPtr->code, newState, argExprs) == FALSE))
            {
                core_set_eval_error(env, TRUE);
                return(FALSE);
            }

            return(TRUE);
        }
    }

    /*=================================================
     * If the specified item was not found in the list
     * of watchable items then return FALSE.
     *=================================================*/

    return(FALSE);
}

/*****************************************************************
 * EnvGetWatchItem: Gets the current state of the specified watch
 *   item. Returns the state of the watch item (0 for off and 1
 *   for on) if the watch item is found in the list of watch
 *   items, otherwise -1 is returned.
 ******************************************************************/
int EnvGetWatchItem(void *env, char *itemName)
{
    struct core_watch_item *wPtr;

    for( wPtr = core_get_watch_data(env)->items; wPtr != NULL; wPtr = wPtr->next )
    {
        if( strcmp(itemName, wPtr->name) == 0 )
        {
            return((int)*(wPtr->flag));
        }
    }

    return(-1);
}

/***************************************************************
 * ValidWatchItem: Returns TRUE if the specified name is found
 *   in the list of watch items, otherwise returns FALSE.
 ****************************************************************/
static struct core_watch_item *ValidWatchItem(void *env, char *itemName, int *recognized)
{
    struct core_watch_item *wPtr;

    *recognized = TRUE;

    if( strcmp(itemName, "all") == 0 )
    {
        return(NULL);
    }

    for( wPtr = core_get_watch_data(env)->items; wPtr != NULL; wPtr = wPtr->next )
    {
        if( strcmp(itemName, wPtr->name) == 0 ) {return(wPtr);}
    }

    *recognized = FALSE;
    return(NULL);
}

/************************************************************
 * GetNthWatchName: Returns the name associated with the nth
 *   item in the list of watchable items. If the nth item
 *   does not exist, then NULL is returned.
 *************************************************************/
char *GetNthWatchName(void *env, int whichItem)
{
    int i;
    struct core_watch_item *wPtr;

    for( wPtr = core_get_watch_data(env)->items, i = 1;
         wPtr != NULL;
         wPtr = wPtr->next, i++ )
    {
        if( i == whichItem )
        {
            return(wPtr->name);
        }
    }

    return(NULL);
}

/**************************************************************
 * GetNthWatchValue: Returns the current state associated with
 *   the nth item in the list of watchable items. If the nth
 *   item does not exist, then -1 is returned.
 ***************************************************************/
int GetNthWatchValue(void *env, int whichItem)
{
    int i;
    struct core_watch_item *wPtr;

    for( wPtr = core_get_watch_data(env)->items, i = 1;
         wPtr != NULL;
         wPtr = wPtr->next, i++ )
    {
        if( i == whichItem )
        {
            return((int)*(wPtr->flag));
        }
    }

    return(-1);
}

/*************************************
 * WatchCommand: H/L access routine
 *   for the watch command.
 **************************************/
void WatchCommand(void *env)
{
    core_data_object val;
    char *argument;
    int recognized;
    struct core_watch_item *wPtr;

    /*========================================
     * Determine which item is to be watched.
     *========================================*/

    if( core_check_arg_type(env, "watch", 1, ATOM, &val) == FALSE )
    {
        return;
    }

    argument = core_convert_data_to_string(val);
    wPtr = ValidWatchItem(env, argument, &recognized);

    if( recognized == FALSE )
    {
        core_set_eval_error(env, TRUE);
        report_explicit_type_error(env, "watch", 1, "watchable symbol");
        return;
    }

    /*=================================================
     * Check to make sure extra arguments are allowed.
     *=================================================*/

    if( core_get_next_arg(core_get_first_arg()) != NULL )
    {
        if((wPtr == NULL) ? TRUE : (wPtr->fn_access == NULL))
        {
            core_set_eval_error(env, TRUE);
            report_arg_count_error(env, "watch", EXACTLY, 1);
            return;
        }
    }

    /*=====================
     * Set the watch item.
     *=====================*/

    EnvSetWatchItem(env, argument, ON, core_get_next_arg(core_get_first_arg()));
}

/***************************************
 * UnwatchCommand: H/L access routine
 *   for the unwatch command.
 ****************************************/
void UnwatchCommand(void *env)
{
    core_data_object val;
    char *argument;
    int recognized;
    struct core_watch_item *wPtr;

    /*==========================================
     * Determine which item is to be unwatched.
     *==========================================*/

    if( core_check_arg_type(env, "unwatch", 1, ATOM, &val) == FALSE )
    {
        return;
    }

    argument = core_convert_data_to_string(val);
    wPtr = ValidWatchItem(env, argument, &recognized);

    if( recognized == FALSE )
    {
        core_set_eval_error(env, TRUE);
        report_explicit_type_error(env, "unwatch", 1, "watchable symbol");
        return;
    }

    /*=================================================
     * Check to make sure extra arguments are allowed.
     *=================================================*/

    if( core_get_next_arg(core_get_first_arg()) != NULL )
    {
        if((wPtr == NULL) ? TRUE : (wPtr->fn_access == NULL))
        {
            core_set_eval_error(env, TRUE);
            report_arg_count_error(env, "unwatch", EXACTLY, 1);
            return;
        }
    }

    /*=====================
     * Set the watch item.
     *=====================*/

    EnvSetWatchItem(env, argument, OFF, core_get_next_arg(core_get_first_arg()));
}

/***********************************************
 * ListWatchItemsCommand: H/L access routines
 *   for the list-watch-items command.
 ************************************************/
void ListWatchItemsCommand(void *env)
{
    struct core_watch_item *wPtr;
    core_data_object val;
    int recognized;

    /*=======================
     * List the watch items.
     *=======================*/

    if( core_get_first_arg() == NULL )
    {
        for( wPtr = core_get_watch_data(env)->items; wPtr != NULL; wPtr = wPtr->next )
        {
            print_router(env, WDISPLAY, wPtr->name);

            if( *(wPtr->flag))
            {
                print_router(env, WDISPLAY, " = on\n");
            }
            else
            {
                print_router(env, WDISPLAY, " = off\n");
            }
        }

        return;
    }

    /*=======================================
     * Determine which item is to be listed.
     *=======================================*/

    if( core_check_arg_type(env, "list-watch-items", 1, ATOM, &val) == FALSE )
    {
        return;
    }

    wPtr = ValidWatchItem(env, core_convert_data_to_string(val), &recognized);

    if((recognized == FALSE) || (wPtr == NULL))
    {
        core_set_eval_error(env, TRUE);
        report_explicit_type_error(env, "list-watch-items", 1, "watchable symbol");
        return;
    }

    /*=================================================
     * Check to make sure extra arguments are allowed.
     *=================================================*/

    if((wPtr->fn_print == NULL) &&
       (core_get_next_arg(core_get_first_arg()) != NULL))
    {
        core_set_eval_error(env, TRUE);
        report_arg_count_error(env, "list-watch-items", EXACTLY, 1);
        return;
    }

    /*====================================
     * List the status of the watch item.
     *====================================*/

    print_router(env, WDISPLAY, wPtr->name);

    if( *(wPtr->flag))
    {
        print_router(env, WDISPLAY, " = on\n");
    }
    else
    {
        print_router(env, WDISPLAY, " = off\n");
    }

    /*============================================
     * List the status of individual watch items.
     *============================================*/

    if( wPtr->fn_print != NULL )
    {
        if((*wPtr->fn_print)(env, WDISPLAY, wPtr->code,
                             core_get_next_arg(core_get_first_arg())) == FALSE )
        {
            core_set_eval_error(env, TRUE);
        }
    }
}

/******************************************
 * GetWatchItemCommand: H/L access routine
 *   for the get-watch-item command.
 *******************************************/
int GetWatchItemCommand(void *env)
{
    core_data_object val;
    char *argument;
    int recognized;

    /*============================================
     * Check for the correct number of arguments.
     *============================================*/

    if( core_check_arg_count(env, "get-watch-item", EXACTLY, 1) == -1 )
    {
        return(FALSE);
    }

    /*========================================
     * Determine which item is to be watched.
     *========================================*/

    if( core_check_arg_type(env, "get-watch-item", 1, ATOM, &val) == FALSE )
    {
        return(FALSE);
    }

    argument = core_convert_data_to_string(val);
    ValidWatchItem(env, argument, &recognized);

    if( recognized == FALSE )
    {
        core_set_eval_error(env, TRUE);
        report_explicit_type_error(env, "get-watch-item", 1, "watchable symbol");
        return(FALSE);
    }

    /*===========================
     * Get the watch item value.
     *===========================*/

    if( EnvGetWatchItem(env, argument) == 1 )
    {
        return(TRUE);
    }

    return(FALSE);
}

/************************************************************
 * WatchFunctionDefinitions: Initializes the watch commands.
 *************************************************************/
void WatchFunctionDefinitions(void *env)
{
    core_define_function(env, "watch",   'v', PTR_FN WatchCommand,   "WatchCommand", "1**w");
    core_define_function(env, "unwatch", 'v', PTR_FN UnwatchCommand, "UnwatchCommand", "1**w");
    core_define_function(env, "get-watch-item", 'b', PTR_FN GetWatchItemCommand,   "GetWatchItemCommand", "11w");
    core_define_function(env, "list-watch-items", 'v', PTR_FN ListWatchItemsCommand,
                         "ListWatchItemsCommand", "0**w");

    add_router(env, WTRACE, 1000, RecognizeWatchRouters, CaptureWatchPrints, NULL, NULL, NULL);
    deactivate_router(env, WTRACE);
}

/*************************************************
 * RecognizeWatchRouters: Looks for WTRACE prints
 **************************************************/
#if WIN_BTC
#pragma argsused
#endif
static BOOLEAN RecognizeWatchRouters(void *env, char *logName)
{
#if MAC_MCW || WIN_MCW || MAC_XCD
#pragma unused(env)
#endif

    if( strcmp(logName, WTRACE) == 0 )
    {
        return(TRUE);
    }

    return(FALSE);
}

/*************************************************
 * CaptureWatchPrints: Suppresses WTRACE messages
 **************************************************/
#if WIN_BTC
#pragma argsused
#endif
static int CaptureWatchPrints(void *env, char *logName, char *str)
{
#if MAC_MCW || WIN_MCW || MAC_XCD
#pragma unused(logName)
#pragma unused(str)
#pragma unused(env)
#endif
    return(1);
}

#endif /* DEBUGGING_FUNCTIONS */
