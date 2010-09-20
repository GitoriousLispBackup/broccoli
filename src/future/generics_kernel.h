#ifndef _H_genrccom
#define _H_genrccom

#define EnvGetDefgenericName(theEnv, x)   core_get_atom_string((struct construct_metadata *)x)
#define EnvGetDefgenericPPForm(theEnv, x) core_get_construct_pp(theEnv, (struct construct_metadata *)x)

#define SetNextDefgeneric(g, t)     core_set_next_construct((struct construct_metadata *)g, \
                                                     (struct construct_metadata *)t)
#define GetDefgenericNamePointer(x) core_get_atom_pointer((struct construct_metadata *)x)
#define SetDefgenericPPForm(g, ppf) core_set_construct_pp(theEnv, (struct construct_metadata *)g, ppf)

#define EnvDefgenericModule(theEnv, x) core_garner_module((struct construct_metadata *)x)

#ifndef __CORE_CONSTRUCTS_H__
#include "core_constructs.h"
#endif
#ifndef __CORE_CONSTRUCTS_QUERY_H__
#include "core_constructs_query.h"
#endif
#ifndef __CORE_EVALUATION_H__
#include "core_evaluation.h"
#endif
#ifndef __MODULES_INIT_H__
#include "modules_init.h"
#endif
#ifndef __FUNCS_GENERICS_H__
#include "funcs_generics.h"
#endif
#ifndef __TYPE_ATOM_H__
#include "type_symbol.h"
#endif

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef _GENRCCOM_SOURCE_
#define LOCALE
#else
#define LOCALE extern
#endif

#define DefgenericModule(x)                 core_garner_module((struct construct_metadata *)x)
#define FindDefgeneric(a)                   EnvFindDefgeneric(GetCurrentEnvironment(), a)
#define GetDefgenericList(a, b)             EnvGetDefgenericList(GetCurrentEnvironment(), a, b)
#define GetDefgenericName(x)                core_get_atom_string((struct construct_metadata *)x)
#define GetDefgenericPPForm(x)              core_get_construct_pp(GetCurrentEnvironment(), (struct construct_metadata *)x)
#define GetDefgenericWatch(a)               EnvGetDefgenericWatch(GetCurrentEnvironment(), a)
#define GetNextDefgeneric(a)                EnvGetNextDefgeneric(GetCurrentEnvironment(), a)
#define IsDefgenericDeletable(a)            EnvIsDefgenericDeletable(GetCurrentEnvironment(), a)
#define ListDefgenerics(a, b)               EnvListDefgenerics(GetCurrentEnvironment(), a, b)
#define SetDefgenericWatch(a, b)            EnvSetDefgenericWatch(GetCurrentEnvironment(), a, b)
#define Undefgeneric(a)                     EnvUndefgeneric(GetCurrentEnvironment(), a)
#define GetDefmethodDescription(a, b, c, d) EnvGetDefmethodDescription(GetCurrentEnvironment(), a, b, c, d)
#define GetDefmethodList(a, b)              EnvGetDefmethodList(GetCurrentEnvironment(), a, b)
#define GetDefmethodPPForm(a, b)            EnvGetDefmethodPPForm(GetCurrentEnvironment(), a, b)
#define GetDefmethodWatch(a, b)             EnvGetDefmethodWatch(GetCurrentEnvironment(), a, b)
#define GetMethodRestrictions(a, b, c)      EnvGetMethodRestrictions(GetCurrentEnvironment(), a, b, c)
#define GetNextDefmethod(a, b)              EnvGetNextDefmethod(GetCurrentEnvironment(), a, b)
#define IsDefmethodDeletable(a, b)          EnvIsDefmethodDeletable(GetCurrentEnvironment(), a, b)
#define ListDefmethods(a, b)                EnvListDefmethods(GetCurrentEnvironment(), a, b)
#define SetDefmethodWatch(a, b, c)          EnvSetDefmethodWatch(GetCurrentEnvironment(), a, b, c)
#define Undefmethod(a, b)                   EnvUndefmethod(GetCurrentEnvironment(), a, b)

LOCALE void         SetupGenericFunctions(void *);
LOCALE void *       EnvFindDefgeneric(void *, char *);
LOCALE DEFGENERIC * LookupDefgenericByMdlOrScope(void *, char *);
LOCALE DEFGENERIC * LookupDefgenericInScope(void *, char *);
LOCALE void *       EnvGetNextDefgeneric(void *, void *);
LOCALE long         EnvGetNextDefmethod(void *, void *, long);
LOCALE int          EnvIsDefgenericDeletable(void *, void *);
LOCALE int          EnvIsDefmethodDeletable(void *, void *, long);
LOCALE void         UndefgenericCommand(void *);
LOCALE void *       GetDefgenericModuleCommand(void *);
LOCALE void         UndefmethodCommand(void *);
LOCALE DEFMETHOD *  GetDefmethodPointer(void *, long);

LOCALE BOOLEAN EnvUndefgeneric(void *, void *);
LOCALE BOOLEAN EnvUndefmethod(void *, void *, long);

#if !OBJECT_SYSTEM
LOCALE void TypeCommand(void *, core_data_object *);
#endif

#if DEBUGGING_FUNCTIONS
LOCALE void     EnvGetDefmethodDescription(void *, char *, int, void *, long);
LOCALE unsigned EnvGetDefgenericWatch(void *, void *);
LOCALE void     EnvSetDefgenericWatch(void *, unsigned, void *);
LOCALE unsigned EnvGetDefmethodWatch(void *, void *, long);
LOCALE void     EnvSetDefmethodWatch(void *, unsigned, void *, long);
LOCALE void     PPDefgenericCommand(void *);
LOCALE void     PPDefmethodCommand(void *);
LOCALE void     ListDefmethodsCommand(void *);
LOCALE char *   EnvGetDefmethodPPForm(void *, void *, long);
LOCALE void     ListDefgenericsCommand(void *);
LOCALE void     EnvListDefgenerics(void *, char *, struct module_definition *);
LOCALE void     EnvListDefmethods(void *, char *, void *);
#endif

LOCALE void GetDefgenericListFunction(void *, core_data_object *);
globle void EnvGetDefgenericList(void *, core_data_object *, struct module_definition *);
LOCALE void GetDefmethodListCommand(void *, core_data_object *);
LOCALE void EnvGetDefmethodList(void *, void *, core_data_object *);
LOCALE void GetMethodRestrictionsCommand(void *, core_data_object *);
LOCALE void EnvGetMethodRestrictions(void *, void *, long, core_data_object *);

#endif
