/* Purpose:  Instance modify and duplicate support routines  */

/* =========================================
 *****************************************
 *            EXTERNAL DEFINITIONS
 *  =========================================
 ***************************************** */
#include "setup.h"

#if OBJECT_SYSTEM

#include "core_arguments.h"
#include "core_memory.h"
#include "core_environment.h"
#include "core_functions.h"
#include "classes_instances_kernel.h"
#include "funcs_instance.h"
#include "classes_instances_init.h"
#include "parser_class_instances.h"
#include "funcs_mics.h"
#include "classes_methods_kernel.h"
#include "funcs_method.h"
#include "classes_methods_call.h"
#include "core_functions_util.h"
#include "router.h"

#define __CLASSES_INSTANCES_MODIFY_SOURCE__
#include "classes_instances_modify.h"

/* =========================================
 *****************************************
 *   INTERNALLY VISIBLE FUNCTION HEADERS
 *  =========================================
 ***************************************** */

static core_data_object * EvaluateSlotOverrides(void *, core_expression_object *, int *, int *);
static void          DeleteSlotOverrideEvaluations(void *, core_data_object *, int);
static void          ModifyMsgHandlerSupport(void *, core_data_object *, int);
static void          DuplicateMsgHandlerSupport(void *, core_data_object *, int);

/* =========================================
 *****************************************
 *       EXTERNALLY VISIBLE FUNCTIONS
 *  =========================================
 ***************************************** */


/***************************************************
 *  NAME         : SetupInstanceModDupCommands
 *  DESCRIPTION  : Defines function interfaces for
 *              modify- and duplicate- instance
 *              functions
 *  INPUTS       : None
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Functions defined to KB
 *  NOTES        : None
 ***************************************************/
globle void SetupInstanceModDupCommands(void *theEnv)
{
    core_define_function(theEnv, "modify-instance", 'u', PTR_FN ModifyInstance, "ModifyInstance", NULL);
    core_define_function(theEnv, "message-modify-instance", 'u', PTR_FN MsgModifyInstance,
                       "MsgModifyInstance", NULL);
    core_define_function(theEnv, "duplicate-instance", 'u', PTR_FN DuplicateInstance, "DuplicateInstance", NULL);
    core_define_function(theEnv, "message-duplicate-instance", 'u', PTR_FN MsgDuplicateInstance,
                       "MsgDuplicateInstance", NULL);

    core_define_function(theEnv, "(direct-modify)", 'u', PTR_FN DirectModifyMsgHandler, "DirectModifyMsgHandler", NULL);
    core_define_function(theEnv, "(message-modify)", 'u', PTR_FN MsgModifyMsgHandler, "MsgModifyMsgHandler", NULL);
    core_define_function(theEnv, "(direct-duplicate)", 'u', PTR_FN DirectDuplicateMsgHandler, "DirectDuplicateMsgHandler", NULL);
    core_define_function(theEnv, "(message-duplicate)", 'u', PTR_FN MsgDuplicateMsgHandler, "MsgDuplicateMsgHandler", NULL);

    core_add_function_parser(theEnv, "modify-instance", ParseInitializeInstance);
    core_add_function_parser(theEnv, "message-modify-instance", ParseInitializeInstance);
    core_add_function_parser(theEnv, "duplicate-instance", ParseInitializeInstance);
    core_add_function_parser(theEnv, "message-duplicate-instance", ParseInitializeInstance);
}

/*************************************************************
 *  NAME         : ModifyInstance
 *  DESCRIPTION  : Modifies slots of an instance via the
 *              direct-modify message
 *  INPUTS       : The address of the result value
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Slot updates performed directly
 *  NOTES        : H/L Syntax:
 *              (modify-instance <instance> <slot-override>*)
 *************************************************************/
globle void ModifyInstance(void *theEnv, core_data_object *result)
{
    INSTANCE_TYPE *ins;
    core_expression_object theExp;
    core_data_object *overrides;
    int oldOMDMV, overrideCount, error;

    /* ===========================================
     *  The slot-overrides need to be evaluated now
     *  to resolve any variable references before a
     *  new frame is pushed for message-handler
     *  execution
     *  =========================================== */

    overrides = EvaluateSlotOverrides(theEnv, core_get_first_arg()->next_arg,
                                      &overrideCount, &error);

    if( error )
    {
        core_set_pointer_type(result, ATOM);
        core_set_pointer_value(result, get_false(theEnv));
        return;
    }

    /* ==================================
     *  Find the instance and make sure it
     *  wasn't deleted by the overrides
     *  ================================== */
    ins = CheckInstance(theEnv, to_string(core_get_expression_function_handle(core_get_evaluation_data(theEnv)->current_expression)));

    if( ins == NULL )
    {
        core_set_pointer_type(result, ATOM);
        core_set_pointer_value(result, get_false(theEnv));
        DeleteSlotOverrideEvaluations(theEnv, overrides, overrideCount);
        return;
    }

    /* ======================================
     *  We are passing the slot override
     *  expression information along
     *  to whatever message-handler implements
     *  the modify
     *  ====================================== */
    theExp.type = DATA_OBJECT_ARRAY;
    theExp.value = (void *)overrides;
    theExp.args = NULL;
    theExp.next_arg = NULL;

    oldOMDMV = InstanceData(theEnv)->ObjectModDupMsgValid;
    InstanceData(theEnv)->ObjectModDupMsgValid = TRUE;
    DirectMessage(theEnv, lookup_atom(theEnv, DIRECT_MODIFY_STRING), ins, result, &theExp);
    InstanceData(theEnv)->ObjectModDupMsgValid = oldOMDMV;

    DeleteSlotOverrideEvaluations(theEnv, overrides, overrideCount);
}

/*************************************************************
 *  NAME         : MsgModifyInstance
 *  DESCRIPTION  : Modifies slots of an instance via the
 *              direct-modify message
 *  INPUTS       : The address of the result value
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Slot updates performed with put- messages
 *  NOTES        : H/L Syntax:
 *              (message-modify-instance <instance>
 *                 <slot-override>*)
 *************************************************************/
globle void MsgModifyInstance(void *theEnv, core_data_object *result)
{
    INSTANCE_TYPE *ins;
    core_expression_object theExp;
    core_data_object *overrides;
    int oldOMDMV, overrideCount, error;

    /* ===========================================
     *  The slot-overrides need to be evaluated now
     *  to resolve any variable references before a
     *  new frame is pushed for message-handler
     *  execution
     *  =========================================== */
    overrides = EvaluateSlotOverrides(theEnv, core_get_first_arg()->next_arg,
                                      &overrideCount, &error);

    if( error )
    {
        core_set_pointer_type(result, ATOM);
        core_set_pointer_value(result, get_false(theEnv));
        return;
    }

    /* ==================================
     *  Find the instance and make sure it
     *  wasn't deleted by the overrides
     *  ================================== */
    ins = CheckInstance(theEnv, to_string(core_get_expression_function_handle(core_get_evaluation_data(theEnv)->current_expression)));

    if( ins == NULL )
    {
        core_set_pointer_type(result, ATOM);
        core_set_pointer_value(result, get_false(theEnv));
        DeleteSlotOverrideEvaluations(theEnv, overrides, overrideCount);
        return;
    }

    /* ======================================
     *  We are passing the slot override
     *  expression information along
     *  to whatever message-handler implements
     *  the modify
     *  ====================================== */
    theExp.type = DATA_OBJECT_ARRAY;
    theExp.value = (void *)overrides;
    theExp.args = NULL;
    theExp.next_arg = NULL;

    oldOMDMV = InstanceData(theEnv)->ObjectModDupMsgValid;
    InstanceData(theEnv)->ObjectModDupMsgValid = TRUE;
    DirectMessage(theEnv, lookup_atom(theEnv, MSG_MODIFY_STRING), ins, result, &theExp);
    InstanceData(theEnv)->ObjectModDupMsgValid = oldOMDMV;

    DeleteSlotOverrideEvaluations(theEnv, overrides, overrideCount);
}

/*************************************************************
 *  NAME         : DuplicateInstance
 *  DESCRIPTION  : Duplicates an instance via the
 *              direct-duplicate message
 *  INPUTS       : The address of the result value
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Slot updates performed directly
 *  NOTES        : H/L Syntax:
 *              (duplicate-instance <instance>
 *                [to <instance-name>] <slot-override>*)
 *************************************************************/
globle void DuplicateInstance(void *theEnv, core_data_object *result)
{
    INSTANCE_TYPE *ins;
    core_data_object newName;
    core_expression_object theExp[2];
    core_data_object *overrides;
    int oldOMDMV, overrideCount, error;

    /* ===========================================
     *  The slot-overrides need to be evaluated now
     *  to resolve any variable references before a
     *  new frame is pushed for message-handler
     *  execution
     *  =========================================== */
    overrides = EvaluateSlotOverrides(theEnv, core_get_first_arg()->next_arg->next_arg,
                                      &overrideCount, &error);

    if( error )
    {
        core_set_pointer_type(result, ATOM);
        core_set_pointer_value(result, get_false(theEnv));
        return;
    }

    /* ==================================
     *  Find the instance and make sure it
     *  wasn't deleted by the overrides
     *  ================================== */
    ins = CheckInstance(theEnv, to_string(core_get_expression_function_handle(core_get_evaluation_data(theEnv)->current_expression)));

    if( ins == NULL )
    {
        core_set_pointer_type(result, ATOM);
        core_set_pointer_value(result, get_false(theEnv));
        DeleteSlotOverrideEvaluations(theEnv, overrides, overrideCount);
        return;
    }

    if( core_check_arg_type(theEnv, to_string(core_get_expression_function_handle(core_get_evaluation_data(theEnv)->current_expression)),
                        2, INSTANCE_NAME, &newName) == FALSE )
    {
        core_set_pointer_type(result, ATOM);
        core_set_pointer_value(result, get_false(theEnv));
        DeleteSlotOverrideEvaluations(theEnv, overrides, overrideCount);
        return;
    }

    /* ======================================
     *  We are passing the slot override
     *  expression information along
     *  to whatever message-handler implements
     *  the duplicate
     *  ====================================== */
    theExp[0].type = INSTANCE_NAME;
    theExp[0].value = newName.value;
    theExp[0].args = NULL;
    theExp[0].next_arg = &theExp[1];
    theExp[1].type = DATA_OBJECT_ARRAY;
    theExp[1].value = (void *)overrides;
    theExp[1].args = NULL;
    theExp[1].next_arg = NULL;

    oldOMDMV = InstanceData(theEnv)->ObjectModDupMsgValid;
    InstanceData(theEnv)->ObjectModDupMsgValid = TRUE;
    DirectMessage(theEnv, lookup_atom(theEnv, DIRECT_DUPLICATE_STRING), ins, result, &theExp[0]);
    InstanceData(theEnv)->ObjectModDupMsgValid = oldOMDMV;

    DeleteSlotOverrideEvaluations(theEnv, overrides, overrideCount);
}

/*************************************************************
 *  NAME         : MsgDuplicateInstance
 *  DESCRIPTION  : Duplicates an instance via the
 *              message-duplicate message
 *  INPUTS       : The address of the result value
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Slot updates performed w/ int & put- messages
 *  NOTES        : H/L Syntax:
 *              (duplicate-instance <instance>
 *                [to <instance-name>] <slot-override>*)
 *************************************************************/
globle void MsgDuplicateInstance(void *theEnv, core_data_object *result)
{
    INSTANCE_TYPE *ins;
    core_data_object newName;
    core_expression_object theExp[2];
    core_data_object *overrides;
    int oldOMDMV, overrideCount, error;

    /* ===========================================
     *  The slot-overrides need to be evaluated now
     *  to resolve any variable references before a
     *  new frame is pushed for message-handler
     *  execution
     *  =========================================== */
    overrides = EvaluateSlotOverrides(theEnv, core_get_first_arg()->next_arg->next_arg,
                                      &overrideCount, &error);

    if( error )
    {
        core_set_pointer_type(result, ATOM);
        core_set_pointer_value(result, get_false(theEnv));
        return;
    }

    /* ==================================
     *  Find the instance and make sure it
     *  wasn't deleted by the overrides
     *  ================================== */
    ins = CheckInstance(theEnv, to_string(core_get_expression_function_handle(core_get_evaluation_data(theEnv)->current_expression)));

    if( ins == NULL )
    {
        core_set_pointer_type(result, ATOM);
        core_set_pointer_value(result, get_false(theEnv));
        DeleteSlotOverrideEvaluations(theEnv, overrides, overrideCount);
        return;
    }

    if( core_check_arg_type(theEnv, to_string(core_get_expression_function_handle(core_get_evaluation_data(theEnv)->current_expression)),
                        2, INSTANCE_NAME, &newName) == FALSE )
    {
        core_set_pointer_type(result, ATOM);
        core_set_pointer_value(result, get_false(theEnv));
        DeleteSlotOverrideEvaluations(theEnv, overrides, overrideCount);
        return;
    }

    /* ======================================
     *  We are passing the slot override
     *  expression information along
     *  to whatever message-handler implements
     *  the duplicate
     *  ====================================== */
    theExp[0].type = INSTANCE_NAME;
    theExp[0].value = newName.value;
    theExp[0].args = NULL;
    theExp[0].next_arg = &theExp[1];
    theExp[1].type = DATA_OBJECT_ARRAY;
    theExp[1].value = (void *)overrides;
    theExp[1].args = NULL;
    theExp[1].next_arg = NULL;

    oldOMDMV = InstanceData(theEnv)->ObjectModDupMsgValid;
    InstanceData(theEnv)->ObjectModDupMsgValid = TRUE;
    DirectMessage(theEnv, lookup_atom(theEnv, MSG_DUPLICATE_STRING), ins, result, &theExp[0]);
    InstanceData(theEnv)->ObjectModDupMsgValid = oldOMDMV;

    DeleteSlotOverrideEvaluations(theEnv, overrides, overrideCount);
}

/*****************************************************
 *  NAME         : DirectDuplicateMsgHandler
 *  DESCRIPTION  : Implementation for the USER class
 *              handler direct-duplicate
 *
 *              Implements duplicate-instance message
 *              with a series of direct slot
 *              placements
 *  INPUTS       : A data object buffer to hold the
 *              result
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Slot values updated
 *  NOTES        : None
 *****************************************************/
globle void DirectDuplicateMsgHandler(void *theEnv, core_data_object *result)
{
    DuplicateMsgHandlerSupport(theEnv, result, FALSE);
}

/*****************************************************
 *  NAME         : MsgDuplicateMsgHandler
 *  DESCRIPTION  : Implementation for the USER class
 *              handler message-duplicate
 *
 *              Implements duplicate-instance message
 *              with a series of put- messages
 *  INPUTS       : A data object buffer to hold the
 *              result
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Slot values updated
 *  NOTES        : None
 *****************************************************/
globle void MsgDuplicateMsgHandler(void *theEnv, core_data_object *result)
{
    DuplicateMsgHandlerSupport(theEnv, result, TRUE);
}

/***************************************************
 *  NAME         : DirectModifyMsgHandler
 *  DESCRIPTION  : Implementation for the USER class
 *              handler direct-modify
 *
 *              Implements modify-instance message
 *              with a series of direct slot
 *              placements
 *  INPUTS       : A data object buffer to hold the
 *              result
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Slot values updated
 *  NOTES        : None
 ***************************************************/
globle void DirectModifyMsgHandler(void *theEnv, core_data_object *result)
{
    ModifyMsgHandlerSupport(theEnv, result, FALSE);
}

/***************************************************
 *  NAME         : MsgModifyMsgHandler
 *  DESCRIPTION  : Implementation for the USER class
 *              handler message-modify
 *
 *              Implements modify-instance message
 *              with a series of put- messages
 *  INPUTS       : A data object buffer to hold the
 *              result
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Slot values updated
 *  NOTES        : None
 ***************************************************/
globle void MsgModifyMsgHandler(void *theEnv, core_data_object *result)
{
    ModifyMsgHandlerSupport(theEnv, result, TRUE);
}

/* =========================================
 *****************************************
 *       INTERNALLY VISIBLE FUNCTIONS
 *  =========================================
 ***************************************** */

/***********************************************************
 *  NAME         : EvaluateSlotOverrides
 *  DESCRIPTION  : Evaluates the slot-override expressions
 *              for modify-instance and duplicate-instance
 *              Evaluations are stored in an array of
 *              data objects, where the metadata
 *              field points at the name of the slot
 *              The data object next fields are used
 *              to link the array as well.
 *  INPUTS       : 1) The slot override expressions
 *              2) A buffer to hold the number
 *                 of slot overrides
 *              3) A buffer to hold an error flag
 *  RETURNS      : The slot override data object array
 *  SIDE EFFECTS : Data object array allocated and initialized
 *              override count and error buffers set
 *  NOTES        : Slot overrides must be evaluated before
 *              calling supporting message-handlers for
 *              modify- and duplicate-instance in the
 *              event that the overrides contain variable
 *              references to an outer frame
 ***********************************************************/
static core_data_object *EvaluateSlotOverrides(void *theEnv, core_expression_object *ovExprs, int *ovCnt, int *error)
{
    core_data_object *ovs;
    int ovi;
    void *slotName;

    *error = FALSE;

    /* ==========================================
     *  There are two expressions chains for every
     *  slot override: one for the slot name and
     *  one for the slot value
     *  ========================================== */
    *ovCnt = core_count_args(ovExprs) / 2;

    if( *ovCnt == 0 )
    {
        return(NULL);
    }

    /* ===============================================
     *  Evaluate all the slot override names and values
     *  and store them in a contiguous array
     *  =============================================== */
    ovs = (core_data_object *)core_mem_alloc_no_init(theEnv, (sizeof(core_data_object) * (*ovCnt)));
    ovi = 0;

    while( ovExprs != NULL )
    {
        if( core_eval_expression(theEnv, ovExprs, &ovs[ovi]))
        {
            goto EvaluateOverridesError;
        }

        if( ovs[ovi].type != ATOM )
        {
            report_explicit_type_error(theEnv, to_string(core_get_expression_function_handle(core_get_evaluation_data(theEnv)->current_expression)),
                               ovi + 1, "slot name");
            core_set_eval_error(theEnv, TRUE);
            goto EvaluateOverridesError;
        }

        slotName = ovs[ovi].value;

        if( ovExprs->next_arg->args )
        {
            if( EvaluateAndStoreInDataObject(theEnv, FALSE, ovExprs->next_arg->args,
                                             &ovs[ovi], TRUE) == FALSE )
            {
                goto EvaluateOverridesError;
            }
        }
        else
        {
            core_set_data_ptr_start(&ovs[ovi], 1);
            core_set_data_ptr_end(&ovs[ovi], 0);
            core_set_pointer_type(&ovs[ovi], LIST);
            core_set_pointer_value(&ovs[ovi], core_get_function_primitive_data(theEnv)->null_arg_value);
        }

        ovs[ovi].metadata = slotName;
        ovExprs = ovExprs->next_arg->next_arg;
        ovs[ovi].next = (ovExprs != NULL) ? &ovs[ovi + 1] : NULL;
        ovi++;
    }

    return(ovs);

EvaluateOverridesError:
    core_mem_release(theEnv, (void *)ovs, (sizeof(core_data_object) * (*ovCnt)));
    *error = TRUE;
    return(NULL);
}

/**********************************************************
 *  NAME         : DeleteSlotOverrideEvaluations
 *  DESCRIPTION  : Deallocates slot override evaluation array
 *  INPUTS       : 1) The data object array
 *              2) The number of elements
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Deallocates slot override data object
 *              array for modify- and duplicate- instance
 *  NOTES        : None
 **********************************************************/
static void DeleteSlotOverrideEvaluations(void *theEnv, core_data_object *ovEvals, int ovCnt)
{
    if( ovEvals != NULL )
    {
        core_mem_release(theEnv, (void *)ovEvals, (sizeof(core_data_object) * ovCnt));
    }
}

/**********************************************************
 *  NAME         : ModifyMsgHandlerSupport
 *  DESCRIPTION  : Support routine for DirectModifyMsgHandler
 *              and MsgModifyMsgHandler
 *
 *              Performs a series of slot updates
 *              directly or with messages
 *  INPUTS       : 1) A data object buffer to hold the result
 *              2) A flag indicating whether to use
 *                 put- messages or direct placement
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Slots updated (messages sent)
 *  NOTES        : None
 **********************************************************/
static void ModifyMsgHandlerSupport(void *theEnv, core_data_object *result, int msgpass)
{
    core_data_object *slotOverrides, *newval, temp, junk;
    core_expression_object msgExp;
    INSTANCE_TYPE *ins;
    INSTANCE_SLOT *insSlot;

    result->type = ATOM;
    result->value = get_false(theEnv);

    if( InstanceData(theEnv)->ObjectModDupMsgValid == FALSE )
    {
        error_print_id(theEnv, "INSMODDP", 1, FALSE);
        print_router(theEnv, WERROR, "Direct/message-modify message valid only in modify-instance.\n");
        core_set_eval_error(theEnv, TRUE);
        return;
    }

    InstanceData(theEnv)->ObjectModDupMsgValid = FALSE;

    ins = GetActiveInstance(theEnv);

    if( ins->garbage )
    {
        StaleInstanceAddress(theEnv, "modify-instance", 0);
        core_set_eval_error(theEnv, TRUE);
        return;
    }

    /* =======================================
     *  Retrieve the slot override data objects
     *  passed from ModifyInstance - the slot
     *  name is stored in the metadata
     *  field - and the next fields are links
     *  ======================================= */
    slotOverrides = (core_data_object *)GetNthMessageArgument(theEnv, 1)->value;

    while( slotOverrides != NULL )
    {
        /* ===========================================================
         *  No evaluation or error checking needs to be done
         *  since this has already been done by EvaluateSlotOverrides()
         *  =========================================================== */
        insSlot = FindInstanceSlot(theEnv, ins, (ATOM_HN *)slotOverrides->metadata);

        if( insSlot == NULL )
        {
            error_method_duplication(theEnv, to_string(slotOverrides->metadata), "modify-instance");
            core_set_eval_error(theEnv, TRUE);
            return;
        }

        if( msgpass )
        {
            msgExp.type = slotOverrides->type;

            if( msgExp.type != LIST )
            {
                msgExp.value = slotOverrides->value;
            }
            else
            {
                msgExp.value = (void *)slotOverrides;
            }

            msgExp.args = NULL;
            msgExp.next_arg = NULL;
            DirectMessage(theEnv, insSlot->desc->overrideMessage, ins, &temp, &msgExp);

            if( core_get_evaluation_data(theEnv)->eval_error ||
                ((temp.type == ATOM) && (temp.value == get_false(theEnv))))
            {
                return;
            }
        }
        else
        {
            if( insSlot->desc->multiple && (slotOverrides->type != LIST))
            {
                temp.type = LIST;
                temp.value = create_list(theEnv, 1L);
                core_set_data_start(temp, 1);
                core_set_data_end(temp, 1);
                set_list_node_type(temp.value, 1, (short)slotOverrides->type);
                set_list_node_value(temp.value, 1, slotOverrides->value);
                newval = &temp;
            }
            else
            {
                newval = slotOverrides;
            }

            if( PutSlotValue(theEnv, ins, insSlot, newval, &junk, "modify-instance") == FALSE )
            {
                return;
            }
        }

        slotOverrides = slotOverrides->next;
    }

    result->value = get_true(theEnv);
}

/*************************************************************
 *  NAME         : DuplicateMsgHandlerSupport
 *  DESCRIPTION  : Support routine for DirectDuplicateMsgHandler
 *              and MsgDuplicateMsgHandler
 *
 *              Performs a series of slot updates
 *              directly or with messages
 *  INPUTS       : 1) A data object buffer to hold the result
 *              2) A flag indicating whether to use
 *                 put- messages or direct placement
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Slots updated (messages sent)
 *  NOTES        : None
 *************************************************************/
static void DuplicateMsgHandlerSupport(void *theEnv, core_data_object *result, int msgpass)
{
    INSTANCE_TYPE *srcins, *dstins;
    ATOM_HN *newName;
    core_data_object *slotOverrides;
    core_expression_object *valArg, msgExp;
    long i;
    int oldMkInsMsgPass;
    INSTANCE_SLOT *dstInsSlot;
    core_data_object temp, junk, *newval;

    result->type = ATOM;
    result->value = get_false(theEnv);

    if( InstanceData(theEnv)->ObjectModDupMsgValid == FALSE )
    {
        error_print_id(theEnv, "INSMODDP", 2, FALSE);
        print_router(theEnv, WERROR, "Direct/message-duplicate message valid only in duplicate-instance.\n");
        core_set_eval_error(theEnv, TRUE);
        return;
    }

    InstanceData(theEnv)->ObjectModDupMsgValid = FALSE;

    /* ==================================
     *  Grab the slot override expressions
     *  and determine the source instance
     *  and the name of the new instance
     *  ================================== */
    srcins = GetActiveInstance(theEnv);
    newName = (ATOM_HN *)GetNthMessageArgument(theEnv, 1)->value;
    slotOverrides = (core_data_object *)GetNthMessageArgument(theEnv, 2)->value;

    if( srcins->garbage )
    {
        StaleInstanceAddress(theEnv, "duplicate-instance", 0);
        core_set_eval_error(theEnv, TRUE);
        return;
    }

    if( newName == srcins->name )
    {
        error_print_id(theEnv, "INSMODDP", 3, FALSE);
        print_router(theEnv, WERROR, "Instance copy must have a different name in duplicate-instance.\n");
        core_set_eval_error(theEnv, TRUE);
        return;
    }

    /* ==========================================
     *  Create an uninitialized new instance of
     *  the new name (delete old version - if any)
     *  ========================================== */
    oldMkInsMsgPass = InstanceData(theEnv)->MkInsMsgPass;
    InstanceData(theEnv)->MkInsMsgPass = msgpass;
    dstins = BuildInstance(theEnv, newName, srcins->cls, TRUE);
    InstanceData(theEnv)->MkInsMsgPass = oldMkInsMsgPass;

    if( dstins == NULL )
    {
        return;
    }

    dstins->busy++;

    /* ================================
     *  Place slot overrides directly or
     *  with put- messages
     *  ================================ */
    while( slotOverrides != NULL )
    {
        /* ===========================================================
         *  No evaluation or error checking needs to be done
         *  since this has already been done by EvaluateSlotOverrides()
         *  =========================================================== */
        dstInsSlot = FindInstanceSlot(theEnv, dstins, (ATOM_HN *)slotOverrides->metadata);

        if( dstInsSlot == NULL )
        {
            error_method_duplication(theEnv, to_string(slotOverrides->metadata),
                           "duplicate-instance");
            goto DuplicateError;
        }

        if( msgpass )
        {
            msgExp.type = slotOverrides->type;

            if( msgExp.type != LIST )
            {
                msgExp.value = slotOverrides->value;
            }
            else
            {
                msgExp.value = (void *)slotOverrides;
            }

            msgExp.args = NULL;
            msgExp.next_arg = NULL;
            DirectMessage(theEnv, dstInsSlot->desc->overrideMessage, dstins, &temp, &msgExp);

            if( core_get_evaluation_data(theEnv)->eval_error ||
                ((temp.type == ATOM) && (temp.value == get_false(theEnv))))
            {
                goto DuplicateError;
            }
        }
        else
        {
            if( dstInsSlot->desc->multiple && (slotOverrides->type != LIST))
            {
                temp.type = LIST;
                temp.value = create_list(theEnv, 1L);
                core_set_data_start(temp, 1);
                core_set_data_end(temp, 1);
                set_list_node_type(temp.value, 1, (short)slotOverrides->type);
                set_list_node_value(temp.value, 1, slotOverrides->value);
                newval = &temp;
            }
            else
            {
                newval = slotOverrides;
            }

            if( PutSlotValue(theEnv, dstins, dstInsSlot, newval, &junk, "duplicate-instance") == FALSE )
            {
                goto DuplicateError;
            }
        }

        dstInsSlot->override = TRUE;
        slotOverrides = slotOverrides->next;
    }

    /* =======================================
     *  Copy values from source instance to new
     *  directly or with put- messages
     *  ======================================= */
    for( i = 0 ; i < dstins->cls->localInstanceSlotCount ; i++ )
    {
        if( dstins->slots[i].override == FALSE )
        {
            if( msgpass )
            {
                temp.type = (unsigned short)srcins->slots[i].type;
                temp.value = srcins->slots[i].value;

                if( temp.type == LIST )
                {
                    core_set_data_start(temp, 1);
                    core_set_data_end(temp, get_list_length(temp.value));
                }

                valArg = core_convert_data_to_expression(theEnv, &temp);
                DirectMessage(theEnv, dstins->slots[i].desc->overrideMessage,
                              dstins, &temp, valArg);
                core_return_expression(theEnv, valArg);

                if( core_get_evaluation_data(theEnv)->eval_error ||
                    ((temp.type == ATOM) && (temp.value == get_false(theEnv))))
                {
                    goto DuplicateError;
                }
            }
            else
            {
                temp.type = (unsigned short)srcins->slots[i].type;
                temp.value = srcins->slots[i].value;

                if( srcins->slots[i].type == LIST )
                {
                    core_set_data_start(temp, 1);
                    core_set_data_end(temp, get_list_length(srcins->slots[i].value));
                }

                if( PutSlotValue(theEnv, dstins, &dstins->slots[i], &temp, &junk, "duplicate-instance")
                    == FALSE )
                {
                    goto DuplicateError;
                }
            }
        }
    }

    /* =======================================
     *  Send init message for message-duplicate
     *  ======================================= */
    if( msgpass )
    {
        for( i = 0 ; i < dstins->cls->instanceSlotCount ; i++ )
        {
            dstins->slotAddresses[i]->override = TRUE;
        }

        dstins->initializeInProgress = 1;
        DirectMessage(theEnv, MessageHandlerData(theEnv)->INIT_ATOM, dstins, result, NULL);
    }

    dstins->busy--;

    if( dstins->garbage )
    {
        result->type = ATOM;
        result->value = get_false(theEnv);
        core_set_eval_error(theEnv, TRUE);
    }
    else
    {
        result->type = INSTANCE_NAME;
        result->value = (void *)GetFullInstanceName(theEnv, dstins);
    }

    return;

DuplicateError:
    dstins->busy--;
    QuashInstance(theEnv, dstins);
    core_set_eval_error(theEnv, TRUE);
}

#endif
