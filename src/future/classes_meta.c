/* Purpose: Class browsing and examination commands           */

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
#include "classes_init.h"
#include "core_environment.h"
#include "funcs_instance.h"
#include "core_memory.h"
#include "classes_methods_kernel.h"
#include "funcs_method.h"
#include "router.h"
#include "router_string.h"
#include "sysdep.h"

#define __CLASSES_META_SOURCE__
#include "classes_meta.h"

/* =========================================
 *****************************************
 *   INTERNALLY VISIBLE FUNCTION HEADERS
 *  =========================================
 ***************************************** */

static int CheckTwoClasses(void *, char *, DEFCLASS **, DEFCLASS **);
static SLOT_DESC *CheckSlotExists(void *, char *, DEFCLASS * *, BOOLEAN, BOOLEAN);
static SLOT_DESC *LookupSlot(void *, DEFCLASS *, char *, BOOLEAN);

#if DEBUGGING_FUNCTIONS
static DEFCLASS * CheckClass(void *, char *, char *);
static char *     GetClassNameArgument(void *, char *);
static void       PrintClassBrowse(void *, char *, DEFCLASS *, long);
static void       DisplaySeparator(void *, char *, char *, int, int);
static void       DisplaySlotBasicInfo(void *, char *, char *, char *, char *, DEFCLASS *);
static BOOLEAN    PrintSlotSources(void *, char *, ATOM_HN *, PACKED_CLASS_LINKS *, long, int);
static void       DisplaySlotConstraintInfo(void *, char *, char *, char *, unsigned, DEFCLASS *);
static char *     ConstraintCode(CONSTRAINT_META *, unsigned, unsigned);
#endif

/* =========================================
 *****************************************
 *       EXTERNALLY VISIBLE FUNCTIONS
 *  =========================================
 ***************************************** */

#if DEBUGGING_FUNCTIONS

/****************************************************************
 *  NAME         : BrowseClassesCommand
 *  DESCRIPTION  : Displays a "graph" of the class hierarchy
 *  INPUTS       : None
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : None
 *  NOTES        : Syntax : (browse-classes [<class>])
 ****************************************************************/
globle void BrowseClassesCommand(void *theEnv)
{
    register DEFCLASS *cls;

    if( core_get_arg_count(theEnv) == 0 )
    {
        /* ================================================
         *  Find the OBJECT root class (has no superclasses)
         *  ================================================ */
        cls = LookupDefclassByMdlOrScope(theEnv, OBJECT_TYPE_NAME);
    }
    else
    {
        core_data_object tmp;

        if( core_check_arg_type(theEnv, "browse-classes", 1, ATOM, &tmp) == FALSE )
        {
            return;
        }

        cls = LookupDefclassByMdlOrScope(theEnv, core_convert_data_to_string(tmp));

        if( cls == NULL )
        {
            ClassExistError(theEnv, "browse-classes", core_convert_data_to_string(tmp));
            return;
        }
    }

    EnvBrowseClasses(theEnv, WDISPLAY, (void *)cls);
}

/****************************************************************
 *  NAME         : EnvBrowseClasses
 *  DESCRIPTION  : Displays a "graph" of the class hierarchy
 *  INPUTS       : 1) The logical name of the output
 *              2) Class pointer
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : None
 *  NOTES        : None
 ****************************************************************/
globle void EnvBrowseClasses(void *theEnv, char *logical_name, void *clsptr)
{
    PrintClassBrowse(theEnv, logical_name, (DEFCLASS *)clsptr, 0);
}

/****************************************************************
 *  NAME         : DescribeClassCommand
 *  DESCRIPTION  : Displays direct superclasses and
 *                subclasses and the entire precedence
 *                list for a class
 *  INPUTS       : None
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : None
 *  NOTES        : Syntax : (describe-class <class-name>)
 ****************************************************************/
globle void DescribeClassCommand(void *theEnv)
{
    char *cname;
    DEFCLASS *cls;

    cname = GetClassNameArgument(theEnv, "describe-class");

    if( cname == NULL )
    {
        return;
    }

    cls = CheckClass(theEnv, "describe-class", cname);

    if( cls == NULL )
    {
        return;
    }

    EnvDescribeClass(theEnv, WDISPLAY, (void *)cls);
}

/******************************************************
 *  NAME         : EnvDescribeClass
 *  DESCRIPTION  : Displays direct superclasses and
 *                subclasses and the entire precedence
 *                list for a class
 *  INPUTS       : 1) The logical name of the output
 *              2) Class pointer
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : None
 *  NOTES        : None
 ******************************************************/
globle void EnvDescribeClass(void *theEnv, char *logical_name, void *clsptr)
{
    DEFCLASS *cls;
    char buf[83],
         slotNamePrintFormat[12],
         overrideMessagePrintFormat[12];
    int messageBanner;
    long i;
    size_t slotNameLength, maxSlotNameLength;
    size_t overrideMessageLength, maxOverrideMessageLength;

    cls = (DEFCLASS *)clsptr;
    DisplaySeparator(theEnv, logical_name, buf, 82, '=');
    DisplaySeparator(theEnv, logical_name, buf, 82, '*');

    if( cls->abstract )
    {
        print_router(theEnv, logical_name, "Abstract: direct instances of this class cannot be created.\n\n");
    }
    else
    {
        print_router(theEnv, logical_name, "Concrete: direct instances of this class can be created.\n");
        print_router(theEnv, logical_name, "\n");
    }

    PrintPackedClassLinks(theEnv, logical_name, "Direct Superclasses:", &cls->directSuperclasses);
    PrintPackedClassLinks(theEnv, logical_name, "Inheritance Precedence:", &cls->allSuperclasses);
    PrintPackedClassLinks(theEnv, logical_name, "Direct Subclasses:", &cls->directSubclasses);

    if( cls->instanceTemplate != NULL )
    {
        DisplaySeparator(theEnv, logical_name, buf, 82, '-');
        maxSlotNameLength = 5;
        maxOverrideMessageLength = 8;

        for( i = 0 ; i < cls->instanceSlotCount ; i++ )
        {
            slotNameLength = strlen(to_string(cls->instanceTemplate[i]->slotName->name));

            if( slotNameLength > maxSlotNameLength )
            {
                maxSlotNameLength = slotNameLength;
            }

            if( cls->instanceTemplate[i]->noWrite == 0 )
            {
                overrideMessageLength =
                    strlen(to_string(cls->instanceTemplate[i]->overrideMessage));

                if( overrideMessageLength > maxOverrideMessageLength )
                {
                    maxOverrideMessageLength = overrideMessageLength;
                }
            }
        }

        if( maxSlotNameLength > 16 )
        {
            maxSlotNameLength = 16;
        }

        if( maxOverrideMessageLength > 12 )
        {
            maxOverrideMessageLength = 12;
        }

#if WIN_MVC
        sysdep_sprintf(slotNamePrintFormat, "%%-%Id.%Ids : ", maxSlotNameLength, maxSlotNameLength);
        sysdep_sprintf(overrideMessagePrintFormat, "%%-%Id.%Ids ", maxOverrideMessageLength,
                   maxOverrideMessageLength);
#elif WIN_GCC
        sysdep_sprintf(slotNamePrintFormat, "%%-%ld.%lds : ", (long)maxSlotNameLength, (long)maxSlotNameLength);
        sysdep_sprintf(overrideMessagePrintFormat, "%%-%ld.%lds ", (long)maxOverrideMessageLength,
                   (long)maxOverrideMessageLength);
#else
        sysdep_sprintf(slotNamePrintFormat, "%%-%zd.%zds : ", maxSlotNameLength, maxSlotNameLength);
        sysdep_sprintf(overrideMessagePrintFormat, "%%-%zd.%zds ", maxOverrideMessageLength,
                   maxOverrideMessageLength);
#endif

        DisplaySlotBasicInfo(theEnv, logical_name, slotNamePrintFormat, overrideMessagePrintFormat, buf, cls);
        print_router(theEnv, logical_name, "\nConstraint information for slots:\n\n");
        DisplaySlotConstraintInfo(theEnv, logical_name, slotNamePrintFormat, buf, 82, cls);
    }

    if( cls->handlerCount > 0 )
    {
        messageBanner = TRUE;
    }
    else
    {
        messageBanner = FALSE;

        for( i = 1 ; i < cls->allSuperclasses.classCount ; i++ )
        {
            if( cls->allSuperclasses.classArray[i]->handlerCount > 0 )
            {
                messageBanner = TRUE;
                break;
            }
        }
    }

    if( messageBanner )
    {
        DisplaySeparator(theEnv, logical_name, buf, 82, '-');
        print_router(theEnv, logical_name, "Recognized message-handlers:\n");
        DisplayHandlersInLinks(theEnv, logical_name, &cls->allSuperclasses, 0);
    }

    DisplaySeparator(theEnv, logical_name, buf, 82, '*');
    DisplaySeparator(theEnv, logical_name, buf, 82, '=');
}

#endif

/**********************************************************
 *  NAME         : GetCreateAccessorString
 *  DESCRIPTION  : Gets a string describing which
 *              accessors are implicitly created
 *              for a slot: R, W, RW or NIL
 *  INPUTS       : The slot descriptor
 *  RETURNS      : The string description
 *  SIDE EFFECTS : None
 *  NOTES        : Used by (describe-class) and (slot-facets)
 **********************************************************/
globle char *GetCreateAccessorString(void *vsd)
{
    SLOT_DESC *sd = (SLOT_DESC *)vsd;

    if( sd->createReadAccessor && sd->createWriteAccessor )
    {
        return("RW");
    }

    if((sd->createReadAccessor == 0) && (sd->createWriteAccessor == 0))
    {
        return("NIL");
    }
    else
    {
        if( sd->createReadAccessor )
        {
            return("R");
        }
        else
        {
            return("W");
        }
    }
}

/************************************************************
 *  NAME         : GetDefclassModuleCommand
 *  DESCRIPTION  : Determines to which module a class belongs
 *  INPUTS       : None
 *  RETURNS      : The symbolic name of the module
 *  SIDE EFFECTS : None
 *  NOTES        : H/L Syntax: (defclass-module <class-name>)
 ************************************************************/
globle void *GetDefclassModuleCommand(void *theEnv)
{
    return(core_query_module_name(theEnv, "defclass-module", DefclassData(theEnv)->DefclassConstruct));
}

/*********************************************************************
 *  NAME         : SuperclassPCommand
 *  DESCRIPTION  : Determines if a class is a superclass of another
 *  INPUTS       : None
 *  RETURNS      : TRUE if class-1 is a superclass of class-2
 *  SIDE EFFECTS : None
 *  NOTES        : H/L Syntax : (superclassp <class-1> <class-2>)
 *********************************************************************/
globle BOOLEAN SuperclassPCommand(void *theEnv)
{
    DEFCLASS *c1, *c2;

    if( CheckTwoClasses(theEnv, "superclassp", &c1, &c2) == FALSE )
    {
        return(FALSE);
    }

    return(EnvSuperclassP(theEnv, (void *)c1, (void *)c2));
}

/***************************************************
 *  NAME         : EnvSuperclassP
 *  DESCRIPTION  : Determines if the first class is
 *              a superclass of the other
 *  INPUTS       : 1) First class
 *              2) Second class
 *  RETURNS      : TRUE if first class is a
 *              superclass of the first,
 *              FALSE otherwise
 *  SIDE EFFECTS : None
 *  NOTES        : None
 ***************************************************/
#if WIN_BTC
#pragma argsused
#endif
globle BOOLEAN EnvSuperclassP(void *theEnv, void *firstClass, void *secondClass)
{
#if MAC_MCW || WIN_MCW || MAC_XCD
#pragma unused(theEnv)
#endif

    return(HasSuperclass((DEFCLASS *)secondClass, (DEFCLASS *)firstClass));
}

/*********************************************************************
 *  NAME         : SubclassPCommand
 *  DESCRIPTION  : Determines if a class is a subclass of another
 *  INPUTS       : None
 *  RETURNS      : TRUE if class-1 is a subclass of class-2
 *  SIDE EFFECTS : None
 *  NOTES        : H/L Syntax : (subclassp <class-1> <class-2>)
 *********************************************************************/
globle BOOLEAN SubclassPCommand(void *theEnv)
{
    DEFCLASS *c1, *c2;

    if( CheckTwoClasses(theEnv, "subclassp", &c1, &c2) == FALSE )
    {
        return(FALSE);
    }

    return(EnvSubclassP(theEnv, (void *)c1, (void *)c2));
}

/***************************************************
 *  NAME         : EnvSubclassP
 *  DESCRIPTION  : Determines if the first class is
 *              a subclass of the other
 *  INPUTS       : 1) First class
 *              2) Second class
 *  RETURNS      : TRUE if first class is a
 *              subclass of the first,
 *              FALSE otherwise
 *  SIDE EFFECTS : None
 *  NOTES        : None
 ***************************************************/
#if WIN_BTC
#pragma argsused
#endif
globle BOOLEAN EnvSubclassP(void *theEnv, void *firstClass, void *secondClass)
{
#if MAC_MCW || WIN_MCW || MAC_XCD
#pragma unused(theEnv)
#endif

    return(HasSuperclass((DEFCLASS *)firstClass, (DEFCLASS *)secondClass));
}

/*********************************************************************
 *  NAME         : SlotExistPCommand
 *  DESCRIPTION  : Determines if a slot is present in a class
 *  INPUTS       : None
 *  RETURNS      : TRUE if the slot exists, FALSE otherwise
 *  SIDE EFFECTS : None
 *  NOTES        : H/L Syntax : (slot-existp <class> <slot> [inherit])
 *********************************************************************/
globle int SlotExistPCommand(void *theEnv)
{
    DEFCLASS *cls;
    SLOT_DESC *sd;
    int inheritFlag = FALSE;
    core_data_object dobj;

    sd = CheckSlotExists(theEnv, "slot-existp", &cls, FALSE, TRUE);

    if( sd == NULL )
    {
        return(FALSE);
    }

    if( core_get_arg_count(theEnv) == 3 )
    {
        if( core_check_arg_type(theEnv, "slot-existp", 3, ATOM, &dobj) == FALSE )
        {
            return(FALSE);
        }

        if( strcmp(core_convert_data_to_string(dobj), "inherit") != 0 )
        {
            report_explicit_type_error(theEnv, "slot-existp", 3, "keyword \"inherit\"");
            core_set_eval_error(theEnv, TRUE);
            return(FALSE);
        }

        inheritFlag = TRUE;
    }

    return((sd->cls == cls) ? TRUE : inheritFlag);
}

/***************************************************
 *  NAME         : EnvSlotExistP
 *  DESCRIPTION  : Determines if a slot exists
 *  INPUTS       : 1) The class
 *              2) The slot name
 *              3) A flag indicating if the slot
 *                 can be inherited or not
 *  RETURNS      : TRUE if slot exists,
 *              FALSE otherwise
 *  SIDE EFFECTS : None
 *  NOTES        : None
 ***************************************************/
globle BOOLEAN EnvSlotExistP(void *theEnv, void *theDefclass, char *slotName, BOOLEAN inheritFlag)
{
    return((LookupSlot(theEnv, (DEFCLASS *)theDefclass, slotName, inheritFlag) != NULL)
           ? TRUE : FALSE);
}

/************************************************************************************
 *  NAME         : MessageHandlerExistPCommand
 *  DESCRIPTION  : Determines if a message-handler is present in a class
 *  INPUTS       : None
 *  RETURNS      : TRUE if the message header is present, FALSE otherwise
 *  SIDE EFFECTS : None
 *  NOTES        : H/L Syntax : (message-handler-existp <class> <hnd> [<type>])
 ************************************************************************************/
globle int MessageHandlerExistPCommand(void *theEnv)
{
    DEFCLASS *cls;
    ATOM_HN *mname;
    core_data_object temp;
    unsigned mtype = MPRIMARY;

    if( core_check_arg_type(theEnv, "message-handler-existp", 1, ATOM, &temp) == FALSE )
    {
        return(FALSE);
    }

    cls = LookupDefclassByMdlOrScope(theEnv, core_convert_data_to_string(temp));

    if( cls == NULL )
    {
        ClassExistError(theEnv, "message-handler-existp", core_convert_data_to_string(temp));
        return(FALSE);
    }

    if( core_check_arg_type(theEnv, "message-handler-existp", 2, ATOM, &temp) == FALSE )
    {
        return(FALSE);
    }

    mname = (ATOM_HN *)core_get_value(temp);

    if( core_get_arg_count(theEnv) == 3 )
    {
        if( core_check_arg_type(theEnv, "message-handler-existp", 3, ATOM, &temp) == FALSE )
        {
            return(FALSE);
        }

        mtype = HandlerType(theEnv, "message-handler-existp", core_convert_data_to_string(temp));

        if( mtype == MERROR )
        {
            core_set_eval_error(theEnv, TRUE);
            return(FALSE);
        }
    }

    if( FindHandlerByAddress(cls, mname, mtype) != NULL )
    {
        return(TRUE);
    }

    return(FALSE);
}

/**********************************************************************
 *  NAME         : SlotWritablePCommand
 *  DESCRIPTION  : Determines if an existing slot can be written to
 *  INPUTS       : None
 *  RETURNS      : TRUE if the slot is writable, FALSE otherwise
 *  SIDE EFFECTS : None
 *  NOTES        : H/L Syntax : (slot-writablep <class> <slot>)
 **********************************************************************/
globle BOOLEAN SlotWritablePCommand(void *theEnv)
{
    DEFCLASS *theDefclass;
    SLOT_DESC *sd;

    sd = CheckSlotExists(theEnv, "slot-writablep", &theDefclass, TRUE, TRUE);

    if( sd == NULL )
    {
        return(FALSE);
    }

    return((sd->noWrite || sd->initializeOnly) ? FALSE : TRUE);
}

/***************************************************
 *  NAME         : EnvSlotWritableP
 *  DESCRIPTION  : Determines if a slot is writable
 *  INPUTS       : 1) The class
 *              2) The slot name
 *  RETURNS      : TRUE if slot is writable,
 *              FALSE otherwise
 *  SIDE EFFECTS : None
 *  NOTES        : None
 ***************************************************/
globle BOOLEAN EnvSlotWritableP(void *theEnv, void *theDefclass, char *slotName)
{
    SLOT_DESC *sd;

    if((sd = LookupSlot(theEnv, (DEFCLASS *)theDefclass, slotName, TRUE)) == NULL )
    {
        return(FALSE);
    }

    return((sd->noWrite || sd->initializeOnly) ? FALSE : TRUE);
}

/**********************************************************************
 *  NAME         : SlotInitablePCommand
 *  DESCRIPTION  : Determines if an existing slot can be initialized
 *                via an init message-handler or slot-override
 *  INPUTS       : None
 *  RETURNS      : TRUE if the slot is writable, FALSE otherwise
 *  SIDE EFFECTS : None
 *  NOTES        : H/L Syntax : (slot-initablep <class> <slot>)
 **********************************************************************/
globle BOOLEAN SlotInitablePCommand(void *theEnv)
{
    DEFCLASS *theDefclass;
    SLOT_DESC *sd;

    sd = CheckSlotExists(theEnv, "slot-initablep", &theDefclass, TRUE, TRUE);

    if( sd == NULL )
    {
        return(FALSE);
    }

    return((sd->noWrite && (sd->initializeOnly == 0)) ? FALSE : TRUE);
}

/***************************************************
 *  NAME         : EnvSlotInitableP
 *  DESCRIPTION  : Determines if a slot is initable
 *  INPUTS       : 1) The class
 *              2) The slot name
 *  RETURNS      : TRUE if slot is initable,
 *              FALSE otherwise
 *  SIDE EFFECTS : None
 *  NOTES        : None
 ***************************************************/
globle BOOLEAN EnvSlotInitableP(void *theEnv, void *theDefclass, char *slotName)
{
    SLOT_DESC *sd;

    if((sd = LookupSlot(theEnv, (DEFCLASS *)theDefclass, slotName, TRUE)) == NULL )
    {
        return(FALSE);
    }

    return((sd->noWrite && (sd->initializeOnly == 0)) ? FALSE : TRUE);
}

/**********************************************************************
 *  NAME         : SlotPublicPCommand
 *  DESCRIPTION  : Determines if an existing slot is publicly visible
 *                for direct reference by subclasses
 *  INPUTS       : None
 *  RETURNS      : TRUE if the slot is public, FALSE otherwise
 *  SIDE EFFECTS : None
 *  NOTES        : H/L Syntax : (slot-publicp <class> <slot>)
 **********************************************************************/
globle BOOLEAN SlotPublicPCommand(void *theEnv)
{
    DEFCLASS *theDefclass;
    SLOT_DESC *sd;

    sd = CheckSlotExists(theEnv, "slot-publicp", &theDefclass, TRUE, FALSE);

    if( sd == NULL )
    {
        return(FALSE);
    }

    return(sd->publicVisibility ? TRUE : FALSE);
}

/***************************************************
 *  NAME         : EnvSlotPublicP
 *  DESCRIPTION  : Determines if a slot is public
 *  INPUTS       : 1) The class
 *              2) The slot name
 *  RETURNS      : TRUE if slot is public,
 *              FALSE otherwise
 *  SIDE EFFECTS : None
 *  NOTES        : None
 ***************************************************/
globle BOOLEAN EnvSlotPublicP(void *theEnv, void *theDefclass, char *slotName)
{
    SLOT_DESC *sd;

    if((sd = LookupSlot(theEnv, (DEFCLASS *)theDefclass, slotName, FALSE)) == NULL )
    {
        return(FALSE);
    }

    return(sd->publicVisibility ? TRUE : FALSE);
}

/**********************************************************************
 *  NAME         : SlotDirectAccessPCommand
 *  DESCRIPTION  : Determines if an existing slot can be directly
 *                referenced by the class - i.e., if the slot is
 *                private, is the slot defined in the class
 *  INPUTS       : None
 *  RETURNS      : TRUE if the slot is private,
 *                 FALSE otherwise
 *  SIDE EFFECTS : None
 *  NOTES        : H/L Syntax : (slot-direct-accessp <class> <slot>)
 **********************************************************************/
globle BOOLEAN SlotDirectAccessPCommand(void *theEnv)
{
    DEFCLASS *theDefclass;
    SLOT_DESC *sd;

    sd = CheckSlotExists(theEnv, "slot-direct-accessp", &theDefclass, TRUE, TRUE);

    if( sd == NULL )
    {
        return(FALSE);
    }

    return((sd->publicVisibility || (sd->cls == theDefclass)) ? TRUE : FALSE);
}

/***************************************************
 *  NAME         : EnvSlotDirectAccessP
 *  DESCRIPTION  : Determines if a slot is directly
 *              accessible from message-handlers
 *              on class
 *  INPUTS       : 1) The class
 *              2) The slot name
 *  RETURNS      : TRUE if slot is directly
 *              accessible, FALSE otherwise
 *  SIDE EFFECTS : None
 *  NOTES        : None
 ***************************************************/
globle BOOLEAN EnvSlotDirectAccessP(void *theEnv, void *theDefclass, char *slotName)
{
    SLOT_DESC *sd;

    if((sd = LookupSlot(theEnv, (DEFCLASS *)theDefclass, slotName, TRUE)) == NULL )
    {
        return(FALSE);
    }

    return((sd->publicVisibility || (sd->cls == (DEFCLASS *)theDefclass)) ?
           TRUE : FALSE);
}

/**********************************************************************
 *  NAME         : SlotDefaultValueCommand
 *  DESCRIPTION  : Determines the default avlue for the specified slot
 *              of the specified class
 *  INPUTS       : None
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : None
 *  NOTES        : H/L Syntax : (slot-default-value <class> <slot>)
 **********************************************************************/
globle void SlotDefaultValueCommand(void *theEnv, core_data_object_ptr theValue)
{
    DEFCLASS *theDefclass;
    SLOT_DESC *sd;

    core_set_pointer_type(theValue, ATOM);
    core_set_pointer_value(theValue, get_false(theEnv));
    sd = CheckSlotExists(theEnv, "slot-default-value", &theDefclass, TRUE, TRUE);

    if( sd == NULL )
    {
        return;
    }

    if( sd->noDefault )
    {
        core_set_pointer_type(theValue, ATOM);
        core_set_pointer_value(theValue, store_atom(theEnv, "?NONE"));
        return;
    }

    if( sd->dynamicDefault )
    {
        EvaluateAndStoreInDataObject(theEnv, (int)sd->multiple,
                                     (core_expression_object *)sd->defaultValue,
                                     theValue, TRUE);
    }
    else
    {
        core_mem_copy_memory(core_data_object, 1, theValue, sd->defaultValue);
    }
}

/*********************************************************
 *  NAME         : SlotDefaultValue
 *  DESCRIPTION  : Determines the default value for
 *              the specified slot of the specified class
 *  INPUTS       : 1) The class
 *              2) The slot name
 *  RETURNS      : TRUE if slot default value is set,
 *              FALSE otherwise
 *  SIDE EFFECTS : Slot default value evaluated - dynamic
 *              defaults will cause any side effects
 *  NOTES        : None
 *********************************************************/
globle BOOLEAN EnvSlotDefaultValue(void *theEnv, void *theDefclass, char *slotName, core_data_object_ptr theValue)
{
    SLOT_DESC *sd;

    core_set_pointer_type(theValue, ATOM);
    core_set_pointer_value(theValue, get_false(theEnv));

    if((sd = LookupSlot(theEnv, (DEFCLASS *)theDefclass, slotName, TRUE)) == NULL )
    {
        return(FALSE);
    }

    if( sd->noDefault )
    {
        core_set_pointer_type(theValue, ATOM);
        core_set_pointer_value(theValue, store_atom(theEnv, "?NONE"));
        return(TRUE);
    }

    if( sd->dynamicDefault )
    {
        return(EvaluateAndStoreInDataObject(theEnv, (int)sd->multiple,
                                            (core_expression_object *)sd->defaultValue,
                                            theValue, TRUE));
    }

    core_mem_copy_memory(core_data_object, 1, theValue, sd->defaultValue);
    return(TRUE);
}

/********************************************************
 *  NAME         : ClassExistPCommand
 *  DESCRIPTION  : Determines if a class exists
 *  INPUTS       : None
 *  RETURNS      : TRUE if class exists, FALSE otherwise
 *  SIDE EFFECTS : None
 *  NOTES        : H/L Syntax : (class-existp <arg>)
 ********************************************************/
globle BOOLEAN ClassExistPCommand(void *theEnv)
{
    core_data_object temp;

    if( core_check_arg_type(theEnv, "class-existp", 1, ATOM, &temp) == FALSE )
    {
        return(FALSE);
    }

    return((LookupDefclassByMdlOrScope(theEnv, core_convert_data_to_string(temp)) != NULL) ? TRUE : FALSE);
}

/* =========================================
 *****************************************
 *       INTERNALLY VISIBLE FUNCTIONS
 *  =========================================
 ***************************************** */

/******************************************************
 *  NAME         : CheckTwoClasses
 *  DESCRIPTION  : Checks for exactly two class arguments
 *                 for a H/L function
 *  INPUTS       : 1) The function name
 *              2) Caller's buffer for first class
 *              3) Caller's buffer for second class
 *  RETURNS      : TRUE if both found, FALSE otherwise
 *  SIDE EFFECTS : Caller's buffers set
 *  NOTES        : Assumes exactly 2 arguments
 ******************************************************/
static int CheckTwoClasses(void *theEnv, char *func, DEFCLASS **c1, DEFCLASS **c2)
{
    core_data_object temp;

    if( core_check_arg_type(theEnv, func, 1, ATOM, &temp) == FALSE )
    {
        return(FALSE);
    }

    *c1 = LookupDefclassByMdlOrScope(theEnv, core_convert_data_to_string(temp));

    if( *c1 == NULL )
    {
        ClassExistError(theEnv, func, to_string(temp.value));
        return(FALSE);
    }

    if( core_check_arg_type(theEnv, func, 2, ATOM, &temp) == FALSE )
    {
        return(FALSE);
    }

    *c2 = LookupDefclassByMdlOrScope(theEnv, core_convert_data_to_string(temp));

    if( *c2 == NULL )
    {
        ClassExistError(theEnv, func, to_string(temp.value));
        return(FALSE);
    }

    return(TRUE);
}

/***************************************************
 *  NAME         : CheckSlotExists
 *  DESCRIPTION  : Checks first two arguments of
 *              a function for a valid class
 *              and (inherited) slot
 *  INPUTS       : 1) The name of the function
 *              2) A buffer to hold the found class
 *              3) A flag indicating whether the
 *                 non-existence of the slot should
 *                 be an error
 *              4) A flag indicating if the slot
 *                 can be inherited or not
 *  RETURNS      : NULL if slot not found, slot
 *              descriptor otherwise
 *  SIDE EFFECTS : Class buffer set if no errors,
 *              NULL on errors
 *  NOTES        : None
 ***************************************************/
static SLOT_DESC *CheckSlotExists(void *theEnv, char *func, DEFCLASS **classBuffer, BOOLEAN existsErrorFlag, BOOLEAN inheritFlag)
{
    ATOM_HN *ssym;
    int slotIndex;
    SLOT_DESC *sd;

    ssym = CheckClassAndSlot(theEnv, func, classBuffer);

    if( ssym == NULL )
    {
        return(NULL);
    }

    slotIndex = FindInstanceTemplateSlot(theEnv, *classBuffer, ssym);

    if( slotIndex == -1 )
    {
        if( existsErrorFlag )
        {
            error_method_duplication(theEnv, to_string(ssym), func);
            core_set_eval_error(theEnv, TRUE);
        }

        return(NULL);
    }

    sd = (*classBuffer)->instanceTemplate[slotIndex];

    if((sd->cls == *classBuffer) || inheritFlag )
    {
        return(sd);
    }

    error_print_id(theEnv, "CLASSEXM", 1, FALSE);
    print_router(theEnv, WERROR, "Inherited slot ");
    print_router(theEnv, WERROR, to_string(ssym));
    print_router(theEnv, WERROR, " from class ");
    PrintClassName(theEnv, WERROR, sd->cls, FALSE);
    print_router(theEnv, WERROR, " is not valid for function ");
    print_router(theEnv, WERROR, func);
    print_router(theEnv, WERROR, "\n");
    core_set_eval_error(theEnv, TRUE);
    return(NULL);
}

/***************************************************
 *  NAME         : LookupSlot
 *  DESCRIPTION  : Finds a slot in a class
 *  INPUTS       : 1) The class
 *              2) The slot name
 *              3) A flag indicating if inherited
 *                 slots are OK or not
 *  RETURNS      : The slot descriptor address, or
 *              NULL if not found
 *  SIDE EFFECTS : None
 *  NOTES        : None
 ***************************************************/
static SLOT_DESC *LookupSlot(void *theEnv, DEFCLASS *theDefclass, char *slotName, BOOLEAN inheritFlag)
{
    ATOM_HN *slotSymbol;
    int slotIndex;
    SLOT_DESC *sd;

    slotSymbol = lookup_atom(theEnv, slotName);

    if( slotSymbol == NULL )
    {
        return(NULL);
    }

    slotIndex = FindInstanceTemplateSlot(theEnv, theDefclass, slotSymbol);

    if( slotIndex == -1 )
    {
        return(NULL);
    }

    sd = theDefclass->instanceTemplate[slotIndex];

    if((sd->cls != theDefclass) && (inheritFlag == FALSE))
    {
        return(NULL);
    }

    return(sd);
}

#if DEBUGGING_FUNCTIONS

/*****************************************************
 * NAME         : CheckClass
 * DESCRIPTION  : Used for to check class name for
 *             class accessor functions such
 *             as ppdefclass and undefclass
 * INPUTS       : 1) The name of the H/L function
 *             2) Name of the class
 * RETURNS      : The class address,
 *               or NULL if ther was an error
 * SIDE EFFECTS : None
 * NOTES        : None
 ******************************************************/
static DEFCLASS *CheckClass(void *theEnv, char *func, char *cname)
{
    DEFCLASS *cls;

    cls = LookupDefclassByMdlOrScope(theEnv, cname);

    if( cls == NULL )
    {
        ClassExistError(theEnv, func, cname);
    }

    return(cls);
}

/*********************************************************
 *  NAME         : GetClassNameArgument
 *  DESCRIPTION  : Gets a class name-string
 *  INPUTS       : Calling function name
 *  RETURNS      : Class name (NULL on errors)
 *  SIDE EFFECTS : None
 *  NOTES        : Assumes only 1 argument
 *********************************************************/
static char *GetClassNameArgument(void *theEnv, char *fname)
{
    core_data_object temp;

    if( core_check_arg_type(theEnv, fname, 1, ATOM, &temp) == FALSE )
    {
        return(NULL);
    }

    return(core_convert_data_to_string(temp));
}

/****************************************************************
 *  NAME         : PrintClassBrowse
 *  DESCRIPTION  : Displays a "graph" of class and subclasses
 *  INPUTS       : 1) The logical name of the output
 *              2) The class address
 *              3) The depth of the graph
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : None
 *  NOTES        : None
 ****************************************************************/
static void PrintClassBrowse(void *theEnv, char *logical_name, DEFCLASS *cls, long depth)
{
    long i;

    for( i = 0 ; i < depth ; i++ )
    {
        print_router(theEnv, logical_name, "  ");
    }

    print_router(theEnv, logical_name, EnvGetDefclassName(theEnv, (void *)cls));

    if( cls->directSuperclasses.classCount > 1 )
    {
        print_router(theEnv, logical_name, " *");
    }

    print_router(theEnv, logical_name, "\n");

    for( i = 0 ; i < cls->directSubclasses.classCount ; i++ )
    {
        PrintClassBrowse(theEnv, logical_name, cls->directSubclasses.classArray[i], depth + 1);
    }
}

/*********************************************************
 *  NAME         : DisplaySeparator
 *  DESCRIPTION  : Prints a separator line for DescribeClass
 *  INPUTS       : 1) The logical name of the output
 *              2) The buffer to use for the line
 *              3) The buffer size
 *              4) The character to use
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Buffer overwritten and displayed
 *  NOTES        : None
 *********************************************************/
static void DisplaySeparator(void *theEnv, char *logical_name, char *buf, int maxlen, int sepchar)
{
    register int i;

    for( i = 0 ; i < maxlen - 2 ; i++ )
    {
        buf[i] = (char)sepchar;
    }

    buf[i++] = '\n';
    buf[i] = '\0';
    print_router(theEnv, logical_name, buf);
}

/*************************************************************
 *  NAME         : DisplaySlotBasicInfo
 *  DESCRIPTION  : Displays a table summary of basic
 *               facets for the slots of a class
 *               including:
 *               single/multiple
 *               default/no-default/default-dynamic
 *               inherit/no-inherit
 *               read-write/initialize-only/read-only
 *               local/shared
 *               composite/exclusive
 *               reactive/non-reactive
 *               public/private
 *               create-accessor read/write
 *               override-message
 *
 *               The function also displays the source
 *               class(es) for the facets
 *  INPUTS       : 1) The logical name of the output
 *              2) A format string for use in sprintf
 *                 (for printing slot names)
 *              3) A format string for use in sprintf
 *                 (for printing slot override message names)
 *              4) A buffer to store the display in
 *              5) A pointer to the class
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Buffer written to and displayed
 *  NOTES        : None
 *************************************************************/
static void DisplaySlotBasicInfo(void *theEnv, char *logical_name, char *slotNamePrintFormat, char *overrideMessagePrintFormat, char *buf, DEFCLASS *cls)
{
    long i;
    SLOT_DESC *sp;
    char *createString;

    sysdep_sprintf(buf, slotNamePrintFormat, "SLOTS");
    sysdep_strcat(buf, "FLD DEF PRP ACC STO SRC VIS CRT ");
    print_router(theEnv, logical_name, buf);
    sysdep_sprintf(buf, overrideMessagePrintFormat, "OVRD-MSG");
    print_router(theEnv, logical_name, buf);
    print_router(theEnv, logical_name, "SOURCE(S)\n");

    for( i = 0 ; i < cls->instanceSlotCount ; i++ )
    {
        sp = cls->instanceTemplate[i];
        sysdep_sprintf(buf, slotNamePrintFormat, to_string(sp->slotName->name));
        sysdep_strcat(buf, sp->multiple ? "MLT " : "SGL ");

        if( sp->noDefault )
        {
            sysdep_strcat(buf, "NIL ");
        }
        else
        {
            sysdep_strcat(buf, sp->dynamicDefault ? "DYN " : "STC ");
        }

        sysdep_strcat(buf, sp->noInherit ? "NIL " : "INH ");

        if( sp->initializeOnly )
        {
            sysdep_strcat(buf, "INT ");
        }
        else if( sp->noWrite )
        {
            sysdep_strcat(buf, " R  ");
        }
        else
        {
            sysdep_strcat(buf, "RW  ");
        }

        sysdep_strcat(buf, sp->shared ? "SHR " : "LCL ");
        sysdep_strcat(buf, sp->composite ? "CMP " : "EXC ");
        sysdep_strcat(buf, sp->publicVisibility ? "PUB " : "PRV ");
        createString = GetCreateAccessorString(sp);

        if( createString[1] == '\0' )
        {
            sysdep_strcat(buf, " ");
        }

        sysdep_strcat(buf, createString);

        if((createString[1] == '\0') ? TRUE : (createString[2] == '\0'))
        {
            sysdep_strcat(buf, " ");
        }

        sysdep_strcat(buf, " ");
        print_router(theEnv, logical_name, buf);
        sysdep_sprintf(buf, overrideMessagePrintFormat,
                   sp->noWrite ? "NIL" : to_string(sp->overrideMessage));
        print_router(theEnv, logical_name, buf);
        PrintSlotSources(theEnv, logical_name, sp->slotName->name, &sp->cls->allSuperclasses, 0, TRUE);
        print_router(theEnv, logical_name, "\n");
    }
}

/***************************************************
 *  NAME         : PrintSlotSources
 *  DESCRIPTION  : Displays a list of source classes
 *                for a composite class (in order
 *                of most general to specific)
 *  INPUTS       : 1) The logical name of the output
 *              2) The name of the slot
 *              3) The precedence list of the class
 *                 of the slot (the source class
 *                 shold be first in the list)
 *              4) The index into the packed
 *                 links array
 *              5) Flag indicating whether to
 *                 disregard noniherit facet
 *  RETURNS      : TRUE if a class is printed, FALSE
 *              otherwise
 *  SIDE EFFECTS : Recursively prints out appropriate
 *              memebers from list in reverse order
 *  NOTES        : None
 ***************************************************/
static BOOLEAN PrintSlotSources(void *theEnv, char *logical_name, ATOM_HN *sname, PACKED_CLASS_LINKS *sprec, long theIndex, int inhp)
{
    SLOT_DESC *csp;

    if( theIndex == sprec->classCount )
    {
        return(FALSE);
    }

    csp = FindClassSlot(sprec->classArray[theIndex], sname);

    if((csp != NULL) ? ((csp->noInherit == 0) || inhp) : FALSE )
    {
        if( csp->composite )
        {
            if( PrintSlotSources(theEnv, logical_name, sname, sprec, theIndex + 1, FALSE))
            {
                print_router(theEnv, logical_name, " ");
            }
        }

        PrintClassName(theEnv, logical_name, sprec->classArray[theIndex], FALSE);
        return(TRUE);
    }
    else
    {
        return(PrintSlotSources(theEnv, logical_name, sname, sprec, theIndex + 1, FALSE));
    }
}

/*********************************************************
 *  NAME         : DisplaySlotConstraintInfo
 *  DESCRIPTION  : Displays a table summary of type-checking
 *               facets for the slots of a class
 *               including:
 *               type
 *               allowed-symbols
 *               allowed-integers
 *               allowed-floats
 *               allowed-values
 *               allowed-instance-names
 *               range
 *               min-number-of-elements
 *               max-number-of-elements
 *
 *               The function also displays the source
 *               class(es) for the facets
 *  INPUTS       : 1) A format string for use in sprintf
 *              2) A buffer to store the display in
 *              3) Maximum buffer size
 *              4) A pointer to the class
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Buffer written to and displayed
 *  NOTES        : None
 *********************************************************/
static void DisplaySlotConstraintInfo(void *theEnv, char *logical_name, char *slotNamePrintFormat, char *buf, unsigned maxlen, DEFCLASS *cls)
{
    long i;
    CONSTRAINT_META *cr;
    char *strdest = "***describe-class***";

    sysdep_sprintf(buf, slotNamePrintFormat, "SLOTS");
    sysdep_strcat(buf, "SYM STR INN INA EXA FTA INT FLT\n");
    print_router(theEnv, logical_name, buf);

    for( i = 0 ; i < cls->instanceSlotCount ; i++ )
    {
        cr = cls->instanceTemplate[i]->constraint;
        sysdep_sprintf(buf, slotNamePrintFormat, to_string(cls->instanceTemplate[i]->slotName->name));

        if( cr != NULL )
        {
            sysdep_strcat(buf, ConstraintCode(cr, (unsigned)cr->allow_atom,
                                          (unsigned)cr->restrict_atom));
            sysdep_strcat(buf, ConstraintCode(cr, (unsigned)cr->allow_string,
                                          (unsigned)cr->restrict_string));
            sysdep_strcat(buf, ConstraintCode(cr, (unsigned)cr->allow_instance_name,
                                          (unsigned)(cr->restrict_instance_name || cr->restrict_class)));
            sysdep_strcat(buf, ConstraintCode(cr, (unsigned)cr->allow_instance_pointer,
                                          (unsigned)cr->restrict_class));
            sysdep_strcat(buf, ConstraintCode(cr, (unsigned)cr->allow_external_pointer, 0));
            sysdep_strcat(buf, ConstraintCode(cr, (unsigned)cr->allow_integer,
                                          (unsigned)cr->restrict_integer));
            sysdep_strcat(buf, ConstraintCode(cr, (unsigned)cr->allow_float,
                                          (unsigned)cr->restrict_float));
            open_string_dest(theEnv, strdest, buf + strlen(buf), (maxlen - strlen(buf) - 1));

            if( cr->allow_integer || cr->allow_float || cr->allow_any )
            {
                print_router(theEnv, strdest, "RNG:[");
                core_print_expression(theEnv, strdest, cr->min_value);
                print_router(theEnv, strdest, "..");
                core_print_expression(theEnv, strdest, cr->max_value);
                print_router(theEnv, strdest, "] ");
            }

            if( cls->instanceTemplate[i]->multiple )
            {
                print_router(theEnv, strdest, "CRD:[");
                core_print_expression(theEnv, strdest, cr->min_fields);
                print_router(theEnv, strdest, "..");
                core_print_expression(theEnv, strdest, cr->max_fields);
                print_router(theEnv, strdest, "]");
            }
        }
        else
        {
            open_string_dest(theEnv, strdest, buf, maxlen);
            print_router(theEnv, strdest, " +   +   +   +   +   +   +   +  RNG:[-oo..+oo]");

            if( cls->instanceTemplate[i]->multiple )
            {
                print_router(theEnv, strdest, " CRD:[0..+oo]");
            }
        }

        print_router(theEnv, strdest, "\n");
        close_string_dest(theEnv, strdest);
        print_router(theEnv, logical_name, buf);
    }
}

/******************************************************
 *  NAME         : ConstraintCode
 *  DESCRIPTION  : Gives a string code representing the
 *              type of constraint
 *  INPUTS       : 1) The constraint record
 *              2) Allowed Flag
 *              3) Restricted Values flag
 *  RETURNS      : "    " for type not allowed
 *              " +  " for any value of type allowed
 *              " #  " for some values of type allowed
 *  SIDE EFFECTS : None
 *  NOTES        : Used by DisplaySlotConstraintInfo
 ******************************************************/
static char *ConstraintCode(CONSTRAINT_META *cr, unsigned allow, unsigned restrictValues)
{
    if( allow || cr->allow_any )
    {
        if( restrictValues || cr->restrict_any )
        {
            return(" #  ");
        }
        else
        {
            return(" +  ");
        }
    }

    return("    ");
}

#endif

#endif
