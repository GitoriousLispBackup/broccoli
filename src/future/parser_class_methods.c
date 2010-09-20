/* =========================================
 *****************************************
 *            EXTERNAL DEFINITIONS
 *  =========================================
 ***************************************** */
#include "setup.h"

#if OBJECT_SYSTEM

#include <string.h>

#include "classes_kernel.h"
#include "funcs_class.h"
#include "core_memory.h"
#include "core_constructs.h"
#include "parser_constructs.h"
#include "constraints_query.h"
#include "core_environment.h"
#include "parser_expressions.h"
#include "funcs_instance.h"
#include "classes_methods_kernel.h"
#include "funcs_method.h"
#include "core_pretty_print.h"
#include "core_functions_util.h"
#include "router.h"
#include "core_scanner.h"
#include "router_string.h"

#define __PARSER_CLASS_METHODS_SOURCE__
#include "parser_class_methods.h"

/* =========================================
 *****************************************
 *                CONSTANTS
 *  =========================================
 ***************************************** */
#define SELF_LEN         4
#define SELF_SLOT_REF    ':'

/* =========================================
 *****************************************
 *   INTERNALLY VISIBLE FUNCTION HEADERS
 *  =========================================
 ***************************************** */

static BOOLEAN IsParameterSlotReference(void *, char *);
static int     SlotReferenceVar(void *, core_expression_object *, void *);
static int     BindSlotReference(void *, core_expression_object *, void *);
static SLOT_DESC *CheckSlotReference(void *, DEFCLASS *, int, void *, BOOLEAN, core_expression_object *);
static void GenHandlerSlotReference(void *, core_expression_object *, unsigned short, SLOT_DESC *);

/* =========================================
 *****************************************
 *       EXTERNALLY VISIBLE FUNCTIONS
 *  =========================================
 ***************************************** */

/***********************************************************************
 *  NAME         : ParseDefmessageHandler
 *  DESCRIPTION  : Parses a message-handler for a class of objects
 *  INPUTS       : The logical name of the input source
 *  RETURNS      : FALSE if successful parse, TRUE otherwise
 *  SIDE EFFECTS : Handler allocated and inserted into class
 *  NOTES        : H/L Syntax:
 *
 *              (defmessage-handler <class> <name> [<type>] [<comment>]
 *                 (<params>)
 *                 <action>*)
 *
 *              <params> ::= <var>* | <var>* $?<name>
 ***********************************************************************/
globle int ParseDefmessageHandler(void *theEnv, char *readSource)
{
    DEFCLASS *cls;
    ATOM_HN *cname, *mname, *wildcard;
    unsigned mtype = MPRIMARY;
    int min, max, error, lvars;
    core_expression_object *hndParams, *actions;
    HANDLER *hnd;

    core_set_pp_buffer_status(theEnv, ON);
    core_flush_pp_buffer(theEnv);
    core_pp_indent_depth(theEnv, 3);
    core_save_pp_buffer(theEnv, "(defmessage-handler ");

    cname = parse_construct_head(theEnv, readSource, &DefclassData(theEnv)->ObjectParseToken, "defmessage-handler",
                                       NULL, NULL, "~", TRUE, FALSE, TRUE);

    if( cname == NULL )
    {
        return(TRUE);
    }

    cls = LookupDefclassByMdlOrScope(theEnv, to_string(cname));

    if( cls == NULL )
    {
        error_print_id(theEnv, "MSGPSR", 1, FALSE);
        print_router(theEnv, WERROR, "A class must be defined before its message-handlers.\n");
        return(TRUE);
    }

    if((cls == DefclassData(theEnv)->PrimitiveClassMap[INSTANCE_NAME]) ||
       (cls == DefclassData(theEnv)->PrimitiveClassMap[INSTANCE_ADDRESS]) ||
       (cls == DefclassData(theEnv)->PrimitiveClassMap[INSTANCE_NAME]->directSuperclasses.classArray[0]))
    {
        error_print_id(theEnv, "MSGPSR", 8, FALSE);
        print_router(theEnv, WERROR, "Message-handlers cannot be attached to the class ");
        print_router(theEnv, WERROR, EnvGetDefclassName(theEnv, (void *)cls));
        print_router(theEnv, WERROR, ".\n");
        return(TRUE);
    }

    if( HandlersExecuting(cls))
    {
        error_print_id(theEnv, "MSGPSR", 2, FALSE);
        print_router(theEnv, WERROR, "Cannot (re)define message-handlers during execution of \n");
        print_router(theEnv, WERROR, "  other message-handlers for the same class.\n");
        return(TRUE);
    }

    if( core_get_type(DefclassData(theEnv)->ObjectParseToken) != ATOM )
    {
        error_syntax(theEnv, "defmessage-handler");
        return(TRUE);
    }

    core_backup_pp_buffer(theEnv);
    core_backup_pp_buffer(theEnv);
    core_save_pp_buffer(theEnv, " ");
    core_save_pp_buffer(theEnv, DefclassData(theEnv)->ObjectParseToken.pp);
    core_save_pp_buffer(theEnv, " ");
    mname = (ATOM_HN *)core_get_value(DefclassData(theEnv)->ObjectParseToken);
    core_get_token(theEnv, readSource, &DefclassData(theEnv)->ObjectParseToken);

    if( core_get_type(DefclassData(theEnv)->ObjectParseToken) != LPAREN )
    {
        core_save_pp_buffer(theEnv, " ");

        if( core_get_type(DefclassData(theEnv)->ObjectParseToken) != STRING )
        {
            if( core_get_type(DefclassData(theEnv)->ObjectParseToken) != ATOM )
            {
                error_syntax(theEnv, "defmessage-handler");
                return(TRUE);
            }

            mtype = HandlerType(theEnv, "defmessage-handler", core_convert_data_to_string(DefclassData(theEnv)->ObjectParseToken));

            if( mtype == MERROR )
            {
                return(TRUE);
            }

            core_get_token(theEnv, readSource, &DefclassData(theEnv)->ObjectParseToken);

            if( core_get_type(DefclassData(theEnv)->ObjectParseToken) == STRING )
            {
                core_save_pp_buffer(theEnv, " ");
                core_get_token(theEnv, readSource, &DefclassData(theEnv)->ObjectParseToken);
            }
        }
        else
        {
            core_save_pp_buffer(theEnv, " ");
            core_get_token(theEnv, readSource, &DefclassData(theEnv)->ObjectParseToken);
        }
    }

    core_backup_pp_buffer(theEnv);
    core_backup_pp_buffer(theEnv);
    core_inject_ws_into_pp_buffer(theEnv);
    core_save_pp_buffer(theEnv, DefclassData(theEnv)->ObjectParseToken.pp);

    hnd = FindHandlerByAddress(cls, mname, mtype);

    if( core_is_verbose_loading(theEnv) && mopIsWatchCompilations(theEnv))
    {
        print_router(theEnv, WDIALOG, "   Handler ");
        print_router(theEnv, WDIALOG, to_string(mname));
        print_router(theEnv, WDIALOG, " ");
        print_router(theEnv, WDIALOG, MessageHandlerData(theEnv)->hndquals[mtype]);

        if( hnd == NULL )
        {
            print_router(theEnv, WDIALOG, " defined.\n");
        }
        else
        {
            print_router(theEnv, WDIALOG, " redefined.\n");
        }
    }

    if((hnd != NULL) ? hnd->system : FALSE )
    {
        error_print_id(theEnv, "MSGPSR", 3, FALSE);
        print_router(theEnv, WERROR, "System message-handlers may not be modified.\n");
        return(TRUE);
    }

    hndParams = core_generate_constant(theEnv, ATOM, (void *)MessageHandlerData(theEnv)->SELF_ATOM);
    hndParams = core_parse_function_args(theEnv, readSource, &DefclassData(theEnv)->ObjectParseToken, hndParams,
                                    &wildcard, &min, &max, &error, IsParameterSlotReference);

    if( error )
    {
        return(TRUE);
    }

    core_inject_ws_into_pp_buffer(theEnv);
    core_get_expression_data(theEnv)->return_context = TRUE;
    actions = core_parse_function_actions(theEnv, "message-handler", readSource,
                               &DefclassData(theEnv)->ObjectParseToken, hndParams, wildcard,
                               SlotReferenceVar, BindSlotReference, &lvars,
                               (void *)cls);

    if( actions == NULL )
    {
        core_return_expression(theEnv, hndParams);
        return(TRUE);
    }

    if( core_get_type(DefclassData(theEnv)->ObjectParseToken) != RPAREN )
    {
        error_syntax(theEnv, "defmessage-handler");
        core_return_expression(theEnv, hndParams);
        core_return_packed_expression(theEnv, actions);
        return(TRUE);
    }

    core_backup_pp_buffer(theEnv);
    core_backup_pp_buffer(theEnv);
    core_save_pp_buffer(theEnv, DefclassData(theEnv)->ObjectParseToken.pp);
    core_save_pp_buffer(theEnv, "\n");

    /* ===================================================
     *  If we're only checking syntax, don't add the
     *  successfully parsed defmessage-handler to the KB.
     *  =================================================== */

    if( core_construct_data(theEnv)->check_syntax )
    {
        core_return_expression(theEnv, hndParams);
        core_return_packed_expression(theEnv, actions);
        return(FALSE);
    }

    if( hnd != NULL )
    {
        core_decrement_expression(theEnv, hnd->actions);
        core_return_packed_expression(theEnv, hnd->actions);

        if( hnd->pp != NULL )
        {
            core_mem_release(theEnv, (void *)hnd->pp,
               (sizeof(char) * (strlen(hnd->pp) + 1)));
        }
    }
    else
    {
        hnd = InsertHandlerHeader(theEnv, cls, mname, (int)mtype);
        inc_atom_count(hnd->name);
    }

    core_return_expression(theEnv, hndParams);

    hnd->minParams = (short)min;
    hnd->maxParams = (short)max;
    hnd->localVarCount = (short)lvars;
    hnd->actions = actions;
    core_increment_expression(theEnv, hnd->actions);
#if DEBUGGING_FUNCTIONS

    /* ===================================================
     *  Old handler trace status is automatically preserved
     *  =================================================== */
    if( EnvGetConserveMemory(theEnv) == FALSE )
    {
        hnd->pp = core_copy_pp_buffer(theEnv);
    }
    else
#endif
    hnd->pp = NULL;
    return(FALSE);
}

/*******************************************************************************
 *  NAME         : CreateGetAndPutHandlers
 *  DESCRIPTION  : Creates two message-handlers with
 *               the following syntax for the slot:
 *
 *              (defmessage-handler <class> get-<slot-name> primary ()
 *                 ?self:<slot-name>)
 *
 *              For single-field slots:
 *
 *              (defmessage-handler <class> put-<slot-name> primary (?value)
 *                 (bind ?self:<slot-name> ?value))
 *
 *              For list slots:
 *
 *              (defmessage-handler <class> put-<slot-name> primary ($?value)
 *                 (bind ?self:<slot-name> ?value))
 *
 *  INPUTS       : The class slot descriptor
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Message-handlers created
 *  NOTES        : A put handler is not created for read-only slots
 *******************************************************************************/
globle void CreateGetAndPutHandlers(void *theEnv, SLOT_DESC *sd)
{
    char *className, *slotName;
    size_t bufsz;
    char *buf, *handlerRouter = "*** Default Public Handlers ***";
    int oldPWL, oldCM;
    char *oldRouter;
    char *oldString;
    long oldIndex;

    if((sd->createReadAccessor == 0) && (sd->createWriteAccessor == 0))
    {
        return;
    }

    className = to_string(sd->cls->header.name);
    slotName = to_string(sd->slotName->name);

    bufsz = (sizeof(char) * (strlen(className) + (strlen(slotName) * 2) + 80));
    buf = (char *)core_mem_alloc_no_init(theEnv, bufsz);

    oldPWL = core_is_verbose_loading(theEnv);
    core_set_verbose_loading(theEnv, FALSE);
    oldCM = EnvSetConserveMemory(theEnv, TRUE);

    if( sd->createReadAccessor )
    {
        sysdep_sprintf(buf, "%s get-%s () ?self:%s)", className, slotName, slotName);

        oldRouter = get_router_data(theEnv)->fast_get_ch;
        oldString = get_router_data(theEnv)->fast_get_str;
        oldIndex = get_router_data(theEnv)->fast_get_inde;

        get_router_data(theEnv)->fast_get_ch = handlerRouter;
        get_router_data(theEnv)->fast_get_inde = 0;
        get_router_data(theEnv)->fast_get_str = buf;

        ParseDefmessageHandler(theEnv, handlerRouter);
        core_delete_pp_buffer(theEnv);

        /*
         * if (open_string_source(theEnv,handlerRouter,buf,0))
         * {
         * ParseDefmessageHandler(handlerRouter);
         * core_delete_pp_buffer();
         * close_string_source(theEnv,handlerRouter);
         * }
         */
        get_router_data(theEnv)->fast_get_ch = oldRouter;
        get_router_data(theEnv)->fast_get_inde = oldIndex;
        get_router_data(theEnv)->fast_get_str = oldString;
    }

    if( sd->createWriteAccessor )
    {
        sysdep_sprintf(buf, "%s put-%s ($?value) (bind ?self:%s ?value))",
                   className, slotName, slotName);

        oldRouter = get_router_data(theEnv)->fast_get_ch;
        oldString = get_router_data(theEnv)->fast_get_str;
        oldIndex = get_router_data(theEnv)->fast_get_inde;

        get_router_data(theEnv)->fast_get_ch = handlerRouter;
        get_router_data(theEnv)->fast_get_inde = 0;
        get_router_data(theEnv)->fast_get_str = buf;

        ParseDefmessageHandler(theEnv, handlerRouter);
        core_delete_pp_buffer(theEnv);

        /*
         *   if (open_string_source(theEnv,handlerRouter,buf,0))
         *     {
         *      ParseDefmessageHandler(handlerRouter);
         *      core_delete_pp_buffer();
         *      close_string_source(theEnv,handlerRouter);
         *     }
         */
        get_router_data(theEnv)->fast_get_ch = oldRouter;
        get_router_data(theEnv)->fast_get_inde = oldIndex;
        get_router_data(theEnv)->fast_get_str = oldString;
    }

    core_set_verbose_loading(theEnv, oldPWL);
    EnvSetConserveMemory(theEnv, oldCM);

    core_mem_release(theEnv, (void *)buf, bufsz);
}

/* =========================================
 *****************************************
 *       INTERNALLY VISIBLE FUNCTIONS
 *  =========================================
 ***************************************** */

/*****************************************************************
 *  NAME         : IsParameterSlotReference
 *  DESCRIPTION  : Determines if a message-handler parameter is of
 *              the form ?self:<name>, which is not allowed since
 *              this is slot reference syntax
 *  INPUTS       : The paramter name
 *  RETURNS      : TRUE if the parameter is a slot reference,
 *              FALSE otherwise
 *  SIDE EFFECTS : None
 *  NOTES        : None
 *****************************************************************/
static BOOLEAN IsParameterSlotReference(void *theEnv, char *pname)
{
    if((strncmp(pname, SELF_STRING, SELF_LEN) == 0) ?
       (pname[SELF_LEN] == SELF_SLOT_REF) : FALSE )
    {
        error_print_id(theEnv, "MSGPSR", 4, FALSE);
        print_router(theEnv, WERROR, "Illegal slot reference in parameter list.\n");
        return(TRUE);
    }

    return(FALSE);
}

/****************************************************************************
 *  NAME         : SlotReferenceVar
 *  DESCRIPTION  : Replaces direct slot references in handler body
 *                with special function calls to reference active instance
 *                at run-time
 *              The slot in in the class bound at parse-time is always
 *                referenced (early binding).
 *              Slot references of the form ?self:<name> directly reference
 *                arguments[0] (the message object - ?self) to
 *                find the specified slot at run-time
 *  INPUTS       : 1) Variable expression
 *              2) The class of the handler being parsed
 *  RETURNS      : 0 if not recognized, 1 if so, -1 on errors
 *  SIDE EFFECTS : Handler body SCALAR_VARIABLE and LIST_VARIABLE replaced with
 *                direct slot access function
 *  NOTES        : Objects are allowed to directly access their own slots
 *              without sending a message to themselves.  Since the object
 *              is "within the boundary of its internals", this does not
 *              violate the encapsulation principle of OOP.
 ****************************************************************************/
static int SlotReferenceVar(void *theEnv, core_expression_object *varexp, void *userBuffer)
{
    struct token itkn;
    int oldpp;
    SLOT_DESC *sd;

    if((varexp->type != SCALAR_VARIABLE) && (varexp->type != LIST_VARIABLE))
    {
        return(0);
    }

    if((strncmp(to_string(varexp->value), SELF_STRING, SELF_LEN) == 0) ?
       (to_string(varexp->value)[SELF_LEN] == SELF_SLOT_REF) : FALSE )
    {
        open_string_source(theEnv, "hnd-var", to_string(varexp->value) + SELF_LEN + 1, 0);
        oldpp = core_get_pp_buffer_status(theEnv);
        core_set_pp_buffer_status(theEnv, OFF);
        core_get_token(theEnv, "hnd-var", &itkn);
        core_set_pp_buffer_status(theEnv, oldpp);
        close_string_source(theEnv, "hnd-var");

        if( itkn.type != STOP )
        {
            sd = CheckSlotReference(theEnv, (DEFCLASS *)userBuffer, itkn.type, itkn.value,
                                    FALSE, NULL);

            if( sd == NULL )
            {
                return(-1);
            }

            GenHandlerSlotReference(theEnv, varexp, HANDLER_GET, sd);
            return(1);
        }
    }

    return(0);
}

/****************************************************************************
 *  NAME         : BindSlotReference
 *  DESCRIPTION  : Replaces direct slot binds in handler body with special
 *              function calls to reference active instance at run-time
 *              The slot in in the class bound at parse-time is always
 *              referenced (early binding).
 *              Slot references of the form ?self:<name> directly reference
 *                arguments[0] (the message object - ?self) to
 *                find the specified slot at run-time
 *  INPUTS       : 1) Variable expression
 *              2) The class for the message-handler being parsed
 *  RETURNS      : 0 if not recognized, 1 if so, -1 on errors
 *  SIDE EFFECTS : Handler body "bind" call replaced with  direct slot access
 *                function
 *  NOTES        : Objects are allowed to directly access their own slots
 *              without sending a message to themselves.  Since the object
 *              is "within the boundary of its internals", this does not
 *              violate the encapsulation principle of OOP.
 ****************************************************************************/
static int BindSlotReference(void *theEnv, core_expression_object *bindExp, void *userBuffer)
{
    char *bindName;
    struct token itkn;
    int oldpp;
    SLOT_DESC *sd;
    core_expression_object *saveExp;

    bindName = to_string(bindExp->args->value);

    if( strcmp(bindName, SELF_STRING) == 0 )
    {
        error_print_id(theEnv, "MSGPSR", 5, FALSE);
        print_router(theEnv, WERROR, "Active instance parameter cannot be changed.\n");
        return(-1);
    }

    if((strncmp(bindName, SELF_STRING, SELF_LEN) == 0) ?
       (bindName[SELF_LEN] == SELF_SLOT_REF) : FALSE )
    {
        open_string_source(theEnv, "hnd-var", bindName + SELF_LEN + 1, 0);
        oldpp = core_get_pp_buffer_status(theEnv);
        core_set_pp_buffer_status(theEnv, OFF);
        core_get_token(theEnv, "hnd-var", &itkn);
        core_set_pp_buffer_status(theEnv, oldpp);
        close_string_source(theEnv, "hnd-var");

        if( itkn.type != STOP )
        {
            saveExp = bindExp->args->next_arg;
            sd = CheckSlotReference(theEnv, (DEFCLASS *)userBuffer, itkn.type, itkn.value,
                                    TRUE, saveExp);

            if( sd == NULL )
            {
                return(-1);
            }

            GenHandlerSlotReference(theEnv, bindExp, HANDLER_PUT, sd);
            bindExp->args->next_arg = NULL;
            core_return_expression(theEnv, bindExp->args);
            bindExp->args = saveExp;
            return(1);
        }
    }

    return(0);
}

/*********************************************************
 *  NAME         : CheckSlotReference
 *  DESCRIPTION  : Examines a ?self:<slot-name> reference
 *              If the reference is a single-field or
 *              global variable, checking and evaluation
 *              is delayed until run-time.  If the
 *              reference is a symbol, this routine
 *              verifies that the slot is a legal
 *              slot for the reference (i.e., it exists
 *              in the class to which the message-handler
 *              is being attached, it is visible and it
 *              is writable for write reference)
 *  INPUTS       : 1) A buffer holding the class
 *                 of the handler being parsed
 *              2) The type of the slot reference
 *              3) The value of the slot reference
 *              4) A flag indicating if this is a read
 *                 or write access
 *              5) Value expression for write
 *  RETURNS      : Class slot on success, NULL on errors
 *  SIDE EFFECTS : Messages printed on errors.
 *  NOTES        : For static references, this function
 *              insures that the slot is either
 *              publicly visible or that the handler
 *              is being attached to the same class in
 *              which the private slot is defined.
 *********************************************************/
static SLOT_DESC *CheckSlotReference(void *theEnv, DEFCLASS *theDefclass, int theType, void *theValue, BOOLEAN writeFlag, core_expression_object *writeExpression)
{
    int slotIndex;
    SLOT_DESC *sd;
    int vCode;

    if( theType != ATOM )
    {
        error_print_id(theEnv, "MSGPSR", 7, FALSE);
        print_router(theEnv, WERROR, "Illegal value for ?self reference.\n");
        return(NULL);
    }

    slotIndex = FindInstanceTemplateSlot(theEnv, theDefclass, (ATOM_HN *)theValue);

    if( slotIndex == -1 )
    {
        error_print_id(theEnv, "MSGPSR", 6, FALSE);
        print_router(theEnv, WERROR, "No such slot ");
        print_router(theEnv, WERROR, to_string(theValue));
        print_router(theEnv, WERROR, " in class ");
        print_router(theEnv, WERROR, EnvGetDefclassName(theEnv, (void *)theDefclass));
        print_router(theEnv, WERROR, " for ?self reference.\n");
        return(NULL);
    }

    sd = theDefclass->instanceTemplate[slotIndex];

    if((sd->publicVisibility == 0) && (sd->cls != theDefclass))
    {
        SlotVisibilityViolationError(theEnv, sd, theDefclass);
        return(NULL);
    }

    if( !writeFlag )
    {
        return(sd);
    }

    /* =================================================
     *  If a slot is initialize-only, the WithinInit flag
     *  still needs to be checked at run-time, for the
     *  handler could be called out of the context of
     *  an init.
     *  ================================================= */
    if( sd->noWrite && (sd->initializeOnly == 0))
    {
        SlotAccessViolationError(theEnv, to_string(theValue),
                                 FALSE, (void *)theDefclass);
        return(NULL);
    }

    if( getStaticConstraints(theEnv))
    {
        vCode = garner_expression_violations(theEnv, writeExpression, sd->constraint);

        if( vCode != NO_VIOLATION )
        {
            error_print_id(theEnv, "CSTRNCHK", 1, FALSE);
            print_router(theEnv, WERROR, "Expression for ");
            PrintSlot(theEnv, WERROR, sd, NULL, "direct slot write");
            report_constraint_violation_error(theEnv, NULL, NULL, 0, 0, NULL, 0,
                                            vCode, sd->constraint, FALSE);
            return(NULL);
        }
    }

    return(sd);
}

/***************************************************
 *  NAME         : GenHandlerSlotReference
 *  DESCRIPTION  : Creates a bitmap of the class id
 *              and slot index for the get or put
 *              operation. The bitmap and operation
 *              type are stored in the given
 *              expression.
 *  INPUTS       : 1) The expression
 *              2) The operation type
 *              3) The class slot
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Bitmap created and expression
 *              initialized
 *  NOTES        : None
 ***************************************************/
static void GenHandlerSlotReference(void *theEnv, core_expression_object *theExp, unsigned short theType, SLOT_DESC *sd)
{
    HANDLER_SLOT_REFERENCE handlerReference;

    init_bitmap(&handlerReference, sizeof(HANDLER_SLOT_REFERENCE));
    handlerReference.classID = (unsigned short)sd->cls->id;
    handlerReference.slotID = (unsigned)sd->slotName->id;
    theExp->type = theType;
    theExp->value =  store_bitmap(theEnv, (void *)&handlerReference,
                                  (int)sizeof(HANDLER_SLOT_REFERENCE));
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
