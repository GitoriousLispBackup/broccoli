/* Purpose:  Kernel Interface Commands for Instances         */

/* =========================================
 *****************************************
 *            EXTERNAL DEFINITIONS
 *  =========================================
 ***************************************** */
#include "setup.h"

#if OBJECT_SYSTEM

#include "core_arguments.h"
#include "classes_kernel.h"
#include "funcs_class.h"
#include "classes_info.h"
#include "core_environment.h"
#include "parser_expressions.h"
#include "core_evaluation.h"
#include "classes_instances_file.h"
#include "funcs_instance.h"
#include "classes_instances_init.h"
#include "classes_instances_modify.h"
#include "classes_instances_lists.h"
#include "parser_class_instances.h"
#include "core_memory.h"
#include "classes_methods_kernel.h"
#include "funcs_method.h"
#include "router.h"
#include "router_string.h"
#include "sysdep.h"
#include "core_gc.h"
#include "core_command_prompt.h"

#define __CLASSES_INSTANCES_KERNEL_SOURCE__
#include "classes_instances_kernel.h"

/* =========================================
 *****************************************
 *                CONSTANTS
 *  =========================================
 ***************************************** */
#define ALL_QUALIFIER      "inherit"

/* =========================================
 *****************************************
 *   INTERNALLY VISIBLE FUNCTION HEADERS
 *  =========================================
 ***************************************** */

#if DEBUGGING_FUNCTIONS
static long ListInstancesInModule(void *, int, char *, char *, BOOLEAN, BOOLEAN);
static long TabulateInstances(void *, int, char *, DEFCLASS *, BOOLEAN, BOOLEAN);
#endif

static void            PrintInstance(void *, char *, INSTANCE_TYPE *, char *);
static INSTANCE_SLOT * FindISlotByName(void *, INSTANCE_TYPE *, char *);
static void            DeallocateInstanceData(void *);

/* =========================================
 *****************************************
 *       EXTERNALLY VISIBLE FUNCTIONS
 *  =========================================
 ***************************************** */

/*********************************************************
 *  NAME         : SetupInstances
 *  DESCRIPTION  : Initializes instance Hash Table,
 *                Function Parsers, and Data Structures
 *  INPUTS       : None
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : None
 *  NOTES        : None
 *********************************************************/
globle void SetupInstances(void *theEnv)
{
    struct ext_class_info instanceInfo = {{"INSTANCE_ADDRESS",
                                                INSTANCE_ADDRESS, 0, 0, 0,
                                                PrintInstanceName,
                                                PrintInstanceLongForm,
                                                EnvUnmakeInstance,
                                                NULL,
                                                EnvGetNextInstance,
                                                EnvDecrementInstanceCount,
                                                EnvIncrementInstanceCount,
                                                NULL, NULL, NULL, NULL, NULL},
                                               NULL, NULL, NULL, NULL};

    INSTANCE_TYPE dummyInstance = {{NULL, NULL, 0, 0L},
                                   NULL, NULL, 0, 1, 0, 0, 0,
                                   NULL,  0, 0, 0, NULL, NULL, NULL, NULL,
                                   NULL, NULL, NULL, NULL, NULL};

    core_allocate_environment_data(theEnv, INSTANCE_DATA, sizeof(struct instanceData), DeallocateInstanceData);

    InstanceData(theEnv)->MkInsMsgPass = TRUE;
    memcpy(&InstanceData(theEnv)->InstanceInfo, &instanceInfo, sizeof(struct ext_class_info));
    dummyInstance.header.theInfo = &InstanceData(theEnv)->InstanceInfo;
    memcpy(&InstanceData(theEnv)->DummyInstance, &dummyInstance, sizeof(INSTANCE_TYPE));

    InitializeInstanceTable(theEnv);
    core_install_primitive(theEnv, (struct core_data_entity *)&InstanceData(theEnv)->InstanceInfo, INSTANCE_ADDRESS);


    core_define_function(theEnv, "initialize-instance", 'u',
                       PTR_FN InitializeInstanceCommand, "InitializeInstanceCommand", NULL);
    core_define_function(theEnv, "make-instance", 'u', PTR_FN MakeInstanceCommand, "MakeInstanceCommand", NULL);
    core_add_function_parser(theEnv, "initialize-instance", ParseInitializeInstance);
    core_add_function_parser(theEnv, "make-instance", ParseInitializeInstance);

    core_define_function(theEnv, "init-slots", 'u', PTR_FN InitSlotsCommand, "InitSlotsCommand", "00");

    core_define_function(theEnv, "delete-instance", 'b', PTR_FN DeleteInstanceCommand,
                       "DeleteInstanceCommand", "00");
    core_define_function(theEnv, "(create-instance)", 'b', PTR_FN CreateInstanceHandler,
                       "CreateInstanceHandler", "00");
    core_define_function(theEnv, "unmake-instance", 'b', PTR_FN UnmakeInstanceCommand,
                       "UnmakeInstanceCommand", "1*e");

#if DEBUGGING_FUNCTIONS
    core_define_function(theEnv, "instances", 'v', PTR_FN InstancesCommand, "InstancesCommand", "*3w");
    core_define_function(theEnv, "ppinstance", 'v', PTR_FN PPInstanceCommand, "PPInstanceCommand", "00");
#endif

    core_define_function(theEnv, "symbol-to-instance-name", 'u',
                       PTR_FN SymbolToInstanceName, "SymbolToInstanceName", "11w");
    core_define_function(theEnv, "instance-name-to-symbol", 'w',
                       PTR_FN InstanceNameToSymbol, "InstanceNameToSymbol", "11p");
    core_define_function(theEnv, "instance-address", 'u', PTR_FN InstanceAddressCommand,
                       "InstanceAddressCommand", "12eep");
    core_define_function(theEnv, "instance-addressp", 'b', PTR_FN InstanceAddressPCommand,
                       "InstanceAddressPCommand", "11");
    core_define_function(theEnv, "instance-namep", 'b', PTR_FN InstanceNamePCommand,
                       "InstanceNamePCommand", "11");
    core_define_function(theEnv, "instance-name", 'u', PTR_FN InstanceNameCommand,
                       "InstanceNameCommand", "11e");
    core_define_function(theEnv, "instancep", 'b', PTR_FN InstancePCommand, "InstancePCommand", "11");
    core_define_function(theEnv, "instance-existp", 'b', PTR_FN InstanceExistPCommand,
                       "InstanceExistPCommand", "11e");
    core_define_function(theEnv, "class", 'u', PTR_FN ClassCommand, "ClassCommand", "11");

    SetupInstanceModDupCommands(theEnv);
    /* SetupInstanceFileCommands(theEnv); DR0866 */
    SetupInstanceListCommands(theEnv);


    SetupInstanceFileCommands(theEnv); /* DR0866 */

    core_gc_add_cleanup_function(theEnv, "instances", CleanupInstances, 0);
    core_add_reset_listener(theEnv, "instances", DestroyAllInstances, 60);
}

/**************************************
 * DeallocateInstanceData: Deallocates
 *    environment data for instances.
 ***************************************/
static void DeallocateInstanceData(void *theEnv)
{
    INSTANCE_TYPE *tmpIPtr, *nextIPtr;
    long i;
    INSTANCE_SLOT *sp;
    IGARBAGE *tmpGPtr, *nextGPtr;
    struct patternMatch *theMatch, *tmpMatch;

    /*=================================
     * Remove the instance hash table.
     *=================================*/

    core_mem_release(theEnv, InstanceData(theEnv)->InstanceTable,
       (int)(sizeof(INSTANCE_TYPE *) * INSTANCE_TABLE_HASH_SIZE));

    /*=======================
     * Return all instances.
     *=======================*/

    tmpIPtr = InstanceData(theEnv)->InstanceList;

    while( tmpIPtr != NULL )
    {
        nextIPtr = tmpIPtr->nxtList;

        for( i = 0 ; i < tmpIPtr->cls->instanceSlotCount ; i++ )
        {
            sp = tmpIPtr->slotAddresses[i];

            if((sp == &sp->desc->sharedValue) ?
               (--sp->desc->sharedCount == 0) : TRUE )
            {
                if( sp->desc->multiple )
                {
                    release_list(theEnv, (LIST_PTR)sp->value);
                }
            }
        }

        if( tmpIPtr->cls->instanceSlotCount != 0 )
        {
            core_mem_release(theEnv, (void *)tmpIPtr->slotAddresses,
               (tmpIPtr->cls->instanceSlotCount * sizeof(INSTANCE_SLOT *)));

            if( tmpIPtr->cls->localInstanceSlotCount != 0 )
            {
                core_mem_release(theEnv, (void *)tmpIPtr->slots,
                   (tmpIPtr->cls->localInstanceSlotCount * sizeof(INSTANCE_SLOT)));
            }
        }

        core_mem_return_struct(theEnv, instance, tmpIPtr);

        tmpIPtr = nextIPtr;
    }

    /*===============================
     * Get rid of garbage instances.
     *===============================*/

    tmpGPtr = InstanceData(theEnv)->InstanceGarbageList;

    while( tmpGPtr != NULL )
    {
        nextGPtr = tmpGPtr->nxt;
        core_mem_return_struct(theEnv, instance, tmpGPtr->ins);
        core_mem_return_struct(theEnv, igarbage, tmpGPtr);
        tmpGPtr = nextGPtr;
    }
}

/*******************************************************************
 *  NAME         : EnvDeleteInstance
 *  DESCRIPTION  : DIRECTLY removes a named instance from the
 *                hash table and its class's
 *                instance list
 *  INPUTS       : The instance address (NULL to delete all instances)
 *  RETURNS      : 1 if successful, 0 otherwise
 *  SIDE EFFECTS : Instance is deallocated
 *  NOTES        : C interface for deleting instances
 *******************************************************************/
globle BOOLEAN EnvDeleteInstance(void *theEnv, void *iptr)
{
    INSTANCE_TYPE *ins, *itmp;
    int success = 1;

    if( iptr != NULL )
    {
        return(QuashInstance(theEnv, (INSTANCE_TYPE *)iptr));
    }

    ins = InstanceData(theEnv)->InstanceList;

    while( ins != NULL )
    {
        itmp = ins;
        ins = ins->nxtList;

        if( QuashInstance(theEnv, (INSTANCE_TYPE *)itmp) == 0 )
        {
            success = 0;
        }
    }

    if((core_get_evaluation_data(theEnv)->eval_depth == 0) && (!CommandLineData(theEnv)->EvaluatingTopLevelCommand) &&
       (core_get_evaluation_data(theEnv)->current_expression == NULL))
    {
        core_gc_periodic_cleanup(theEnv, TRUE, FALSE);
    }

    return(success);
}

/*******************************************************************
 *  NAME         : EnvUnmakeInstance
 *  DESCRIPTION  : Removes a named instance via message-passing
 *  INPUTS       : The instance address (NULL to delete all instances)
 *  RETURNS      : 1 if successful, 0 otherwise
 *  SIDE EFFECTS : Instance is deallocated
 *  NOTES        : C interface for deleting instances
 *******************************************************************/
globle BOOLEAN EnvUnmakeInstance(void *theEnv, void *iptr)
{
    INSTANCE_TYPE *ins;
    int success = 1, svmaintain;

    svmaintain = InstanceData(theEnv)->MaintainGarbageInstances;
    InstanceData(theEnv)->MaintainGarbageInstances = TRUE;
    ins = (INSTANCE_TYPE *)iptr;

    if( ins != NULL )
    {
        if( ins->garbage )
        {
            success = 0;
        }
        else
        {
            DirectMessage(theEnv, MessageHandlerData(theEnv)->DELETE_ATOM, ins, NULL, NULL);

            if( ins->garbage == 0 )
            {
                success = 0;
            }
        }
    }
    else
    {
        ins = InstanceData(theEnv)->InstanceList;

        while( ins != NULL )
        {
            DirectMessage(theEnv, MessageHandlerData(theEnv)->DELETE_ATOM, ins, NULL, NULL);

            if( ins->garbage == 0 )
            {
                success = 0;
            }

            ins = ins->nxtList;

            while((ins != NULL) ? ins->garbage : FALSE )
            {
                ins = ins->nxtList;
            }
        }
    }

    InstanceData(theEnv)->MaintainGarbageInstances = svmaintain;
    CleanupInstances(theEnv);

    if((core_get_evaluation_data(theEnv)->eval_depth == 0) && (!CommandLineData(theEnv)->EvaluatingTopLevelCommand) &&
       (core_get_evaluation_data(theEnv)->current_expression == NULL))
    {
        core_gc_periodic_cleanup(theEnv, TRUE, FALSE);
    }

    return(success);
}

#if DEBUGGING_FUNCTIONS

/*******************************************************************
 *  NAME         : InstancesCommand
 *  DESCRIPTION  : Lists all instances associated
 *                with a particular class
 *  INPUTS       : None
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : None
 *  NOTES        : H/L Syntax : (instances [<class-name> [inherit]])
 *******************************************************************/
globle void InstancesCommand(void *theEnv)
{
    int argno, inheritFlag = FALSE;
    void *theDefmodule;
    char *className = NULL;
    core_data_object temp;

    theDefmodule = (void *)get_current_module(theEnv);

    argno = core_get_arg_count(theEnv);

    if( argno > 0 )
    {
        if( core_check_arg_type(theEnv, "instances", 1, ATOM, &temp) == FALSE )
        {
            return;
        }

        theDefmodule = lookup_module(theEnv, core_convert_data_to_string(temp));

        if((theDefmodule != NULL) ? FALSE :
           (strcmp(core_convert_data_to_string(temp), "*") != 0))
        {
            core_set_eval_error(theEnv, TRUE);
            report_explicit_type_error(theEnv, "instances", 1, "defmodule name");
            return;
        }

        if( argno > 1 )
        {
            if( core_check_arg_type(theEnv, "instances", 2, ATOM, &temp) == FALSE )
            {
                return;
            }

            className = core_convert_data_to_string(temp);

            if( LookupDefclassAnywhere(theEnv, (struct module_definition *)theDefmodule, className) == NULL )
            {
                if( strcmp(className, "*") == 0 )
                {
                    className = NULL;
                }
                else
                {
                    ClassExistError(theEnv, "instances", className);
                    return;
                }
            }

            if( argno > 2 )
            {
                if( core_check_arg_type(theEnv, "instances", 3, ATOM, &temp) == FALSE )
                {
                    return;
                }

                if( strcmp(core_convert_data_to_string(temp), ALL_QUALIFIER) != 0 )
                {
                    core_set_eval_error(theEnv, TRUE);
                    report_explicit_type_error(theEnv, "instances", 3, "keyword \"inherit\"");
                    return;
                }

                inheritFlag = TRUE;
            }
        }
    }

    EnvInstances(theEnv, WDISPLAY, theDefmodule, className, inheritFlag);
}

/********************************************************
 *  NAME         : PPInstanceCommand
 *  DESCRIPTION  : Displays the current slot-values
 *                of an instance
 *  INPUTS       : None
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : None
 *  NOTES        : H/L Syntax : (ppinstance <instance>)
 ********************************************************/
globle void PPInstanceCommand(void *theEnv)
{
    INSTANCE_TYPE *ins;

    if( CheckCurrentMessage(theEnv, "ppinstance", TRUE) == FALSE )
    {
        return;
    }

    ins = GetActiveInstance(theEnv);

    if( ins->garbage == 1 )
    {
        return;
    }

    PrintInstance(theEnv, WDISPLAY, ins, "\n");
    print_router(theEnv, WDISPLAY, "\n");
}

/***************************************************************
 * NAME         : EnvInstances
 * DESCRIPTION  : Lists instances of classes
 * INPUTS       : 1) The logical name for the output
 *             2) Address of the module (NULL for all classes)
 *             3) Name of the class
 *                (NULL for all classes in specified module)
 *             4) A flag indicating whether to print instances
 *                of subclasses or not
 * RETURNS      : Nothing useful
 * SIDE EFFECTS : None
 * NOTES        : None
 **************************************************************/
globle void EnvInstances(void *theEnv, char *logical_name, void *theVModule, char *className, int inheritFlag)
{
    int id;
    struct module_definition *module_def;
    long count = 0L;

    /* ===========================================
     *  Grab a traversal id to avoid printing out
     *  instances twice due to multiple inheritance
     *  =========================================== */
    if((id = GetTraversalID(theEnv)) == -1 )
    {
        return;
    }

    save_current_module(theEnv);

    /* ====================================
     *  For all modules, print out instances
     *  of specified class(es)
     *  ==================================== */
    if( theVModule == NULL )
    {
        module_def = (struct module_definition *)get_next_module(theEnv, NULL);

        while( module_def != NULL )
        {
            if( core_get_halt_eval(theEnv) == TRUE )
            {
                restore_current_module(theEnv);
                ReleaseTraversalID(theEnv);
                return;
            }

            print_router(theEnv, logical_name, get_module_name(theEnv, (void *)module_def));
            print_router(theEnv, logical_name, ":\n");
            set_current_module(theEnv, (void *)module_def);
            count += ListInstancesInModule(theEnv, id, logical_name, className, inheritFlag, TRUE);
            module_def = (struct module_definition *)get_next_module(theEnv, (void *)module_def);
        }
    }

    /* ====================================
     *  For the specified module, print out
     *  instances of the specified class(es)
     *  ==================================== */
    else
    {
        set_current_module(theEnv, (void *)theVModule);
        count = ListInstancesInModule(theEnv, id, logical_name, className, inheritFlag, FALSE);
    }

    restore_current_module(theEnv);
    ReleaseTraversalID(theEnv);

    if( core_get_evaluation_data(theEnv)->halt == FALSE )
    {
        core_print_tally(theEnv, logical_name, count, "instance", "instances");
    }
}

#endif

/*********************************************************
 *  NAME         : EnvMakeInstance
 *  DESCRIPTION  : C Interface for creating and
 *                initializing a class instance
 *  INPUTS       : The make-instance call string,
 *                 e.g. "([bill] of man (age 34))"
 *  RETURNS      : The instance address if instance created,
 *                 NULL otherwise
 *  SIDE EFFECTS : Creates the instance and returns
 *                 the result in caller's buffer
 *  NOTES        : None
 *********************************************************/
globle void *EnvMakeInstance(void *theEnv, char *mkstr)
{
    char *router = "***MKINS***";
    struct token tkn;
    core_expression_object *top;
    core_data_object result;

    result.type = ATOM;
    result.value = get_false(theEnv);

    if( open_string_source(theEnv, router, mkstr, 0) == 0 )
    {
        return(NULL);
    }

    core_get_token(theEnv, router, &tkn);

    if( tkn.type == LPAREN )
    {
        top = core_generate_constant(theEnv, FCALL, (void *)core_lookup_function(theEnv, "make-instance"));

        if( ParseSimpleInstance(theEnv, top, router) != NULL )
        {
            core_get_token(theEnv, router, &tkn);

            if( tkn.type == STOP )
            {
                core_increment_expression(theEnv, top);
                core_eval_expression(theEnv, top, &result);
                core_decrement_expression(theEnv, top);
            }
            else
            {
                error_syntax(theEnv, "instance definition");
            }

            core_return_expression(theEnv, top);
        }
    }
    else
    {
        error_syntax(theEnv, "instance definition");
    }

    close_string_source(theEnv, router);

    if((core_get_evaluation_data(theEnv)->eval_depth == 0) && (!CommandLineData(theEnv)->EvaluatingTopLevelCommand) &&
       (core_get_evaluation_data(theEnv)->current_expression == NULL))
    {
        core_gc_periodic_cleanup(theEnv, TRUE, FALSE);
    }

    if((result.type == ATOM) && (result.value == get_false(theEnv)))
    {
        return(NULL);
    }

    return((void *)FindInstanceBySymbol(theEnv, (ATOM_HN *)result.value));
}

/***************************************************************
 *  NAME         : EnvCreateRawInstance
 *  DESCRIPTION  : Creates an empty of instance of the specified
 *                class.  No slot-overrides or class defaults
 *                are applied.
 *  INPUTS       : 1) Address of class
 *              2) Name of the new instance
 *  RETURNS      : The instance address if instance created,
 *                 NULL otherwise
 *  SIDE EFFECTS : Old instance of same name deleted (if possible)
 *  NOTES        : None
 ***************************************************************/
globle void *EnvCreateRawInstance(void *theEnv, void *cptr, char *iname)
{
    return((void *)BuildInstance(theEnv, (ATOM_HN *)store_atom(theEnv, iname), (DEFCLASS *)cptr, FALSE));
}

/***************************************************************************
 *  NAME         : EnvFindInstance
 *  DESCRIPTION  : Looks up a specified instance in the instance hash table
 *  INPUTS       : Name-string of the instance
 *  RETURNS      : The address of the found instance, NULL otherwise
 *  SIDE EFFECTS : None
 *  NOTES        : None
 ***************************************************************************/
globle void *EnvFindInstance(void *theEnv, void *module_def, char *iname, unsigned searchImports)
{
    ATOM_HN *isym;

    isym = lookup_atom(theEnv, iname);

    if( isym == NULL )
    {
        return(NULL);
    }

    if( module_def == NULL )
    {
        module_def = (void *)get_current_module(theEnv);
    }

    return((void *)FindInstanceInModule(theEnv, isym, (struct module_definition *)module_def,
                                        ((struct module_definition *)get_current_module(theEnv)), searchImports));
}

/***************************************************************************
 *  NAME         : EnvValidInstanceAddress
 *  DESCRIPTION  : Determines if an instance address is still valid
 *  INPUTS       : Instance address
 *  RETURNS      : 1 if the address is still valid, 0 otherwise
 *  SIDE EFFECTS : None
 *  NOTES        : None
 ***************************************************************************/
#if WIN_BTC
#pragma argsused
#endif
globle int EnvValidInstanceAddress(void *theEnv, void *iptr)
{
#if MAC_MCW || WIN_MCW || MAC_XCD
#pragma unused(theEnv)
#endif

    return((((INSTANCE_TYPE *)iptr)->garbage == 0) ? 1 : 0);
}

/***************************************************
 *  NAME         : EnvDirectGetSlot
 *  DESCRIPTION  : Gets a slot value
 *  INPUTS       : 1) Instance address
 *              2) Slot name
 *              3) Caller's result buffer
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : None
 *  NOTES        : None
 ***************************************************/
globle void EnvDirectGetSlot(void *theEnv, void *ins, char *sname, core_data_object *result)
{
    INSTANCE_SLOT *sp;

    if(((INSTANCE_TYPE *)ins)->garbage == 1 )
    {
        core_set_eval_error(theEnv, TRUE);
        result->type = ATOM;
        result->value = get_false(theEnv);
        return;
    }

    sp = FindISlotByName(theEnv, (INSTANCE_TYPE *)ins, sname);

    if( sp == NULL )
    {
        core_set_eval_error(theEnv, TRUE);
        result->type = ATOM;
        result->value = get_false(theEnv);
        return;
    }

    result->type = (unsigned short)sp->type;
    result->value = sp->value;

    if( sp->type == LIST )
    {
        result->begin = 0;
        core_set_data_ptr_end(result, GetInstanceSlotLength(sp));
    }

    core_pass_return_value(theEnv, result);
}

/*********************************************************
 *  NAME         : EnvDirectPutSlot
 *  DESCRIPTION  : Gets a slot value
 *  INPUTS       : 1) Instance address
 *              2) Slot name
 *              3) Caller's new value buffer
 *  RETURNS      : TRUE if put successful, FALSE otherwise
 *  SIDE EFFECTS : None
 *  NOTES        : None
 *********************************************************/
globle int EnvDirectPutSlot(void *theEnv, void *ins, char *sname, core_data_object *val)
{
    INSTANCE_SLOT *sp;
    core_data_object junk;

    if((((INSTANCE_TYPE *)ins)->garbage == 1) || (val == NULL))
    {
        core_set_eval_error(theEnv, TRUE);
        return(FALSE);
    }

    sp = FindISlotByName(theEnv, (INSTANCE_TYPE *)ins, sname);

    if( sp == NULL )
    {
        core_set_eval_error(theEnv, TRUE);
        return(FALSE);
    }

    if( PutSlotValue(theEnv, (INSTANCE_TYPE *)ins, sp, val, &junk, "external put"))
    {
        if((core_get_evaluation_data(theEnv)->eval_depth == 0) && (!CommandLineData(theEnv)->EvaluatingTopLevelCommand) &&
           (core_get_evaluation_data(theEnv)->current_expression == NULL))
        {
            core_gc_periodic_cleanup(theEnv, TRUE, FALSE);
        }

        return(TRUE);
    }

    return(FALSE);
}

/***************************************************
 *  NAME         : GetInstanceName
 *  DESCRIPTION  : Returns name of instance
 *  INPUTS       : Pointer to instance
 *  RETURNS      : Name of instance
 *  SIDE EFFECTS : None
 *  NOTES        : None
 ***************************************************/
#if WIN_BTC
#pragma argsused
#endif
globle char *EnvGetInstanceName(void *theEnv, void *iptr)
{
#if MAC_MCW || WIN_MCW || MAC_XCD
#pragma unused(theEnv)
#endif

    if(((INSTANCE_TYPE *)iptr)->garbage == 1 )
    {
        return(NULL);
    }

    return(to_string(((INSTANCE_TYPE *)iptr)->name));
}

/***************************************************
 *  NAME         : EnvGetInstanceClass
 *  DESCRIPTION  : Returns class of instance
 *  INPUTS       : Pointer to instance
 *  RETURNS      : Pointer to class of instance
 *  SIDE EFFECTS : None
 *  NOTES        : None
 ***************************************************/
#if WIN_BTC
#pragma argsused
#endif
globle void *EnvGetInstanceClass(void *theEnv, void *iptr)
{
#if MAC_MCW || WIN_MCW || MAC_XCD
#pragma unused(theEnv)
#endif

    if(((INSTANCE_TYPE *)iptr)->garbage == 1 )
    {
        return(NULL);
    }

    return((void *)((INSTANCE_TYPE *)iptr)->cls);
}

/***************************************************
 *  NAME         : GetGlobalNumberOfInstances
 *  DESCRIPTION  : Returns the total number of
 *                instances in all modules
 *  INPUTS       : None
 *  RETURNS      : The instance count
 *  SIDE EFFECTS : None
 *  NOTES        : None
 ***************************************************/
globle unsigned long GetGlobalNumberOfInstances(void *theEnv)
{
    return(InstanceData(theEnv)->GlobalNumberOfInstances);
}

/***************************************************
 *  NAME         : EnvGetNextInstance
 *  DESCRIPTION  : Returns next instance in list
 *              (or first instance in list)
 *  INPUTS       : Pointer to previous instance
 *              (or NULL to get first instance)
 *  RETURNS      : The next instance or first instance
 *  SIDE EFFECTS : None
 *  NOTES        : None
 ***************************************************/
globle void *EnvGetNextInstance(void *theEnv, void *iptr)
{
    if( iptr == NULL )
    {
        return((void *)InstanceData(theEnv)->InstanceList);
    }

    if(((INSTANCE_TYPE *)iptr)->garbage == 1 )
    {
        return(NULL);
    }

    return((void *)((INSTANCE_TYPE *)iptr)->nxtList);
}

/***************************************************
 *  NAME         : GetNextInstanceInScope
 *  DESCRIPTION  : Returns next instance in list
 *              (or first instance in list)
 *              which class is in scope
 *  INPUTS       : Pointer to previous instance
 *              (or NULL to get first instance)
 *  RETURNS      : The next instance or first instance
 *              which class is in scope of the
 *              current module
 *  SIDE EFFECTS : None
 *  NOTES        : None
 ***************************************************/
globle void *GetNextInstanceInScope(void *theEnv, void *iptr)
{
    INSTANCE_TYPE *ins = (INSTANCE_TYPE *)iptr;

    if( ins == NULL )
    {
        ins = InstanceData(theEnv)->InstanceList;
    }
    else if( ins->garbage )
    {
        return(NULL);
    }
    else
    {
        ins = ins->nxtList;
    }

    while( ins != NULL )
    {
        if( DefclassInScope(theEnv, ins->cls, NULL))
        {
            return((void *)ins);
        }

        ins = ins->nxtList;
    }

    return(NULL);
}

/***************************************************
 *  NAME         : EnvGetNextInstanceInClass
 *  DESCRIPTION  : Finds next instance of class
 *              (or first instance of class)
 *  INPUTS       : 1) Class address
 *              2) Instance address
 *                 (NULL to get first instance)
 *  RETURNS      : The next or first class instance
 *  SIDE EFFECTS : None
 *  NOTES        : None
 ***************************************************/
#if WIN_BTC
#pragma argsused
#endif
globle void *EnvGetNextInstanceInClass(void *theEnv, void *cptr, void *iptr)
{
#if MAC_MCW || WIN_MCW || MAC_XCD
#pragma unused(theEnv)
#endif

    if( iptr == NULL )
    {
        return((void *)((DEFCLASS *)cptr)->instanceList);
    }

    if(((INSTANCE_TYPE *)iptr)->garbage == 1 )
    {
        return(NULL);
    }

    return((void *)((INSTANCE_TYPE *)iptr)->nxtClass);
}

/***************************************************
 *  NAME         : EnvGetNextInstanceInClassAndSubclasses
 *  DESCRIPTION  : Finds next instance of class
 *              (or first instance of class) and
 *              all of its subclasses
 *  INPUTS       : 1) Class address
 *              2) Instance address
 *                 (NULL to get first instance)
 *  RETURNS      : The next or first class instance
 *  SIDE EFFECTS : None
 *  NOTES        : None
 ***************************************************/
globle void *EnvGetNextInstanceInClassAndSubclasses(void *theEnv, void **cptr, void *iptr, core_data_object *iterationInfo)
{
    INSTANCE_TYPE *nextInstance;
    DEFCLASS *theClass;

    theClass = (DEFCLASS *)*cptr;

    if( iptr == NULL )
    {
        ClassSubclassAddresses(theEnv, theClass, iterationInfo, TRUE);
        nextInstance = theClass->instanceList;
    }
    else if(((INSTANCE_TYPE *)iptr)->garbage == 1 )
    {
        nextInstance = NULL;
    }
    else
    {
        nextInstance = ((INSTANCE_TYPE *)iptr)->nxtClass;
    }

    while((nextInstance == NULL) &&
          (core_get_data_ptr_start(iterationInfo) <= core_get_data_ptr_end(iterationInfo)))
    {
        theClass = (struct defclass *)get_list_node_value(core_convert_data_ptr_to_pointer(iterationInfo),
                                                       core_get_data_ptr_start(iterationInfo));
        *cptr = theClass;
        core_set_data_ptr_start(iterationInfo, core_get_data_ptr_start(iterationInfo) + 1);
        nextInstance = theClass->instanceList;
    }

    return(nextInstance);
}

/***************************************************
 *  NAME         : EnvGetInstancePPForm
 *  DESCRIPTION  : Writes slot names and values to
 *               caller's buffer
 *  INPUTS       : 1) Caller's buffer
 *              2) Size of buffer (not including
 *                 space for terminating '\0')
 *              3) Instance address
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Caller's buffer written
 *  NOTES        : None
 ***************************************************/
globle void EnvGetInstancePPForm(void *theEnv, char *buf, unsigned buflen, void *iptr)
{
    char *pbuf = "***InstancePPForm***";

    if(((INSTANCE_TYPE *)iptr)->garbage == 1 )
    {
        return;
    }

    if( open_string_dest(theEnv, pbuf, buf, buflen + 1) == 0 )
    {
        return;
    }

    PrintInstance(theEnv, pbuf, (INSTANCE_TYPE *)iptr, " ");
    close_string_dest(theEnv, pbuf);
}

/*********************************************************
 *  NAME         : ClassCommand
 *  DESCRIPTION  : Returns the class of an instance
 *  INPUTS       : Caller's result buffer
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : None
 *  NOTES        : H/L Syntax : (class <object>)
 *              Can also be called by (type <object>)
 *                if you have generic functions installed
 *********************************************************/
globle void ClassCommand(void *theEnv, core_data_object *result)
{
    INSTANCE_TYPE *ins;
    char *func;
    core_data_object temp;

    func = to_string(((struct core_function_definition *)
                          core_get_evaluation_data(theEnv)->current_expression->value)->function_handle);
    result->type = ATOM;
    result->value = get_false(theEnv);
    core_eval_expression(theEnv, core_get_first_arg(), &temp);

    if( temp.type == INSTANCE_ADDRESS )
    {
        ins = (INSTANCE_TYPE *)temp.value;

        if( ins->garbage == 1 )
        {
            StaleInstanceAddress(theEnv, func, 0);
            core_set_eval_error(theEnv, TRUE);
            return;
        }

        result->value = (void *)GetDefclassNamePointer((void *)ins->cls);
    }
    else if( temp.type == INSTANCE_NAME )
    {
        ins = FindInstanceBySymbol(theEnv, (ATOM_HN *)temp.value);

        if( ins == NULL )
        {
            NoInstanceError(theEnv, to_string(temp.value), func);
            return;
        }

        result->value = (void *)GetDefclassNamePointer((void *)ins->cls);
    }
    else
    {
        switch( temp.type )
        {
        case INTEGER:
        case FLOAT:
        case ATOM:
        case STRING:
        case LIST:
        case EXTERNAL_ADDRESS:
            result->value = (void *)
                            GetDefclassNamePointer((void *)
                                                   DefclassData(theEnv)->PrimitiveClassMap[temp.type]);
            return;

        default:
            error_print_id(theEnv, "INSCOM", 1, FALSE);
            print_router(theEnv, WERROR, "Undefined type in function ");
            print_router(theEnv, WERROR, func);
            print_router(theEnv, WERROR, ".\n");
            core_set_eval_error(theEnv, TRUE);
        }
    }
}

/******************************************************
 *  NAME         : CreateInstanceHandler
 *  DESCRIPTION  : Message handler called after instance creation
 *  INPUTS       : None
 *  RETURNS      : TRUE if successful,
 *              FALSE otherwise
 *  SIDE EFFECTS : None
 *  NOTES        : Does nothing. Provided so it can be overridden.
 ******************************************************/
#if WIN_BTC
#pragma argsused
#endif
globle BOOLEAN CreateInstanceHandler(void *theEnv)
{
#if MAC_MCW || WIN_MCW || MAC_XCD
#pragma unused(theEnv)
#endif

    return(TRUE);
}

/******************************************************
 *  NAME         : DeleteInstanceCommand
 *  DESCRIPTION  : Removes a named instance from the
 *                hash table and its class's
 *                instance list
 *  INPUTS       : None
 *  RETURNS      : TRUE if successful,
 *              FALSE otherwise
 *  SIDE EFFECTS : Instance is deallocated
 *  NOTES        : This is an internal function that
 *                only be called by a handler
 ******************************************************/
globle BOOLEAN DeleteInstanceCommand(void *theEnv)
{
    if( CheckCurrentMessage(theEnv, "delete-instance", TRUE))
    {
        return(QuashInstance(theEnv, GetActiveInstance(theEnv)));
    }

    return(FALSE);
}

/********************************************************************
 *  NAME         : UnmakeInstanceCommand
 *  DESCRIPTION  : Uses message-passing to delete the
 *                specified instance
 *  INPUTS       : None
 *  RETURNS      : TRUE if successful, FALSE otherwise
 *  SIDE EFFECTS : Instance is deallocated
 *  NOTES        : Syntax: (unmake-instance <instance-expression>+ | *)
 ********************************************************************/
globle BOOLEAN UnmakeInstanceCommand(void *theEnv)
{
    core_expression_object *theArgument;
    core_data_object theResult;
    INSTANCE_TYPE *ins;
    int argNumber = 1, rtn = TRUE;

    theArgument = core_get_first_arg();

    while( theArgument != NULL )
    {
        core_eval_expression(theEnv, theArgument, &theResult);

        if((theResult.type == INSTANCE_NAME) || (theResult.type == ATOM))
        {
            ins = FindInstanceBySymbol(theEnv, (ATOM_HN *)theResult.value);

            if((ins == NULL) ? (strcmp(core_convert_data_to_string(theResult), "*") != 0) : FALSE )
            {
                NoInstanceError(theEnv, core_convert_data_to_string(theResult), "unmake-instance");
                return(FALSE);
            }
        }
        else if( theResult.type == INSTANCE_ADDRESS )
        {
            ins = (INSTANCE_TYPE *)theResult.value;

            if( ins->garbage )
            {
                StaleInstanceAddress(theEnv, "unmake-instance", 0);
                core_set_eval_error(theEnv, TRUE);
                return(FALSE);
            }
        }
        else
        {
            report_explicit_type_error(theEnv, "unmake-instance", argNumber, "instance-address, instance-name, or the symbol *");
            core_set_eval_error(theEnv, TRUE);
            return(FALSE);
        }

        if( EnvUnmakeInstance(theEnv, ins) == FALSE )
        {
            rtn = FALSE;
        }

        if( ins == NULL )
        {
            return(rtn);
        }

        argNumber++;
        theArgument = core_get_next_arg(theArgument);
    }

    return(rtn);
}

/*****************************************************************
 *  NAME         : SymbolToInstanceName
 *  DESCRIPTION  : Converts a symbol from type ATOM
 *                to type INSTANCE_NAME
 *  INPUTS       : The address of the value buffer
 *  RETURNS      : The new INSTANCE_NAME symbol
 *  SIDE EFFECTS : None
 *  NOTES        : H/L Syntax : (symbol-to-instance-name <symbol>)
 *****************************************************************/
globle void SymbolToInstanceName(void *theEnv, core_data_object *result)
{
    if( core_check_arg_type(theEnv, "symbol-to-instance-name", 1, ATOM, result) == FALSE )
    {
        core_set_pointer_type(result, ATOM);
        core_set_pointer_value(result, get_false(theEnv));
        return;
    }

    core_set_pointer_type(result, INSTANCE_NAME);
}

/*****************************************************************
 *  NAME         : InstanceNameToSymbol
 *  DESCRIPTION  : Converts a symbol from type INSTANCE_NAME
 *                to type ATOM
 *  INPUTS       : None
 *  RETURNS      : Symbol FALSE on errors - or converted instance name
 *  SIDE EFFECTS : None
 *  NOTES        : H/L Syntax : (instance-name-to-symbol <iname>)
 *****************************************************************/
globle void *InstanceNameToSymbol(void *theEnv)
{
    core_data_object result;

    if( core_check_arg_type(theEnv, "instance-name-to-symbol", 1, INSTANCE_NAME, &result) == FALSE )
    {
        return((ATOM_HN *)get_false(theEnv));
    }

    return((ATOM_HN *)result.value);
}

/*********************************************************************************
 *  NAME         : InstanceAddressCommand
 *  DESCRIPTION  : Returns the address of an instance
 *  INPUTS       : The address of the value buffer
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Stores instance address in caller's buffer
 *  NOTES        : H/L Syntax : (instance-address [<module-name>] <instance-name>)
 *********************************************************************************/
globle void InstanceAddressCommand(void *theEnv, core_data_object *result)
{
    INSTANCE_TYPE *ins;
    core_data_object temp;
    struct module_definition *module_def;
    unsigned searchImports;

    result->type = ATOM;
    result->value = get_false(theEnv);

    if( core_get_arg_count(theEnv) > 1 )
    {
        if( core_check_arg_type(theEnv, "instance-address", 1, ATOM, &temp) == FALSE )
        {
            return;
        }

        module_def = (struct module_definition *)lookup_module(theEnv, core_convert_data_to_string(temp));

        if((module_def == NULL) ? (strcmp(core_convert_data_to_string(temp), "*") != 0) : FALSE )
        {
            report_explicit_type_error(theEnv, "instance-address", 1, "module name");
            core_set_eval_error(theEnv, TRUE);
            return;
        }

        if( module_def == NULL )
        {
            searchImports = TRUE;
            module_def = ((struct module_definition *)get_current_module(theEnv));
        }
        else
        {
            searchImports = FALSE;
        }

        if( core_check_arg_type(theEnv, "instance-address", 2, INSTANCE_NAME, &temp)
            == FALSE )
        {
            return;
        }

        ins = FindInstanceInModule(theEnv, (ATOM_HN *)temp.value, module_def,
                                   ((struct module_definition *)get_current_module(theEnv)), searchImports);

        if( ins != NULL )
        {
            result->type = INSTANCE_ADDRESS;
            result->value = (void *)ins;
        }
        else
        {
            NoInstanceError(theEnv, to_string(temp.value), "instance-address");
        }
    }
    else if( core_check_arg_type(theEnv, "instance-address", 1, INSTANCE_OR_INSTANCE_NAME, &temp))
    {
        if( temp.type == INSTANCE_ADDRESS )
        {
            ins = (INSTANCE_TYPE *)temp.value;

            if( ins->garbage == 0 )
            {
                result->type = INSTANCE_ADDRESS;
                result->value = temp.value;
            }
            else
            {
                StaleInstanceAddress(theEnv, "instance-address", 0);
                core_set_eval_error(theEnv, TRUE);
            }
        }
        else
        {
            ins = FindInstanceBySymbol(theEnv, (ATOM_HN *)temp.value);

            if( ins != NULL )
            {
                result->type = INSTANCE_ADDRESS;
                result->value = (void *)ins;
            }
            else
            {
                NoInstanceError(theEnv, to_string(temp.value), "instance-address");
            }
        }
    }
}

/***************************************************************
 *  NAME         : InstanceNameCommand
 *  DESCRIPTION  : Gets the name of an INSTANCE
 *  INPUTS       : The address of the value buffer
 *  RETURNS      : The INSTANCE_NAME symbol
 *  SIDE EFFECTS : None
 *  NOTES        : H/L Syntax : (instance-name <instance>)
 ***************************************************************/
globle void InstanceNameCommand(void *theEnv, core_data_object *result)
{
    INSTANCE_TYPE *ins;
    core_data_object temp;

    result->type = ATOM;
    result->value = get_false(theEnv);

    if( core_check_arg_type(theEnv, "instance-name", 1, INSTANCE_OR_INSTANCE_NAME, &temp) == FALSE )
    {
        return;
    }

    if( temp.type == INSTANCE_ADDRESS )
    {
        ins = (INSTANCE_TYPE *)temp.value;

        if( ins->garbage == 1 )
        {
            StaleInstanceAddress(theEnv, "instance-name", 0);
            core_set_eval_error(theEnv, TRUE);
            return;
        }
    }
    else
    {
        ins = FindInstanceBySymbol(theEnv, (ATOM_HN *)temp.value);

        if( ins == NULL )
        {
            NoInstanceError(theEnv, to_string(temp.value), "instance-name");
            return;
        }
    }

    result->type = INSTANCE_NAME;
    result->value = (void *)ins->name;
}

/**************************************************************
 *  NAME         : InstanceAddressPCommand
 *  DESCRIPTION  : Determines if a value is of type INSTANCE
 *  INPUTS       : None
 *  RETURNS      : TRUE if type INSTANCE_ADDRESS, FALSE otherwise
 *  SIDE EFFECTS : None
 *  NOTES        : H/L Syntax : (instance-addressp <arg>)
 **************************************************************/
globle BOOLEAN InstanceAddressPCommand(void *theEnv)
{
    core_data_object temp;

    core_eval_expression(theEnv, core_get_first_arg(), &temp);
    return((core_get_type(temp) == INSTANCE_ADDRESS) ? TRUE : FALSE);
}

/**************************************************************
 *  NAME         : InstanceNamePCommand
 *  DESCRIPTION  : Determines if a value is of type INSTANCE_NAME
 *  INPUTS       : None
 *  RETURNS      : TRUE if type INSTANCE_NAME, FALSE otherwise
 *  SIDE EFFECTS : None
 *  NOTES        : H/L Syntax : (instance-namep <arg>)
 **************************************************************/
globle BOOLEAN InstanceNamePCommand(void *theEnv)
{
    core_data_object temp;

    core_eval_expression(theEnv, core_get_first_arg(), &temp);
    return((core_get_type(temp) == INSTANCE_NAME) ? TRUE : FALSE);
}

/*****************************************************************
 *  NAME         : InstancePCommand
 *  DESCRIPTION  : Determines if a value is of type INSTANCE_ADDRESS
 *                or INSTANCE_NAME
 *  INPUTS       : None
 *  RETURNS      : TRUE if type INSTANCE_NAME or INSTANCE_ADDRESS,
 *                  FALSE otherwise
 *  SIDE EFFECTS : None
 *  NOTES        : H/L Syntax : (instancep <arg>)
 *****************************************************************/
globle BOOLEAN InstancePCommand(void *theEnv)
{
    core_data_object temp;

    core_eval_expression(theEnv, core_get_first_arg(), &temp);

    if((core_get_type(temp) == INSTANCE_NAME) || (core_get_type(temp) == INSTANCE_ADDRESS))
    {
        return(TRUE);
    }

    return(FALSE);
}

/********************************************************
 *  NAME         : InstanceExistPCommand
 *  DESCRIPTION  : Determines if an instance exists
 *  INPUTS       : None
 *  RETURNS      : TRUE if instance exists, FALSE otherwise
 *  SIDE EFFECTS : None
 *  NOTES        : H/L Syntax : (instance-existp <arg>)
 ********************************************************/
globle BOOLEAN InstanceExistPCommand(void *theEnv)
{
    core_data_object temp;

    core_eval_expression(theEnv, core_get_first_arg(), &temp);

    if( temp.type == INSTANCE_ADDRESS )
    {
        return((((INSTANCE_TYPE *)temp.value)->garbage == 0) ? TRUE : FALSE);
    }

    if((temp.type == INSTANCE_NAME) || (temp.type == ATOM))
    {
        return((FindInstanceBySymbol(theEnv, (ATOM_HN *)temp.value) != NULL) ?
               TRUE : FALSE);
    }

    report_explicit_type_error(theEnv, "instance-existp", 1, "instance name, instance address or symbol");
    core_set_eval_error(theEnv, TRUE);
    return(FALSE);
}

/* =========================================
 *****************************************
 *       INTERNALLY VISIBLE FUNCTIONS
 *  =========================================
 ***************************************** */

#if DEBUGGING_FUNCTIONS

/***************************************************
 *  NAME         : ListInstancesInModule
 *  DESCRIPTION  : List instances of specified
 *              class(es) in a module
 *  INPUTS       : 1) Traversal id to avoid multiple
 *                 passes over same class
 *              2) Logical name of output
 *              3) The name of the class
 *                 (NULL for all classes)
 *              4) Flag indicating whether to
 *                 include instances of subclasses
 *              5) A flag indicating whether to
 *                 indent because of module name
 *  RETURNS      : The number of instances listed
 *  SIDE EFFECTS : Instances listed to logical output
 *  NOTES        : Assumes defclass scope flags
 *              are up to date
 ***************************************************/
static long ListInstancesInModule(void *theEnv, int id, char *logical_name, char *className, BOOLEAN inheritFlag, BOOLEAN allModulesFlag)
{
    void *theDefclass, *theInstance;
    long count = 0L;

    /* ===================================
     *  For the specified module, print out
     *  instances of all the classes
     *  =================================== */
    if( className == NULL )
    {
        /* ==============================================
         *  If instances are being listed for all modules,
         *  only list the instances of classes in this
         *  module (to avoid listing instances twice)
         *  ============================================== */
        if( allModulesFlag )
        {
            for( theDefclass = EnvGetNextDefclass(theEnv, NULL) ;
                 theDefclass != NULL ;
                 theDefclass = EnvGetNextDefclass(theEnv, theDefclass))
            {
                count += TabulateInstances(theEnv, id, logical_name,
                                           (DEFCLASS *)theDefclass, FALSE, allModulesFlag);
            }
        }

        /* ===================================================
         *  If instances are only be listed for one module,
         *  list all instances visible to the module (including
         *  ones belonging to classes in other modules)
         *  =================================================== */
        else
        {
            theInstance = GetNextInstanceInScope(theEnv, NULL);

            while( theInstance != NULL )
            {
                if( core_get_halt_eval(theEnv) == TRUE )
                {
                    return(count);
                }

                count++;
                PrintInstanceNameAndClass(theEnv, logical_name, (INSTANCE_TYPE *)theInstance, TRUE);
                theInstance = GetNextInstanceInScope(theEnv, theInstance);
            }
        }
    }

    /* ===================================
     *  For the specified module, print out
     *  instances of the specified class
     *  =================================== */
    else
    {
        theDefclass = (void *)LookupDefclassAnywhere(theEnv, ((struct module_definition *)get_current_module(theEnv)), className);

        if( theDefclass != NULL )
        {
            count += TabulateInstances(theEnv, id, logical_name,
                                       (DEFCLASS *)theDefclass, inheritFlag, allModulesFlag);
        }
        else if( !allModulesFlag )
        {
            ClassExistError(theEnv, "instances", className);
        }
    }

    return(count);
}

/******************************************************
 *  NAME         : TabulateInstances
 *  DESCRIPTION  : Displays all instances for a class
 *  INPUTS       : 1) The traversal id for the classes
 *              2) The logical name of the output
 *              3) The class address
 *              4) A flag indicating whether to
 *                 print out instances of subclasses
 *                 or not.
 *              5) A flag indicating whether to
 *                 indent because of module name
 *  RETURNS      : The number of instances (including
 *                 subclasses' instances)
 *  SIDE EFFECTS : None
 *  NOTES        : None
 ******************************************************/
static long TabulateInstances(void *theEnv, int id, char *logical_name, DEFCLASS *cls, BOOLEAN inheritFlag, BOOLEAN allModulesFlag)
{
    INSTANCE_TYPE *ins;
    long i;
    long count = 0;

    if( TestTraversalID(cls->traversalRecord, id))
    {
        return(0L);
    }

    SetTraversalID(cls->traversalRecord, id);

    for( ins = cls->instanceList ; ins != NULL ; ins = ins->nxtClass )
    {
        if( core_get_evaluation_data(theEnv)->halt )
        {
            return(count);
        }

        if( allModulesFlag )
        {
            print_router(theEnv, logical_name, "   ");
        }

        PrintInstanceNameAndClass(theEnv, logical_name, ins, TRUE);
        count++;
    }

    if( inheritFlag )
    {
        for( i = 0 ; i < cls->directSubclasses.classCount ; i++ )
        {
            if( core_get_evaluation_data(theEnv)->halt )
            {
                return(count);
            }

            count += TabulateInstances(theEnv, id, logical_name,
                                       cls->directSubclasses.classArray[i], inheritFlag, allModulesFlag);
        }
    }

    return(count);
}

#endif

/***************************************************
 *  NAME         : PrintInstance
 *  DESCRIPTION  : Displays an instance's slots
 *  INPUTS       : 1) Logical name for output
 *              2) Instance address
 *              3) String used to separate
 *                 slot printouts
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : None
 *  NOTES        : Assumes instance is valid
 ***************************************************/
static void PrintInstance(void *theEnv, char *logical_name, INSTANCE_TYPE *ins, char *separator)
{
    long i;
    register INSTANCE_SLOT *sp;

    PrintInstanceNameAndClass(theEnv, logical_name, ins, FALSE);

    for( i = 0 ; i < ins->cls->instanceSlotCount ; i++ )
    {
        print_router(theEnv, logical_name, separator);
        sp = ins->slotAddresses[i];
        print_router(theEnv, logical_name, "(");
        print_router(theEnv, logical_name, to_string(sp->desc->slotName->name));

        if( sp->type != LIST )
        {
            print_router(theEnv, logical_name, " ");
            core_print_atom(theEnv, logical_name, (int)sp->type, sp->value);
        }
        else if( GetInstanceSlotLength(sp) != 0 )
        {
            print_router(theEnv, logical_name, " ");
            print_list(theEnv, logical_name, (LIST_PTR)sp->value, 0,
                      (long)(GetInstanceSlotLength(sp) - 1), FALSE);
        }

        print_router(theEnv, logical_name, ")");
    }
}

/***************************************************
 *  NAME         : FindISlotByName
 *  DESCRIPTION  : Looks up an instance slot by
 *                instance name and slot name
 *  INPUTS       : 1) Instance address
 *              2) Instance name-string
 *  RETURNS      : The instance slot address, NULL if
 *                does not exist
 *  SIDE EFFECTS : None
 *  NOTES        : None
 ***************************************************/
static INSTANCE_SLOT *FindISlotByName(void *theEnv, INSTANCE_TYPE *ins, char *sname)
{
    ATOM_HN *ssym;

    ssym = lookup_atom(theEnv, sname);

    if( ssym == NULL )
    {
        return(NULL);
    }

    return(FindInstanceSlot(theEnv, ins, ssym));
}

#endif
