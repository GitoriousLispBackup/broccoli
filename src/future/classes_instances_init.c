/* Purpose:  Creation and Deletion of Instances Routines     */

/* =========================================
 *****************************************
 *            EXTERNAL DEFINITIONS
 *  =========================================
 ***************************************** */
#include "setup.h"

#if OBJECT_SYSTEM

#include "classes_kernel.h"
#include "funcs_class.h"
#include "core_environment.h"
#include "core_memory.h"
#include "core_functions.h"
#include "funcs_instance.h"
#include "modules_query.h"
#include "classes_methods_kernel.h"
#include "funcs_method.h"
#include "core_functions_util.h"
#include "router.h"
#include "sysdep.h"
#include "core_gc.h"

#define __CLASSES_INSTANCES_INIT_SOURCE__
#include "classes_instances_init.h"

#include "classes_instances_kernel.h"
#include "core_watch.h"

/* =========================================
 *****************************************
 *                CONSTANTS
 *  =========================================
 ***************************************** */
#define MAKE_TRACE   "==>"
#define UNMAKE_TRACE "<=="

/* =========================================
 *****************************************
 *   INTERNALLY VISIBLE FUNCTION HEADERS
 *  =========================================
 ***************************************** */

static INSTANCE_TYPE * NewInstance(void *);
static INSTANCE_TYPE * InstanceLocationInfo(void *, DEFCLASS *, ATOM_HN *, INSTANCE_TYPE **, unsigned *);
static void            InstallInstance(void *, INSTANCE_TYPE *, int);
static void BuildDefaultSlots(void *, BOOLEAN);
static int  CoreInitializeInstance(void *, INSTANCE_TYPE *, core_expression_object *);
static int  InsertSlotOverrides(void *, INSTANCE_TYPE *, core_expression_object *);
static void EvaluateClassDefaults(void *, INSTANCE_TYPE *);

#if DEBUGGING_FUNCTIONS
static void PrintInstanceWatch(void *, char *, INSTANCE_TYPE *);
#endif

/* =========================================
 *****************************************
 *       EXTERNALLY VISIBLE FUNCTIONS
 *  =========================================
 ***************************************** */

/***********************************************************
 *  NAME         : InitializeInstanceCommand
 *  DESCRIPTION  : Initializes an instance of a class
 *  INPUTS       : The address of the result value
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Instance intialized
 *  NOTES        : H/L Syntax:
 *              (active-initialize-instance <instance-name>
 *                 <slot-override>*)
 ***********************************************************/
globle void InitializeInstanceCommand(void *theEnv, core_data_object *result)
{
    INSTANCE_TYPE *ins;

    core_set_pointer_type(result, ATOM);
    core_set_pointer_value(result, get_false(theEnv));
    ins = CheckInstance(theEnv, "initialize-instance");

    if( ins == NULL )
    {
        return;
    }

    if( CoreInitializeInstance(theEnv, ins, core_get_first_arg()->next_arg) == TRUE )
    {
        core_set_pointer_type(result, INSTANCE_NAME);
        core_set_pointer_value(result, (void *)ins->name);
    }
}

/****************************************************************
 *  NAME         : MakeInstanceCommand
 *  DESCRIPTION  : Creates and initializes an instance of a class
 *  INPUTS       : The address of the result value
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Instance intialized
 *  NOTES        : H/L Syntax:
 *              (active-make-instance <instance-name> of <class>
 *                 <slot-override>*)
 ****************************************************************/
globle void MakeInstanceCommand(void *theEnv, core_data_object *result)
{
    ATOM_HN *iname;
    INSTANCE_TYPE *ins;
    core_data_object temp;
    DEFCLASS *cls;

    core_set_pointer_type(result, ATOM);
    core_set_pointer_value(result, get_false(theEnv));
    core_eval_expression(theEnv, core_get_first_arg(), &temp);

    if((core_get_type(temp) != ATOM) &&
       (core_get_type(temp) != INSTANCE_NAME))
    {
        error_print_id(theEnv, "INSMNGR", 1, FALSE);
        print_router(theEnv, WERROR, "Expected a valid name for new instance.\n");
        core_set_eval_error(theEnv, TRUE);
        return;
    }

    iname = (ATOM_HN *)core_get_value(temp);

    if( core_get_first_arg()->next_arg->type == DEFCLASS_PTR )
    {
        cls = (DEFCLASS *)core_get_first_arg()->next_arg->value;
    }
    else
    {
        core_eval_expression(theEnv, core_get_first_arg()->next_arg, &temp);

        if( core_get_type(temp) != ATOM )
        {
            error_print_id(theEnv, "INSMNGR", 2, FALSE);
            print_router(theEnv, WERROR, "Expected a valid class name for new instance.\n");
            core_set_eval_error(theEnv, TRUE);
            return;
        }

        cls = LookupDefclassInScope(theEnv, core_convert_data_to_string(temp));

        if( cls == NULL )
        {
            ClassExistError(theEnv, to_string(core_get_expression_function_handle(core_get_evaluation_data(theEnv)->current_expression)),
                            core_convert_data_to_string(temp));
            core_set_eval_error(theEnv, TRUE);
            return;
        }
    }

    ins = BuildInstance(theEnv, iname, cls, TRUE);

    if( ins == NULL )
    {
        return;
    }

    if( CoreInitializeInstance(theEnv, ins, core_get_first_arg()->next_arg->next_arg) == TRUE )
    {
        result->type = INSTANCE_NAME;
        result->value = (void *)GetFullInstanceName(theEnv, ins);
    }
    else
    {
        QuashInstance(theEnv, ins);
    }
}

/***************************************************
 *  NAME         : GetFullInstanceName
 *  DESCRIPTION  : If this function is called while
 *              the current module is other than
 *              the one in which the instance
 *              resides, then the module name is
 *              prepended to the instance name.
 *              Otherwise - the base name only is
 *              returned.
 *  INPUTS       : The instance
 *  RETURNS      : The instance name symbol (with
 *              module name and :: prepended)
 *  SIDE EFFECTS : Temporary buffer allocated possibly
 *              and new symbol created
 *  NOTES        : Used to differentiate between
 *              instances of the same name in
 *              different modules
 ***************************************************/
globle ATOM_HN *GetFullInstanceName(void *theEnv, INSTANCE_TYPE *ins)
{
    char *parent_module, *buffer;
    size_t bufsz;
    ATOM_HN *iname;

    if( ins == &InstanceData(theEnv)->DummyInstance )
    {
        return((ATOM_HN *)store_atom(theEnv, "Dummy Instance"));
    }

    if( ins->garbage )
    {
        return(ins->name);
    }

    if( ins->cls->header.my_module->module_def == ((struct module_definition *)get_current_module(theEnv)))
    {
        return(ins->name);
    }

    parent_module = get_module_name(theEnv, (void *)ins->cls->header.my_module->module_def);
    bufsz = (sizeof(char) * (strlen(parent_module) +
                             strlen(to_string(ins->name)) + 3));
    buffer = (char *)core_mem_alloc_no_init(theEnv, bufsz);
    sysdep_sprintf(buffer, "%s::%s", parent_module, to_string(ins->name));
    iname = (ATOM_HN *)store_atom(theEnv, buffer);
    core_mem_release(theEnv, (void *)buffer, bufsz);
    return(iname);
}

/***************************************************
 *  NAME         : BuildInstance
 *  DESCRIPTION  : Creates an uninitialized instance
 *  INPUTS       : 1) Name of the instance
 *              2) Class pointer
 *              3) Flag indicating whether init
 *                 message will be called for
 *                 this instance or not
 *  RETURNS      : The address of the new instance,
 *                NULL on errors (or when a
 *                a logical basis in a rule was
 *                deleted int the same RHS in
 *                which the instance creation
 *                occurred)
 *  SIDE EFFECTS : Old definition (if any) is deleted
 *  NOTES        : None
 ***************************************************/
globle INSTANCE_TYPE *BuildInstance(void *theEnv, ATOM_HN *iname, DEFCLASS *cls, BOOLEAN initMessage)
{
    INSTANCE_TYPE *ins, *iprv;
    unsigned hashTableIndex;
    unsigned modulePosition;
    ATOM_HN *parent_module;
    core_data_object temp;


    if( cls->abstract )
    {
        error_print_id(theEnv, "INSMNGR", 3, FALSE);
        print_router(theEnv, WERROR, "Cannot create instances of abstract class ");
        print_router(theEnv, WERROR, EnvGetDefclassName(theEnv, (void *)cls));
        print_router(theEnv, WERROR, ".\n");
        core_set_eval_error(theEnv, TRUE);
        return(NULL);
    }

    modulePosition = find_module_separator(to_string(iname));

    if( modulePosition )
    {
        parent_module = garner_module_name(theEnv, modulePosition, to_string(iname));

        if((parent_module == NULL) ||
           (parent_module != cls->header.my_module->module_def->name))
        {
            error_print_id(theEnv, "INSMNGR", 11, TRUE);
            print_router(theEnv, WERROR, "Invalid module specifier in new instance name.\n");
            core_set_eval_error(theEnv, TRUE);
            return(NULL);
        }

        iname = garner_construct_name(theEnv, modulePosition, to_string(iname));
    }

    ins = InstanceLocationInfo(theEnv, cls, iname, &iprv, &hashTableIndex);

    if( ins != NULL )
    {
        if( ins->installed == 0 )
        {
            error_print_id(theEnv, "INSMNGR", 4, FALSE);
            print_router(theEnv, WERROR, "The instance ");
            print_router(theEnv, WERROR, to_string(iname));
            print_router(theEnv, WERROR, " has a slot-value which depends on the instance definition.\n");
            core_set_eval_error(theEnv, TRUE);
            return(NULL);
        }

        ins->busy++;
        inc_atom_count(iname);

        if( ins->garbage == 0 )
        {
            if( InstanceData(theEnv)->MkInsMsgPass )
            {
                DirectMessage(theEnv, MessageHandlerData(theEnv)->DELETE_ATOM, ins, NULL, NULL);
            }
            else
            {
                QuashInstance(theEnv, ins);
            }
        }

        ins->busy--;
        dec_atom_count(theEnv, iname);

        if( ins->garbage == 0 )
        {
            error_print_id(theEnv, "INSMNGR", 5, FALSE);
            print_router(theEnv, WERROR, "Unable to delete old instance ");
            print_router(theEnv, WERROR, to_string(iname));
            print_router(theEnv, WERROR, ".\n");
            core_set_eval_error(theEnv, TRUE);
            return(NULL);
        }
    }

    /* =============================================================
     *  Create the base instance from the defaults of the inheritance
     *  precedence list
     *  ============================================================= */
    InstanceData(theEnv)->CurrentInstance = NewInstance(theEnv);


    InstanceData(theEnv)->CurrentInstance->name = iname;
    InstanceData(theEnv)->CurrentInstance->cls = cls;
    BuildDefaultSlots(theEnv, initMessage);

    /* ============================================================
     *  Put the instance in the instance hash table and put it on its
     *   class's instance list
     *  ============================================================ */
    InstanceData(theEnv)->CurrentInstance->hashTableIndex = hashTableIndex;

    if( iprv == NULL )
    {
        InstanceData(theEnv)->CurrentInstance->nxtHash = InstanceData(theEnv)->InstanceTable[hashTableIndex];

        if( InstanceData(theEnv)->InstanceTable[hashTableIndex] != NULL )
        {
            InstanceData(theEnv)->InstanceTable[hashTableIndex]->prvHash = InstanceData(theEnv)->CurrentInstance;
        }

        InstanceData(theEnv)->InstanceTable[hashTableIndex] = InstanceData(theEnv)->CurrentInstance;
    }
    else
    {
        InstanceData(theEnv)->CurrentInstance->nxtHash = iprv->nxtHash;

        if( iprv->nxtHash != NULL )
        {
            iprv->nxtHash->prvHash = InstanceData(theEnv)->CurrentInstance;
        }

        iprv->nxtHash = InstanceData(theEnv)->CurrentInstance;
        InstanceData(theEnv)->CurrentInstance->prvHash = iprv;
    }

    /* ======================================
     *  Put instance in global and class lists
     *  ====================================== */
    if( InstanceData(theEnv)->CurrentInstance->cls->instanceList == NULL )
    {
        InstanceData(theEnv)->CurrentInstance->cls->instanceList = InstanceData(theEnv)->CurrentInstance;
    }
    else
    {
        InstanceData(theEnv)->CurrentInstance->cls->instanceListBottom->nxtClass = InstanceData(theEnv)->CurrentInstance;
    }

    InstanceData(theEnv)->CurrentInstance->prvClass = InstanceData(theEnv)->CurrentInstance->cls->instanceListBottom;
    InstanceData(theEnv)->CurrentInstance->cls->instanceListBottom = InstanceData(theEnv)->CurrentInstance;

    if( InstanceData(theEnv)->InstanceList == NULL )
    {
        InstanceData(theEnv)->InstanceList = InstanceData(theEnv)->CurrentInstance;
    }
    else
    {
        InstanceData(theEnv)->InstanceListBottom->nxtList = InstanceData(theEnv)->CurrentInstance;
    }

    InstanceData(theEnv)->CurrentInstance->prvList = InstanceData(theEnv)->InstanceListBottom;
    InstanceData(theEnv)->InstanceListBottom = InstanceData(theEnv)->CurrentInstance;
    InstanceData(theEnv)->ChangesToInstances = TRUE;

    /* ==============================================================================
     *  Install the instance's name and slot-value symbols (prevent them from becoming
     *  is_ephemeral) - the class name and slot names are accounted for by the class
     *  ============================================================================== */
    InstallInstance(theEnv, InstanceData(theEnv)->CurrentInstance, TRUE);

    ins = InstanceData(theEnv)->CurrentInstance;
    InstanceData(theEnv)->CurrentInstance = NULL;

    if( InstanceData(theEnv)->MkInsMsgPass )
    {
        DirectMessage(theEnv, MessageHandlerData(theEnv)->CREATE_ATOM, ins, &temp, NULL);
    }


    return(ins);
}

/*****************************************************************************
 *  NAME         : InitSlotsCommand
 *  DESCRIPTION  : Calls Kernel Expression Evaluator core_eval_expression
 *                for each expression-value of an instance expression
 *
 *              Evaluates default slots only - slots that were specified
 *              by overrides (sp->override == 1) are ignored)
 *  INPUTS       : 1) Instance address
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Each core_data_object slot in the instance's slot array is replaced
 *                by the evaluation (by core_eval_expression) of the expression
 *                in the slot list.  The old expression-values
 *                are deleted.
 *  NOTES        : H/L Syntax: (init-slots <instance>)
 *****************************************************************************/
globle void InitSlotsCommand(void *theEnv, core_data_object *result)
{
    core_set_pointer_type(result, ATOM);
    core_set_pointer_value(result, get_false(theEnv));
    core_get_evaluation_data(theEnv)->eval_error = FALSE;

    if( CheckCurrentMessage(theEnv, "init-slots", TRUE) == FALSE )
    {
        return;
    }

    EvaluateClassDefaults(theEnv, GetActiveInstance(theEnv));

    if( !core_get_evaluation_data(theEnv)->eval_error )
    {
        core_set_pointer_type(result, INSTANCE_ADDRESS);
        core_set_pointer_value(result, (void *)GetActiveInstance(theEnv));
    }
}

/******************************************************
 *  NAME         : QuashInstance
 *  DESCRIPTION  : Deletes an instance if it is not in
 *                use, otherwise sticks it on the
 *                garbage list
 *  INPUTS       : The instance
 *  RETURNS      : 1 if successful, 0 otherwise
 *  SIDE EFFECTS : Instance deleted or added to garbage
 *  NOTES        : Even though the instance is removed
 *                from the class list, hash table and
 *                instance list, its links remain
 *                unchanged so that outside loops
 *                can still determine where the next
 *                node in the list is (assuming the
 *                instance was garbage collected).
 ******************************************************/
globle BOOLEAN QuashInstance(void *theEnv, INSTANCE_TYPE *ins)
{
    register int iflag;
    IGARBAGE *gptr;


    if( ins->garbage == 1 )
    {
        return(0);
    }

    if( ins->installed == 0 )
    {
        error_print_id(theEnv, "INSMNGR", 6, FALSE);
        print_router(theEnv, WERROR, "Cannot delete instance ");
        print_router(theEnv, WERROR, to_string(ins->name));
        print_router(theEnv, WERROR, " during initialization.\n");
        core_set_eval_error(theEnv, TRUE);
        return(0);
    }

#if DEBUGGING_FUNCTIONS

    if( ins->cls->traceInstances )
    {
        PrintInstanceWatch(theEnv, UNMAKE_TRACE, ins);
    }

#endif


    if( ins->prvHash != NULL )
    {
        ins->prvHash->nxtHash = ins->nxtHash;
    }
    else
    {
        InstanceData(theEnv)->InstanceTable[ins->hashTableIndex] = ins->nxtHash;
    }

    if( ins->nxtHash != NULL )
    {
        ins->nxtHash->prvHash = ins->prvHash;
    }

    if( ins->prvClass != NULL )
    {
        ins->prvClass->nxtClass = ins->nxtClass;
    }
    else
    {
        ins->cls->instanceList = ins->nxtClass;
    }

    if( ins->nxtClass != NULL )
    {
        ins->nxtClass->prvClass = ins->prvClass;
    }
    else
    {
        ins->cls->instanceListBottom = ins->prvClass;
    }

    if( ins->prvList != NULL )
    {
        ins->prvList->nxtList = ins->nxtList;
    }
    else
    {
        InstanceData(theEnv)->InstanceList = ins->nxtList;
    }

    if( ins->nxtList != NULL )
    {
        ins->nxtList->prvList = ins->prvList;
    }
    else
    {
        InstanceData(theEnv)->InstanceListBottom = ins->prvList;
    }

    iflag = ins->installed;
    InstallInstance(theEnv, ins, FALSE);

    /* ==============================================
     *  If the instance is the basis for an executing
     *  rule, don't bother deleting its slots yet, for
     *  they may still be is_needed by pattern variables
     *  ============================================== */
    if((iflag == 1)
       )
    {
        RemoveInstanceData(theEnv, ins);
    }

    if((ins->busy == 0) && (ins->depth > core_get_evaluation_data(theEnv)->eval_depth) &&
       (InstanceData(theEnv)->MaintainGarbageInstances == FALSE)
       )
    {
        dec_atom_count(theEnv, ins->name);
        core_mem_return_struct(theEnv, instance, ins);
    }
    else
    {
        gptr = core_mem_get_struct(theEnv, igarbage);
        ins->garbage = 1;
        gptr->ins = ins;
        gptr->nxt = InstanceData(theEnv)->InstanceGarbageList;
        InstanceData(theEnv)->InstanceGarbageList = gptr;
        core_get_gc_data(theEnv)->generational_item_count += 2;
        core_get_gc_data(theEnv)->generational_item_sz += InstanceSizeHeuristic(ins) + sizeof(IGARBAGE);
    }

    InstanceData(theEnv)->ChangesToInstances = TRUE;
    return(1);
}

/* =========================================
 *****************************************
 *       INTERNALLY VISIBLE FUNCTIONS
 *  =========================================
 ***************************************** */

/********************************************************
 *  NAME         : NewInstance
 *  DESCRIPTION  : Allocates and initializes a new instance
 *  INPUTS       : None
 *  RETURNS      : The address of the new instance
 *  SIDE EFFECTS : None
 *  NOTES        : None
 ********************************************************/
static INSTANCE_TYPE *NewInstance(void *theEnv)
{
    INSTANCE_TYPE *instance;

    instance = core_mem_get_struct(theEnv, instance);
    instance->busy = 0;
    instance->installed = 0;
    instance->garbage = 0;
    instance->initSlotsCalled = 0;
    instance->initializeInProgress = 0;
    instance->depth = core_get_evaluation_data(theEnv)->eval_depth;
    instance->name = NULL;
    instance->hashTableIndex = 0;
    instance->cls = NULL;
    instance->slots = NULL;
    instance->slotAddresses = NULL;
    instance->prvClass = NULL;
    instance->nxtClass = NULL;
    instance->prvHash = NULL;
    instance->nxtHash = NULL;
    instance->prvList = NULL;
    instance->nxtList = NULL;
    return(instance);
}

/*****************************************************************
 *  NAME         : InstanceLocationInfo
 *  DESCRIPTION  : Determines where a specified instance belongs
 *                in the instance hash table
 *  INPUTS       : 1) The class of the new instance
 *              2) The symbol for the name of the instance
 *              3) Caller's buffer for previous node address
 *              4) Caller's buffer for hash value
 *  RETURNS      : The address of the found instance, NULL otherwise
 *  SIDE EFFECTS : None
 *  NOTES        : Instance names only have to be unique within
 *              a module
 *****************************************************************/
static INSTANCE_TYPE *InstanceLocationInfo(void *theEnv, DEFCLASS *cls, ATOM_HN *iname, INSTANCE_TYPE **prv, unsigned *hashTableIndex)
{
    INSTANCE_TYPE *ins;

    *hashTableIndex = HashInstance(iname);
    ins = InstanceData(theEnv)->InstanceTable[*hashTableIndex];

    /* ========================================
     *  Make sure all instances of the same name
     *  are grouped together regardless of what
     *  module their classes are in
     *  ======================================== */
    *prv = NULL;

    while((ins != NULL) ? (ins->name != iname) : FALSE )
    {
        *prv = ins;
        ins = ins->nxtHash;
    }

    while((ins != NULL) ? (ins->name == iname) : FALSE )
    {
        if( ins->cls->header.my_module->module_def ==
            cls->header.my_module->module_def )
        {
            return(ins);
        }

        *prv = ins;
        ins = ins->nxtHash;
    }

    return(NULL);
}

/********************************************************
 *  NAME         : InstallInstance
 *  DESCRIPTION  : Prevent name and slot value symbols
 *                from being is_ephemeral (all others
 *                taken care of by class defn)
 *  INPUTS       : 1) The address of the instance
 *              2) A flag indicating whether to
 *                 install or deinstall
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Symbol counts incremented or decremented
 *  NOTES        : Slot symbol installations are handled
 *                by PutSlotValue
 ********************************************************/
static void InstallInstance(void *theEnv, INSTANCE_TYPE *ins, int set)
{
    if( set == TRUE )
    {
        if( ins->installed )
        {
            return;
        }

#if DEBUGGING_FUNCTIONS

        if( ins->cls->traceInstances )
        {
            PrintInstanceWatch(theEnv, MAKE_TRACE, ins);
        }

#endif
        ins->installed = 1;
        ins->depth = core_get_evaluation_data(theEnv)->eval_depth;
        inc_atom_count(ins->name);
        IncrementDefclassBusyCount(theEnv, (void *)ins->cls);
        InstanceData(theEnv)->GlobalNumberOfInstances++;
    }
    else
    {
        if( !ins->installed )
        {
            return;
        }

        ins->installed = 0;
        InstanceData(theEnv)->GlobalNumberOfInstances--;

        /* =======================================
         *  Class counts is decremented by
         *  RemoveInstanceData() when slot data is
         *  truly deleted - and name count is
         *  deleted by CleanupInstances() or
         *  QuashInstance() when instance is
         *  truly deleted
         *  ======================================= */
    }
}

/****************************************************************
 *  NAME         : BuildDefaultSlots
 *  DESCRIPTION  : The current instance's address is
 *                in the global variable CurrentInstance.
 *                This builds the slots and the default values
 *                from the direct class of the instance and its
 *                inheritances.
 *  INPUTS       : Flag indicating whether init message will be
 *              called for this instance or not
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Allocates the slot array for
 *                the current instance
 *  NOTES        : The current instance's address is
 *              stored in a global variable
 ****************************************************************/
static void BuildDefaultSlots(void *theEnv, BOOLEAN initMessage)
{
    register unsigned i, j;
    unsigned scnt;
    unsigned lscnt;
    INSTANCE_SLOT *dst = NULL, **adst;
    SLOT_DESC **src;

    scnt = InstanceData(theEnv)->CurrentInstance->cls->instanceSlotCount;
    lscnt = InstanceData(theEnv)->CurrentInstance->cls->localInstanceSlotCount;

    if( scnt > 0 )
    {
        InstanceData(theEnv)->CurrentInstance->slotAddresses = adst =
                                                                   (INSTANCE_SLOT **)core_mem_alloc_no_init(theEnv, (sizeof(INSTANCE_SLOT *) * scnt));

        if( lscnt != 0 )
        {
            InstanceData(theEnv)->CurrentInstance->slots = dst =
                                                               (INSTANCE_SLOT *)core_mem_alloc_no_init(theEnv, (sizeof(INSTANCE_SLOT) * lscnt));
        }

        src = InstanceData(theEnv)->CurrentInstance->cls->instanceTemplate;

        /* ==================================================
         *  A map of slot addresses is created - shared slots
         *  point at values in the class, and local slots
         *  point at values in the instance
         *
         *  Also - slots are always given an initial value
         *  (since slots cannot be unbound). If there is
         *  already an instance of a class with a shared slot,
         *  that value is left alone
         *  ================================================== */
        for( i = 0, j = 0 ; i < scnt ; i++ )
        {
            if( src[i]->shared )
            {
                src[i]->sharedCount++;
                adst[i] = &(src[i]->sharedValue);
            }
            else
            {
                dst[j].desc = src[i];
                dst[j].value = NULL;
                adst[i] = &dst[j++];
            }

            if( adst[i]->value == NULL )
            {
                adst[i]->valueRequired = initMessage;

                if( adst[i]->desc->multiple )
                {
                    adst[i]->type = LIST;
                    adst[i]->value = create_sized_list(theEnv, 0L);
                    install_list(theEnv, (LIST_PTR)adst[i]->value);
                }
                else
                {
                    adst[i]->type = ATOM;
                    adst[i]->value = store_atom(theEnv, "nil");
                    core_install_data(theEnv, (int)adst[i]->type, adst[i]->value);
                }
            }
            else
            {
                adst[i]->valueRequired = FALSE;
            }

            adst[i]->override = FALSE;
        }
    }
}

/*******************************************************************
 *  NAME         : CoreInitializeInstance
 *  DESCRIPTION  : Performs the core work for initializing an instance
 *  INPUTS       : 1) The instance address
 *              2) Slot override expressions
 *  RETURNS      : TRUE if all OK, FALSE otherwise
 *  SIDE EFFECTS : eval_error set on errors - slots evaluated
 *  NOTES        : None
 *******************************************************************/
static int CoreInitializeInstance(void *theEnv, INSTANCE_TYPE *ins, core_expression_object *ovrexp)
{
    core_data_object temp;

    if( ins->installed == 0 )
    {
        error_print_id(theEnv, "INSMNGR", 7, FALSE);
        print_router(theEnv, WERROR, "Instance ");
        print_router(theEnv, WERROR, to_string(ins->name));
        print_router(theEnv, WERROR, " is already being initialized.\n");
        core_set_eval_error(theEnv, TRUE);
        return(FALSE);
    }

    /* =======================================================
     *  Replace all default-slot values with any slot-overrides
     *  ======================================================= */
    ins->busy++;
    ins->installed = 0;

    /* =================================================================
     *  If the slots are initialized properly - the initializeInProgress
     *  flag will be turned off.
     *  ================================================================= */
    ins->initializeInProgress = 1;
    ins->initSlotsCalled = 0;

    if( InsertSlotOverrides(theEnv, ins, ovrexp) == FALSE )
    {
        ins->installed = 1;
        ins->busy--;
        return(FALSE);
    }

    /* =================================================================
     *  Now that all the slot expressions are established - replace them
     *  with their evaluation
     *  ================================================================= */

    if( InstanceData(theEnv)->MkInsMsgPass )
    {
        DirectMessage(theEnv, MessageHandlerData(theEnv)->INIT_ATOM, ins, &temp, NULL);
    }
    else
    {
        EvaluateClassDefaults(theEnv, ins);
    }

    ins->busy--;
    ins->installed = 1;

    if( core_get_evaluation_data(theEnv)->eval_error )
    {
        error_print_id(theEnv, "INSMNGR", 8, FALSE);
        print_router(theEnv, WERROR, "An error occurred during the initialization of instance ");
        print_router(theEnv, WERROR, to_string(ins->name));
        print_router(theEnv, WERROR, ".\n");
        return(FALSE);
    }

    ins->initializeInProgress = 0;
    return((ins->initSlotsCalled == 0) ? FALSE : TRUE);
}

/**********************************************************
 *  NAME         : InsertSlotOverrides
 *  DESCRIPTION  : Replaces value-expression for a slot
 *  INPUTS       : 1) The instance address
 *              2) The address of the beginning of the
 *                 list of slot-expressions
 *  RETURNS      : TRUE if all okay, FALSE otherwise
 *  SIDE EFFECTS : Old slot expression deallocated
 *  NOTES        : Assumes symbols not yet installed
 *              EVALUATES the slot-name expression but
 *                 simply copies the slot value-expression
 **********************************************************/
static int InsertSlotOverrides(void *theEnv, INSTANCE_TYPE *ins, core_expression_object *slot_exp)
{
    INSTANCE_SLOT *slot;
    core_data_object temp, junk;

    core_get_evaluation_data(theEnv)->eval_error = FALSE;

    while( slot_exp != NULL )
    {
        if((core_eval_expression(theEnv, slot_exp, &temp) == TRUE) ? TRUE :
           (core_get_type(temp) != ATOM))
        {
            error_print_id(theEnv, "INSMNGR", 9, FALSE);
            print_router(theEnv, WERROR, "Expected a valid slot name for slot-override.\n");
            core_set_eval_error(theEnv, TRUE);
            return(FALSE);
        }

        slot = FindInstanceSlot(theEnv, ins, (ATOM_HN *)core_get_value(temp));

        if( slot == NULL )
        {
            error_print_id(theEnv, "INSMNGR", 13, FALSE);
            print_router(theEnv, WERROR, "Slot ");
            print_router(theEnv, WERROR, core_convert_data_to_string(temp));
            print_router(theEnv, WERROR, " does not exist in instance ");
            print_router(theEnv, WERROR, to_string(ins->name));
            print_router(theEnv, WERROR, ".\n");
            core_set_eval_error(theEnv, TRUE);
            return(FALSE);
        }

        if( InstanceData(theEnv)->MkInsMsgPass )
        {
            DirectMessage(theEnv, slot->desc->overrideMessage,
                          ins, NULL, slot_exp->next_arg->args);
        }
        else if( slot_exp->next_arg->args )
        {
            if( EvaluateAndStoreInDataObject(theEnv, (int)slot->desc->multiple,
                                             slot_exp->next_arg->args, &temp, TRUE))
            {
                PutSlotValue(theEnv, ins, slot, &temp, &junk, "function make-instance");
            }
        }
        else
        {
            core_set_data_ptr_start(&temp, 1);
            core_set_data_ptr_end(&temp, 0);
            core_set_pointer_type(&temp, LIST);
            core_set_pointer_value(&temp, core_get_function_primitive_data(theEnv)->null_arg_value);
            PutSlotValue(theEnv, ins, slot, &temp, &junk, "function make-instance");
        }

        if( core_get_evaluation_data(theEnv)->eval_error )
        {
            return(FALSE);
        }

        slot->override = TRUE;
        slot_exp = slot_exp->next_arg->next_arg;
    }

    return(TRUE);
}

/*****************************************************************************
 *  NAME         : EvaluateClassDefaults
 *  DESCRIPTION  : Evaluates default slots only - slots that were specified
 *              by overrides (sp->override == 1) are ignored)
 *  INPUTS       : 1) Instance address
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Each core_data_object slot in the instance's slot array is replaced
 *                by the evaluation (by core_eval_expression) of the expression
 *                in the slot list.  The old expression-values
 *                are deleted.
 *  NOTES        : None
 *****************************************************************************/
static void EvaluateClassDefaults(void *theEnv, INSTANCE_TYPE *ins)
{
    INSTANCE_SLOT *slot;
    core_data_object temp, junk;
    long i;

    if( ins->initializeInProgress == 0 )
    {
        error_print_id(theEnv, "INSMNGR", 15, FALSE);
        core_set_eval_error(theEnv, TRUE);
        print_router(theEnv, WERROR, "init-slots not valid in this context.\n");
        return;
    }

    for( i = 0 ; i < ins->cls->instanceSlotCount ; i++ )
    {
        slot = ins->slotAddresses[i];

        /* ===========================================================
         *  Slot-overrides are just a short-hand for put-slots, so they
         *  should be done with messages.  Defaults are from the class
         *  definition and can be placed directly.
         *  =========================================================== */
        if( !slot->override )
        {
            if( slot->desc->dynamicDefault )
            {
                if( EvaluateAndStoreInDataObject(theEnv, (int)slot->desc->multiple,
                                                 (core_expression_object *)slot->desc->defaultValue,
                                                 &temp, TRUE))
                {
                    PutSlotValue(theEnv, ins, slot, &temp, &junk, "function init-slots");
                }
            }
            else if(((slot->desc->shared == 0) || (slot->desc->sharedCount == 1)) &&
                    (slot->desc->noDefault == 0))
            {
                DirectPutSlotValue(theEnv, ins, slot, (core_data_object *)slot->desc->defaultValue, &junk);
            }
            else if( slot->valueRequired )
            {
                error_print_id(theEnv, "INSMNGR", 14, FALSE);
                print_router(theEnv, WERROR, "Override required for slot ");
                print_router(theEnv, WERROR, to_string(slot->desc->slotName->name));
                print_router(theEnv, WERROR, " in instance ");
                print_router(theEnv, WERROR, to_string(ins->name));
                print_router(theEnv, WERROR, ".\n");
                core_set_eval_error(theEnv, TRUE);
            }

            slot->valueRequired = FALSE;

            if( ins->garbage == 1 )
            {
                print_router(theEnv, WERROR, to_string(ins->name));
                print_router(theEnv, WERROR, " instance deleted by slot-override evaluation.\n");
                core_set_eval_error(theEnv, TRUE);
            }

            if( core_get_evaluation_data(theEnv)->eval_error )
            {
                return;
            }
        }

        slot->override = FALSE;
    }

    ins->initSlotsCalled = 1;
}

#if DEBUGGING_FUNCTIONS

/***************************************************
 *  NAME         : PrintInstanceWatch
 *  DESCRIPTION  : Prints out a trace message for the
 *              creation/deletion of an instance
 *  INPUTS       : 1) The trace string indicating if
 *                 this is a creation or deletion
 *              2) The instance
 *  RETURNS      : Nothing usful
 *  SIDE EFFECTS : Watch message printed
 *  NOTES        : None
 ***************************************************/
static void PrintInstanceWatch(void *theEnv, char *traceString, INSTANCE_TYPE *theInstance)
{
    print_router(theEnv, WTRACE, traceString);
    print_router(theEnv, WTRACE, " instance ");
    PrintInstanceNameAndClass(theEnv, WTRACE, theInstance, TRUE);
}

#endif

#endif
