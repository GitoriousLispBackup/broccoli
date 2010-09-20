/* Purpose: Generic Functions Interface Routines             */

/* =========================================
 *****************************************
 *            EXTERNAL DEFINITIONS
 *  =========================================
 ***************************************** */
#include "setup.h"

#if DEFGENERIC_CONSTRUCT

#include <string.h>

#include "core_constructs.h"
#include "parser_generics.h"

#if OBJECT_SYSTEM
#include "classes_kernel.h"
#include "classes_instances_kernel.h"
#endif

#if DEBUGGING_FUNCTIONS
#include "core_watch.h"
#endif

#include "core_arguments.h"
#include "parser_constructs.h"
#include "core_environment.h"
#include "core_functions.h"
#include "generics_call.h"
#include "core_memory.h"
#include "parser_modules.h"
#include "type_list.h"
#include "router.h"

#define _GENRCCOM_SOURCE_
#include "genrccom.h"

/* =========================================
 *****************************************
 *   INTERNALLY VISIBLE FUNCTION HEADERS
 *  =========================================
 ***************************************** */

static void    PrintGenericCall(void *, char *, void *);
static BOOLEAN EvaluateGenericCall(void *, void *, core_data_object *);
static void    DecrementGenericBusyCount(void *, void *);
static void    IncrementGenericBusyCount(void *, void *);
static void    DeallocateDefgenericData(void *);
static void    DestroyDefgenericAction(void *, struct construct_metadata *, void *);


static void SaveDefgenerics(void *, void *, char *);
static void SaveDefmethods(void *, void *, char *);
static void SaveDefmethodsForDefgeneric(void *, struct construct_metadata *, void *);
static void RemoveDefgenericMethod(void *, DEFGENERIC *, long);


#if DEBUGGING_FUNCTIONS
static long     ListMethodsForGeneric(void *, char *, DEFGENERIC *);
static unsigned DefgenericWatchAccess(void *, int, unsigned, core_expression_object *);
static unsigned DefgenericWatchPrint(void *, char *, int, core_expression_object *);
static unsigned DefmethodWatchAccess(void *, int, unsigned, core_expression_object *);
static unsigned DefmethodWatchPrint(void *, char *, int, core_expression_object *);
static unsigned DefmethodWatchSupport(void *, char *, char *, unsigned,
                                      void(*) (void *, char *, void *, long),
                                      void(*) (void *, unsigned, void *, long),
                                      core_expression_object *);
static void PrintMethodWatchFlag(void *, char *, void *, long);
#endif

/* =========================================
 *****************************************
 *       EXTERNALLY VISIBLE FUNCTIONS
 *  =========================================
 ***************************************** */

/***********************************************************
 *  NAME         : SetupGenericFunctions
 *  DESCRIPTION  : Initializes all generic function
 *                data structures, constructs and functions
 *  INPUTS       : None
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Generic function H/L functions set up
 *  NOTES        : None
 ***********************************************************/
globle void SetupGenericFunctions(void *theEnv)
{
    core_data_entity_object genericEntityRecord =
    {"GCALL",                   GCALL,                                    0,                                        0,                         1,
     PrintGenericCall,          PrintGenericCall,
     NULL,                      EvaluateGenericCall,                      NULL,
     DecrementGenericBusyCount, IncrementGenericBusyCount,
     NULL,                      NULL,                                     NULL,                                     NULL,                      NULL};

    core_allocate_environment_data(theEnv, DEFGENERIC_DATA, sizeof(struct defgenericData), DeallocateDefgenericData);
    memcpy(&DefgenericData(theEnv)->generic_info, &genericEntityRecord, sizeof(struct core_data_entity));

    core_install_primitive(theEnv, &DefgenericData(theEnv)->generic_info, GCALL);

    DefgenericData(theEnv)->DefgenericModuleIndex =
        install_module_item(theEnv, "defgeneric",
                           AllocateDefgenericModule, FreeDefgenericModule,
                           NULL,
                           NULL,
                           EnvFindDefgeneric);

    DefgenericData(theEnv)->DefgenericConstruct =  core_define_construct(theEnv, "defgeneric", "defgenerics",
                                                                ParseDefgeneric,
                                                                EnvFindDefgeneric,
                                                                core_get_atom_pointer, core_get_construct_pp,
                                                                core_query_module_header, EnvGetNextDefgeneric,
                                                                core_set_next_construct, EnvIsDefgenericDeletable,
                                                                EnvUndefgeneric,
                                                                RemoveDefgeneric
                                                                );

    core_add_clear_listener(theEnv, "defgeneric", ClearDefgenericsReady, 0);



#if DEFMODULE_CONSTRUCT
    AddPortConstructItem(theEnv, "defgeneric", ATOM);
#endif
    core_define_construct(theEnv, "defmethod", "defmethods", ParseDefmethod,
                 NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

    /* ================================================================
     *  Make sure defmethods are cleared last, for other constructs may
     *   be using them and need to be cleared first
     *
     *  Need to be cleared in two stages so that mutually dependent
     *   constructs (like classes) can be cleared
     *  ================================================================ */
    core_add_saver(theEnv, "defgeneric", SaveDefgenerics, 1000);
    core_add_saver(theEnv, "defmethod", SaveDefmethods, -1000);
    core_define_function(theEnv, "undefgeneric", 'v', PTR_FN UndefgenericCommand, "UndefgenericCommand", "11w");
    core_define_function(theEnv, "undefmethod", 'v', PTR_FN UndefmethodCommand, "UndefmethodCommand", "22*wg");

    core_define_function(theEnv, "call-next-method", 'u', PTR_FN CallNextMethod, "CallNextMethod", "00");
    core_set_function_overload(theEnv, "call-next-method", TRUE, FALSE);
    core_define_function(theEnv, "call-specific-method", 'u', PTR_FN CallSpecificMethod,
                       "CallSpecificMethod", "2**wi");
    core_set_function_overload(theEnv, "call-specific-method", TRUE, FALSE);
    core_define_function(theEnv, "override-next-method", 'u', PTR_FN OverrideNextMethod,
                       "OverrideNextMethod", NULL);
    core_set_function_overload(theEnv, "override-next-method", TRUE, FALSE);
    core_define_function(theEnv, "next-methodp", 'b', PTR_FN NextMethodP, "NextMethodP", "00");
    core_set_function_overload(theEnv, "next-methodp", TRUE, FALSE);

    core_define_function(theEnv, "(gnrc-current-arg)", 'u', PTR_FN GetGenericCurrentArgument,
                       "GetGenericCurrentArgument", NULL);

#if DEBUGGING_FUNCTIONS
    core_define_function(theEnv, "ppdefgeneric", 'v', PTR_FN PPDefgenericCommand, "PPDefgenericCommand", "11w");
    core_define_function(theEnv, "list-defgenerics", 'v', PTR_FN ListDefgenericsCommand, "ListDefgenericsCommand", "01");
    core_define_function(theEnv, "ppdefmethod", 'v', PTR_FN PPDefmethodCommand, "PPDefmethodCommand", "22*wi");
    core_define_function(theEnv, "list-defmethods", 'v', PTR_FN ListDefmethodsCommand, "ListDefmethodsCommand", "01w");
    core_define_function(theEnv, "preview-generic", 'v', PTR_FN PreviewGeneric, "PreviewGeneric", "1**w");
#endif

    core_define_function(theEnv, "get-defgeneric-list", 'm', PTR_FN GetDefgenericListFunction,
                       "GetDefgenericListFunction", "01");
    core_define_function(theEnv, "get-defmethod-list", 'm', PTR_FN GetDefmethodListCommand,
                       "GetDefmethodListCommand", "01w");
    core_define_function(theEnv, "get-method-restrictions", 'm', PTR_FN GetMethodRestrictionsCommand,
                       "GetMethodRestrictionsCommand", "22iw");
    core_define_function(theEnv, "defgeneric-module", 'w', PTR_FN GetDefgenericModuleCommand,
                       "GetDefgenericModuleCommand", "11w");

#if OBJECT_SYSTEM
    core_define_function(theEnv, "type", 'u', PTR_FN ClassCommand, "ClassCommand", "11u");
#else
    core_define_function(theEnv, "type", 'u', PTR_FN TypeCommand, "TypeCommand", "11u");
#endif


#if DEBUGGING_FUNCTIONS
    AddWatchItem(theEnv, "generic-functions", 0, &DefgenericData(theEnv)->WatchGenerics, 34,
                 DefgenericWatchAccess, DefgenericWatchPrint);
    AddWatchItem(theEnv, "methods", 0, &DefgenericData(theEnv)->WatchMethods, 33,
                 DefmethodWatchAccess, DefmethodWatchPrint);
#endif
}

/****************************************************
 * DeallocateDefgenericData: Deallocates environment
 *    data for the defgeneric construct.
 *****************************************************/
static void DeallocateDefgenericData(void *theEnv)
{
    struct defgenericModule *theModuleItem;
    void *module_def;


    core_do_for_all_constructs(theEnv, DestroyDefgenericAction, DefgenericData(theEnv)->DefgenericModuleIndex, FALSE, NULL);

    for( module_def = get_next_module(theEnv, NULL);
         module_def != NULL;
         module_def = get_next_module(theEnv, module_def))
    {
        theModuleItem = (struct defgenericModule *)
                        get_module_item(theEnv, (struct module_definition *)module_def,
                                      DefgenericData(theEnv)->DefgenericModuleIndex);

        core_mem_return_struct(theEnv, defgenericModule, theModuleItem);
    }
}

/***************************************************
 * DestroyDefgenericAction: Action used to remove
 *   defgenerics as a result of core_delete_environment.
 ****************************************************/
#if WIN_BTC
#pragma argsused
#endif
static void DestroyDefgenericAction(void *theEnv, struct construct_metadata *theConstruct, void *buffer)
{
#if MAC_MCW || WIN_MCW || MAC_XCD
#pragma unused(buffer)
#endif
    struct defgeneric *theDefgeneric = (struct defgeneric *)theConstruct;
    long i;

    if( theDefgeneric == NULL )
    {
        return;
    }

    for( i = 0 ; i < theDefgeneric->mcnt ; i++ )
    {
        DestroyMethodInfo(theEnv, theDefgeneric, &theDefgeneric->methods[i]);
    }

    if( theDefgeneric->mcnt != 0 )
    {
        core_mem_release(theEnv, (void *)theDefgeneric->methods, (sizeof(DEFMETHOD) * theDefgeneric->mcnt));
    }

    core_delete_construct_metadata(theEnv, &theDefgeneric->header);

    core_mem_return_struct(theEnv, defgeneric, theDefgeneric);
}

/***************************************************
 *  NAME         : EnvFindDefgeneric
 *  DESCRIPTION  : Searches for a generic
 *  INPUTS       : The name of the generic
 *              (possibly including a module name)
 *  RETURNS      : Pointer to the generic if
 *              found, otherwise NULL
 *  SIDE EFFECTS : None
 *  NOTES        : None
 ***************************************************/
globle void *EnvFindDefgeneric(void *theEnv, char *genericModuleAndName)
{
    return(core_lookup_named_construct(theEnv, genericModuleAndName, DefgenericData(theEnv)->DefgenericConstruct));
}

/***************************************************
 *  NAME         : LookupDefgenericByMdlOrScope
 *  DESCRIPTION  : Finds a defgeneric anywhere (if
 *              module is specified) or in current
 *              or imported modules
 *  INPUTS       : The defgeneric name
 *  RETURNS      : The defgeneric (NULL if not found)
 *  SIDE EFFECTS : Error message printed on
 *               ambiguous references
 *  NOTES        : None
 ***************************************************/
globle DEFGENERIC *LookupDefgenericByMdlOrScope(void *theEnv, char *defgenericName)
{
    return((DEFGENERIC *)core_lookup_construct(theEnv, DefgenericData(theEnv)->DefgenericConstruct, defgenericName, TRUE));
}

/***************************************************
 *  NAME         : LookupDefgenericInScope
 *  DESCRIPTION  : Finds a defgeneric in current or
 *                imported modules (module
 *                specifier is not allowed)
 *  INPUTS       : The defgeneric name
 *  RETURNS      : The defgeneric (NULL if not found)
 *  SIDE EFFECTS : Error message printed on
 *               ambiguous references
 *  NOTES        : None
 ***************************************************/
globle DEFGENERIC *LookupDefgenericInScope(void *theEnv, char *defgenericName)
{
    return((DEFGENERIC *)core_lookup_construct(theEnv, DefgenericData(theEnv)->DefgenericConstruct, defgenericName, FALSE));
}

/***********************************************************
 *  NAME         : EnvGetNextDefgeneric
 *  DESCRIPTION  : Finds first or next generic function
 *  INPUTS       : The address of the current generic function
 *  RETURNS      : The address of the next generic function
 *                (NULL if none)
 *  SIDE EFFECTS : None
 *  NOTES        : If ptr == NULL, the first generic function
 *                 is returned.
 ***********************************************************/
globle void *EnvGetNextDefgeneric(void *theEnv, void *ptr)
{
    return((void *)core_get_next_construct_metadata(theEnv, (struct construct_metadata *)ptr, DefgenericData(theEnv)->DefgenericModuleIndex));
}

/***********************************************************
 *  NAME         : EnvGetNextDefmethod
 *  DESCRIPTION  : Find the next method for a generic function
 *  INPUTS       : 1) The generic function address
 *              2) The index of the current method
 *  RETURNS      : The index of the next method
 *                 (0 if none)
 *  SIDE EFFECTS : None
 *  NOTES        : If index == 0, the index of the first
 *                method is returned
 ***********************************************************/
#if WIN_BTC
#pragma argsused
#endif
globle long EnvGetNextDefmethod(void *theEnv, void *ptr, long theIndex)
{
    DEFGENERIC *gfunc;
    long mi;

#if MAC_MCW || WIN_MCW || MAC_XCD
#pragma unused(theEnv)
#endif

    gfunc = (DEFGENERIC *)ptr;

    if( theIndex == 0 )
    {
        if( gfunc->methods != NULL )
        {
            return(gfunc->methods[0].index);
        }

        return(0);
    }

    mi = FindMethodByIndex(gfunc, theIndex);

    if((mi + 1) == gfunc->mcnt )
    {
        return(0);
    }

    return(gfunc->methods[mi + 1].index);
}

/*****************************************************
 *  NAME         : GetDefmethodPointer
 *  DESCRIPTION  : Returns a pointer to a method
 *  INPUTS       : 1) Pointer to a defgeneric
 *              2) Array index of method in generic's
 *                 method array (+1)
 *  RETURNS      : Pointer to the method.
 *  SIDE EFFECTS : None
 *  NOTES        : None
 *****************************************************/
globle DEFMETHOD *GetDefmethodPointer(void *ptr, long theIndex)
{
    return(&((DEFGENERIC *)ptr)->methods[theIndex - 1]);
}

/***************************************************
 *  NAME         : EnvIsDefgenericDeletable
 *  DESCRIPTION  : Determines if a generic function
 *                can be deleted
 *  INPUTS       : Address of the generic function
 *  RETURNS      : TRUE if deletable, FALSE otherwise
 *  SIDE EFFECTS : None
 *  NOTES        : None
 ***************************************************/
globle int EnvIsDefgenericDeletable(void *theEnv, void *ptr)
{
    if( !core_are_constructs_deleteable(theEnv))
    {
        return(FALSE);
    }

    return((((DEFGENERIC *)ptr)->busy == 0) ? TRUE : FALSE);
}

/***************************************************
 *  NAME         : EnvIsDefmethodDeletable
 *  DESCRIPTION  : Determines if a generic function
 *                method can be deleted
 *  INPUTS       : 1) Address of the generic function
 *              2) Index of the method
 *  RETURNS      : TRUE if deletable, FALSE otherwise
 *  SIDE EFFECTS : None
 *  NOTES        : None
 ***************************************************/
globle int EnvIsDefmethodDeletable(void *theEnv, void *ptr, long theIndex)
{
    if( !core_are_constructs_deleteable(theEnv))
    {
        return(FALSE);
    }

    if(((DEFGENERIC *)ptr)->methods[FindMethodByIndex((DEFGENERIC *)ptr, theIndex)].system )
    {
        return(FALSE);
    }

    return((MethodsExecuting((DEFGENERIC *)ptr) == FALSE) ? TRUE : FALSE);
}

/**********************************************************
 *  NAME         : UndefgenericCommand
 *  DESCRIPTION  : Deletes all methods for a generic function
 *  INPUTS       : None
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : methods deallocated
 *  NOTES        : H/L Syntax: (undefgeneric <name> | *)
 **********************************************************/
globle void UndefgenericCommand(void *theEnv)
{
    core_uninstall_construct_command(theEnv, "undefgeneric", DefgenericData(theEnv)->DefgenericConstruct);
}

/****************************************************************
 *  NAME         : GetDefgenericModuleCommand
 *  DESCRIPTION  : Determines to which module a defgeneric belongs
 *  INPUTS       : None
 *  RETURNS      : The symbolic name of the module
 *  SIDE EFFECTS : None
 *  NOTES        : H/L Syntax: (defgeneric-module <generic-name>)
 ****************************************************************/
globle void *GetDefgenericModuleCommand(void *theEnv)
{
    return(core_query_module_name(theEnv, "defgeneric-module", DefgenericData(theEnv)->DefgenericConstruct));
}

/**************************************************************
 *  NAME         : UndefmethodCommand
 *  DESCRIPTION  : Deletes one method for a generic function
 *  INPUTS       : None
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : methods deallocated
 *  NOTES        : H/L Syntax: (undefmethod <name> <index> | *)
 **************************************************************/
globle void UndefmethodCommand(void *theEnv)
{
    core_data_object temp;
    DEFGENERIC *gfunc;
    long mi;

    if( core_check_arg_type(theEnv, "undefmethod", 1, ATOM, &temp) == FALSE )
    {
        return;
    }

    gfunc = LookupDefgenericByMdlOrScope(theEnv, core_convert_data_to_string(temp));

    if((gfunc == NULL) ? (strcmp(core_convert_data_to_string(temp), "*") != 0) : FALSE )
    {
        error_print_id(theEnv, "GENRCCOM", 1, FALSE);
        print_router(theEnv, WERROR, "No such generic function ");
        print_router(theEnv, WERROR, core_convert_data_to_string(temp));
        print_router(theEnv, WERROR, " in function undefmethod.\n");
        return;
    }

    core_get_arg_at(theEnv, 2, &temp);

    if( temp.type == ATOM )
    {
        if( strcmp(core_convert_data_to_string(temp), "*") != 0 )
        {
            error_print_id(theEnv, "GENRCCOM", 2, FALSE);
            print_router(theEnv, WERROR, "Expected a valid method index in function undefmethod.\n");
            return;
        }

        mi = 0;
    }
    else if( temp.type == INTEGER )
    {
        mi = (long)core_convert_data_to_long(temp);

        if( mi == 0 )
        {
            error_print_id(theEnv, "GENRCCOM", 2, FALSE);
            print_router(theEnv, WERROR, "Expected a valid method index in function undefmethod.\n");
            return;
        }
    }
    else
    {
        error_print_id(theEnv, "GENRCCOM", 2, FALSE);
        print_router(theEnv, WERROR, "Expected a valid method index in function undefmethod.\n");
        return;
    }

    EnvUndefmethod(theEnv, (void *)gfunc, mi);
}

/**************************************************************
 *  NAME         : EnvUndefgeneric
 *  DESCRIPTION  : Deletes all methods for a generic function
 *  INPUTS       : The generic-function address (NULL for all)
 *  RETURNS      : TRUE if generic successfully deleted,
 *              FALSE otherwise
 *  SIDE EFFECTS : methods deallocated
 *  NOTES        : None
 **************************************************************/
globle BOOLEAN EnvUndefgeneric(void *theEnv, void *vptr)
{
    DEFGENERIC *gfunc;
    int success = TRUE;

    gfunc = (DEFGENERIC *)vptr;

    if( gfunc == NULL )
    {
        if( ClearDefmethods(theEnv) == FALSE )
        {
            success = FALSE;
        }

        if( ClearDefgenerics(theEnv) == FALSE )
        {
            success = FALSE;
        }

        return(success);
    }

    if( EnvIsDefgenericDeletable(theEnv, vptr) == FALSE )
    {
        return(FALSE);
    }

    undefine_module_construct(theEnv, (struct construct_metadata *)vptr);
    RemoveDefgeneric(theEnv, gfunc);
    return(TRUE);
}

/**************************************************************
 *  NAME         : EnvUndefmethod
 *  DESCRIPTION  : Deletes one method for a generic function
 *  INPUTS       : 1) Address of generic function (can be NULL)
 *              2) Method index (0 for all)
 *  RETURNS      : TRUE if method deleted successfully,
 *              FALSE otherwise
 *  SIDE EFFECTS : methods deallocated
 *  NOTES        : None
 **************************************************************/
globle BOOLEAN EnvUndefmethod(void *theEnv, void *vptr, long mi)
{
    DEFGENERIC *gfunc;

    long nmi;

    gfunc = (DEFGENERIC *)vptr;

    if( gfunc == NULL )
    {
        if( mi != 0 )
        {
            error_print_id(theEnv, "GENRCCOM", 3, FALSE);
            print_router(theEnv, WERROR, "Incomplete method specification for deletion.\n");
            return(FALSE);
        }

        return(ClearDefmethods(theEnv));
    }

    if( MethodsExecuting(gfunc))
    {
        MethodAlterError(theEnv, gfunc);
        return(FALSE);
    }

    if( mi == 0 )
    {
        RemoveAllExplicitMethods(theEnv, gfunc);
    }
    else
    {
        nmi = CheckMethodExists(theEnv, "undefmethod", gfunc, mi);

        if( nmi == -1 )
        {
            return(FALSE);
        }

        RemoveDefgenericMethod(theEnv, gfunc, nmi);
    }

    return(TRUE);
}

#if DEBUGGING_FUNCTIONS

/*****************************************************
 *  NAME         : EnvGetDefmethodDescription
 *  DESCRIPTION  : Prints a synopsis of method parameter
 *                restrictions into caller's buffer
 *  INPUTS       : 1) Caller's buffer
 *              2) Buffer size (not including space
 *                 for terminating '\0')
 *              3) Address of generic function
 *              4) Index of method
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Caller's buffer written
 *  NOTES        : Terminating '\n' not written
 *****************************************************/
globle void EnvGetDefmethodDescription(void *theEnv, char *buf, int buflen, void *ptr, long theIndex)
{
    DEFGENERIC *gfunc;
    long mi;

#if MAC_MCW || WIN_MCW || MAC_XCD
#pragma unused(theEnv)
#endif

    gfunc = (DEFGENERIC *)ptr;
    mi = FindMethodByIndex(gfunc, theIndex);
    PrintMethod(theEnv, buf, buflen, &gfunc->methods[mi]);
}

/*********************************************************
 *  NAME         : EnvGetDefgenericWatch
 *  DESCRIPTION  : Determines if trace messages are
 *              gnerated when executing generic function
 *  INPUTS       : A pointer to the generic
 *  RETURNS      : TRUE if a trace is active,
 *              FALSE otherwise
 *  SIDE EFFECTS : None
 *  NOTES        : None
 *********************************************************/
#if WIN_BTC
#pragma argsused
#endif
globle unsigned EnvGetDefgenericWatch(void *theEnv, void *theGeneric)
{
#if MAC_MCW || WIN_MCW || MAC_XCD
#pragma unused(theEnv)
#endif

    return(((DEFGENERIC *)theGeneric)->trace);
}

/*********************************************************
 *  NAME         : EnvSetDefgenericWatch
 *  DESCRIPTION  : Sets the trace to ON/OFF for the
 *              generic function
 *  INPUTS       : 1) TRUE to set the trace on,
 *                 FALSE to set it off
 *              2) A pointer to the generic
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Watch flag for the generic set
 *  NOTES        : None
 *********************************************************/
#if WIN_BTC
#pragma argsused
#endif
globle void EnvSetDefgenericWatch(void *theEnv, unsigned newState, void *theGeneric)
{
#if MAC_MCW || WIN_MCW || MAC_XCD
#pragma unused(theEnv)
#endif

    ((DEFGENERIC *)theGeneric)->trace = newState;
}

/*********************************************************
 *  NAME         : EnvGetDefmethodWatch
 *  DESCRIPTION  : Determines if trace messages for calls
 *              to this method will be generated or not
 *  INPUTS       : 1) A pointer to the generic
 *              2) The index of the method
 *  RETURNS      : TRUE if a trace is active,
 *              FALSE otherwise
 *  SIDE EFFECTS : None
 *  NOTES        : None
 *********************************************************/
#if WIN_BTC
#pragma argsused
#endif
globle unsigned EnvGetDefmethodWatch(void *theEnv, void *theGeneric, long theIndex)
{
    DEFGENERIC *gfunc;
    long mi;

#if MAC_MCW || WIN_MCW || MAC_XCD
#pragma unused(theEnv)
#endif

    gfunc = (DEFGENERIC *)theGeneric;
    mi = FindMethodByIndex(gfunc, theIndex);
    return(gfunc->methods[mi].trace);
}

/*********************************************************
 *  NAME         : EnvSetDefmethodWatch
 *  DESCRIPTION  : Sets the trace to ON/OFF for the
 *              calling of the method
 *  INPUTS       : 1) TRUE to set the trace on,
 *                 FALSE to set it off
 *              2) A pointer to the generic
 *              3) The index of the method
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Watch flag for the method set
 *  NOTES        : None
 *********************************************************/
#if WIN_BTC
#pragma argsused
#endif
globle void EnvSetDefmethodWatch(void *theEnv, unsigned newState, void *theGeneric, long theIndex)
{
    DEFGENERIC *gfunc;
    long mi;

#if MAC_MCW || WIN_MCW || MAC_XCD
#pragma unused(theEnv)
#endif

    gfunc = (DEFGENERIC *)theGeneric;
    mi = FindMethodByIndex(gfunc, theIndex);
    gfunc->methods[mi].trace = newState;
}

/********************************************************
 *  NAME         : PPDefgenericCommand
 *  DESCRIPTION  : Displays the pretty-print form of
 *               a generic function header
 *  INPUTS       : None
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : None
 *  NOTES        : H/L Syntax: (ppdefgeneric <name>)
 ********************************************************/
globle void PPDefgenericCommand(void *theEnv)
{
    core_construct_pp_command(theEnv, "ppdefgeneric", DefgenericData(theEnv)->DefgenericConstruct);
}

/**********************************************************
 *  NAME         : PPDefmethodCommand
 *  DESCRIPTION  : Displays the pretty-print form of
 *               a method
 *  INPUTS       : None
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : None
 *  NOTES        : H/L Syntax: (ppdefmethod <name> <index>)
 **********************************************************/
globle void PPDefmethodCommand(void *theEnv)
{
    core_data_object temp;
    char *gname;
    DEFGENERIC *gfunc;
    int gi;

    if( core_check_arg_type(theEnv, "ppdefmethod", 1, ATOM, &temp) == FALSE )
    {
        return;
    }

    gname = core_convert_data_to_string(temp);

    if( core_check_arg_type(theEnv, "ppdefmethod", 2, INTEGER, &temp) == FALSE )
    {
        return;
    }

    gfunc = CheckGenericExists(theEnv, "ppdefmethod", gname);

    if( gfunc == NULL )
    {
        return;
    }

    gi = CheckMethodExists(theEnv, "ppdefmethod", gfunc, (long)core_convert_data_to_long(temp));

    if( gi == -1 )
    {
        return;
    }

    if( gfunc->methods[gi].pp != NULL )
    {
        core_print_chunkify(theEnv, WDISPLAY, gfunc->methods[gi].pp);
    }
}

/******************************************************
 *  NAME         : ListDefmethodsCommand
 *  DESCRIPTION  : Lists a brief description of methods
 *                for a particular generic function
 *  INPUTS       : None
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : None
 *  NOTES        : H/L Syntax: (list-defmethods <name>)
 ******************************************************/
globle void ListDefmethodsCommand(void *theEnv)
{
    core_data_object temp;
    DEFGENERIC *gfunc;

    if( core_get_arg_count(theEnv) == 0 )
    {
        EnvListDefmethods(theEnv, WDISPLAY, NULL);
    }
    else
    {
        if( core_check_arg_type(theEnv, "list-defmethods", 1, ATOM, &temp) == FALSE )
        {
            return;
        }

        gfunc = CheckGenericExists(theEnv, "list-defmethods", core_convert_data_to_string(temp));

        if( gfunc != NULL )
        {
            EnvListDefmethods(theEnv, WDISPLAY, (void *)gfunc);
        }
    }
}

/***************************************************************
 *  NAME         : EnvGetDefmethodPPForm
 *  DESCRIPTION  : Getsa generic function method pretty print form
 *  INPUTS       : 1) Address of the generic function
 *              2) Index of the method
 *  RETURNS      : Method ppform
 *  SIDE EFFECTS : None
 *  NOTES        : None
 ***************************************************************/
#if WIN_BTC
#pragma argsused
#endif
globle char *EnvGetDefmethodPPForm(void *theEnv, void *ptr, long theIndex)
{
    DEFGENERIC *gfunc;
    int mi;

#if MAC_MCW || WIN_MCW || MAC_XCD
#pragma unused(theEnv)
#endif

    gfunc = (DEFGENERIC *)ptr;
    mi = FindMethodByIndex(gfunc, theIndex);
    return(gfunc->methods[mi].pp);
}

/***************************************************
 *  NAME         : ListDefgenericsCommand
 *  DESCRIPTION  : Displays all defgeneric names
 *  INPUTS       : None
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Defgeneric names printed
 *  NOTES        : H/L Interface
 ***************************************************/
globle void ListDefgenericsCommand(void *theEnv)
{
    core_garner_module_constructs_command(theEnv, "list-defgenerics", DefgenericData(theEnv)->DefgenericConstruct);
}

/***************************************************
 *  NAME         : EnvListDefgenerics
 *  DESCRIPTION  : Displays all defgeneric names
 *  INPUTS       : 1) The logical name of the output
 *              2) The module
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Defgeneric names printed
 *  NOTES        : C Interface
 ***************************************************/
globle void EnvListDefgenerics(void *theEnv, char *logical_name, struct module_definition *module_def)
{
    core_get_module_constructs_list(theEnv, DefgenericData(theEnv)->DefgenericConstruct, logical_name, module_def);
}

/******************************************************
 *  NAME         : EnvListDefmethods
 *  DESCRIPTION  : Lists a brief description of methods
 *                for a particular generic function
 *  INPUTS       : 1) The logical name of the output
 *              2) Generic function to list methods for
 *                 (NULL means list all methods)
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : None
 *  NOTES        : None
 ******************************************************/
globle void EnvListDefmethods(void *theEnv, char *logical_name, void *vptr)
{
    DEFGENERIC *gfunc;
    long count;

    if( vptr != NULL )
    {
        count = ListMethodsForGeneric(theEnv, logical_name, (DEFGENERIC *)vptr);
    }
    else
    {
        count = 0L;

        for( gfunc = (DEFGENERIC *)EnvGetNextDefgeneric(theEnv, NULL) ;
             gfunc != NULL ;
             gfunc = (DEFGENERIC *)EnvGetNextDefgeneric(theEnv, (void *)gfunc))
        {
            count += ListMethodsForGeneric(theEnv, logical_name, gfunc);

            if( EnvGetNextDefgeneric(theEnv, (void *)gfunc) != NULL )
            {
                print_router(theEnv, logical_name, "\n");
            }
        }
    }

    core_print_tally(theEnv, logical_name, count, "method", "methods");
}

#endif

/***************************************************************
 *  NAME         : GetDefgenericListFunction
 *  DESCRIPTION  : Groups all defgeneric names into
 *              a list list
 *  INPUTS       : A data object buffer to hold
 *              the list result
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : List allocated and filled
 *  NOTES        : H/L Syntax: (get-defgeneric-list [<module>])
 ***************************************************************/
globle void GetDefgenericListFunction(void *theEnv, core_data_object*returnValue)
{
    core_garner_construct_list(theEnv, "get-defgeneric-list", returnValue, DefgenericData(theEnv)->DefgenericConstruct);
}

/***************************************************************
 *  NAME         : EnvGetDefgenericList
 *  DESCRIPTION  : Groups all defgeneric names into
 *              a list list
 *  INPUTS       : 1) A data object buffer to hold
 *                 the list result
 *              2) The module from which to obtain defgenerics
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : List allocated and filled
 *  NOTES        : External C access
 ***************************************************************/
globle void EnvGetDefgenericList(void *theEnv, core_data_object *returnValue, struct module_definition *module_def)
{
    core_get_module_constructs_list(theEnv, returnValue, DefgenericData(theEnv)->DefgenericConstruct, module_def);
}

/***********************************************************
 *  NAME         : GetDefmethodListCommand
 *  DESCRIPTION  : Groups indices of all methdos for a generic
 *              function into a list variable
 *              (NULL means get methods for all generics)
 *  INPUTS       : A data object buffer
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : List set to list of method indices
 *  NOTES        : None
 ***********************************************************/
globle void GetDefmethodListCommand(void *theEnv, core_data_object_ptr returnValue)
{
    core_data_object temp;
    DEFGENERIC *gfunc;

    if( core_get_arg_count(theEnv) == 0 )
    {
        EnvGetDefmethodList(theEnv, NULL, returnValue);
    }
    else
    {
        if( core_check_arg_type(theEnv, "get-defmethod-list", 1, ATOM, &temp) == FALSE )
        {
            core_create_error_list(theEnv, returnValue);
            return;
        }

        gfunc = CheckGenericExists(theEnv, "get-defmethod-list", core_convert_data_to_string(temp));

        if( gfunc != NULL )
        {
            EnvGetDefmethodList(theEnv, (void *)gfunc, returnValue);
        }
        else
        {
            core_create_error_list(theEnv, returnValue);
        }
    }
}

/***********************************************************
 *  NAME         : EnvGetDefmethodList
 *  DESCRIPTION  : Groups indices of all methdos for a generic
 *              function into a list variable
 *              (NULL means get methods for all generics)
 *  INPUTS       : 1) A pointer to a generic function
 *              2) A data object buffer
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : List set to list of method indices
 *  NOTES        : None
 ***********************************************************/
globle void EnvGetDefmethodList(void *theEnv, void *vgfunc, core_data_object_ptr returnValue)
{
    DEFGENERIC *gfunc, *svg, *svnxt;
    long i, j;
    unsigned long count;
    LIST_PTR theList;

    if( vgfunc != NULL )
    {
        gfunc = (DEFGENERIC *)vgfunc;
        svnxt = (DEFGENERIC *)EnvGetNextDefgeneric(theEnv, vgfunc);
        SetNextDefgeneric(vgfunc, NULL);
    }
    else
    {
        gfunc = (DEFGENERIC *)EnvGetNextDefgeneric(theEnv, NULL);
        svnxt = (gfunc != NULL) ? (DEFGENERIC *)EnvGetNextDefgeneric(theEnv, (void *)gfunc) : NULL;
    }

    count = 0;

    for( svg = gfunc ;
         gfunc != NULL ;
         gfunc = (DEFGENERIC *)EnvGetNextDefgeneric(theEnv, (void *)gfunc))
    {
        count += (unsigned long)gfunc->mcnt;
    }

    count *= 2;
    core_set_pointer_type(returnValue, LIST);
    core_set_data_ptr_start(returnValue, 1);
    core_set_data_ptr_end(returnValue, count);
    theList = (LIST_PTR)create_list(theEnv, count);
    core_set_pointer_value(returnValue, theList);

    for( gfunc = svg, i = 1 ;
         gfunc != NULL ;
         gfunc = (DEFGENERIC *)EnvGetNextDefgeneric(theEnv, (void *)gfunc))
    {
        for( j = 0 ; j < gfunc->mcnt ; j++ )
        {
            set_list_node_type(theList, i, ATOM);
            set_list_node_value(theList, i++, GetDefgenericNamePointer((void *)gfunc));
            set_list_node_type(theList, i, INTEGER);
            set_list_node_value(theList, i++, store_long(theEnv, (long long)gfunc->methods[j].index));
        }
    }

    if( svg != NULL )
    {
        SetNextDefgeneric((void *)svg, (void *)svnxt);
    }
}

/***********************************************************************************
 *  NAME         : GetMethodRestrictionsCommand
 *  DESCRIPTION  : Stores restrictions of a method in list
 *  INPUTS       : A data object buffer to hold a list
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : List created (length zero on errors)
 *  NOTES        : Syntax: (get-method-restrictions <generic-function> <method-index>)
 ***********************************************************************************/
globle void GetMethodRestrictionsCommand(void *theEnv, core_data_object *result)
{
    core_data_object temp;
    DEFGENERIC *gfunc;

    if( core_check_arg_type(theEnv, "get-method-restrictions", 1, ATOM, &temp) == FALSE )
    {
        core_create_error_list(theEnv, result);
        return;
    }

    gfunc = CheckGenericExists(theEnv, "get-method-restrictions", core_convert_data_to_string(temp));

    if( gfunc == NULL )
    {
        core_create_error_list(theEnv, result);
        return;
    }

    if( core_check_arg_type(theEnv, "get-method-restrictions", 2, INTEGER, &temp) == FALSE )
    {
        core_create_error_list(theEnv, result);
        return;
    }

    if( CheckMethodExists(theEnv, "get-method-restrictions", gfunc, (long)core_convert_data_to_long(temp)) == -1 )
    {
        core_create_error_list(theEnv, result);
        return;
    }

    EnvGetMethodRestrictions(theEnv, (void *)gfunc, (unsigned)core_convert_data_to_long(temp), result);
}

/***********************************************************************
 *  NAME         : EnvGetMethodRestrictions
 *  DESCRIPTION  : Stores restrictions of a method in list
 *  INPUTS       : 1) Pointer to the generic function
 *              2) The method index
 *              3) A data object buffer to hold a list
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : List created (length zero on errors)
 *  NOTES        : The restrictions are stored in the list
 *              in the following format:
 *
 *              <min-number-of-arguments>
 *              <max-number-of-arguments> (-1 if wildcard allowed)
 *              <restriction-count>
 *              <index of 1st restriction>
 *                    .
 *                    .
 *              <index of nth restriction>
 *              <restriction 1>
 *                  <query TRUE/FALSE>
 *                  <number-of-classes>
 *                  <class 1>
 *                     .
 *                     .
 *                  <class n>
 *                 .
 *                 .
 *                 .
 *               <restriction n>
 *
 *               Thus, for the method
 *               (defmethod foo ((?a NUMBER ATOM) (?b (= 1 1)) $?c))
 *               (get-method-restrictions foo 1) would yield
 *
 *               (2 -1 3 7 11 13 FALSE 2 NUMBER ATOM TRUE 0 FALSE 0)
 ***********************************************************************/
globle void EnvGetMethodRestrictions(void *theEnv, void *vgfunc, long mi, core_data_object *result)
{
    short i, j;
    register DEFMETHOD *meth;
    register RESTRICTION *rptr;
    long count;
    int roffset, rstrctIndex;
    LIST_PTR theList;

    meth = ((DEFGENERIC *)vgfunc)->methods + FindMethodByIndex((DEFGENERIC *)vgfunc, mi);
    count = 3;

    for( i = 0 ; i < meth->restrictionCount ; i++ )
    {
        count += meth->restrictions[i].tcnt + 3;
    }

    theList = (LIST_PTR)create_list(theEnv, count);
    core_set_pointer_type(result, LIST);
    core_set_pointer_value(result, theList);
    core_set_data_ptr_start(result, 1);
    core_set_data_ptr_end(result, count);
    set_list_node_type(theList, 1, INTEGER);
    set_list_node_value(theList, 1, store_long(theEnv, (long long)meth->minRestrictions));
    set_list_node_type(theList, 2, INTEGER);
    set_list_node_value(theList, 2, store_long(theEnv, (long long)meth->maxRestrictions));
    set_list_node_type(theList, 3, INTEGER);
    set_list_node_value(theList, 3, store_long(theEnv, (long long)meth->restrictionCount));
    roffset = 3 + meth->restrictionCount + 1;
    rstrctIndex = 4;

    for( i = 0 ; i < meth->restrictionCount ; i++ )
    {
        rptr = meth->restrictions + i;
        set_list_node_type(theList, rstrctIndex, INTEGER);
        set_list_node_value(theList, rstrctIndex++, store_long(theEnv, (long long)roffset));
        set_list_node_type(theList, roffset, ATOM);
        set_list_node_value(theList, roffset++, (rptr->query != NULL) ? get_true(theEnv) : get_false(theEnv));
        set_list_node_type(theList, roffset, INTEGER);
        set_list_node_value(theList, roffset++, store_long(theEnv, (long long)rptr->tcnt));

        for( j = 0 ; j < rptr->tcnt ; j++ )
        {
            set_list_node_type(theList, roffset, ATOM);
#if OBJECT_SYSTEM
            set_list_node_value(theList, roffset++, store_atom(theEnv, EnvGetDefclassName(theEnv, rptr->types[j])));
#else
            set_list_node_value(theList, roffset++, store_atom(theEnv, TypeName(theEnv, to_int(rptr->types[j]))));
#endif
        }
    }
}

/* =========================================
 *****************************************
 *       INTERNALLY VISIBLE FUNCTIONS
 *  =========================================
 ***************************************** */

/***************************************************
 *  NAME         : PrintGenericCall
 *  DESCRIPTION  : core_print_expression() support function
 *              for generic function calls
 *  INPUTS       : 1) The output logical name
 *              2) The generic function
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Call expression printed
 *  NOTES        : None
 ***************************************************/
#if WIN_BTC && (!DEVELOPER)
#pragma argsused
#endif
static void PrintGenericCall(void *theEnv, char *logName, void *value)
{
#if DEVELOPER

    print_router(theEnv, logName, "(");
    print_router(theEnv, logName, EnvGetDefgenericName(theEnv, value));

    if( core_get_first_arg() != NULL )
    {
        print_router(theEnv, logName, " ");
        core_print_expression(theEnv, logName, core_get_first_arg());
    }

    print_router(theEnv, logName, ")");
#else
#if MAC_MCW || WIN_MCW || MAC_XCD
#pragma unused(theEnv)
#pragma unused(logName)
#pragma unused(value)
#endif
#endif
}

/*******************************************************
 *  NAME         : EvaluateGenericCall
 *  DESCRIPTION  : Primitive support function for
 *              calling a generic function
 *  INPUTS       : 1) The generic function
 *              2) A data object buffer to hold
 *                 the evaluation result
 *  RETURNS      : FALSE if the generic function
 *              returns the symbol FALSE,
 *              TRUE otherwise
 *  SIDE EFFECTS : Data obejct buffer set and any
 *              side-effects of calling the generic
 *  NOTES        : None
 *******************************************************/
static BOOLEAN EvaluateGenericCall(void *theEnv, void *value, core_data_object *result)
{
    GenericDispatch(theEnv, (DEFGENERIC *)value, NULL, NULL, core_get_first_arg(), result);

    if((core_get_pointer_type(result) == ATOM) &&
       (core_get_pointer_value(result) == get_false(theEnv)))
    {
        return(FALSE);
    }

    return(TRUE);
}

/***************************************************
 *  NAME         : DecrementGenericBusyCount
 *  DESCRIPTION  : Lowers the busy count of a
 *              generic function construct
 *  INPUTS       : The generic function
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Busy count decremented if a clear
 *              is not in progress (see comment)
 *  NOTES        : None
 ***************************************************/
static void DecrementGenericBusyCount(void *theEnv, void *value)
{
    /* ==============================================
     *  The generics to which expressions in other
     *  constructs may refer may already have been
     *  deleted - thus, it is important not to modify
     *  the busy flag during a clear.
     *  ============================================== */
    if( !core_construct_data(theEnv)->clear_in_progress )
    {
        ((DEFGENERIC *)value)->busy--;
    }
}

/***************************************************
 *  NAME         : IncrementGenericBusyCount
 *  DESCRIPTION  : Raises the busy count of a
 *              generic function construct
 *  INPUTS       : The generic function
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Busy count incremented
 *  NOTES        : None
 ***************************************************/
#if WIN_BTC
#pragma argsused
#endif
static void IncrementGenericBusyCount(void *theEnv, void *value)
{
#if MAC_MCW || WIN_MCW || MAC_XCD
#pragma unused(theEnv)
#endif
    ((DEFGENERIC *)value)->busy++;
}

/**********************************************************************
 *  NAME         : SaveDefgenerics
 *  DESCRIPTION  : Outputs pretty-print forms of generic function headers
 *  INPUTS       : The logical name of the output
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : None
 *  NOTES        : None
 **********************************************************************/
static void SaveDefgenerics(void *theEnv, void *module_def, char *logName)
{
    core_save_construct(theEnv, module_def, logName, DefgenericData(theEnv)->DefgenericConstruct);
}

/**********************************************************************
 *  NAME         : SaveDefmethods
 *  DESCRIPTION  : Outputs pretty-print forms of generic function methods
 *  INPUTS       : The logical name of the output
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : None
 *  NOTES        : None
 **********************************************************************/
static void SaveDefmethods(void *theEnv, void *module_def, char *logName)
{
    core_do_for_all_constructs_in_module(theEnv, module_def, SaveDefmethodsForDefgeneric,
                               DefgenericData(theEnv)->DefgenericModuleIndex,
                               FALSE, (void *)logName);
}

/***************************************************
 *  NAME         : SaveDefmethodsForDefgeneric
 *  DESCRIPTION  : Save the pretty-print forms of
 *              all methods for a generic function
 *              to a file
 *  INPUTS       : 1) The defgeneric
 *              2) The logical name of the output
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Methods written
 *  NOTES        : None
 ***************************************************/
static void SaveDefmethodsForDefgeneric(void *theEnv, struct construct_metadata *theDefgeneric, void *userBuffer)
{
    DEFGENERIC *gfunc = (DEFGENERIC *)theDefgeneric;
    char *logName = (char *)userBuffer;
    long i;

    for( i = 0 ; i < gfunc->mcnt ; i++ )
    {
        if( gfunc->methods[i].pp != NULL )
        {
            core_print_chunkify(theEnv, logName, gfunc->methods[i].pp);
            print_router(theEnv, logName, "\n");
        }
    }
}

/****************************************************
 *  NAME         : RemoveDefgenericMethod
 *  DESCRIPTION  : Removes a generic function method
 *                from the array and removes the
 *                generic too if its the last method
 *  INPUTS       : 1) The generic function
 *              2) The array index of the method
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : List adjusted
 *              Nodes deallocated
 *  NOTES        : Assumes deletion is safe
 ****************************************************/
static void RemoveDefgenericMethod(void *theEnv, DEFGENERIC *gfunc, long gi)
{
    DEFMETHOD *narr;
    long b, e;

    if( gfunc->methods[gi].system )
    {
        core_set_eval_error(theEnv, TRUE);
        error_print_id(theEnv, "GENRCCOM", 4, FALSE);
        print_router(theEnv, WERROR, "Cannot remove implicit system function method for generic function ");
        print_router(theEnv, WERROR, EnvGetDefgenericName(theEnv, (void *)gfunc));
        print_router(theEnv, WERROR, ".\n");
        return;
    }

    DeleteMethodInfo(theEnv, gfunc, &gfunc->methods[gi]);

    if( gfunc->mcnt == 1 )
    {
        core_mem_release(theEnv, (void *)gfunc->methods, (int)sizeof(DEFMETHOD));
        gfunc->mcnt = 0;
        gfunc->methods = NULL;
    }
    else
    {
        gfunc->mcnt--;
        narr = (DEFMETHOD *)core_mem_alloc_no_init(theEnv, (sizeof(DEFMETHOD) * gfunc->mcnt));

        for( b = e = 0 ; b < gfunc->mcnt ; b++, e++ )
        {
            if(((int)b) == gi )
            {
                e++;
            }

            core_mem_copy_memory(DEFMETHOD, 1, &narr[b], &gfunc->methods[e]);
        }

        core_mem_release(theEnv, (void *)gfunc->methods, (sizeof(DEFMETHOD) * (gfunc->mcnt + 1)));
        gfunc->methods = narr;
    }
}

#if DEBUGGING_FUNCTIONS

/******************************************************
 *  NAME         : ListMethodsForGeneric
 *  DESCRIPTION  : Lists a brief description of methods
 *                for a particular generic function
 *  INPUTS       : 1) The logical name of the output
 *              2) Generic function to list methods for
 *  RETURNS      : The number of methods printed
 *  SIDE EFFECTS : None
 *  NOTES        : None
 ******************************************************/
static long ListMethodsForGeneric(void *theEnv, char *logical_name, DEFGENERIC *gfunc)
{
    long gi;
    char buf[256];

    for( gi = 0 ; gi < gfunc->mcnt ; gi++ )
    {
        print_router(theEnv, logical_name, EnvGetDefgenericName(theEnv, (void *)gfunc));
        print_router(theEnv, logical_name, " #");
        PrintMethod(theEnv, buf, 255, &gfunc->methods[gi]);
        print_router(theEnv, logical_name, buf);
        print_router(theEnv, logical_name, "\n");
    }

    return((long)gfunc->mcnt);
}

/******************************************************************
 *  NAME         : DefgenericWatchAccess
 *  DESCRIPTION  : Parses a list of generic names passed by
 *              AddWatchItem() and sets the traces accordingly
 *  INPUTS       : 1) A code indicating which trace flag is to be set
 *                 Ignored
 *              2) The value to which to set the trace flags
 *              3) A list of expressions containing the names
 *                 of the generics for which to set traces
 *  RETURNS      : TRUE if all OK, FALSE otherwise
 *  SIDE EFFECTS : Watch flags set in specified generics
 *  NOTES        : Accessory function for AddWatchItem()
 ******************************************************************/
#if WIN_BTC
#pragma argsused
#endif
static unsigned DefgenericWatchAccess(void *theEnv, int code, unsigned newState, core_expression_object *argExprs)
{
#if MAC_MCW || WIN_MCW || MAC_XCD
#pragma unused(code)
#endif

    return(ConstructSetWatchAccess(theEnv, DefgenericData(theEnv)->DefgenericConstruct, newState, argExprs,
                                   EnvGetDefgenericWatch, EnvSetDefgenericWatch));
}

/***********************************************************************
 *  NAME         : DefgenericWatchPrint
 *  DESCRIPTION  : Parses a list of generic names passed by
 *              AddWatchItem() and displays the traces accordingly
 *  INPUTS       : 1) The logical name of the output
 *              2) A code indicating which trace flag is to be examined
 *                 Ignored
 *              3) A list of expressions containing the names
 *                 of the generics for which to examine traces
 *  RETURNS      : TRUE if all OK, FALSE otherwise
 *  SIDE EFFECTS : Watch flags displayed for specified generics
 *  NOTES        : Accessory function for AddWatchItem()
 ***********************************************************************/
#if WIN_BTC
#pragma argsused
#endif
static unsigned DefgenericWatchPrint(void *theEnv, char *logName, int code, core_expression_object *argExprs)
{
#if MAC_MCW || WIN_MCW || MAC_XCD
#pragma unused(code)
#endif

    return(ConstructPrintWatchAccess(theEnv, DefgenericData(theEnv)->DefgenericConstruct, logName, argExprs,
                                     EnvGetDefgenericWatch, EnvSetDefgenericWatch));
}

/******************************************************************
 *  NAME         : DefmethodWatchAccess
 *  DESCRIPTION  : Parses a list of methods passed by
 *              AddWatchItem() and sets the traces accordingly
 *  INPUTS       : 1) A code indicating which trace flag is to be set
 *                 Ignored
 *              2) The value to which to set the trace flags
 *              3) A list of expressions containing the methods
 *                for which to set traces
 *  RETURNS      : TRUE if all OK, FALSE otherwise
 *  SIDE EFFECTS : Watch flags set in specified methods
 *  NOTES        : Accessory function for AddWatchItem()
 ******************************************************************/
#if WIN_BTC
#pragma argsused
#endif
static unsigned DefmethodWatchAccess(void *theEnv, int code, unsigned newState, core_expression_object *argExprs)
{
#if MAC_MCW || WIN_MCW || MAC_XCD
#pragma unused(code)
#endif

    if( newState )
    {
        return(DefmethodWatchSupport(theEnv, "watch", NULL, newState, NULL, EnvSetDefmethodWatch, argExprs));
    }
    else
    {
        return(DefmethodWatchSupport(theEnv, "unwatch", NULL, newState, NULL, EnvSetDefmethodWatch, argExprs));
    }
}

/***********************************************************************
 *  NAME         : DefmethodWatchPrint
 *  DESCRIPTION  : Parses a list of methods passed by
 *              AddWatchItem() and displays the traces accordingly
 *  INPUTS       : 1) The logical name of the output
 *              2) A code indicating which trace flag is to be examined
 *                 Ignored
 *              3) A list of expressions containing the methods for
 *                 which to examine traces
 *  RETURNS      : TRUE if all OK, FALSE otherwise
 *  SIDE EFFECTS : Watch flags displayed for specified methods
 *  NOTES        : Accessory function for AddWatchItem()
 ***********************************************************************/
#if WIN_BTC
#pragma argsused
#endif
static unsigned DefmethodWatchPrint(void *theEnv, char *logName, int code, core_expression_object *argExprs)
{
#if MAC_MCW || WIN_MCW || MAC_XCD
#pragma unused(code)
#endif
    return(DefmethodWatchSupport(theEnv, "list-watch-items", logName, 0,
                                 PrintMethodWatchFlag, NULL, argExprs));
}

/*******************************************************
 *  NAME         : DefmethodWatchSupport
 *  DESCRIPTION  : Sets or displays methods specified
 *  INPUTS       : 1) The calling function name
 *              2) The logical output name for displays
 *                 (can be NULL)
 *              3) The new set state
 *              4) The print function (can be NULL)
 *              5) The trace function (can be NULL)
 *              6) The methods expression list
 *  RETURNS      : TRUE if all OK,
 *              FALSE otherwise
 *  SIDE EFFECTS : Method trace flags set or displayed
 *  NOTES        : None
 *******************************************************/
static unsigned DefmethodWatchSupport(void *theEnv, char *funcName, char *logName, unsigned newState, void (*fn_print)(void *, char *, void *, long), void (*traceFunc)(void *, unsigned, void *, long), core_expression_object *argExprs)
{
    void *theGeneric;
    unsigned long theMethod = 0;
    int argIndex = 2;
    core_data_object genericName, methodIndex;
    struct module_definition *module_def;

    /* ==============================
     *  If no methods are specified,
     *  show the trace for all methods
     *  in all generics
     *  ============================== */
    if( argExprs == NULL )
    {
        save_current_module(theEnv);
        module_def = (struct module_definition *)get_next_module(theEnv, NULL);

        while( module_def != NULL )
        {
            set_current_module(theEnv, (void *)module_def);

            if( traceFunc == NULL )
            {
                print_router(theEnv, logName, get_module_name(theEnv, (void *)module_def));
                print_router(theEnv, logName, ":\n");
            }

            theGeneric = EnvGetNextDefgeneric(theEnv, NULL);

            while( theGeneric != NULL )
            {
                theMethod = EnvGetNextDefmethod(theEnv, theGeneric, 0);

                while( theMethod != 0 )
                {
                    if( traceFunc != NULL )
                    {
                        (*traceFunc)(theEnv, newState, theGeneric, theMethod);
                    }
                    else
                    {
                        print_router(theEnv, logName, "   ");
                        (*fn_print)(theEnv, logName, theGeneric, theMethod);
                    }

                    theMethod = EnvGetNextDefmethod(theEnv, theGeneric, theMethod);
                }

                theGeneric = EnvGetNextDefgeneric(theEnv, theGeneric);
            }

            module_def = (struct module_definition *)get_next_module(theEnv, (void *)module_def);
        }

        restore_current_module(theEnv);
        return(TRUE);
    }

    /* =========================================
     *  Set the traces for every method specified
     *  ========================================= */
    while( argExprs != NULL )
    {
        if( core_eval_expression(theEnv, argExprs, &genericName))
        {
            return(FALSE);
        }

        if((genericName.type != ATOM) ? TRUE :
           ((theGeneric = (void *)
                          LookupDefgenericByMdlOrScope(theEnv, core_convert_data_to_string(genericName))) == NULL))
        {
            report_explicit_type_error(theEnv, funcName, argIndex, "generic function name");
            return(FALSE);
        }

        if( core_get_next_arg(argExprs) == NULL )
        {
            theMethod = 0;
        }
        else
        {
            argExprs = core_get_next_arg(argExprs);
            argIndex++;

            if( core_eval_expression(theEnv, argExprs, &methodIndex))
            {
                return(FALSE);
            }

            if((methodIndex.type != INTEGER) ? FALSE :
               ((core_convert_data_to_long(methodIndex) <= 0) ? FALSE :
                (FindMethodByIndex((DEFGENERIC *)theGeneric, theMethod) != -1)))
            {
                theMethod = (long)core_convert_data_to_long(methodIndex);
            }
            else
            {
                report_explicit_type_error(theEnv, funcName, argIndex, "method index");
                return(FALSE);
            }
        }

        if( theMethod == 0 )
        {
            theMethod = EnvGetNextDefmethod(theEnv, theGeneric, 0);

            while( theMethod != 0 )
            {
                if( traceFunc != NULL )
                {
                    (*traceFunc)(theEnv, newState, theGeneric, theMethod);
                }
                else
                {
                    (*fn_print)(theEnv, logName, theGeneric, theMethod);
                }

                theMethod = EnvGetNextDefmethod(theEnv, theGeneric, theMethod);
            }
        }
        else
        {
            if( traceFunc != NULL )
            {
                (*traceFunc)(theEnv, newState, theGeneric, theMethod);
            }
            else
            {
                (*fn_print)(theEnv, logName, theGeneric, theMethod);
            }
        }

        argExprs = core_get_next_arg(argExprs);
        argIndex++;
    }

    return(TRUE);
}

/***************************************************
 *  NAME         : PrintMethodWatchFlag
 *  DESCRIPTION  : Displays trace value for method
 *  INPUTS       : 1) The logical name of the output
 *              2) The generic function
 *              3) The method index
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : None
 *  NOTES        : None
 ***************************************************/
static void PrintMethodWatchFlag(void *theEnv, char *logName, void *theGeneric, long theMethod)
{
    char buf[60];

    print_router(theEnv, logName, EnvGetDefgenericName(theEnv, theGeneric));
    print_router(theEnv, logName, " ");
    EnvGetDefmethodDescription(theEnv, buf, 59, theGeneric, theMethod);
    print_router(theEnv, logName, buf);

    if( EnvGetDefmethodWatch(theEnv, theGeneric, theMethod))
    {
        print_router(theEnv, logName, " = on\n");
    }
    else
    {
        print_router(theEnv, logName, " = off\n");
    }
}

#endif

#if !OBJECT_SYSTEM

/***************************************************
 *  NAME         : TypeCommand
 *  DESCRIPTION  : Works like "class" in COOL
 *  INPUTS       : None
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : None
 *  NOTES        : H/L Syntax: (type <primitive>)
 ***************************************************/
globle void TypeCommand(void *theEnv, core_data_object *result)
{
    core_eval_expression(theEnv, core_get_first_arg(), result);
    result->value = (void *)store_atom(theEnv, TypeName(theEnv, result->type));
    result->type = ATOM;
}

#endif

#endif
