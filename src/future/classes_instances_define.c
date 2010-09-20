/* Purpose: Kernel definstances interface commands
 *              and routines                                 */

/* =========================================
 *****************************************
 *            EXTERNAL DEFINITIONS
 *  =========================================
 ***************************************** */
#include "setup.h"

#if DEFINSTANCES_CONSTRUCT

#include "core_arguments.h"
#include "classes_kernel.h"
#include "funcs_class.h"
#include "core_constructs_query.h"
#include "parser_constructs.h"
#include "constant.h"
#include "core_constructs.h"
#include "core_environment.h"
#include "core_evaluation.h"
#include "core_functions.h"
#include "funcs_instance.h"
#include "parser_class_instances.h"
#include "core_memory.h"
#include "parser_modules.h"
#include "router.h"
#include "core_scanner.h"
#include "type_symbol.h"
#include "core_gc.h"

#define __CLASSES_INSTANCES_DEFINE_SOURCE__
#include "classes_instances_define.h"

/* =========================================
 *****************************************
 *                CONSTANTS
 *  =========================================
 ***************************************** */
#define ACTIVE_RLN "active"

/* =========================================
 *****************************************
 *   INTERNALLY VISIBLE FUNCTION HEADERS
 *  =========================================
 ***************************************** */

static int       ParseDefinstances(void *, char *);
static ATOM_HN * ParseDefinstancesName(void *, char *, int *);
static void      RemoveDefinstances(void *, void *);
static void      SaveDefinstances(void *, void *, char *);
static BOOLEAN   RemoveAllDefinstances(void *);
static void      DefinstancesDeleteError(void *, char *);


static void *  AllocateModule(void *);
static void    ReturnModule(void *, void *);
static BOOLEAN ClearDefinstancesReady(void *);
static void    CheckDefinstancesBusy(void *, struct construct_metadata *, void *);
static void    DestroyDefinstancesAction(void *, struct construct_metadata *, void *);

static void ResetDefinstances(void *);
static void ResetDefinstancesAction(void *, struct construct_metadata *, void *);
static void DeallocateDefinstancesData(void *);

/* =========================================
 *****************************************
 *       EXTERNALLY VISIBLE FUNCTIONS
 *  =========================================
 ***************************************** */

/***************************************************
 *  NAME         : SetupDefinstances
 *  DESCRIPTION  : Adds the definstance support routines
 *                to the Kernel
 *  INPUTS       : None
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Appropriate function lists modified
 *  NOTES        : None
 ***************************************************/
globle void SetupDefinstances(void *theEnv)
{
    core_allocate_environment_data(theEnv, DEFINSTANCES_DATA, sizeof(struct definstancesData), DeallocateDefinstancesData);

    DefinstancesData(theEnv)->DefinstancesModuleIndex =
        install_module_item(theEnv, "definstances",
                           AllocateModule, ReturnModule,
                           NULL,
                           NULL,
                           EnvFindDefinstances);

    DefinstancesData(theEnv)->DefinstancesConstruct =
        core_define_construct(theEnv, "definstances", "definstances",
                     ParseDefinstances,
                     EnvFindDefinstances,
                     core_get_atom_pointer, core_get_construct_pp,
                     core_query_module_header, EnvGetNextDefinstances, core_set_next_construct,
                     EnvIsDefinstancesDeletable, EnvUndefinstances,
                     RemoveDefinstances
                     );

    core_add_clear_listener(theEnv, "definstances", ClearDefinstancesReady, 0);

    core_define_function(theEnv, "undefinstances", 'v', PTR_FN UndefinstancesCommand, "UndefinstancesCommand", "11w");
    core_add_saver(theEnv, "definstances", SaveDefinstances, 0);



#if DEBUGGING_FUNCTIONS
    core_define_function(theEnv, "ppdefinstances", 'v', PTR_FN PPDefinstancesCommand, "PPDefinstancesCommand", "11w");
    core_define_function(theEnv, "list-definstances", 'v', PTR_FN ListDefinstancesCommand, "ListDefinstancesCommand", "01");
#endif

    core_define_function(theEnv, "get-definstances-list", 'm', PTR_FN GetDefinstancesListFunction,
                       "GetDefinstancesListFunction", "01");
    core_define_function(theEnv, "definstances-module", 'w', PTR_FN GetDefinstancesModuleCommand,
                       "GetDefinstancesModuleCommand", "11w");

    core_add_reset_listener(theEnv, "definstances", (void(*) (void *))ResetDefinstances, 0);
}

/******************************************************
 * DeallocateDefinstancesData: Deallocates environment
 *    data for the definstances construct.
 *******************************************************/
static void DeallocateDefinstancesData(void *theEnv)
{
    struct definstancesModule *theModuleItem;
    void *module_def;


    core_do_for_all_constructs(theEnv, DestroyDefinstancesAction, DefinstancesData(theEnv)->DefinstancesModuleIndex, FALSE, NULL);

    for( module_def = get_next_module(theEnv, NULL);
         module_def != NULL;
         module_def = get_next_module(theEnv, module_def))
    {
        theModuleItem = (struct definstancesModule *)
                        get_module_item(theEnv, (struct module_definition *)module_def,
                                      DefinstancesData(theEnv)->DefinstancesModuleIndex);
        core_mem_return_struct(theEnv, definstancesModule, theModuleItem);
    }
}

/****************************************************
 * DestroyDefinstancesAction: Action used to remove
 *   definstances as a result of core_delete_environment.
 *****************************************************/
#if WIN_BTC
#pragma argsused
#endif
static void DestroyDefinstancesAction(void *theEnv, struct construct_metadata *theConstruct, void *buffer)
{
#if MAC_MCW || WIN_MCW || MAC_XCD
#pragma unused(buffer)
#endif
    struct definstances *theDefinstances = (struct definstances *)theConstruct;

    if( theDefinstances == NULL )
    {
        return;
    }

    core_return_packed_expression(theEnv, theDefinstances->mkinstance);

    core_delete_construct_metadata(theEnv, &theDefinstances->header);

    core_mem_return_struct(theEnv, definstances, theDefinstances);
}

/***********************************************************
 *  NAME         : EnvGetNextDefinstances
 *  DESCRIPTION  : Finds first or next definstances
 *  INPUTS       : The address of the current definstances
 *  RETURNS      : The address of the next definstances
 *                (NULL if none)
 *  SIDE EFFECTS : None
 *  NOTES        : If ptr == NULL, the first definstances
 *                 is returned.
 ***********************************************************/
globle void *EnvGetNextDefinstances(void *theEnv, void *ptr)
{
    return((void *)core_get_next_construct_metadata(theEnv, (struct construct_metadata *)ptr,
                                        DefinstancesData(theEnv)->DefinstancesModuleIndex));
}

/***************************************************
 *  NAME         : EnvFindDefinstances
 *  DESCRIPTION  : Looks up a definstance construct
 *                by name-string
 *  INPUTS       : The symbolic name
 *  RETURNS      : The definstance address, or NULL
 *                 if not found
 *  SIDE EFFECTS : None
 *  NOTES        : None
 ***************************************************/
globle void *EnvFindDefinstances(void *theEnv, char *name)
{
    return(core_lookup_named_construct(theEnv, name, DefinstancesData(theEnv)->DefinstancesConstruct));
}

/***************************************************
 *  NAME         : EnvIsDefinstancesDeletable
 *  DESCRIPTION  : Determines if a definstances
 *                can be deleted
 *  INPUTS       : Address of the definstances
 *  RETURNS      : TRUE if deletable, FALSE otherwise
 *  SIDE EFFECTS : None
 *  NOTES        : None
 ***************************************************/
globle int EnvIsDefinstancesDeletable(void *theEnv, void *ptr)
{
    if( !core_are_constructs_deleteable(theEnv))
    {
        return(FALSE);
    }

    return((((DEFINSTANCES *)ptr)->busy == 0) ? TRUE : FALSE);
}

/***********************************************************
 *  NAME         : UndefinstancesCommand
 *  DESCRIPTION  : Removes a definstance
 *  INPUTS       : None
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Definstance deallocated
 *  NOTES        : H/L Syntax : (undefinstances <name> | *)
 ***********************************************************/
globle void UndefinstancesCommand(void *theEnv)
{
    core_uninstall_construct_command(theEnv, "undefinstances", DefinstancesData(theEnv)->DefinstancesConstruct);
}

/*****************************************************************
 *  NAME         : GetDefinstancesModuleCommand
 *  DESCRIPTION  : Determines to which module a definstances belongs
 *  INPUTS       : None
 *  RETURNS      : The symbolic name of the module
 *  SIDE EFFECTS : None
 *  NOTES        : H/L Syntax: (definstances-module <defins-name>)
 *****************************************************************/
globle void *GetDefinstancesModuleCommand(void *theEnv)
{
    return(core_query_module_name(theEnv, "definstances-module", DefinstancesData(theEnv)->DefinstancesConstruct));
}

/***********************************************************
 *  NAME         : EnvUndefinstances
 *  DESCRIPTION  : Removes a definstance
 *  INPUTS       : Address of definstances to remove
 *  RETURNS      : TRUE if successful,
 *              FALSE otherwise
 *  SIDE EFFECTS : Definstance deallocated
 *  NOTES        : None
 ***********************************************************/
globle BOOLEAN EnvUndefinstances(void *theEnv, void *vptr)
{
    DEFINSTANCES *dptr;

    dptr = (DEFINSTANCES *)vptr;


    if( dptr == NULL )
    {
        return(RemoveAllDefinstances(theEnv));
    }

    if( EnvIsDefinstancesDeletable(theEnv, vptr) == FALSE )
    {
        return(FALSE);
    }

    undefine_module_construct(theEnv, (struct construct_metadata *)vptr);
    RemoveDefinstances(theEnv, (void *)dptr);
    return(TRUE);
}

#if DEBUGGING_FUNCTIONS

/***************************************************************
 *  NAME         : PPDefinstancesCommand
 *  DESCRIPTION  : Prints out the pretty-print form of a definstance
 *  INPUTS       : None
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : None
 *  NOTES        : H/L Syntax : (ppdefinstances <name>)
 ***************************************************************/
globle void PPDefinstancesCommand(void *theEnv)
{
    core_construct_pp_command(theEnv, "ppdefinstances", DefinstancesData(theEnv)->DefinstancesConstruct);
}

/***************************************************
 *  NAME         : ListDefinstancesCommand
 *  DESCRIPTION  : Displays all definstances names
 *  INPUTS       : None
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Definstances name sprinted
 *  NOTES        : H/L Interface
 ***************************************************/
globle void ListDefinstancesCommand(void *theEnv)
{
    core_garner_module_constructs_command(theEnv, "list-definstances", DefinstancesData(theEnv)->DefinstancesConstruct);
}

/***************************************************
 *  NAME         : EnvListDefinstances
 *  DESCRIPTION  : Displays all definstances names
 *  INPUTS       : 1) The logical name of the output
 *              2) The module
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Definstances names printed
 *  NOTES        : C Interface
 ***************************************************/
globle void EnvListDefinstances(void *theEnv, char *logical_name, struct module_definition *module_def)
{
    core_get_module_constructs_list(theEnv, DefinstancesData(theEnv)->DefinstancesConstruct, logical_name, module_def);
}

#endif

/****************************************************************
 *  NAME         : GetDefinstancesListFunction
 *  DESCRIPTION  : Groups all definstances names into
 *              a list list
 *  INPUTS       : A data object buffer to hold
 *              the list result
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : List allocated and filled
 *  NOTES        : H/L Syntax: (get-definstances-list [<module>])
 ****************************************************************/
globle void GetDefinstancesListFunction(void *theEnv, core_data_object*returnValue)
{
    core_garner_construct_list(theEnv, "get-definstances-list", returnValue, DefinstancesData(theEnv)->DefinstancesConstruct);
}

/***************************************************************
 *  NAME         : EnvGetDefinstancesList
 *  DESCRIPTION  : Groups all definstances names into
 *              a list list
 *  INPUTS       : 1) A data object buffer to hold
 *                 the list result
 *              2) The module from which to obtain definstances
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : List allocated and filled
 *  NOTES        : External C access
 ***************************************************************/
globle void EnvGetDefinstancesList(void *theEnv, core_data_object *returnValue, struct module_definition *module_def)
{
    core_get_module_constructs_list(theEnv, returnValue, DefinstancesData(theEnv)->DefinstancesConstruct, module_def);
}

/* =========================================
 *****************************************
 *       INTERNALLY VISIBLE FUNCTIONS
 *  =========================================
 ***************************************** */


/*********************************************************************
 *  NAME         : ParseDefinstances
 *  DESCRIPTION  : Parses and allocates a definstances construct
 *  INPUTS       : The logical name of the input source
 *  RETURNS      : FALSE if no errors, TRUE otherwise
 *  SIDE EFFECTS : Definstances parsed and created
 *  NOTES        : H/L Syntax :
 *
 *              (definstances  <name> [active] [<comment>]
 *                 <instance-definition>+)
 *
 *              <instance-definition> ::=
 *                 (<instance-name> of <class-name> <slot-override>*)
 *
 *              <slot-override> ::= (<slot-name> <value-expression>*)
 *********************************************************************/
static int ParseDefinstances(void *theEnv, char *readSource)
{
    ATOM_HN *dname;
    void *mkinsfcall;
    core_expression_object *mkinstance, *mkbot = NULL;
    DEFINSTANCES *dobj;
    int active;

    core_set_pp_buffer_status(theEnv, ON);
    core_flush_pp_buffer(theEnv);
    core_pp_indent_depth(theEnv, 3);
    core_save_pp_buffer(theEnv, "(definstances ");

    dname = ParseDefinstancesName(theEnv, readSource, &active);

    if( dname == NULL )
    {
        return(TRUE);
    }

    dobj = core_mem_get_struct(theEnv, definstances);
    core_init_construct_header(theEnv, "definstances", (struct construct_metadata *)dobj, dname);
    dobj->busy = 0;
    dobj->mkinstance = NULL;
    mkinsfcall = (void *)core_lookup_function(theEnv, "make-instance");

    while( core_get_type(DefclassData(theEnv)->ObjectParseToken) == LPAREN )
    {
        mkinstance = core_generate_constant(theEnv, UNKNOWN_VALUE, mkinsfcall);
        mkinstance = ParseInitializeInstance(theEnv, mkinstance, readSource);

        if( mkinstance == NULL )
        {
            core_return_expression(theEnv, dobj->mkinstance);
            core_mem_return_struct(theEnv, definstances, dobj);
            return(TRUE);
        }

        if( core_does_expression_have_variables(mkinstance, FALSE) == TRUE )
        {
            error_local_variable(theEnv, "definstances");
            core_return_expression(theEnv, mkinstance);
            core_return_expression(theEnv, dobj->mkinstance);
            core_mem_return_struct(theEnv, definstances, dobj);
            return(TRUE);
        }

        if( mkbot == NULL )
        {
            dobj->mkinstance = mkinstance;
        }
        else
        {
            core_get_next_arg(mkbot) = mkinstance;
        }

        mkbot = mkinstance;
        core_get_token(theEnv, readSource, &DefclassData(theEnv)->ObjectParseToken);
        core_backup_pp_buffer(theEnv);
        core_inject_ws_into_pp_buffer(theEnv);
        core_save_pp_buffer(theEnv, DefclassData(theEnv)->ObjectParseToken.pp);
    }

    if( core_get_type(DefclassData(theEnv)->ObjectParseToken) != RPAREN )
    {
        core_return_expression(theEnv, dobj->mkinstance);
        core_mem_return_struct(theEnv, definstances, dobj);
        error_syntax(theEnv, "definstances");
        return(TRUE);
    }
    else
    {
        if( core_construct_data(theEnv)->check_syntax )
        {
            core_return_expression(theEnv, dobj->mkinstance);
            core_mem_return_struct(theEnv, definstances, dobj);
            return(FALSE);
        }

#if DEBUGGING_FUNCTIONS

        if( EnvGetConserveMemory(theEnv) == FALSE )
        {
            if( dobj->mkinstance != NULL )
            {
                core_backup_pp_buffer(theEnv);
            }

            core_backup_pp_buffer(theEnv);
            core_save_pp_buffer(theEnv, ")\n");
            SetDefinstancesPPForm((void *)dobj, core_copy_pp_buffer(theEnv));
        }

#endif
        mkinstance = dobj->mkinstance;
        dobj->mkinstance = core_pack_expression(theEnv, mkinstance);
        core_return_expression(theEnv, mkinstance);
        inc_atom_count(GetDefinstancesNamePointer((void *)dobj));
        core_increment_expression(theEnv, dobj->mkinstance);
    }

    core_add_to_module((struct construct_metadata *)dobj);
    return(FALSE);
}

/*************************************************************
 *  NAME         : ParseDefinstancesName
 *  DESCRIPTION  : Parses definstance name and optional comment
 *              and optional "active" keyword
 *  INPUTS       : 1) The logical name of the input source
 *              2) Buffer to hold flag indicating if
 *                 definstances should cause pattern-matching
 *                 to occur during slot-overrides
 *  RETURNS      : Address of name symbol, or
 *                NULL if there was an error
 *  SIDE EFFECTS : Token after name or comment is scanned
 *  NOTES        : Assumes "(definstances" has already
 *                been scanned.
 *************************************************************/
static ATOM_HN *ParseDefinstancesName(void *theEnv, char *readSource, int *active)
{
    ATOM_HN *dname;

    *active = FALSE;
    dname = parse_construct_head(theEnv, readSource, &DefclassData(theEnv)->ObjectParseToken, "definstances",
                                       EnvFindDefinstances, EnvUndefinstances, "@",
                                       TRUE, FALSE, TRUE);

    if( dname == NULL )
    {
        return(NULL);
    }


    if( core_get_type(DefclassData(theEnv)->ObjectParseToken) == STRING )
    {
        core_backup_pp_buffer(theEnv);
        core_backup_pp_buffer(theEnv);
        core_save_pp_buffer(theEnv, " ");
        core_save_pp_buffer(theEnv, DefclassData(theEnv)->ObjectParseToken.pp);
        core_inject_ws_into_pp_buffer(theEnv);
        core_get_token(theEnv, readSource, &DefclassData(theEnv)->ObjectParseToken);
    }

    return(dname);
}

/**************************************************************
 *  NAME         : RemoveDefinstances
 *  DESCRIPTION  : Deallocates and removes a definstance construct
 *  INPUTS       : The definstance address
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Existing definstance construct deleted
 *  NOTES        : Assumes busy count of definstance is 0
 **************************************************************/
static void RemoveDefinstances(void *theEnv, void *vdptr)
{
    DEFINSTANCES *dptr = (DEFINSTANCES *)vdptr;

    dec_atom_count(theEnv, GetDefinstancesNamePointer((void *)dptr));
    core_decrement_expression(theEnv, dptr->mkinstance);
    core_return_packed_expression(theEnv, dptr->mkinstance);
    SetDefinstancesPPForm((void *)dptr, NULL);
    ext_clear_data(theEnv, dptr->header.ext_data);
    core_mem_return_struct(theEnv, definstances, dptr);
}

/***************************************************
 *  NAME         : SaveDefinstances
 *  DESCRIPTION  : Prints pretty print form of
 *                definstances to specified output
 *  INPUTS       : The logical name of the output
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : None
 *  NOTES        : None
 ***************************************************/
static void SaveDefinstances(void *theEnv, void *module_def, char *logName)
{
    core_save_construct(theEnv, module_def, logName, DefinstancesData(theEnv)->DefinstancesConstruct);
}

/***************************************************
 *  NAME         : RemoveAllDefinstances
 *  DESCRIPTION  : Removes all definstances constructs
 *  INPUTS       : None
 *  RETURNS      : TRUE if successful,
 *              FALSE otherwise
 *  SIDE EFFECTS : All definstances deallocated
 *  NOTES        : None
 ***************************************************/
static BOOLEAN RemoveAllDefinstances(void *theEnv)
{
    DEFINSTANCES *dptr, *dhead;
    int success = TRUE;

    dhead = (DEFINSTANCES *)EnvGetNextDefinstances(theEnv, NULL);

    while( dhead != NULL )
    {
        dptr = dhead;
        dhead = (DEFINSTANCES *)EnvGetNextDefinstances(theEnv, (void *)dhead);

        if( EnvIsDefinstancesDeletable(theEnv, (void *)dptr))
        {
            undefine_module_construct(theEnv, (struct construct_metadata *)dptr);
            RemoveDefinstances(theEnv, (void *)dptr);
        }
        else
        {
            DefinstancesDeleteError(theEnv, EnvGetDefinstancesName(theEnv, (void *)dptr));
            success = FALSE;
        }
    }

    return(success);
}

/***************************************************
 *  NAME         : DefinstancesDeleteError
 *  DESCRIPTION  : Prints an error message for
 *              unsuccessful definstances
 *              deletion attempts
 *  INPUTS       : The name of the definstances
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Error message printed
 *  NOTES        : None
 ***************************************************/
static void DefinstancesDeleteError(void *theEnv, char *dname)
{
    error_deletion(theEnv, "definstances", dname);
}

/*****************************************************
 *  NAME         : AllocateModule
 *  DESCRIPTION  : Creates and initializes a
 *              list of definstances for a new module
 *  INPUTS       : None
 *  RETURNS      : The new definstances module
 *  SIDE EFFECTS : Definstances module created
 *  NOTES        : None
 *****************************************************/
static void *AllocateModule(void *theEnv)
{
    return((void *)core_mem_get_struct(theEnv, definstancesModule));
}

/***************************************************
 *  NAME         : ReturnModule
 *  DESCRIPTION  : Removes a definstances module and
 *              all associated definstances
 *  INPUTS       : The definstances module
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Module and definstances deleted
 *  NOTES        : None
 ***************************************************/
static void ReturnModule(void *theEnv, void *theItem)
{
    core_free_construct_header_module(theEnv, (struct module_header *)theItem, DefinstancesData(theEnv)->DefinstancesConstruct);
    core_mem_return_struct(theEnv, definstancesModule, theItem);
}

/***************************************************
 *  NAME         : ClearDefinstancesReady
 *  DESCRIPTION  : Determines if it is safe to
 *              remove all definstances
 *              Assumes *all* constructs will be
 *              deleted
 *  INPUTS       : None
 *  RETURNS      : TRUE if all definstances can
 *              be deleted, FALSE otherwise
 *  SIDE EFFECTS : None
 *  NOTES        : Used by (clear) and (bload)
 ***************************************************/
static BOOLEAN ClearDefinstancesReady(void *theEnv)
{
    int flagBuffer = TRUE;

    core_do_for_all_constructs(theEnv, CheckDefinstancesBusy, DefinstancesData(theEnv)->DefinstancesModuleIndex,
                       FALSE, (void *)&flagBuffer);
    return(flagBuffer);
}

/***************************************************
 *  NAME         : CheckDefinstancesBusy
 *  DESCRIPTION  : Determines if a definstances is
 *              in use or not
 *  INPUTS       : 1) The definstances
 *              2) A buffer to set to 0 if the
 *                 the definstances is busy
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Buffer set to 0 if definstances
 *              busy
 *  NOTES        : The flag buffer is not modified
 *              if definstances is not busy
 *              (assumed to be initialized to 1)
 ***************************************************/
#if WIN_BTC
#pragma argsused
#endif
static void CheckDefinstancesBusy(void *theEnv, struct construct_metadata *theDefinstances, void *userBuffer)
{
#if MAC_MCW || WIN_MCW || MAC_XCD
#pragma unused(theEnv)
#endif

    if(((DEFINSTANCES *)theDefinstances)->busy > 0 )
    {
        *(int *)userBuffer = FALSE;
    }
}

/***************************************************
 *  NAME         : ResetDefinstances
 *  DESCRIPTION  : Calls core_eval_expression for each of
 *                the make-instance calls in all
 *                of the definstances constructs
 *  INPUTS       : None
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : All instances in the definstances
 *                are evaluated (and created if
 *                there are no errors)
 *              Any previously existing instances
 *              are deleted first.
 *  NOTES        : None
 ***************************************************/
static void ResetDefinstances(void *theEnv)
{
    core_do_for_all_constructs(theEnv, ResetDefinstancesAction, DefinstancesData(theEnv)->DefinstancesModuleIndex, TRUE, NULL);
}

/***************************************************
 *  NAME         : ResetDefinstancesAction
 *  DESCRIPTION  : Performs all the make-instance
 *              calls in a definstances
 *  INPUTS       : 1) The definstances
 *              2) User data buffer (ignored)
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Instances created
 *  NOTES        : None
 ***************************************************/
#if WIN_BTC
#pragma argsused
#endif
static void ResetDefinstancesAction(void *theEnv, struct construct_metadata *vDefinstances, void *userBuffer)
{
#if MAC_MCW || WIN_MCW || MAC_XCD
#pragma unused(userBuffer)
#endif
    DEFINSTANCES *theDefinstances = (DEFINSTANCES *)vDefinstances;
    core_expression_object *theExp;
    core_data_object temp;

    save_current_module(theEnv);
    set_current_module(theEnv, (void *)vDefinstances->my_module->module_def);
    theDefinstances->busy++;

    for( theExp = theDefinstances->mkinstance ;
         theExp != NULL ;
         theExp = core_get_next_arg(theExp))
    {
        core_eval_expression(theEnv, theExp, &temp);

        if( core_get_evaluation_data(theEnv)->halt ||
            ((core_get_type(temp) == ATOM) &&
             (core_get_value(temp) == get_false(theEnv))))
        {
            restore_current_module(theEnv);
            theDefinstances->busy--;
            return;
        }
    }

    theDefinstances->busy--;
    restore_current_module(theEnv);
}

#endif
