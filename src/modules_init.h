/* Purpose: Defines basic module_definition primitive functions such
 *   as allocating and deallocating, traversing, and finding
 *   module_definition data structures.                              */

#ifndef __MODULES_INIT_H__
#define __MODULES_INIT_H__

struct module_definition;
struct portable_item;
struct module_header;
struct module_item;

#ifndef _STDIO_INCLUDED_
#include <stdio.h>
#define _STDIO_INCLUDED_
#endif

#ifndef __PARSER_MODULES_H__
#include "parser_modules.h"
#endif
#ifndef __CORE_GC_H__
#include "core_gc.h"
#endif
#ifndef __TYPE_ATOM_H__
#include "type_symbol.h"
#endif
#ifndef __CORE_EVALUATION_H__
#include "core_evaluation.h"
#endif
#ifndef __CORE_CONSTRUCTS_H__
#include "core_constructs.h"
#endif

/*********************************************************************
 * module_definition
 * ----------
 * name: The name of the module_definition (stored as a reference in the
 *   table).
 *
 * pp: The pretty print representation of the module_definition (used by
 *   the save and ppdefmodule commands).
 *
 * items: An array of pointers to the module specific data used
 *   by each construct specified with the install_module_item
 *   function. The data pointer stored in the array is allocated by
 *   the fn_allocate in module_item data structure.
 *
 * imports: The list of items which are being imported by this
 *   module from other modules.
 *
 * next: A pointer to the next module_definition data structure.
 **********************************************************************/
struct module_definition
{
    struct atom_hash_node *   name;
    char *                    pp;
    struct module_header **   items;
    struct portable_item *    imports;
    struct portable_item *    exports;
    unsigned                  visited;
    long                      id;
    struct ext_data *         ext_datum;
    struct module_definition *next;
};

struct portable_item
{
    struct atom_hash_node *parent_module;
    struct atom_hash_node *type;
    struct atom_hash_node *name;
    struct portable_item  *next;
};

struct module_header
{
    struct module_definition  *module_def;
    struct construct_metadata *first_item;
    struct construct_metadata *last_item;
};

#define MODULE_HEADER_PTR (struct module_header *)

/*********************************************************************
 * module_item
 * ----------
 * name: The name of the construct which can be placed in a module.
 *
 * fn_allocate: Used to allocate a data structure containing all
 *   pertinent information related to a specific construct for a
 *   given module. For example, the deffacts construct stores a
 *   pointer to the first and last deffacts for each each module.
 *
 * freer: Used to deallocate a data structure allocated by
 *   the fn_allocate. In addition, the freer deletes
 *   all constructs of the specified type in the given module.
 *
 * fn_not_used: Used during a binary load to establish a
 *   link between the module_definition data structure and the data structure
 *   containing all pertinent module information for a specific
 *   construct.
 *
 * finder: Used to determine if a specified construct is in a
 *   specific module. The name is the specific construct is passed as
 *   a string and the function returns a pointer to the specified
 *   construct if it exists.
 *
 * exportable: If TRUE, then the specified construct type can be
 *   exported (and hence imported). If FALSE, it can't be exported.
 *
 * next: A pointer to the next module_item data structure.
 **********************************************************************/

struct module_item
{
    char *              name;
    int                 index;
    void *              (*fn_allocate)(void *);
    void                (*fn_free)(void *, void *);
    void *              (*fn_not_used)(void *, int);
    void                (*fn_not_used2)(void *, FILE *, int, int, int);
    void *              (*fn_find)(void *, char *);
    struct module_item *next;
};

typedef struct module_stack_element
{
    BOOLEAN can_change;
    struct module_definition *module_def;
    struct module_stack_element*next;
} MODULE_STACK_ELEMENT;

#define MODULE_DATA_INDEX 4

struct module_data
{
    struct module_item *          last_item;
    struct core_gc_call_function *module_change_listeners;
    MODULE_STACK_ELEMENT *        module_stack;
    BOOLEAN                       should_notify_listeners;
    struct module_definition *    submodules;
    struct module_definition *    current_submodule;
    struct module_definition *    last_submodule;
    int                           items_count;
    struct module_item *          items;
    long                          change_index;
    int                           redefinable;
    struct portConstructItem *    exportable_items;
    long                          submodules_count;
    struct core_gc_call_function *module_define_listeners;
};

#define get_module_data(env) ((struct module_data *)core_get_environment_data(env, MODULE_DATA_INDEX))

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef __MODULES_INIT_SOURCE__
#define LOCALE
#else
#define LOCALE extern
#endif

LOCALE void                            init_modules(void *);
LOCALE void                          * lookup_module(void *, char *);
LOCALE char                          * get_module_name(void *, void *);
LOCALE char                          * get_module_pp(void *, void *);
LOCALE void                          * get_next_module(void *, void *);
LOCALE void                            remove_all_modules(void *);

LOCALE int install_module_item(void *, char *,
                               void *(*)(void *),
                               void(*) (void *, void *),
                               void *(*)(void *, int),
                               void(*) (void *, FILE *, int, int, int),
                               void *(*)(void *, char *));

LOCALE void                          * get_module_item(void *, struct module_definition *, int);
LOCALE void                            set_module_item(void *, struct module_definition *, int, void *);
LOCALE void                          * get_current_module(void *);
LOCALE void                          * set_current_module(void *, void *);
LOCALE void                          * broccoli_get_current_module(void *);
LOCALE void                          * broccoli_set_current_module(void *);
LOCALE int                             get_items_count(void *);
LOCALE void                            create_main_module(void *);
LOCALE void                            set_module_list(void *, void *);
LOCALE struct module_item            * get_module_list(void *);
LOCALE struct module_item            * lookup_module_item(void *, char *);
LOCALE void                            save_current_module(void *);
LOCALE void                            restore_current_module(void *);
LOCALE void add_module_change_listener(void *, char *, void(*) (void *), int);
LOCALE void error_illegal_module_specifier(void *);
LOCALE void init_module_data(void *);

#endif
