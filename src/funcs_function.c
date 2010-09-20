/* =========================================
 *****************************************
 *            EXTERNAL DEFINITIONS
 *  =========================================
 ***************************************** */
#include "setup.h"

#if DEFFUNCTION_CONSTRUCT

#include "core_constructs.h"
#include "parser_constructs.h"
#include "parser_functions.h"
#include "parser_modules.h"

#include "core_environment.h"

#include "core_functions.h"

#include "functions_kernel.h"

#if DEBUGGING_FUNCTIONS
#include "core_watch.h"
#endif

#include "core_arguments.h"
#include "core_memory.h"
#include "core_constructs_query.h"
#include "router.h"

#define __FUNCS_FUNCTIONS_SOURCE__
#include "funcs_function.h"

/* =========================================
 *****************************************
 *   INTERNALLY VISIBLE FUNCTION HEADERS
 *  =========================================
 ***************************************** */

static void    _print_function_call(void *, char *, void *);
static BOOLEAN _eval_function_call(void *, void *, core_data_object *);
static void    _inc_busy_count(void *, void *);
static void    _dec_busy_count(void *, void *);
static void    _delete_function_data(void *);

static void    _delete_function(void *, struct construct_metadata *, void *);
static void *  _allocate_module(void *);
static void    _get_module(void *, void *);
static BOOLEAN _clear_functions_ready(void *);

static BOOLEAN _remove_all_functions(void *);
static void    _error_function_definition(void *, char *);
static void    _save_header(void *, void *, char *);
static void    _save_function_header(void *, struct construct_metadata *, void *);
static void    _save_functions(void *, void *, char *);

/* =========================================
 *****************************************
 *       EXTERNALLY VISIBLE FUNCTIONS
 *  =========================================
 ***************************************** */

/***************************************************
 *  NAME         : init_functions
 *  DESCRIPTION  : Initializes parsers and access
 *              functions for FUNCTIONS_GROUP_NAME
 *  INPUTS       : None
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Deffunction environment initialized
 *  NOTES        : None
 ***************************************************/
void init_functions(void *env)
{
    core_data_entity_object deffunctionEntityRecord =
    {"PCALL",              PCALL,                                        0,                                                0,                                                1,
     _print_function_call, _print_function_call,
     NULL,                 _eval_function_call,                          NULL,
     _inc_busy_count,      _dec_busy_count,
     NULL,                 NULL,                                         NULL,                                             NULL,                                             NULL};

    core_allocate_environment_data(env, FUNCTION_DATA_INDEX, sizeof(struct function_data), _delete_function_data);
    memcpy(&get_function_data(env)->entity_record, &deffunctionEntityRecord, sizeof(struct core_data_entity));

    core_install_primitive(env, &get_function_data(env)->entity_record, PCALL);

    get_function_data(env)->module_index =
        install_module_item(env, FUNC_NAME_CREATE_FUNC,
                            _allocate_module, _get_module,
                            NULL,
                            NULL,
                            lookup_function);

    get_function_data(env)->function_construct = core_define_construct(env, FUNC_NAME_CREATE_FUNC, FUNCTIONS_GROUP_NAME,
                                                                       parse_function,
                                                                       lookup_function,
                                                                       core_get_atom_pointer, core_get_construct_pp,
                                                                       core_query_module_header, get_next_function,
                                                                       core_set_next_construct, is_function_deletable,
                                                                       undefine_function,
                                                                       remove_function
                                                                       );
    core_add_clear_listener(env, FUNC_NAME_CREATE_FUNC, _clear_functions_ready, 0);

#if DEFMODULE_CONSTRUCT
    AddPortConstructItem(env, FUNC_NAME_CREATE_FUNC, ATOM);
#endif
    core_add_saver(env, "deffunction-headers", _save_header, 1000);
    core_add_saver(env, FUNCTIONS_GROUP_NAME, _save_functions, 0);

#if DEBUGGING_FUNCTIONS
    core_define_function(env, "list-FUNCTIONS_GROUP_NAME", 'v', PTR_FN ListDeffunctionsCommand, "ListDeffunctionsCommand", "01");
    core_define_function(env, "ppdeffunction", 'v', PTR_FN PPDeffunctionCommand, "PPDeffunctionCommand", "11w");
#endif

#if OFF
    core_define_function(env, "undeffunction", 'v', PTR_FN broccoli_undefine_function, "UndeffunctionCommand", "11w");
    core_define_function(env, "get-deffunction-list", 'm', PTR_FN broccoli_list_functions, "GetDeffunctionListFunction", "01");
    core_define_function(env, "deffunction-module", 'w', PTR_FN broccoli_lookup_function_module, "GetDeffunctionModuleCommand", "11w");
#endif




#if DEBUGGING_FUNCTIONS
    AddWatchItem(env, FUNCTIONS_GROUP_NAME, 0, &get_function_data(env)->is_watching, 32,
                 DeffunctionWatchAccess, DeffunctionWatchPrint);
#endif
}

/*****************************************************
 * DeallocateDeffunctionData: Deallocates environment
 *    data for the deffunction construct.
 ******************************************************/
static void _delete_function_data(void *env)
{
    struct function_module *theModuleItem;
    void *theModule;


    core_do_for_all_constructs(env, _delete_function, get_function_data(env)->module_index, FALSE, NULL);

    for( theModule = get_next_module(env, NULL);
         theModule != NULL;
         theModule = get_next_module(env, theModule))
    {
        theModuleItem = (struct function_module *)
                        get_module_item(env, (struct module_definition *)theModule,
                                        get_function_data(env)->module_index);
        core_mem_return_struct(env, function_module, theModuleItem);
    }
}

/****************************************************
 * DestroyDeffunctionAction: Action used to remove
 *   FUNCTIONS_GROUP_NAME as a result of core_delete_environment.
 *****************************************************/
#if WIN_BTC
#pragma argsused
#endif
static void _delete_function(void *env, struct construct_metadata *cons, void *buffer)
{
#if MAC_MCW || WIN_MCW || MAC_XCD
#pragma unused(buffer)
#endif
    struct function_definition *theDeffunction = (struct function_definition *)cons;

    if( theDeffunction == NULL )
    {
        return;
    }

    core_return_packed_expression(env, theDeffunction->code);

    core_delete_construct_metadata(env, &theDeffunction->header);

    core_mem_return_struct(env, function_definition, theDeffunction);
}

/***************************************************
 *  NAME         : lookup_function
 *  DESCRIPTION  : Searches for a deffunction
 *  INPUTS       : The name of the deffunction
 *              (possibly including a module name)
 *  RETURNS      : Pointer to the deffunction if
 *              found, otherwise NULL
 *  SIDE EFFECTS : None
 *  NOTES        : None
 ***************************************************/
void *lookup_function(void *env, char *dfnxModuleAndName)
{
    return(core_lookup_named_construct(env, dfnxModuleAndName, get_function_data(env)->function_construct));
}

/***************************************************
 *  NAME         : lookup_function_in_scope
 *  DESCRIPTION  : Finds a deffunction in current or
 *                imported modules (module
 *                specifier is not allowed)
 *  INPUTS       : The deffunction name
 *  RETURNS      : The deffunction (NULL if not found)
 *  SIDE EFFECTS : Error message printed on
 *               ambiguous references
 *  NOTES        : None
 ***************************************************/
FUNCTION_DEFINITION *lookup_function_in_scope(void *env, char *deffunctionName)
{
    return((FUNCTION_DEFINITION *)core_lookup_construct(env, get_function_data(env)->function_construct, deffunctionName, FALSE));
}

/***************************************************
 *  NAME         : undefine_function
 *  DESCRIPTION  : External interface routine for
 *              removing a deffunction
 *  INPUTS       : Deffunction pointer
 *  RETURNS      : FALSE if unsuccessful,
 *              TRUE otherwise
 *  SIDE EFFECTS : Deffunction deleted, if possible
 *  NOTES        : None
 ***************************************************/
BOOLEAN undefine_function(void *env, void *vptr)
{
    if( vptr == NULL )
    {
        return(_remove_all_functions(env));
    }

    if( is_function_deletable(env, vptr) == FALSE )
    {
        return(FALSE);
    }

    undefine_module_construct(env, (struct construct_metadata *)vptr);
    remove_function(env, vptr);
    return(TRUE);
}

/****************************************************
 *  NAME         : get_next_function
 *  DESCRIPTION  : Accesses list of FUNCTIONS_GROUP_NAME
 *  INPUTS       : Deffunction pointer
 *  RETURNS      : The next deffunction, or the
 *              first deffunction (if input is NULL)
 *  SIDE EFFECTS : None
 *  NOTES        : None
 ****************************************************/
void *get_next_function(void *env, void *ptr)
{
    return((void *)core_get_next_construct_metadata(env, (struct construct_metadata *)ptr, get_function_data(env)->module_index));
}

/***************************************************
 *  NAME         : is_function_deletable
 *  DESCRIPTION  : Determines if a deffunction is
 *              executing or referenced by another
 *              expression
 *  INPUTS       : Deffunction pointer
 *  RETURNS      : TRUE if the deffunction can
 *              be deleted, FALSE otherwise
 *  SIDE EFFECTS : None
 *  NOTES        : None
 ***************************************************/
int is_function_deletable(void *env, void *ptr)
{
    FUNCTION_DEFINITION *dptr;

    if( !core_are_constructs_deleteable(env))
    {
        return(FALSE);
    }

    dptr = (FUNCTION_DEFINITION *)ptr;

    return(((dptr->busy == 0) && (dptr->executing == 0)) ? TRUE : FALSE);
}

/***************************************************
 *  NAME         : remove_function
 *  DESCRIPTION  : Removes a deffunction
 *  INPUTS       : Deffunction pointer
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Deffunction deallocated
 *  NOTES        : Assumes deffunction is not in use!!
 ***************************************************/
void remove_function(void *env, void *vdptr)
{
    FUNCTION_DEFINITION *dptr = (FUNCTION_DEFINITION *)vdptr;

    if( dptr == NULL )
    {
        return;
    }

    dec_atom_count(env, get_function_name_ptr((void *)dptr));
    core_decrement_expression(env, dptr->code);
    core_return_packed_expression(env, dptr->code);
    set_function_pp((void *)dptr, NULL);
    ext_clear_data(env, dptr->header.ext_data);
    core_mem_return_struct(env, function_definition, dptr);
}

/********************************************************
 *  NAME         : broccoli_undefine_function
 *  DESCRIPTION  : Deletes the named deffunction(s)
 *  INPUTS       : None
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Deffunction(s) removed
 *  NOTES        : H/L Syntax: (undeffunction <name> | *)
 ********************************************************/
void broccoli_undefine_function(void *env)
{
    core_uninstall_construct_command(env, "undeffunction", get_function_data(env)->function_construct);
}

/****************************************************************
 *  NAME         : broccoli_lookup_function_module
 *  DESCRIPTION  : Determines to which module a deffunction belongs
 *  INPUTS       : None
 *  RETURNS      : The symbolic name of the module
 *  SIDE EFFECTS : None
 *  NOTES        : H/L Syntax: (deffunction-module <dfnx-name>)
 ****************************************************************/
void *broccoli_lookup_function_module(void *env)
{
    return(core_query_module_name(env, "deffunction-module", get_function_data(env)->function_construct));
}

/***************************************************************
 *  NAME         : broccoli_list_functions
 *  DESCRIPTION  : Groups all deffunction names into
 *              a list list
 *  INPUTS       : A data object buffer to hold
 *              the list result
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : List allocated and filled
 *  NOTES        : H/L Syntax: (get-deffunction-list [<module>])
 ***************************************************************/
void broccoli_list_functions(void *env, core_data_object *ret)
{
    core_garner_construct_list(env, "get-deffunction-list", ret, get_function_data(env)->function_construct);
}

/*******************************************************
 *  NAME         : verify_function_call
 *  DESCRIPTION  : Checks the number of arguments
 *              passed to a deffunction
 *  INPUTS       : 1) Deffunction pointer
 *              2) The number of arguments
 *  RETURNS      : TRUE if OK, FALSE otherwise
 *  SIDE EFFECTS : Message printed on errors
 *  NOTES        : None
 *******************************************************/
int verify_function_call(void *env, void *vdptr, int args)
{
    FUNCTION_DEFINITION *dptr;

    if( vdptr == NULL )
    {
        return(FALSE);
    }

    dptr = (FUNCTION_DEFINITION *)vdptr;

    if( args < dptr->min_args )
    {
        if( dptr->max_args == -1 )
        {
            report_arg_count_error(env, get_function_name(env, (void *)dptr),
                                   AT_LEAST, dptr->min_args);
        }
        else
        {
            report_arg_count_error(env, get_function_name(env, (void *)dptr),
                                   EXACTLY, dptr->min_args);
        }

        return(FALSE);
    }
    else if((args > dptr->min_args) &&
            (dptr->max_args != -1))
    {
        report_arg_count_error(env, get_function_name(env, (void *)dptr),
                               EXACTLY, dptr->min_args);
        return(FALSE);
    }

    return(TRUE);
}

/* =========================================
 *****************************************
 *       INTERNALLY VISIBLE FUNCTIONS
 *  =========================================
 ***************************************** */

/***************************************************
 *  NAME         : PrintDeffunctionCall
 *  DESCRIPTION  : core_print_expression() support function
 *              for deffunction calls
 *  INPUTS       : 1) The output logical name
 *              2) The deffunction
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Call expression printed
 *  NOTES        : None
 ***************************************************/
#if WIN_BTC
#pragma argsused
#endif
static void _print_function_call(void *env, char *logName, void *value)
{
#if DEVELOPER

    print_router(env, logName, "(");
    print_router(env, logName, get_function_name(env, value));

    if( core_get_first_arg() != NULL )
    {
        print_router(env, logName, " ");
        core_print_expression(env, logName, core_get_first_arg());
    }

    print_router(env, logName, ")");
#else
#if MAC_MCW || WIN_MCW || MAC_XCD
#pragma unused(env)
#pragma unused(logName)
#pragma unused(value)
#endif
#endif
}

/*******************************************************
 *  NAME         : EvaluateDeffunctionCall
 *  DESCRIPTION  : Primitive support function for
 *              calling a deffunction
 *  INPUTS       : 1) The deffunction
 *              2) A data object buffer to hold
 *                 the evaluation result
 *  RETURNS      : FALSE if the deffunction
 *              returns the symbol FALSE,
 *              TRUE otherwise
 *  SIDE EFFECTS : Data obejct buffer set and any
 *              side-effects of calling the deffunction
 *  NOTES        : None
 *******************************************************/
static BOOLEAN _eval_function_call(void *env, void *value, core_data_object *result)
{
    function_execute(env, (FUNCTION_DEFINITION *)value, core_get_first_arg(), result);

    if((core_get_pointer_type(result) == ATOM) &&
       (core_get_pointer_value(result) == get_false(env)))
    {
        return(FALSE);
    }

    return(TRUE);
}

/***************************************************
 *  NAME         : DecrementDeffunctionBusyCount
 *  DESCRIPTION  : Lowers the busy count of a
 *              deffunction construct
 *  INPUTS       : The deffunction
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Busy count decremented if a clear
 *              is not in progress (see comment)
 *  NOTES        : None
 ***************************************************/
static void _inc_busy_count(void *env, void *value)
{
    /* ==============================================
     *  The FUNCTIONS_GROUP_NAME to which expressions in other
     *  constructs may refer may already have been
     *  deleted - thus, it is important not to modify
     *  the busy flag during a clear.
     *  ============================================== */
    if( !core_construct_data(env)->clear_in_progress )
    {
        ((FUNCTION_DEFINITION *)value)->busy--;
    }
}

/***************************************************
 *  NAME         : IncrementDeffunctionBusyCount
 *  DESCRIPTION  : Raises the busy count of a
 *              deffunction construct
 *  INPUTS       : The deffunction
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Busy count incremented
 *  NOTES        : None
 ***************************************************/
#if WIN_BTC
#pragma argsused
#endif
static void _dec_busy_count(void *env, void *value)
{
#if MAC_MCW || WIN_MCW || MAC_XCD
#pragma unused(env)
#endif

    ((FUNCTION_DEFINITION *)value)->busy++;
}

/*****************************************************
 *  NAME         : AllocateModule
 *  DESCRIPTION  : Creates and initializes a
 *              list of FUNCTIONS_GROUP_NAME for a new module
 *  INPUTS       : None
 *  RETURNS      : The new deffunction module
 *  SIDE EFFECTS : Deffunction module created
 *  NOTES        : None
 *****************************************************/
static void *_allocate_module(void *env)
{
    return((void *)core_mem_get_struct(env, function_module));
}

/***************************************************
 *  NAME         : ReturnModule
 *  DESCRIPTION  : Removes a deffunction module and
 *              all associated FUNCTIONS_GROUP_NAME
 *  INPUTS       : The deffunction module
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Module and FUNCTIONS_GROUP_NAME deleted
 *  NOTES        : None
 ***************************************************/
static void _get_module(void *env, void *theItem)
{
    core_free_construct_header_module(env, (struct module_header *)theItem, get_function_data(env)->function_construct);
    core_mem_return_struct(env, function_module, theItem);
}

/***************************************************
 *  NAME         : ClearDeffunctionsReady
 *  DESCRIPTION  : Determines if it is safe to
 *              remove all FUNCTIONS_GROUP_NAME
 *              Assumes *all* constructs will be
 *              deleted - only checks to see if
 *              any FUNCTIONS_GROUP_NAME are currently
 *              executing
 *  INPUTS       : None
 *  RETURNS      : TRUE if no FUNCTIONS_GROUP_NAME are
 *              executing, FALSE otherwise
 *  SIDE EFFECTS : None
 *  NOTES        : Used by (clear) and (bload)
 ***************************************************/
static BOOLEAN _clear_functions_ready(void *env)
{
    return((get_function_data(env)->executing_function != NULL) ? FALSE : TRUE);
}

/***************************************************
 *  NAME         : RemoveAllDeffunctions
 *  DESCRIPTION  : Removes all FUNCTIONS_GROUP_NAME
 *  INPUTS       : None
 *  RETURNS      : TRUE if all FUNCTIONS_GROUP_NAME
 *              removed, FALSE otherwise
 *  SIDE EFFECTS : Deffunctions removed
 *  NOTES        : None
 ***************************************************/
static BOOLEAN _remove_all_functions(void *env)
{
    FUNCTION_DEFINITION *dptr, *dtmp;
    unsigned oldbusy;
    BOOLEAN success = TRUE;


    dptr = (FUNCTION_DEFINITION *)get_next_function(env, NULL);

    while( dptr != NULL )
    {
        if( dptr->executing > 0 )
        {
            _error_function_definition(env, get_function_name(env, (void *)dptr));
            success = FALSE;
        }
        else
        {
            oldbusy = dptr->busy;
            core_decrement_expression(env, dptr->code);
            dptr->busy = oldbusy;
            core_return_packed_expression(env, dptr->code);
            dptr->code = NULL;
        }

        dptr = (FUNCTION_DEFINITION *)get_next_function(env, (void *)dptr);
    }

    dptr = (FUNCTION_DEFINITION *)get_next_function(env, NULL);

    while( dptr != NULL )
    {
        dtmp = dptr;
        dptr = (FUNCTION_DEFINITION *)get_next_function(env, (void *)dptr);

        if( dtmp->executing == 0 )
        {
            if( dtmp->busy > 0 )
            {
                warning_print_id(env, "DFFNXFUN", 1, FALSE);
                print_router(env, WWARNING, "Deffunction ");
                print_router(env, WWARNING, get_function_name(env, (void *)dtmp));
                print_router(env, WWARNING, " only partially deleted due to usage by other constructs.\n");
                set_function_pp((void *)dtmp, NULL);
                success = FALSE;
            }
            else
            {
                undefine_module_construct(env, (struct construct_metadata *)dtmp);
                remove_function(env, dtmp);
            }
        }
    }

    return(success);
}

/****************************************************
 *  NAME         : DeffunctionDeleteError
 *  DESCRIPTION  : Prints out an error message when
 *              a deffunction deletion attempt fails
 *  INPUTS       : The deffunction name
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Error message printed
 *  NOTES        : None
 ****************************************************/
static void _error_function_definition(void *env, char *dfnxName)
{
    error_deletion(env, FUNC_NAME_CREATE_FUNC, dfnxName);
}

/***************************************************
 *  NAME         : SaveDeffunctionHeaders
 *  DESCRIPTION  : Writes out deffunction forward
 *              declarations for (save) command
 *  INPUTS       : The logical output name
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Writes out FUNCTIONS_GROUP_NAME with no
 *              body of actions
 *  NOTES        : Used for FUNCTIONS_GROUP_NAME which are
 *              mutually recursive with other
 *              constructs
 ***************************************************/
static void _save_header(void *env, void *theModule, char *logicalName)
{
    core_do_for_all_constructs_in_module(env, theModule, _save_function_header,
                                         get_function_data(env)->module_index,
                                         FALSE, (void *)logicalName);
}

/***************************************************
 *  NAME         : SaveDeffunctionHeader
 *  DESCRIPTION  : Writes a deffunction forward
 *              declaration to the save file
 *  INPUTS       : 1) The deffunction
 *              2) The logical name of the output
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Defffunction header written
 *  NOTES        : None
 ***************************************************/
static void _save_function_header(void *env, struct construct_metadata *theDeffunction, void *userBuffer)
{
    FUNCTION_DEFINITION *dfnxPtr = (FUNCTION_DEFINITION *)theDeffunction;
    char *logicalName = (char *)userBuffer;
    register int i;

    if( get_function_pp(env, (void *)dfnxPtr) != NULL )
    {
        print_router(env, logicalName, "(deffunction ");
        print_router(env, logicalName, get_function_metadata(env, (void *)dfnxPtr));
        print_router(env, logicalName, "::");
        print_router(env, logicalName, get_function_name(env, (void *)dfnxPtr));
        print_router(env, logicalName, " (");

        for( i = 0 ; i < dfnxPtr->min_args ; i++ )
        {
            print_router(env, logicalName, "?p");
            core_print_long(env, logicalName, (long long)i);

            if( i != dfnxPtr->min_args - 1 )
            {
                print_router(env, logicalName, " ");
            }
        }

        if( dfnxPtr->max_args == -1 )
        {
            if( dfnxPtr->min_args != 0 )
            {
                print_router(env, logicalName, " ");
            }

            print_router(env, logicalName, "$?wildargs))\n\n");
        }
        else
        {
            print_router(env, logicalName, "))\n\n");
        }
    }
}

/***************************************************
 *  NAME         : SaveDeffunctions
 *  DESCRIPTION  : Writes out FUNCTIONS_GROUP_NAME
 *              for (save) command
 *  INPUTS       : The logical output name
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Writes out FUNCTIONS_GROUP_NAME
 *  NOTES        : None
 ***************************************************/
static void _save_functions(void *env, void *theModule, char *logicalName)
{
    core_save_construct(env, theModule, logicalName, get_function_data(env)->function_construct);
}

#endif
