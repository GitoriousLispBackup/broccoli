/* Purpose: Internal class manipulation routines             */

/* =========================================
 *****************************************
 *            EXTERNAL DEFINITIONS
 *  =========================================
 ***************************************** */

#include <stdlib.h>

#include "setup.h"

#if OBJECT_SYSTEM


#include "classes_kernel.h"
#include "classes_init.h"
#include "constant.h"
#include "core_constructs.h"
#include "core_constructs_query.h"
#include "parser_constructs.h"
#include "core_environment.h"
#include "core_evaluation.h"
#include "classes_instances_kernel.h"
#include "funcs_instance.h"
#include "classes_instances_init.h"
#include "core_memory.h"
#include "modules_query.h"
#include "funcs_method.h"
#include "router.h"
#include "core_scanner.h"
#include "core_gc.h"

#define __FUNCS_CLASS_SOURCE__
#include "funcs_class.h"

/* =========================================
 *****************************************
 *                CONSTANTS
 *  =========================================
 ***************************************** */
#define BIG_PRIME          11329

#define CLASS_ID_MAP_CHUNK 30

#define PUT_PREFIX         "put-"
#define PUT_PREFIX_LENGTH  4

/* =========================================
 *****************************************
 *   INTERNALLY VISIBLE FUNCTION HEADERS
 *  =========================================
 ***************************************** */

static unsigned HashSlotName(ATOM_HN *);

static int  NewSlotNameID(void *);
static void DeassignClassID(void *, unsigned);

/* =========================================
 *****************************************
 *       EXTERNALLY VISIBLE FUNCTIONS
 *  =========================================
 ***************************************** */

/***************************************************
 *  NAME         : IncrementDefclassBusyCount
 *  DESCRIPTION  : Increments use count of defclass
 *  INPUTS       : The class
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Busy count incremented
 *  NOTES        : None
 ***************************************************/
#if WIN_BTC
#pragma argsused
#endif
globle void IncrementDefclassBusyCount(void *theEnv, void *theDefclass)
{
#if MAC_MCW || WIN_MCW || MAC_XCD
#pragma unused(theEnv)
#endif

    ((DEFCLASS *)theDefclass)->busy++;
}

/***************************************************
 *  NAME         : DecrementDefclassBusyCount
 *  DESCRIPTION  : Decrements use count of defclass
 *  INPUTS       : The class
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Busy count decremented
 *  NOTES        : Since use counts are ignored on
 *              a clear and defclasses might be
 *              deleted already anyway, this is
 *              a no-op on a clear
 ***************************************************/
globle void DecrementDefclassBusyCount(void *theEnv, void *theDefclass)
{
    if( !core_construct_data(theEnv)->clear_in_progress )
    {
        ((DEFCLASS *)theDefclass)->busy--;
    }
}

/****************************************************
 *  NAME         : InstancesPurge
 *  DESCRIPTION  : Removes all instances
 *  INPUTS       : None
 *  RETURNS      : TRUE if all instances deleted,
 *              FALSE otherwise
 *  SIDE EFFECTS : The instance hash table is cleared
 *  NOTES        : None
 ****************************************************/
globle BOOLEAN InstancesPurge(void *theEnv)
{
    int svdepth;

    DestroyAllInstances(theEnv);
    svdepth = core_get_evaluation_data(theEnv)->eval_depth;

    if( core_get_evaluation_data(theEnv)->eval_depth == 0 )
    {
        core_get_evaluation_data(theEnv)->eval_depth = -1;
    }

    CleanupInstances(theEnv);
    core_get_evaluation_data(theEnv)->eval_depth = svdepth;
    return((InstanceData(theEnv)->InstanceList != NULL) ? FALSE : TRUE);
}

/***************************************************
 *  NAME         : InitializeClasses
 *  DESCRIPTION  : Allocates class hash table
 *              Initializes class hash table
 *                to all NULL addresses
 *              Creates system classes
 *  INPUTS       : None
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Hash table initialized
 *  NOTES        : None
 ***************************************************/
globle void InitializeClasses(void *theEnv)
{
    register int i;

    DefclassData(theEnv)->ClassTable =
        (DEFCLASS **)core_mem_alloc_no_init(theEnv, (int)(sizeof(DEFCLASS *) * CLASS_TABLE_HASH_SIZE));

    for( i = 0 ; i < CLASS_TABLE_HASH_SIZE ; i++ )
    {
        DefclassData(theEnv)->ClassTable[i] = NULL;
    }

    DefclassData(theEnv)->SlotNameTable =
        (SLOT_NAME **)core_mem_alloc_no_init(theEnv, (int)(sizeof(SLOT_NAME *) * SLOT_NAME_TABLE_HASH_SIZE));

    for( i = 0 ; i < SLOT_NAME_TABLE_HASH_SIZE ; i++ )
    {
        DefclassData(theEnv)->SlotNameTable[i] = NULL;
    }
}

/********************************************************
 *  NAME         : FindClassSlot
 *  DESCRIPTION  : Searches for a named slot in a class
 *  INPUTS       : 1) The class address
 *              2) The symbolic slot name
 *  RETURNS      : Address of slot if found, NULL otherwise
 *  SIDE EFFECTS : None
 *  NOTES        : Only looks in class defn, does not
 *                examine inheritance paths
 ********************************************************/
globle SLOT_DESC *FindClassSlot(DEFCLASS *cls, ATOM_HN *sname)
{
    long i;

    for( i = 0 ; i < cls->slotCount ; i++ )
    {
        if( cls->slots[i].slotName->name == sname )
        {
            return(&cls->slots[i]);
        }
    }

    return(NULL);
}

/***************************************************************
 *  NAME         : ClassExistError
 *  DESCRIPTION  : Prints out error message for non-existent class
 *  INPUTS       : 1) Name of function having the error
 *              2) The name of the non-existent class
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : None
 *  NOTES        : None
 ***************************************************************/
globle void ClassExistError(void *theEnv, char *func, char *cname)
{
    error_print_id(theEnv, "CLASSFUN", 1, FALSE);
    print_router(theEnv, WERROR, "Unable to find class ");
    print_router(theEnv, WERROR, cname);
    print_router(theEnv, WERROR, " in function ");
    print_router(theEnv, WERROR, func);
    print_router(theEnv, WERROR, ".\n");
    core_set_eval_error(theEnv, TRUE);
}

/*********************************************
 *  NAME         : DeleteClassLinks
 *  DESCRIPTION  : Deallocates a class link list
 *  INPUTS       : The address of the list
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : None
 *  NOTES        : None
 *********************************************/
globle void DeleteClassLinks(void *theEnv, CLASS_LINK *clink)
{
    CLASS_LINK *ctmp;

    for( ctmp = clink ; ctmp != NULL ; ctmp = clink )
    {
        clink = clink->nxt;
        core_mem_return_struct(theEnv, classLink, ctmp);
    }
}

/******************************************************
 *  NAME         : PrintClassName
 *  DESCRIPTION  : Displays a class's name
 *  INPUTS       : 1) Logical name of output
 *              2) The class
 *              3) Flag indicating whether to
 *                 print carriage-return at end
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Class name printed (and module name
 *              too if class is not in current module)
 *  NOTES        : None
 ******************************************************/
globle void PrintClassName(void *theEnv, char *logical_name, DEFCLASS *theDefclass, BOOLEAN linefeedFlag)
{
    if((theDefclass->header.my_module->module_def != ((struct module_definition *)get_current_module(theEnv))) &&
       (theDefclass->system == 0))
    {
        print_router(theEnv, logical_name,
                       get_module_name(theEnv, theDefclass->header.my_module->module_def));
        print_router(theEnv, logical_name, "::");
    }

    print_router(theEnv, logical_name, to_string(theDefclass->header.name));

    if( linefeedFlag )
    {
        print_router(theEnv, logical_name, "\n");
    }
}

#if DEBUGGING_FUNCTIONS

/***************************************************
 *  NAME         : PrintPackedClassLinks
 *  DESCRIPTION  : Displays the names of classes in
 *              a list with a title
 *  INPUTS       : 1) The logical name of the output
 *              2) Title string
 *              3) List of class links
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : None
 *  NOTES        : None
 ***************************************************/
globle void PrintPackedClassLinks(void *theEnv, char *logical_name, char *title, PACKED_CLASS_LINKS *plinks)
{
    long i;

    print_router(theEnv, logical_name, title);

    for( i = 0 ; i < plinks->classCount ; i++ )
    {
        print_router(theEnv, logical_name, " ");
        PrintClassName(theEnv, logical_name, plinks->classArray[i], FALSE);
    }

    print_router(theEnv, logical_name, "\n");
}

#endif


/*******************************************************
 *  NAME         : PutClassInTable
 *  DESCRIPTION  : Inserts a class in the class hash table
 *  INPUTS       : The class
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Class inserted
 *  NOTES        : None
 *******************************************************/
globle void PutClassInTable(void *theEnv, DEFCLASS *cls)
{
    cls->hashTableIndex = HashClass(GetDefclassNamePointer((void *)cls));
    cls->nxtHash = DefclassData(theEnv)->ClassTable[cls->hashTableIndex];
    DefclassData(theEnv)->ClassTable[cls->hashTableIndex] = cls;
}

/*********************************************************
 *  NAME         : RemoveClassFromTable
 *  DESCRIPTION  : Removes a class from the class hash table
 *  INPUTS       : The class
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Class removed
 *  NOTES        : None
 *********************************************************/
globle void RemoveClassFromTable(void *theEnv, DEFCLASS *cls)
{
    DEFCLASS *prvhsh, *hshptr;

    prvhsh = NULL;
    hshptr = DefclassData(theEnv)->ClassTable[cls->hashTableIndex];

    while( hshptr != cls )
    {
        prvhsh = hshptr;
        hshptr = hshptr->nxtHash;
    }

    if( prvhsh == NULL )
    {
        DefclassData(theEnv)->ClassTable[cls->hashTableIndex] = cls->nxtHash;
    }
    else
    {
        prvhsh->nxtHash = cls->nxtHash;
    }
}

/***************************************************
 *  NAME         : AddClassLink
 *  DESCRIPTION  : Adds a class link from one class to
 *               another
 *  INPUTS       : 1) The packed links in which to
 *                 insert the new class
 *              2) The subclass pointer
 *              3) Index of where to place the
 *                 class (-1 to append)
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Link created and attached
 *  NOTES        : Assumes the pack structure belongs
 *              to a class and does not need to
 *              be deallocated
 ***************************************************/
globle void AddClassLink(void *theEnv, PACKED_CLASS_LINKS *src, DEFCLASS *cls, int posn)
{
    PACKED_CLASS_LINKS dst;

    dst.classArray = (DEFCLASS **)core_mem_alloc_no_init(theEnv, (sizeof(DEFCLASS *) * (src->classCount + 1)));

    if( posn == -1 )
    {
        core_mem_copy_memory(DEFCLASS *, src->classCount, dst.classArray, src->classArray);
        dst.classArray[src->classCount] = cls;
    }
    else
    {
        if( posn != 0 )
        {
            core_mem_copy_memory(DEFCLASS *, posn, dst.classArray, src->classArray);
        }

        core_mem_copy_memory(DEFCLASS *, src->classCount - posn,
                      dst.classArray + posn + 1, src->classArray + posn);
        dst.classArray[posn] = cls;
    }

    dst.classCount = (unsigned short)(src->classCount + 1);
    DeletePackedClassLinks(theEnv, src, FALSE);
    src->classCount = dst.classCount;
    src->classArray = dst.classArray;
}

/***************************************************
 *  NAME         : DeleteSubclassLink
 *  DESCRIPTION  : Removes a class from another
 *              class's subclass list
 *  INPUTS       : 1) The superclass whose subclass
 *                 list is to be modified
 *              2) The subclass to be removed
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : The subclass list is changed
 *  NOTES        : None
 ***************************************************/
globle void DeleteSubclassLink(void *theEnv, DEFCLASS *sclass, DEFCLASS *cls)
{
    long deletedIndex;
    PACKED_CLASS_LINKS *src, dst;

    src = &sclass->directSubclasses;

    for( deletedIndex = 0 ; deletedIndex < src->classCount ; deletedIndex++ )
    {
        if( src->classArray[deletedIndex] == cls )
        {
            break;
        }
    }

    if( deletedIndex == src->classCount )
    {
        return;
    }

    if( src->classCount > 1 )
    {
        dst.classArray = (DEFCLASS **)core_mem_alloc_no_init(theEnv, (sizeof(DEFCLASS *) * (src->classCount - 1)));

        if( deletedIndex != 0 )
        {
            core_mem_copy_memory(DEFCLASS *, deletedIndex, dst.classArray, src->classArray);
        }

        core_mem_copy_memory(DEFCLASS *, src->classCount - deletedIndex - 1,
                      dst.classArray + deletedIndex, src->classArray + deletedIndex + 1);
    }
    else
    {
        dst.classArray = NULL;
    }

    dst.classCount = (unsigned short)(src->classCount - 1);
    DeletePackedClassLinks(theEnv, src, FALSE);
    src->classCount = dst.classCount;
    src->classArray = dst.classArray;
}

/**************************************************************
 *  NAME         : NewClass
 *  DESCRIPTION  : Allocates and initalizes a new class structure
 *  INPUTS       : The symbolic name of the new class
 *  RETURNS      : The address of the new class
 *  SIDE EFFECTS : None
 *  NOTES        : None
 **************************************************************/
globle DEFCLASS *NewClass(void *theEnv, ATOM_HN *className)
{
    register DEFCLASS *cls;

    cls = core_mem_get_struct(theEnv, defclass);
    core_init_construct_header(theEnv, "defclass", (struct construct_metadata *)cls, className);

    cls->id = 0;
    cls->installed = 0;
    cls->busy = 0;
    cls->system = 0;
    cls->abstract = 0;
    cls->reactive = 1;
#if DEBUGGING_FUNCTIONS
    cls->traceInstances = DefclassData(theEnv)->WatchInstances;
    cls->traceSlots = DefclassData(theEnv)->WatchSlots;
#endif
    cls->hashTableIndex = 0;
    cls->directSuperclasses.classCount = 0;
    cls->directSuperclasses.classArray = NULL;
    cls->directSubclasses.classCount = 0;
    cls->directSubclasses.classArray = NULL;
    cls->allSuperclasses.classCount = 0;
    cls->allSuperclasses.classArray = NULL;
    cls->slots = NULL;
    cls->instanceTemplate = NULL;
    cls->slotNameMap = NULL;
    cls->instanceSlotCount = 0;
    cls->localInstanceSlotCount = 0;
    cls->slotCount = 0;
    cls->maxSlotNameID = 0;
    cls->handlers = NULL;
    cls->handlerOrderMap = NULL;
    cls->handlerCount = 0;
    cls->instanceList = NULL;
    cls->instanceListBottom = NULL;
    cls->nxtHash = NULL;
    cls->scopeMap = NULL;
    init_bitmap(cls->traversalRecord, TRAVERSAL_BYTES);
    return(cls);
}

/***************************************************
 *  NAME         : DeletePackedClassLinks
 *  DESCRIPTION  : Dealloacates a contiguous array
 *              holding class links
 *  INPUTS       : 1) The class link segment
 *              2) A flag indicating whether to
 *                 delete the top pack structure
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Class links deallocated
 *  NOTES        : None
 ***************************************************/
globle void DeletePackedClassLinks(void *theEnv, PACKED_CLASS_LINKS *plp, int deleteTop)
{
    if( plp->classCount > 0 )
    {
        core_mem_release(theEnv, (void *)plp->classArray, (sizeof(DEFCLASS *) * plp->classCount));
        plp->classCount = 0;
        plp->classArray = NULL;
    }

    if( deleteTop )
    {
        core_mem_return_struct(theEnv, packedClassLinks, plp);
    }
}

/***************************************************
 *  NAME         : AssignClassID
 *  DESCRIPTION  : Assigns a unique id to a class
 *              and puts its address in the
 *              id map
 *  INPUTS       : The class
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Class id assigned and map set
 *  NOTES        : None
 ***************************************************/
globle void AssignClassID(void *theEnv, DEFCLASS *cls)
{
    register unsigned i;

    if((DefclassData(theEnv)->MaxClassID % CLASS_ID_MAP_CHUNK) == 0 )
    {
        DefclassData(theEnv)->ClassIDMap = (DEFCLASS **)core_mem_realloc(theEnv, (void *)DefclassData(theEnv)->ClassIDMap,
                                                                   (unsigned)(DefclassData(theEnv)->MaxClassID * sizeof(DEFCLASS *)),
                                                                   (unsigned)((DefclassData(theEnv)->MaxClassID + CLASS_ID_MAP_CHUNK) * sizeof(DEFCLASS *)));
        DefclassData(theEnv)->AvailClassID += (unsigned short)CLASS_ID_MAP_CHUNK;

        for( i = DefclassData(theEnv)->MaxClassID ; i < (unsigned)(DefclassData(theEnv)->MaxClassID + CLASS_ID_MAP_CHUNK) ; i++ )
        {
            DefclassData(theEnv)->ClassIDMap[i] = NULL;
        }
    }

    DefclassData(theEnv)->ClassIDMap[DefclassData(theEnv)->MaxClassID] = cls;
    cls->id = DefclassData(theEnv)->MaxClassID++;
}

/*********************************************************
 *  NAME         : AddSlotName
 *  DESCRIPTION  : Adds a new slot entry (or increments
 *              the use count of an existing node).
 *  INPUTS       : 1) The slot name
 *              2) The new canonical id for the slot name
 *              3) A flag indicating whether the
 *                 given id must be used or not
 *  RETURNS      : The id of the (old) node
 *  SIDE EFFECTS : Slot name entry added or use count
 *              incremented
 *  NOTES        : None
 *********************************************************/
globle SLOT_NAME *AddSlotName(void *theEnv, ATOM_HN *slotName, int newid, int usenewid)
{
    SLOT_NAME *snp;
    unsigned hashTableIndex;
    char *buf;
    size_t bufsz;

    hashTableIndex = HashSlotName(slotName);
    snp = DefclassData(theEnv)->SlotNameTable[hashTableIndex];

    while((snp != NULL) ? (snp->name != slotName) : FALSE )
    {
        snp = snp->nxt;
    }

    if( snp != NULL )
    {
        if( usenewid && (newid != snp->id))
        {
            error_system(theEnv, "CLASSFUN", 1);
            exit_router(theEnv, EXIT_FAILURE);
        }

        snp->use++;
    }
    else
    {
        snp = core_mem_get_struct(theEnv, slotName);
        snp->name = slotName;
        snp->hashTableIndex = hashTableIndex;
        snp->use = 1;
        snp->id = (short)(usenewid ? newid : NewSlotNameID(theEnv));
        snp->nxt = DefclassData(theEnv)->SlotNameTable[hashTableIndex];
        DefclassData(theEnv)->SlotNameTable[hashTableIndex] = snp;
        inc_atom_count(slotName);
        bufsz = (sizeof(char) *
                 (PUT_PREFIX_LENGTH + strlen(to_string(slotName)) + 1));
        buf = (char *)core_mem_alloc_no_init(theEnv, bufsz);
        sysdep_strcpy(buf, PUT_PREFIX);
        sysdep_strcat(buf, to_string(slotName));
        snp->putHandlerName = (ATOM_HN *)store_atom(theEnv, buf);
        inc_atom_count(snp->putHandlerName);
        core_mem_release(theEnv, (void *)buf, bufsz);
        snp->save_index = 0L;
    }

    return(snp);
}

/***************************************************
 *  NAME         : DeleteSlotName
 *  DESCRIPTION  : Removes a slot name entry from
 *              the table of all slot names if
 *              no longer in use
 *  INPUTS       : The slot name hash node
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Slot name entry deleted or use
 *              count decremented
 *  NOTES        : None
 ***************************************************/
globle void DeleteSlotName(void *theEnv, SLOT_NAME *slotName)
{
    SLOT_NAME *snp, *prv;

    if( slotName == NULL )
    {
        return;
    }

    prv = NULL;
    snp = DefclassData(theEnv)->SlotNameTable[slotName->hashTableIndex];

    while( snp != slotName )
    {
        prv = snp;
        snp = snp->nxt;
    }

    snp->use--;

    if( snp->use != 0 )
    {
        return;
    }

    if( prv == NULL )
    {
        DefclassData(theEnv)->SlotNameTable[snp->hashTableIndex] = snp->nxt;
    }
    else
    {
        prv->nxt = snp->nxt;
    }

    dec_atom_count(theEnv, snp->name);
    dec_atom_count(theEnv, snp->putHandlerName);
    core_mem_return_struct(theEnv, slotName, snp);
}

/*******************************************************************
 *  NAME         : RemoveDefclass
 *  DESCRIPTION  : Deallocates a class structure and
 *              all its fields - returns all symbols
 *              in use by the class back to the symbol
 *              manager for is_ephemeral removal
 *  INPUTS       : The address of the class
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : None
 *  NOTES        : Assumes class has no subclasses
 *              IMPORTANT WARNING!! : Assumes class
 *                busy count and all instances' busy
 *                counts are 0 and all handlers' busy counts are 0!
 *******************************************************************/
LOCALE void RemoveDefclass(void *theEnv, void *vcls)
{
    DEFCLASS *cls = (DEFCLASS *)vcls;
    HANDLER *hnd;
    long i;

    /* ====================================================
     *  Remove all of this class's superclasses' links to it
     *  ==================================================== */
    for( i = 0 ; i < cls->directSuperclasses.classCount ; i++ )
    {
        DeleteSubclassLink(theEnv, cls->directSuperclasses.classArray[i], cls);
    }

    RemoveClassFromTable(theEnv, cls);

    InstallClass(theEnv, cls, FALSE);

    DeletePackedClassLinks(theEnv, &cls->directSuperclasses, FALSE);
    DeletePackedClassLinks(theEnv, &cls->allSuperclasses, FALSE);
    DeletePackedClassLinks(theEnv, &cls->directSubclasses, FALSE);

    for( i = 0 ; i < cls->slotCount ; i++ )
    {
        if( cls->slots[i].defaultValue != NULL )
        {
            if( cls->slots[i].dynamicDefault )
            {
                core_return_packed_expression(theEnv, (core_expression_object *)cls->slots[i].defaultValue);
            }
            else
            {
                core_mem_return_struct(theEnv, core_data, cls->slots[i].defaultValue);
            }
        }

        DeleteSlotName(theEnv, cls->slots[i].slotName);
        removeConstraint(theEnv, cls->slots[i].constraint);
    }

    if( cls->instanceSlotCount != 0 )
    {
        core_mem_release(theEnv, (void *)cls->instanceTemplate,
           (sizeof(SLOT_DESC *) * cls->instanceSlotCount));
        core_mem_release(theEnv, (void *)cls->slotNameMap,
           (sizeof(unsigned) * (cls->maxSlotNameID + 1)));
    }

    if( cls->slotCount != 0 )
    {
        core_mem_release(theEnv, (void *)cls->slots, (sizeof(SLOT_DESC) * cls->slotCount));
    }

    for( i = 0 ; i < cls->handlerCount ; i++ )
    {
        hnd = &cls->handlers[i];

        if( hnd->actions != NULL )
        {
            core_return_packed_expression(theEnv, hnd->actions);
        }

        if( hnd->pp != NULL )
        {
            core_mem_release(theEnv, (void *)hnd->pp, (sizeof(char) * (strlen(hnd->pp) + 1)));
        }

        if( hnd->ext_data != NULL )
        {
            ext_clear_data(theEnv, hnd->ext_data);
        }
    }

    if( cls->handlerCount != 0 )
    {
        core_mem_release(theEnv, (void *)cls->handlers, (sizeof(HANDLER) * cls->handlerCount));
        core_mem_release(theEnv, (void *)cls->handlerOrderMap, (sizeof(unsigned) * cls->handlerCount));
    }

    SetDefclassPPForm((void *)cls, NULL);
    DeassignClassID(theEnv, (unsigned)cls->id);
    core_mem_return_struct(theEnv, defclass, cls);
}

/*******************************************************************
 *  NAME         : DestroyDefclass
 *  DESCRIPTION  : Deallocates a class structure and
 *              all its fields.
 *  INPUTS       : The address of the class
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : None
 *  NOTES        :
 *******************************************************************/
LOCALE void DestroyDefclass(void *theEnv, void *vcls)
{
    DEFCLASS *cls = (DEFCLASS *)vcls;
    long i;

    HANDLER *hnd;

    DeletePackedClassLinks(theEnv, &cls->directSuperclasses, FALSE);
    DeletePackedClassLinks(theEnv, &cls->allSuperclasses, FALSE);
    DeletePackedClassLinks(theEnv, &cls->directSubclasses, FALSE);

    for( i = 0 ; i < cls->slotCount ; i++ )
    {
        if( cls->slots[i].defaultValue != NULL )
        {
            if( cls->slots[i].dynamicDefault )
            {
                core_return_packed_expression(theEnv, (core_expression_object *)cls->slots[i].defaultValue);
            }
            else
            {
                core_mem_return_struct(theEnv, core_data, cls->slots[i].defaultValue);
            }
        }
    }


    if( cls->instanceSlotCount != 0 )
    {
        core_mem_release(theEnv, (void *)cls->instanceTemplate,
           (sizeof(SLOT_DESC *) * cls->instanceSlotCount));
        core_mem_release(theEnv, (void *)cls->slotNameMap,
           (sizeof(unsigned) * (cls->maxSlotNameID + 1)));
    }

    if( cls->slotCount != 0 )
    {
        core_mem_release(theEnv, (void *)cls->slots, (sizeof(SLOT_DESC) * cls->slotCount));
    }

    for( i = 0 ; i < cls->handlerCount ; i++ )
    {
        hnd = &cls->handlers[i];

        if( hnd->actions != NULL )
        {
            core_return_packed_expression(theEnv, hnd->actions);
        }

        if( hnd->pp != NULL )
        {
            core_mem_release(theEnv, (void *)hnd->pp, (sizeof(char) * (strlen(hnd->pp) + 1)));
        }

        if( hnd->ext_data != NULL )
        {
            ext_clear_data(theEnv, hnd->ext_data);
        }
    }

    if( cls->handlerCount != 0 )
    {
        core_mem_release(theEnv, (void *)cls->handlers, (sizeof(HANDLER) * cls->handlerCount));
        core_mem_release(theEnv, (void *)cls->handlerOrderMap, (sizeof(unsigned) * cls->handlerCount));
    }

    core_delete_construct_metadata(theEnv, &cls->header);

    core_mem_return_struct(theEnv, defclass, cls);
}

/***************************************************
 *  NAME         : InstallClass
 *  DESCRIPTION  : In(De)crements all symbol counts for
 *              for symbols in use by class
 *              Disallows (allows) symbols to become
 *              is_ephemeral.
 *  INPUTS       : 1) The class address
 *              2) 1 - install, 0 - deinstall
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : None
 *  NOTES        : None
 ***************************************************/
globle void InstallClass(void *theEnv, DEFCLASS *cls, int set)
{
    SLOT_DESC *slot;
    HANDLER *hnd;
    long i;

    if((set && cls->installed) ||
       ((set == FALSE) && (cls->installed == 0)))
    {
        return;
    }

    /* ==================================================================
     *  Handler installation is handled when message-handlers are defined:
     *  see ParseDefmessageHandler() in MSGCOM.C
     *
     *  Slot installation is handled by ParseSlot() in CLASSPSR.C
     *  Scope map installation is handled by CreateClassScopeMap()
     *  ================================================================== */
    if( set == FALSE )
    {
        cls->installed = 0;

        dec_atom_count(theEnv, cls->header.name);

#if DEFMODULE_CONSTRUCT
        dec_bitmap_count(theEnv, cls->scopeMap);
#endif
        ext_clear_data(theEnv, cls->header.ext_data);

        for( i = 0 ; i < cls->slotCount ; i++ )
        {
            slot = &cls->slots[i];
            dec_atom_count(theEnv, slot->overrideMessage);

            if( slot->defaultValue != NULL )
            {
                if( slot->dynamicDefault )
                {
                    core_decrement_expression(theEnv, (core_expression_object *)slot->defaultValue);
                }
                else
                {
                    core_value_decrement(theEnv, (core_data_object *)slot->defaultValue);
                }
            }
        }

        for( i = 0 ; i < cls->handlerCount ; i++ )
        {
            hnd = &cls->handlers[i];
            dec_atom_count(theEnv, hnd->name);

            if( hnd->actions != NULL )
            {
                core_decrement_expression(theEnv, hnd->actions);
            }
        }
    }
    else
    {
        cls->installed = 1;
        inc_atom_count(cls->header.name);
    }
}

/***************************************************
 *  NAME         : IsClassBeingUsed
 *  DESCRIPTION  : Checks the busy flag of a class
 *                and ALL classes that inherit from
 *                it to make sure that it is not
 *                in use before deletion
 *  INPUTS       : The class
 *  RETURNS      : TRUE if in use, FALSE otherwise
 *  SIDE EFFECTS : None
 *  NOTES        : Recursively examines all subclasses
 ***************************************************/
globle int IsClassBeingUsed(DEFCLASS *cls)
{
    long i;

    if( cls->busy > 0 )
    {
        return(TRUE);
    }

    for( i = 0 ; i < cls->directSubclasses.classCount ; i++ )
    {
        if( IsClassBeingUsed(cls->directSubclasses.classArray[i]))
        {
            return(TRUE);
        }
    }

    return(FALSE);
}

/***************************************************
 *  NAME         : RemoveAllUserClasses
 *  DESCRIPTION  : Removes all classes
 *  INPUTS       : None
 *  RETURNS      : TRUE if succesful, FALSE otherwise
 *  SIDE EFFECTS : The class hash table is cleared
 *  NOTES        : None
 ***************************************************/
globle int RemoveAllUserClasses(void *theEnv)
{
    void *userClasses, *ctmp;
    int success = TRUE;


    /* ====================================================
     *  Don't delete built-in system classes at head of list
     *  ==================================================== */
    userClasses = EnvGetNextDefclass(theEnv, NULL);

    while( userClasses != NULL )
    {
        if(((DEFCLASS *)userClasses)->system == 0 )
        {
            break;
        }

        userClasses = EnvGetNextDefclass(theEnv, userClasses);
    }

    while( userClasses != NULL )
    {
        ctmp = userClasses;
        userClasses = EnvGetNextDefclass(theEnv, userClasses);

        if( EnvIsDefclassDeletable(theEnv, ctmp))
        {
            undefine_module_construct(theEnv, (struct construct_metadata *)ctmp);
            RemoveDefclass(theEnv, ctmp);
        }
        else
        {
            success = FALSE;
            error_deletion(theEnv, "defclass", EnvGetDefclassName(theEnv, ctmp));
        }
    }

    return(success);
}

/****************************************************
 *  NAME         : DeleteClassUAG
 *  DESCRIPTION  : Deallocates a class and all its
 *              subclasses
 *  INPUTS       : The address of the class
 *  RETURNS      : 1 if successful, 0 otherwise
 *  SIDE EFFECTS : Removes the class from each of
 *              its superclasses' subclass lists
 *  NOTES        : None
 ****************************************************/
globle int DeleteClassUAG(void *theEnv, DEFCLASS *cls)
{
    long subCount;

    while( cls->directSubclasses.classCount != 0 )
    {
        subCount = cls->directSubclasses.classCount;
        DeleteClassUAG(theEnv, cls->directSubclasses.classArray[0]);

        if( cls->directSubclasses.classCount == subCount )
        {
            return(FALSE);
        }
    }

    if( EnvIsDefclassDeletable(theEnv, (void *)cls))
    {
        undefine_module_construct(theEnv, (struct construct_metadata *)cls);
        RemoveDefclass(theEnv, (void *)cls);
        return(TRUE);
    }

    return(FALSE);
}

/*********************************************************
 *  NAME         : MarkBitMapSubclasses
 *  DESCRIPTION  : Recursively marks the ids of a class
 *              and all its subclasses in a bitmap
 *  INPUTS       : 1) The bitmap
 *              2) The class
 *              3) A code indicating whether to set
 *                 or clear the bits of the map
 *                 corresponding to the class ids
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : BitMap marked
 *  NOTES        : IMPORTANT!!!!  Assumes the bitmap is
 *              large enough to hold all ids encountered!
 *********************************************************/
globle void MarkBitMapSubclasses(char *map, DEFCLASS *cls, int set)
{
    long i;

    if( set )
    {
        core_bitmap_set(map, cls->id);
    }
    else
    {
        core_bitmap_clear(map, cls->id);
    }

    for( i = 0 ; i < cls->directSubclasses.classCount ; i++ )
    {
        MarkBitMapSubclasses(map, cls->directSubclasses.classArray[i], set);
    }
}

/***************************************************
 *  NAME         : FindSlotNameID
 *  DESCRIPTION  : Finds the id of a slot name
 *  INPUTS       : The slot name
 *  RETURNS      : The slot name id (-1 if not found)
 *  SIDE EFFECTS : None
 *  NOTES        : A slot name always has the same
 *              id regardless of what class uses
 *              it.  In this way, a slot can
 *              be referred to by index independent
 *              of class.  Each class stores a
 *              map showing which slot name indices
 *              go to which slot.  This provides
 *              for immediate lookup of slots
 *              given the index (object pattern
 *              matching uses this).
 ***************************************************/
globle short FindSlotNameID(void *theEnv, ATOM_HN *slotName)
{
    SLOT_NAME *snp;

    snp = DefclassData(theEnv)->SlotNameTable[HashSlotName(slotName)];

    while((snp != NULL) ? (snp->name != slotName) : FALSE )
    {
        snp = snp->nxt;
    }

    return((snp != NULL) ? (short)snp->id : (short)-1);
}

/***************************************************
 *  NAME         : FindIDSlotName
 *  DESCRIPTION  : Finds the slot anme for an id
 *  INPUTS       : The id
 *  RETURNS      : The slot name (NULL if not found)
 *  SIDE EFFECTS : None
 *  NOTES        : None
 ***************************************************/
globle ATOM_HN *FindIDSlotName(void *theEnv, int id)
{
    SLOT_NAME *snp;

    snp = FindIDSlotNameHash(theEnv, id);
    return((snp != NULL) ? snp->name : NULL);
}

/***************************************************
 *  NAME         : FindIDSlotNameHash
 *  DESCRIPTION  : Finds the slot anme for an id
 *  INPUTS       : The id
 *  RETURNS      : The slot name (NULL if not found)
 *  SIDE EFFECTS : None
 *  NOTES        : None
 ***************************************************/
globle SLOT_NAME *FindIDSlotNameHash(void *theEnv, int id)
{
    register int i;
    SLOT_NAME *snp;

    for( i = 0 ; i < SLOT_NAME_TABLE_HASH_SIZE ; i++ )
    {
        snp = DefclassData(theEnv)->SlotNameTable[i];

        while( snp != NULL )
        {
            if( snp->id == id )
            {
                return(snp);
            }

            snp = snp->nxt;
        }
    }

    return(NULL);
}

/***************************************************
 *  NAME         : GetTraversalID
 *  DESCRIPTION  : Returns a unique integer ID for a
 *               traversal into the class hierarchy
 *  INPUTS       : None
 *  RETURNS      : The id, or -1 if none available
 *  SIDE EFFECTS : eval_error set when no ids
 *                available
 *  NOTES        : Used for recursive traversals of
 *               class hierarchy to assure that a
 *               class is only visited once
 ***************************************************/
globle int GetTraversalID(void *theEnv)
{
    register unsigned i;
    register DEFCLASS *cls;

    if( DefclassData(theEnv)->CTID >= MAX_TRAVERSALS )
    {
        error_print_id(theEnv, "CLASSFUN", 2, FALSE);
        print_router(theEnv, WERROR, "Maximum number of simultaneous class hierarchy\n  traversals exceeded ");
        core_print_long(theEnv, WERROR, (long)MAX_TRAVERSALS);
        print_router(theEnv, WERROR, ".\n");
        core_set_eval_error(theEnv, TRUE);
        return(-1);
    }

    for( i = 0 ; i < CLASS_TABLE_HASH_SIZE ; i++ )
    {
        for( cls = DefclassData(theEnv)->ClassTable[i] ; cls != NULL ; cls = cls->nxtHash )
        {
            ClearTraversalID(cls->traversalRecord, DefclassData(theEnv)->CTID);
        }
    }

    return(DefclassData(theEnv)->CTID++);
}

/***************************************************
 *  NAME         : ReleaseTraversalID
 *  DESCRIPTION  : Releases an ID for future use
 *              Also clears id from all classes
 *  INPUTS       : None
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Old ID released for later reuse
 *  NOTES        : Releases ID returned by most recent
 *                call to GetTraversalID()
 ***************************************************/
globle void ReleaseTraversalID(void *theEnv)
{
    DefclassData(theEnv)->CTID--;
}

/*******************************************************
 *  NAME         : HashClass
 *  DESCRIPTION  : Generates a hash index for a given
 *              class name
 *  INPUTS       : The address of the class name ATOM_HN
 *  RETURNS      : The hash index value
 *  SIDE EFFECTS : None
 *  NOTES        : Counts on the fact that the symbol
 *              has already been hashed into the
 *              symbol table - uses that hash value
 *              multiplied by a prime for a new hash
 *******************************************************/
globle unsigned HashClass(ATOM_HN *cname)
{
    unsigned long tally;

    tally = ((unsigned long)cname->bucket) * BIG_PRIME;
    return((unsigned)(tally % CLASS_TABLE_HASH_SIZE));
}

/* =========================================
 *****************************************
 *       INTERNALLY VISIBLE FUNCTIONS
 *  =========================================
 ***************************************** */

/*******************************************************
 *  NAME         : HashSlotName
 *  DESCRIPTION  : Generates a hash index for a given
 *              slot name
 *  INPUTS       : The address of the slot name ATOM_HN
 *  RETURNS      : The hash index value
 *  SIDE EFFECTS : None
 *  NOTES        : Counts on the fact that the symbol
 *              has already been hashed into the
 *              symbol table - uses that hash value
 *              multiplied by a prime for a new hash
 *******************************************************/
static unsigned HashSlotName(ATOM_HN *sname)
{
    unsigned long tally;

    tally = ((unsigned long)sname->bucket) * BIG_PRIME;
    return((unsigned)(tally % SLOT_NAME_TABLE_HASH_SIZE));
}

/***********************************************
 *  NAME         : NewSlotNameID
 *  DESCRIPTION  : Returns  an unused slot name id
 *              as close to 1 as possible
 *  INPUTS       : None
 *  RETURNS      : The new unused id
 *  SIDE EFFECTS : None
 *  NOTES        : None
 ***********************************************/
static int NewSlotNameID(void *theEnv)
{
    int newid = 0;
    register unsigned i;
    SLOT_NAME *snp;

    while( TRUE )
    {
        for( i = 0 ; i < SLOT_NAME_TABLE_HASH_SIZE ; i++ )
        {
            snp = DefclassData(theEnv)->SlotNameTable[i];

            while((snp != NULL) ? (snp->id != newid) : FALSE )
            {
                snp = snp->nxt;
            }

            if( snp != NULL )
            {
                break;
            }
        }

        if( i < SLOT_NAME_TABLE_HASH_SIZE )
        {
            newid++;
        }
        else
        {
            break;
        }
    }

    return(newid);
}

/***************************************************
 *  NAME         : DeassignClassID
 *  DESCRIPTION  : Reduces id map and MaxClassID if
 *              no ids in use above the one being
 *              released.
 *  INPUTS       : The id
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : ID map and MaxClassID possibly
 *              reduced
 *  NOTES        : None
 ***************************************************/
static void DeassignClassID(void *theEnv, unsigned id)
{
    int i;
    int reallocReqd;
    unsigned short oldChunk = 0, newChunk = 0;

    DefclassData(theEnv)->ClassIDMap[id] = NULL;

    for( i = id + 1 ; i < DefclassData(theEnv)->MaxClassID ; i++ )
    {
        if( DefclassData(theEnv)->ClassIDMap[i] != NULL )
        {
            return;
        }
    }

    reallocReqd = FALSE;

    while( DefclassData(theEnv)->ClassIDMap[id] == NULL )
    {
        DefclassData(theEnv)->MaxClassID = (unsigned short)id;

        if((DefclassData(theEnv)->MaxClassID % CLASS_ID_MAP_CHUNK) == 0 )
        {
            newChunk = DefclassData(theEnv)->MaxClassID;

            if( reallocReqd == FALSE )
            {
                oldChunk = (unsigned short)(DefclassData(theEnv)->MaxClassID + CLASS_ID_MAP_CHUNK);
                reallocReqd = TRUE;
            }
        }

        if( id == 0 )
        {
            break;
        }

        id--;
    }

    if( reallocReqd )
    {
        DefclassData(theEnv)->ClassIDMap = (DEFCLASS **)core_mem_realloc(theEnv, (void *)DefclassData(theEnv)->ClassIDMap,
                                                                   (unsigned)(oldChunk * sizeof(DEFCLASS *)),
                                                                   (unsigned)(newChunk * sizeof(DEFCLASS *)));

        DefclassData(theEnv)->AvailClassID = newChunk;
    }
}

#endif
