/* Purpose: Provides a set of utility functions useful to
 *   other modules. Primarily these are the functions for
 *   handling periodic garbage collection and appending
 *   string data.                                            */

#define __CORE_GC_SOURCE__

#include "setup.h"

#include <ctype.h>
#include <stdlib.h>

#include <stdio.h>
#define _STDIO_INCLUDED_
#include <string.h>

#include "core_environment.h"
#include "core_evaluation.h"
#include "core_memory.h"
#include "type_list.h"
#include "core_utilities.h"
#include "sysdep.h"

#include "core_gc.h"

#define MAX_GENERATION_COUNT 1000L
#define MAX_GENERATION_SZ    10240L
#define COUNT_INCREMENT      1000L
#define SIZE_INCREMENT       10240L

/**************************************
 * LOCAL FUNCTION PROTOTYPES
 ***************************************/

static void _delete_gc_data(void *);
static struct core_gc_call_function *_add_contextual_call_function(void *, char *, int, void(*) (void *), struct core_gc_call_function *, BOOLEAN, void *);


/***********************************************
 * core_init_gc_data: Allocates environment
 *    data for utility routines.
 ************************************************/
void core_init_gc_data(void *env)
{
    core_allocate_environment_data(env, UTILITY_DATA_INDEX, sizeof(struct core_gc_data), _delete_gc_data);

    core_get_gc_data(env)->gc_locks = 0;
    core_get_gc_data(env)->is_using_gc_heuristics = TRUE;
    core_get_gc_data(env)->is_using_periodic_functions = TRUE;
    core_get_gc_data(env)->is_using_yield_function = TRUE;

    core_get_gc_data(env)->generational_item_count_max = MAX_GENERATION_COUNT;
    core_get_gc_data(env)->generational_item_sz_max = MAX_GENERATION_SZ;
    core_get_gc_data(env)->last_eval_depth = -1;
}

/*************************************************
 * DeallocateUtilityData: Deallocates environment
 *    data for utility routines.
 **************************************************/
static void _delete_gc_data(void *env)
{
    struct core_gc_call_function *tmpPtr, *nextPtr;
    struct core_gc_tracked_memory *tmpTM, *nextTM;

    tmpTM = core_get_gc_data(env)->tracked_memory;

    while( tmpTM != NULL )
    {
        nextTM = tmpTM->next;
        core_mem_free(env, tmpTM->memory, tmpTM->size);
        core_mem_return_struct(env, core_gc_tracked_memory, tmpTM);
        tmpTM = nextTM;
    }

    tmpPtr = core_get_gc_data(env)->fn_periodic_list;

    while( tmpPtr != NULL )
    {
        nextPtr = tmpPtr->next;
        core_mem_return_struct(env, core_gc_call_function, tmpPtr);
        tmpPtr = nextPtr;
    }

    tmpPtr = core_get_gc_data(env)->fn_cleanup_list;

    while( tmpPtr != NULL )
    {
        nextPtr = tmpPtr->next;
        core_mem_return_struct(env, core_gc_call_function, tmpPtr);
        tmpPtr = nextPtr;
    }
}

/************************************************************
 * core_gc_periodic_cleanup: Returns garbage created during execution
 *   that has not been returned to the memory pool yet. The
 *   cleanup is normally deferred so that an executing rule
 *   can still access these data structures. Always calls a
 *   series of functions that should be called periodically.
 *   Usually used by interfaces to update displays.
 *************************************************************/
void core_gc_periodic_cleanup(void *env, BOOLEAN cleanupAllDepths, BOOLEAN useHeuristics)
{
    int oldDepth = -1;
    struct core_gc_call_function *cleanupPtr, *periodPtr;

    /*===================================
     * Don't use heuristics if disabled.
     *===================================*/

    if( !core_get_gc_data(env)->is_using_gc_heuristics )
    {
        useHeuristics = FALSE;
    }

    /*=============================================
     * Call functions for handling periodic tasks.
     *=============================================*/

    if( core_get_gc_data(env)->is_using_periodic_functions )
    {
        for( periodPtr = core_get_gc_data(env)->fn_periodic_list;
             periodPtr != NULL;
             periodPtr = periodPtr->next )
        {
            if( periodPtr->environment_aware )
            {
                (*periodPtr->func)(env);
            }
            else
            {
                (*(void(*) (void))periodPtr->func)();
            }
        }
    }

    /*===================================================
     * If the last level we performed cleanup was deeper
     * than the current level, reset the values used by
     * the heuristics to determine if garbage collection
     * should be performed. If the heuristic values had
     * to be incremented because there was no garbage
     * that could be cleaned up, we don't want to keep
     * those same high values permanently so we reset
     * them when we go back to a lower evaluation depth.
     *===================================================*/

    if( core_get_gc_data(env)->last_eval_depth > core_get_evaluation_data(env)->eval_depth )
    {
        core_get_gc_data(env)->last_eval_depth = core_get_evaluation_data(env)->eval_depth;
        core_get_gc_data(env)->generational_item_count_max = MAX_GENERATION_COUNT;
        core_get_gc_data(env)->generational_item_sz_max = MAX_GENERATION_SZ;
    }

    /*======================================================
     * If we're using heuristics to determine if garbage
     * collection to occur, then check to see if enough
     * garbage has been created to make cleanup worthwhile.
     *======================================================*/

    if( core_get_gc_data(env)->gc_locks > 0 )
    {
        return;
    }

    if( useHeuristics &&
        (core_get_gc_data(env)->generational_item_count < core_get_gc_data(env)->generational_item_count_max) &&
        (core_get_gc_data(env)->generational_item_sz < core_get_gc_data(env)->generational_item_sz_max))
    {
        return;
    }

    /*==========================================================
     * If cleanup is being performed at all depths, rather than
     * just the current evaluation depth, then temporarily set
     * the evaluation depth to a level that will force cleanup
     * at all depths.
     *==========================================================*/

    if( cleanupAllDepths )
    {
        oldDepth = core_get_evaluation_data(env)->eval_depth;
        core_get_evaluation_data(env)->eval_depth = -1;
    }

    /*=============================================
     * Free up list values no longer in use.
     *=============================================*/

    flush_lists(env);

    /*=====================================
     * Call the list of cleanup functions.
     *=====================================*/

    for( cleanupPtr = core_get_gc_data(env)->fn_cleanup_list;
         cleanupPtr != NULL;
         cleanupPtr = cleanupPtr->next )
    {
        if( cleanupPtr->environment_aware )
        {
            (*cleanupPtr->func)(env);
        }
        else
        {
            (*(void(*) (void))cleanupPtr->func)();
        }
    }

    /*================================================
     * Free up atomic values that are no longer used.
     *================================================*/

    remove_ephemeral_atoms(env);

    /*=========================================
     * Restore the evaluation depth if cleanup
     * was performed on all depths.
     *=========================================*/

    if( cleanupAllDepths )
    {
        core_get_evaluation_data(env)->eval_depth = oldDepth;
    }

    /*============================================================
     * If very little memory was freed up, then increment the
     * values used by the heuristics so that we don't continually
     * try to free up memory that isn't being released.
     *============================================================*/

    if((core_get_gc_data(env)->generational_item_count + COUNT_INCREMENT) > core_get_gc_data(env)->generational_item_count_max )
    {
        core_get_gc_data(env)->generational_item_count_max = core_get_gc_data(env)->generational_item_count + COUNT_INCREMENT;
    }

    if((core_get_gc_data(env)->generational_item_sz + SIZE_INCREMENT) > core_get_gc_data(env)->generational_item_sz_max )
    {
        core_get_gc_data(env)->generational_item_sz_max = core_get_gc_data(env)->generational_item_sz + SIZE_INCREMENT;
    }

    /*===============================================================
     * Remember the evaluation depth at which garbage collection was
     * last performed. This information is used for resetting the
     * is_ephemeral count and size numbers used by the heuristics.
     *===============================================================*/

    core_get_gc_data(env)->last_eval_depth = core_get_evaluation_data(env)->eval_depth;
}

/**************************************************
 * core_gc_add_cleanup_function: Adds a function to the list
 *   of functions called to perform cleanup such
 *   as returning free memory to the memory pool.
 ***************************************************/
BOOLEAN core_gc_add_cleanup_function(void *env, char *name, void (*func)(void *), int priority)
{
    core_get_gc_data(env)->fn_cleanup_list =
        core_gc_add_call_function(env, name, priority,
                                  (void(*) (void *))func,
                                  core_get_gc_data(env)->fn_cleanup_list, TRUE);
    return(1);
}

/****************************************************
 * core_gc_string_printform: Generates printed representation
 *   of a string. Replaces / with // and " with /".
 *****************************************************/
char *core_gc_string_printform(void *env, char *str)
{
    int i = 0;
    size_t pos = 0;
    size_t max = 0;
    char *theString = NULL;
    void *thePtr;

    theString = core_gc_expand_string(env, '"', theString, &pos, &max, max + 80);

    while( str[i] != EOS )
    {
        if((str[i] == '"') || (str[i] == '\\'))
        {
            theString = core_gc_expand_string(env, '\\', theString, &pos, &max, max + 80);
            theString = core_gc_expand_string(env, str[i], theString, &pos, &max, max + 80);
        }
        else
        {
            theString = core_gc_expand_string(env, str[i], theString, &pos, &max, max + 80);
        }

        i++;
    }

    theString = core_gc_expand_string(env, '"', theString, &pos, &max, max + 80);

    thePtr = store_atom(env, theString);
    core_mem_release(env, theString, max);
    return(to_string(thePtr));
}

/**********************************************************
 * core_gc_append_strings: Appends two strings together. The string
 *   created is added to the symbol_table, so it is not
 *   necessary to deallocate the string returned.
 ***********************************************************/
char *core_gc_append_strings(void *env, char *str1, char *str2)
{
    size_t pos = 0;
    size_t max = 0;
    char *theString = NULL;
    void *thePtr;

    theString = core_gc_append_to_string(env, str1, theString, &pos, &max);
    theString = core_gc_append_to_string(env, str2, theString, &pos, &max);

    thePtr = store_atom(env, theString);
    core_mem_release(env, theString, max);
    return(to_string(thePtr));
}

/*****************************************************
 * core_gc_append_to_string: Appends a string to another string
 *   (expanding the other string if necessary).
 ******************************************************/
char *core_gc_append_to_string(void *env, char *appendStr, char *oldStr, size_t *oldPos, size_t *oldMax)
{
    size_t length;

    /*=========================================
     * Expand the old string so it can contain
     * the new string (if necessary).
     *=========================================*/

    length = strlen(appendStr);

    if( length + *oldPos + 1 > *oldMax )
    {
        oldStr = (char *)core_mem_realloc(env, oldStr, *oldMax, length + *oldPos + 1);
        *oldMax = length + *oldPos + 1;
    }

    /*==============================================================
     * Return NULL if the old string was not successfully expanded.
     *==============================================================*/

    if( oldStr == NULL )
    {
        return(NULL);
    }

    /*===============================================
     * Append the new string to the expanded string.
     *===============================================*/

    sysdep_strcpy(&oldStr[*oldPos], appendStr);
    *oldPos += (int)length;

    /*============================================================
     * Return the expanded string containing the appended string.
     *============================================================*/

    return(oldStr);
}

/******************************************************
 * core_gc_append_subset_to_string: Appends a string to another string
 *   (expanding the other string if necessary). Only a
 *   specified number of characters are appended from
 *   the string.
 *******************************************************/
char *core_gc_append_subset_to_string(void *env, char *appendStr, char *oldStr, size_t length, size_t *oldPos, size_t *oldMax)
{
    size_t lengthWithEOS;

    /*====================================
     * Determine the number of characters
     * to be appended from the string.
     *====================================*/

    if( appendStr[length - 1] != '\0' )
    {
        lengthWithEOS = length + 1;
    }
    else
    {
        lengthWithEOS = length;
    }

    /*=========================================
     * Expand the old string so it can contain
     * the new string (if necessary).
     *=========================================*/

    if( lengthWithEOS + *oldPos > *oldMax )
    {
        oldStr = (char *)core_mem_realloc(env, oldStr, *oldMax, *oldPos + lengthWithEOS);
        *oldMax = *oldPos + lengthWithEOS;
    }

    /*==============================================================
     * Return NULL if the old string was not successfully expanded.
     *==============================================================*/

    if( oldStr == NULL )
    {
        return(NULL);
    }

    /*==================================
     * Append N characters from the new
     * string to the expanded string.
     *==================================*/

    sysdep_strncpy(&oldStr[*oldPos], appendStr, length);
    *oldPos += (lengthWithEOS - 1);
    oldStr[*oldPos] = '\0';

    /*============================================================
     * Return the expanded string containing the appended string.
     *============================================================*/

    return(oldStr);
}

/******************************************************
 * core_gc_expand_string: Adds a character to a string,
 *   reallocating space for the string if it needs to
 *   be enlarged. The backspace character causes the
 *   size of the string to reduced if it is "added" to
 *   the string.
 *******************************************************/
char *core_gc_expand_string(void *env, int inchar, char *str, size_t *pos, size_t *max, size_t newSize)
{
    if((*pos + 1) >= *max )
    {
        str = (char *)core_mem_realloc(env, str, *max, newSize);
        *max = newSize;
    }

    if( inchar != '\b' )
    {
        str[*pos] = (char)inchar;
        (*pos)++;
        str[*pos] = '\0';
    }
    else
    {
        if( *pos > 0 )
        {
            (*pos)--;
        }

        str[*pos] = '\0';
    }

    return(str);
}

/****************************************************************
 * core_gc_add_call_function: Adds a function to a list of functions
 *   which are called to perform certain operations (e.g. clear,
 *   reset, and bload functions).
 *****************************************************************/
struct core_gc_call_function *core_gc_add_call_function(void *env, char *name, int priority, void (*func)(void *), struct core_gc_call_function *head, BOOLEAN environmentAware)
{
    return(_add_contextual_call_function(env, name, priority, func, head, environmentAware, NULL));
}

/**********************************************************
 * core_gc_add_contextual_call_function: Adds a function to a
 *   list of functions which are called to perform certain
 *   operations (e.g. clear, reset, and bload functions).
 ***********************************************************/
struct core_gc_call_function *_add_contextual_call_function(void *env, char *name, int priority, void (*func)(void *), struct core_gc_call_function *head, BOOLEAN environmentAware, void *context)
{
    struct core_gc_call_function *newPtr, *currentPtr, *lastPtr = NULL;

    newPtr = core_mem_get_struct(env, core_gc_call_function);

    newPtr->name = name;
    newPtr->func = func;
    newPtr->priority = priority;
    newPtr->environment_aware = (short)environmentAware;
    newPtr->context = context;

    if( head == NULL )
    {
        newPtr->next = NULL;
        return(newPtr);
    }

    currentPtr = head;

    while((currentPtr != NULL) ? (priority < currentPtr->priority) : FALSE )
    {
        lastPtr = currentPtr;
        currentPtr = currentPtr->next;
    }

    if( lastPtr == NULL )
    {
        newPtr->next = head;
        head = newPtr;
    }
    else
    {
        newPtr->next = currentPtr;
        lastPtr->next = newPtr;
    }

    return(head);
}

/****************************************************************
 * core_gc_remove_call_function: Removes a function from a list of
 *   functions which are called to perform certain operations
 *   (e.g. clear, reset, and bload functions).
 *****************************************************************/
struct core_gc_call_function *core_gc_remove_call_function(void *env, char *name, struct core_gc_call_function *head, int *found)
{
    struct core_gc_call_function *currentPtr, *lastPtr;

    *found = FALSE;
    lastPtr = NULL;
    currentPtr = head;

    while( currentPtr != NULL )
    {
        if( strcmp(name, currentPtr->name) == 0 )
        {
            *found = TRUE;

            if( lastPtr == NULL )
            {
                head = currentPtr->next;
            }
            else
            {
                lastPtr->next = currentPtr->next;
            }

            core_mem_return_struct(env, core_gc_call_function, currentPtr);
            return(head);
        }

        lastPtr = currentPtr;
        currentPtr = currentPtr->next;
    }

    return(head);
}

/*************************************************************
 * core_gc_delete_call_list: Removes all functions from a list of
 *   functions which are called to perform certain operations
 *   (e.g. clear, reset, and bload functions).
 **************************************************************/
void core_gc_delete_call_list(void *env, struct core_gc_call_function *list)
{
    struct core_gc_call_function *tmpPtr, *nextPtr;

    tmpPtr = list;

    while( tmpPtr != NULL )
    {
        nextPtr = tmpPtr->next;
        core_mem_return_struct(env, core_gc_call_function, tmpPtr);
        tmpPtr = nextPtr;
    }
}

/*******************************************
 * core_gc_yield_time: Yields time to a user-defined
 *   function. Intended to allow foreground
 *   application responsiveness when
 *   running in the background.
 ********************************************/
void core_gc_yield_time(void *env)
{
    if((core_get_gc_data(env)->fn_yield_time != NULL) && core_get_gc_data(env)->is_using_yield_function )
    {
        (*core_get_gc_data(env)->fn_yield_time)();
    }
}

/*******************************************
 * core_gc_add_tracked_memory:
 ********************************************/
struct core_gc_tracked_memory *core_gc_add_tracked_memory(void *env, void *theMemory, size_t theSize)
{
    struct core_gc_tracked_memory *newPtr;

    newPtr = core_mem_get_struct(env, core_gc_tracked_memory);

    newPtr->prev = NULL;
    newPtr->memory = theMemory;
    newPtr->size = theSize;
    newPtr->next = core_get_gc_data(env)->tracked_memory;
    core_get_gc_data(env)->tracked_memory = newPtr;

    return(newPtr);
}

/*******************************************
 * core_gc_remove_tracked_memory:
 ********************************************/
void core_gc_remove_tracked_memory(void *env, struct core_gc_tracked_memory *theTracker)
{
    if( theTracker->prev == NULL )
    {
        core_get_gc_data(env)->tracked_memory = theTracker->next;
    }
    else
    {
        theTracker->prev->next = theTracker->next;
    }

    if( theTracker->next != NULL )
    {
        theTracker->next->prev = theTracker->prev;
    }

    core_mem_return_struct(env, core_gc_tracked_memory, theTracker);
}
