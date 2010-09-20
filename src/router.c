/* Purpose: Provides a centralized mechanism for handling
 *   input and output requests.                              */

#define _ROUTER_SOURCE_

#include <stdio.h>
#define _STDIO_INCLUDED_
#include <stdlib.h>
#include <string.h>

#include "setup.h"

#include "core_arguments.h"
#include "constant.h"
#include "core_environment.h"
#include "core_functions.h"
#include "router_file.h"
#include "core_memory.h"
#include "router_string.h"
#include "sysdep.h"

#include "router.h"

/**************************************
 * LOCAL FUNCTION PROTOTYPES
 ***************************************/
static int  _query_router(void *, char *, struct router *);
static void _delete_router_data(void *);
static BOOLEAN _add_contextual_router(void *,
                                      char *, int,
                                      int(*) (void *, char *),
                                      int(*) (void *, char *, char *),
                                      int(*) (void *, char *),
                                      int(*) (void *, int, char *),
                                      int(*) (void *, int),
                                      void *);

/********************************************************
 * init_routers: Initializes output streams.
 *********************************************************/
void init_routers(void *env)
{
    core_allocate_environment_data(env, ROUTER_DATA_INDEX, sizeof(struct router_data), _delete_router_data);

    get_router_data(env)->command_buffer_input_count = 0;
    get_router_data(env)->is_waiting = TRUE;

    core_define_function(env, FUNC_NAME_QUIT, RT_VOID, PTR_FN broccoli_quit, "ExitCommand", FUNC_CNSTR_QUIT);
    init_file_router(env);
    init_string_routers(env);
}

/************************************************
 * DeallocateRouterData: Deallocates environment
 *    data for I/O routers.
 *************************************************/
static void _delete_router_data(void *env)
{
    struct router *tmpPtr, *nextPtr;

    tmpPtr = get_router_data(env)->routers_list;

    while( tmpPtr != NULL )
    {
        nextPtr = tmpPtr->next;
        core_mem_free(env, tmpPtr->name, strlen(tmpPtr->name) + 1);
        core_mem_return_struct(env, router, tmpPtr);
        tmpPtr = nextPtr;
    }
}

/******************************************
 * print_router: Generic print function.
 *******************************************/
int print_router(void *env, char *logicalName, char *str)
{
    struct router *currentPtr;

    /*===================================================
     * If the "fast save" option is being used, then the
     * logical name is actually a pointer to a file and
     * fprintf can be called directly to bypass querying
     * all of the routers.
     *===================================================*/

    if(((char *)get_router_data(env)->fast_save_file_ptr) == logicalName )
    {
        fprintf(get_router_data(env)->fast_save_file_ptr, "%s", str);
        return(2);
    }

    /*==============================================
     * Search through the list of routers until one
     * is found that will handle the print request.
     *==============================================*/

    currentPtr = get_router_data(env)->routers_list;

    while( currentPtr != NULL )
    {
        if((currentPtr->printer != NULL) ? _query_router(env, logicalName, currentPtr) : FALSE )
        {
            core_set_environment_router_context(env, currentPtr->context);

            if( currentPtr->environment_aware )
            {
                (*currentPtr->printer)(env, logicalName, str);
            }
            else
            {
                ((int(*) (char *, char *))(*currentPtr->printer))(logicalName, str);
            }

            return(1);
        }

        currentPtr = currentPtr->next;
    }

    /*=====================================================
     * The logical name was not recognized by any routers.
     *=====================================================*/

    if( strcmp(WERROR, logicalName) != 0 )
    {
        error_unknown_router(env, logicalName);
    }

    return(0);
}

/*************************************************
 * get_ch_router: Generic get character function.
 **************************************************/
int get_ch_router(void *env, char *logicalName)
{
    struct router *currentPtr;
    int inchar;

    /*===================================================
     * If the "fast load" option is being used, then the
     * logical name is actually a pointer to a file and
     * getc can be called directly to bypass querying
     * all of the routers.
     *===================================================*/

    if(((char *)get_router_data(env)->fast_load_file_ptr) == logicalName )
    {
        inchar = getc(get_router_data(env)->fast_load_file_ptr);

        if((inchar == '\r') || (inchar == '\n'))
        {
            if(((char *)get_router_data(env)->fast_load_file_ptr) == get_router_data(env)->line_count_router )
            {
                core_inc_line_count(env);
            }
        }

        /* if (inchar == '\r') return('\n'); */

        return(inchar);
    }

    /*===============================================
     * If the "fast string get" option is being used
     * for the specified logical name, then bypass
     * the router system and extract the character
     * directly from the fast get string.
     *===============================================*/

    if( get_router_data(env)->fast_get_ch == logicalName )
    {
        inchar = (unsigned char)get_router_data(env)->fast_get_str[get_router_data(env)->fast_get_inde];

        get_router_data(env)->fast_get_inde++;

        if( inchar == '\0' )
        {
            return(EOF);
        }

        if((inchar == '\r') || (inchar == '\n'))
        {
            if( get_router_data(env)->fast_get_ch == get_router_data(env)->line_count_router )
            {
                core_inc_line_count(env);
            }
        }

        /* if (inchar == '\r') return('\n'); */

        return(inchar);
    }

    /*==============================================
     * Search through the list of routers until one
     * is found that will handle the getc request.
     *==============================================*/

    currentPtr = get_router_data(env)->routers_list;

    while( currentPtr != NULL )
    {
        if((currentPtr->charget != NULL) ? _query_router(env, logicalName, currentPtr) : FALSE )
        {
            core_set_environment_router_context(env, currentPtr->context);

            if( currentPtr->environment_aware )
            {
                inchar = (*currentPtr->charget)(env, logicalName);
            }
            else
            {
                inchar = ((int(*) (char *))(*currentPtr->charget))(logicalName);
            }

            if((inchar == '\r') || (inchar == '\n'))
            {
                if((get_router_data(env)->line_count_router != NULL) &&
                   (strcmp(logicalName, get_router_data(env)->line_count_router) == 0))
                {
                    core_inc_line_count(env);
                }
            }

            /* if (inchar == '\r') return('\n'); */

            /*
             * if (inchar != '\b')
             * { return(inchar); }
             */
            return(inchar);
        }

        currentPtr = currentPtr->next;
    }

    /*=====================================================
     * The logical name was not recognized by any routers.
     *=====================================================*/

    error_unknown_router(env, logicalName);
    return(-1);
}

/*****************************************************
 * unget_ch_router: Generic unget character function.
 ******************************************************/
int unget_ch_router(void *env, int ch, char *logicalName)
{
    struct router *currentPtr;

    /*===================================================*/
    /* If the "fast load" option is being used, then the */
    /* logical name is actually a pointer to a file and  */
    /* ungetc can be called directly to bypass querying  */
    /* all of the routers.                               */
    /*===================================================*/

    if(((char *)get_router_data(env)->fast_load_file_ptr) == logicalName )
    {
        if((ch == '\r') || (ch == '\n'))
        {
            if(((char *)get_router_data(env)->fast_load_file_ptr) == get_router_data(env)->line_count_router )
            {
                core_dec_line_count(env);
            }
        }

        return(ungetc(ch, get_router_data(env)->fast_load_file_ptr));
    }

    /*===============================================*/
    /* If the "fast string get" option is being used */
    /* for the specified logical name, then bypass   */
    /* the router system and unget the character     */
    /* directly from the fast get string.            */
    /*===============================================*/

    if( get_router_data(env)->fast_get_ch == logicalName )
    {
        if((ch == '\r') || (ch == '\n'))
        {
            if( get_router_data(env)->fast_get_ch == get_router_data(env)->line_count_router )
            {
                core_dec_line_count(env);
            }
        }

        if( get_router_data(env)->fast_get_inde > 0 )
        {
            get_router_data(env)->fast_get_inde--;
        }

        return(ch);
    }

    /*===============================================*/
    /* Search through the list of routers until one  */
    /* is found that will handle the ungetc request. */
    /*===============================================*/

    currentPtr = get_router_data(env)->routers_list;

    while( currentPtr != NULL )
    {
        if((currentPtr->charunget != NULL) ? _query_router(env, logicalName, currentPtr) : FALSE )
        {
            if((ch == '\r') || (ch == '\n'))
            {
                if((get_router_data(env)->line_count_router != NULL) &&
                   (strcmp(logicalName, get_router_data(env)->line_count_router) == 0))
                {
                    core_dec_line_count(env);
                }
            }

            core_set_environment_router_context(env, currentPtr->context);

            if( currentPtr->environment_aware )
            {
                return((*currentPtr->charunget)(env, ch, logicalName));
            }
            else
            {
                return(((int(*) (int, char *))(*currentPtr->charunget))(ch, logicalName));
            }
        }

        currentPtr = currentPtr->next;
    }

    /*=====================================================*/
    /* The logical name was not recognized by any routers. */
    /*=====================================================*/

    error_unknown_router(env, logicalName);
    return(-1);
}

/****************************************************
 * broccoli_quit: H/L command for exiting the program.
 *****************************************************/
void broccoli_quit(void *env)
{
    int argCnt;
    int status;

    if((argCnt = core_check_arg_count(env, "exit", NO_MORE_THAN, 1)) == -1 )
    {
        return;
    }

    if( argCnt == 0 )
    {
        exit_router(env, EXIT_SUCCESS);
    }
    else
    {
        status = 1;

        if( core_get_eval_error(env))
        {
            return;
        }

        exit_router(env, status);
    }

    return;
}

/**********************************************
 * exit_router: Generic exit function. Calls
 *   all of the router exit functions.
 ***********************************************/
void exit_router(void *env, int num)
{
    struct router *currentPtr, *nextPtr;

    get_router_data(env)->abort = FALSE;
    currentPtr = get_router_data(env)->routers_list;

    while( currentPtr != NULL )
    {
        nextPtr = currentPtr->next;

        if( currentPtr->active == TRUE )
        {
            if( currentPtr->exiter != NULL )
            {
                core_set_environment_router_context(env, currentPtr->context);

                if( currentPtr->environment_aware )
                {
                    (*currentPtr->exiter)(env, num);
                }
                else
                {
                    ((int(*) (int))(*currentPtr->exiter))(num);
                }
            }
        }

        currentPtr = nextPtr;
    }

    if( get_router_data(env)->abort )
    {
        return;
    }

    sysdep_exit(env, num);
}

/*******************************************
 * abort_exit: Forces ExitRouter to terminate
 *   after calling all closing routers.
 ********************************************/
void abort_exit(void *env)
{
    get_router_data(env)->abort = TRUE;
}

/***********************************************************
 * add_router: Adds an I/O router to the list of routers.
 ************************************************************/
BOOLEAN add_router(void *env, char *routerName, int priority, int (*queryFunction)(void *, char *), int (*printFunction)(void *, char *, char *), int (*getcFunction)(void *, char *), int (*ungetcFunction)(void *, int, char *), int (*exitFunction)(void *, int))
{
    return(_add_contextual_router(env, routerName, priority,
                                  queryFunction, printFunction, getcFunction,
                                  ungetcFunction, exitFunction, NULL));
}

/**********************************************************************
 * add_contextual_router: Adds an I/O router to the list of routers.
 ***********************************************************************/
BOOLEAN _add_contextual_router(void *env, char *routerName, int priority, int (*queryFunction)(void *, char *), int (*printFunction)(void *, char *, char *), int (*getcFunction)(void *, char *), int (*ungetcFunction)(void *, int, char *), int (*exitFunction)(void *, int), void *context)
{
    struct router *newPtr, *lastPtr, *currentPtr;
    char  *nameCopy;

    newPtr = core_mem_get_struct(env, router);

    nameCopy = (char *)core_mem_alloc(env, strlen(routerName) + 1);
    sysdep_strcpy(nameCopy, routerName);
    newPtr->name = nameCopy;

    newPtr->active = TRUE;
    newPtr->environment_aware = TRUE;
    newPtr->context = context;
    newPtr->priority = priority;
    newPtr->query = queryFunction;
    newPtr->printer = printFunction;
    newPtr->exiter = exitFunction;
    newPtr->charget = getcFunction;
    newPtr->charunget = ungetcFunction;
    newPtr->next = NULL;

    if( get_router_data(env)->routers_list == NULL )
    {
        get_router_data(env)->routers_list = newPtr;
        return(1);
    }

    lastPtr = NULL;
    currentPtr = get_router_data(env)->routers_list;

    while((currentPtr != NULL) ? (priority < currentPtr->priority) : FALSE )
    {
        lastPtr = currentPtr;
        currentPtr = currentPtr->next;
    }

    if( lastPtr == NULL )
    {
        newPtr->next = get_router_data(env)->routers_list;
        get_router_data(env)->routers_list = newPtr;
    }
    else
    {
        newPtr->next = currentPtr;
        lastPtr->next = newPtr;
    }

    return(1);
}

/****************************************************************
 * delete_router: Removes an I/O router from the list of routers.
 *****************************************************************/
int delete_router(void *env, char *routerName)
{
    struct router *currentPtr, *lastPtr;

    currentPtr = get_router_data(env)->routers_list;
    lastPtr = NULL;

    while( currentPtr != NULL )
    {
        if( strcmp(currentPtr->name, routerName) == 0 )
        {
            core_mem_free(env, currentPtr->name, strlen(currentPtr->name) + 1);

            if( lastPtr == NULL )
            {
                get_router_data(env)->routers_list = currentPtr->next;
                core_mem_release(env, currentPtr, (int)sizeof(struct router));
                return(1);
            }

            lastPtr->next = currentPtr->next;
            core_mem_release(env, currentPtr, (int)sizeof(struct router));
            return(1);
        }

        lastPtr = currentPtr;
        currentPtr = currentPtr->next;
    }

    return(0);
}

/********************************************************************
 * query_router: Determines if any router recognizes a logical name.
 *********************************************************************/
int query_router(void *env, char *logicalName)
{
    struct router *currentPtr;

    currentPtr = get_router_data(env)->routers_list;

    while( currentPtr != NULL )
    {
        if( _query_router(env, logicalName, currentPtr) == TRUE )
        {
            return(TRUE);
        }

        currentPtr = currentPtr->next;
    }

    return(FALSE);
}

/***********************************************
 * QueryRouter: Determines if a specific router
 *    recognizes a logical name.
 ************************************************/
static int _query_router(void *env, char *logicalName, struct router *currentPtr)
{
    /*===================================================*/
    /* If the router is inactive, then it can't respond. */
    /*===================================================*/

    if( currentPtr->active == FALSE )
    {
        return(FALSE);
    }

    /*=============================================================*/
    /* If the router has no query function, then it can't respond. */
    /*=============================================================*/

    if( currentPtr->query == NULL )
    {
        return(FALSE);
    }

    /*=========================================*/
    /* Call the router's query function to see */
    /* if it recognizes the logical name.      */
    /*=========================================*/

    core_set_environment_router_context(env, currentPtr->context);

    if( currentPtr->environment_aware )
    {
        if((*currentPtr->query)(env, logicalName) == TRUE )
        {
            return(TRUE);
        }
    }
    else
    {
        if(((int(*) (char *))(*currentPtr->query))(logicalName) == TRUE )
        {
            return(TRUE);
        }
    }

    return(FALSE);
}

/******************************************************
 * deactivate_router: Deactivates a specific router.
 *******************************************************/
int deactivate_router(void *env, char *routerName)
{
    struct router *currentPtr;

    currentPtr = get_router_data(env)->routers_list;

    while( currentPtr != NULL )
    {
        if( strcmp(currentPtr->name, routerName) == 0 )
        {
            currentPtr->active = FALSE;
            return(TRUE);
        }

        currentPtr = currentPtr->next;
    }

    return(FALSE);
}

/**************************************************
 * activate_router: Activates a specific router.
 ***************************************************/
int activate_router(void *env, char *routerName)
{
    struct router *currentPtr;

    currentPtr = get_router_data(env)->routers_list;

    while( currentPtr != NULL )
    {
        if( strcmp(currentPtr->name, routerName) == 0 )
        {
            currentPtr->active = TRUE;
            return(TRUE);
        }

        currentPtr = currentPtr->next;
    }

    return(FALSE);
}

/*******************************************************
 * set_fast_load: Used to bypass router system for loads.
 ********************************************************/
void set_fast_load(void *env, FILE *filePtr)
{
    get_router_data(env)->fast_load_file_ptr = filePtr;
}

/*******************************************************
 * set_fast_save: Used to bypass router system for saves.
 ********************************************************/
void set_fast_save(void *env, FILE *filePtr)
{
    get_router_data(env)->fast_save_file_ptr = filePtr;
}

/*****************************************************
 * get_fast_load: Returns the "fast load" file pointer.
 ******************************************************/
FILE *get_fast_load(void *env)
{
    return(get_router_data(env)->fast_load_file_ptr);
}

/*****************************************************
 * get_fast_save: Returns the "fast save" file pointer.
 ******************************************************/
FILE *get_fast_save(void *env)
{
    return(get_router_data(env)->fast_save_file_ptr);
}

/****************************************************
 * error_unknown_router: Standard error message
 *   for an unrecognized router name.
 *****************************************************/
void error_unknown_router(void *env, char *logicalName)
{
    error_print_id(env, "ROUTER", 1, FALSE);
    print_router(env, WERROR, "Logical name ");
    print_router(env, WERROR, logicalName);
    print_router(env, WERROR, " was not recognized.\n");
}
