/* Purpose: I/O Router routines which allow strings to be
 *   used as input and output sources.                       */

#define __ROUTER_STRING_SOURCE__

#include <stdio.h>
#define _STDIO_INCLUDED_
#include <stdlib.h>
#include <string.h>

#include "setup.h"

#include "constant.h"
#include "core_environment.h"
#include "core_memory.h"
#include "router.h"
#include "sysdep.h"

#include "router_string.h"

#define READ_STRING  0
#define WRITE_STRING 1

/**************************************
 * LOCAL FUNCTION PROTOTYPES
 ***************************************/

static int                      _find_str(void *, char *);
static int                      _print_str(void *, char *, char *);
static int                      _get_ch_str(void *, char *);
static int                      _unget_ch_str(void *, int, char *);
static struct string_router   * _find_str_router(void *, char *);
static int _create_r_str_source(void *, char *, char *, size_t, size_t);
static void _delete_string_router_data(void *);

/*********************************************************
 * init_string_routers: Initializes string I/O router.
 **********************************************************/
void init_string_routers(void *env)
{
    core_allocate_environment_data(env, STRING_ROUTER_DATA_INDEX, sizeof(struct string_router_data), _delete_string_router_data);

    add_router(env, "string", 0, _find_str, _print_str, _get_ch_str, _unget_ch_str, NULL);
}

/******************************************
 * DeallocateStringRouterData: Deallocates
 *    environment data for string routers.
 *******************************************/
static void _delete_string_router_data(void *env)
{
    struct string_router *tmpPtr, *nextPtr;

    tmpPtr = get_string_router_data(env)->string_router_list;

    while( tmpPtr != NULL )
    {
        nextPtr = tmpPtr->next;
        core_mem_release(env, tmpPtr->name, strlen(tmpPtr->name) + 1);
        core_mem_return_struct(env, string_router, tmpPtr);
        tmpPtr = nextPtr;
    }
}

/************************************************************
 * FindString: Find routine for string router logical names.
 *************************************************************/
static int _find_str(void *env, char *fileid)
{
    struct string_router *head;

    head = get_string_router_data(env)->string_router_list;

    while( head != NULL )
    {
        if( strcmp(head->name, fileid) == 0 )
        {
            return(TRUE);
        }

        head = head->next;
    }

    return(FALSE);
}

/*************************************************
 * PrintString: Print routine for string routers.
 **************************************************/
static int _print_str(void *env, char *logicalName, char *str)
{
    struct string_router *head;

    head = _find_str_router(env, logicalName);

    if( head == NULL )
    {
        error_system(env, "ROUTER", 3);
        exit_router(env, EXIT_FAILURE);
    }

    if( head->rw_type != WRITE_STRING )
    {
        return(1);
    }

    if( head->max_position == 0 )
    {
        return(1);
    }

    if((head->position + 1) >= head->max_position )
    {
        return(1);
    }

    sysdep_strncpy(&head->str[head->position],
                   str, (STD_SIZE)(head->max_position - head->position) - 1);

    head->position += strlen(str);

    return(1);
}

/***********************************************
 * GetcString: Getc routine for string routers.
 ************************************************/
static int _get_ch_str(void *env, char *logicalName)
{
    struct string_router *head;
    int rc;

    head = _find_str_router(env, logicalName);

    if( head == NULL )
    {
        error_system(env, "ROUTER", 1);
        exit_router(env, EXIT_FAILURE);
    }

    if( head->rw_type != READ_STRING )
    {
        return(EOF);
    }

    if( head->position >= head->max_position )
    {
        head->position++;
        return(EOF);
    }

    rc = (unsigned char)head->str[head->position];
    head->position++;

    return(rc);
}

/***************************************************
 * UngetcString: Ungetc routine for string routers.
 ****************************************************/
#if WIN_BTC
#pragma argsused
#endif
static int _unget_ch_str(void *env, int ch, char *logicalName)
{
    struct string_router *head;

#if MAC_MCW || WIN_MCW || MAC_XCD
#pragma unused(ch)
#endif

    head = _find_str_router(env, logicalName);

    if( head == NULL )
    {
        error_system(env, "ROUTER", 2);
        exit_router(env, EXIT_FAILURE);
    }

    if( head->rw_type != READ_STRING )
    {
        return(0);
    }

    if( head->position > 0 )
    {
        head->position--;
    }

    return(1);
}

/***********************************************
 * open_string_source: Opens a new string router.
 ************************************************/
int open_string_source(void *env, char *name, char *str, size_t currentPosition)
{
    size_t maximumPosition;

    if( str == NULL )
    {
        currentPosition = 0;
        maximumPosition = 0;
    }
    else
    {
        maximumPosition = strlen(str);
    }

    return(_create_r_str_source(env, name, str, currentPosition, maximumPosition));
}

/*****************************************************
 * open_text_source: Opens a new string router for text
 *   (which is not NULL terminated).
 ******************************************************/
int open_text_source(void *env, char *name, char *str, size_t currentPosition, size_t maximumPosition)
{
    if( str == NULL )
    {
        currentPosition = 0;
        maximumPosition = 0;
    }

    return(_create_r_str_source(env, name, str, currentPosition, maximumPosition));
}

/*****************************************************************
 * CreateReadStringSource: Creates a new string router for input.
 ******************************************************************/
static int _create_r_str_source(void *env, char *name, char *str, size_t currentPosition, size_t maximumPosition)
{
    struct string_router *newStringRouter;

    if( _find_str_router(env, name) != NULL )
    {
        return(0);
    }

    newStringRouter = core_mem_get_struct(env, string_router);
    newStringRouter->name = (char *)core_mem_alloc_and_init(env, strlen(name) + 1);
    sysdep_strcpy(newStringRouter->name, name);
    newStringRouter->str = str;
    newStringRouter->position = currentPosition;
    newStringRouter->rw_type = READ_STRING;
    newStringRouter->max_position = maximumPosition;
    newStringRouter->next = get_string_router_data(env)->string_router_list;
    get_string_router_data(env)->string_router_list = newStringRouter;

    return(1);
}

/*********************************************
 * close_string_source: Closes a string router.
 **********************************************/
int close_string_source(void *env, char *name)
{
    struct string_router *head, *last;

    last = NULL;
    head = get_string_router_data(env)->string_router_list;

    while( head != NULL )
    {
        if( strcmp(head->name, name) == 0 )
        {
            if( last == NULL )
            {
                get_string_router_data(env)->string_router_list = head->next;
                core_mem_release(env, head->name, strlen(head->name) + 1);
                core_mem_return_struct(env, string_router, head);
                return(1);
            }
            else
            {
                last->next = head->next;
                core_mem_release(env, head->name, strlen(head->name) + 1);
                core_mem_return_struct(env, string_router, head);
                return(1);
            }
        }

        last = head;
        head = head->next;
    }

    return(0);
}

/*****************************************************************
 * open_string_dest: Opens a new string router for printing.
 ******************************************************************/
int open_string_dest(void *env, char *name, char *str, size_t maximumPosition)
{
    struct string_router *newStringRouter;

    if( _find_str_router(env, name) != NULL )
    {
        return(0);
    }

    newStringRouter = core_mem_get_struct(env, string_router);
    newStringRouter->name = (char *)core_mem_alloc_and_init(env, (int)strlen(name) + 1);
    sysdep_strcpy(newStringRouter->name, name);
    newStringRouter->str = str;
    newStringRouter->position = 0;
    newStringRouter->rw_type = WRITE_STRING;
    newStringRouter->max_position = maximumPosition;
    newStringRouter->next = get_string_router_data(env)->string_router_list;
    get_string_router_data(env)->string_router_list = newStringRouter;

    return(1);
}

/**************************************************
 * close_string_dest: Closes a string router.
 ***************************************************/
int close_string_dest(void *env, char *name)
{
    return(close_string_source(env, name));
}

/******************************************************************
 * FindStringRouter: Returns a pointer to the named string router.
 *******************************************************************/
static struct string_router *_find_str_router(void *env, char *name)
{
    struct string_router *head;

    head = get_string_router_data(env)->string_router_list;

    while( head != NULL )
    {
        if( strcmp(head->name, name) == 0 )
        {
            return(head);
        }

        head = head->next;
    }

    return(NULL);
}
