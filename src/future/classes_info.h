#ifndef __CLASSES_INFO_H__
#define __CLASSES_INFO_H__

#ifndef __CORE_EVALUATION_H__
#include "core_evaluation.h"
#endif

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef __CLASSES_INFO_SOURCE__
#define LOCALE
#else
#define LOCALE extern
#endif

#define ClassAbstractP(a)                 EnvClassAbstractP(GetCurrentEnvironment(), a)
#define ClassReactiveP(a)                 EnvClassReactiveP(GetCurrentEnvironment(), a)
#define ClassSlots(a, b, c)               EnvClassSlots(GetCurrentEnvironment(), a, b, c)
#define ClassSubclasses(a, b, c)          EnvClassSubclasses(GetCurrentEnvironment(), a, b, c)
#define ClassSuperclasses(a, b, c)        EnvClassSuperclasses(GetCurrentEnvironment(), a, b, c)
#define SlotAllowedValues(a, b, c)        EnvSlotAllowedValues(GetCurrentEnvironment(), a, b, c)
#define SlotAllowedClasses(a, b, c)       EnvSlotAllowedClasses(GetCurrentEnvironment(), a, b, c)
#define SlotCardinality(a, b, c)          EnvSlotCardinality(GetCurrentEnvironment(), a, b, c)
#define SlotFacets(a, b, c)               EnvSlotFacets(GetCurrentEnvironment(), a, b, c)
#define SlotRange(a, b, c)                EnvSlotRange(GetCurrentEnvironment(), a, b, c)
#define SlotSources(a, b, c)              EnvSlotSources(GetCurrentEnvironment(), a, b, c)
#define SlotTypes(a, b, c)                EnvSlotTypes(GetCurrentEnvironment(), a, b, c)
#define GetDefmessageHandlerList(a, b, c) EnvGetDefmessageHandlerList(GetCurrentEnvironment(), a, b, c)

LOCALE BOOLEAN ClassAbstractPCommand(void *);
LOCALE void *  ClassInfoFnxArgs(void *, char *, int *);
LOCALE void    ClassSlotsCommand(void *, core_data_object *);
LOCALE void    ClassSuperclassesCommand(void *, core_data_object *);
LOCALE void    ClassSubclassesCommand(void *, core_data_object *);
LOCALE void    GetDefmessageHandlersListCmd(void *, core_data_object *);
LOCALE void    SlotFacetsCommand(void *, core_data_object *);
LOCALE void    SlotSourcesCommand(void *, core_data_object *);
LOCALE void    SlotTypesCommand(void *, core_data_object *);
LOCALE void    SlotAllowedValuesCommand(void *, core_data_object *);
LOCALE void    SlotAllowedClassesCommand(void *, core_data_object *);
LOCALE void    SlotRangeCommand(void *, core_data_object *);
LOCALE void    SlotCardinalityCommand(void *, core_data_object *);
LOCALE BOOLEAN EnvClassAbstractP(void *, void *);
LOCALE void    EnvClassSlots(void *, void *, core_data_object *, int);
LOCALE void    EnvGetDefmessageHandlerList(void *, void *, core_data_object *, int);
LOCALE void    EnvClassSuperclasses(void *, void *, core_data_object *, int);
LOCALE void    EnvClassSubclasses(void *, void *, core_data_object *, int);
LOCALE void    ClassSubclassAddresses(void *, void *, core_data_object *, int);
LOCALE void    EnvSlotFacets(void *, void *, char *, core_data_object *);
LOCALE void    EnvSlotSources(void *, void *, char *, core_data_object *);
LOCALE void    EnvSlotTypes(void *, void *, char *, core_data_object *);
LOCALE void    EnvSlotAllowedValues(void *, void *, char *, core_data_object *);
LOCALE void    EnvSlotAllowedClasses(void *, void *, char *, core_data_object *);
LOCALE void    EnvSlotRange(void *, void *, char *, core_data_object *);
LOCALE void    EnvSlotCardinality(void *, void *, char *, core_data_object *);

#endif
