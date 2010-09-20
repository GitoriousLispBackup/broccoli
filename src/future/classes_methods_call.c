/* =========================================
 *****************************************
 *            EXTERNAL DEFINITIONS
 *  =========================================
 ***************************************** */
#include "setup.h"

#if OBJECT_SYSTEM

#ifndef _STDIO_INCLUDED_
#include <stdio.h>
#define _STDIO_INCLUDED_
#endif
#include <stdlib.h>

#include "core_arguments.h"
#include "classes_kernel.h"
#include "funcs_class.h"
#include "core_memory.h"
#include "core_constructs.h"
#include "core_environment.h"
#include "parser_expressions.h"
#include "funcs_instance.h"
#include "classes_methods_kernel.h"
#include "funcs_method.h"
#include "type_list.h"
#include "funcs_function.h"
#include "core_functions_util.h"
#include "funcs_profiling.h"
#include "router.h"
#include "funcs_string.h"
#include "core_gc.h"
#include "core_command_prompt.h"

#define __CLASSES_METHODS_CALL_SOURCE__
#include "classes_methods_call.h"

#include "classes_instances_kernel.h"

/* =========================================
 *****************************************
 *   INTERNALLY VISIBLE FUNCTION HEADERS
 *  =========================================
 ***************************************** */

static void           PerformMessage(void *, core_data_object *, core_expression_object *, ATOM_HN *);
static HANDLER_LINK * FindApplicableHandlers(void *, DEFCLASS *, ATOM_HN *);
static void           CallHandlers(void *, core_data_object *);
static void           EarlySlotBindError(void *, INSTANCE_TYPE *, DEFCLASS *, unsigned);

/* =========================================
 *****************************************
 *       EXTERNALLY VISIBLE FUNCTIONS
 *  =========================================
 ***************************************** */

/*****************************************************
 *  NAME         : DirectMessage
 *  DESCRIPTION  : Plugs in given instance and
 *               performs specified message
 *  INPUTS       : 1) Message symbolic name
 *              2) The instance address
 *              3) Address of core_data_object buffer
 *                 (NULL if don't care)
 *              4) Message argument expressions
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Side effects of message execution
 *  NOTES        : None
 *****************************************************/
globle void DirectMessage(void *theEnv, ATOM_HN *msg, INSTANCE_TYPE *ins, core_data_object *resultbuf, core_expression_object *remargs)
{
    core_expression_object args;
    core_data_object temp;

    if( resultbuf == NULL )
    {
        resultbuf = &temp;
    }

    args.next_arg = remargs;
    args.args = NULL;
    args.type = INSTANCE_ADDRESS;
    args.value = (void *)ins;
    PerformMessage(theEnv, resultbuf, &args, msg);
}

/***************************************************
 *  NAME         : EnvSend
 *  DESCRIPTION  : C Interface for sending messages to
 *               instances
 *  INPUTS       : 1) The data object of the instance
 *              2) The message name-string
 *              3) The message arguments string
 *                 (Constants only)
 *              4) Caller's buffer for result
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Executes message and stores result
 *                caller's buffer
 *  NOTES        : None
 ***************************************************/
globle void EnvSend(void *theEnv, core_data_object *idata, char *msg, char *args, core_data_object *result)
{
    int error;
    core_expression_object *iexp;
    ATOM_HN *msym;

    if((core_get_evaluation_data(theEnv)->eval_depth == 0) && (!CommandLineData(theEnv)->EvaluatingTopLevelCommand) &&
       (core_get_evaluation_data(theEnv)->current_expression == NULL))
    {
        core_gc_periodic_cleanup(theEnv, TRUE, FALSE);
    }

    core_set_eval_error(theEnv, FALSE);
    result->type = ATOM;
    result->value = get_false(theEnv);
    msym = lookup_atom(theEnv, msg);

    if( msym == NULL )
    {
        PrintNoHandlerError(theEnv, msg);
        core_set_eval_error(theEnv, TRUE);
        return;
    }

    iexp = core_generate_constant(theEnv, idata->type, idata->value);
    iexp->next_arg = parse_constants(theEnv, args, &error);

    if( error == TRUE )
    {
        core_return_expression(theEnv, iexp);
        core_set_eval_error(theEnv, TRUE);
        return;
    }

    PerformMessage(theEnv, result, iexp, msym);
    core_return_expression(theEnv, iexp);
}

/*****************************************************
 *  NAME         : DestroyHandlerLinks
 *  DESCRIPTION  : Iteratively deallocates handler-links
 *  INPUTS       : The handler-link list
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Deallocation of links
 *  NOTES        : None
 *****************************************************/
globle void DestroyHandlerLinks(void *theEnv, HANDLER_LINK *mhead)
{
    HANDLER_LINK *tmp;

    while( mhead != NULL )
    {
        tmp = mhead;
        mhead = mhead->nxt;
        tmp->hnd->busy--;
        DecrementDefclassBusyCount(theEnv, (void *)tmp->hnd->cls);
        core_mem_return_struct(theEnv, messageHandlerLink, tmp);
    }
}

/***********************************************************************
 *  NAME         : SendCommand
 *  DESCRIPTION  : Determines the applicable handler(s) and sets up the
 *                core calling frame.  Then calls the core frame.
 *  INPUTS       : Caller's space for storing the result of the handler(s)
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Any side-effects caused by the execution of handlers in
 *                the core framework
 *  NOTES        : H/L Syntax : (send <instance> <hnd> <args>*)
 ***********************************************************************/
globle void SendCommand(void *theEnv, core_data_object *result)
{
    core_expression_object args;
    ATOM_HN *msg;
    core_data_object temp;

    result->type = ATOM;
    result->value = get_false(theEnv);

    if( core_check_arg_type(theEnv, "send", 2, ATOM, &temp) == FALSE )
    {
        return;
    }

    msg = (ATOM_HN *)temp.value;

    /* =============================================
     *  Get the instance or primitive for the message
     *  ============================================= */
    args.type = core_get_first_arg()->type;
    args.value = core_get_first_arg()->value;
    args.args = core_get_first_arg()->args;
    args.next_arg = core_get_first_arg()->next_arg->next_arg;

    PerformMessage(theEnv, result, &args, msg);
}

/***************************************************
 *  NAME         : GetNthMessageArgument
 *  DESCRIPTION  : Returns the address of the nth
 *              (starting at 1) which is an
 *              argument of the current message
 *              dispatch
 *  INPUTS       : None
 *  RETURNS      : The message argument
 *  SIDE EFFECTS : None
 *  NOTES        : The active instance is always
 *              stored as the first argument (0) in
 *              the call frame of the message
 ***************************************************/
globle core_data_object *GetNthMessageArgument(void *theEnv, int n)
{
    return(&core_get_function_primitive_data(theEnv)->arguments[n]);
}

/*****************************************************
 *  NAME         : NextHandlerAvailable
 *  DESCRIPTION  : Determines if there the currently
 *                executing handler can call a
 *                shadowed handler
 *              Used before calling call-next-handler
 *  INPUTS       : None
 *  RETURNS      : TRUE if shadow ready, FALSE otherwise
 *  SIDE EFFECTS : None
 *  NOTES        : H/L Syntax: (next-handlerp)
 *****************************************************/
globle int NextHandlerAvailable(void *theEnv)
{
    if( MessageHandlerData(theEnv)->CurrentCore == NULL )
    {
        return(FALSE);
    }

    if( MessageHandlerData(theEnv)->CurrentCore->hnd->type == MAROUND )
    {
        return((MessageHandlerData(theEnv)->NextInCore != NULL) ? TRUE : FALSE);
    }

    if((MessageHandlerData(theEnv)->CurrentCore->hnd->type == MPRIMARY) && (MessageHandlerData(theEnv)->NextInCore != NULL))
    {
        return((MessageHandlerData(theEnv)->NextInCore->hnd->type == MPRIMARY) ? TRUE : FALSE);
    }

    return(FALSE);
}

/********************************************************
 *  NAME         : CallNextHandler
 *  DESCRIPTION  : This function allows around-handlers
 *                to execute the rest of the core frame.
 *              It also allows primary handlers
 *                to execute shadowed primaries.
 *
 *              The original handler arguments are
 *                left intact.
 *  INPUTS       : The caller's result-value buffer
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : The core frame is called and any
 *                appropriate changes are made when
 *                used in an around handler
 *                See CallHandlers()
 *              But when call-next-handler is called
 *                from a primary, the same shadowed
 *                primary is called over and over
 *                again for repeated calls to
 *                call-next-handler.
 *  NOTES        : H/L Syntax: (call-next-handler) OR
 *                 (override-next-handler <arg> ...)
 ********************************************************/
globle void CallNextHandler(void *theEnv, core_data_object *result)
{
    core_expression_object args;
    int overridep;
    HANDLER_LINK *oldNext, *oldCurrent;

#if PROFILING_FUNCTIONS
    struct profileFrameInfo profileFrame;
#endif

    core_set_pointer_type(result, ATOM);
    core_set_pointer_value(result, get_false(theEnv));
    core_get_evaluation_data(theEnv)->eval_error = FALSE;

    if( core_get_evaluation_data(theEnv)->halt )
    {
        return;
    }

    if( NextHandlerAvailable(theEnv) == FALSE )
    {
        error_print_id(theEnv, "MSGPASS", 1, FALSE);
        print_router(theEnv, WERROR, "Shadowed message-handlers not applicable in current context.\n");
        core_set_eval_error(theEnv, TRUE);
        return;
    }

    if( core_get_evaluation_data(theEnv)->current_expression->value == (void *)core_lookup_function(theEnv, "override-next-handler"))
    {
        overridep = 1;
        args.type = core_get_function_primitive_data(theEnv)->arguments[0].type;

        if( args.type != LIST )
        {
            args.value = (void *)core_get_function_primitive_data(theEnv)->arguments[0].value;
        }
        else
        {
            args.value = (void *)&core_get_function_primitive_data(theEnv)->arguments[0];
        }

        args.next_arg = core_get_first_arg();
        args.args = NULL;
        core_push_function_args(theEnv, &args, core_count_args(&args),
                           to_string(MessageHandlerData(theEnv)->CurrentMessageName), "message",
                           UnboundHandlerErr);

        if( core_get_evaluation_data(theEnv)->eval_error )
        {
            get_flow_control_data(theEnv)->return_flag = FALSE;
            return;
        }
    }
    else
    {
        overridep = 0;
    }

    oldNext = MessageHandlerData(theEnv)->NextInCore;
    oldCurrent = MessageHandlerData(theEnv)->CurrentCore;

    if( MessageHandlerData(theEnv)->CurrentCore->hnd->type == MAROUND )
    {
        if( MessageHandlerData(theEnv)->NextInCore->hnd->type == MAROUND )
        {
            MessageHandlerData(theEnv)->CurrentCore = MessageHandlerData(theEnv)->NextInCore;
            MessageHandlerData(theEnv)->NextInCore = MessageHandlerData(theEnv)->NextInCore->nxt;
#if DEBUGGING_FUNCTIONS

            if( MessageHandlerData(theEnv)->CurrentCore->hnd->trace )
            {
                WatchHandler(theEnv, WTRACE, MessageHandlerData(theEnv)->CurrentCore, BEGIN_TRACE);
            }

#endif

            if( CheckHandlerArgCount(theEnv))
            {
#if PROFILING_FUNCTIONS
                StartProfile(theEnv, &profileFrame,
                             &MessageHandlerData(theEnv)->CurrentCore->hnd->ext_data,
                             ProfileFunctionData(theEnv)->ProfileConstructs);
#endif

                core_eval_function_actions(theEnv, MessageHandlerData(theEnv)->CurrentCore->hnd->cls->header.my_module->module_def,
                                    MessageHandlerData(theEnv)->CurrentCore->hnd->actions,
                                    MessageHandlerData(theEnv)->CurrentCore->hnd->localVarCount,
                                    result, UnboundHandlerErr);
#if PROFILING_FUNCTIONS
                EndProfile(theEnv, &profileFrame);
#endif
            }

#if DEBUGGING_FUNCTIONS

            if( MessageHandlerData(theEnv)->CurrentCore->hnd->trace )
            {
                WatchHandler(theEnv, WTRACE, MessageHandlerData(theEnv)->CurrentCore, END_TRACE);
            }

#endif
        }
        else
        {
            CallHandlers(theEnv, result);
        }
    }
    else
    {
        MessageHandlerData(theEnv)->CurrentCore = MessageHandlerData(theEnv)->NextInCore;
        MessageHandlerData(theEnv)->NextInCore = MessageHandlerData(theEnv)->NextInCore->nxt;
#if DEBUGGING_FUNCTIONS

        if( MessageHandlerData(theEnv)->CurrentCore->hnd->trace )
        {
            WatchHandler(theEnv, WTRACE, MessageHandlerData(theEnv)->CurrentCore, BEGIN_TRACE);
        }

#endif

        if( CheckHandlerArgCount(theEnv))
        {
#if PROFILING_FUNCTIONS
            StartProfile(theEnv, &profileFrame,
                         &MessageHandlerData(theEnv)->CurrentCore->hnd->ext_data,
                         ProfileFunctionData(theEnv)->ProfileConstructs);
#endif

            core_eval_function_actions(theEnv, MessageHandlerData(theEnv)->CurrentCore->hnd->cls->header.my_module->module_def,
                                MessageHandlerData(theEnv)->CurrentCore->hnd->actions,
                                MessageHandlerData(theEnv)->CurrentCore->hnd->localVarCount,
                                result, UnboundHandlerErr);
#if PROFILING_FUNCTIONS
            EndProfile(theEnv, &profileFrame);
#endif
        }

#if DEBUGGING_FUNCTIONS

        if( MessageHandlerData(theEnv)->CurrentCore->hnd->trace )
        {
            WatchHandler(theEnv, WTRACE, MessageHandlerData(theEnv)->CurrentCore, END_TRACE);
        }

#endif
    }

    MessageHandlerData(theEnv)->NextInCore = oldNext;
    MessageHandlerData(theEnv)->CurrentCore = oldCurrent;

    if( overridep )
    {
        core_pop_function_args(theEnv);
    }

    get_flow_control_data(theEnv)->return_flag = FALSE;
}

/*************************************************************************
 *  NAME         : FindApplicableOfName
 *  DESCRIPTION  : Groups all handlers of all types of the specified
 *                class of the specified name into the applicable handler
 *                list
 *  INPUTS       : 1) The class address
 *              2-3) The tops and bottoms of the four handler type lists:
 *                   around, before, primary and after
 *              4) The message name symbol
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Modifies the handler lists to include applicable handlers
 *  NOTES        : None
 *************************************************************************/
globle void FindApplicableOfName(void *theEnv, DEFCLASS *cls, HANDLER_LINK *tops[4], HANDLER_LINK *bots[4], ATOM_HN *mname)
{
    register int i;
    register int e;
    HANDLER *hnd;
    unsigned *arr;
    HANDLER_LINK *tmp;

    i = FindHandlerNameGroup(cls, mname);

    if( i == -1 )
    {
        return;
    }

    e = ((int)cls->handlerCount) - 1;
    hnd = cls->handlers;
    arr = cls->handlerOrderMap;

    for(; i <= e ; i++ )
    {
        if( hnd[arr[i]].name != mname )
        {
            break;
        }

        tmp = core_mem_get_struct(theEnv, messageHandlerLink);
        hnd[arr[i]].busy++;
        IncrementDefclassBusyCount(theEnv, (void *)hnd[arr[i]].cls);
        tmp->hnd = &hnd[arr[i]];

        if( tops[tmp->hnd->type] == NULL )
        {
            tmp->nxt = NULL;
            tops[tmp->hnd->type] = bots[tmp->hnd->type] = tmp;
        }

        else if( tmp->hnd->type == MAFTER )
        {
            tmp->nxt = tops[tmp->hnd->type];
            tops[tmp->hnd->type] = tmp;
        }

        else
        {
            bots[tmp->hnd->type]->nxt = tmp;
            bots[tmp->hnd->type] = tmp;
            tmp->nxt = NULL;
        }
    }
}

/*************************************************************************
 *  NAME         : JoinHandlerLinks
 *  DESCRIPTION  : Joins the queues of different handlers together
 *  INPUTS       : 1-2) The tops and bottoms of the four handler type lists:
 *                   around, before, primary and after
 *              3) The message name symbol
 *  RETURNS      : The top of the joined lists, NULL on errors
 *  SIDE EFFECTS : Links all the handler type lists together, or all the
 *                lists are destroyed if there are no primary handlers
 *  NOTES        : None
 *************************************************************************/
globle HANDLER_LINK *JoinHandlerLinks(void *theEnv, HANDLER_LINK *tops[4], HANDLER_LINK *bots[4], ATOM_HN *mname)
{
    register int i;
    HANDLER_LINK *mlink;

    if( tops[MPRIMARY] == NULL )
    {
        PrintNoHandlerError(theEnv, to_string(mname));

        for( i = MAROUND ; i <= MAFTER ; i++ )
        {
            DestroyHandlerLinks(theEnv, tops[i]);
        }

        core_set_eval_error(theEnv, TRUE);
        return(NULL);
    }

    mlink = tops[MPRIMARY];

    if( tops[MBEFORE] != NULL )
    {
        bots[MBEFORE]->nxt = mlink;
        mlink = tops[MBEFORE];
    }

    if( tops[MAROUND] != NULL )
    {
        bots[MAROUND]->nxt = mlink;
        mlink = tops[MAROUND];
    }

    bots[MPRIMARY]->nxt = tops[MAFTER];

    return(mlink);
}

/***************************************************
 *  NAME         : PrintHandlerSlotGetFunction
 *  DESCRIPTION  : Developer access function for
 *              printing direct slot references
 *              in message-handlers
 *  INPUTS       : 1) The logical name of the output
 *              2) The bitmap expression
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Expression printed
 *  NOTES        : None
 ***************************************************/
#if WIN_BTC && (!DEVELOPER)
#pragma argsused
#endif
globle void PrintHandlerSlotGetFunction(void *theEnv, char *logical_name, void *theValue)
{
#if DEVELOPER
    HANDLER_SLOT_REFERENCE *theReference;
    DEFCLASS *theDefclass;
    SLOT_DESC *sd;

    theReference = (HANDLER_SLOT_REFERENCE *)to_bitmap(theValue);
    print_router(theEnv, logical_name, "?self:[");
    theDefclass = DefclassData(theEnv)->ClassIDMap[theReference->classID];
    print_router(theEnv, logical_name, to_string(theDefclass->header.name));
    print_router(theEnv, logical_name, "]");
    sd = theDefclass->instanceTemplate[theDefclass->slotNameMap[theReference->slotID] - 1];
    print_router(theEnv, logical_name, to_string(sd->slotName->name));
#else
#if MAC_MCW || WIN_MCW || MAC_XCD
#pragma unused(theEnv)
#pragma unused(logicalName)
#pragma unused(theValue)
#endif
#endif
}

/***************************************************
 *  NAME         : HandlerSlotGetFunction
 *  DESCRIPTION  : Access function for handling the
 *              statically-bound direct slot
 *              references in message-handlers
 *  INPUTS       : 1) The bitmap expression
 *              2) A data object buffer
 *  RETURNS      : TRUE if OK, FALSE
 *              on errors
 *  SIDE EFFECTS : Data object buffer gets value of
 *              slot. On errors, buffer gets
 *              symbol FALSE, eval_error
 *              is set and error messages are
 *              printed
 *  NOTES        : It is possible for a handler
 *              (attached to a superclass of
 *               the currently active instance)
 *              containing these static references
 *              to be called for an instance
 *              which does not contain the slots
 *              (e.g., an instance of a subclass
 *               where the original slot was
 *               no-inherit or the subclass
 *               overrode the original slot)
 ***************************************************/
globle BOOLEAN HandlerSlotGetFunction(void *theEnv, void *theValue, core_data_object *theResult)
{
    HANDLER_SLOT_REFERENCE *theReference;
    DEFCLASS *theDefclass;
    INSTANCE_TYPE *theInstance;
    INSTANCE_SLOT *sp;
    unsigned instanceSlotIndex;

    theReference = (HANDLER_SLOT_REFERENCE *)to_bitmap(theValue);
    theInstance = (INSTANCE_TYPE *)core_get_function_primitive_data(theEnv)->arguments[0].value;
    theDefclass = DefclassData(theEnv)->ClassIDMap[theReference->classID];

    if( theInstance->garbage )
    {
        StaleInstanceAddress(theEnv, "for slot get", 0);
        theResult->type = ATOM;
        theResult->value = get_false(theEnv);
        core_set_eval_error(theEnv, TRUE);
        return(FALSE);
    }

    if( theInstance->cls == theDefclass )
    {
        instanceSlotIndex = theInstance->cls->slotNameMap[theReference->slotID];
        sp = theInstance->slotAddresses[instanceSlotIndex - 1];
    }
    else
    {
        if( theReference->slotID > theInstance->cls->maxSlotNameID )
        {
            goto HandlerGetError;
        }

        instanceSlotIndex = theInstance->cls->slotNameMap[theReference->slotID];

        if( instanceSlotIndex == 0 )
        {
            goto HandlerGetError;
        }

        instanceSlotIndex--;
        sp = theInstance->slotAddresses[instanceSlotIndex];

        if( sp->desc->cls != theDefclass )
        {
            goto HandlerGetError;
        }
    }

    theResult->type = (unsigned short)sp->type;
    theResult->value = sp->value;

    if( sp->type == LIST )
    {
        theResult->begin = 0;
        core_set_data_ptr_end(theResult, GetInstanceSlotLength(sp));
    }

    return(TRUE);

HandlerGetError:
    EarlySlotBindError(theEnv, theInstance, theDefclass, theReference->slotID);
    theResult->type = ATOM;
    theResult->value = get_false(theEnv);
    core_set_eval_error(theEnv, TRUE);
    return(FALSE);
}

/***************************************************
 *  NAME         : PrintHandlerSlotPutFunction
 *  DESCRIPTION  : Developer access function for
 *              printing direct slot bindings
 *              in message-handlers
 *  INPUTS       : 1) The logical name of the output
 *              2) The bitmap expression
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Expression printed
 *  NOTES        : None
 ***************************************************/
#if WIN_BTC && (!DEVELOPER)
#pragma argsused
#endif
globle void PrintHandlerSlotPutFunction(void *theEnv, char *logical_name, void *theValue)
{
#if DEVELOPER
    HANDLER_SLOT_REFERENCE *theReference;
    DEFCLASS *theDefclass;
    SLOT_DESC *sd;

    theReference = (HANDLER_SLOT_REFERENCE *)to_bitmap(theValue);
    print_router(theEnv, logical_name, "(bind ?self:[");
    theDefclass = DefclassData(theEnv)->ClassIDMap[theReference->classID];
    print_router(theEnv, logical_name, to_string(theDefclass->header.name));
    print_router(theEnv, logical_name, "]");
    sd = theDefclass->instanceTemplate[theDefclass->slotNameMap[theReference->slotID] - 1];
    print_router(theEnv, logical_name, to_string(sd->slotName->name));

    if( core_get_first_arg() != NULL )
    {
        print_router(theEnv, logical_name, " ");
        core_print_expression(theEnv, logical_name, core_get_first_arg());
    }

    print_router(theEnv, logical_name, ")");
#else
#if MAC_MCW || WIN_MCW || MAC_XCD
#pragma unused(theEnv)
#pragma unused(logicalName)
#pragma unused(theValue)
#endif
#endif
}

/***************************************************
 *  NAME         : HandlerSlotPutFunction
 *  DESCRIPTION  : Access function for handling the
 *              statically-bound direct slot
 *              bindings in message-handlers
 *  INPUTS       : 1) The bitmap expression
 *              2) A data object buffer
 *  RETURNS      : TRUE if OK, FALSE
 *              on errors
 *  SIDE EFFECTS : Data object buffer gets symbol
 *              TRUE and slot is set. On errors,
 *              buffer gets symbol FALSE,
 *              eval_error is set and error
 *              messages are printed
 *  NOTES        : It is possible for a handler
 *              (attached to a superclass of
 *               the currently active instance)
 *              containing these static references
 *              to be called for an instance
 *              which does not contain the slots
 *              (e.g., an instance of a subclass
 *               where the original slot was
 *               no-inherit or the subclass
 *               overrode the original slot)
 ***************************************************/
globle BOOLEAN HandlerSlotPutFunction(void *theEnv, void *theValue, core_data_object *theResult)
{
    HANDLER_SLOT_REFERENCE *theReference;
    DEFCLASS *theDefclass;
    INSTANCE_TYPE *theInstance;
    INSTANCE_SLOT *sp;
    unsigned instanceSlotIndex;
    core_data_object theSetVal;

    theReference = (HANDLER_SLOT_REFERENCE *)to_bitmap(theValue);
    theInstance = (INSTANCE_TYPE *)core_get_function_primitive_data(theEnv)->arguments[0].value;
    theDefclass = DefclassData(theEnv)->ClassIDMap[theReference->classID];

    if( theInstance->garbage )
    {
        StaleInstanceAddress(theEnv, "for slot put", 0);
        theResult->type = ATOM;
        theResult->value = get_false(theEnv);
        core_set_eval_error(theEnv, TRUE);
        return(FALSE);
    }

    if( theInstance->cls == theDefclass )
    {
        instanceSlotIndex = theInstance->cls->slotNameMap[theReference->slotID];
        sp = theInstance->slotAddresses[instanceSlotIndex - 1];
    }
    else
    {
        if( theReference->slotID > theInstance->cls->maxSlotNameID )
        {
            goto HandlerPutError;
        }

        instanceSlotIndex = theInstance->cls->slotNameMap[theReference->slotID];

        if( instanceSlotIndex == 0 )
        {
            goto HandlerPutError;
        }

        instanceSlotIndex--;
        sp = theInstance->slotAddresses[instanceSlotIndex];

        if( sp->desc->cls != theDefclass )
        {
            goto HandlerPutError;
        }
    }

    /* =======================================================
     *  The slot has already been verified not to be read-only.
     *  However, if it is initialize-only, we need to make sure
     *  that we are initializing the instance (something we
     *  could not verify at parse-time)
     *  ======================================================= */
    if( sp->desc->initializeOnly && (!theInstance->initializeInProgress))
    {
        SlotAccessViolationError(theEnv, to_string(sp->desc->slotName->name),
                                 TRUE, (void *)theInstance);
        goto HandlerPutError2;
    }

    /* ======================================
     *  No arguments means to use the
     *  special null_arg_value to reset the slot
     *  to its default value
     *  ====================================== */
    if( core_get_first_arg())
    {
        if( EvaluateAndStoreInDataObject(theEnv, (int)sp->desc->multiple,
                                         core_get_first_arg(), &theSetVal, TRUE) == FALSE )
        {
            goto HandlerPutError2;
        }
    }
    else
    {
        core_set_data_start(theSetVal, 1);
        core_set_data_end(theSetVal, 0);
        core_set_type(theSetVal, LIST);
        core_set_value(theSetVal, core_get_function_primitive_data(theEnv)->null_arg_value);
    }

    if( PutSlotValue(theEnv, theInstance, sp, &theSetVal, theResult, NULL) == FALSE )
    {
        goto HandlerPutError2;
    }

    return(TRUE);

HandlerPutError:
    EarlySlotBindError(theEnv, theInstance, theDefclass, theReference->slotID);

HandlerPutError2:
    theResult->type = ATOM;
    theResult->value = get_false(theEnv);
    core_set_eval_error(theEnv, TRUE);

    return(FALSE);
}

/*****************************************************
 *  NAME         : DynamicHandlerGetSlot
 *  DESCRIPTION  : Directly references a slot's value
 *              (uses dynamic binding to lookup slot)
 *  INPUTS       : The caller's result buffer
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Caller's result buffer set
 *  NOTES        : H/L Syntax: (get <slot>)
 *****************************************************/
globle void DynamicHandlerGetSlot(void *theEnv, core_data_object *result)
{
    INSTANCE_SLOT *sp;
    INSTANCE_TYPE *ins;
    core_data_object temp;

    result->type = ATOM;
    result->value = get_false(theEnv);

    if( CheckCurrentMessage(theEnv, "dynamic-get", TRUE) == FALSE )
    {
        return;
    }

    core_eval_expression(theEnv, core_get_first_arg(), &temp);

    if( temp.type != ATOM )
    {
        report_explicit_type_error(theEnv, "dynamic-get", 1, "symbol");
        core_set_eval_error(theEnv, TRUE);
        return;
    }

    ins = GetActiveInstance(theEnv);
    sp = FindInstanceSlot(theEnv, ins, (ATOM_HN *)temp.value);

    if( sp == NULL )
    {
        error_method_duplication(theEnv, to_string(temp.value), "dynamic-get");
        return;
    }

    if((sp->desc->publicVisibility == 0) &&
       (MessageHandlerData(theEnv)->CurrentCore->hnd->cls != sp->desc->cls))
    {
        SlotVisibilityViolationError(theEnv, sp->desc, MessageHandlerData(theEnv)->CurrentCore->hnd->cls);
        core_set_eval_error(theEnv, TRUE);
        return;
    }

    result->type = (unsigned short)sp->type;
    result->value = sp->value;

    if( sp->type == LIST )
    {
        result->begin = 0;
        core_set_data_ptr_end(result, GetInstanceSlotLength(sp));
    }
}

/***********************************************************
 *  NAME         : DynamicHandlerPutSlot
 *  DESCRIPTION  : Directly puts a slot's value
 *              (uses dynamic binding to lookup slot)
 *  INPUTS       : Data obejct buffer for holding slot value
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Slot modified - and caller's buffer set
 *              to value (or symbol FALSE on errors)
 *  NOTES        : H/L Syntax: (put <slot> <value>*)
 ***********************************************************/
globle void DynamicHandlerPutSlot(void *theEnv, core_data_object *theResult)
{
    INSTANCE_SLOT *sp;
    INSTANCE_TYPE *ins;
    core_data_object temp;

    theResult->type = ATOM;
    theResult->value = get_false(theEnv);

    if( CheckCurrentMessage(theEnv, "dynamic-put", TRUE) == FALSE )
    {
        return;
    }

    core_eval_expression(theEnv, core_get_first_arg(), &temp);

    if( temp.type != ATOM )
    {
        report_explicit_type_error(theEnv, "dynamic-put", 1, "symbol");
        core_set_eval_error(theEnv, TRUE);
        return;
    }

    ins = GetActiveInstance(theEnv);
    sp = FindInstanceSlot(theEnv, ins, (ATOM_HN *)temp.value);

    if( sp == NULL )
    {
        error_method_duplication(theEnv, to_string(temp.value), "dynamic-put");
        return;
    }

    if((sp->desc->noWrite == 0) ? FALSE :
       ((sp->desc->initializeOnly == 0) || (!ins->initializeInProgress)))
    {
        SlotAccessViolationError(theEnv, to_string(sp->desc->slotName->name),
                                 TRUE, (void *)ins);
        core_set_eval_error(theEnv, TRUE);
        return;
    }

    if((sp->desc->publicVisibility == 0) &&
       (MessageHandlerData(theEnv)->CurrentCore->hnd->cls != sp->desc->cls))
    {
        SlotVisibilityViolationError(theEnv, sp->desc, MessageHandlerData(theEnv)->CurrentCore->hnd->cls);
        core_set_eval_error(theEnv, TRUE);
        return;
    }

    if( core_get_first_arg()->next_arg )
    {
        if( EvaluateAndStoreInDataObject(theEnv, (int)sp->desc->multiple,
                                         core_get_first_arg()->next_arg, &temp, TRUE) == FALSE )
        {
            return;
        }
    }
    else
    {
        core_set_data_ptr_start(&temp, 1);
        core_set_data_ptr_end(&temp, 0);
        core_set_pointer_type(&temp, LIST);
        core_set_pointer_value(&temp, core_get_function_primitive_data(theEnv)->null_arg_value);
    }

    PutSlotValue(theEnv, ins, sp, &temp, theResult, NULL);
}

/* =========================================
 *****************************************
 *       INTERNALLY VISIBLE FUNCTIONS
 *  =========================================
 ***************************************** */

/*****************************************************
 *  NAME         : PerformMessage
 *  DESCRIPTION  : Calls core framework for a message
 *  INPUTS       : 1) Caller's result buffer
 *              2) Message argument expressions
 *                 (including implicit object)
 *              3) Message name
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Any side-effects of message execution
 *                 and caller's result buffer set
 *  NOTES        : None
 *****************************************************/
static void PerformMessage(void *theEnv, core_data_object *result, core_expression_object *args, ATOM_HN *mname)
{
    int oldce;
    /* HANDLER_LINK *oldCore; */
    DEFCLASS *cls = NULL;
    INSTANCE_TYPE *ins = NULL;
    ATOM_HN *oldName;

#if PROFILING_FUNCTIONS
    struct profileFrameInfo profileFrame;
#endif

    result->type = ATOM;
    result->value = get_false(theEnv);
    core_get_evaluation_data(theEnv)->eval_error = FALSE;

    if( core_get_evaluation_data(theEnv)->halt )
    {
        return;
    }

    oldce = core_is_construct_executing(theEnv);
    core_set_construct_executing(theEnv, TRUE);
    oldName = MessageHandlerData(theEnv)->CurrentMessageName;
    MessageHandlerData(theEnv)->CurrentMessageName = mname;
    core_get_evaluation_data(theEnv)->eval_depth++;

    core_push_function_args(theEnv, args, core_count_args(args),
                       to_string(MessageHandlerData(theEnv)->CurrentMessageName), "message",
                       UnboundHandlerErr);


    if( core_get_evaluation_data(theEnv)->eval_error )
    {
        core_get_evaluation_data(theEnv)->eval_depth--;
        MessageHandlerData(theEnv)->CurrentMessageName = oldName;
        core_gc_periodic_cleanup(theEnv, FALSE, TRUE);
        core_set_construct_executing(theEnv, oldce);
        return;
    }

    if( core_get_function_primitive_data(theEnv)->arguments->type == INSTANCE_ADDRESS )
    {
        ins = (INSTANCE_TYPE *)core_get_function_primitive_data(theEnv)->arguments->value;

        if( ins->garbage == 1 )
        {
            StaleInstanceAddress(theEnv, "send", 0);
            core_set_eval_error(theEnv, TRUE);
        }
        else if( DefclassInScope(theEnv, ins->cls, (struct module_definition *)get_current_module(theEnv)) == FALSE )
        {
            NoInstanceError(theEnv, to_string(ins->name), "send");
        }
        else
        {
            cls = ins->cls;
            ins->busy++;
        }
    }
    else if( core_get_function_primitive_data(theEnv)->arguments->type == INSTANCE_NAME )
    {
        ins = FindInstanceBySymbol(theEnv, (ATOM_HN *)core_get_function_primitive_data(theEnv)->arguments->value);

        if( ins == NULL )
        {
            error_print_id(theEnv, "MSGPASS", 2, FALSE);
            print_router(theEnv, WERROR, "No such instance ");
            print_router(theEnv, WERROR, to_string((ATOM_HN *)core_get_function_primitive_data(theEnv)->arguments->value));
            print_router(theEnv, WERROR, " in function send.\n");
            core_set_eval_error(theEnv, TRUE);
        }
        else
        {
            core_get_function_primitive_data(theEnv)->arguments->value = (void *)ins;
            core_get_function_primitive_data(theEnv)->arguments->type = INSTANCE_ADDRESS;
            cls = ins->cls;
            ins->busy++;
        }
    }
    else if((cls = DefclassData(theEnv)->PrimitiveClassMap[core_get_function_primitive_data(theEnv)->arguments->type]) == NULL )
    {
        error_system(theEnv, "MSGPASS", 1);
        exit_router(theEnv, EXIT_FAILURE);
    }

    if( core_get_evaluation_data(theEnv)->eval_error )
    {
        core_pop_function_args(theEnv);
        core_get_evaluation_data(theEnv)->eval_depth--;
        MessageHandlerData(theEnv)->CurrentMessageName = oldName;
        core_gc_periodic_cleanup(theEnv, FALSE, TRUE);
        core_set_construct_executing(theEnv, oldce);
        return;
    }

    /* oldCore = MessageHandlerData(theEnv)->TopOfCore; */

    if( MessageHandlerData(theEnv)->TopOfCore != NULL )
    {
        MessageHandlerData(theEnv)->TopOfCore->nxtInStack = MessageHandlerData(theEnv)->OldCore;
    }

    MessageHandlerData(theEnv)->OldCore = MessageHandlerData(theEnv)->TopOfCore;

    MessageHandlerData(theEnv)->TopOfCore = FindApplicableHandlers(theEnv, cls, mname);

    if( MessageHandlerData(theEnv)->TopOfCore != NULL )
    {
        HANDLER_LINK *oldCurrent, *oldNext;

        oldCurrent = MessageHandlerData(theEnv)->CurrentCore;
        oldNext = MessageHandlerData(theEnv)->NextInCore;

        if( MessageHandlerData(theEnv)->TopOfCore->hnd->type == MAROUND )
        {
            MessageHandlerData(theEnv)->CurrentCore = MessageHandlerData(theEnv)->TopOfCore;
            MessageHandlerData(theEnv)->NextInCore = MessageHandlerData(theEnv)->TopOfCore->nxt;
#if DEBUGGING_FUNCTIONS

            if( MessageHandlerData(theEnv)->WatchMessages )
            {
                WatchMessage(theEnv, WTRACE, BEGIN_TRACE);
            }

            if( MessageHandlerData(theEnv)->CurrentCore->hnd->trace )
            {
                WatchHandler(theEnv, WTRACE, MessageHandlerData(theEnv)->CurrentCore, BEGIN_TRACE);
            }

#endif

            if( CheckHandlerArgCount(theEnv))
            {
#if PROFILING_FUNCTIONS
                StartProfile(theEnv, &profileFrame,
                             &MessageHandlerData(theEnv)->CurrentCore->hnd->ext_data,
                             ProfileFunctionData(theEnv)->ProfileConstructs);
#endif


                core_eval_function_actions(theEnv, MessageHandlerData(theEnv)->CurrentCore->hnd->cls->header.my_module->module_def,
                                    MessageHandlerData(theEnv)->CurrentCore->hnd->actions,
                                    MessageHandlerData(theEnv)->CurrentCore->hnd->localVarCount,
                                    result, UnboundHandlerErr);


#if PROFILING_FUNCTIONS
                EndProfile(theEnv, &profileFrame);
#endif
            }

#if DEBUGGING_FUNCTIONS

            if( MessageHandlerData(theEnv)->CurrentCore->hnd->trace )
            {
                WatchHandler(theEnv, WTRACE, MessageHandlerData(theEnv)->CurrentCore, END_TRACE);
            }

            if( MessageHandlerData(theEnv)->WatchMessages )
            {
                WatchMessage(theEnv, WTRACE, END_TRACE);
            }

#endif
        }
        else
        {
            MessageHandlerData(theEnv)->CurrentCore = NULL;
            MessageHandlerData(theEnv)->NextInCore = MessageHandlerData(theEnv)->TopOfCore;
#if DEBUGGING_FUNCTIONS

            if( MessageHandlerData(theEnv)->WatchMessages )
            {
                WatchMessage(theEnv, WTRACE, BEGIN_TRACE);
            }

#endif
            CallHandlers(theEnv, result);
#if DEBUGGING_FUNCTIONS

            if( MessageHandlerData(theEnv)->WatchMessages )
            {
                WatchMessage(theEnv, WTRACE, END_TRACE);
            }

#endif
        }

        DestroyHandlerLinks(theEnv, MessageHandlerData(theEnv)->TopOfCore);
        MessageHandlerData(theEnv)->CurrentCore = oldCurrent;
        MessageHandlerData(theEnv)->NextInCore = oldNext;
    }

    /* MessageHandlerData(theEnv)->TopOfCore = oldCore; */
    MessageHandlerData(theEnv)->TopOfCore = MessageHandlerData(theEnv)->OldCore;

    if( MessageHandlerData(theEnv)->OldCore != NULL )
    {
        MessageHandlerData(theEnv)->OldCore = MessageHandlerData(theEnv)->OldCore->nxtInStack;
    }

    get_flow_control_data(theEnv)->return_flag = FALSE;

    if( ins != NULL )
    {
        ins->busy--;
    }

    /* ==================================
     *  Restore the original calling frame
     *  ================================== */
    core_pop_function_args(theEnv);
    core_get_evaluation_data(theEnv)->eval_depth--;
    MessageHandlerData(theEnv)->CurrentMessageName = oldName;
    core_pass_return_value(theEnv, result);
    core_gc_periodic_cleanup(theEnv, FALSE, TRUE);
    core_set_construct_executing(theEnv, oldce);

    if( core_get_evaluation_data(theEnv)->eval_error )
    {
        result->type = ATOM;
        result->value = get_false(theEnv);
    }
}

/*****************************************************************************
 *  NAME         : FindApplicableHandlers
 *  DESCRIPTION  : Given a message name, this routine forms the "core frame"
 *                for the message : a list of all applicable class handlers.
 *                An applicable class handler is one whose name matches
 *                  the message and whose class matches the instance.
 *
 *                The list is in the following order :
 *
 *                All around handlers (from most specific to most general)
 *                All before handlers (from most specific to most general)
 *                All primary handlers (from most specific to most general)
 *                All after handlers (from most general to most specific)
 *
 *  INPUTS       : 1) The class of the instance (or primitive) for the message
 *              2) The message name
 *  RETURNS      : NULL if no applicable handlers or errors,
 *                the list of handlers otherwise
 *  SIDE EFFECTS : Links are allocated for the list
 *  NOTES        : The instance is the first thing on the arguments
 *              The number of arguments is in arguments_sz
 *****************************************************************************/
static HANDLER_LINK *FindApplicableHandlers(void *theEnv, DEFCLASS *cls, ATOM_HN *mname)
{
    register int i;
    HANDLER_LINK *tops[4], *bots[4];

    for( i = MAROUND ; i <= MAFTER ; i++ )
    {
        tops[i] = bots[i] = NULL;
    }

    for( i = 0 ; i < cls->allSuperclasses.classCount ; i++ )
    {
        FindApplicableOfName(theEnv, cls->allSuperclasses.classArray[i], tops, bots, mname);
    }

    return(JoinHandlerLinks(theEnv, tops, bots, mname));
}

/***************************************************************
 *  NAME         : CallHandlers
 *  DESCRIPTION  : Moves though the current message frame
 *                for a send-message as follows :
 *
 *              Call all before handlers and ignore their
 *                return values.
 *              Call the first primary handler and
 *                ignore the rest.  The return value
 *                of the handler frame is this message's value.
 *              Call all after handlers and ignore their
 *                return values.
 *  INPUTS       : Caller's buffer for the return value of
 *                the message
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : The handlers are evaluated.
 *  NOTES        : IMPORTANT : The global NextInCore should be
 *              pointing to the first handler to be executed.
 ***************************************************************/
static void CallHandlers(void *theEnv, core_data_object *result)
{
#if WIN_BTC
    HANDLER_LINK *oldCurrent, *oldNext;                /* prevents warning */
#else
    HANDLER_LINK *oldCurrent = NULL, *oldNext = NULL;  /* prevents warning */
#endif
    core_data_object temp;
#if PROFILING_FUNCTIONS
    struct profileFrameInfo profileFrame;
#endif

    if( core_get_evaluation_data(theEnv)->halt )
    {
        return;
    }

    oldCurrent = MessageHandlerData(theEnv)->CurrentCore;
    oldNext = MessageHandlerData(theEnv)->NextInCore;

    while( MessageHandlerData(theEnv)->NextInCore->hnd->type == MBEFORE )
    {
        MessageHandlerData(theEnv)->CurrentCore = MessageHandlerData(theEnv)->NextInCore;
        MessageHandlerData(theEnv)->NextInCore = MessageHandlerData(theEnv)->NextInCore->nxt;
#if DEBUGGING_FUNCTIONS

        if( MessageHandlerData(theEnv)->CurrentCore->hnd->trace )
        {
            WatchHandler(theEnv, WTRACE, MessageHandlerData(theEnv)->CurrentCore, BEGIN_TRACE);
        }

#endif

        if( CheckHandlerArgCount(theEnv))
        {
#if PROFILING_FUNCTIONS
            StartProfile(theEnv, &profileFrame,
                         &MessageHandlerData(theEnv)->CurrentCore->hnd->ext_data,
                         ProfileFunctionData(theEnv)->ProfileConstructs);
#endif

            core_eval_function_actions(theEnv, MessageHandlerData(theEnv)->CurrentCore->hnd->cls->header.my_module->module_def,
                                MessageHandlerData(theEnv)->CurrentCore->hnd->actions,
                                MessageHandlerData(theEnv)->CurrentCore->hnd->localVarCount,
                                &temp, UnboundHandlerErr);


#if PROFILING_FUNCTIONS
            EndProfile(theEnv, &profileFrame);
#endif
        }

#if DEBUGGING_FUNCTIONS

        if( MessageHandlerData(theEnv)->CurrentCore->hnd->trace )
        {
            WatchHandler(theEnv, WTRACE, MessageHandlerData(theEnv)->CurrentCore, END_TRACE);
        }

#endif
        get_flow_control_data(theEnv)->return_flag = FALSE;

        if((MessageHandlerData(theEnv)->NextInCore == NULL) || core_get_evaluation_data(theEnv)->halt )
        {
            MessageHandlerData(theEnv)->NextInCore = oldNext;
            MessageHandlerData(theEnv)->CurrentCore = oldCurrent;
            return;
        }
    }

    if( MessageHandlerData(theEnv)->NextInCore->hnd->type == MPRIMARY )
    {
        MessageHandlerData(theEnv)->CurrentCore = MessageHandlerData(theEnv)->NextInCore;
        MessageHandlerData(theEnv)->NextInCore = MessageHandlerData(theEnv)->NextInCore->nxt;
#if DEBUGGING_FUNCTIONS

        if( MessageHandlerData(theEnv)->CurrentCore->hnd->trace )
        {
            WatchHandler(theEnv, WTRACE, MessageHandlerData(theEnv)->CurrentCore, BEGIN_TRACE);
        }

#endif

        if( CheckHandlerArgCount(theEnv))
        {
#if PROFILING_FUNCTIONS
            StartProfile(theEnv, &profileFrame,
                         &MessageHandlerData(theEnv)->CurrentCore->hnd->ext_data,
                         ProfileFunctionData(theEnv)->ProfileConstructs);
#endif


            core_eval_function_actions(theEnv, MessageHandlerData(theEnv)->CurrentCore->hnd->cls->header.my_module->module_def,
                                MessageHandlerData(theEnv)->CurrentCore->hnd->actions,
                                MessageHandlerData(theEnv)->CurrentCore->hnd->localVarCount,
                                result, UnboundHandlerErr);

#if PROFILING_FUNCTIONS
            EndProfile(theEnv, &profileFrame);
#endif
        }


#if DEBUGGING_FUNCTIONS

        if( MessageHandlerData(theEnv)->CurrentCore->hnd->trace )
        {
            WatchHandler(theEnv, WTRACE, MessageHandlerData(theEnv)->CurrentCore, END_TRACE);
        }

#endif
        get_flow_control_data(theEnv)->return_flag = FALSE;

        if((MessageHandlerData(theEnv)->NextInCore == NULL) || core_get_evaluation_data(theEnv)->halt )
        {
            MessageHandlerData(theEnv)->NextInCore = oldNext;
            MessageHandlerData(theEnv)->CurrentCore = oldCurrent;
            return;
        }

        while( MessageHandlerData(theEnv)->NextInCore->hnd->type == MPRIMARY )
        {
            MessageHandlerData(theEnv)->NextInCore = MessageHandlerData(theEnv)->NextInCore->nxt;

            if( MessageHandlerData(theEnv)->NextInCore == NULL )
            {
                MessageHandlerData(theEnv)->NextInCore = oldNext;
                MessageHandlerData(theEnv)->CurrentCore = oldCurrent;
                return;
            }
        }
    }

    while( MessageHandlerData(theEnv)->NextInCore->hnd->type == MAFTER )
    {
        MessageHandlerData(theEnv)->CurrentCore = MessageHandlerData(theEnv)->NextInCore;
        MessageHandlerData(theEnv)->NextInCore = MessageHandlerData(theEnv)->NextInCore->nxt;
#if DEBUGGING_FUNCTIONS

        if( MessageHandlerData(theEnv)->CurrentCore->hnd->trace )
        {
            WatchHandler(theEnv, WTRACE, MessageHandlerData(theEnv)->CurrentCore, BEGIN_TRACE);
        }

#endif

        if( CheckHandlerArgCount(theEnv))
        {
#if PROFILING_FUNCTIONS
            StartProfile(theEnv, &profileFrame,
                         &MessageHandlerData(theEnv)->CurrentCore->hnd->ext_data,
                         ProfileFunctionData(theEnv)->ProfileConstructs);
#endif


            core_eval_function_actions(theEnv, MessageHandlerData(theEnv)->CurrentCore->hnd->cls->header.my_module->module_def,
                                MessageHandlerData(theEnv)->CurrentCore->hnd->actions,
                                MessageHandlerData(theEnv)->CurrentCore->hnd->localVarCount,
                                &temp, UnboundHandlerErr);

#if PROFILING_FUNCTIONS
            EndProfile(theEnv, &profileFrame);
#endif
        }


#if DEBUGGING_FUNCTIONS

        if( MessageHandlerData(theEnv)->CurrentCore->hnd->trace )
        {
            WatchHandler(theEnv, WTRACE, MessageHandlerData(theEnv)->CurrentCore, END_TRACE);
        }

#endif
        get_flow_control_data(theEnv)->return_flag = FALSE;

        if((MessageHandlerData(theEnv)->NextInCore == NULL) || core_get_evaluation_data(theEnv)->halt )
        {
            MessageHandlerData(theEnv)->NextInCore = oldNext;
            MessageHandlerData(theEnv)->CurrentCore = oldCurrent;
            return;
        }
    }

    MessageHandlerData(theEnv)->NextInCore = oldNext;
    MessageHandlerData(theEnv)->CurrentCore = oldCurrent;
}

/********************************************************
 *  NAME         : EarlySlotBindError
 *  DESCRIPTION  : Prints out an error message when
 *              a message-handler from a superclass
 *              which contains a static-bind
 *              slot access is not valid for the
 *              currently active instance (i.e.
 *              the instance is not using the
 *              superclass's slot)
 *  INPUTS       : 1) The currently active instance
 *              2) The defclass holding the invalid slot
 *              3) The canonical id of the slot
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Error message printed
 *  NOTES        : None
 ********************************************************/
static void EarlySlotBindError(void *theEnv, INSTANCE_TYPE *theInstance, DEFCLASS *theDefclass, unsigned slotID)
{
    SLOT_DESC *sd;

    sd = theDefclass->instanceTemplate[theDefclass->slotNameMap[slotID] - 1];
    error_print_id(theEnv, "MSGPASS", 3, FALSE);
    print_router(theEnv, WERROR, "Static reference to slot ");
    print_router(theEnv, WERROR, to_string(sd->slotName->name));
    print_router(theEnv, WERROR, " of class ");
    PrintClassName(theEnv, WERROR, theDefclass, FALSE);
    print_router(theEnv, WERROR, " does not apply to ");
    PrintInstanceNameAndClass(theEnv, WERROR, theInstance, TRUE);
}

#endif
