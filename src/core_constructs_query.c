/* Purpose: Contains generic routines for deleting, pretty
 *   printing, finding, obtaining module information,
 *   obtaining lists of constructs, listing constructs, and
 *   manipulation routines.                                  */

#define __CORE_CONSTRUCTS_QUERY_SOURCE__

#include <string.h>

#include "setup.h"

#include "constant.h"
#include "core_environment.h"
#include "core_memory.h"
#include "modules_init.h"
#include "core_arguments.h"
#include "type_list.h"
#include "modules_query.h"
#include "router.h"
#include "core_gc.h"
#include "core_command_prompt.h"
#include "sysdep.h"

#include "parser_constructs.h"

#include "core_constructs_query.h"

/**************************************
 * LOCAL FUNCTION PROTOTYPES
 ***************************************/

static BOOLEAN                    _delete_named_construct(void *, char *, struct construct *);
static int                        _pp_construct(void *, char *, char *, struct construct *);
static struct module_definition * _query_module(void *, char *, struct construct *);

/***********************************
 * core_add_to_module: Adds a
 * construct to the current module.
 ************************************/
void core_add_to_module(struct construct_metadata *cons)
{
    if( cons->my_module->last_item == NULL )
    {
        cons->my_module->first_item = cons;
    }
    else
    {
        cons->my_module->last_item->next = cons;
    }

    cons->my_module->last_item = cons;
    cons->next = NULL;
}

/***************************************************
 * DeleteNamedConstruct: Generic driver routine for
 *   deleting a specific construct from a module.
 ****************************************************/
static BOOLEAN _delete_named_construct(void *env, char *constructName, struct construct *constructClass)
{
    void *constructPtr;

    /*=============================
     * Constructs can't be deleted
     * while a bload is in effect.
     *=============================*/


    /*===============================
     * Look for the named construct.
     *===============================*/

    constructPtr = (*constructClass->finder)(env, constructName);

    /*========================================
     * If the construct was found, delete it.
     *========================================*/

    if( constructPtr != NULL )
    {
        return((*constructClass->deleter)(env, constructPtr));
    }

    /*========================================
     * If the construct wasn't found, but the
     * special symbol * was used, then delete
     * all constructs of the specified type.
     *========================================*/

    if( strcmp("*", constructName) == 0 )
    {
        (*constructClass->deleter)(env, NULL);
        return(TRUE);
    }

    /*===============================
     * Otherwise, return FALSE to
     * indicate no deletion occured.
     *===============================*/

    return(FALSE);
}

/******************************************
 * core_lookup_named_construct: Generic routine for
 *   searching for a specified construct.
 *******************************************/
void *core_lookup_named_construct(void *env, char *constructName, struct construct *constructClass)
{
    void *cons;
    ATOM_HN *findValue;

    /*==========================
     * Save the current module.
     *==========================*/

    save_current_module(env);

    /*=========================================================
     * Extract the construct name. If a module was specified,
     * then garner_module_reference will set the current
     * module to the module specified in the name.
     *=========================================================*/

    constructName = garner_module_reference(env, constructName);

    /*=================================================
     * If a valid construct name couldn't be extracted
     * or the construct name isn't in the symbol table
     * (which means the construct doesn't exist), then
     * return NULL to indicate the specified construct
     * couldn't be found.
     *=================================================*/

    if((constructName == NULL) ?
       TRUE :
       ((findValue = (ATOM_HN *)lookup_atom(env, constructName)) == NULL))
    {
        restore_current_module(env);
        return(NULL);
    }

    /*===============================================
     * Loop through every construct of the specified
     * class in the current module checking to see
     * if the construct's name matches the construct
     * being sought. If found, restore the current
     * module and return a pointer to the construct.
     *===============================================*/

    for( cons = (*constructClass->next_item_geter)(env, NULL);
         cons != NULL;
         cons = (*constructClass->next_item_geter)(env, cons))
    {
        if( findValue == (*constructClass->namer)((struct construct_metadata *)cons))
        {
            restore_current_module(env);
            return(cons);
        }
    }

    /*=============================
     * Restore the current module.
     *=============================*/

    restore_current_module(env);

    /*====================================
     * Return NULL to indicated the named
     * construct was not found.
     *====================================*/

    return(NULL);
}

/****************************************
 * core_uninstall_construct_command: Driver routine
 *   for the undef<construct> commands.
 *****************************************/
void core_uninstall_construct_command(void *env, char *command, struct construct *constructClass)
{
    char *constructName;
    char buffer[80];

    /*==============================================
     * Get the name of the construct to be deleted.
     *==============================================*/

    sysdep_sprintf(buffer, "%s name", constructClass->construct_name);

    constructName = core_get_atom(env, command, buffer);

    if( constructName == NULL )
    {
        return;
    }


    /*=============================================
     * Check to see if the named construct exists.
     *=============================================*/

    if(((*constructClass->finder)(env, constructName) == NULL) &&
       (strcmp("*", constructName) != 0))
    {
        error_lookup(env, constructClass->construct_name, constructName);
        return;
    }

    /*===============================================
     * If the construct does exist, try deleting it.
     *===============================================*/

    else if( _delete_named_construct(env, constructName, constructClass) == FALSE )
    {
        error_deletion(env, constructClass->construct_name, constructName);
        return;
    }

    return;
}

/*****************************************
 * core_construct_pp_command: Driver routine for
 *   the ppdef<construct> commands.
 ******************************************/
void core_construct_pp_command(void *env, char *command, struct construct *constructClass)
{
    char *constructName;
    char buffer[80];

    /*===============================
     * Get the name of the construct
     * to be "pretty printed."
     *===============================*/

    sysdep_sprintf(buffer, "%s name", constructClass->construct_name);

    constructName = core_get_atom(env, command, buffer);

    if( constructName == NULL )
    {
        return;
    }

    /*================================
     * Call the driver routine for
     * pretty printing the construct.
     *================================*/

    if( _pp_construct(env, constructName, WDISPLAY, constructClass) == FALSE )
    {
        error_lookup(env, constructClass->construct_name, constructName);
    }
}

/**********************************
 * PPConstruct: Driver routine for
 *   pretty printing a construct.
 ***********************************/
static int _pp_construct(void *env, char *constructName, char *logicalName, struct construct *constructClass)
{
    void *constructPtr;

    /*==================================
     * Use the construct's name to find
     * a pointer to actual construct.
     *==================================*/

    constructPtr = (*constructClass->finder)(env, constructName);

    if( constructPtr == NULL )
    {
        return(FALSE);
    }

    /*==============================================
     * If the pretty print form is NULL (because of
     * conserve-mem), return TRUE (which indicates
     * the construct was found).
     *==============================================*/

    if((*constructClass->pper)(env, (struct construct_metadata *)constructPtr) == NULL )
    {
        return(TRUE);
    }

    /*============================================
     * Print the pretty print string in smaller
     * chunks. (VMS had a bug that didn't allow
     * printing a string greater than 512 bytes.)
     *============================================*/

    core_print_chunkify(env, logicalName, (*constructClass->pper)(env, (struct construct_metadata *)constructPtr));

    /*=======================================
     * Return TRUE to indicate the construct
     * was found and pretty printed.
     *=======================================*/

    return(TRUE);
}

/********************************************
 * core_query_module_name: Driver routine
 *   for def<construct>-module routines
 *********************************************/
ATOM_HN *core_query_module_name(void *env, char *command, struct construct *constructClass)
{
    char *constructName;
    char buffer[80];
    struct module_definition *constructModule;

    /*=========================================
     * Get the name of the construct for which
     * we want to determine its module.
     *=========================================*/

    sysdep_sprintf(buffer, "%s name", constructClass->construct_name);

    constructName = core_get_atom(env, command, buffer);

    if( constructName == NULL )
    {
        return((ATOM_HN *)get_false(env));
    }

    /*==========================================
     * Get a pointer to the construct's module.
     *==========================================*/

    constructModule = _query_module(env, constructName, constructClass);

    if( constructModule == NULL )
    {
        error_lookup(env, constructClass->construct_name, constructName);
        return((ATOM_HN *)get_false(env));
    }

    /*============================================
     * Return the name of the construct's module.
     *============================================*/

    return(constructModule->name);
}

/*****************************************
 * coreQueryModule: Driver routine for
 *   getting the module for a construct
 ******************************************/
struct module_definition *_query_module(void *env, char *constructName, struct construct *constructClass)
{
    struct construct_metadata *constructPtr;
    int count;
    unsigned position;
    ATOM_HN *theName;

    /*====================================================
     * If the construct name contains a module specifier,
     * then get a pointer to the module_definition associated
     * with the specified name.
     *====================================================*/

    if((position = find_module_separator(constructName)) != FALSE )
    {
        theName = garner_module_name(env, position, constructName);

        if( theName != NULL )
        {
            return((struct module_definition *)lookup_module(env, to_string(theName)));
        }
    }

    /*============================================
     * No module was specified, so search for the
     * named construct in the current module and
     * modules from which it imports.
     *============================================*/

    constructPtr = (struct construct_metadata *)
                   lookup_imported_construct(env, constructClass->construct_name, NULL, constructName,
                                             &count, TRUE, NULL);

    if( constructPtr == NULL ) {return(NULL);}

    return(constructPtr->my_module->module_def);
}

/**********************************
 * core_save_construct: Generic routine
 *   for saving a construct class.
 ***********************************/
void core_save_construct(void *env, void *theModule, char *logicalName, struct construct *constructClass)
{
    char *ppform;
    struct construct_metadata *cons;

    /*==========================
     * Save the current module.
     *==========================*/

    save_current_module(env);

    /*===========================
     * Set the current module to
     * the one we're examining.
     *===========================*/

    set_current_module(env, theModule);

    /*==============================================
     * Loop through each construct of the specified
     * construct class in the module.
     *==============================================*/

    for( cons = (struct construct_metadata *)
                (*constructClass->next_item_geter)(env, NULL);
         cons != NULL;
         cons = (struct construct_metadata *)
                (*constructClass->next_item_geter)(env, cons))
    {
        /*==========================================
         * Print the construct's pretty print form.
         *==========================================*/

        ppform = (*constructClass->pper)(env, cons);

        if( ppform != NULL )
        {
            core_print_chunkify(env, logicalName, ppform);
            print_router(env, logicalName, "\n");
        }
    }

    /*=============================
     * Restore the current module.
     *=============================*/

    restore_current_module(env);
}

/********************************************************
 * core_garner_module: Generic routine for returning
 *   the name of the module to which a construct belongs
 *********************************************************/
char *core_garner_module(struct construct_metadata *cons)
{
    return(get_module_name(NULL, (void *)cons->my_module->module_def));
}

/********************************************************
 * core_get_atom_string: Generic routine for returning
 *   the name string of a construct.
 *********************************************************/
char *core_get_atom_string(struct construct_metadata *cons)
{
    return(to_string(cons->name));
}

/*********************************************************
 * core_get_atom_pointer: Generic routine for returning
 *   the name pointer of a construct.
 **********************************************************/
ATOM_HN *core_get_atom_pointer(struct construct_metadata *cons)
{
    return(cons->name);
}

/***********************************************
 * core_garner_construct_list: Generic Routine
 *   for retrieving the constructs in a module.
 ************************************************/
void core_garner_construct_list(void *env, char *functionName, core_data_object_ptr ret, struct construct *constructClass)
{
    struct module_definition *theModule;
    core_data_object result;
    int numArgs;

    /*============================================
     * Check for the correct number of arguments.
     *============================================*/

    if((numArgs = core_check_arg_count(env, functionName, NO_MORE_THAN, 1)) == -1 )
    {
        core_create_error_list(env, ret);
        return;
    }

    /*====================================
     * If an argument was given, check to
     * see that it's a valid module name.
     *====================================*/

    if( numArgs == 1 )
    {
        /*======================================
         * Only symbols are valid module names.
         *======================================*/

        core_get_arg_at(env, 1, &result);

        if( core_get_type(result) != ATOM )
        {
            core_create_error_list(env, ret);
            report_explicit_type_error(env, functionName, 1, "defmodule name");
            return;
        }

        /*===========================================
         * Verify that the named module exists or is
         * the symbol * (for obtaining the construct
         * list for all modules).
         *===========================================*/

        if((theModule = (struct module_definition *)lookup_module(env, core_convert_data_to_string(result))) == NULL )
        {
            if( strcmp("*", core_convert_data_to_string(result)) != 0 )
            {
                core_create_error_list(env, ret);
                report_explicit_type_error(env, functionName, 1, "defmodule name");
                return;
            }

            theModule = NULL;
        }
    }

    /*=====================================
     * Otherwise use the current module to
     * generate the construct list.
     *=====================================*/

    else
    {
        theModule = ((struct module_definition *)get_current_module(env));
    }

    /*=============================
     * Call the driver routine to
     * get the list of constructs.
     *=============================*/

    core_get_module_constructs_list(env, ret, constructClass, theModule);
}

/*******************************************
 * core_get_module_constructs_list: Generic C Routine for
 *   retrieving the constructs in a module.
 ********************************************/
void core_get_module_constructs_list(void *env, core_data_object_ptr ret, struct construct *constructClass, struct module_definition *theModule)
{
    void *cons;
    unsigned long count = 0;
    struct list *list;
    ATOM_HN *theName;
    struct module_definition *loopModule;
    int allModules = FALSE;

#if WIN_BTC
    size_t largestConstructNameSize, bufferSize = 80;      /* prevents warning */
#else
    size_t largestConstructNameSize = 0, bufferSize = 80;  /* prevents warning */
#endif
    char *buffer;

    /*==========================
     * Save the current module.
     *==========================*/

    save_current_module(env);

    /*=======================================
     * If the module specified is NULL, then
     * get all constructs in all modules.
     *=======================================*/

    if( theModule == NULL )
    {
        theModule = (struct module_definition *)get_next_module(env, NULL);
        allModules = TRUE;
    }

    /*======================================================
     * Count the number of constructs to  be retrieved and
     * determine the buffer size is_needed to store the
     * module-name::construct-names that will be generated.
     *======================================================*/

    loopModule = theModule;

    while( loopModule != NULL )
    {
        size_t tempSize;

        /*======================================================
         * Set the current module to the module being examined.
         *======================================================*/

        set_current_module(env, (void *)loopModule);

        /*===========================================
         * Loop over every construct in the  module.
         *===========================================*/

        cons = NULL;
        largestConstructNameSize = 0;

        while((cons = (*constructClass->next_item_geter)(env, cons)) != NULL )
        {
            /*================================
             * Increment the construct count.
             *================================*/

            count++;

            /*=================================================
             * Is this the largest construct name encountered?
             *=================================================*/

            tempSize = strlen(to_string((*constructClass->namer)((struct construct_metadata *)cons)));

            if( tempSize > largestConstructNameSize )
            {
                largestConstructNameSize = tempSize;
            }
        }

        /*========================================
         * Determine the size of the module name.
         *========================================*/

        tempSize = strlen(get_module_name(env, loopModule));

        /*======================================================
         * The buffer must be large enough for the module name,
         * the largest name of all the constructs, and the ::.
         *======================================================*/

        if((tempSize + largestConstructNameSize + 5) > bufferSize )
        {
            bufferSize = tempSize + largestConstructNameSize + 5;
        }

        /*=============================
         * Move on to the next module.
         *=============================*/

        if( allModules )
        {
            loopModule = (struct module_definition *)get_next_module(env, loopModule);
        }
        else
        {
            loopModule = NULL;
        }
    }

    /*===========================
     * Allocate the name buffer.
     *===========================*/

    buffer = (char *)core_mem_alloc(env, bufferSize);

    /*================================
     * Create the list value to
     * store the construct names.
     *================================*/

    core_set_pointer_type(ret, LIST);
    core_set_data_ptr_start(ret, 1);
    core_set_data_ptr_end(ret, (long)count);
    list = (struct list *)create_list(env, count);
    core_set_pointer_value(ret, (void *)list);

    /*===========================
     * Store the construct names
     * in the list value.
     *===========================*/

    loopModule = theModule;
    count = 1;

    while( loopModule != NULL )
    {
        /*============================
         * Set the current module to
         * the module being examined.
         *============================*/

        set_current_module(env, (void *)loopModule);

        /*===============================
         * Add each construct name found
         * in the module to the list.
         *===============================*/

        cons = NULL;

        while((cons = (*constructClass->next_item_geter)(env, cons)) != NULL )
        {
            theName = (*constructClass->namer)((struct construct_metadata *)cons);
            set_list_node_type(list, count, ATOM);

            if( allModules )
            {
                sysdep_strcpy(buffer, get_module_name(env, loopModule));
                sysdep_strcat(buffer, "::");
                sysdep_strcat(buffer, to_string(theName));
                set_list_node_value(list, count, store_atom(env, buffer));
            }
            else
            {
                set_list_node_value(list, count, store_atom(env, to_string(theName)));
            }

            count++;
        }

        /*==================================
         * Move on to the next module (if
         * the list is to contain the names
         * of constructs from all modules).
         *==================================*/

        if( allModules )
        {
            loopModule = (struct module_definition *)get_next_module(env, loopModule);
        }
        else
        {
            loopModule = NULL;
        }
    }

    /*=========================
     * Return the name buffer.
     *=========================*/

    core_mem_free(env, buffer, bufferSize);

    /*=============================
     * Restore the current module.
     *=============================*/

    restore_current_module(env);
}

/********************************************
 * core_garner_module_constructs_command: Generic Routine for
 *   listing the constructs in a module.
 *********************************************/
void core_garner_module_constructs_command(void *env, char *functionName, struct construct *constructClass)
{
    struct module_definition *theModule;
    core_data_object result;
    int numArgs;

    /*============================================
     * Check for the correct number of arguments.
     *============================================*/

    if((numArgs = core_check_arg_count(env, functionName, NO_MORE_THAN, 1)) == -1 )
    {
        return;
    }

    /*====================================
     * If an argument was given, check to
     * see that it's a valid module name.
     *====================================*/

    if( numArgs == 1 )
    {
        /*======================================
         * Only symbols are valid module names.
         *======================================*/

        core_get_arg_at(env, 1, &result);

        if( core_get_type(result) != ATOM )
        {
            report_explicit_type_error(env, functionName, 1, "defmodule name");
            return;
        }

        /*===========================================
         * Verify that the named module exists or is
         * the symbol * (for obtaining the construct
         * list for all modules).
         *===========================================*/

        if((theModule = (struct module_definition *)lookup_module(env, core_convert_data_to_string(result))) == NULL )
        {
            if( strcmp("*", core_convert_data_to_string(result)) != 0 )
            {
                report_explicit_type_error(env, functionName, 1, "defmodule name");
                return;
            }

            theModule = NULL;
        }
    }

    /*=====================================
     * Otherwise use the current module to
     * generate the construct list.
     *=====================================*/

    else
    {
        theModule = ((struct module_definition *)get_current_module(env));
    }

    /*=========================
     * Call the driver routine
     * to list the constructs.
     *=========================*/

    core_get_module_constructs(env, constructClass, WDISPLAY, theModule);
}

/****************************************
 * core_get_module_constructs: Generic C Routine for
 *   listing the constructs in a module.
 *****************************************/
void core_get_module_constructs(void *env, struct construct *constructClass, char *logicalName, struct module_definition *theModule)
{
    void *constructPtr;
    ATOM_HN *constructName;
    long count = 0;
    int allModules = FALSE;

    /*==========================
     * Save the current module.
     *==========================*/

    save_current_module(env);

    /*=======================================
     * If the module specified is NULL, then
     * list all constructs in all modules.
     *=======================================*/

    if( theModule == NULL )
    {
        theModule = (struct module_definition *)get_next_module(env, NULL);
        allModules = TRUE;
    }

    /*==================================
     * Loop through all of the modules.
     *==================================*/

    while( theModule != NULL )
    {
        /*========================================
         * If we're printing the construct in all
         * modules, then preface each module
         * listing with the name of the module.
         *========================================*/

        if( allModules )
        {
            print_router(env, logicalName, get_module_name(env, theModule));
            print_router(env, logicalName, ":\n");
        }

        /*===============================
         * Set the current module to the
         * module we're examining.
         *===============================*/

        set_current_module(env, (void *)theModule);

        /*===========================================
         * List all of the constructs in the module.
         *===========================================*/

        for( constructPtr = (*constructClass->next_item_geter)(env, NULL);
             constructPtr != NULL;
             constructPtr = (*constructClass->next_item_geter)(env, constructPtr))
        {
            if( core_get_evaluation_data(env)->halt == TRUE )
            {
                return;
            }

            constructName = (*constructClass->namer)((struct construct_metadata *)constructPtr);

            if( constructName != NULL )
            {
                if( allModules )
                {
                    print_router(env, WDISPLAY, "   ");
                }

                print_router(env, logicalName, to_string(constructName));
                print_router(env, logicalName, "\n");
            }

            count++;
        }

        /*====================================
         * Move on to the next module (if the
         * listing is to contain the names of
         * constructs from all modules).
         *====================================*/

        if( allModules )
        {
            theModule = (struct module_definition *)get_next_module(env, theModule);
        }
        else
        {
            theModule = NULL;
        }
    }

    /*=================================================
     * Print the tally and restore the current module.
     *=================================================*/

    core_print_tally(env, WDISPLAY, count, constructClass->construct_name,
                     constructClass->construct_name_plural);

    restore_current_module(env);
}

/*********************************************************
 * core_set_next_construct: Sets the next field of one construct
 *   to point to another construct of the same type.
 **********************************************************/
void core_set_next_construct(struct construct_metadata *cons, struct construct_metadata *targetConstruct)
{
    cons->next = targetConstruct;
}

/*******************************************************************
 * core_query_module_header: Returns the construct module for a given
 *   construct (note that this is a pointer to a data structure
 *   like the deffactsModule, not a pointer to an environment
 *   module which contains a number of types of constructs.
 ********************************************************************/
struct module_header *core_query_module_header(struct construct_metadata *cons)
{
    return(cons->my_module);
}

/************************************************
 * core_get_construct_pp: Returns the pretty print
 *   representation for the specified construct.
 *************************************************/
#if WIN_BTC
#pragma argsused
#endif
char *core_get_construct_pp(void *env, struct construct_metadata *cons)
{
#if MAC_MCW || WIN_MCW || MAC_XCD
#pragma unused(env)
#endif

    return(cons->pp);
}

/***************************************************
 * core_get_next_construct_metadata: Returns the next construct
 *   items from a list of constructs.
 ****************************************************/
struct construct_metadata *core_get_next_construct_metadata(void *env, struct construct_metadata *cons, int moduleIndex)
{
    struct module_header *theModuleItem;

    if( cons == NULL )
    {
        theModuleItem = (struct module_header *)
                        get_module_item(env, NULL, moduleIndex);

        if( theModuleItem == NULL ) {return(NULL);}

        return(theModuleItem->first_item);
    }

    return(cons->next);
}

/*****************************************
 * core_free_construct_header_module: Deallocates
 *   the data structures associated with
 *   the construct module item header.
 ******************************************/
void core_free_construct_header_module(void *env, struct module_header *theModuleItem, struct construct *constructClass)
{
    struct construct_metadata *thisOne, *nextOne;

    thisOne = theModuleItem->first_item;

    while( thisOne != NULL )
    {
        nextOne = thisOne->next;
        (*constructClass->freer)(env, thisOne);
        thisOne = nextOne;
    }
}

/*********************************************
 * core_do_for_all_constructs: Executes an action for
 *   all constructs of a specified type.
 **********************************************/
long core_do_for_all_constructs(void *env, void (*actionFunction)(void *, struct construct_metadata *, void *), int moduleItemIndex, int interruptable, void *userBuffer)
{
    struct construct_metadata *cons;
    struct module_header *theModuleItem;
    void *theModule;
    long moduleCount = 0L;

    /*==========================
     * Save the current module.
     *==========================*/

    save_current_module(env);

    /*==================================
     * Loop through all of the modules.
     *==================================*/

    for( theModule = get_next_module(env, NULL);
         theModule != NULL;
         theModule = get_next_module(env, theModule), moduleCount++ )
    {
        /*=============================
         * Set the current module to
         * the module we're examining.
         *=============================*/

        set_current_module(env, (void *)theModule);

        /*================================================
         * Perform the action for each of the constructs.
         *================================================*/

        theModuleItem = (struct module_header *)
                        get_module_item(env, (struct module_definition *)theModule, moduleItemIndex);

        for( cons = theModuleItem->first_item;
             cons != NULL;
             cons = cons->next )
        {
            if( interruptable )
            {
                if( core_get_halt_eval(env) == TRUE )
                {
                    restore_current_module(env);
                    return(-1L);
                }
            }

            (*actionFunction)(env, cons, userBuffer);
        }
    }

    /*=============================
     * Restore the current module.
     *=============================*/

    restore_current_module(env);

    /*=========================================
     * Return the number of modules traversed.
     *=========================================*/

    return(moduleCount);
}

/*****************************************************
 * core_do_for_all_constructs_in_module: Executes an action for
 *   all constructs of a specified type in a module.
 ******************************************************/
void core_do_for_all_constructs_in_module(void *env, void *theModule, void (*actionFunction)(void *, struct construct_metadata *, void *), int moduleItemIndex, int interruptable, void *userBuffer)
{
    struct construct_metadata *cons;
    struct module_header *theModuleItem;

    /*==========================
     * Save the current module.
     *==========================*/

    save_current_module(env);

    /*=============================
     * Set the current module to
     * the module we're examining.
     *=============================*/

    set_current_module(env, (void *)theModule);

    /*================================================
     * Perform the action for each of the constructs.
     *================================================*/

    theModuleItem = (struct module_header *)
                    get_module_item(env, (struct module_definition *)theModule, moduleItemIndex);

    for( cons = theModuleItem->first_item;
         cons != NULL;
         cons = cons->next )
    {
        if( interruptable )
        {
            if( core_get_halt_eval(env) == TRUE )
            {
                restore_current_module(env);
                return;
            }
        }

        (*actionFunction)(env, cons, userBuffer);
    }

    /*=============================
     * Restore the current module.
     *=============================*/

    restore_current_module(env);
}

/****************************************************
 * core_init_construct_header: Initializes construct
 *   header info, including to which module item the
 *   new construct belongs
 *****************************************************/
void core_init_construct_header(void *env, char *constructType, struct construct_metadata *cons, ATOM_HN *consName)
{
    struct module_item *theModuleItem;
    struct module_header *theItemHeader;

    theModuleItem = lookup_module_item(env, constructType);
    theItemHeader = (struct module_header *)
                    get_module_item(env, NULL, theModuleItem->index);

    cons->my_module = theItemHeader;
    cons->name = consName;
    cons->pp = NULL;
    cons->next = NULL;
    cons->ext_data = NULL;
}

/************************************************
 * core_set_construct_pp: Sets a construct's pretty
 *   print form and deletes the old one.
 *************************************************/
void core_set_construct_pp(void *env, struct construct_metadata *cons, char *ppForm)
{
    if( cons->pp != NULL )
    {
        core_mem_release(env, (void *)cons->pp,
                         ((strlen(cons->pp) + 1) * sizeof(char)));
    }

    cons->pp = ppForm;
}

#if DEBUGGING_FUNCTIONS

/*****************************************************
 * ConstructPrintWatchAccess: Provides an interface
 *   to the list-watch-items function for a construct
 ******************************************************/
unsigned ConstructPrintWatchAccess(void *env, struct construct *constructClass, char *logName, core_expression_object *argExprs, unsigned (*getWatchFunc)(void *, void *), void (*setWatchFunc)(void *, unsigned, void *))
{
    return(ConstructWatchSupport(env, constructClass, "list-watch-items", logName, argExprs,
                                 FALSE, FALSE, getWatchFunc, setWatchFunc));
}

/*************************************************
 * ConstructSetWatchAccess: Provides an interface
 *   to the watch function for a construct
 **************************************************/
unsigned ConstructSetWatchAccess(void *env, struct construct *constructClass, unsigned newState, core_expression_object *argExprs, unsigned (*getWatchFunc)(void *, void *), void (*setWatchFunc)(void *, unsigned, void *))
{
    return(ConstructWatchSupport(env, constructClass, "watch", WERROR, argExprs,
                                 TRUE, newState, getWatchFunc, setWatchFunc));
}

/*****************************************************
 * ConstructWatchSupport: Generic construct interface
 *   into watch and list-watch-items.
 ******************************************************/
static unsigned ConstructWatchSupport(void *env, struct construct *constructClass, char *funcName, char *logName, core_expression_object *argExprs, BOOLEAN setFlag, unsigned newState, unsigned (*getWatchFunc)(void *, void *), void (*setWatchFunc)(void *, unsigned, void *))
{
    struct module_definition *module_def;
    void *cons;
    core_data_object construct_name;
    int argIndex = 2;

    /*========================================
     * If no constructs are specified, then
     * show/set the trace for all constructs.
     *========================================*/

    if( argExprs == NULL )
    {
        /*==========================
         * Save the current module.
         *==========================*/

        save_current_module(env);

        /*===========================
         * Loop through each module.
         *===========================*/

        for( module_def = (struct module_definition *)get_next_module(env, NULL);
             module_def != NULL;
             module_def = (struct module_definition *)get_next_module(env, (void *)module_def))
        {
            /*============================
             * Set the current module to
             * the module being examined.
             *============================*/

            set_current_module(env, (void *)module_def);

            /*====================================================
             * If we're displaying the names of constructs with
             * watch flags enabled, then preface each module
             * listing of constructs with the name of the module.
             *====================================================*/

            if( setFlag == FALSE )
            {
                print_router(env, logName, get_module_name(env, (void *)module_def));
                print_router(env, logName, ":\n");
            }

            /*============================================
             * Loop through each construct in the module.
             *============================================*/

            for( cons = (*constructClass->next_item_geter)(env, NULL);
                 cons != NULL;
                 cons = (*constructClass->next_item_geter)(env, cons))
            {
                /*=============================================
                 * Either set the watch flag for the construct
                 * or display its current state.
                 *=============================================*/

                if( setFlag )
                {
                    (*setWatchFunc)(env, newState, cons);
                }
                else
                {
                    print_router(env, logName, "   ");
                    ConstructPrintWatch(env, logName, constructClass, cons, getWatchFunc);
                }
            }
        }

        /*=============================
         * Restore the current module.
         *=============================*/

        restore_current_module(env);

        /*====================================
         * Return TRUE to indicate successful
         * completion of the command.
         *====================================*/

        return(TRUE);
    }

    /*==================================================
     * Show/set the trace for each specified construct.
     *==================================================*/

    while( argExprs != NULL )
    {
        /*==========================================
         * Evaluate the argument that should be a
         * construct name. Return FALSE is an error
         * occurs when evaluating the argument.
         *==========================================*/

        if( core_eval_expression(env, argExprs, &construct_name))
        {
            return(FALSE);
        }

        /*================================================
         * Check to see that it's a valid construct name.
         *================================================*/

        if((construct_name.type != ATOM) ? TRUE :
           ((cons = core_lookup_construct(env, constructClass,
                                          core_convert_data_to_string(construct_name), TRUE)) == NULL))
        {
            report_explicit_type_error(env, funcName, argIndex, constructClass->construct_name);
            return(FALSE);
        }

        /*=============================================
         * Either set the watch flag for the construct
         * or display its current state.
         *=============================================*/

        if( setFlag )
        {
            (*setWatchFunc)(env, newState, cons);
        }
        else
        {
            ConstructPrintWatch(env, logName, constructClass, cons, getWatchFunc);
        }

        /*===============================
         * Move on to the next argument.
         *===============================*/

        argIndex++;
        argExprs = core_get_next_arg(argExprs);
    }

    /*====================================
     * Return TRUE to indicate successful
     * completion of the command.
     *====================================*/

    return(TRUE);
}

/************************************************
 * ConstructPrintWatch: Displays the trace value
 *   of a construct for list-watch-items
 *************************************************/
static void ConstructPrintWatch(void *env, char *logName, struct construct *constructClass, void *cons, unsigned (*getWatchFunc)(void *, void *))
{
    print_router(env, logName, to_string((*constructClass->namer)((struct construct_metadata *)cons)));

    if((*getWatchFunc)(env, cons))
    {
        print_router(env, logName, " = on\n");
    }
    else
    {
        print_router(env, logName, " = off\n");
    }
}

#endif /* DEBUGGING_FUNCTIONS */

/****************************************************
 * core_lookup_construct: Finds a construct in the current
 *   or imported modules. If specified, will also
 *   look for construct in a non-imported module.
 *****************************************************/
void *core_lookup_construct(void *env, struct construct *constructClass, char *constructName, BOOLEAN moduleNameAllowed)
{
    void *cons;
    char *constructType;
    int moduleCount;

    /*============================================
     * Look for the specified construct in the
     * current module or in any imported modules.
     *============================================*/

    constructType = constructClass->construct_name;
    cons = lookup_imported_construct(env, constructType, NULL, constructName,
                                     &moduleCount, TRUE, NULL);

    /*===========================================
     * Return NULL if the reference is ambiguous
     * (it was found in more than one module).
     *===========================================*/

    if( cons != NULL )
    {
        if( moduleCount > 1 )
        {
            error_ambiguous_reference(env, constructType, constructName);
            return(NULL);
        }

        return(cons);
    }

    /*=============================================
     * If specified, check to see if the construct
     * is in a non-imported module.
     *=============================================*/

    if( moduleNameAllowed && find_module_separator(constructName))
    {
        cons = (*constructClass->finder)(env, constructName);
    }

    /*====================================
     * Return a pointer to the construct.
     *====================================*/

    return(cons);
}

/**********************************************************
 * core_are_constructs_deleteable: Returns a boolean value indicating
 *   whether constructs in general can be deleted.
 ***********************************************************/
#if WIN_BTC
#pragma argsused
#endif
BOOLEAN core_are_constructs_deleteable(void *env)
{
#if MAC_MCW || WIN_MCW || MAC_XCD
#pragma unused(env)
#endif

    return(TRUE);
}
