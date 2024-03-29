/* =========================================
 *****************************************
 *            EXTERNAL DEFINITIONS
 *  =========================================
 ***************************************** */
#include "setup.h"

#if OBJECT_SYSTEM

#include <string.h>

#include "core_arguments.h"
#include "classes_kernel.h"
#include "funcs_class.h"
#include "classes_info.h"
#include "core_environment.h"
#include "funcs_instance.h"
#include "classes_instances_modify.h"
#include "funcs_method.h"
#include "classes_methods_call.h"
#include "core_memory.h"
#include "core_functions_util.h"
#include "router.h"

#include "core_functions.h"

#include "core_constructs.h"
#include "parser_class_methods.h"

#if DEBUGGING_FUNCTIONS
#include "core_watch.h"
#endif

#define __CLASSES_METHODS_KERNEL_SOURCE__
#include "classes_methods_kernel.h"

/* =========================================
 *****************************************
 *   INTERNALLY VISIBLE FUNCTION HEADERS
 *  =========================================
 ***************************************** */

static void CreateSystemHandlers(void *);

static int WildDeleteHandler(void *, DEFCLASS *, ATOM_HN *, char *);

#if DEBUGGING_FUNCTIONS
static unsigned DefmessageHandlerWatchAccess(void *, int, unsigned, core_expression_object *);
static unsigned DefmessageHandlerWatchPrint(void *, char *, int, core_expression_object *);
static unsigned DefmessageHandlerWatchSupport(void *, char *, char *, int,
                                              void(*) (void *, char *, void *, int),
                                              void(*) (void *, int, void *, int),
                                              core_expression_object *);
static unsigned WatchClassHandlers(void *, void *, char *, int, char *, int, int,
                                   void(*) (void *, char *, void *, int),
                                   void(*) (void *, int, void *, int));
static void PrintHandlerWatchFlag(void *, char *, void *, int);
#endif

static void DeallocateMessageHandlerData(void *);

/* =========================================
 *****************************************
 *       EXTERNALLY VISIBLE FUNCTIONS
 *  =========================================
 ***************************************** */

/***************************************************
 *  NAME         : SetupMessageHandlers
 *  DESCRIPTION  : Sets up internal symbols and
 *              fucntion definitions pertaining to
 *              message-handlers.  Also creates
 *              system handlers
 *  INPUTS       : None
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Functions and data structures
 *              initialized
 *  NOTES        : Should be called before
 *              SetupInstanceModDupCommands() in
 *              INSMODDP.C
 ***************************************************/
globle void SetupMessageHandlers(void *theEnv)
{
    core_data_entity_object handlerGetInfo = {"HANDLER_GET",               HANDLER_GET,                 0,                           1,           1,
                                    PrintHandlerSlotGetFunction,
                                    PrintHandlerSlotGetFunction, NULL,
                                    HandlerSlotGetFunction,
                                    NULL,                        NULL,                        NULL,                        NULL,        NULL, NULL,NULL, NULL},

                  handlerPutInfo = {"HANDLER_PUT",               HANDLER_PUT,                 0,                           1,           1,
                                    PrintHandlerSlotPutFunction,
                                    PrintHandlerSlotPutFunction, NULL,
                                    HandlerSlotPutFunction,
                                    NULL,                        NULL,                        NULL,                        NULL,        NULL, NULL,NULL, NULL};

    core_allocate_environment_data(theEnv, MESSAGE_HANDLER_DATA, sizeof(struct messageHandlerData), DeallocateMessageHandlerData);
    memcpy(&MessageHandlerData(theEnv)->HandlerGetInfo, &handlerGetInfo, sizeof(struct core_data_entity));
    memcpy(&MessageHandlerData(theEnv)->HandlerPutInfo, &handlerPutInfo, sizeof(struct core_data_entity));

    MessageHandlerData(theEnv)->hndquals[0] = "around";
    MessageHandlerData(theEnv)->hndquals[1] = "before";
    MessageHandlerData(theEnv)->hndquals[2] = "primary";
    MessageHandlerData(theEnv)->hndquals[3] = "after";

    core_install_primitive(theEnv, &MessageHandlerData(theEnv)->HandlerGetInfo, HANDLER_GET);
    core_install_primitive(theEnv, &MessageHandlerData(theEnv)->HandlerPutInfo, HANDLER_PUT);

    MessageHandlerData(theEnv)->INIT_ATOM = (ATOM_HN *)store_atom(theEnv, INIT_STRING);
    inc_atom_count(MessageHandlerData(theEnv)->INIT_ATOM);

    MessageHandlerData(theEnv)->DELETE_ATOM = (ATOM_HN *)store_atom(theEnv, DELETE_STRING);
    inc_atom_count(MessageHandlerData(theEnv)->DELETE_ATOM);

    MessageHandlerData(theEnv)->CREATE_ATOM = (ATOM_HN *)store_atom(theEnv, CREATE_STRING);
    inc_atom_count(MessageHandlerData(theEnv)->CREATE_ATOM);

    core_add_clear_fn(theEnv, "defclass", CreateSystemHandlers, -100);

    MessageHandlerData(theEnv)->SELF_ATOM = (ATOM_HN *)store_atom(theEnv, SELF_STRING);
    inc_atom_count(MessageHandlerData(theEnv)->SELF_ATOM);

    core_define_construct(theEnv, "defmessage-handler", "defmessage-handlers",
                 ParseDefmessageHandler, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    core_define_function(theEnv, "undefmessage-handler", 'v', PTR_FN UndefmessageHandlerCommand,
                       "UndefmessageHandlerCommand", "23w");


    core_define_function(theEnv, "send", 'u', PTR_FN SendCommand, "SendCommand", "2*uuw");

#if DEBUGGING_FUNCTIONS
    core_define_function(theEnv, "preview-send", 'v', PTR_FN PreviewSendCommand, "PreviewSendCommand", "22w");

    core_define_function(theEnv, "ppdefmessage-handler", 'v', PTR_FN PPDefmessageHandlerCommand,
                       "PPDefmessageHandlerCommand", "23w");
    core_define_function(theEnv, "list-defmessage-handlers", 'v', PTR_FN ListDefmessageHandlersCommand,
                       "ListDefmessageHandlersCommand", "02w");
#endif

    core_define_function(theEnv, "next-handlerp", 'b', PTR_FN NextHandlerAvailable, "NextHandlerAvailable", "00");
    core_set_function_overload(theEnv, "next-handlerp", TRUE, FALSE);
    core_define_function(theEnv, "call-next-handler", 'u',
                       PTR_FN CallNextHandler, "CallNextHandler", "00");
    core_set_function_overload(theEnv, "call-next-handler", TRUE, FALSE);
    core_define_function(theEnv, "override-next-handler", 'u',
                       PTR_FN CallNextHandler, "CallNextHandler", NULL);
    core_set_function_overload(theEnv, "override-next-handler", TRUE, FALSE);

    core_define_function(theEnv, "dynamic-get", 'u', PTR_FN DynamicHandlerGetSlot, "DynamicHandlerGetSlot", "11w");
    core_define_function(theEnv, "dynamic-put", 'u', PTR_FN DynamicHandlerPutSlot, "DynamicHandlerPutSlot", "1**w");
    core_define_function(theEnv, "get", 'u', PTR_FN DynamicHandlerGetSlot, "DynamicHandlerGetSlot", "11w");
    core_define_function(theEnv, "put", 'u', PTR_FN DynamicHandlerPutSlot, "DynamicHandlerPutSlot", "1**w");

#if DEBUGGING_FUNCTIONS
    AddWatchItem(theEnv, "messages", 0, &MessageHandlerData(theEnv)->WatchMessages, 36, NULL, NULL);
    AddWatchItem(theEnv, "message-handlers", 0, &MessageHandlerData(theEnv)->WatchHandlers, 35,
                 DefmessageHandlerWatchAccess, DefmessageHandlerWatchPrint);
#endif
}

/******************************************************
 * DeallocateMessageHandlerData: Deallocates environment
 *    data for the message handler functionality.
 ******************************************************/
static void DeallocateMessageHandlerData(void *theEnv)
{
    HANDLER_LINK *tmp, *mhead, *chead;

    mhead = MessageHandlerData(theEnv)->TopOfCore;

    while( mhead != NULL )
    {
        tmp = mhead;
        mhead = mhead->nxt;
        core_mem_return_struct(theEnv, messageHandlerLink, tmp);
    }

    chead = MessageHandlerData(theEnv)->OldCore;

    while( chead != NULL )
    {
        mhead = chead;
        chead = chead->nxtInStack;

        while( mhead != NULL )
        {
            tmp = mhead;
            mhead = mhead->nxt;
            core_mem_return_struct(theEnv, messageHandlerLink, tmp);
        }
    }
}

/*****************************************************
 *  NAME         : EnvGetDefmessageHandlerName
 *  DESCRIPTION  : Gets the name of a message-handler
 *  INPUTS       : 1) Pointer to a class
 *              2) Array index of handler in class's
 *                 message-handler array (+1)
 *  RETURNS      : Name-string of message-handler
 *  SIDE EFFECTS : None
 *  NOTES        : None
 *****************************************************/
#if WIN_BTC
#pragma argsused
#endif
char *EnvGetDefmessageHandlerName(void *theEnv, void *ptr, int theIndex)
{
#if MAC_MCW || WIN_MCW || MAC_XCD
#pragma unused(theEnv)
#endif

    return(to_string(((DEFCLASS *)ptr)->handlers[theIndex - 1].name));
}

/*****************************************************
 *  NAME         : EnvGetDefmessageHandlerType
 *  DESCRIPTION  : Gets the type of a message-handler
 *  INPUTS       : 1) Pointer to a class
 *              2) Array index of handler in class's
 *                 message-handler array (+1)
 *  RETURNS      : Type-string of message-handler
 *  SIDE EFFECTS : None
 *  NOTES        : None
 *****************************************************/
globle char *EnvGetDefmessageHandlerType(void *theEnv, void *ptr, int theIndex)
{
    return(MessageHandlerData(theEnv)->hndquals[((DEFCLASS *)ptr)->handlers[theIndex - 1].type]);
}

/**************************************************************
 *  NAME         : EnvGetNextDefmessageHandler
 *  DESCRIPTION  : Finds first or next handler for a class
 *  INPUTS       : 1) The address of the handler's class
 *              2) The array index of the current handler (+1)
 *  RETURNS      : The array index (+1) of the next handler, or 0
 *                if there is none
 *  SIDE EFFECTS : None
 *  NOTES        : If index == 0, the first handler array index
 *              (i.e. 1) returned
 **************************************************************/
#if WIN_BTC
#pragma argsused
#endif
globle int EnvGetNextDefmessageHandler(void *theEnv, void *ptr, int theIndex)
{
    DEFCLASS *cls;

#if MAC_MCW || WIN_MCW || MAC_XCD
#pragma unused(theEnv)
#endif

    cls = (DEFCLASS *)ptr;

    if( theIndex == 0 )
    {
        return((cls->handlers != NULL) ? 1 : 0);
    }

    if( theIndex == cls->handlerCount )
    {
        return(0);
    }

    return(theIndex + 1);
}

/*****************************************************
 *  NAME         : GetDefmessageHandlerPointer
 *  DESCRIPTION  : Returns a pointer to a handler
 *  INPUTS       : 1) Pointer to a class
 *              2) Array index of handler in class's
 *                 message-handler array (+1)
 *  RETURNS      : Pointer to the handler.
 *  SIDE EFFECTS : None
 *  NOTES        : None
 *****************************************************/
globle HANDLER *GetDefmessageHandlerPointer(void *ptr, int theIndex)
{
    return(&((DEFCLASS *)ptr)->handlers[theIndex - 1]);
}

#if DEBUGGING_FUNCTIONS

/*********************************************************
 *  NAME         : EnvGetDefmessageHandlerWatch
 *  DESCRIPTION  : Determines if trace messages for calls
 *              to this handler will be generated or not
 *  INPUTS       : 1) A pointer to the class
 *              2) The index of the handler
 *  RETURNS      : TRUE if a trace is active,
 *              FALSE otherwise
 *  SIDE EFFECTS : None
 *  NOTES        : None
 *********************************************************/
#if WIN_BTC
#pragma argsused
#endif
globle unsigned EnvGetDefmessageHandlerWatch(void *theEnv, void *theClass, int theIndex)
{
#if MAC_MCW || WIN_MCW || MAC_XCD
#pragma unused(theEnv)
#endif

    return(((DEFCLASS *)theClass)->handlers[theIndex - 1].trace);
}

/*********************************************************
 *  NAME         : EnvSetDefmessageHandlerWatch
 *  DESCRIPTION  : Sets the trace to ON/OFF for the
 *              calling of the handler
 *  INPUTS       : 1) TRUE to set the trace on,
 *                 FALSE to set it off
 *              2) A pointer to the class
 *              3) The index of the handler
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Watch flag for the handler set
 *  NOTES        : None
 *********************************************************/
#if WIN_BTC
#pragma argsused
#endif
globle void EnvSetDefmessageHandlerWatch(void *theEnv, int newState, void *theClass, int theIndex)
{
#if MAC_MCW || WIN_MCW || MAC_XCD
#pragma unused(theEnv)
#endif

    ((DEFCLASS *)theClass)->handlers[theIndex - 1].trace = newState;
}

#endif

/***************************************************
 *  NAME         : EnvFindDefmessageHandler
 *  DESCRIPTION  : Determines the index of a specfied
 *               message-handler
 *  INPUTS       : 1) A pointer to the class
 *              2) Name-string of the handler
 *              3) Handler-type: "around","before",
 *                 "primary", or "after"
 *  RETURNS      : The index of the handler
 *                (0 if not found)
 *  SIDE EFFECTS : None
 *  NOTES        : None
 ***************************************************/
globle unsigned EnvFindDefmessageHandler(void *theEnv, void *ptr, char *hname, char *htypestr)
{
    unsigned htype;
    ATOM_HN *hsym;
    DEFCLASS *cls;
    int theIndex;

    htype = HandlerType(theEnv, "handler-lookup", htypestr);

    if( htype == MERROR )
    {
        return(0);
    }

    hsym = lookup_atom(theEnv, hname);

    if( hsym == NULL )
    {
        return(0);
    }

    cls = (DEFCLASS *)ptr;
    theIndex = FindHandlerByIndex(cls, hsym, (unsigned)htype);
    return((unsigned)(theIndex + 1));
}

/***************************************************
 *  NAME         : EnvIsDefmessageHandlerDeletable
 *  DESCRIPTION  : Determines if a message-handler
 *                can be deleted
 *  INPUTS       : 1) Address of the handler's class
 *              2) Index of the handler
 *  RETURNS      : TRUE if deletable, FALSE otherwise
 *  SIDE EFFECTS : None
 *  NOTES        : None
 ***************************************************/
globle int EnvIsDefmessageHandlerDeletable(void *theEnv, void *ptr, int theIndex)
{
    DEFCLASS *cls;

    if( !core_are_constructs_deleteable(theEnv))
    {
        return(FALSE);
    }

    cls = (DEFCLASS *)ptr;

    if( cls->handlers[theIndex - 1].system == 1 )
    {
        return(FALSE);
    }

    return((HandlersExecuting(cls) == FALSE) ? TRUE : FALSE);
}

/******************************************************************************
 *  NAME         : UndefmessageHandlerCommand
 *  DESCRIPTION  : Deletes a handler from a class
 *  INPUTS       : None
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Handler deleted if possible
 *  NOTES        : H/L Syntax: (undefmessage-handler <class> <handler> [<type>])
 ******************************************************************************/
globle void UndefmessageHandlerCommand(void *theEnv)
{
    ATOM_HN *mname;
    char *tname;
    core_data_object tmp;
    DEFCLASS *cls;


    if( core_check_arg_type(theEnv, "undefmessage-handler", 1, ATOM, &tmp) == FALSE )
    {
        return;
    }

    cls = LookupDefclassByMdlOrScope(theEnv, core_convert_data_to_string(tmp));

    if((cls == NULL) ? (strcmp(core_convert_data_to_string(tmp), "*") != 0) : FALSE )
    {
        ClassExistError(theEnv, "undefmessage-handler", core_convert_data_to_string(tmp));
        return;
    }

    if( core_check_arg_type(theEnv, "undefmessage-handler", 2, ATOM, &tmp) == FALSE )
    {
        return;
    }

    mname = (ATOM_HN *)tmp.value;

    if( core_get_arg_count(theEnv) == 3 )
    {
        if( core_check_arg_type(theEnv, "undefmessage-handler", 3, ATOM, &tmp) == FALSE )
        {
            return;
        }

        tname = core_convert_data_to_string(tmp);

        if( strcmp(tname, "*") == 0 )
        {
            tname = NULL;
        }
    }
    else
    {
        tname = MessageHandlerData(theEnv)->hndquals[MPRIMARY];
    }

    WildDeleteHandler(theEnv, cls, mname, tname);
}

/***********************************************************
 *  NAME         : EnvUndefmessageHandler
 *  DESCRIPTION  : Deletes a handler from a class
 *  INPUTS       : 1) Class address    (Can be NULL)
 *              2) Handler index (can be 0)
 *  RETURNS      : 1 if successful, 0 otherwise
 *  SIDE EFFECTS : Handler deleted if possible
 *  NOTES        : None
 ***********************************************************/
globle int EnvUndefmessageHandler(void *theEnv, void *vptr, int mhi)
{
    DEFCLASS *cls;


    if( vptr == NULL )
    {
        if( mhi != 0 )
        {
            error_print_id(theEnv, "MSGCOM", 1, FALSE);
            print_router(theEnv, WERROR, "Incomplete message-handler specification for deletion.\n");
            return(0);
        }

        return(WildDeleteHandler(theEnv, NULL, NULL, NULL));
    }

    if( mhi == 0 )
    {
        return(WildDeleteHandler(theEnv, (DEFCLASS *)vptr, NULL, NULL));
    }

    cls = (DEFCLASS *)vptr;

    if( HandlersExecuting(cls))
    {
        HandlerDeleteError(theEnv, EnvGetDefclassName(theEnv, (void *)cls));
        return(0);
    }

    cls->handlers[mhi - 1].mark = 1;
    DeallocateMarkedHandlers(theEnv, cls);
    return(1);
}

#if DEBUGGING_FUNCTIONS

/*******************************************************************************
 *  NAME         : PPDefmessageHandlerCommand
 *  DESCRIPTION  : Displays the pretty-print form (if any) for a handler
 *  INPUTS       : None
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : None
 *  NOTES        : H/L Syntax: (ppdefmessage-handler <class> <message> [<type>])
 *******************************************************************************/
globle void PPDefmessageHandlerCommand(void *theEnv)
{
    core_data_object temp;
    ATOM_HN *csym, *msym;
    char *tname;
    DEFCLASS *cls = NULL;
    unsigned mtype;
    HANDLER *hnd;

    if( core_check_arg_type(theEnv, "ppdefmessage-handler", 1, ATOM, &temp) == FALSE )
    {
        return;
    }

    csym = lookup_atom(theEnv, core_convert_data_to_string(temp));

    if( core_check_arg_type(theEnv, "ppdefmessage-handler", 2, ATOM, &temp) == FALSE )
    {
        return;
    }

    msym = lookup_atom(theEnv, core_convert_data_to_string(temp));

    if( core_get_arg_count(theEnv) == 3 )
    {
        if( core_check_arg_type(theEnv, "ppdefmessage-handler", 3, ATOM, &temp) == FALSE )
        {
            return;
        }

        tname = core_convert_data_to_string(temp);
    }
    else
    {
        tname = MessageHandlerData(theEnv)->hndquals[MPRIMARY];
    }

    mtype = HandlerType(theEnv, "ppdefmessage-handler", tname);

    if( mtype == MERROR )
    {
        core_set_eval_error(theEnv, TRUE);
        return;
    }

    if( csym != NULL )
    {
        cls = LookupDefclassByMdlOrScope(theEnv, to_string(csym));
    }

    if(((cls == NULL) || (msym == NULL)) ? TRUE :
       ((hnd = FindHandlerByAddress(cls, msym, (unsigned)mtype)) == NULL))
    {
        error_print_id(theEnv, "MSGCOM", 2, FALSE);
        print_router(theEnv, WERROR, "Unable to find message-handler ");
        print_router(theEnv, WERROR, to_string(msym));
        print_router(theEnv, WERROR, " ");
        print_router(theEnv, WERROR, tname);
        print_router(theEnv, WERROR, " for class ");
        print_router(theEnv, WERROR, to_string(csym));
        print_router(theEnv, WERROR, " in function ppdefmessage-handler.\n");
        core_set_eval_error(theEnv, TRUE);
        return;
    }

    if( hnd->pp != NULL )
    {
        core_print_chunkify(theEnv, WDISPLAY, hnd->pp);
    }
}

/*****************************************************************************
 *  NAME         : ListDefmessageHandlersCommand
 *  DESCRIPTION  : Depending on arguments, does lists handlers which
 *                match restrictions
 *  INPUTS       : None
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : None
 *  NOTES        : H/L Syntax: (list-defmessage-handlers [<class> [inherit]]))
 *****************************************************************************/
globle void ListDefmessageHandlersCommand(void *theEnv)
{
    int inhp;
    void *clsptr;

    if( core_get_arg_count(theEnv) == 0 )
    {
        EnvListDefmessageHandlers(theEnv, WDISPLAY, NULL, 0);
    }
    else
    {
        clsptr = ClassInfoFnxArgs(theEnv, "list-defmessage-handlers", &inhp);

        if( clsptr == NULL )
        {
            return;
        }

        EnvListDefmessageHandlers(theEnv, WDISPLAY, clsptr, inhp);
    }
}

/********************************************************************
 *  NAME         : PreviewSendCommand
 *  DESCRIPTION  : Displays a list of the core for a message describing
 *                shadows,etc.
 *  INPUTS       : None
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Temporary core created and destroyed
 *  NOTES        : H/L Syntax: (preview-send <class> <msg>)
 ********************************************************************/
globle void PreviewSendCommand(void *theEnv)
{
    DEFCLASS *cls;
    core_data_object temp;

    /* =============================
     *  Get the class for the message
     *  ============================= */
    if( core_check_arg_type(theEnv, "preview-send", 1, ATOM, &temp) == FALSE )
    {
        return;
    }

    cls = LookupDefclassByMdlOrScope(theEnv, core_convert_data_to_string(temp));

    if( cls == NULL )
    {
        ClassExistError(theEnv, "preview-send", to_string(temp.value));
        return;
    }

    if( core_check_arg_type(theEnv, "preview-send", 2, ATOM, &temp) == FALSE )
    {
        return;
    }

    EnvPreviewSend(theEnv, WDISPLAY, (void *)cls, core_convert_data_to_string(temp));
}

/********************************************************
 *  NAME         : EnvGetDefmessageHandlerPPForm
 *  DESCRIPTION  : Gets a message-handler pretty print form
 *  INPUTS       : 1) Address of the handler's class
 *              2) Index of the handler
 *  RETURNS      : TRUE if printable, FALSE otherwise
 *  SIDE EFFECTS : None
 *  NOTES        : None
 ********************************************************/
#if WIN_BTC
#pragma argsused
#endif
globle char *EnvGetDefmessageHandlerPPForm(void *theEnv, void *ptr, int theIndex)
{
#if MAC_MCW || WIN_MCW || MAC_XCD
#pragma unused(theEnv)
#endif

    return(((DEFCLASS *)ptr)->handlers[theIndex - 1].pp);
}

/*******************************************************************
 *  NAME         : EnvListDefmessageHandlers
 *  DESCRIPTION  : Lists message-handlers for a class
 *  INPUTS       : 1) The logical name of the output
 *              2) Class name (NULL to display all handlers)
 *              3) A flag indicating whether to list inherited
 *                 handlers or not
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : None
 *  NOTES        : None
 *******************************************************************/
globle void EnvListDefmessageHandlers(void *theEnv, char *logName, void *vptr, int inhp)
{
    DEFCLASS *cls;
    long cnt;
    PACKED_CLASS_LINKS plinks;

    if( vptr != NULL )
    {
        cls = (DEFCLASS *)vptr;

        if( inhp )
        {
            cnt = DisplayHandlersInLinks(theEnv, logName, &cls->allSuperclasses, 0);
        }
        else
        {
            plinks.classCount = 1;
            plinks.classArray = &cls;
            cnt = DisplayHandlersInLinks(theEnv, logName, &plinks, 0);
        }
    }
    else
    {
        plinks.classCount = 1;
        cnt = 0L;

        for( cls = (DEFCLASS *)EnvGetNextDefclass(theEnv, NULL) ;
             cls != NULL ;
             cls = (DEFCLASS *)EnvGetNextDefclass(theEnv, (void *)cls))
        {
            plinks.classArray = &cls;
            cnt += DisplayHandlersInLinks(theEnv, logName, &plinks, 0);
        }
    }

    core_print_tally(theEnv, logName, cnt, "message-handler", "message-handlers");
}

/********************************************************************
 *  NAME         : EnvPreviewSend
 *  DESCRIPTION  : Displays a list of the core for a message describing
 *                shadows,etc.
 *  INPUTS       : 1) Logical name of output
 *              2) Class pointer
 *              3) Message name-string
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Temporary core created and destroyed
 *  NOTES        : None
 ********************************************************************/
globle void EnvPreviewSend(void *theEnv, char *logical_name, void *clsptr, char *msgname)
{
    HANDLER_LINK *core;
    ATOM_HN *msym;

    msym = lookup_atom(theEnv, msgname);

    if( msym == NULL )
    {
        return;
    }

    core = FindPreviewApplicableHandlers(theEnv, (DEFCLASS *)clsptr, msym);

    if( core != NULL )
    {
        DisplayCore(theEnv, logical_name, core, 0);
        DestroyHandlerLinks(theEnv, core);
    }
}

/****************************************************
 *  NAME         : DisplayHandlersInLinks
 *  DESCRIPTION  : Recursively displays all handlers
 *               for an array of classes
 *  INPUTS       : 1) The logical name of the output
 *              2) The packed class links
 *              3) The index to print from the links
 *  RETURNS      : The number of handlers printed
 *  SIDE EFFECTS : None
 *  NOTES        : Used by DescribeClass()
 ****************************************************/
globle long DisplayHandlersInLinks(void *theEnv, char *logName, PACKED_CLASS_LINKS *plinks, int theIndex)
{
    long i;
    long cnt;

    cnt = (long)plinks->classArray[theIndex]->handlerCount;

    if(((int)theIndex) < (plinks->classCount - 1))
    {
        cnt += DisplayHandlersInLinks(theEnv, logName, plinks, theIndex + 1);
    }

    for( i = 0 ; i < plinks->classArray[theIndex]->handlerCount ; i++ )
    {
        PrintHandler(theEnv, logName, &plinks->classArray[theIndex]->handlers[i], TRUE);
    }

    return(cnt);
}

#endif

/* =========================================
 *****************************************
 *       INTERNALLY VISIBLE FUNCTIONS
 *  =========================================
 ***************************************** */


/**********************************************************
 *  NAME         : CreateSystemHandlers
 *  DESCRIPTION  : Attachess the system message-handlers
 *              after a (clear)
 *  INPUTS       : None
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : System handlers created
 *  NOTES        : Must be called after CreateSystemClasses()
 **********************************************************/
static void CreateSystemHandlers(void *theEnv)
{
    NewSystemHandler(theEnv, USER_TYPE_NAME, INIT_STRING, "init-slots", 0);
    NewSystemHandler(theEnv, USER_TYPE_NAME, DELETE_STRING, "delete-instance", 0);
    NewSystemHandler(theEnv, USER_TYPE_NAME, CREATE_STRING, "(create-instance)", 0);

#if DEBUGGING_FUNCTIONS
    NewSystemHandler(theEnv, USER_TYPE_NAME, PRINT_STRING, "ppinstance", 0);
#endif

    NewSystemHandler(theEnv, USER_TYPE_NAME, DIRECT_MODIFY_STRING, "(direct-modify)", 1);
    NewSystemHandler(theEnv, USER_TYPE_NAME, MSG_MODIFY_STRING, "(message-modify)", 1);
    NewSystemHandler(theEnv, USER_TYPE_NAME, DIRECT_DUPLICATE_STRING, "(direct-duplicate)", 2);
    NewSystemHandler(theEnv, USER_TYPE_NAME, MSG_DUPLICATE_STRING, "(message-duplicate)", 2);
}

/************************************************************
 *  NAME         : WildDeleteHandler
 *  DESCRIPTION  : Deletes a handler from a class
 *  INPUTS       : 1) Class address (Can be NULL)
 *              2) Message Handler Name (Can be NULL)
 *              3) Type name ("primary", etc.)
 *  RETURNS      : 1 if successful, 0 otherwise
 *  SIDE EFFECTS : Handler deleted if possible
 *  NOTES        : None
 ************************************************************/
static int WildDeleteHandler(void *theEnv, DEFCLASS *cls, ATOM_HN *msym, char *tname)
{
    int mtype;

    if( msym == NULL )
    {
        msym = (ATOM_HN *)store_atom(theEnv, "*");
    }

    if( tname != NULL )
    {
        mtype = (int)HandlerType(theEnv, "undefmessage-handler", tname);

        if( mtype == MERROR )
        {
            return(0);
        }
    }
    else
    {
        mtype = -1;
    }

    if( cls == NULL )
    {
        int success = 1;

        for( cls = (DEFCLASS *)EnvGetNextDefclass(theEnv, NULL) ;
             cls != NULL ;
             cls = (DEFCLASS *)EnvGetNextDefclass(theEnv, (void *)cls))
        {
            if( DeleteHandler(theEnv, cls, msym, mtype, FALSE) == 0 )
            {
                success = 0;
            }
        }

        return(success);
    }

    return(DeleteHandler(theEnv, cls, msym, mtype, TRUE));
}

#if DEBUGGING_FUNCTIONS

/******************************************************************
 *  NAME         : DefmessageHandlerWatchAccess
 *  DESCRIPTION  : Parses a list of class names passed by
 *              AddWatchItem() and sets the traces accordingly
 *  INPUTS       : 1) A code indicating which trace flag is to be set
 *                 0 - Watch instance creation/deletion
 *                 1 - Watch slot changes to instances
 *              2) The value to which to set the trace flags
 *              3) A list of expressions containing the names
 *                 of the classes for which to set traces
 *  RETURNS      : TRUE if all OK, FALSE otherwise
 *  SIDE EFFECTS : Watch flags set in specified classes
 *  NOTES        : Accessory function for AddWatchItem()
 ******************************************************************/
#if WIN_BTC
#pragma argsused
#endif
static unsigned DefmessageHandlerWatchAccess(void *theEnv, int code, unsigned newState, core_expression_object *argExprs)
{
#if MAC_MCW || WIN_MCW || MAC_XCD
#pragma unused(code)
#endif

    if( newState )
    {
        return(DefmessageHandlerWatchSupport(theEnv, "watch", NULL, newState,
                                             NULL, EnvSetDefmessageHandlerWatch, argExprs));
    }
    else
    {
        return(DefmessageHandlerWatchSupport(theEnv, "unwatch", NULL, newState,
                                             NULL, EnvSetDefmessageHandlerWatch, argExprs));
    }
}

/***********************************************************************
 *  NAME         : DefmessageHandlerWatchPrint
 *  DESCRIPTION  : Parses a list of class names passed by
 *              AddWatchItem() and displays the traces accordingly
 *  INPUTS       : 1) The logical name of the output
 *              2) A code indicating which trace flag is to be examined
 *                 0 - Watch instance creation/deletion
 *                 1 - Watch slot changes to instances
 *              3) A list of expressions containing the names
 *                 of the classes for which to examine traces
 *  RETURNS      : TRUE if all OK, FALSE otherwise
 *  SIDE EFFECTS : Watch flags displayed for specified classes
 *  NOTES        : Accessory function for AddWatchItem()
 ***********************************************************************/
#if WIN_BTC
#pragma argsused
#endif
static unsigned DefmessageHandlerWatchPrint(void *theEnv, char *logName, int code, core_expression_object *argExprs)
{
#if MAC_MCW || WIN_MCW || MAC_XCD
#pragma unused(code)
#endif
    return(DefmessageHandlerWatchSupport(theEnv, "list-watch-items", logName, -1,
                                         PrintHandlerWatchFlag, NULL, argExprs));
}

/*******************************************************
 *  NAME         : DefmessageHandlerWatchSupport
 *  DESCRIPTION  : Sets or displays handlers specified
 *  INPUTS       : 1) The calling function name
 *              2) The logical output name for displays
 *                 (can be NULL)
 *              4) The new set state (can be -1)
 *              5) The print function (can be NULL)
 *              6) The trace function (can be NULL)
 *              7) The handlers expression list
 *  RETURNS      : TRUE if all OK,
 *              FALSE otherwise
 *  SIDE EFFECTS : Handler trace flags set or displayed
 *  NOTES        : None
 *******************************************************/
static unsigned DefmessageHandlerWatchSupport(void *theEnv, char *funcName, char *logName, int newState, void (*fn_print)(void *, char *, void *, int), void (*traceFunc)(void *, int, void *, int), core_expression_object *argExprs)
{
    struct module_definition *module_def;
    void *theClass;
    char *theHandlerStr;
    int theType;
    int argIndex = 2;
    core_data_object tmpData;

    /* ===============================
     *  If no handlers are specified,
     *  show the trace for all handlers
     *  in all handlers
     *  =============================== */
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

            theClass = EnvGetNextDefclass(theEnv, NULL);

            while( theClass != NULL )
            {
                if( WatchClassHandlers(theEnv, theClass, NULL, -1, logName, newState,
                                       TRUE, fn_print, traceFunc) == FALSE )
                {
                    return(FALSE);
                }

                theClass = EnvGetNextDefclass(theEnv, theClass);
            }

            module_def = (struct module_definition *)get_next_module(theEnv, (void *)module_def);
        }

        restore_current_module(theEnv);
        return(TRUE);
    }

    /* ================================================
     *  Set or show the traces for the specified handler
     *  ================================================ */
    while( argExprs != NULL )
    {
        if( core_eval_expression(theEnv, argExprs, &tmpData))
        {
            return(FALSE);
        }

        if( tmpData.type != ATOM )
        {
            report_explicit_type_error(theEnv, funcName, argIndex, "class name");
            return(FALSE);
        }

        theClass = (void *)LookupDefclassByMdlOrScope(theEnv, core_convert_data_to_string(tmpData));

        if( theClass == NULL )
        {
            report_explicit_type_error(theEnv, funcName, argIndex, "class name");
            return(FALSE);
        }

        if( core_get_next_arg(argExprs) != NULL )
        {
            argExprs = core_get_next_arg(argExprs);
            argIndex++;

            if( core_eval_expression(theEnv, argExprs, &tmpData))
            {
                return(FALSE);
            }

            if( tmpData.type != ATOM )
            {
                report_explicit_type_error(theEnv, funcName, argIndex, "handler name");
                return(FALSE);
            }

            theHandlerStr = core_convert_data_to_string(tmpData);

            if( core_get_next_arg(argExprs) != NULL )
            {
                argExprs = core_get_next_arg(argExprs);
                argIndex++;

                if( core_eval_expression(theEnv, argExprs, &tmpData))
                {
                    return(FALSE);
                }

                if( tmpData.type != ATOM )
                {
                    report_explicit_type_error(theEnv, funcName, argIndex, "handler type");
                    return(FALSE);
                }

                if((theType = (int)HandlerType(theEnv, funcName, core_convert_data_to_string(tmpData))) == MERROR )
                {
                    return(FALSE);
                }
            }
            else
            {
                theType = -1;
            }
        }
        else
        {
            theHandlerStr = NULL;
            theType = -1;
        }

        if( WatchClassHandlers(theEnv, theClass, theHandlerStr, theType, logName,
                               newState, FALSE, fn_print, traceFunc) == FALSE )
        {
            report_explicit_type_error(theEnv, funcName, argIndex, "handler");
            return(FALSE);
        }

        argIndex++;
        argExprs = core_get_next_arg(argExprs);
    }

    return(TRUE);
}

/*******************************************************
 *  NAME         : WatchClassHandlers
 *  DESCRIPTION  : Sets or displays handlers specified
 *  INPUTS       : 1) The class
 *              2) The handler name (or NULL wildcard)
 *              3) The handler type (or -1 wildcard)
 *              4) The logical output name for displays
 *                 (can be NULL)
 *              5) The new set state (can be -1)
 *              6) The print function (can be NULL)
 *              7) The trace function (can be NULL)
 *  RETURNS      : TRUE if all OK,
 *              FALSE otherwise
 *  SIDE EFFECTS : Handler trace flags set or displayed
 *  NOTES        : None
 *******************************************************/
static unsigned WatchClassHandlers(void *theEnv, void *theClass, char *theHandlerStr, int theType, char *logName, int newState, int indentp, void (*fn_print)(void *, char *, void *, int), void (*traceFunc)(void *, int, void *, int))
{
    unsigned theHandler;
    int found = FALSE;

    theHandler = EnvGetNextDefmessageHandler(theEnv, theClass, 0);

    while( theHandler != 0 )
    {
        if((theType == -1) ? TRUE :
           (theType == (int)((DEFCLASS *)theClass)->handlers[theHandler - 1].type))
        {
            if((theHandlerStr == NULL) ? TRUE :
               (strcmp(theHandlerStr, EnvGetDefmessageHandlerName(theEnv, theClass, theHandler)) == 0))
            {
                if( traceFunc != NULL )
                {
                    (*traceFunc)(theEnv, newState, theClass, theHandler);
                }
                else
                {
                    if( indentp )
                    {
                        print_router(theEnv, logName, "   ");
                    }

                    (*fn_print)(theEnv, logName, theClass, theHandler);
                }

                found = TRUE;
            }
        }

        theHandler = EnvGetNextDefmessageHandler(theEnv, theClass, theHandler);
    }

    if((theHandlerStr != NULL) && (theType != -1) && (found == FALSE))
    {
        return(FALSE);
    }

    return(TRUE);
}

/***************************************************
 *  NAME         : PrintHandlerWatchFlag
 *  DESCRIPTION  : Displays trace value for handler
 *  INPUTS       : 1) The logical name of the output
 *              2) The class
 *              3) The handler index
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : None
 *  NOTES        : None
 ***************************************************/
static void PrintHandlerWatchFlag(void *theEnv, char *logName, void *theClass, int theHandler)
{
    print_router(theEnv, logName, EnvGetDefclassName(theEnv, theClass));
    print_router(theEnv, logName, " ");
    print_router(theEnv, logName, EnvGetDefmessageHandlerName(theEnv, theClass, theHandler));
    print_router(theEnv, logName, " ");
    print_router(theEnv, logName, EnvGetDefmessageHandlerType(theEnv, theClass, theHandler));

    if( EnvGetDefmessageHandlerWatch(theEnv, theClass, theHandler))
    {
        print_router(theEnv, logName, " = on\n");
    }
    else
    {
        print_router(theEnv, logName, " = off\n");
    }
}

#endif /* DEBUGGING_FUNCTIONS */
#endif
