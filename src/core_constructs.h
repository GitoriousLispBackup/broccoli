#ifndef __CORE_CONSTRUCTS_H__

#define __CORE_CONSTRUCTS_H__

struct construct_metadata;
struct construct;

#ifndef __MODULES_INIT_H__
#include "modules_init.h"
#endif
#ifndef __TYPE_ATOM_H__
#include "type_symbol.h"
#endif

#include "extensions_data.h"

struct construct_metadata
{
    struct atom_hash_node       *name;
    char                        *pp;
    struct module_header        *my_module;
    struct construct_metadata   *next;
    struct ext_data             *ext_data;
};

#define CHS (struct construct_metadata *)

struct construct
{
    char *                       construct_name;
    char *                       construct_name_plural;
    int                          (*parser)(void *, char *);
    void *                       (*finder)(void *, char *);
    struct atom_hash_node *      (*namer)(struct construct_metadata *);
    char *                       (*pper)(void *, struct construct_metadata *);
    struct module_header        *(*module_header_geter)(struct construct_metadata *);
    void *                       (*next_item_geter)(void *, void *);
    void                         (*next_item_seter)(struct construct_metadata *, struct construct_metadata *);
    BOOLEAN                      (*is_deletable_checker)(void *, void *);
    int                          (*deleter)(void *, void *);
    void                         (*freer)(void *, void *);
    struct construct *           next;
};

#ifndef __CORE_EVALUATION_H__
#include "core_evaluation.h"
#endif
#ifndef __CORE_SCANNER_H__
#include "core_scanner.h"
#endif

#define CONSTRUCT_DATA_INDEX 42

struct construct_data
{
    int                             clear_ready_in_progress;
    int                             clear_in_progress;
    int                             reset_ready_in_progress;
    int                             reset_in_progress;
    struct core_gc_call_function   *saver_list;
    BOOLEAN                         verbose_loading;
#if METAOBJECT_PROTOCOL
    unsigned WatchCompilations;
#endif
    struct construct *              construct_list;
    struct core_gc_call_function   *reset_listeners;
    struct core_gc_call_function   *clear_listeners;
    struct core_gc_call_function   *clear_ready_listeners;
    int                             executing;
    int                             (*before_reset_hook)(void *);
    int                             check_syntax;
    int                             parsing;
};

#define core_construct_data(env) ((struct construct_data *)core_get_environment_data(env, CONSTRUCT_DATA_INDEX))

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef __CORE_CONSTRUCTS_SOURCE__
#define LOCALE
#else
#define LOCALE extern
#endif

LOCALE void core_clear(void *);
LOCALE void core_reset(void *);
LOCALE int  core_save(void *, char *);

LOCALE void core_init_constructs(void *);
LOCALE BOOLEAN core_add_saver(void *, char *, void(*) (void *, void *, char *), int);
LOCALE BOOLEAN core_remove_saver(void *, char *);
LOCALE BOOLEAN core_add_reset_listener(void *, char *, void(*) (void *), int);
LOCALE BOOLEAN core_remove_reset_listener(void *, char *);
LOCALE BOOLEAN core_add_clear_listener(void *, char *, int(*) (void *), int);
LOCALE BOOLEAN core_remove_clear_listener(void *, char *);
LOCALE BOOLEAN core_add_clear_fn(void *, char *, void(*) (void *), int);
LOCALE BOOLEAN core_remove_clear_fn(void *, char *);
LOCALE struct construct              *core_define_construct(void *, char *, char *,
                                                            int(*) (void *, char *),
                                                            void *(*)(void *, char *),
                                                            ATOM_HN *(*)(struct construct_metadata *),
                                                            char *(*)(void *, struct construct_metadata *),
                                                            struct module_header *(*)(struct construct_metadata *),
                                                            void *(*)(void *, void *),
                                                            void(*) (struct construct_metadata *, struct construct_metadata *),
                                                            BOOLEAN(*) (void *, void *),
                                                            int(*) (void *, void *),
                                                            void(*) (void *, void *));
LOCALE int core_undefine_construct(void *, char *);
LOCALE void core_set_verbose_loading(void *, BOOLEAN);
LOCALE BOOLEAN core_is_verbose_loading(void *);
LOCALE int     core_is_construct_executing(void *);
LOCALE void    core_set_construct_executing(void *, int);
LOCALE void    core_init_constructs_manager(void *);
LOCALE int(*core_set_reset_fn(void *, int(*) (void *))) (void *);
LOCALE void core_garner_construct_list_by_name(void *, core_data_object *, void *(*)(void *, void *), char *(*)(void *, void *));
LOCALE struct construct              * core_lookup_construct_by_name(void *, char *);
LOCALE void                            core_delete_construct_metadata(void *, struct construct_metadata *);

LOCALE void broccoli_reset(void *);
LOCALE void broccoli_clear(void *);

#if METAOBJECT_PROTOCOL
LOCALE void     mopSetWatchCompilations(void *, unsigned);
LOCALE unsigned mopIsWatchCompilations(void *);
#endif

/***
 * Function names and constraints
 **/
#define FUNC_NAME_CLEAR_ALL         "clear"
#define FUNC_NAME_RESET             "reset"

#define FUNC_CNSTR_CLEAR_ALL        "00"
#define FUNC_CNSTR_RESET            "00"
#endif
