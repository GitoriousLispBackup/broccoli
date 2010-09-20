#ifndef __FUNCS_GENERICS_H__
#define __FUNCS_GENERICS_H__

typedef struct defgenericModule DEFGENERIC_MODULE;
typedef struct restriction      RESTRICTION;
typedef struct method           DEFMETHOD;
typedef struct defgeneric       DEFGENERIC;

#ifndef _STDIO_INCLUDED_
#define _STDIO_INCLUDED_
#include <stdio.h>
#endif

#ifndef __CORE_CONSTRUCTS_H__
#include "core_constructs.h"
#endif
#ifndef __MODULES_INIT_H__
#include "modules_init.h"
#endif
#ifndef __TYPE_ATOM_H__
#include "type_symbol.h"
#endif
#ifndef __EXPRESSIONS_H__
#include "core_expressions.h"
#endif
#ifndef __CORE_EVALUATION_H__
#include "core_evaluation.h"
#endif

#if OBJECT_SYSTEM
#ifndef __CLASSES_H__
#include "classes.h"
#endif
#endif

struct defgenericModule
{
    struct module_header header;
};

struct restriction
{
    void **     types;
    core_expression_object *query;
    short       tcnt;
};

struct method
{
    short    index;
    unsigned busy;
    short    restrictionCount;
    short    minRestrictions;
    short    maxRestrictions;
    short    localVarCount;
    unsigned system :
    1;
    unsigned trace :
    1;
    RESTRICTION *    restrictions;
    core_expression_object *     actions;
    char *           ppForm;
    struct ext_data *usrData;
};

struct defgeneric
{
    struct construct_metadata header;
    unsigned               busy, trace;
    DEFMETHOD *            methods;
    short                  mcnt;
    short                  new_index;
};

#define DEFGENERIC_DATA 27

struct defgenericData
{
    struct construct *DefgenericConstruct;
    int               DefgenericModuleIndex;
    core_data_entity_object     GenericEntityRecord;
#if DEBUGGING_FUNCTIONS
    unsigned WatchGenerics;
    unsigned WatchMethods;
#endif
    DEFGENERIC * CurrentGeneric;
    DEFMETHOD *  CurrentMethod;
    core_data_object *GenericCurrentArgument;
    unsigned     OldGenericBusySave;
    struct token GenericInputToken;
};

#define DefgenericData(theEnv)  ((struct defgenericData *)core_get_environment_data(theEnv, DEFGENERIC_DATA))
#define SaveBusyCount(gfunc)    (DefgenericData(theEnv)->OldGenericBusySave = gfunc->busy)
#define RestoreBusyCount(gfunc) (gfunc->busy = DefgenericData(theEnv)->OldGenericBusySave)

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef __FUNCS_GENERICS_SOURCE__
#define LOCALE
#else
#define LOCALE extern
#endif

LOCALE BOOLEAN ClearDefgenericsReady(void *);
LOCALE void *  AllocateDefgenericModule(void *);
LOCALE void    FreeDefgenericModule(void *, void *);


LOCALE int  ClearDefmethods(void *);
LOCALE int  RemoveAllExplicitMethods(void *, DEFGENERIC *);
LOCALE void RemoveDefgeneric(void *, void *);
LOCALE int  ClearDefgenerics(void *);
LOCALE void MethodAlterError(void *, DEFGENERIC *);
LOCALE void DeleteMethodInfo(void *, DEFGENERIC *, DEFMETHOD *);
LOCALE void DestroyMethodInfo(void *, DEFGENERIC *, DEFMETHOD *);
LOCALE int  MethodsExecuting(DEFGENERIC *);
#if !OBJECT_SYSTEM
LOCALE BOOLEAN SubsumeType(int, int);
#endif

LOCALE long FindMethodByIndex(DEFGENERIC *, long);
#if DEBUGGING_FUNCTIONS
LOCALE void PreviewGeneric(void *);
LOCALE void PrintMethod(void *, char *, int, DEFMETHOD *);
#endif
LOCALE DEFGENERIC * CheckGenericExists(void *, char *, char *);
LOCALE long         CheckMethodExists(void *, char *, DEFGENERIC *, long);

#if !OBJECT_SYSTEM
LOCALE char *TypeName(void *, int);
#endif

LOCALE void PrintGenericName(void *, char *, DEFGENERIC *);

#endif
