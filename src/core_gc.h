/* Purpose: Provides a set of utility functions useful to
 *   other modules. Primarily these are the functions for
 *   handling periodic garbage collection and appending
 *   string data.                                            */

#ifndef __CORE_GC_H__
#define __CORE_GC_H__

#ifdef LOCALE
#undef LOCALE
#endif

struct core_gc_call_function
{
    char *                        name;
    void                          (*func)(void *);
    int                           priority;
    struct core_gc_call_function *next;
    short int                     environment_aware;
    void *                        context;
};

struct core_gc_tracked_memory
{
    void *                         memory;
    struct core_gc_tracked_memory *next;
    struct core_gc_tracked_memory *prev;
    size_t                         size;
};

#define UTILITY_DATA_INDEX 55

struct core_gc_data
{
    struct core_gc_call_function * fn_cleanup_list;
    struct core_gc_call_function * fn_periodic_list;
    short                          gc_locks;
    short                          is_using_gc_heuristics;
    short                          is_using_periodic_functions;
    short                          is_using_yield_function;
    long                           generational_item_count;
    long                           generational_item_sz;
    long                           generational_item_count_max;
    long                           generational_item_sz_max;
    void                           (*fn_yield_time)(void);
    int                            last_eval_depth;
    struct core_gc_tracked_memory *tracked_memory;
};

#define core_get_gc_data(env) ((struct core_gc_data *)core_get_environment_data(env, UTILITY_DATA_INDEX))

#ifdef __CORE_GC_SOURCE__
#define LOCALE
#else
#define LOCALE extern
#endif

LOCALE void core_init_gc_data(void *);
LOCALE void core_gc_periodic_cleanup(void *, BOOLEAN, BOOLEAN);
LOCALE BOOLEAN core_gc_add_cleanup_function(void *, char *, void(*) (void *), int);
LOCALE char * core_gc_append_strings(void *, char *, char *);
LOCALE char * core_gc_string_printform(void *, char *);
LOCALE char * core_gc_append_to_string(void *, char *, char *, size_t *, size_t *);
LOCALE char *core_gc_append_subset_to_string(void *, char *, char *, size_t, size_t *, size_t *);
LOCALE char *core_gc_expand_string(void *, int, char *, size_t *, size_t *, size_t);
LOCALE struct core_gc_call_function  *core_gc_add_call_function(void *, char *, int, void(*) (void *), struct core_gc_call_function *, BOOLEAN);
LOCALE struct core_gc_call_function       * core_gc_remove_call_function(void *, char *, struct core_gc_call_function *, int *);
LOCALE void                                 core_gc_delete_call_list(void *, struct core_gc_call_function *);
LOCALE void                                 core_gc_yield_time(void *);
LOCALE struct core_gc_tracked_memory          *core_gc_add_tracked_memory(void *, void *, size_t);
LOCALE void core_gc_remove_tracked_memory(void *, struct core_gc_tracked_memory *);

#endif
