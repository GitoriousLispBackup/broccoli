#ifndef __CLASSES_INSTANCES_QUERY_H__
#define __CLASSES_INSTANCES_QUERY_H__

#if INSTANCE_SET_QUERIES

#ifndef __CLASSES_H__
#include "classes.h"
#endif

typedef struct query_class
{
    DEFCLASS *cls;
    struct module_definition *module_def;
    struct query_class *chain, *nxt;
} QUERY_CLASS;

typedef struct query_soln
{
    INSTANCE_TYPE **soln;
    struct query_soln *nxt;
} QUERY_SOLN;

typedef struct query_core
{
    INSTANCE_TYPE **solns;
    core_expression_object *    query, *action;
    QUERY_SOLN *    soln_set, *soln_bottom;
    unsigned        soln_size, soln_cnt;
    core_data_object *   result;
} QUERY_CORE;

typedef struct query_stack
{
    QUERY_CORE *core;
    struct query_stack *nxt;
} QUERY_STACK;

#define INSTANCE_QUERY_DATA 31

struct instanceQueryData
{
    ATOM_HN *    QUERY_DELIMETER_ATOM;
    QUERY_CORE * QueryCore;
    QUERY_STACK *QueryCoreStack;
    int          AbortQuery;
};

#define InstanceQueryData(theEnv) ((struct instanceQueryData *)core_get_environment_data(theEnv, INSTANCE_QUERY_DATA))


#ifdef LOCALE
#undef LOCALE
#endif

#ifdef __CLASSES_INSTANCES_QUERY_SOURCE__
#define LOCALE
#else
#define LOCALE extern
#endif

#define QUERY_DELIMETER_STRING     "(QDS)"

LOCALE void    SetupQuery(void *);
LOCALE void *  GetQueryInstance(void *);
LOCALE void    GetQueryInstanceSlot(void *, core_data_object *);
LOCALE BOOLEAN AnyInstances(void *);
LOCALE void    QueryFindInstance(void *, core_data_object *);
LOCALE void    QueryFindAllInstances(void *, core_data_object *);
LOCALE void    QueryDoForInstance(void *, core_data_object *);
LOCALE void    QueryDoForAllInstances(void *, core_data_object *);
LOCALE void    DelayedQueryDoForAllInstances(void *, core_data_object *);

#endif

#endif
