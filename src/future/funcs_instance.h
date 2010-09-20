#ifndef __FUNCS_INSTANCE_H__
#define __FUNCS_INSTANCE_H__

#ifndef __CORE_EVALUATION_H__
#include "core_evaluation.h"
#endif
#ifndef __MODULES_INIT_H__
#include "modules_init.h"
#endif
#ifndef __CLASSES_H__
#include "classes.h"
#endif

typedef struct igarbage
{
    INSTANCE_TYPE *ins;
    struct igarbage *nxt;
} IGARBAGE;

#define INSTANCE_TABLE_HASH_SIZE 8191
#define InstanceSizeHeuristic(ins)      sizeof(INSTANCE_TYPE)

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef __FUNCS_INSTANCE_SOURCE__
#define LOCALE
#else
#define LOCALE extern
#endif

#define DecrementInstanceCount(a) EnvDecrementInstanceCount(GetCurrentEnvironment(), a)
#define GetInstancesChanged()     EnvGetInstancesChanged(GetCurrentEnvironment())
#define IncrementInstanceCount(a) EnvIncrementInstanceCount(GetCurrentEnvironment(), a)
#define SetInstancesChanged(a)    EnvSetInstancesChanged(GetCurrentEnvironment(), a)

LOCALE void            EnvIncrementInstanceCount(void *, void *);
LOCALE void            EnvDecrementInstanceCount(void *, void *);
LOCALE void            InitializeInstanceTable(void *);
LOCALE void            CleanupInstances(void *);
LOCALE unsigned        HashInstance(ATOM_HN *);
LOCALE void            DestroyAllInstances(void *);
LOCALE void            RemoveInstanceData(void *, INSTANCE_TYPE *);
LOCALE INSTANCE_TYPE * FindInstanceBySymbol(void *, ATOM_HN *);
LOCALE INSTANCE_TYPE * FindInstanceInModule(void *, ATOM_HN *, struct module_definition *, struct module_definition *, unsigned);
LOCALE INSTANCE_SLOT * FindInstanceSlot(void *, INSTANCE_TYPE *, ATOM_HN *);
LOCALE int             FindInstanceTemplateSlot(void *, DEFCLASS *, ATOM_HN *);
LOCALE int             PutSlotValue(void *, INSTANCE_TYPE *, INSTANCE_SLOT *, core_data_object *, core_data_object *, char *);
LOCALE int             DirectPutSlotValue(void *, INSTANCE_TYPE *, INSTANCE_SLOT *, core_data_object *, core_data_object *);
LOCALE BOOLEAN         ValidSlotValue(void *, core_data_object *, SLOT_DESC *, INSTANCE_TYPE *, char *);
LOCALE INSTANCE_TYPE * CheckInstance(void *, char *);
LOCALE void            NoInstanceError(void *, char *, char *);
LOCALE void            StaleInstanceAddress(void *, char *, int);
LOCALE int             EnvGetInstancesChanged(void *);
LOCALE void            EnvSetInstancesChanged(void *, int);
LOCALE void            PrintSlot(void *, char *, SLOT_DESC *, INSTANCE_TYPE *, char *);
LOCALE void PrintInstanceNameAndClass(void *, char *, INSTANCE_TYPE *, BOOLEAN);
LOCALE void PrintInstanceName(void *, char *, void *);
LOCALE void PrintInstanceLongForm(void *, char *, void *);

#endif
