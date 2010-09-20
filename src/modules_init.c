/* Purpose: Defines basic module_definition primitive functions such
 *   as allocating and deallocating, traversing, and finding
 *   module_definition data structures.                              */

#define __MODULES_INIT_SOURCE__

#include "setup.h"

#include <stdio.h>
#include <string.h>
#define _STDIO_INCLUDED_

#include "core_memory.h"
#include "constant.h"
#include "router.h"
#include "core_functions.h"
#include "core_arguments.h"
#include "core_constructs.h"
#include "parser_modules.h"
#include "modules_kernel.h"
#include "core_gc.h"
#include "core_environment.h"

#include "modules_init.h"

/**************************************
 * LOCAL FUNCTION PROTOTYPES
 ***************************************/

static void ReturnDefmodule(void *, struct module_definition *, BOOLEAN);
static void DeallocateDefmoduleData(void *);

/*************************************************************
 * init_modules: Initializes the module_definition construct.
 **************************************************************/
void init_module_data(void *env)
{
    core_allocate_environment_data(env, MODULE_DATA_INDEX, sizeof(struct module_data), NULL);
    core_add_environment_cleaner(env, "defmodules", DeallocateDefmoduleData, -1000);
    get_module_data(env)->should_notify_listeners = TRUE;
    get_module_data(env)->redefinable = TRUE;
}

/***************************************************
 * DeallocateDefmoduleData: Deallocates environment
 *    data for the module_definition construct.
 ****************************************************/
static void DeallocateDefmoduleData(void *env)
{
    struct module_stack_element *tmpMSPtr, *nextMSPtr;
    struct module_item *tmpMIPtr, *nextMIPtr;

    struct module_definition *tmpDMPtr, *nextDMPtr;
    struct portConstructItem *tmpPCPtr, *nextPCPtr;


    tmpDMPtr = get_module_data(env)->submodules;

    while( tmpDMPtr != NULL )
    {
        nextDMPtr = tmpDMPtr->next;
        ReturnDefmodule(env, tmpDMPtr, TRUE);
        tmpDMPtr = nextDMPtr;
    }

    tmpPCPtr = get_module_data(env)->exportable_items;

    while( tmpPCPtr != NULL )
    {
        nextPCPtr = tmpPCPtr->next;
        core_mem_return_struct(env, portConstructItem, tmpPCPtr);
        tmpPCPtr = nextPCPtr;
    }


    tmpMSPtr = get_module_data(env)->module_stack;

    while( tmpMSPtr != NULL )
    {
        nextMSPtr = tmpMSPtr->next;
        core_mem_return_struct(env, module_stack_element, tmpMSPtr);
        tmpMSPtr = nextMSPtr;
    }

    tmpMIPtr = get_module_data(env)->items;

    while( tmpMIPtr != NULL )
    {
        nextMIPtr = tmpMIPtr->next;
        core_mem_return_struct(env, module_item, tmpMIPtr);
        tmpMIPtr = nextMIPtr;
    }

    core_gc_delete_call_list(env, get_module_data(env)->module_define_listeners);
    core_gc_delete_call_list(env, get_module_data(env)->module_change_listeners);
}

/*************************************************************
 * init_modules: Initializes the module_definition construct.
 **************************************************************/
void init_modules(void *env)
{
    init_module_functions(env);

    create_main_module(env);

#if DEFMODULE_CONSTRUCT
    core_define_construct(env, "defmodule", "defmodules", ParseDefmodule, NULL, NULL, NULL, NULL,
                          NULL, NULL, NULL, NULL, NULL);
#endif

#if DEFMODULE_CONSTRUCT
    core_define_function(env, "get-current-module", 'w',
                         PTR_FN broccoli_get_current_module,
                         "GetCurrentModuleCommand", "00");

    core_define_function(env, "set-current-module", 'w',
                         PTR_FN broccoli_set_current_module,
                         "SetCurrentModuleCommand", "11w");
#endif
}

/*****************************************************
 * install_module_item: Called to register a construct
 *   which can be placed within a module.
 ******************************************************/
int install_module_item(void *env, char *theItem, void *(*allocateFunction)(void *), void (*freeFunction)(void *, void *), void *(*bloadModuleReference)(void *, int), void (*constructsToCModuleReference)(void *, FILE *, int, int, int), void *(*findFunction)(void *, char *))
{
    struct module_item *newModuleItem;

    newModuleItem = core_mem_get_struct(env, module_item);
    newModuleItem->name = theItem;
    newModuleItem->fn_allocate = allocateFunction;
    newModuleItem->fn_free = freeFunction;
    newModuleItem->fn_not_used = bloadModuleReference;
    newModuleItem->fn_not_used2 = constructsToCModuleReference;
    newModuleItem->fn_find = findFunction;
    newModuleItem->index = get_module_data(env)->items_count++;
    newModuleItem->next = NULL;

    if( get_module_data(env)->last_item == NULL )
    {
        get_module_data(env)->items = newModuleItem;
        get_module_data(env)->last_item = newModuleItem;
    }
    else
    {
        get_module_data(env)->last_item->next = newModuleItem;
        get_module_data(env)->last_item = newModuleItem;
    }

    return(newModuleItem->index);
}

/**********************************************************
 * get_module_list: Returns the list of module items.
 ***********************************************************/
struct module_item *get_module_list(void *env)
{
    return(get_module_data(env)->items);
}

/**************************************************************
 * get_items_count: Returns the number of module items.
 ***************************************************************/
int get_items_count(void *env)
{
    return(get_module_data(env)->items_count);
}

/*******************************************************
 * lookup_module_item: Finds the module item data structure
 *   corresponding to the specified name.
 ********************************************************/
struct module_item *lookup_module_item(void *env, char *theName)
{
    struct module_item *theModuleItem;

    for( theModuleItem = get_module_data(env)->items;
         theModuleItem != NULL;
         theModuleItem = theModuleItem->next )
    {
        if( strcmp(theModuleItem->name, theName) == 0 ) {return(theModuleItem);}
    }

    return(NULL);
}

/*****************************************
 * get_current_module: Returns a pointer
 *   to the current module.
 ******************************************/
void *get_current_module(void *env)
{
    return((void *)get_module_data(env)->current_submodule);
}

/*************************************************************
 * set_current_module: Sets the value of the current module.
 **************************************************************/
void *set_current_module(void *env, void *xNewValue)
{
    struct module_definition *newValue = (struct module_definition *)xNewValue;
    struct core_gc_call_function *changeFunctions;
    void *rv;

    /*=============================================
     * Change the current module to the specified
     * module and save the previous current module
     * for the return value.
     *=============================================*/

    rv = (void *)get_module_data(env)->current_submodule;
    get_module_data(env)->current_submodule = newValue;

    /*==========================================================
     * Call the list of registered functions that need to know
     * when the module has changed. The module change functions
     * should only be called if this is a "real" module change.
     * Many routines temporarily change the module to look for
     * constructs, etc. The save_current_module function will
     * disable the change functions from being called.
     *==========================================================*/

    if( get_module_data(env)->should_notify_listeners )
    {
        get_module_data(env)->change_index++;
        changeFunctions = get_module_data(env)->module_change_listeners;

        while( changeFunctions != NULL )
        {
            (*(void(*) (void *))changeFunctions->func)(env);
            changeFunctions = changeFunctions->next;
        }
    }

    /*=====================================
     * Return the previous current module.
     *=====================================*/

    return(rv);
}

/*******************************************************
 * save_current_module: Saves current module on stack and
 *   prevents SetCurrentModule() from calling change
 *   functions
 ********************************************************/
void save_current_module(void *env)
{
    MODULE_STACK_ELEMENT *tmp;

    tmp = core_mem_get_struct(env, module_stack_element);
    tmp->can_change = get_module_data(env)->should_notify_listeners;
    get_module_data(env)->should_notify_listeners = FALSE;
    tmp->module_def = get_module_data(env)->current_submodule;
    tmp->next = get_module_data(env)->module_stack;
    get_module_data(env)->module_stack = tmp;
}

/*********************************************************
 * restore_current_module: Restores saved module and resets
 *   ability of SetCurrentModule() to call changed
 *   functions to previous state
 **********************************************************/
void restore_current_module(void *env)
{
    MODULE_STACK_ELEMENT *tmp;

    tmp = get_module_data(env)->module_stack;
    get_module_data(env)->module_stack = tmp->next;
    get_module_data(env)->should_notify_listeners = tmp->can_change;
    get_module_data(env)->current_submodule = tmp->module_def;
    core_mem_return_struct(env, module_stack_element, tmp);
}

/************************************************************
 * get_module_item: Returns the data pointer for the specified
 *   module item in the specified module. If no module is
 *   indicated, then the module item for the current module
 *   is returned.
 *************************************************************/
void *get_module_item(void *env, struct module_definition *theModule, int moduleItemIndex)
{
    if( theModule == NULL )
    {
        if( get_module_data(env)->current_submodule == NULL )
        {
            return(NULL);
        }

        theModule = get_module_data(env)->current_submodule;
    }

    if( theModule->items == NULL )
    {
        return(NULL);
    }

    return((void *)theModule->items[moduleItemIndex]);
}

/***********************************************************
 * set_module_item: Sets the data pointer for the specified
 *   module item in the specified module. If no module is
 *   indicated, then the module item for the current module
 *   is returned.
 ************************************************************/
void set_module_item(void *env, struct module_definition *theModule, int moduleItemIndex, void *newValue)
{
    if( theModule == NULL )
    {
        if( get_module_data(env)->current_submodule == NULL )
        {
            return;
        }

        theModule = get_module_data(env)->current_submodule;
    }

    if( theModule->items == NULL )
    {
        return;
    }

    theModule->items[moduleItemIndex] = (struct module_header *)newValue;
}

/*****************************************************
 * create_main_module: Creates the default MAIN module.
 ******************************************************/
void create_main_module(void *env)
{
    struct module_definition *newDefmodule;
    struct module_item *theItem;
    int i;
    struct module_header *theHeader;

    /*=======================================
     * Allocate the module_definition data structure
     * and name it the MAIN module.
     *=======================================*/

    newDefmodule = core_mem_get_struct(env, module_definition);
    newDefmodule->name = (ATOM_HN *)store_atom(env, "MAIN");
    inc_atom_count(newDefmodule->name);
    newDefmodule->next = NULL;
    newDefmodule->pp = NULL;
    newDefmodule->imports = NULL;
    newDefmodule->exports = NULL;
    newDefmodule->id = 0L;
    newDefmodule->ext_datum = NULL;

    /*==================================
     * Initialize the array for storing
     * the module's construct lists.
     *==================================*/

    if( get_module_data(env)->items_count == 0 )
    {
        newDefmodule->items = NULL;
    }
    else
    {
        newDefmodule->items = (struct module_header **)
                              core_mem_alloc_no_init(env, sizeof(void *) * get_module_data(env)->items_count);

        for( i = 0, theItem = get_module_data(env)->items;
             (i < get_module_data(env)->items_count) && (theItem != NULL);
             i++, theItem = theItem->next )
        {
            if( theItem->fn_allocate == NULL )
            {
                newDefmodule->items[i] = NULL;
            }
            else
            {
                newDefmodule->items[i] = (struct module_header *)
                                         (*theItem->fn_allocate)(env);
                theHeader = (struct module_header *)newDefmodule->items[i];
                theHeader->module_def = newDefmodule;
                theHeader->first_item = NULL;
                theHeader->last_item = NULL;
            }
        }
    }

    /*=======================================
     * Add the module to the list of modules
     * and make it the current module.
     *=======================================*/

#if DEFMODULE_CONSTRUCT
    SetNumberOfDefmodules(env, 1L);
#endif

    get_module_data(env)->last_submodule = newDefmodule;
    get_module_data(env)->submodules = newDefmodule;
    set_current_module(env, (void *)newDefmodule);
}

/********************************************************************
 * set_module_list: Sets the list of defmodules to the specified
 *   value. Normally used when initializing a run-time module or
 *   when bloading a binary file to install the list of defmodules.
 *********************************************************************/
void set_module_list(void *env, void *defmodulePtr)
{
    get_module_data(env)->submodules = (struct module_definition *)defmodulePtr;

    get_module_data(env)->last_submodule = get_module_data(env)->submodules;

    if( get_module_data(env)->last_submodule == NULL )
    {
        return;
    }

    while( get_module_data(env)->last_submodule->next != NULL )
    {
        get_module_data(env)->last_submodule = get_module_data(env)->last_submodule->next;
    }
}

/*******************************************************************
 * get_next_module: If passed a NULL pointer, returns the first
 *   module_definition in the submodules. Otherwise returns the next
 *   module_definition following the module_definition passed as an argument.
 ********************************************************************/
void *get_next_module(void *env, void *defmodulePtr)
{
    if( defmodulePtr == NULL )
    {
        return((void *)get_module_data(env)->submodules);
    }
    else
    {
        return((void *)(((struct module_definition *)defmodulePtr)->next));
    }
}

/****************************************
 * get_module_name: Returns the name
 *   of the specified module_definition.
 *****************************************/
#if WIN_BTC
#pragma argsused
#endif
char *get_module_name(void *env, void *defmodulePtr)
{
#if MAC_MCW || WIN_MCW || MAC_XCD
#pragma unused(env)
#endif

    return(to_string(((struct module_definition *)defmodulePtr)->name));
}

/**************************************************
 * get_module_pp: Returns the pretty print
 *   representation of the specified module_definition.
 ***************************************************/
#if WIN_BTC
#pragma argsused
#endif
char *get_module_pp(void *env, void *defmodulePtr)
{
#if MAC_MCW || WIN_MCW || MAC_XCD
#pragma unused(env)
#endif

    return(((struct module_definition *)defmodulePtr)->pp);
}

/**********************************************
 * remove_all_modules: Removes all defmodules
 *   from the current environment.
 ***********************************************/
void remove_all_modules(void *env)
{
    struct module_definition *nextDefmodule;

    while( get_module_data(env)->submodules != NULL )
    {
        nextDefmodule = get_module_data(env)->submodules->next;
        ReturnDefmodule(env, get_module_data(env)->submodules, FALSE);
        get_module_data(env)->submodules = nextDefmodule;
    }

    get_module_data(env)->current_submodule = NULL;
    get_module_data(env)->last_submodule = NULL;
}

/***********************************************************
 * ReturnDefmodule: Returns the data structures associated
 *   with a module_definition construct to the pool of free memory.
 ************************************************************/
static void ReturnDefmodule(void *env, struct module_definition *theDefmodule, BOOLEAN environmentClear)
{
    int i;
    struct module_item *theItem;
    struct portable_item *theSpec, *nextSpec;

    /*=====================================================
     * Set the current module to the module being deleted.
     *=====================================================*/

    if( theDefmodule == NULL )
    {
        return;
    }

    if( !environmentClear )
    {
        set_current_module(env, (void *)theDefmodule);
    }

    /*============================================
     * Call the free functions for the constructs
     * belonging to this module.
     *============================================*/

    if( theDefmodule->items != NULL )
    {
        if( !environmentClear )
        {
            for( i = 0, theItem = get_module_data(env)->items;
                 (i < get_module_data(env)->items_count) && (theItem != NULL);
                 i++, theItem = theItem->next )
            {
                if( theItem->fn_free != NULL )
                {
                    (*theItem->fn_free)(env, theDefmodule->items[i]);
                }
            }
        }

        core_mem_release(env, theDefmodule->items, sizeof(void *) * get_module_data(env)->items_count);
    }

    /*======================================================
     * Decrement the symbol count for the module_definition's name.
     *======================================================*/

    if( !environmentClear )
    {
        dec_atom_count(env, theDefmodule->name);
    }

    /*====================================
     * Free the items in the import list.
     *====================================*/

    theSpec = theDefmodule->imports;

    while( theSpec != NULL )
    {
        nextSpec = theSpec->next;

        if( !environmentClear )
        {
            if( theSpec->parent_module != NULL )
            {
                dec_atom_count(env, theSpec->parent_module);
            }

            if( theSpec->type != NULL )
            {
                dec_atom_count(env, theSpec->type);
            }

            if( theSpec->name != NULL )
            {
                dec_atom_count(env, theSpec->name);
            }
        }

        core_mem_return_struct(env, portable_item, theSpec);
        theSpec = nextSpec;
    }

    /*====================================
     * Free the items in the export list.
     *====================================*/

    theSpec = theDefmodule->exports;

    while( theSpec != NULL )
    {
        nextSpec = theSpec->next;

        if( !environmentClear )
        {
            if( theSpec->parent_module != NULL )
            {
                dec_atom_count(env, theSpec->parent_module);
            }

            if( theSpec->type != NULL )
            {
                dec_atom_count(env, theSpec->type);
            }

            if( theSpec->name != NULL )
            {
                dec_atom_count(env, theSpec->name);
            }
        }

        core_mem_return_struct(env, portable_item, theSpec);
        theSpec = nextSpec;
    }

    /*=========================================
     * Free the module_definition pretty print string.
     *=========================================*/

    if( theDefmodule->pp != NULL )
    {
        core_mem_release(env, theDefmodule->pp,
                         (int)sizeof(char) * (strlen(theDefmodule->pp) + 1));
    }

    /*=======================
     * Return the user data.
     *=======================*/

    ext_clear_data(env, theDefmodule->ext_datum);

    /*======================================
     * Return the module_definition data structure.
     *======================================*/

    core_mem_return_struct(env, module_definition, theDefmodule);
}

/*********************************************************************
 * lookup_module: Searches for a module_definition in the list of defmodules.
 *   Returns a pointer to the module_definition if found, otherwise NULL.
 **********************************************************************/
void *lookup_module(void *env, char *defmoduleName)
{
    struct module_definition *defmodulePtr;
    ATOM_HN *findValue;

    if((findValue = (ATOM_HN *)lookup_atom(env, defmoduleName)) == NULL )
    {
        return(NULL);
    }

    defmodulePtr = get_module_data(env)->submodules;

    while( defmodulePtr != NULL )
    {
        if( defmodulePtr->name == findValue )
        {
            return((void *)defmodulePtr);
        }

        defmodulePtr = defmodulePtr->next;
    }

    return(NULL);
}

/************************************************
 * broccoli_get_current_module: H/L access routine
 *   for the get-current-module command.
 *************************************************/
void *broccoli_get_current_module(void *env)
{
    struct module_definition *theModule;

    core_check_arg_count(env, "get-current-module", EXACTLY, 0);

    theModule = (struct module_definition *)get_current_module(env);

    if( theModule == NULL )
    {
        return((ATOM_HN *)get_false(env));
    }

    return((ATOM_HN *)store_atom(env, to_string(theModule->name)));
}

/************************************************
 * broccoli_set_current_module: H/L access routine
 *   for the set-current-module command.
 *************************************************/
void *broccoli_set_current_module(void *env)
{
    core_data_object argPtr;
    char *argument;
    struct module_definition *theModule;
    ATOM_HN *defaultReturn;

    /*=====================================================
     * Check for the correct number and type of arguments.
     *=====================================================*/

    theModule = ((struct module_definition *)get_current_module(env));

    if( theModule == NULL )
    {
        return((ATOM_HN *)get_false(env));
    }

    defaultReturn = (ATOM_HN *)store_atom(env, to_string(((struct module_definition *)get_current_module(env))->name));

    if( core_check_arg_count(env, "set-current-module", EXACTLY, 1) == -1 )
    {
        return(defaultReturn);
    }

    if( core_check_arg_type(env, "set-current-module", 1, ATOM, &argPtr) == FALSE )
    {
        return(defaultReturn);
    }

    argument = core_convert_data_to_string(argPtr);

    /*================================================
     * Set the current module to the specified value.
     *================================================*/

    theModule = (struct module_definition *)lookup_module(env, argument);

    if( theModule == NULL )
    {
        error_lookup(env, "defmodule", argument);
        return(defaultReturn);
    }

    set_current_module(env, (void *)theModule);

    /*================================
     * Return the new current module.
     *================================*/

    return((ATOM_HN *)defaultReturn);
}

/************************************************
 * add_module_change_listener: Adds a function
 *   to the list of functions to be called after
 *   a module change occurs.
 *************************************************/
void add_module_change_listener(void *env, char *name, void (*func)(void *), int priority)
{
    get_module_data(env)->module_change_listeners =
        core_gc_add_call_function(env, name, priority, func, get_module_data(env)->module_change_listeners, TRUE);
}

/***********************************************
 * error_illegal_module_specifier: Error message
 *   for the illegal use of a module specifier.
 ************************************************/
void error_illegal_module_specifier(void *env)
{
    error_print_id(env, "MODULE", 1, TRUE);
    print_router(env, WERROR, "Illegal use of the module form.\n");
}
