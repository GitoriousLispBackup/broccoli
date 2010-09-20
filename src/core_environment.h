/* Purpose: Routines for supporting multiple environments.   */

#ifndef __CORE_ENVIRONMENT_H__
#define __CORE_ENVIRONMENT_H__

#ifndef __TYPE_ATOM_H__
#include "type_symbol.h"
#endif

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef __CORE_ENVIRONMENT_SOURCE__
#define LOCALE
#else
#define LOCALE extern
#endif

#define USER_ENVIRONMENT_DATA_INDEX         70
#define MAXIMUM_ENVIRONMENT_POSITIONS       100

struct environment_cleaner
{
    char *                             name;
    void                               (*func)(void *);
    int                                priority;
    struct environment_cleaner        *next;
};

struct core_environment
{
    unsigned int initialized :
    1;
    unsigned long environment_index;
    void *        context;
    void *        router_context;
    void *        function_context;
    void *        callback_context;
    void **       data;
    void(**cleaners) (void *);
    struct environment_cleaner *environment_cleaner_list;
    struct core_environment *   next;
};

typedef struct core_environment   ENVIRONMENT_DATA;
typedef struct core_environment * ENVIRONMENT_DATA_PTR;

#define core_get_environment_data(env, position)        (((struct core_environment *)env)->data[position])
#define core_set_environment_data(env, position, value) (((struct core_environment *)env)->data[position] = value)

LOCALE BOOLEAN core_allocate_environment_data(void *, unsigned int, unsigned long, void(*) (void *));
LOCALE BOOLEAN                         core_delete_environment_data(void);
LOCALE void                          * core_create_environment(void);
LOCALE BOOLEAN                         core_delete_environment(void *);
LOCALE BOOLEAN core_add_environment_cleaner(void *, char *, void(*) (void *), int);
LOCALE void                          * core_set_environment_function_context(void *, void *);
LOCALE void                          * core_get_environment_callback_context(void *);
LOCALE void                          * core_set_environment_callback_context(void *, void *);
LOCALE void                          * core_get_environment_context(void *);
LOCALE void                          * core_set_environment_context(void *, void *);
LOCALE void                          * core_get_environment_router_context(void *);
LOCALE void                          * core_set_environment_router_context(void *, void *);
LOCALE void                          * core_get_environment_function_context(void *);

#endif
