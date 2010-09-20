/* Purpose: Routines for attaching user data to constructs,
 *   facts, instances, user functions, etc.                  */

#define __EXTENSIONS_DATA_SOURCE__

#include <stdlib.h>

#include "setup.h"

#include "core_environment.h"

#include "extensions_data.h"

/************************************************
 * ext_init_data: Allocates environment
 *    data for user data routines.
 *************************************************/
void ext_init_data(void *env)
{
    core_allocate_environment_data(env, USER_DATA_DATA_INDEX, sizeof(struct ext_data_datum), NULL);
}

/*****************************************************
 * ext_install_record: Installs a user data record
 *   in the user data record array and returns the
 *   integer data ID associated with the record.
 ******************************************************/
unsigned char ext_install_record(void *env, struct ext_data_record *theRecord)
{
    theRecord->id = ext_get_data(env)->count;
    ext_get_data(env)->records[ext_get_data(env)->count] = theRecord;
    return(ext_get_data(env)->count++);
}

/****************************************************
 * ext_get_data_item: Searches for user data information
 *   from a list of user data structures. A new user
 *   data structure is created if one is not found.
 *****************************************************/
struct ext_data *ext_get_data_item(void *env, unsigned char userDataID, struct ext_data **list)
{
    struct ext_data *theData;

    for( theData = *list;
         theData != NULL;
         theData = theData->next )
    {
        if( theData->id == userDataID )
        {
            return(theData);
        }
    }

    theData = (struct ext_data *)(*ext_get_data(env)->records[userDataID]->fn_create)(env);
    theData->id = userDataID;
    theData->next = *list;
    *list = theData;

    return(theData);
}

/****************************************************
 * ext_validate_data: Searches for user data information
 *   from a list of user data structures. NULL is
 *   returned if the appropriate user data structure
 *   is not found.
 *****************************************************/
struct ext_data *ext_validate_data(unsigned char userDataID, struct ext_data *list)
{
    struct ext_data *theData;

    for( theData = list;
         theData != NULL;
         theData = theData->next )
    {
        if( theData->id == userDataID )
        {
            return(theData);
        }
    }

    return(NULL);
}

/**************************************************************
 * ext_clear_data: Deallocates a linked list of user data.
 ***************************************************************/
void ext_clear_data(void *env, struct ext_data *list)
{
    struct ext_data *nextData;

    while( list != NULL )
    {
        nextData = list->next;
        (*ext_get_data(env)->records[list->id]->fn_delete)(env, list);
        list = nextData;
    }
}

/************************************************
 * ext_delete_data_item: Removes user data information
 *   from a list of user data structures.
 *************************************************/
struct ext_data *ext_delete_data_item(void *env, unsigned char userDataID, struct ext_data *list)
{
    struct ext_data *theData, *lastData = NULL;

    for( theData = list;
         theData != NULL;
         theData = theData->next )
    {
        if( theData->id == userDataID )
        {
            if( lastData == NULL )
            {
                list = theData->next;
            }
            else
            {
                lastData->next = theData->next;
            }

            (*ext_get_data(env)->records[userDataID]->fn_delete)(env, theData);
            return(list);
        }

        lastData = theData;
    }

    return(list);
}
