/* Purpose:  Access routines for Instance List Slots   */

/* =========================================
 *****************************************
 *            EXTERNAL DEFINITIONS
 *  =========================================
 ***************************************** */
#include "setup.h"

#if OBJECT_SYSTEM

#include "core_arguments.h"
#include "core_environment.h"
#include "core_functions.h"
#include "funcs_instance.h"
#include "funcs_method.h"
#include "classes_methods_call.h"
#include "funcs_list.h"
#include "router.h"

#define __CLASSES_INSTANCES_LISTS_SOURCE__
#include "classes_instances_lists.h"

/* =========================================
 *****************************************
 *                CONSTANTS
 *  =========================================
 ***************************************** */
#define INSERT         0
#define REPLACE        1
#define DELETE_OP      2

/* =========================================
 *****************************************
 *   INTERNALLY VISIBLE FUNCTION HEADERS
 *  =========================================
 ***************************************** */

static INSTANCE_TYPE * CheckListSlotInstance(void *, char *);
static INSTANCE_SLOT * CheckListSlotModify(void *, int, char *, INSTANCE_TYPE *, core_expression_object *, long *, long *, core_data_object *);
static void            AssignSlotToDataObject(core_data_object *, INSTANCE_SLOT *);

/* =========================================
 *****************************************
 *       EXTERNALLY VISIBLE FUNCTIONS
 *  =========================================
 ***************************************** */


/***************************************************
 *  NAME         : SetupInstanceListCommands
 *  DESCRIPTION  : Defines function interfaces for
 *              manipulating instance multislots
 *  INPUTS       : None
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Functions defined to KB
 *  NOTES        : None
 ***************************************************/
globle void SetupInstanceListCommands(void *theEnv)
{
    /* ===================================
     *  Old version 5.1 compatibility names
     *  =================================== */
    core_define_function(theEnv, "direct-mv-replace", 'b', PTR_FN DirectMVReplaceCommand,
                       "DirectMVReplaceCommand", "4**wii");
    core_define_function(theEnv, "direct-mv-insert", 'b', PTR_FN DirectMVInsertCommand,
                       "DirectMVInsertCommand", "3**wi");
    core_define_function(theEnv, "direct-mv-delete", 'b', PTR_FN DirectMVDeleteCommand,
                       "DirectMVDeleteCommand", "33iw");
    core_define_function(theEnv, "mv-slot-replace", 'u', PTR_FN MVSlotReplaceCommand,
                       "MVSlotReplaceCommand", "5*uewii");
    core_define_function(theEnv, "mv-slot-insert", 'u', PTR_FN MVSlotInsertCommand,
                       "MVSlotInsertCommand", "4*uewi");
    core_define_function(theEnv, "mv-slot-delete", 'u', PTR_FN MVSlotDeleteCommand,
                       "MVSlotDeleteCommand", "44iew");

    /* =====================
     *  New version 6.0 names
     *  ===================== */
    core_define_function(theEnv, "slot-direct-replace$", 'b', PTR_FN DirectMVReplaceCommand,
                       "DirectMVReplaceCommand", "4**wii");
    core_define_function(theEnv, "slot-direct-insert$", 'b', PTR_FN DirectMVInsertCommand,
                       "DirectMVInsertCommand", "3**wi");
    core_define_function(theEnv, "slot-direct-delete$", 'b', PTR_FN DirectMVDeleteCommand,
                       "DirectMVDeleteCommand", "33iw");
    core_define_function(theEnv, "slot-replace$", 'u', PTR_FN MVSlotReplaceCommand,
                       "MVSlotReplaceCommand", "5*uewii");
    core_define_function(theEnv, "slot-insert$", 'u', PTR_FN MVSlotInsertCommand,
                       "MVSlotInsertCommand", "4*uewi");
    core_define_function(theEnv, "slot-delete$", 'u', PTR_FN MVSlotDeleteCommand,
                       "MVSlotDeleteCommand", "44iew");
}

/***********************************************************************************
 *  NAME         : MVSlotReplaceCommand
 *  DESCRIPTION  : Allows user to replace a specified field of a multi-value slot
 *              The slot is directly read (w/o a get- message) and the new
 *                slot-value is placed via a put- message.
 *              This function is not valid for single-value slots.
 *  INPUTS       : Caller's result buffer
 *  RETURNS      : TRUE if multi-value slot successfully modified,
 *              FALSE otherwise
 *  SIDE EFFECTS : Put messsage sent for slot
 *  NOTES        : H/L Syntax : (slot-replace$ <instance> <slot>
 *                              <range-begin> <range-end> <value>)
 ***********************************************************************************/
globle void MVSlotReplaceCommand(void *theEnv, core_data_object *result)
{
    core_data_object newval, newseg, oldseg;
    INSTANCE_TYPE *ins;
    INSTANCE_SLOT *sp;
    long rb, re;
    core_expression_object arg;

    result->type = ATOM;
    result->value = get_false(theEnv);
    ins = CheckListSlotInstance(theEnv, "slot-replace$");

    if( ins == NULL )
    {
        return;
    }

    sp = CheckListSlotModify(theEnv, REPLACE, "slot-replace$", ins,
                             core_get_first_arg()->next_arg, &rb, &re, &newval);

    if( sp == NULL )
    {
        return;
    }

    AssignSlotToDataObject(&oldseg, sp);

    if( ReplaceMultiValueField(theEnv, &newseg, &oldseg, rb, re, &newval, "slot-replace$") == FALSE )
    {
        return;
    }

    arg.type = LIST;
    arg.value = (void *)&newseg;
    arg.next_arg = NULL;
    arg.args = NULL;
    DirectMessage(theEnv, sp->desc->overrideMessage, ins, result, &arg);
}

/***********************************************************************************
 *  NAME         : MVSlotInsertCommand
 *  DESCRIPTION  : Allows user to insert a specified field of a multi-value slot
 *              The slot is directly read (w/o a get- message) and the new
 *                slot-value is placed via a put- message.
 *              This function is not valid for single-value slots.
 *  INPUTS       : Caller's result buffer
 *  RETURNS      : TRUE if multi-value slot successfully modified, FALSE otherwise
 *  SIDE EFFECTS : Put messsage sent for slot
 *  NOTES        : H/L Syntax : (slot-insert$ <instance> <slot> <index> <value>)
 ***********************************************************************************/
globle void MVSlotInsertCommand(void *theEnv, core_data_object *result)
{
    core_data_object newval, newseg, oldseg;
    INSTANCE_TYPE *ins;
    INSTANCE_SLOT *sp;
    long theIndex;
    core_expression_object arg;

    result->type = ATOM;
    result->value = get_false(theEnv);
    ins = CheckListSlotInstance(theEnv, "slot-insert$");

    if( ins == NULL )
    {
        return;
    }

    sp = CheckListSlotModify(theEnv, INSERT, "slot-insert$", ins,
                             core_get_first_arg()->next_arg, &theIndex, NULL, &newval);

    if( sp == NULL )
    {
        return;
    }

    AssignSlotToDataObject(&oldseg, sp);

    if( InsertMultiValueField(theEnv, &newseg, &oldseg, theIndex, &newval, "slot-insert$") == FALSE )
    {
        return;
    }

    arg.type = LIST;
    arg.value = (void *)&newseg;
    arg.next_arg = NULL;
    arg.args = NULL;
    DirectMessage(theEnv, sp->desc->overrideMessage, ins, result, &arg);
}

/***********************************************************************************
 *  NAME         : MVSlotDeleteCommand
 *  DESCRIPTION  : Allows user to delete a specified field of a multi-value slot
 *              The slot is directly read (w/o a get- message) and the new
 *                slot-value is placed via a put- message.
 *              This function is not valid for single-value slots.
 *  INPUTS       : Caller's result buffer
 *  RETURNS      : TRUE if multi-value slot successfully modified, FALSE otherwise
 *  SIDE EFFECTS : Put message sent for slot
 *  NOTES        : H/L Syntax : (slot-delete$ <instance> <slot>
 *                              <range-begin> <range-end>)
 ***********************************************************************************/
globle void MVSlotDeleteCommand(void *theEnv, core_data_object *result)
{
    core_data_object newseg, oldseg;
    INSTANCE_TYPE *ins;
    INSTANCE_SLOT *sp;
    long rb, re;
    core_expression_object arg;

    result->type = ATOM;
    result->value = get_false(theEnv);
    ins = CheckListSlotInstance(theEnv, "slot-delete$");

    if( ins == NULL )
    {
        return;
    }

    sp = CheckListSlotModify(theEnv, DELETE_OP, "slot-delete$", ins,
                             core_get_first_arg()->next_arg, &rb, &re, NULL);

    if( sp == NULL )
    {
        return;
    }

    AssignSlotToDataObject(&oldseg, sp);

    if( DeleteMultiValueField(theEnv, &newseg, &oldseg, rb, re, "slot-delete$") == FALSE )
    {
        return;
    }

    arg.type = LIST;
    arg.value = (void *)&newseg;
    arg.next_arg = NULL;
    arg.args = NULL;
    DirectMessage(theEnv, sp->desc->overrideMessage, ins, result, &arg);
}

/*****************************************************************
 *  NAME         : DirectMVReplaceCommand
 *  DESCRIPTION  : Directly replaces a slot's value
 *  INPUTS       : None
 *  RETURNS      : TRUE if put OK, FALSE otherwise
 *  SIDE EFFECTS : Slot modified
 *  NOTES        : H/L Syntax: (direct-slot-replace$ <slot>
 *                             <range-begin> <range-end> <value>)
 *****************************************************************/
globle BOOLEAN DirectMVReplaceCommand(void *theEnv)
{
    INSTANCE_SLOT *sp;
    INSTANCE_TYPE *ins;
    long rb, re;
    core_data_object newval, newseg, oldseg;

    if( CheckCurrentMessage(theEnv, "direct-slot-replace$", TRUE) == FALSE )
    {
        return(FALSE);
    }

    ins = GetActiveInstance(theEnv);
    sp = CheckListSlotModify(theEnv, REPLACE, "direct-slot-replace$", ins,
                             core_get_first_arg(), &rb, &re, &newval);

    if( sp == NULL )
    {
        return(FALSE);
    }

    AssignSlotToDataObject(&oldseg, sp);

    if( ReplaceMultiValueField(theEnv, &newseg, &oldseg, rb, re, &newval, "direct-slot-replace$")
        == FALSE )
    {
        return(FALSE);
    }

    if( PutSlotValue(theEnv, ins, sp, &newseg, &newval, "function direct-slot-replace$"))
    {
        return(TRUE);
    }

    return(FALSE);
}

/************************************************************************
 *  NAME         : DirectMVInsertCommand
 *  DESCRIPTION  : Directly inserts a slot's value
 *  INPUTS       : None
 *  RETURNS      : TRUE if put OK, FALSE otherwise
 *  SIDE EFFECTS : Slot modified
 *  NOTES        : H/L Syntax: (direct-slot-insert$ <slot> <index> <value>)
 ************************************************************************/
globle BOOLEAN DirectMVInsertCommand(void *theEnv)
{
    INSTANCE_SLOT *sp;
    INSTANCE_TYPE *ins;
    long theIndex;
    core_data_object newval, newseg, oldseg;

    if( CheckCurrentMessage(theEnv, "direct-slot-insert$", TRUE) == FALSE )
    {
        return(FALSE);
    }

    ins = GetActiveInstance(theEnv);
    sp = CheckListSlotModify(theEnv, INSERT, "direct-slot-insert$", ins,
                             core_get_first_arg(), &theIndex, NULL, &newval);

    if( sp == NULL )
    {
        return(FALSE);
    }

    AssignSlotToDataObject(&oldseg, sp);

    if( InsertMultiValueField(theEnv, &newseg, &oldseg, theIndex, &newval, "direct-slot-insert$")
        == FALSE )
    {
        return(FALSE);
    }

    if( PutSlotValue(theEnv, ins, sp, &newseg, &newval, "function direct-slot-insert$"))
    {
        return(TRUE);
    }

    return(FALSE);
}

/*****************************************************************
 *  NAME         : DirectMVDeleteCommand
 *  DESCRIPTION  : Directly deletes a slot's value
 *  INPUTS       : None
 *  RETURNS      : TRUE if put OK, FALSE otherwise
 *  SIDE EFFECTS : Slot modified
 *  NOTES        : H/L Syntax: (direct-slot-delete$ <slot>
 *                             <range-begin> <range-end>)
 *****************************************************************/
globle BOOLEAN DirectMVDeleteCommand(void *theEnv)
{
    INSTANCE_SLOT *sp;
    INSTANCE_TYPE *ins;
    long rb, re;
    core_data_object newseg, oldseg;

    if( CheckCurrentMessage(theEnv, "direct-slot-delete$", TRUE) == FALSE )
    {
        return(FALSE);
    }

    ins = GetActiveInstance(theEnv);
    sp = CheckListSlotModify(theEnv, DELETE_OP, "direct-slot-delete$", ins,
                             core_get_first_arg(), &rb, &re, NULL);

    if( sp == NULL )
    {
        return(FALSE);
    }

    AssignSlotToDataObject(&oldseg, sp);

    if( DeleteMultiValueField(theEnv, &newseg, &oldseg, rb, re, "direct-slot-delete$")
        == FALSE )
    {
        return(FALSE);
    }

    if( PutSlotValue(theEnv, ins, sp, &newseg, &oldseg, "function direct-slot-delete$"))
    {
        return(TRUE);
    }

    return(FALSE);
}

/* =========================================
 *****************************************
 *       INTERNALLY VISIBLE FUNCTIONS
 *  =========================================
 ***************************************** */

/**********************************************************************
 *  NAME         : CheckListSlotInstance
 *  DESCRIPTION  : Gets the instance for the functions slot-replace$,
 *                 insert and delete
 *  INPUTS       : The function name
 *  RETURNS      : The instance address, NULL on errors
 *  SIDE EFFECTS : None
 *  NOTES        : None
 **********************************************************************/
static INSTANCE_TYPE *CheckListSlotInstance(void *theEnv, char *func)
{
    INSTANCE_TYPE *ins;
    core_data_object temp;

    if( core_check_arg_type(theEnv, func, 1, INSTANCE_OR_INSTANCE_NAME, &temp) == FALSE )
    {
        core_set_eval_error(theEnv, TRUE);
        return(NULL);
    }

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
    else
    {
        ins = FindInstanceBySymbol(theEnv, (ATOM_HN *)temp.value);

        if( ins == NULL )
        {
            NoInstanceError(theEnv, to_string(temp.value), func);
        }
    }

    return(ins);
}

/*********************************************************************
 *  NAME         : CheckListSlotModify
 *  DESCRIPTION  : For the functions slot-replace$, insert, & delete
 *                 as well as direct-slot-replace$, insert, & delete
 *                 this function gets the slot, index, and optional
 *                 field-value for these functions
 *  INPUTS       : 1) A code indicating the type of operation
 *                   INSERT    (0) : Requires one index
 *                   REPLACE   (1) : Requires two indices
 *                   DELETE_OP (2) : Requires two indices
 *              2) Function name-string
 *              3) Instance address
 *              4) Argument expression chain
 *              5) Caller's buffer for index (or beginning of range)
 *              6) Caller's buffer for end of range
 *                  (can be NULL for INSERT)
 *              7) Caller's new-field value buffer
 *                  (can be NULL for DELETE_OP)
 *  RETURNS      : The address of the instance-slot,
 *                 NULL on errors
 *  SIDE EFFECTS : Caller's index buffer set
 *              Caller's new-field value buffer set (if not NULL)
 *                Will allocate an is_ephemeral segment to store more
 *                  than 1 new field value
 *              eval_error set on errors
 *  NOTES        : Assume the argument chain is at least 2
 *                expressions deep - slot, index, and optional values
 *********************************************************************/
static INSTANCE_SLOT *CheckListSlotModify(void *theEnv, int code, char *func, INSTANCE_TYPE *ins, core_expression_object *args, long *rb, long *re, core_data_object *newval)
{
    core_data_object temp;
    INSTANCE_SLOT *sp;
    int start;

    start = (args == core_get_first_arg()) ? 1 : 2;
    core_get_evaluation_data(theEnv)->eval_error = FALSE;
    core_eval_expression(theEnv, args, &temp);

    if( temp.type != ATOM )
    {
        report_explicit_type_error(theEnv, func, start, "symbol");
        core_set_eval_error(theEnv, TRUE);
        return(NULL);
    }

    sp = FindInstanceSlot(theEnv, ins, (ATOM_HN *)temp.value);

    if( sp == NULL )
    {
        error_method_duplication(theEnv, to_string(temp.value), func);
        return(NULL);
    }

    if( sp->desc->multiple == 0 )
    {
        error_print_id(theEnv, "INSMULT", 1, FALSE);
        print_router(theEnv, WERROR, "Function ");
        print_router(theEnv, WERROR, func);
        print_router(theEnv, WERROR, " cannot be used on single-field slot ");
        print_router(theEnv, WERROR, to_string(sp->desc->slotName->name));
        print_router(theEnv, WERROR, " in instance ");
        print_router(theEnv, WERROR, to_string(ins->name));
        print_router(theEnv, WERROR, ".\n");
        core_set_eval_error(theEnv, TRUE);
        return(NULL);
    }

    core_eval_expression(theEnv, args->next_arg, &temp);

    if( temp.type != INTEGER )
    {
        report_explicit_type_error(theEnv, func, start + 1, "integer");
        core_set_eval_error(theEnv, TRUE);
        return(NULL);
    }

    args = args->next_arg->next_arg;
    *rb = (long)to_long(temp.value);

    if((code == REPLACE) || (code == DELETE_OP))
    {
        core_eval_expression(theEnv, args, &temp);

        if( temp.type != INTEGER )
        {
            report_explicit_type_error(theEnv, func, start + 2, "integer");
            core_set_eval_error(theEnv, TRUE);
            return(NULL);
        }

        *re = (long)to_long(temp.value);
        args = args->next_arg;
    }

    if((code == INSERT) || (code == REPLACE))
    {
        if( EvaluateAndStoreInDataObject(theEnv, 1, args, newval, TRUE) == FALSE )
        {
            return(NULL);
        }
    }

    return(sp);
}

/***************************************************
 *  NAME         : AssignSlotToDataObject
 *  DESCRIPTION  : Assigns the value of a list
 *              slot to a data object
 *  INPUTS       : 1) The data object buffer
 *              2) The instance slot
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Data object fields set
 *  NOTES        : Assumes slot is a multislot
 ***************************************************/
static void AssignSlotToDataObject(core_data_object *theDataObject, INSTANCE_SLOT *theSlot)
{
    theDataObject->type = (unsigned short)theSlot->type;
    theDataObject->value = theSlot->value;
    theDataObject->begin = 0;
    core_set_data_ptr_end(theDataObject, GetInstanceSlotLength(theSlot));
}

#endif

/***************************************************
 *  NAME         :
 *  DESCRIPTION  :
 *  INPUTS       :
 *  RETURNS      :
 *  SIDE EFFECTS :
 *  NOTES        :
 ***************************************************/
