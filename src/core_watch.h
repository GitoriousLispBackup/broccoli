/* Purpose: Support functions for the watch and unwatch
 *   commands.                                               */

#ifndef __CORE_WATCH_H__
#define __CORE_WATCH_H__

#ifndef __EXPRESSIONS_H__
#include "core_expressions.h"
#endif

#define WATCH_DATA_INDEX 54

struct core_watch_item
{
    char *                  name;
    unsigned *              flag;
    int                     code, priority;
    unsigned                (*fn_access)(void *, int, unsigned, struct core_expression *);
    unsigned                (*fn_print)(void *, char *, int, struct core_expression *);
    struct core_watch_item *next;
};

struct core_watch_data
{
    struct core_watch_item *items;
};

#define core_get_watch_data(env) ((struct core_watch_data *)core_get_environment_data(env, WATCH_DATA_INDEX))

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef __CORE_WATCH_SOURCE__
#define LOCALE
#else
#define LOCALE extern
#endif
#if DEBUGGING_FUNCTIONS
LOCALE BOOLEAN core_watch(void *, char *);
LOCALE BOOLEAN EnvUnwatch(void *, char *);
LOCALE void    InitializeWatchData(void *);
LOCALE int     EnvSetWatchItem(void *, char *, unsigned, struct core_expression *);
LOCALE int     EnvGetWatchItem(void *, char *);
LOCALE BOOLEAN AddWatchItem(void *, char *, int, unsigned *, int, unsigned(*) (void *, int, unsigned, struct core_expression *), unsigned(*) (void *, char *, int, struct core_expression *));
LOCALE char                          * GetNthWatchName(void *, int);
LOCALE int                             GetNthWatchValue(void *, int);
LOCALE void                            WatchCommand(void *);
LOCALE void                            UnwatchCommand(void *);
LOCALE void                            ListWatchItemsCommand(void *);
LOCALE void                            WatchFunctionDefinitions(void *);
LOCALE int                             GetWatchItemCommand(void *);
#endif
#endif
