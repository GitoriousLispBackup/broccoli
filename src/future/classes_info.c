/* Purpose: Class Information Interface Support Routines      */

/* =========================================
 *****************************************
 *            EXTERNAL DEFINITIONS
 *  =========================================
 ***************************************** */
#include "setup.h"

#if OBJECT_SYSTEM

#ifndef _STDIO_INCLUDED_
#define _STDIO_INCLUDED_
#include <stdio.h>
#endif

#include <string.h>

#include "core_arguments.h"
#include "classes_kernel.h"
#include "classes_meta.h"
#include "funcs_class.h"
#include "classes_init.h"
#include "core_environment.h"
#include "core_memory.h"
#include "funcs_instance.h"
#include "classes_methods_kernel.h"
#include "funcs_method.h"
#include "type_list.h"
#include "core_utilities.h"

#define __CLASSES_INFO_SOURCE__
#include "classes_info.h"

/* =========================================
 *****************************************
 *   INTERNALLY VISIBLE FUNCTION HEADERS
 *  =========================================
 ***************************************** */

static void SlotInfoSupportFunction(void *, core_data_object *, char *, void(*) (void *, void *, char *, core_data_object *));
static unsigned    CountSubclasses(DEFCLASS *, int, int);
static unsigned    StoreSubclasses(void *, unsigned, DEFCLASS *, int, int, short);
static SLOT_DESC * SlotInfoSlot(void *, core_data_object *, DEFCLASS *, char *, char *);

/*********************************************************************
 *  NAME         : ClassAbstractPCommand
 *  DESCRIPTION  : Determines if direct instances of a class can be made
 *  INPUTS       : None
 *  RETURNS      : TRUE (1) if class is abstract, FALSE (0) if concrete
 *  SIDE EFFECTS : None
 *  NOTES        : Syntax: (class-abstractp <class>)
 *********************************************************************/
globle int ClassAbstractPCommand(void *theEnv)
{
    core_data_object tmp;
    DEFCLASS *cls;

    if( core_check_arg_type(theEnv, "class-abstractp", 1, ATOM, &tmp) == FALSE )
    {
        return(FALSE);
    }

    cls = LookupDefclassByMdlOrScope(theEnv, core_convert_data_to_string(tmp));

    if( cls == NULL )
    {
        ClassExistError(theEnv, "class-abstractp", to_string(tmp.value));
        return(FALSE);
    }

    return(EnvClassAbstractP(theEnv, (void *)cls));
}

/***********************************************************
 *  NAME         : ClassInfoFnxArgs
 *  DESCRIPTION  : Examines arguments for:
 *                class-slots, get-defmessage-handler-list,
 *                class-superclasses and class-subclasses
 *  INPUTS       : 1) Name of function
 *              2) A buffer to hold a flag indicating if
 *                 the inherit keyword was specified
 *  RETURNS      : Pointer to the class on success,
 *                NULL on errors
 *  SIDE EFFECTS : inhp flag set
 *              error flag set
 *  NOTES        : None
 ***********************************************************/
globle void *ClassInfoFnxArgs(void *theEnv, char *fnx, int *inhp)
{
    void *clsptr;
    core_data_object tmp;

    *inhp = 0;

    if( core_get_arg_count(theEnv) == 0 )
    {
        report_arg_count_error(theEnv, fnx, AT_LEAST, 1);
        core_set_eval_error(theEnv, TRUE);
        return(NULL);
    }

    if( core_check_arg_type(theEnv, fnx, 1, ATOM, &tmp) == FALSE )
    {
        return(NULL);
    }

    clsptr = (void *)LookupDefclassByMdlOrScope(theEnv, core_convert_data_to_string(tmp));

    if( clsptr == NULL )
    {
        ClassExistError(theEnv, fnx, to_string(tmp.value));
        return(NULL);
    }

    if( core_get_arg_count(theEnv) == 2 )
    {
        if( core_check_arg_type(theEnv, fnx, 2, ATOM, &tmp) == FALSE )
        {
            return(NULL);
        }

        if( strcmp(to_string(tmp.value), "inherit") == 0 )
        {
            *inhp = 1;
        }
        else
        {
            error_syntax(theEnv, fnx);
            core_set_eval_error(theEnv, TRUE);
            return(NULL);
        }
    }

    return(clsptr);
}

/********************************************************************
 *  NAME         : ClassSlotsCommand
 *  DESCRIPTION  : Groups slot info for a class into a list value
 *                for dynamic perusal
 *  INPUTS       : Data object buffer to hold the slots of the class
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Creates a list storing the names of
 *                 the slots of the class
 *  NOTES        : Syntax: (class-slots <class> [inherit])
 ********************************************************************/
globle void ClassSlotsCommand(void *theEnv, core_data_object *result)
{
    int inhp;
    void *clsptr;

    clsptr = ClassInfoFnxArgs(theEnv, "class-slots", &inhp);

    if( clsptr == NULL )
    {
        core_create_error_list(theEnv, result);
        return;
    }

    EnvClassSlots(theEnv, clsptr, result, inhp);
}

/************************************************************************
 *  NAME         : ClassSuperclassesCommand
 *  DESCRIPTION  : Groups superclasses for a class into a list value
 *                for dynamic perusal
 *  INPUTS       : Data object buffer to hold the superclasses of the class
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Creates a list storing the names of
 *                 the superclasses of the class
 *  NOTES        : Syntax: (class-superclasses <class> [inherit])
 ************************************************************************/
globle void ClassSuperclassesCommand(void *theEnv, core_data_object *result)
{
    int inhp;
    void *clsptr;

    clsptr = ClassInfoFnxArgs(theEnv, "class-superclasses", &inhp);

    if( clsptr == NULL )
    {
        core_create_error_list(theEnv, result);
        return;
    }

    EnvClassSuperclasses(theEnv, clsptr, result, inhp);
}

/************************************************************************
 *  NAME         : ClassSubclassesCommand
 *  DESCRIPTION  : Groups subclasses for a class into a list value
 *                for dynamic perusal
 *  INPUTS       : Data object buffer to hold the subclasses of the class
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Creates a list storing the names of
 *                 the subclasses of the class
 *  NOTES        : Syntax: (class-subclasses <class> [inherit])
 ************************************************************************/
globle void ClassSubclassesCommand(void *theEnv, core_data_object *result)
{
    int inhp;
    void *clsptr;

    clsptr = ClassInfoFnxArgs(theEnv, "class-subclasses", &inhp);

    if( clsptr == NULL )
    {
        core_create_error_list(theEnv, result);
        return;
    }

    EnvClassSubclasses(theEnv, clsptr, result, inhp);
}

/***********************************************************************
 *  NAME         : GetDefmessageHandlersListCmd
 *  DESCRIPTION  : Groups message-handlers for a class into a list
 *                value for dynamic perusal
 *  INPUTS       : Data object buffer to hold the handlers of the class
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Creates a list storing the names of
 *                 the message-handlers of the class
 *  NOTES        : Syntax: (get-defmessage-handler-list <class> [inherit])
 ***********************************************************************/
globle void GetDefmessageHandlersListCmd(void *theEnv, core_data_object *result)
{
    int inhp;
    void *clsptr;

    if( core_get_arg_count(theEnv) == 0 )
    {
        EnvGetDefmessageHandlerList(theEnv, NULL, result, 0);
    }
    else
    {
        clsptr = ClassInfoFnxArgs(theEnv, "get-defmessage-handler-list", &inhp);

        if( clsptr == NULL )
        {
            core_create_error_list(theEnv, result);
            return;
        }

        EnvGetDefmessageHandlerList(theEnv, clsptr, result, inhp);
    }
}

/*********************************
 *  Slot Information Access Functions
 *********************************/
globle void SlotFacetsCommand(void *theEnv, core_data_object *result)
{
    SlotInfoSupportFunction(theEnv, result, "slot-facets", EnvSlotFacets);
}

globle void SlotSourcesCommand(void *theEnv, core_data_object *result)
{
    SlotInfoSupportFunction(theEnv, result, "slot-sources", EnvSlotSources);
}

globle void SlotTypesCommand(void *theEnv, core_data_object *result)
{
    SlotInfoSupportFunction(theEnv, result, "slot-types", EnvSlotTypes);
}

globle void SlotAllowedValuesCommand(void *theEnv, core_data_object *result)
{
    SlotInfoSupportFunction(theEnv, result, "slot-allowed-values", EnvSlotAllowedValues);
}

globle void SlotAllowedClassesCommand(void *theEnv, core_data_object *result)
{
    SlotInfoSupportFunction(theEnv, result, "slot-allowed-classes", EnvSlotAllowedClasses);
}

globle void SlotRangeCommand(void *theEnv, core_data_object *result)
{
    SlotInfoSupportFunction(theEnv, result, "slot-range", EnvSlotRange);
}

globle void SlotCardinalityCommand(void *theEnv, core_data_object *result)
{
    SlotInfoSupportFunction(theEnv, result, "slot-cardinality", EnvSlotCardinality);
}

/********************************************************************
 *  NAME         : EnvClassAbstractP
 *  DESCRIPTION  : Determines if a class is abstract or not
 *  INPUTS       : Generic pointer to class
 *  RETURNS      : 1 if class is abstract, 0 otherwise
 *  SIDE EFFECTS : None
 *  NOTES        : None
 ********************************************************************/
#if WIN_BTC
#pragma argsused
#endif
globle BOOLEAN EnvClassAbstractP(void *theEnv, void *clsptr)
{
#if MAC_MCW || WIN_MCW || MAC_XCD
#pragma unused(theEnv)
#endif

    return(((DEFCLASS *)clsptr)->abstract);
}

/********************************************************************
 *  NAME         : EnvClassSlots
 *  DESCRIPTION  : Groups slot info for a class into a list value
 *                for dynamic perusal
 *  INPUTS       : 1) Generic pointer to class
 *              2) Data object buffer to hold the slots of the class
 *              3) Include (1) or exclude (0) inherited slots
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Creates a list storing the names of
 *                 the slots of the class
 *  NOTES        : None
 ********************************************************************/
globle void EnvClassSlots(void *theEnv, void *clsptr, core_data_object *result, int inhp)
{
    long size;
    register DEFCLASS *cls;
    long i;

    cls = (DEFCLASS *)clsptr;
    size = inhp ? cls->instanceSlotCount : cls->slotCount;
    result->type = LIST;
    core_set_data_ptr_start(result, 1);
    core_set_data_ptr_end(result, size);
    result->value = (void *)create_list(theEnv, size);

    if( size == 0 )
    {
        return;
    }

    if( inhp )
    {
        for( i = 0 ; i < cls->instanceSlotCount ; i++ )
        {
            set_list_node_type(result->value, i + 1, ATOM);
            set_list_node_value(result->value, i + 1, cls->instanceTemplate[i]->slotName->name);
        }
    }
    else
    {
        for( i = 0 ; i < cls->slotCount ; i++ )
        {
            set_list_node_type(result->value, i + 1, ATOM);
            set_list_node_value(result->value, i + 1, cls->slots[i].slotName->name);
        }
    }
}

/************************************************************************
 *  NAME         : EnvGetDefmessageHandlerList
 *  DESCRIPTION  : Groups handler info for a class into a list value
 *                for dynamic perusal
 *  INPUTS       : 1) Generic pointer to class (NULL to get handlers for
 *                 all classes)
 *              2) Data object buffer to hold the handlers of the class
 *              3) Include (1) or exclude (0) inherited handlers
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Creates a list storing the names and types of
 *                 the message-handlers of the class
 *  NOTES        : None
 ************************************************************************/
globle void EnvGetDefmessageHandlerList(void *theEnv, void *clsptr, core_data_object *result, int inhp)
{
    DEFCLASS *cls, *svcls, *svnxt, *supcls;
    long j;
    register int classi, classiLimit;
    unsigned long i, sublen, len;

    if( clsptr == NULL )
    {
        inhp = 0;
        cls = (DEFCLASS *)EnvGetNextDefclass(theEnv, NULL);
        svnxt = (DEFCLASS *)EnvGetNextDefclass(theEnv, (void *)cls);
    }
    else
    {
        cls = (DEFCLASS *)clsptr;
        svnxt = (DEFCLASS *)EnvGetNextDefclass(theEnv, (void *)cls);
        SetNextDefclass((void *)cls, NULL);
    }

    for( svcls = cls, i = 0 ;
         cls != NULL ;
         cls = (DEFCLASS *)EnvGetNextDefclass(theEnv, (void *)cls))
    {
        classiLimit = inhp ? cls->allSuperclasses.classCount : 1;

        for( classi = 0 ; classi < classiLimit ; classi++ )
        {
            i += cls->allSuperclasses.classArray[classi]->handlerCount;
        }
    }

    len = i * 3;
    result->type = LIST;
    core_set_data_ptr_start(result, 1);
    core_set_data_ptr_end(result, len);
    result->value = (void *)create_list(theEnv, len);

    for( cls = svcls, sublen = 0 ;
         cls != NULL ;
         cls = (DEFCLASS *)EnvGetNextDefclass(theEnv, (void *)cls))
    {
        classiLimit = inhp ? cls->allSuperclasses.classCount : 1;

        for( classi = 0 ; classi < classiLimit ; classi++ )
        {
            supcls = cls->allSuperclasses.classArray[classi];

            if( inhp == 0 )
            {
                i = sublen + 1;
            }
            else
            {
                i = len - (supcls->handlerCount * 3) - sublen + 1;
            }

            for( j = 0 ; j < supcls->handlerCount ; j++ )
            {
                set_list_node_type(result->value, i, ATOM);
                set_list_node_value(result->value, i++, GetDefclassNamePointer((void *)supcls));
                set_list_node_type(result->value, i, ATOM);
                set_list_node_value(result->value, i++, supcls->handlers[j].name);
                set_list_node_type(result->value, i, ATOM);
                set_list_node_value(result->value, i++, store_atom(theEnv, MessageHandlerData(theEnv)->hndquals[supcls->handlers[j].type]));
            }

            sublen += supcls->handlerCount * 3;
        }
    }

    if( svcls != NULL )
    {
        SetNextDefclass((void *)svcls, (void *)svnxt);
    }
}

/***************************************************************************
 *  NAME         : EnvClassSuperclasses
 *  DESCRIPTION  : Groups the names of superclasses into a list
 *                value for dynamic perusal
 *  INPUTS       : 1) Generic pointer to class
 *              2) Data object buffer to hold the superclasses of the class
 *              3) Include (1) or exclude (0) indirect superclasses
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Creates a list storing the names of
 *                 the superclasses of the class
 *  NOTES        : None
 ***************************************************************************/
globle void EnvClassSuperclasses(void *theEnv, void *clsptr, core_data_object *result, int inhp)
{
    PACKED_CLASS_LINKS *plinks;
    unsigned offset;
    long i, j;

    if( inhp )
    {
        plinks = &((DEFCLASS *)clsptr)->allSuperclasses;
        offset = 1;
    }
    else
    {
        plinks = &((DEFCLASS *)clsptr)->directSuperclasses;
        offset = 0;
    }

    result->type = LIST;
    result->begin = 0;
    core_set_data_ptr_end(result, plinks->classCount - offset);
    result->value = (void *)create_list(theEnv, result->end + 1U);

    if( result->end == -1 )
    {
        return;
    }

    for( i = offset, j = 1 ; i < plinks->classCount ; i++, j++ )
    {
        set_list_node_type(result->value, j, ATOM);
        set_list_node_value(result->value, j, GetDefclassNamePointer((void *)plinks->classArray[i]));
    }
}

/**************************************************************************
 *  NAME         : EnvClassSubclasses
 *  DESCRIPTION  : Groups the names of subclasses for a class into a
 *                list value for dynamic perusal
 *  INPUTS       : 1) Generic pointer to class
 *              2) Data object buffer to hold the sublclasses of the class
 *              3) Include (1) or exclude (0) indirect subclasses
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Creates a list storing the names
 *                 the subclasses of the class
 *  NOTES        : None
 **************************************************************************/
globle void EnvClassSubclasses(void *theEnv, void *clsptr, core_data_object *result, int inhp)
{
    register unsigned i;
    register int id;

    if((id = GetTraversalID(theEnv)) == -1 )
    {
        return;
    }

    i = CountSubclasses((DEFCLASS *)clsptr, inhp, id);
    ReleaseTraversalID(theEnv);
    result->type = LIST;
    result->begin = 0;
    core_set_data_ptr_end(result, i);
    result->value = (void *)create_list(theEnv, i);

    if( i == 0 )
    {
        return;
    }

    if((id = GetTraversalID(theEnv)) == -1 )
    {
        return;
    }

    StoreSubclasses(result->value, 1, (DEFCLASS *)clsptr, inhp, id, TRUE);
    ReleaseTraversalID(theEnv);
}

/**************************************************************************
 *  NAME         : ClassSubclassAddresses
 *  DESCRIPTION  : Groups the class addresses of subclasses for a class into a
 *                list value for dynamic perusal
 *  INPUTS       : 1) Generic pointer to class
 *              2) Data object buffer to hold the sublclasses of the class
 *              3) Include (1) or exclude (0) indirect subclasses
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Creates a list storing the subclass
 *                 addresss of the class
 *  NOTES        : None
 **************************************************************************/
globle void ClassSubclassAddresses(void *theEnv, void *clsptr, core_data_object *result, int inhp)
{
    register unsigned i;
    register int id;

    if((id = GetTraversalID(theEnv)) == -1 )
    {
        return;
    }

    i = CountSubclasses((DEFCLASS *)clsptr, inhp, id);
    ReleaseTraversalID(theEnv);
    result->type = LIST;
    result->begin = 0;
    core_set_data_ptr_end(result, i);
    result->value = (void *)create_list(theEnv, i);

    if( i == 0 )
    {
        return;
    }

    if((id = GetTraversalID(theEnv)) == -1 )
    {
        return;
    }

    StoreSubclasses(result->value, 1, (DEFCLASS *)clsptr, inhp, id, FALSE);
    ReleaseTraversalID(theEnv);
}

/**************************************************************************
 *  NAME         : Slot...  Slot information access functions
 *  DESCRIPTION  : Groups the sources/facets/types/allowed-values/range or
 *                cardinality  of a slot for a class into a list
 *                value for dynamic perusal
 *  INPUTS       : 1) Generic pointer to class
 *              2) Name of the slot
 *              3) Data object buffer to hold the attributes of the class
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Creates a list storing the attributes for the slot
 *                of the class
 *  NOTES        : None
 **************************************************************************/

globle void EnvSlotFacets(void *theEnv, void *clsptr, char *sname, core_data_object *result)
{
    register int i;
    register SLOT_DESC *sp;

    if((sp = SlotInfoSlot(theEnv, result, (DEFCLASS *)clsptr, sname, "slot-facets")) == NULL )
    {
        return;
    }

    result->end = 8;
    result->value = (void *)create_list(theEnv, 9L);

    for( i = 1 ; i <= 9 ; i++ )
    {
        set_list_node_type(result->value, i, ATOM);
    }


    if( sp->multiple )
    {
        set_list_node_value(result->value, 1, store_atom(theEnv, "MLT"));
    }
    else
    {
        set_list_node_value(result->value, 1, store_atom(theEnv, "SGL"));
    }

    if( sp->noDefault )
    {
        set_list_node_value(result->value, 2, store_atom(theEnv, "NIL"));
    }
    else
    {
        if( sp->dynamicDefault )
        {
            set_list_node_value(result->value, 2, store_atom(theEnv, "DYN"));
        }
        else
        {
            set_list_node_value(result->value, 2, store_atom(theEnv, "STC"));
        }
    }

    if( sp->noInherit )
    {
        set_list_node_value(result->value, 3, store_atom(theEnv, "NIL"));
    }
    else
    {
        set_list_node_value(result->value, 3, store_atom(theEnv, "INH"));
    }

    if( sp->initializeOnly )
    {
        set_list_node_value(result->value, 4, store_atom(theEnv, "INT"));
    }
    else if( sp->noWrite )
    {
        set_list_node_value(result->value, 4, store_atom(theEnv, "R"));
    }
    else
    {
        set_list_node_value(result->value, 4, store_atom(theEnv, "RW"));
    }

    if( sp->shared )
    {
        set_list_node_value(result->value, 5, store_atom(theEnv, "SHR"));
    }
    else
    {
        set_list_node_value(result->value, 5, store_atom(theEnv, "LCL"));
    }


    if( sp->composite )
    {
        set_list_node_value(result->value, 6, store_atom(theEnv, "CMP"));
    }
    else
    {
        set_list_node_value(result->value, 6, store_atom(theEnv, "EXC"));
    }

    if( sp->publicVisibility )
    {
        set_list_node_value(result->value, 7, store_atom(theEnv, "PUB"));
    }
    else
    {
        set_list_node_value(result->value, 7, store_atom(theEnv, "PRV"));
    }

    set_list_node_value(result->value, 8, store_atom(theEnv, GetCreateAccessorString((void *)sp)));
    set_list_node_value(result->value, 9, sp->noWrite ? store_atom(theEnv, "NIL") : (void *)sp->overrideMessage);
}

globle void EnvSlotSources(void *theEnv, void *clsptr, char *sname, core_data_object *result)
{
    register unsigned i;
    register int classi;
    register SLOT_DESC *sp, *csp;
    CLASS_LINK *ctop, *ctmp;
    DEFCLASS *cls;

    if((sp = SlotInfoSlot(theEnv, result, (DEFCLASS *)clsptr, sname, "slot-sources")) == NULL )
    {
        return;
    }

    i = 1;
    ctop = core_mem_get_struct(theEnv, classLink);
    ctop->cls = sp->cls;
    ctop->nxt = NULL;

    if( sp->composite )
    {
        for( classi = 1 ; classi < sp->cls->allSuperclasses.classCount ; classi++ )
        {
            cls = sp->cls->allSuperclasses.classArray[classi];
            csp = FindClassSlot(cls, sp->slotName->name);

            if((csp != NULL) ? (csp->noInherit == 0) : FALSE )
            {
                ctmp = core_mem_get_struct(theEnv, classLink);
                ctmp->cls = cls;
                ctmp->nxt = ctop;
                ctop = ctmp;
                i++;

                if( csp->composite == 0 )
                {
                    break;
                }
            }
        }
    }

    core_set_data_ptr_end(result, i);
    result->value = (void *)create_list(theEnv, i);

    for( ctmp = ctop, i = 1 ; ctmp != NULL ; ctmp = ctmp->nxt, i++ )
    {
        set_list_node_type(result->value, i, ATOM);
        set_list_node_value(result->value, i, GetDefclassNamePointer((void *)ctmp->cls));
    }

    DeleteClassLinks(theEnv, ctop);
}

globle void EnvSlotTypes(void *theEnv, void *clsptr, char *sname, core_data_object *result)
{
    register unsigned i, j;
    register SLOT_DESC *sp;
    char typemap[2];
    unsigned msize;

    if((sp = SlotInfoSlot(theEnv, result, (DEFCLASS *)clsptr, sname, "slot-types")) == NULL )
    {
        return;
    }

    if((sp->constraint != NULL) ? sp->constraint->allow_any : TRUE )
    {
        typemap[0] = typemap[1] = (char)0xFF;
        core_bitmap_clear(typemap, LIST);
        msize = 8;
    }
    else
    {
        typemap[0] = typemap[1] = (char)0x00;
        msize = 0;

        if( sp->constraint->allow_atom )
        {
            msize++;
            core_bitmap_set(typemap, ATOM);
        }

        if( sp->constraint->allow_string )
        {
            msize++;
            core_bitmap_set(typemap, STRING);
        }

        if( sp->constraint->allow_float )
        {
            msize++;
            core_bitmap_set(typemap, FLOAT);
        }

        if( sp->constraint->allow_integer )
        {
            msize++;
            core_bitmap_set(typemap, INTEGER);
        }

        if( sp->constraint->allow_instance_name )
        {
            msize++;
            core_bitmap_set(typemap, INSTANCE_NAME);
        }

        if( sp->constraint->allow_instance_pointer )
        {
            msize++;
            core_bitmap_set(typemap, INSTANCE_ADDRESS);
        }

        if( sp->constraint->allow_external_pointer )
        {
            msize++;
            core_bitmap_set(typemap, EXTERNAL_ADDRESS);
        }
    }

    core_set_data_ptr_end(result, msize);
    result->value = create_list(theEnv, msize);
    i = 1;
    j = 0;

    while( i <= msize )
    {
        if( core_bitmap_test(typemap, j))
        {
            set_list_node_type(result->value, i, ATOM);
            set_list_node_value(result->value, i,
                             (void *)GetDefclassNamePointer((void *)
                                                            DefclassData(theEnv)->PrimitiveClassMap[j]));
            i++;
        }

        j++;
    }
}

globle void EnvSlotAllowedValues(void *theEnv, void *clsptr, char *sname, core_data_object *result)
{
    register int i;
    register SLOT_DESC *sp;
    register core_expression_object *theExp;

    if((sp = SlotInfoSlot(theEnv, result, (DEFCLASS *)clsptr, sname, "slot-allowed-values")) == NULL )
    {
        return;
    }

    if((sp->constraint != NULL) ? (sp->constraint->restriction_list == NULL) : TRUE )
    {
        result->type = ATOM;
        result->value = get_false(theEnv);
        return;
    }

    result->end = core_calculate_expression_size(sp->constraint->restriction_list) - 1;
    result->value = create_list(theEnv, (unsigned long)(result->end + 1));
    i = 1;
    theExp = sp->constraint->restriction_list;

    while( theExp != NULL )
    {
        set_list_node_type(result->value, i, theExp->type);
        set_list_node_value(result->value, i, theExp->value);
        theExp = theExp->next_arg;
        i++;
    }
}

globle void EnvSlotAllowedClasses(void *theEnv, void *clsptr, char *sname, core_data_object *result)
{
    register int i;
    register SLOT_DESC *sp;
    register core_expression_object *theExp;

    if((sp = SlotInfoSlot(theEnv, result, (DEFCLASS *)clsptr, sname, "slot-allowed-classes")) == NULL )
    {
        return;
    }

    if((sp->constraint != NULL) ? (sp->constraint->class_list == NULL) : TRUE )
    {
        result->type = ATOM;
        result->value = get_false(theEnv);
        return;
    }

    result->end = core_calculate_expression_size(sp->constraint->class_list) - 1;
    result->value = create_list(theEnv, (unsigned long)(result->end + 1));
    i = 1;
    theExp = sp->constraint->class_list;

    while( theExp != NULL )
    {
        set_list_node_type(result->value, i, theExp->type);
        set_list_node_value(result->value, i, theExp->value);
        theExp = theExp->next_arg;
        i++;
    }
}

globle void EnvSlotRange(void *theEnv, void *clsptr, char *sname, core_data_object *result)
{
    register SLOT_DESC *sp;

    if((sp = SlotInfoSlot(theEnv, result, (DEFCLASS *)clsptr, sname, "slot-range")) == NULL )
    {
        return;
    }

    if((sp->constraint == NULL) ? FALSE :
       (sp->constraint->allow_any || sp->constraint->allow_float ||
        sp->constraint->allow_integer))
    {
        result->end = 1;
        result->value = create_list(theEnv, 2L);
        set_list_node_type(result->value, 1, sp->constraint->min_value->type);
        set_list_node_value(result->value, 1, sp->constraint->min_value->value);
        set_list_node_type(result->value, 2, sp->constraint->max_value->type);
        set_list_node_value(result->value, 2, sp->constraint->max_value->value);
    }
    else
    {
        result->type = ATOM;
        result->value = get_false(theEnv);
        return;
    }
}

globle void EnvSlotCardinality(void *theEnv, void *clsptr, char *sname, core_data_object *result)
{
    register SLOT_DESC *sp;

    if((sp = SlotInfoSlot(theEnv, result, (DEFCLASS *)clsptr, sname, "slot-cardinality")) == NULL )
    {
        return;
    }

    if( sp->multiple == 0 )
    {
        core_create_error_list(theEnv, result);
        return;
    }

    result->end = 1;
    result->value = create_list(theEnv, 2L);

    if( sp->constraint != NULL )
    {
        set_list_node_type(result->value, 1, sp->constraint->min_fields->type);
        set_list_node_value(result->value, 1, sp->constraint->min_fields->value);
        set_list_node_type(result->value, 2, sp->constraint->max_fields->type);
        set_list_node_value(result->value, 2, sp->constraint->max_fields->value);
    }
    else
    {
        set_list_node_type(result->value, 1, INTEGER);
        set_list_node_value(result->value, 1, get_atom_data(theEnv)->zero_atom);
        set_list_node_type(result->value, 2, ATOM);
        set_list_node_value(result->value, 2, get_atom_data(theEnv)->positive_inf_atom);
    }
}

/* =========================================
 *****************************************
 *       INTERNALLY VISIBLE FUNCTIONS
 *  =========================================
 ***************************************** */

/*****************************************************
 *  NAME         : SlotInfoSupportFunction
 *  DESCRIPTION  : Support routine for slot-sources,
 *                slot-facets, et. al.
 *  INPUTS       : 1) Data object buffer
 *              2) Name of the H/L caller
 *              3) Pointer to support function to call
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Support function called and data
 *               object buffer set
 *  NOTES        : None
 *****************************************************/
static void SlotInfoSupportFunction(void *theEnv, core_data_object *result, char *fnxname, void (*fnx)(void *, void *, char *, core_data_object *))
{
    ATOM_HN *ssym;
    DEFCLASS *cls;

    ssym = CheckClassAndSlot(theEnv, fnxname, &cls);

    if( ssym == NULL )
    {
        core_create_error_list(theEnv, result);
        return;
    }

    (*fnx)(theEnv, (void *)cls, to_string(ssym), result);
}

/*****************************************************************
 *  NAME         : CountSubclasses
 *  DESCRIPTION  : Counts the number of direct or indirect
 *                subclasses for a class
 *  INPUTS       : 1) Address of class
 *              2) Include (1) or exclude (0) indirect subclasses
 *              3) Traversal id
 *  RETURNS      : The number of subclasses
 *  SIDE EFFECTS : None
 *  NOTES        : None
 *****************************************************************/
static unsigned CountSubclasses(DEFCLASS *cls, int inhp, int tvid)
{
    long i, cnt;
    register DEFCLASS *subcls;

    for( cnt = 0, i = 0 ; i < cls->directSubclasses.classCount ; i++ )
    {
        subcls = cls->directSubclasses.classArray[i];

        if( TestTraversalID(subcls->traversalRecord, tvid) == 0 )
        {
            cnt++;
            SetTraversalID(subcls->traversalRecord, tvid);

            if( inhp && (subcls->directSubclasses.classCount != 0))
            {
                cnt += CountSubclasses(subcls, inhp, tvid);
            }
        }
    }

    return(cnt);
}

/*********************************************************************
 *  NAME         : StoreSubclasses
 *  DESCRIPTION  : Stores the names of direct or indirect
 *                subclasses for a class in a mutlifield
 *  INPUTS       : 1) Caller's list buffer
 *              2) Starting index
 *              3) Address of the class
 *              4) Include (1) or exclude (0) indirect subclasses
 *              5) Traversal id
 *  RETURNS      : The number of subclass names stored in the list
 *  SIDE EFFECTS : List set with subclass names
 *  NOTES        : Assumes list is big enough to hold subclasses
 *********************************************************************/
static unsigned StoreSubclasses(void *mfval, unsigned si, DEFCLASS *cls, int inhp, int tvid, short storeName)
{
    long i, classi;
    register DEFCLASS *subcls;

    for( i = si, classi = 0 ; classi < cls->directSubclasses.classCount ; classi++ )
    {
        subcls = cls->directSubclasses.classArray[classi];

        if( TestTraversalID(subcls->traversalRecord, tvid) == 0 )
        {
            SetTraversalID(subcls->traversalRecord, tvid);

            if( storeName )
            {
                set_list_node_type(mfval, i, ATOM);
                set_list_node_value(mfval, i++, (void *)GetDefclassNamePointer((void *)subcls));
            }
            else
            {
                set_list_node_type(mfval, i, DEFCLASS_PTR);
                set_list_node_value(mfval, i++, (void *)subcls);
            }

            if( inhp && (subcls->directSubclasses.classCount != 0))
            {
                i += StoreSubclasses(mfval, i, subcls, inhp, tvid, storeName);
            }
        }
    }

    return(i - si);
}

/*********************************************************
 *  NAME         : SlotInfoSlot
 *  DESCRIPTION  : Runtime support routine for slot-sources,
 *                slot-facets, et. al. which looks up
 *                a slot
 *  INPUTS       : 1) Data object buffer
 *              2) Class pointer
 *              3) Name-string of slot to find
 *              4) The name of the calling function
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Support function called and data object
 *               buffer initialized
 *  NOTES        : None
 *********************************************************/
static SLOT_DESC *SlotInfoSlot(void *theEnv, core_data_object *result, DEFCLASS *cls, char *sname, char *fnxname)
{
    ATOM_HN *ssym;
    int i;

    if((ssym = lookup_atom(theEnv, sname)) == NULL )
    {
        core_set_eval_error(theEnv, TRUE);
        core_create_error_list(theEnv, result);
        return(NULL);
    }

    i = FindInstanceTemplateSlot(theEnv, cls, ssym);

    if( i == -1 )
    {
        error_method_duplication(theEnv, sname, fnxname);
        core_set_eval_error(theEnv, TRUE);
        core_create_error_list(theEnv, result);
        return(NULL);
    }

    result->type = LIST;
    result->begin = 0;
    return(cls->instanceTemplate[i]);
}

#endif
