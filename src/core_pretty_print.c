/* Purpose: Routines for processing the pretty print
 *   representation of constructs.                           */

#define __CORE_PRETTY_PRINT_SOURCE__

#include <stdio.h>
#define _STDIO_INCLUDED_
#include <string.h>
#include <ctype.h>

#include "setup.h"

#include "constant.h"
#include "core_environment.h"
#include "core_memory.h"
#include "sysdep.h"
#include "core_gc.h"

#include "core_pretty_print.h"

/**************************************
 * LOCAL FUNCTION PROTOTYPES
 ***************************************/

static void _delete_pp_buffer(void *);

/***************************************************
 * core_init_pp_buffer: Allocates environment
 *    data for pretty print routines.
 ****************************************************/
void core_init_pp_buffer(void *env)
{
    core_allocate_environment_data(env, PRETTY_PRINT_DATA_INDE, sizeof(struct core_pp_buffer), _delete_pp_buffer);

    core_pp_get_buffer(env)->is_enabled = TRUE;
}

/*****************************************************
 * DeallocatePrettyPrintData: Deallocates environment
 *    data for the pretty print routines.
 ******************************************************/
static void _delete_pp_buffer(void *env)
{
    if( core_pp_get_buffer(env)->data != NULL )
    {
        core_mem_release(env, core_pp_get_buffer(env)->data, core_pp_get_buffer(env)->max);
    }
}

/******************************************************
 * core_flush_pp_buffer: Resets the pretty print save buffer.
 *******************************************************/
void core_flush_pp_buffer(void *env)
{
    if( core_pp_get_buffer(env)->data == NULL )
    {
        return;
    }

    core_pp_get_buffer(env)->lookback1 = 0;
    core_pp_get_buffer(env)->lookback2 = 0;
    core_pp_get_buffer(env)->position = 0;
    core_pp_get_buffer(env)->data[0] = EOS;
    return;
}

/********************************************************************
 * core_delete_pp_buffer: Resets and removes the pretty print save buffer.
 *********************************************************************/
void core_delete_pp_buffer(void *env)
{
    core_pp_get_buffer(env)->lookback1 = 0;
    core_pp_get_buffer(env)->lookback2 = 0;
    core_pp_get_buffer(env)->position = 0;

    if( core_pp_get_buffer(env)->data != NULL )
    {
        core_mem_release(env, core_pp_get_buffer(env)->data, core_pp_get_buffer(env)->max);
    }

    core_pp_get_buffer(env)->data = NULL;
    core_pp_get_buffer(env)->max = 0;
}

/********************************************
 * core_save_pp_buffer: Appends a string to the end
 *   of the pretty print save buffer.
 *********************************************/
void core_save_pp_buffer(void *env, char *str)
{
    size_t increment;

    /*==========================================
     * If the pretty print buffer isn't is_needed,
     * then don't bother writing to it.
     *==========================================*/

    if((core_pp_get_buffer(env)->status == OFF) || (!core_pp_get_buffer(env)->is_enabled))
    {
        return;
    }

    /*===============================
     * Determine the increment size.
     *===============================*/

    increment = 512;

    if( core_pp_get_buffer(env)->position > increment )
    {
        increment = core_pp_get_buffer(env)->position * 3;
    }

    /*================================================
     * If the pretty print buffer isn't big enough to
     * contain the string, then increase its size.
     *================================================*/

    if( strlen(str) + core_pp_get_buffer(env)->position + 1 >= core_pp_get_buffer(env)->max )
    {
        core_pp_get_buffer(env)->data =
            (char *)core_mem_realloc(env, core_pp_get_buffer(env)->data,
                                     core_pp_get_buffer(env)->max,
                                     core_pp_get_buffer(env)->max + increment);
        core_pp_get_buffer(env)->max += increment;
    }

    /*==================================================
     * Remember the previous tokens saved to the pretty
     * print buffer in case it is necessary to back up.
     *==================================================*/

    core_pp_get_buffer(env)->lookback2 = core_pp_get_buffer(env)->lookback1;
    core_pp_get_buffer(env)->lookback1 = core_pp_get_buffer(env)->position;

    /*=============================================
     * Save the string to the pretty print buffer.
     *=============================================*/

    core_pp_get_buffer(env)->data = core_gc_append_to_string(env, str, core_pp_get_buffer(env)->data, &core_pp_get_buffer(env)->position, &core_pp_get_buffer(env)->max);
}

/**************************************************
 * core_backup_pp_buffer:  Removes the last string added to the
 *   pretty print save buffer. Only capable of
 *   backing up for the two most recent additions.
 ***************************************************/
void core_backup_pp_buffer(void *env)
{
    if((core_pp_get_buffer(env)->status == OFF) ||
       (core_pp_get_buffer(env)->data == NULL) ||
       (!core_pp_get_buffer(env)->is_enabled))
    {
        return;
    }

    core_pp_get_buffer(env)->position = core_pp_get_buffer(env)->lookback1;
    core_pp_get_buffer(env)->lookback1 = core_pp_get_buffer(env)->lookback2;
    core_pp_get_buffer(env)->data[core_pp_get_buffer(env)->position] = EOS;
}

/*************************************************
 * core_copy_pp_buffer: Makes a copy of the pretty print
 *   save buffer.
 **************************************************/
char *core_copy_pp_buffer(void *env)
{
    size_t length;
    char *newString;

    length = (1 + strlen(core_pp_get_buffer(env)->data)) * (int)sizeof(char);
    newString = (char *)core_mem_alloc_no_init(env, length);

    sysdep_strcpy(newString, core_pp_get_buffer(env)->data);
    return(newString);
}

/***********************************************************
 * core_get_pp_buffer_handle: Returns a pointer to the data.
 ************************************************************/
char *core_get_pp_buffer_handle(void *env)
{
    return(core_pp_get_buffer(env)->data);
}

/******************************************
 * core_inject_ws_into_pp_buffer: Prints white spaces into
 *   the pretty print buffer.
 *******************************************/
void core_inject_ws_into_pp_buffer(void *env)
{
    int i;
    char buffer[120];

    if((core_pp_get_buffer(env)->status == OFF) ||
       (!core_pp_get_buffer(env)->is_enabled))
    {
        return;
    }

    buffer[0] = '\n';

    for( i = 1 ; i <= core_pp_get_buffer(env)->space_depth ; i++ )
    {
        buffer[i] = ' ';
    }

    buffer[i] = EOS;

    core_save_pp_buffer(env, buffer);
}

/***********************************************
 * core_increment_pp_buffer_depth: Increments indentation
 *   depth for pretty printing.
 ************************************************/
void core_increment_pp_buffer_depth(void *env, int value)
{
    core_pp_get_buffer(env)->space_depth += value;
}

/***********************************************
 * core_decrement_pp_buffer_depth: Decrements indentation
 *   depth for pretty printing.
 ************************************************/
void core_decrement_pp_buffer_depth(void *env, int value)
{
    core_pp_get_buffer(env)->space_depth -= value;
}

/***********************************
 * core_pp_indent_depth: Sets indentation
 *   depth for pretty printing.
 ************************************/
void core_pp_indent_depth(void *env, int value)
{
    core_pp_get_buffer(env)->space_depth = value;
}

/*****************************************
 * core_set_pp_buffer_status: Sets status
 *   flag to boolean value of ON or OFF.
 ******************************************/
void core_set_pp_buffer_status(void *env, int value)
{
    core_pp_get_buffer(env)->status = value;
}

/***********************************
 * core_get_pp_buffer_status: Returns value
 *   of the status flag.
 ************************************/
int core_get_pp_buffer_status(void *env)
{
    return(core_pp_get_buffer(env)->status);
}

/*****************************************
 * core_set_pp_buffer_enabled:
 ******************************************/
int core_set_pp_buffer_enabled(void *env, int value)
{
    int oldValue;

    oldValue = core_pp_get_buffer(env)->is_enabled;
    core_pp_get_buffer(env)->is_enabled = value;
    return(oldValue);
}

/***********************************
 * core_get_pp_buffer_enabled:
 ************************************/
int core_get_pp_buffer_enabled(void *env)
{
    return(core_pp_get_buffer(env)->is_enabled);
}
