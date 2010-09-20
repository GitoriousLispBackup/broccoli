#ifndef __CLASSES_INSTANCES_KERNEL_H__
#define __CLASSES_INSTANCES_KERNEL_H__

#ifndef __CLASSES_H__
#include "classes.h"
#endif

#ifndef __FUNCS_INSTANCE_H__
#include "funcs_instance.h"
#endif

#define INSTANCE_DATA 29

struct instanceData
{
    INSTANCE_TYPE              DummyInstance;
    INSTANCE_TYPE **           InstanceTable;
    int                        MaintainGarbageInstances;
    int                        MkInsMsgPass;
    int                        ChangesToInstances;
    IGARBAGE *                 InstanceGarbageList;
    struct ext_class_info InstanceInfo;
    INSTANCE_TYPE *            InstanceList;
    unsigned long              GlobalNumberOfInstances;
    INSTANCE_TYPE *            CurrentInstance;
    INSTANCE_TYPE *            InstanceListBottom;
    BOOLEAN                    ObjectModDupMsgValid;
};

#define InstanceData(theEnv) ((struct instanceData *)core_get_environment_data(theEnv, INSTANCE_DATA))

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef __CLASSES_INSTANCES_KERNEL_SOURCE__
#define LOCALE
#else
#define LOCALE extern
#endif

#define CreateRawInstance(a, b)                      EnvCreateRawInstance(GetCurrentEnvironment(), a, b)
#define DeleteInstance(a)                            EnvDeleteInstance(GetCurrentEnvironment(), a)
#define DirectGetSlot(a, b, c)                       EnvDirectGetSlot(GetCurrentEnvironment(), a, b, c)
#define DirectPutSlot(a, b, c)                       EnvDirectPutSlot(GetCurrentEnvironment(), a, b, c)
#define FindInstance(a, b, c)                        EnvFindInstance(GetCurrentEnvironment(), a, b, c)
#define GetInstanceClass(a)                          EnvGetInstanceClass(GetCurrentEnvironment(), a)
#define GetInstanceName(a)                           EnvGetInstanceName(GetCurrentEnvironment(), a)
#define GetInstancePPForm(a, b, c)                   EnvGetInstancePPForm(GetCurrentEnvironment(), a, b, c)
#define GetNextInstance(a)                           EnvGetNextInstance(GetCurrentEnvironment(), a)
#define GetNextInstanceInClass(a, b)                 EnvGetNextInstanceInClass(GetCurrentEnvironment(), a, b)
#define GetNextInstanceInClassAndSubclasses(a, b, c) EnvGetNextInstanceInClassAndSubclasses(GetCurrentEnvironment(), a, b, c)
#define Instances(a, b, c, d)                        EnvInstances(GetCurrentEnvironment(), a, b, c, d)
#define MakeInstance(a)                              EnvMakeInstance(GetCurrentEnvironment(), a)
#define UnmakeInstance(a)                            EnvUnmakeInstance(GetCurrentEnvironment(), a)
#define ValidInstanceAddress(a)                      EnvValidInstanceAddress(GetCurrentEnvironment(), a)

LOCALE void    SetupInstances(void *);
LOCALE BOOLEAN EnvDeleteInstance(void *, void *);
LOCALE BOOLEAN EnvUnmakeInstance(void *, void *);
#if DEBUGGING_FUNCTIONS
LOCALE void InstancesCommand(void *);
LOCALE void PPInstanceCommand(void *);
LOCALE void EnvInstances(void *, char *, void *, char *, int);
#endif
LOCALE void *        EnvMakeInstance(void *, char *);
LOCALE void *        EnvCreateRawInstance(void *, void *, char *);
LOCALE void *        EnvFindInstance(void *, void *, char *, unsigned);
LOCALE int           EnvValidInstanceAddress(void *, void *);
LOCALE void          EnvDirectGetSlot(void *, void *, char *, core_data_object *);
LOCALE int           EnvDirectPutSlot(void *, void *, char *, core_data_object *);
LOCALE char *        EnvGetInstanceName(void *, void *);
LOCALE void *        EnvGetInstanceClass(void *, void *);
LOCALE unsigned long GetGlobalNumberOfInstances(void *);
LOCALE void *        EnvGetNextInstance(void *, void *);
LOCALE void *        GetNextInstanceInScope(void *, void *);
LOCALE void *        EnvGetNextInstanceInClass(void *, void *, void *);
LOCALE void *        EnvGetNextInstanceInClassAndSubclasses(void *, void **, void *, core_data_object *);
LOCALE void          EnvGetInstancePPForm(void *, char *, unsigned, void *);
LOCALE void          ClassCommand(void *, core_data_object *);
LOCALE BOOLEAN       DeleteInstanceCommand(void *);
LOCALE BOOLEAN       UnmakeInstanceCommand(void *);
LOCALE void          SymbolToInstanceName(void *, core_data_object *);
LOCALE void *        InstanceNameToSymbol(void *);
LOCALE void          InstanceAddressCommand(void *, core_data_object *);
LOCALE void          InstanceNameCommand(void *, core_data_object *);
LOCALE BOOLEAN       InstanceAddressPCommand(void *);
LOCALE BOOLEAN       InstanceNamePCommand(void *);
LOCALE BOOLEAN       InstancePCommand(void *);
LOCALE BOOLEAN       InstanceExistPCommand(void *);
LOCALE BOOLEAN       CreateInstanceHandler(void *);

#endif
