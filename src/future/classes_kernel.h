#ifndef __KERNEL_CLASS_H__
#define __KERNEL_CLASS_H__

#define CONVENIENCE_MODE  0
#define CONSERVATION_MODE 1

#define EnvGetDefclassName(theEnv, x)   core_get_atom_string((struct construct_metadata *)x)
#define EnvGetDefclassPPForm(theEnv, x) core_get_construct_pp(theEnv, (struct construct_metadata *)x)

#define GetDefclassNamePointer(x) core_get_atom_pointer((struct construct_metadata *)x)
#define GetDefclassModule(x)      core_query_module_header((struct construct_metadata *)x)

#define SetNextDefclass(c, t) core_set_next_construct((struct construct_metadata *)c, \
                                               (struct construct_metadata *)t)

#define SetDefclassPPForm(c, ppf) core_set_construct_pp(theEnv, (struct construct_metadata *)c, ppf)

#define EnvDefclassModule(theEnv, x) core_garner_module((struct construct_metadata *)x)

#ifndef __CORE_CONSTRUCTS_QUERY_H__
#include "core_constructs_query.h"
#endif
#ifndef __MODULES_INIT_H__
#include "modules_init.h"
#endif
#ifndef __CLASSES_H__
#include "classes.h"
#endif
#ifndef __TYPE_ATOM_H__
#include "type_symbol.h"
#endif

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef __KERNEL_CLASS_SOURCE__
#define LOCALE
#else
#define LOCALE extern
#endif

#define DefclassModule(x)               core_garner_module((struct construct_metadata *)x)
#define FindDefclass(a)                 EnvFindDefclass(GetCurrentEnvironment(), a)
#define GetDefclassList(a, b)           EnvGetDefclassList(GetCurrentEnvironment(), a, b)
#define GetDefclassName(x)              core_get_atom_string((struct construct_metadata *)x)
#define GetDefclassPPForm(x)            core_get_construct_pp(GetCurrentEnvironment(), (struct construct_metadata *)x)
#define GetDefclassWatchInstances(a)    EnvGetDefclassWatchInstances(GetCurrentEnvironment(), a)
#define GetDefclassWatchSlots(a)        EnvGetDefclassWatchSlots(GetCurrentEnvironment(), a)
#define GetNextDefclass(a)              EnvGetNextDefclass(GetCurrentEnvironment(), a)
#define IsDefclassDeletable(a)          EnvIsDefclassDeletable(GetCurrentEnvironment(), a)
#define ListDefclasses(a, b)            EnvListDefclasses(GetCurrentEnvironment(), a, b)
#define SetDefclassWatchInstances(a, b) EnvSetDefclassWatchInstances(GetCurrentEnvironment(), a, b)
#define SetDefclassWatchSlots(a, b)     EnvSetDefclassWatchSlots(GetCurrentEnvironment(), a, b)
#define Undefclass(a)                   EnvUndefclass(GetCurrentEnvironment(), a)
#define SetClassDefaultsMode(a)         EnvSetClassDefaultsMode(GetCurrentEnvironment(), a)
#define GetClassDefaultsMode()          EnvGetClassDefaultsMode(GetCurrentEnvironment())

LOCALE void *     EnvFindDefclass(void *, char *);
LOCALE DEFCLASS * LookupDefclassByMdlOrScope(void *, char *);
LOCALE DEFCLASS * LookupDefclassInScope(void *, char *);
LOCALE DEFCLASS * LookupDefclassAnywhere(void *, struct module_definition *, char *);
LOCALE BOOLEAN    DefclassInScope(void *, DEFCLASS *, struct module_definition *);
LOCALE void *     EnvGetNextDefclass(void *, void *);
LOCALE BOOLEAN    EnvIsDefclassDeletable(void *, void *);

LOCALE void           UndefclassCommand(void *);
LOCALE unsigned short EnvSetClassDefaultsMode(void *, unsigned short);
LOCALE unsigned short EnvGetClassDefaultsMode(void *);
LOCALE void *         GetClassDefaultsModeCommand(void *);
LOCALE void *         SetClassDefaultsModeCommand(void *);

#if DEBUGGING_FUNCTIONS
LOCALE void     PPDefclassCommand(void *);
LOCALE void     ListDefclassesCommand(void *);
LOCALE void     EnvListDefclasses(void *, char *, struct module_definition *);
LOCALE unsigned EnvGetDefclassWatchInstances(void *, void *);
LOCALE void     EnvSetDefclassWatchInstances(void *, unsigned, void *);
LOCALE unsigned EnvGetDefclassWatchSlots(void *, void *);
LOCALE void     EnvSetDefclassWatchSlots(void *, unsigned, void *);
LOCALE unsigned DefclassWatchAccess(void *, int, unsigned, core_expression_object *);
LOCALE unsigned DefclassWatchPrint(void *, char *, int, core_expression_object *);
#endif

LOCALE void    GetDefclassListFunction(void *, core_data_object *);
LOCALE void    EnvGetDefclassList(void *, core_data_object *, struct module_definition *);
LOCALE BOOLEAN EnvUndefclass(void *, void *);
LOCALE BOOLEAN HasSuperclass(DEFCLASS *, DEFCLASS *);

LOCALE ATOM_HN *CheckClassAndSlot(void *, char *, DEFCLASS **);

LOCALE void SaveDefclasses(void *, void *, char *);

#endif
