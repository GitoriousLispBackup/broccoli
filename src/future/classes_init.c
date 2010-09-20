/* Purpose: Defclass Initialization Routines                  */

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

#include "classes_kernel.h"
#include "classes_meta.h"
#include "funcs_class.h"
#include "classes_info.h"
#include "parser_class.h"
#include "core_constructs_query.h"
#include "parser_constructs.h"
#include "core_environment.h"
#include "core_functions.h"
#include "classes_instances_kernel.h"
#include "core_memory.h"
#include "parser_modules.h"
#include "modules_query.h"
#include "classes_methods_kernel.h"
#include "core_watch.h"

#if DEFINSTANCES_CONSTRUCT
#include "classes_instances_define.h"
#endif

#if INSTANCE_SET_QUERIES
#include "classes_instances_query.h"
#endif

#define __CLASSES_INIT_SOURCE__
#include "classes_init.h"

/* =========================================
 *****************************************
 *                CONSTANTS
 *  =========================================
 ***************************************** */
#define SUPERCLASS_RLN       "is-a"
#define NAME_RLN             "name"
#define INITIAL_OBJECT_NAME  "initial-object"

/* =========================================
 *****************************************
 *   INTERNALLY VISIBLE FUNCTION HEADERS
 *  =========================================
 ***************************************** */

static void SetupDefclasses(void *);
static void DeallocateDefclassData(void *);

static void       DestroyDefclassAction(void *, struct construct_metadata *, void *);
static DEFCLASS * AddSystemClass(void *, char *, DEFCLASS *);
static void *     AllocateModule(void *);
static void       ReturnModule(void *, void *);

#if DEFMODULE_CONSTRUCT
static void UpdateDefclassesScope(void *);
#endif

/* =========================================
 *****************************************
 *       EXTERNALLY VISIBLE FUNCTIONS
 *  =========================================
 ***************************************** */

/**********************************************************
 *  NAME         : SetupObjectSystem
 *  DESCRIPTION  : Initializes all COOL constructs, functions,
 *                and data structures
 *  INPUTS       : None
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : COOL initialized
 *  NOTES        : Order of setup calls is important
 **********************************************************/
globle void SetupObjectSystem(void *theEnv)
{
    core_data_entity_object defclassEntityRecord = {"DEFCLASS_PTR",             DEFCLASS_PTR,               1,                          0,            0,
                                          NULL,                       NULL,                       NULL,                       NULL,         NULL,
                                          DecrementDefclassBusyCount,
                                          IncrementDefclassBusyCount,
                                          NULL,                       NULL,                       NULL,                       NULL,         NULL};

    core_allocate_environment_data(theEnv, DEFCLASS_DATA, sizeof(struct defclassData), NULL);
    core_add_environment_cleaner(theEnv, "defclasses", DeallocateDefclassData, -500);

    memcpy(&DefclassData(theEnv)->DefclassEntityRecord, &defclassEntityRecord, sizeof(struct core_data_entity));

    DefclassData(theEnv)->ClassDefaultsMode = CONVENIENCE_MODE;
    DefclassData(theEnv)->ISA_ATOM = (ATOM_HN *)store_atom(theEnv, SUPERCLASS_RLN);
    inc_atom_count(DefclassData(theEnv)->ISA_ATOM);
    DefclassData(theEnv)->NAME_ATOM = (ATOM_HN *)store_atom(theEnv, NAME_RLN);
    inc_atom_count(DefclassData(theEnv)->NAME_ATOM);

    SetupDefclasses(theEnv);
    SetupInstances(theEnv);
    SetupMessageHandlers(theEnv);

#if DEFINSTANCES_CONSTRUCT
    SetupDefinstances(theEnv);
#endif

#if INSTANCE_SET_QUERIES
    SetupQuery(theEnv);
#endif
}

/**************************************************
 * DeallocateDefclassData: Deallocates environment
 *    data for the defclass construct.
 ***************************************************/
static void DeallocateDefclassData(void *theEnv)
{
    SLOT_NAME *tmpSNPPtr, *nextSNPPtr;
    int i;
    struct defclassModule *theModuleItem;
    void *module_def;
    int bloaded = FALSE;


    /*=============================
     * Destroy all the defclasses.
     *=============================*/

    if( !bloaded )
    {
        core_do_for_all_constructs(theEnv, DestroyDefclassAction, DefclassData(theEnv)->DefclassModuleIndex, FALSE, NULL);

        for( module_def = get_next_module(theEnv, NULL);
             module_def != NULL;
             module_def = get_next_module(theEnv, module_def))
        {
            theModuleItem = (struct defclassModule *)
                            get_module_item(theEnv, (struct module_definition *)module_def,
                                          DefclassData(theEnv)->DefclassModuleIndex);
            core_mem_return_struct(theEnv, defclassModule, theModuleItem);
        }
    }

    /*==========================
     * Remove the class tables.
     *==========================*/

    if( !bloaded )
    {
        if( DefclassData(theEnv)->ClassIDMap != NULL )
        {
            core_mem_free(theEnv, DefclassData(theEnv)->ClassIDMap, DefclassData(theEnv)->AvailClassID * sizeof(DEFCLASS *));
        }
    }

    if( DefclassData(theEnv)->ClassTable != NULL )
    {
        core_mem_free(theEnv, DefclassData(theEnv)->ClassTable, sizeof(DEFCLASS *) * CLASS_TABLE_HASH_SIZE);
    }

    /*==============================
     * Free up the slot name table.
     *==============================*/

    if( !bloaded )
    {
        for( i = 0; i < SLOT_NAME_TABLE_HASH_SIZE; i++ )
        {
            tmpSNPPtr = DefclassData(theEnv)->SlotNameTable[i];

            while( tmpSNPPtr != NULL )
            {
                nextSNPPtr = tmpSNPPtr->nxt;
                core_mem_return_struct(theEnv, slotName, tmpSNPPtr);
                tmpSNPPtr = nextSNPPtr;
            }
        }
    }

    if( DefclassData(theEnv)->SlotNameTable != NULL )
    {
        core_mem_free(theEnv, DefclassData(theEnv)->SlotNameTable, sizeof(SLOT_NAME *) * SLOT_NAME_TABLE_HASH_SIZE);
    }
}

/********************************************************
 * DestroyDefclassAction: Action used to remove defclass
 *   as a result of core_delete_environment.
 *********************************************************/
#if WIN_BTC
#pragma argsused
#endif
static void DestroyDefclassAction(void *theEnv, struct construct_metadata *theConstruct, void *buffer)
{
#if MAC_MCW || WIN_MCW || MAC_XCD
#pragma unused(buffer)
#endif
    struct defclass *theDefclass = (struct defclass *)theConstruct;

    if( theDefclass == NULL )
    {
        return;
    }

    DestroyDefclass(theEnv, theDefclass);
}

/***************************************************************
 *  NAME         : CreateSystemClasses
 *  DESCRIPTION  : Creates the built-in system classes
 *  INPUTS       : None
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : System classes inserted in the
 *  class hash table
 *  NOTES        : The binary/load save indices for the primitive
 *  types (integer, float, symbol and string,
 *  list, external-address)
 *  are very important.  Need to be able to refer
 *  to types with the same index regardless of
 *  whether the object system is installed or
 *  not.  Thus, the bsave/blaod indices of these
 *  classes match their integer codes.
 *  WARNING!!: Assumes no classes exist yet!
 ***************************************************************/
globle void CreateSystemClasses(void *theEnv)
{
    DEFCLASS *user, *any, *primitive, *number, *lexeme, *address, *instance;


    /* ===================================
     *  Add canonical slot name entries for
     *  the is-a and name fields - used for
     *  object patterns
     *  =================================== */
    AddSlotName(theEnv, DefclassData(theEnv)->ISA_ATOM, ISA_ID, TRUE);
    AddSlotName(theEnv, DefclassData(theEnv)->NAME_ATOM, NAME_ID, TRUE);

    /* =========================================================
     *  Bsave Indices for non-primitive classes start at 9
     *  Object is 9, Primitive is 10, Number is 11,
     *  Lexeme is 12, Address is 13, and Instance is 14.
     *  because: float = 0, integer = 1, symbol = 2, string = 3,
     *  list = 4, and external-address = 5 and
     *  unused_1 = 6, instance-adress = 7 and
     *  instance-name = 8.
     *  ========================================================= */
    any = AddSystemClass(theEnv, OBJECT_TYPE_NAME, NULL);
    primitive = AddSystemClass(theEnv, PRIMITIVE_TYPE_NAME, any);
    user = AddSystemClass(theEnv, USER_TYPE_NAME, any);

    number = AddSystemClass(theEnv, NUMBER_TYPE_NAME, primitive);
    DefclassData(theEnv)->PrimitiveClassMap[INTEGER] = AddSystemClass(theEnv, INTEGER_TYPE_NAME, number);
    DefclassData(theEnv)->PrimitiveClassMap[FLOAT] = AddSystemClass(theEnv, FLOAT_TYPE_NAME, number);
    lexeme = AddSystemClass(theEnv, LEXEME_TYPE_NAME, primitive);
    DefclassData(theEnv)->PrimitiveClassMap[ATOM] = AddSystemClass(theEnv, ATOM_TYPE_NAME, lexeme);
    DefclassData(theEnv)->PrimitiveClassMap[STRING] = AddSystemClass(theEnv, STRING_TYPE_NAME, lexeme);
    DefclassData(theEnv)->PrimitiveClassMap[LIST] = AddSystemClass(theEnv, LIST_TYPE_NAME, primitive);
    address = AddSystemClass(theEnv, ADDRESS_TYPE_NAME, primitive);
    DefclassData(theEnv)->PrimitiveClassMap[EXTERNAL_ADDRESS] = AddSystemClass(theEnv, EXTERNAL_ADDRESS_TYPE_NAME, address);
    instance = AddSystemClass(theEnv, INSTANCE_TYPE_NAME, primitive);
    DefclassData(theEnv)->PrimitiveClassMap[INSTANCE_ADDRESS] = AddSystemClass(theEnv, INSTANCE_ADDRESS_TYPE_NAME, instance);
    DefclassData(theEnv)->PrimitiveClassMap[INSTANCE_NAME] = AddSystemClass(theEnv, INSTANCE_NAME_TYPE_NAME, instance);

    /* ================================================================================
     * INSTANCE-ADDRESS is-a INSTANCE and ADDRESS.  The links between INSTANCE-ADDRESS
     * and ADDRESS still need to be made.
     * =============================================================================== */
    AddClassLink(theEnv, &DefclassData(theEnv)->PrimitiveClassMap[INSTANCE_ADDRESS]->directSuperclasses, address, -1);
    AddClassLink(theEnv, &DefclassData(theEnv)->PrimitiveClassMap[INSTANCE_ADDRESS]->allSuperclasses, address, 2);
    AddClassLink(theEnv, &address->directSubclasses, DefclassData(theEnv)->PrimitiveClassMap[INSTANCE_ADDRESS], -1);

    /* =======================================================================
     *  The order of the class in the list MUST correspond to their type codes!
     *  See CONSTANT.H
     *  ======================================================================= */
    core_add_to_module((struct construct_metadata *)DefclassData(theEnv)->PrimitiveClassMap[FLOAT]);
    core_add_to_module((struct construct_metadata *)DefclassData(theEnv)->PrimitiveClassMap[INTEGER]);
    core_add_to_module((struct construct_metadata *)DefclassData(theEnv)->PrimitiveClassMap[ATOM]);
    core_add_to_module((struct construct_metadata *)DefclassData(theEnv)->PrimitiveClassMap[STRING]);
    core_add_to_module((struct construct_metadata *)DefclassData(theEnv)->PrimitiveClassMap[LIST]);
    core_add_to_module((struct construct_metadata *)DefclassData(theEnv)->PrimitiveClassMap[EXTERNAL_ADDRESS]);
    core_add_to_module((struct construct_metadata *)DefclassData(theEnv)->PrimitiveClassMap[INSTANCE_ADDRESS]);
    core_add_to_module((struct construct_metadata *)DefclassData(theEnv)->PrimitiveClassMap[INSTANCE_NAME]);
    core_add_to_module((struct construct_metadata *)any);
    core_add_to_module((struct construct_metadata *)primitive);
    core_add_to_module((struct construct_metadata *)number);
    core_add_to_module((struct construct_metadata *)lexeme);
    core_add_to_module((struct construct_metadata *)address);
    core_add_to_module((struct construct_metadata *)instance);
    core_add_to_module((struct construct_metadata *)user);

    for( any = (DEFCLASS *)EnvGetNextDefclass(theEnv, NULL) ;
         any != NULL ;
         any = (DEFCLASS *)EnvGetNextDefclass(theEnv, (void *)any))
    {
        AssignClassID(theEnv, any);
    }
}

/* =========================================
 *****************************************
 *       INTERNALLY VISIBLE FUNCTIONS
 *  =========================================
 ***************************************** */

/*********************************************************
 *  NAME         : SetupDefclasses
 *  DESCRIPTION  : Initializes Class Hash Table,
 *                Function Parsers, and Data Structures
 *  INPUTS       : None
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS :
 *  NOTES        : None
 *********************************************************/
static void SetupDefclasses(void *theEnv)
{
    core_install_primitive(theEnv, &DefclassData(theEnv)->DefclassEntityRecord, DEFCLASS_PTR);

    DefclassData(theEnv)->DefclassModuleIndex =
        install_module_item(theEnv, "defclass",
                           AllocateModule, ReturnModule,
                           NULL,
                           NULL,
                           EnvFindDefclass);

    DefclassData(theEnv)->DefclassConstruct =  core_define_construct(theEnv, "defclass", "defclasses",
                                                            ParseDefclass,
                                                            EnvFindDefclass,
                                                            core_get_atom_pointer, core_get_construct_pp,
                                                            core_query_module_header, EnvGetNextDefclass,
                                                            core_set_next_construct, EnvIsDefclassDeletable,
                                                            EnvUndefclass,
                                                            RemoveDefclass
                                                            );

    core_add_clear_listener(theEnv, "defclass", InstancesPurge, 0);

    core_add_clear_fn(theEnv, "defclass", CreateSystemClasses, 0);
    InitializeClasses(theEnv);

#if DEFMODULE_CONSTRUCT
    AddPortConstructItem(theEnv, "defclass", ATOM);
    AddAfterModuleDefinedFunction(theEnv, "defclass", UpdateDefclassesScope, 0);
#endif
    core_define_function(theEnv, "undefclass", 'v', PTR_FN UndefclassCommand, "UndefclassCommand", "11w");

    core_add_saver(theEnv, "defclass", SaveDefclasses, 10);

#if DEBUGGING_FUNCTIONS
    core_define_function(theEnv, "list-defclasses", 'v', PTR_FN ListDefclassesCommand, "ListDefclassesCommand", "01");
    core_define_function(theEnv, "ppdefclass", 'v', PTR_FN PPDefclassCommand, "PPDefclassCommand", "11w");
    core_define_function(theEnv, "describe-class", 'v', PTR_FN DescribeClassCommand, "DescribeClassCommand", "11w");
    core_define_function(theEnv, "browse-classes", 'v', PTR_FN BrowseClassesCommand, "BrowseClassesCommand", "01w");
#endif

    core_define_function(theEnv, "get-defclass-list", 'm', PTR_FN GetDefclassListFunction,
                       "GetDefclassListFunction", "01");
    core_define_function(theEnv, "superclassp", 'b', PTR_FN SuperclassPCommand, "SuperclassPCommand", "22w");
    core_define_function(theEnv, "subclassp", 'b', PTR_FN SubclassPCommand, "SubclassPCommand", "22w");
    core_define_function(theEnv, "class-existp", 'b', PTR_FN ClassExistPCommand, "ClassExistPCommand", "11w");
    core_define_function(theEnv, "message-handler-existp", 'b',
                       PTR_FN MessageHandlerExistPCommand, "MessageHandlerExistPCommand", "23w");
    core_define_function(theEnv, "class-abstractp", 'b', PTR_FN ClassAbstractPCommand, "ClassAbstractPCommand", "11w");
    core_define_function(theEnv, "class-slots", 'm', PTR_FN ClassSlotsCommand, "ClassSlotsCommand", "12w");
    core_define_function(theEnv, "class-superclasses", 'm',
                       PTR_FN ClassSuperclassesCommand, "ClassSuperclassesCommand", "12w");
    core_define_function(theEnv, "class-subclasses", 'm',
                       PTR_FN ClassSubclassesCommand, "ClassSubclassesCommand", "12w");
    core_define_function(theEnv, "get-defmessage-handler-list", 'm',
                       PTR_FN GetDefmessageHandlersListCmd, "GetDefmessageHandlersListCmd", "02w");
    core_define_function(theEnv, "slot-existp", 'b', PTR_FN SlotExistPCommand, "SlotExistPCommand", "23w");
    core_define_function(theEnv, "slot-facets", 'm', PTR_FN SlotFacetsCommand, "SlotFacetsCommand", "22w");
    core_define_function(theEnv, "slot-sources", 'm', PTR_FN SlotSourcesCommand, "SlotSourcesCommand", "22w");
    core_define_function(theEnv, "slot-types", 'm', PTR_FN SlotTypesCommand, "SlotTypesCommand", "22w");
    core_define_function(theEnv, "slot-allowed-values", 'm', PTR_FN SlotAllowedValuesCommand, "SlotAllowedValuesCommand", "22w");
    core_define_function(theEnv, "slot-allowed-classes", 'm', PTR_FN SlotAllowedClassesCommand, "SlotAllowedClassesCommand", "22w");
    core_define_function(theEnv, "slot-range", 'm', PTR_FN SlotRangeCommand, "SlotRangeCommand", "22w");
    core_define_function(theEnv, "slot-cardinality", 'm', PTR_FN SlotCardinalityCommand, "SlotCardinalityCommand", "22w");
    core_define_function(theEnv, "slot-writablep", 'b', PTR_FN SlotWritablePCommand, "SlotWritablePCommand", "22w");
    core_define_function(theEnv, "slot-initablep", 'b', PTR_FN SlotInitablePCommand, "SlotInitablePCommand", "22w");
    core_define_function(theEnv, "slot-publicp", 'b', PTR_FN SlotPublicPCommand, "SlotPublicPCommand", "22w");
    core_define_function(theEnv, "slot-direct-accessp", 'b', PTR_FN SlotDirectAccessPCommand,
                       "SlotDirectAccessPCommand", "22w");
    core_define_function(theEnv, "slot-default-value", 'u', PTR_FN SlotDefaultValueCommand,
                       "SlotDefaultValueCommand", "22w");
    core_define_function(theEnv, "defclass-module", 'w', PTR_FN GetDefclassModuleCommand,
                       "GetDefclassModuleCommand", "11w");
    core_define_function(theEnv, "get-class-defaults-mode", 'w', PTR_FN GetClassDefaultsModeCommand,  "GetClassDefaultsModeCommand", "00");
    core_define_function(theEnv, "set-class-defaults-mode", 'w', PTR_FN SetClassDefaultsModeCommand,  "SetClassDefaultsModeCommand", "11w");

#if DEBUGGING_FUNCTIONS
    AddWatchItem(theEnv, "instances", 0, &DefclassData(theEnv)->WatchInstances, 75, DefclassWatchAccess, DefclassWatchPrint);
    AddWatchItem(theEnv, "slots", 1, &DefclassData(theEnv)->WatchSlots, 74, DefclassWatchAccess, DefclassWatchPrint);
#endif
}

/*********************************************************
 *  NAME         : AddSystemClass
 *  DESCRIPTION  : Performs all necessary allocations
 *                for adding a system class
 *  INPUTS       : 1) The name-string of the system class
 *              2) The address of the parent class
 *                 (NULL if none)
 *  RETURNS      : The address of the new system class
 *  SIDE EFFECTS : Allocations performed
 *  NOTES        : Assumes system-class name is unique
 *              Also assumes SINGLE INHERITANCE for
 *                system classes to simplify precedence
 *                list determination
 *              Adds classes to has table but NOT to
 *               class list (this is responsibility
 *               of caller)
 *********************************************************/
static DEFCLASS *AddSystemClass(void *theEnv, char *name, DEFCLASS *parent)
{
    DEFCLASS *sys;
    long i;
    char defaultScopeMap[1];

    sys = NewClass(theEnv, (ATOM_HN *)store_atom(theEnv, name));
    sys->abstract = 1;
    inc_atom_count(sys->header.name);
    sys->installed = 1;
    sys->system = 1;
    sys->hashTableIndex = HashClass(sys->header.name);

    AddClassLink(theEnv, &sys->allSuperclasses, sys, -1);

    if( parent != NULL )
    {
        AddClassLink(theEnv, &sys->directSuperclasses, parent, -1);
        AddClassLink(theEnv, &parent->directSubclasses, sys, -1);
        AddClassLink(theEnv, &sys->allSuperclasses, parent, -1);

        for( i = 1 ; i < parent->allSuperclasses.classCount ; i++ )
        {
            AddClassLink(theEnv, &sys->allSuperclasses, parent->allSuperclasses.classArray[i], -1);
        }
    }

    sys->nxtHash = DefclassData(theEnv)->ClassTable[sys->hashTableIndex];
    DefclassData(theEnv)->ClassTable[sys->hashTableIndex] = sys;

    /* =========================================
     *  Add default scope maps for a system class
     *  There is only one module (MAIN) so far -
     *  which has an id of 0
     *  ========================================= */
    init_bitmap((void *)defaultScopeMap, (int)sizeof(char));
    core_bitmap_set(defaultScopeMap, 0);
#if DEFMODULE_CONSTRUCT
    sys->scopeMap = (BITMAP_HN *)store_bitmap(theEnv, (void *)defaultScopeMap, (int)sizeof(char));
    inc_bitmap_count(sys->scopeMap);
#endif
    return(sys);
}

/*****************************************************
 *  NAME         : AllocateModule
 *  DESCRIPTION  : Creates and initializes a
 *              list of deffunctions for a new module
 *  INPUTS       : None
 *  RETURNS      : The new deffunction module
 *  SIDE EFFECTS : Deffunction module created
 *  NOTES        : None
 *****************************************************/
static void *AllocateModule(void *theEnv)
{
    return((void *)core_mem_get_struct(theEnv, defclassModule));
}

/***************************************************
 *  NAME         : ReturnModule
 *  DESCRIPTION  : Removes a deffunction module and
 *              all associated deffunctions
 *  INPUTS       : The deffunction module
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Module and deffunctions deleted
 *  NOTES        : None
 ***************************************************/
static void ReturnModule(void *theEnv, void *theItem)
{
    core_free_construct_header_module(theEnv, (struct module_header *)theItem, DefclassData(theEnv)->DefclassConstruct);
    DeleteSlotName(theEnv, FindIDSlotNameHash(theEnv, ISA_ID));
    DeleteSlotName(theEnv, FindIDSlotNameHash(theEnv, NAME_ID));
    core_mem_return_struct(theEnv, defclassModule, theItem);
}

#if DEFMODULE_CONSTRUCT

/***************************************************
 *  NAME         : UpdateDefclassesScope
 *  DESCRIPTION  : This function updates the scope
 *              bitmaps for existing classes when
 *              a new module is defined
 *  INPUTS       : None
 *  RETURNS      : Nothing
 *  SIDE EFFECTS : Class scope bitmaps are updated
 *  NOTES        : None
 ***************************************************/
static void UpdateDefclassesScope(void *theEnv)
{
    register unsigned i;
    DEFCLASS *theDefclass;
    int newModuleID, count;
    char *newScopeMap;
    unsigned newScopeMapSize;
    char *className;
    struct module_definition *matchModule;

    newModuleID = (int)((struct module_definition *)get_current_module(theEnv))->id;
    newScopeMapSize = (sizeof(char) * ((GetNumberOfDefmodules(theEnv) / BITS_PER_BYTE) + 1));
    newScopeMap = (char *)core_mem_alloc_no_init(theEnv, newScopeMapSize);

    for( i = 0 ; i < CLASS_TABLE_HASH_SIZE ; i++ )
    {
        for( theDefclass = DefclassData(theEnv)->ClassTable[i] ;
             theDefclass != NULL ;
             theDefclass = theDefclass->nxtHash )
        {
            matchModule = theDefclass->header.my_module->module_def;
            className = to_string(theDefclass->header.name);
            init_bitmap((void *)newScopeMap, newScopeMapSize);
            core_mem_copy_memory(char, theDefclass->scopeMap->size,
                          newScopeMap, to_bitmap(theDefclass->scopeMap));
            dec_bitmap_count(theEnv, theDefclass->scopeMap);

            if( theDefclass->system )
            {
                core_bitmap_set(newScopeMap, newModuleID);
            }
            else if( lookup_imported_construct(theEnv, "defclass", matchModule,
                                           className, &count, TRUE, NULL) != NULL )
            {
                core_bitmap_set(newScopeMap, newModuleID);
            }

            theDefclass->scopeMap = (BITMAP_HN *)store_bitmap(theEnv, (void *)newScopeMap, newScopeMapSize);
            inc_bitmap_count(theDefclass->scopeMap);
        }
    }

    core_mem_release(theEnv, (void *)newScopeMap, newScopeMapSize);
}

#endif

#endif
