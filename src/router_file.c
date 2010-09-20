/* Purpose: I/O Router routines which allow files to be used
 *   as input and output sources.                            */

#define __ROUTER_FILE_SOURCE__

#include <stdio.h>
#define _STDIO_INCLUDED_
#include <string.h>

#include "setup.h"

#include "constant.h"
#include "core_environment.h"
#include "core_memory.h"
#include "router.h"
#include "sysdep.h"

#include "router_file.h"

/**************************************
 * LOCAL FUNCTION PROTOTYPES
 ***************************************/

static int    _exit_file(void *, int);
static int    _print_file(void *, char *, char *);
static int    _get_ch_file(void *, char *);
static int    _unget_ch_file(void *, int, char *);
static void   _delete_file_router_data(void *);
static FILE * _lookup_file_ptr(void *, char *);

/**************************************************************
 * init_file_router: Initializes file input/output router.
 ***************************************************************/
void init_file_router(void *env)
{
    core_allocate_environment_data(env, FILE_ROUTER_DATA_INDEX, sizeof(struct file_router_data), _delete_file_router_data);

    add_router(env, "fileio", 0, find_file,
               _print_file, _get_ch_file,
               _unget_ch_file, _exit_file);
}

/****************************************
 * DeallocateFileRouterData: Deallocates
 *    environment data for file routers.
 *****************************************/
static void _delete_file_router_data(void *env)
{
    struct file_router *tmpPtr, *nextPtr;

    tmpPtr = get_file_router_data(env)->file_routers_list;

    while( tmpPtr != NULL )
    {
        nextPtr = tmpPtr->next;
        sysdep_close_file(env, tmpPtr->stream);
        core_mem_release(env, tmpPtr->logical_name, strlen(tmpPtr->logical_name) + 1);
        core_mem_return_struct(env, file_router, tmpPtr);
        tmpPtr = nextPtr;
    }
}

/****************************************
 * lookup_file_ptr: Returns a pointer to a file
 *   stream for a given logical name.
 *****************************************/
FILE *_lookup_file_ptr(void *env, char *logicalName)
{
    struct file_router *fptr;

    /*========================================================
     * Check to see if standard input or output is requested.
     *========================================================*/

    if( strcmp(logicalName, "stdout") == 0 )
    {
        return(stdout);
    }
    else if( strcmp(logicalName, "stdin") == 0 )
    {
        return(stdin);
    }
    else if( strcmp(logicalName, WTRACE) == 0 )
    {
        return(stdout);
    }
    else if( strcmp(logicalName, WDIALOG) == 0 )
    {
        return(stdout);
    }
    else if( strcmp(logicalName, WPROMPT) == 0 )
    {
        return(stdout);
    }
    else if( strcmp(logicalName, WDISPLAY) == 0 )
    {
        return(stdout);
    }
    else if( strcmp(logicalName, WERROR) == 0 )
    {
        return(stdout);
    }
    else if( strcmp(logicalName, WWARNING) == 0 )
    {
        return(stdout);
    }

    /*==============================================================
     * Otherwise, look up the logical name on the global file list.
     *==============================================================*/

    fptr = get_file_router_data(env)->file_routers_list;

    while((fptr != NULL) ? (strcmp(logicalName, fptr->logical_name) != 0) : FALSE )
    {
        fptr = fptr->next;
    }

    if( fptr != NULL )
    {
        return(fptr->stream);
    }

    return(NULL);
}

/****************************************************
 * find_file: Find routine for file router logical
 *   names. Returns TRUE if the specified logical
 *   name has an associated file stream (which means
 *   that the logical name can be handled by the
 *   file router). Otherwise, FALSE is returned.
 *****************************************************/
int find_file(void *env, char *logicalName)
{
    if( _lookup_file_ptr(env, logicalName) != NULL )
    {
        return(TRUE);
    }

    return(FALSE);
}

/*******************************************
 * ExitFile:  Exit routine for file router.
 ********************************************/
#if WIN_BTC
#pragma argsused
#endif
static int _exit_file(void *env, int num)
{
#if MAC_MCW || WIN_MCW || MAC_XCD
#pragma unused(num)
#endif
#if IO_FUNCTIONS
    close_all_files(env);
#else
#if MAC_MCW || WIN_MCW || MAC_XCD
#pragma unused(env)
#endif
#endif
    return(1);
}

/********************************************
 * PrintFile: Print routine for file router.
 *********************************************/
static int _print_file(void *env, char *logicalName, char *str)
{
    FILE *fptr;

    fptr = _lookup_file_ptr(env, logicalName);
    fprintf(fptr, "%s", str);
    fflush(fptr);
    return(1);
}

/******************************************
 * GetcFile: Getc routine for file router.
 *******************************************/
static int _get_ch_file(void *env, char *logicalName)
{
    FILE *fptr;
    int theChar;

    fptr = _lookup_file_ptr(env, logicalName);

    theChar = getc(fptr);

    /*=================================================
     * The following code prevents Control-D on UNIX
     * machines from terminating all input from stdin.
     *=================================================*/

    if((fptr == stdin) && (theChar == EOF))
    {
        clearerr(stdin);
    }

    return(theChar);
}

/**********************************************
 * UngetcFile: Ungetc routine for file router.
 ***********************************************/
static int _unget_ch_file(void *env, int ch, char *logicalName)
{
    FILE *fptr;

    fptr = _lookup_file_ptr(env, logicalName);
    return(ungetc(ch, fptr));
}

/********************************************************
 * OpenFile: Opens a file with the specified access mode
 *   and stores the opened stream on the list of files
 *   associated with logical names Returns TRUE if the
 *   file was succesfully opened, otherwise FALSE.
 *********************************************************/
int open_file(void *env, char *fileName, char *accessMode, char *logicalName)
{
    FILE *newstream;
    struct file_router *newRouter;

    /*==================================
     * Make sure the file can be opened
     * with the specified access mode.
     *==================================*/

    if((newstream = sysdep_open_file(env, fileName, accessMode)) == NULL )
    {
        return(FALSE);
    }

    /*===========================
     * Create a new file router.
     *===========================*/

    newRouter = core_mem_get_struct(env, file_router);
    newRouter->logical_name = (char *)core_mem_alloc_no_init(env, strlen(logicalName) + 1);
    sysdep_strcpy(newRouter->logical_name, logicalName);
    newRouter->stream = newstream;

    /*==========================================
     * Add the newly opened file to the list of
     * files associated with logical names.
     *==========================================*/

    newRouter->next = get_file_router_data(env)->file_routers_list;
    get_file_router_data(env)->file_routers_list = newRouter;

    /*==================================
     * Return TRUE to indicate the file
     * was opened successfully.
     *==================================*/

    return(TRUE);
}

/************************************************************
 * close_file: Closes the file associated with the specified
 *   logical name. Returns TRUE if the file was successfully
 *   closed, otherwise FALSE.
 *************************************************************/
int close_file(void *env, char *fid)
{
    struct file_router *fptr, *prev;

    for( fptr = get_file_router_data(env)->file_routers_list, prev = NULL;
         fptr != NULL;
         fptr = fptr->next )
    {
        if( strcmp(fptr->logical_name, fid) == 0 )
        {
            sysdep_close_file(env, fptr->stream);
            core_mem_release(env, fptr->logical_name, strlen(fptr->logical_name) + 1);

            if( prev == NULL )
            {
                get_file_router_data(env)->file_routers_list = fptr->next;
            }
            else
            {
                prev->next = fptr->next;
            }

            core_mem_release(env, fptr, (int)sizeof(struct file_router));

            return(TRUE);
        }

        prev = fptr;
    }

    return(FALSE);
}

/*********************************************
 * close_all_files: Closes all files associated
 *   with a file I/O router. Returns TRUE if
 *   any file was closed, otherwise FALSE.
 **********************************************/
int close_all_files(void *env)
{
    struct file_router *fptr, *prev;

    if( get_file_router_data(env)->file_routers_list == NULL )
    {
        return(FALSE);
    }

    fptr = get_file_router_data(env)->file_routers_list;

    while( fptr != NULL )
    {
        sysdep_close_file(env, fptr->stream);
        prev = fptr;
        core_mem_release(env, fptr->logical_name, strlen(fptr->logical_name) + 1);
        fptr = fptr->next;
        core_mem_release(env, prev, (int)sizeof(struct file_router));
    }

    get_file_router_data(env)->file_routers_list = NULL;

    return(TRUE);
}
