#ifndef __CLASSES_INSTANCES_DEFINE_H__
#define __CLASSES_INSTANCES_DEFINE_H__

#if DEFINSTANCES_CONSTRUCT

#define EnvGetDefinstancesName(theEnv, x)   core_get_atom_string((struct construct_metadata *)x)
#define EnvGetDefinstancesPPForm(theEnv, x) core_get_construct_pp(theEnv, (struct construct_metadata *)x)

#define GetDefinstancesNamePointer(x) core_get_atom_pointer((struct construct_metadata *)x)
#define SetDefinstancesPPForm(d, ppf) core_set_construct_pp(theEnv, (struct construct_metadata *)d, ppf)

#define GetDefinstancesModuleName(x)     core_garner_module((struct construct_metadata *)x)
#define EnvDefinstancesModule(theEnv, x) core_garner_module((struct construct_metadata *)x)

struct definstances;

#ifndef __CORE_CONSTRUCTS_H__
#include "core_constructs.h"
#endif
#ifndef __CORE_CONSTRUCTS_QUERY_H__
#include "core_constructs_query.h"
#endif
#ifndef __MODULES_INIT_H__
#include "modules_init.h"
#endif
#ifndef __CLASSES_H__
#include "classes.h"
#endif

typedef struct definstancesModule
{
    struct module_header header;
} DEFINSTANCES_MODULE;

typedef struct definstances
{
    struct construct_metadata header;
    unsigned    busy;
    core_expression_object *mkinstance;
} DEFINSTANCES;

#define DEFINSTANCES_DATA 22

struct definstancesData
{
    struct construct *DefinstancesConstruct;
    int               DefinstancesModuleIndex;
};

#define DefinstancesData(theEnv) ((struct definstancesData *)core_get_environment_data(theEnv, DEFINSTANCES_DATA))


#ifdef LOCALE
#undef LOCALE
#endif

#ifdef __CLASSES_INSTANCES_DEFINE_SOURCE__
#define LOCALE
#else
#define LOCALE extern
#endif

#define DefinstancesModule(x)      core_garner_module((struct construct_metadata *)x)
#define FindDefinstances(a)        EnvFindDefinstances(GetCurrentEnvironment(), a)
#define GetDefinstancesList(a, b)  EnvGetDefinstancesList(GetCurrentEnvironment(), a, b)
#define GetDefinstancesName(x)     core_get_atom_string((struct construct_metadata *)x)
#define GetDefinstancesPPForm(x)   core_get_construct_pp(GetCurrentEnvironment(), (struct construct_metadata *)x)
#define GetNextDefinstances(a)     EnvGetNextDefinstances(GetCurrentEnvironment(), a)
#define IsDefinstancesDeletable(a) EnvIsDefinstancesDeletable(GetCurrentEnvironment(), a)
#define ListDefinstances(a, b)     EnvListDefinstances(GetCurrentEnvironment(), a, b)
#define Undefinstances(a)          EnvUndefinstances(GetCurrentEnvironment(), a)

LOCALE void    SetupDefinstances(void *);
LOCALE void *  EnvGetNextDefinstances(void *, void *);
LOCALE void *  EnvFindDefinstances(void *, char *);
LOCALE int     EnvIsDefinstancesDeletable(void *, void *);
LOCALE void    UndefinstancesCommand(void *);
LOCALE void *  GetDefinstancesModuleCommand(void *);
LOCALE BOOLEAN EnvUndefinstances(void *, void *);

#if DEBUGGING_FUNCTIONS
LOCALE void PPDefinstancesCommand(void *);
LOCALE void ListDefinstancesCommand(void *);
LOCALE void EnvListDefinstances(void *, char *, struct module_definition *);
#endif

LOCALE void GetDefinstancesListFunction(void *, core_data_object *);
LOCALE void EnvGetDefinstancesList(void *, core_data_object *, struct module_definition *);

#endif

#endif
