#ifndef __FUNCS_CLASS_H__
#define __FUNCS_CLASS_H__

#ifndef __CLASSES_H__
#include "classes.h"
#endif

#define TestTraversalID(traversalRecord, id)  core_bitmap_test(traversalRecord, id)
#define SetTraversalID(traversalRecord, id)   core_bitmap_set(traversalRecord, id)
#define ClearTraversalID(traversalRecord, id) core_bitmap_clear(traversalRecord, id)

#define CLASS_TABLE_HASH_SIZE     167
#define SLOT_NAME_TABLE_HASH_SIZE 167

#define INITIAL_OBJECT_CLASS_NAME "INITIAL-OBJECT"

#define ISA_ID  0
#define NAME_ID 1

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef __FUNCS_CLASS_SOURCE__
#define LOCALE
#else
#define LOCALE extern
#endif

LOCALE void    IncrementDefclassBusyCount(void *, void *);
LOCALE void    DecrementDefclassBusyCount(void *, void *);
LOCALE BOOLEAN InstancesPurge(void *theEnv);

LOCALE void        InitializeClasses(void *);
LOCALE SLOT_DESC * FindClassSlot(DEFCLASS *, ATOM_HN *);
LOCALE void        ClassExistError(void *, char *, char *);
LOCALE void        DeleteClassLinks(void *, CLASS_LINK *);
LOCALE void PrintClassName(void *, char *, DEFCLASS *, BOOLEAN);

#if DEBUGGING_FUNCTIONS
LOCALE void PrintPackedClassLinks(void *, char *, char *, PACKED_CLASS_LINKS *);
#endif

LOCALE void        PutClassInTable(void *, DEFCLASS *);
LOCALE void        RemoveClassFromTable(void *, DEFCLASS *);
LOCALE void        AddClassLink(void *, PACKED_CLASS_LINKS *, DEFCLASS *, int);
LOCALE void        DeleteSubclassLink(void *, DEFCLASS *, DEFCLASS *);
LOCALE DEFCLASS *  NewClass(void *, ATOM_HN *);
LOCALE void        DeletePackedClassLinks(void *, PACKED_CLASS_LINKS *, int);
LOCALE void        AssignClassID(void *, DEFCLASS *);
LOCALE SLOT_NAME * AddSlotName(void *, ATOM_HN *, int, int);
LOCALE void        DeleteSlotName(void *, SLOT_NAME *);
LOCALE void        RemoveDefclass(void *, void *);
LOCALE void        InstallClass(void *, DEFCLASS *, int);
LOCALE void        DestroyDefclass(void *, void *);

LOCALE int  IsClassBeingUsed(DEFCLASS *);
LOCALE int  RemoveAllUserClasses(void *);
LOCALE int  DeleteClassUAG(void *, DEFCLASS *);
LOCALE void MarkBitMapSubclasses(char *, DEFCLASS *, int);

LOCALE short       FindSlotNameID(void *, ATOM_HN *);
LOCALE ATOM_HN *   FindIDSlotName(void *, int);
LOCALE SLOT_NAME * FindIDSlotNameHash(void *, int);
LOCALE int         GetTraversalID(void *);
LOCALE void        ReleaseTraversalID(void *);
LOCALE unsigned    HashClass(ATOM_HN *);

#ifndef _CLASSFUN_SOURCE_

#if DEBUGGING_FUNCTIONS
extern unsigned WatchInstances, WatchSlots;
#endif
#endif

#define DEFCLASS_DATA 21

#define PRIMITIVE_CLASSES 9

struct defclassData
{
    struct construct *DefclassConstruct;
    int               DefclassModuleIndex;
    core_data_entity_object     DefclassEntityRecord;
    DEFCLASS *        PrimitiveClassMap[PRIMITIVE_CLASSES];
    DEFCLASS **       ClassIDMap;
    DEFCLASS **       ClassTable;
    unsigned short    MaxClassID;
    unsigned short    AvailClassID;
    SLOT_NAME **      SlotNameTable;
    ATOM_HN *         ISA_ATOM;
    ATOM_HN *         NAME_ATOM;
#if DEBUGGING_FUNCTIONS
    unsigned WatchInstances;
    unsigned WatchSlots;
#endif
    unsigned short CTID;
    struct token   ObjectParseToken;
    unsigned short ClassDefaultsMode;
};

#define DefclassData(theEnv) ((struct defclassData *)core_get_environment_data(theEnv, DEFCLASS_DATA))

#endif
