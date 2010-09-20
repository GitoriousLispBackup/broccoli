/* =========================================
 *****************************************
 *            EXTERNAL DEFINITIONS
 *  =========================================
 ***************************************** */

#include <stdlib.h>

#include "setup.h"

#if OBJECT_SYSTEM

#include "core_arguments.h"
#include "classes_kernel.h"
#include "classfun.h"
#include "constraints_query.h"
#include "core_environment.h"
#include "classes_instances_kernel.h"
#include "classes_instances_init.h"
#include "core_memory.h"
#include "modules_query.h"
#include "classes_methods_kernel.h"
#include "funcs_method.h"
#include "core_functions_util.h"
#include "router.h"
#include "core_gc.h"

#define __FUNCS_INSTANCE_H__
#include "funcs_instance.h"

/* =========================================
 *****************************************
 *                CONSTANTS
 *  =========================================
 ***************************************** */
#define BIG_PRIME    11329

/* =========================================
 *****************************************
 *   INTERNALLY VISIBLE FUNCTION HEADERS
 *  =========================================
 ***************************************** */

static INSTANCE_TYPE *FindImportedInstance(void *, struct module_definition *, struct module_definition *, INSTANCE_TYPE *);


/* =========================================
 *****************************************
 *       EXTERNALLY VISIBLE FUNCTIONS
 *  =========================================
 ***************************************** */

/***************************************************
 *  NAME         : EnvIncrementInstanceCount
 *  DESCRIPTION  : Increments instance busy count -
 *                prevents it from being deleted
 *  INPUTS       : The address of the instance
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Count set
 *  NOTES        : None
 ***************************************************/
#if WIN_BTC
#pragma argsused
#endif
globle void EnvIncrementInstanceCount(void *theEnv, void *vptr)
{
#if MAC_MCW || WIN_MCW || MAC_XCD
#pragma unused(theEnv)
#endif

    ((INSTANCE_TYPE *)vptr)->busy++;
}

/***************************************************
 *  NAME         : EnvDecrementInstanceCount
 *  DESCRIPTION  : Decrements instance busy count -
 *                might allow it to be deleted
 *  INPUTS       : The address of the instance
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Count set
 *  NOTES        : None
 ***************************************************/
#if WIN_BTC
#pragma argsused
#endif
globle void EnvDecrementInstanceCount(void *theEnv, void *vptr)
{
#if MAC_MCW || WIN_MCW || MAC_XCD
#pragma unused(theEnv)
#endif

    ((INSTANCE_TYPE *)vptr)->busy--;
}

/***************************************************
 *  NAME         : InitializeInstanceTable
 *  DESCRIPTION  : Initializes instance hash table
 *               to all NULL addresses
 *  INPUTS       : None
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Hash table initialized
 *  NOTES        : None
 ***************************************************/
globle void InitializeInstanceTable(void *theEnv)
{
    register int i;

    InstanceData(theEnv)->InstanceTable = (INSTANCE_TYPE **)
                                          core_mem_alloc_no_init(theEnv, (int)(sizeof(INSTANCE_TYPE *) * INSTANCE_TABLE_HASH_SIZE));

    for( i = 0 ; i < INSTANCE_TABLE_HASH_SIZE ; i++ )
    {
        InstanceData(theEnv)->InstanceTable[i] = NULL;
    }
}

/*******************************************************
 *  NAME         : CleanupInstances
 *  DESCRIPTION  : Iterates through instance garbage
 *                list looking for nodes that
 *                have become unused - and purges
 *                them
 *  INPUTS       : None
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Non-busy instance garbage nodes deleted
 *  NOTES        : None
 *******************************************************/
globle void CleanupInstances(void *theEnv)
{
    IGARBAGE *gprv, *gtmp, *dump;

    if( InstanceData(theEnv)->MaintainGarbageInstances )
    {
        return;
    }

    gprv = NULL;
    gtmp = InstanceData(theEnv)->InstanceGarbageList;

    while( gtmp != NULL )
    {
        if((gtmp->ins->busy == 0) && (gtmp->ins->depth > core_get_evaluation_data(theEnv)->eval_depth)
           )
        {
            core_get_gc_data(theEnv)->generational_item_count -= 2;
            core_get_gc_data(theEnv)->generational_item_sz -= InstanceSizeHeuristic(gtmp->ins) + sizeof(IGARBAGE);
            dec_atom_count(theEnv, gtmp->ins->name);
            core_mem_return_struct(theEnv, instance, gtmp->ins);

            if( gprv == NULL )
            {
                InstanceData(theEnv)->InstanceGarbageList = gtmp->nxt;
            }
            else
            {
                gprv->nxt = gtmp->nxt;
            }

            dump = gtmp;
            gtmp = gtmp->nxt;
            core_mem_return_struct(theEnv, igarbage, dump);
        }
        else
        {
            gprv = gtmp;
            gtmp = gtmp->nxt;
        }
    }
}

/*******************************************************
 *  NAME         : HashInstance
 *  DESCRIPTION  : Generates a hash index for a given
 *              instance name
 *  INPUTS       : The address of the instance name ATOM_HN
 *  RETURNS      : The hash index value
 *  SIDE EFFECTS : None
 *  NOTES        : Counts on the fact that the symbol
 *              has already been hashed into the
 *              symbol table - uses that hash value
 *              multiplied by a prime for a new hash
 *******************************************************/
globle unsigned HashInstance(ATOM_HN *cname)
{
    unsigned long tally;

    tally = ((unsigned long)cname->bucket) * BIG_PRIME;
    return((unsigned)(tally % INSTANCE_TABLE_HASH_SIZE));
}

/***************************************************
 *  NAME         : DestroyAllInstances
 *  DESCRIPTION  : Deallocates all instances,
 *               reinitializes hash table and
 *               resets class instance pointers
 *  INPUTS       : None
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : All instances deallocated
 *  NOTES        : None
 ***************************************************/
globle void DestroyAllInstances(void *theEnv)
{
    INSTANCE_TYPE *iptr;
    int svmaintain;

    save_current_module(theEnv);
    svmaintain = InstanceData(theEnv)->MaintainGarbageInstances;
    InstanceData(theEnv)->MaintainGarbageInstances = TRUE;
    iptr = InstanceData(theEnv)->InstanceList;

    while( iptr != NULL )
    {
        set_current_module(theEnv, (void *)iptr->cls->header.my_module->module_def);
        DirectMessage(theEnv, MessageHandlerData(theEnv)->DELETE_ATOM, iptr, NULL, NULL);
        iptr = iptr->nxtList;

        while((iptr != NULL) ? iptr->garbage : FALSE )
        {
            iptr = iptr->nxtList;
        }
    }

    InstanceData(theEnv)->MaintainGarbageInstances = svmaintain;
    restore_current_module(theEnv);
}

/******************************************************
 *  NAME         : RemoveInstanceData
 *  DESCRIPTION  : Deallocates all the data objects
 *              in instance slots and then dealloactes
 *              the slots themeselves
 *  INPUTS       : The instance
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Instance slots removed
 *  NOTES        : An instance made with CopyInstanceData
 *              will have shared values removed
 *              in all cases because they are not
 *              "real" instances.
 *              Instance class busy count decremented
 ******************************************************/
globle void RemoveInstanceData(void *theEnv, INSTANCE_TYPE *ins)
{
    long i;
    INSTANCE_SLOT *sp;

    DecrementDefclassBusyCount(theEnv, (void *)ins->cls);

    for( i = 0 ; i < ins->cls->instanceSlotCount ; i++ )
    {
        sp = ins->slotAddresses[i];

        if((sp == &sp->desc->sharedValue) ?
           (--sp->desc->sharedCount == 0) : TRUE )
        {
            if( sp->desc->multiple )
            {
                uninstall_list(theEnv, (LIST_PTR)sp->value);
                track_list(theEnv, (LIST_PTR)sp->value);
            }
            else
            {
                core_decrement_atom(theEnv, (int)sp->type, sp->value);
            }

            sp->value = NULL;
        }
    }

    if( ins->cls->instanceSlotCount != 0 )
    {
        core_mem_release(theEnv, (void *)ins->slotAddresses,
           (ins->cls->instanceSlotCount * sizeof(INSTANCE_SLOT *)));

        if( ins->cls->localInstanceSlotCount != 0 )
        {
            core_mem_release(theEnv, (void *)ins->slots,
               (ins->cls->localInstanceSlotCount * sizeof(INSTANCE_SLOT)));
        }
    }

    ins->slots = NULL;
    ins->slotAddresses = NULL;
}

/***************************************************************************
 *  NAME         : FindInstanceBySymbol
 *  DESCRIPTION  : Looks up a specified instance in the instance hash table
 *  INPUTS       : The symbol for the name of the instance
 *  RETURNS      : The address of the found instance, NULL otherwise
 *  SIDE EFFECTS : None
 *  NOTES        : An instance is searched for by name first in the
 *              current module - then in imported modules according
 *              to the order given in the current module's definition
 ***************************************************************************/
globle INSTANCE_TYPE *FindInstanceBySymbol(void *theEnv, ATOM_HN *moduleAndInstanceName)
{
    unsigned modulePosition, searchImports;
    ATOM_HN *parent_module, *instanceName;
    struct module_definition *currentModule, *module_def;

    currentModule = ((struct module_definition *)get_current_module(theEnv));

    /* =======================================
     *  Instance names of the form [<name>] are
     *  searched for only in the current module
     *  ======================================= */
    modulePosition = find_module_separator(to_string(moduleAndInstanceName));

    if( modulePosition == FALSE )
    {
        module_def = currentModule;
        instanceName = moduleAndInstanceName;
        searchImports = FALSE;
    }

    /* =========================================
     *  Instance names of the form [::<name>] are
     *  searched for in the current module and
     *  imported modules in the definition order
     *  ========================================= */
    else if( modulePosition == 1 )
    {
        module_def = currentModule;
        instanceName = garner_construct_name(theEnv, modulePosition, to_string(moduleAndInstanceName));
        searchImports = TRUE;
    }

    /* =============================================
     *  Instance names of the form [<module>::<name>]
     *  are searched for in the specified module
     *  ============================================= */
    else
    {
        parent_module = garner_module_name(theEnv, modulePosition, to_string(moduleAndInstanceName));
        module_def = (struct module_definition *)lookup_module(theEnv, to_string(parent_module));
        instanceName = garner_construct_name(theEnv, modulePosition, to_string(moduleAndInstanceName));

        if( module_def == NULL )
        {
            return(NULL);
        }

        searchImports = FALSE;
    }

    return(FindInstanceInModule(theEnv, instanceName, module_def, currentModule, searchImports));
}

/***************************************************
 *  NAME         : FindInstanceInModule
 *  DESCRIPTION  : Finds an instance of the given name
 *              in the given module in scope of
 *              the given current module
 *              (will also search imported modules
 *               if specified)
 *  INPUTS       : 1) The instance name (no module)
 *              2) The module to search
 *              3) The currently active module
 *              4) A flag indicating whether
 *                 to search imported modules of
 *                 given module as well
 *  RETURNS      : The instance (NULL if none found)
 *  SIDE EFFECTS : None
 *  NOTES        : None
 ***************************************************/
globle INSTANCE_TYPE *FindInstanceInModule(void *theEnv, ATOM_HN *instanceName, struct module_definition *module_def, struct module_definition *currentModule, unsigned searchImports)
{
    INSTANCE_TYPE *startInstance, *ins;

    /* ===============================
     *  Find the first instance of the
     *  correct name in the hash chain
     *  =============================== */
    startInstance = InstanceData(theEnv)->InstanceTable[HashInstance(instanceName)];

    while( startInstance != NULL )
    {
        if( startInstance->name == instanceName )
        {
            break;
        }

        startInstance = startInstance->nxtHash;
    }

    if( startInstance == NULL )
    {
        return(NULL);
    }

    /* ===========================================
     *  Look for the instance in the specified
     *  module - if the class of the found instance
     *  is in scope of the current module, we have
     *  found the instance
     *  =========================================== */
    for( ins = startInstance ;
         (ins != NULL) ? (ins->name == startInstance->name) : FALSE ;
         ins = ins->nxtHash )
    {
        if((ins->cls->header.my_module->module_def == module_def) &&
           DefclassInScope(theEnv, ins->cls, currentModule))
        {
            return(ins);
        }
    }

    /* ================================
     *  For ::<name> formats, we need to
     *  search imported modules too
     *  ================================ */
    if( searchImports == FALSE )
    {
        return(NULL);
    }

    set_module_unvisited(theEnv);
    return(FindImportedInstance(theEnv, module_def, currentModule, startInstance));
}

/********************************************************************
 *  NAME         : FindInstanceSlot
 *  DESCRIPTION  : Finds an instance slot by name
 *  INPUTS       : 1) The address of the instance
 *              2) The symbolic name of the slot
 *  RETURNS      : The address of the slot, NULL if not found
 *  SIDE EFFECTS : None
 *  NOTES        : None
 ********************************************************************/
globle INSTANCE_SLOT *FindInstanceSlot(void *theEnv, INSTANCE_TYPE *ins, ATOM_HN *sname)
{
    register int i;

    i = FindInstanceTemplateSlot(theEnv, ins->cls, sname);
    return((i != -1) ? ins->slotAddresses[i] : NULL);
}

/********************************************************************
 *  NAME         : FindInstanceTemplateSlot
 *  DESCRIPTION  : Performs a search on an class's instance
 *                template slot array to find a slot by name
 *  INPUTS       : 1) The address of the class
 *              2) The symbolic name of the slot
 *  RETURNS      : The index of the slot, -1 if not found
 *  SIDE EFFECTS : None
 *  NOTES        : The slot's unique id is used as index into
 *              the slot map array.
 ********************************************************************/
globle int FindInstanceTemplateSlot(void *theEnv, DEFCLASS *cls, ATOM_HN *sname)
{
    int sid;

    sid = FindSlotNameID(theEnv, sname);

    if( sid == -1 )
    {
        return(-1);
    }

    if( sid > (int)cls->maxSlotNameID )
    {
        return(-1);
    }

    return((int)cls->slotNameMap[sid] - 1);
}

/*******************************************************
 *  NAME         : PutSlotValue
 *  DESCRIPTION  : Evaluates new slot-expression and
 *                stores it as a list
 *                variable for the slot.
 *  INPUTS       : 1) The address of the instance
 *                 (NULL if no trace-messages desired)
 *              2) The address of the slot
 *              3) The address of the value
 *              4) core_data_object_ptr to store the
 *                 set value
 *              5) The command doing the put-
 *  RETURNS      : FALSE on errors, or TRUE
 *  SIDE EFFECTS : Old value deleted and new one allocated
 *              Old value symbols deinstalled
 *              New value symbols installed
 *  NOTES        : None
 *******************************************************/
globle int PutSlotValue(void *theEnv, INSTANCE_TYPE *ins, INSTANCE_SLOT *sp, core_data_object *val, core_data_object *setVal, char *theCommand)
{
    if( ValidSlotValue(theEnv, val, sp->desc, ins, theCommand) == FALSE )
    {
        core_set_pointer_type(setVal, ATOM);
        core_set_pointer_value(setVal, get_false(theEnv));
        return(FALSE);
    }

    return(DirectPutSlotValue(theEnv, ins, sp, val, setVal));
}

/*******************************************************
 *  NAME         : DirectPutSlotValue
 *  DESCRIPTION  : Evaluates new slot-expression and
 *                stores it as a list
 *                variable for the slot.
 *  INPUTS       : 1) The address of the instance
 *                 (NULL if no trace-messages desired)
 *              2) The address of the slot
 *              3) The address of the value
 *              4) core_data_object_ptr to store the
 *                 set value
 *  RETURNS      : FALSE on errors, or TRUE
 *  SIDE EFFECTS : Old value deleted and new one allocated
 *              Old value symbols deinstalled
 *              New value symbols installed
 *  NOTES        : None
 *******************************************************/
globle int DirectPutSlotValue(void *theEnv, INSTANCE_TYPE *ins, INSTANCE_SLOT *sp, core_data_object *val, core_data_object *setVal)
{
    register long i, j; /* 6.04 Bug Fix */

    core_data_object tmpVal;

    core_set_pointer_type(setVal, ATOM);
    core_set_pointer_value(setVal, get_false(theEnv));

    if( val == NULL )
    {
        error_system(theEnv, "INSFUN", 1);
        exit_router(theEnv, EXIT_FAILURE);
    }
    else if( core_get_pointer_value(val) == core_get_function_primitive_data(theEnv)->null_arg_value )
    {
        if( sp->desc->dynamicDefault )
        {
            val = &tmpVal;

            if( !EvaluateAndStoreInDataObject(theEnv, sp->desc->multiple,
                                              (core_expression_object *)sp->desc->defaultValue, val, TRUE))
            {
                return(FALSE);
            }
        }
        else
        {
            val = (core_data_object *)sp->desc->defaultValue;
        }
    }


    if( sp->desc->multiple == 0 )
    {
        core_decrement_atom(theEnv, (int)sp->type, sp->value);

        /* ======================================
         *  Assumed that multfield already checked
         *  to be of cardinality 1
         *  ====================================== */
        if( core_get_pointer_type(val) == LIST )
        {
            sp->type = get_list_node_type(core_get_pointer_value(val), core_get_data_ptr_start(val));
            sp->value = get_list_node_value(core_get_pointer_value(val), core_get_data_ptr_start(val));
        }
        else
        {
            sp->type = val->type;
            sp->value = val->value;
        }

        core_install_data(theEnv, (int)sp->type, sp->value);
        core_set_pointer_type(setVal, sp->type);
        core_set_pointer_value(setVal, sp->value);
    }
    else
    {
        uninstall_list(theEnv, (LIST_PTR)sp->value);
        track_list(theEnv, (LIST_PTR)sp->value);
        sp->type = LIST;

        if( val->type == LIST )
        {
            sp->value = create_sized_list(theEnv, (unsigned long)core_get_data_ptr_length(val));

            for( i = 1, j = core_get_data_ptr_start(val) ; i <= core_get_data_ptr_length(val) ; i++, j++ )
            {
                set_list_node_type(sp->value, i, get_list_node_type(val->value, j));
                set_list_node_value(sp->value, i, get_list_node_value(val->value, j));
            }
        }
        else
        {
            sp->value = create_sized_list(theEnv, 1L);
            set_list_node_type(sp->value, 1, (short)val->type);
            set_list_node_value(sp->value, 1, val->value);
        }

        install_list(theEnv, (struct list *)sp->value);
        core_set_pointer_type(setVal, LIST);
        core_set_pointer_value(setVal, sp->value);
        core_set_data_ptr_start(setVal, 1);
        core_set_data_ptr_end(setVal, get_list_length(sp->value));
    }

    /* ==================================================
     *  6.05 Bug fix - any slot set directly or indirectly
     *  by a slot override or other side-effect during an
     *  instance initialization should not have its
     *  default value set
     *  ================================================== */

    sp->override = ins->initializeInProgress;

#if DEBUGGING_FUNCTIONS

    if( ins->cls->traceSlots )
    {
        if( sp->desc->shared )
        {
            print_router(theEnv, WTRACE, "::= shared slot ");
        }
        else
        {
            print_router(theEnv, WTRACE, "::= local slot ");
        }

        print_router(theEnv, WTRACE, to_string(sp->desc->slotName->name));
        print_router(theEnv, WTRACE, " in instance ");
        print_router(theEnv, WTRACE, to_string(ins->name));
        print_router(theEnv, WTRACE, " <- ");

        if( sp->type != LIST )
        {
            core_print_atom(theEnv, WTRACE, (int)sp->type, sp->value);
        }
        else
        {
            print_list(theEnv, WTRACE, (LIST_PTR)sp->value, 0,
                      (long)(GetInstanceSlotLength(sp) - 1), TRUE);
        }

        print_router(theEnv, WTRACE, "\n");
    }

#endif
    InstanceData(theEnv)->ChangesToInstances = TRUE;


    return(TRUE);
}

/*******************************************************************
 *  NAME         : ValidSlotValue
 *  DESCRIPTION  : Determines if a value is appropriate
 *                for a slot-value
 *  INPUTS       : 1) The value buffer
 *              2) Slot descriptor
 *              3) Instance for which slot is being checked
 *                 (can be NULL)
 *              4) Buffer holding printout of the offending command
 *                 (if NULL assumes message-handler is executing
 *                  and calls PrintHandler for CurrentCore instead)
 *  RETURNS      : TRUE if value is OK, FALSE otherwise
 *  SIDE EFFECTS : Sets eval_error if slot is not OK
 *  NOTES        : Examines all fields of a list
 *******************************************************************/
globle int ValidSlotValue(void *theEnv, core_data_object *val, SLOT_DESC *sd, INSTANCE_TYPE *ins, char *theCommand)
{
    register int violationCode;

    /* ===================================
     *  Special null_arg_value means to reset
     *  slot to default value
     *  =================================== */
    if( core_get_pointer_value(val) == core_get_function_primitive_data(theEnv)->null_arg_value )
    {
        return(TRUE);
    }

    if((sd->multiple == 0) && (val->type == LIST) &&
       (core_get_data_ptr_length(val) != 1))
    {
        error_print_id(theEnv, "INSFUN", 7, FALSE);
        core_print_data(theEnv, WERROR, val);
        print_router(theEnv, WERROR, " illegal for single-field ");
        PrintSlot(theEnv, WERROR, sd, ins, theCommand);
        print_router(theEnv, WERROR, ".\n");
        core_set_eval_error(theEnv, TRUE);
        return(FALSE);
    }

    if( val->type == RVOID )
    {
        error_print_id(theEnv, "INSFUN", 8, FALSE);
        print_router(theEnv, WERROR, "Void function illegal value for ");
        PrintSlot(theEnv, WERROR, sd, ins, theCommand);
        print_router(theEnv, WERROR, ".\n");
        core_set_eval_error(theEnv, TRUE);
        return(FALSE);
    }

    if( getDynamicConstraints(theEnv))
    {
        violationCode = garner_violations(theEnv, val, sd->constraint);

        if( violationCode != NO_VIOLATION )
        {
            error_print_id(theEnv, "CSTRNCHK", 1, FALSE);

            if((core_get_pointer_type(val) == LIST) && (sd->multiple == 0))
            {
                core_print_atom(theEnv, WERROR, get_list_node_type(core_get_pointer_value(val), core_get_data_ptr_start(val)),
                          get_list_node_value(core_get_pointer_value(val), core_get_data_ptr_end(val)));
            }
            else
            {
                core_print_data(theEnv, WERROR, val);
            }

            print_router(theEnv, WERROR, " for ");
            PrintSlot(theEnv, WERROR, sd, ins, theCommand);
            report_constraint_violation_error(theEnv, NULL, NULL, 0, 0, NULL, 0,
                                            violationCode, sd->constraint, FALSE);
            core_set_eval_error(theEnv, TRUE);
            return(FALSE);
        }
    }

    return(TRUE);
}

/********************************************************
 *  NAME         : CheckInstance
 *  DESCRIPTION  : Checks to see if the first argument to
 *              a function is a valid instance
 *  INPUTS       : Name of the calling function
 *  RETURNS      : The address of the instance
 *  SIDE EFFECTS : eval_error set and messages printed
 *              on errors
 *  NOTES        : Used by Initialize and ModifyInstance
 ********************************************************/
globle INSTANCE_TYPE *CheckInstance(void *theEnv, char *func)
{
    INSTANCE_TYPE *ins;
    core_data_object temp;

    core_eval_expression(theEnv, core_get_first_arg(), &temp);

    if( temp.type == INSTANCE_ADDRESS )
    {
        ins = (INSTANCE_TYPE *)temp.value;

        if( ins->garbage == 1 )
        {
            StaleInstanceAddress(theEnv, func, 0);
            core_set_eval_error(theEnv, TRUE);
            return(NULL);
        }
    }
    else if((temp.type == INSTANCE_NAME) ||
            (temp.type == ATOM))
    {
        ins = FindInstanceBySymbol(theEnv, (ATOM_HN *)temp.value);

        if( ins == NULL )
        {
            NoInstanceError(theEnv, to_string(temp.value), func);
            return(NULL);
        }
    }
    else
    {
        error_print_id(theEnv, "INSFUN", 1, FALSE);
        print_router(theEnv, WERROR, "Expected a valid instance in function ");
        print_router(theEnv, WERROR, func);
        print_router(theEnv, WERROR, ".\n");
        core_set_eval_error(theEnv, TRUE);
        return(NULL);
    }

    return(ins);
}

/***************************************************
 *  NAME         : NoInstanceError
 *  DESCRIPTION  : Prints out an appropriate error
 *               message when an instance cannot be
 *               found for a function
 *  INPUTS       : 1) The instance name
 *              2) The function name
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : None
 *  NOTES        : None
 ***************************************************/
globle void NoInstanceError(void *theEnv, char *iname, char *func)
{
    error_print_id(theEnv, "INSFUN", 2, FALSE);
    print_router(theEnv, WERROR, "No such instance ");
    print_router(theEnv, WERROR, iname);
    print_router(theEnv, WERROR, " in function ");
    print_router(theEnv, WERROR, func);
    print_router(theEnv, WERROR, ".\n");
    core_set_eval_error(theEnv, TRUE);
}

/***************************************************
 *  NAME         : StaleInstanceAddress
 *  DESCRIPTION  : Prints out an appropriate error
 *               message when an instance address
 *               is no longer valid
 *  INPUTS       : The function name
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : None
 *  NOTES        : None
 ***************************************************/
globle void StaleInstanceAddress(void *theEnv, char *func, int whichArg)
{
    error_print_id(theEnv, "INSFUN", 4, FALSE);
    print_router(theEnv, WERROR, "Invalid instance-address in function ");
    print_router(theEnv, WERROR, func);

    if( whichArg > 0 )
    {
        print_router(theEnv, WERROR, ", argument #");
        core_print_long(theEnv, WERROR, (long long)whichArg);
    }

    print_router(theEnv, WERROR, ".\n");
}

/**********************************************************************
 *  NAME         : EnvGetInstancesChanged
 *  DESCRIPTION  : Returns whether instances have changed
 *                (any were added/deleted or slot values were changed)
 *                since last time flag was set to FALSE
 *  INPUTS       : None
 *  RETURNS      : The instances-changed flag
 *  SIDE EFFECTS : None
 *  NOTES        : Used by interfaces to update instance windows
 **********************************************************************/
globle int EnvGetInstancesChanged(void *theEnv)
{
    return(InstanceData(theEnv)->ChangesToInstances);
}

/*******************************************************
 *  NAME         : EnvSetInstancesChanged
 *  DESCRIPTION  : Sets instances-changed flag (see above)
 *  INPUTS       : The value (TRUE or FALSE)
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : The flag is set
 *  NOTES        : None
 *******************************************************/
globle void EnvSetInstancesChanged(void *theEnv, int changed)
{
    InstanceData(theEnv)->ChangesToInstances = changed;
}

/*******************************************************************
 *  NAME         : PrintSlot
 *  DESCRIPTION  : Displays the name and origin of a slot
 *  INPUTS       : 1) The logical output name
 *              2) The slot descriptor
 *              3) The instance source (can be NULL)
 *              4) Buffer holding printout of the offending command
 *                 (if NULL assumes message-handler is executing
 *                  and calls PrintHandler for CurrentCore instead)
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Message printed
 *  NOTES        : None
 *******************************************************************/
globle void PrintSlot(void *theEnv, char *logName, SLOT_DESC *sd, INSTANCE_TYPE *ins, char *theCommand)
{
    print_router(theEnv, logName, "slot ");
    print_router(theEnv, logName, to_string(sd->slotName->name));

    if( ins != NULL )
    {
        print_router(theEnv, logName, " of instance [");
        print_router(theEnv, logName, to_string(ins->name));
        print_router(theEnv, logName, "]");
    }
    else if( sd->cls != NULL )
    {
        print_router(theEnv, logName, " of class ");
        print_router(theEnv, logName, EnvGetDefclassName(theEnv, (void *)sd->cls));
    }

    print_router(theEnv, logName, " found in ");

    if( theCommand != NULL )
    {
        print_router(theEnv, logName, theCommand);
    }
    else
    {
        PrintHandler(theEnv, logName, MessageHandlerData(theEnv)->CurrentCore->hnd, FALSE);
    }
}

/*****************************************************
 *  NAME         : PrintInstanceNameAndClass
 *  DESCRIPTION  : Displays an instance's name and class
 *  INPUTS       : 1) Logical name of output
 *              2) The instance
 *              3) Flag indicating whether to
 *                 print carriage-return at end
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Instnace name and class printed
 *  NOTES        : None
 *****************************************************/
globle void PrintInstanceNameAndClass(void *theEnv, char *logical_name, INSTANCE_TYPE *theInstance, BOOLEAN linefeedFlag)
{
    print_router(theEnv, logical_name, "[");
    print_router(theEnv, logical_name, EnvGetInstanceName(theEnv, (void *)theInstance));
    print_router(theEnv, logical_name, "] of ");
    PrintClassName(theEnv, logical_name, theInstance->cls, linefeedFlag);
}

/***************************************************
 *  NAME         : PrintInstanceName
 *  DESCRIPTION  : Used by the rule system commands
 *              such as (matches) and (agenda)
 *              to print out the name of an instance
 *  INPUTS       : 1) The logical output name
 *              2) A pointer to the instance
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Name of instance printed
 *  NOTES        : None
 ***************************************************/
globle void PrintInstanceName(void *theEnv, char *logName, void *vins)
{
    INSTANCE_TYPE *ins;

    ins = (INSTANCE_TYPE *)vins;

    if( ins->garbage )
    {
        print_router(theEnv, logName, "<stale instance [");
        print_router(theEnv, logName, to_string(ins->name));
        print_router(theEnv, logName, "]>");
    }
    else
    {
        print_router(theEnv, logName, "[");
        print_router(theEnv, logName, to_string(GetFullInstanceName(theEnv, ins)));
        print_router(theEnv, logName, "]");
    }
}

/***************************************************
 *  NAME         : PrintInstanceLongForm
 *  DESCRIPTION  : Used by kernel to print
 *              instance addresses
 *  INPUTS       : 1) The logical output name
 *              2) A pointer to the instance
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Address of instance printed
 *  NOTES        : None
 ***************************************************/
globle void PrintInstanceLongForm(void *theEnv, char *logName, void *vins)
{
    INSTANCE_TYPE *ins = (INSTANCE_TYPE *)vins;

    if( core_get_utility_data(theEnv)->converting_inst_address_to_name )
    {
        if( ins == &InstanceData(theEnv)->DummyInstance )
        {
            print_router(theEnv, logName, "\"<Dummy Instance>\"");
        }
        else
        {
            print_router(theEnv, logName, "[");
            print_router(theEnv, logName, to_string(GetFullInstanceName(theEnv, ins)));
            print_router(theEnv, logName, "]");
        }
    }
    else
    {
        if( core_get_utility_data(theEnv)->stringifying_pointer )
        {
            print_router(theEnv, logName, "\"");
        }

        if( ins == &InstanceData(theEnv)->DummyInstance )
        {
            print_router(theEnv, logName, "<Dummy Instance>");
        }
        else if( ins->garbage )
        {
            print_router(theEnv, logName, "<Stale Instance-");
            print_router(theEnv, logName, to_string(ins->name));
            print_router(theEnv, logName, ">");
        }
        else
        {
            print_router(theEnv, logName, "<Instance-");
            print_router(theEnv, logName, to_string(GetFullInstanceName(theEnv, ins)));
            print_router(theEnv, logName, ">");
        }

        if( core_get_utility_data(theEnv)->stringifying_pointer )
        {
            print_router(theEnv, logName, "\"");
        }
    }
}

/* =========================================
 *****************************************
 *       INTERNALLY VISIBLE FUNCTIONS
 *  =========================================
 ***************************************** */

/*****************************************************
 *  NAME         : FindImportedInstance
 *  DESCRIPTION  : Searches imported modules for an
 *              instance of the correct name
 *              The imports are searched recursively
 *              in the order of the module definition
 *  INPUTS       : 1) The module for which to
 *                 search imported modules
 *              2) The currently active module
 *              3) The first instance of the
 *                 correct name (cannot be NULL)
 *  RETURNS      : An instance of the correct name
 *              imported from another module which
 *              is in scope of the current module
 *  SIDE EFFECTS : None
 *  NOTES        : None
 *****************************************************/
static INSTANCE_TYPE *FindImportedInstance(void *theEnv, struct module_definition *module_def, struct module_definition *currentModule, INSTANCE_TYPE *startInstance)
{
    struct portable_item *imports;
    INSTANCE_TYPE *ins;

    if( module_def->visited )
    {
        return(NULL);
    }

    module_def->visited = TRUE;
    imports = module_def->imports;

    while( imports != NULL )
    {
        module_def = (struct module_definition *)
                    lookup_module(theEnv, to_string(imports->parent_module));

        for( ins = startInstance ;
             (ins != NULL) ? (ins->name == startInstance->name) : FALSE ;
             ins = ins->nxtHash )
        {
            if((ins->cls->header.my_module->module_def == module_def) &&
               DefclassInScope(theEnv, ins->cls, currentModule))
            {
                return(ins);
            }
        }

        ins = FindImportedInstance(theEnv, module_def, currentModule, startInstance);

        if( ins != NULL )
        {
            return(ins);
        }

        imports = imports->next;
    }

    /* ========================================================
     *  Make sure instances of system classes are always visible
     *  ======================================================== */
    for( ins = startInstance ;
         (ins != NULL) ? (ins->name == startInstance->name) : FALSE ;
         ins = ins->nxtHash )
    {
        if( ins->cls->system )
        {
            return(ins);
        }
    }

    return(NULL);
}

#endif
