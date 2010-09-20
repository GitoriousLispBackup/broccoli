/* Purpose: Routines for adding new user or system defined
 *   functions.                                              */

#ifndef __CORE_FUNCTIONS_H__

#define __CORE_FUNCTIONS_H__

#ifndef __TYPE_ATOM_H__
#include "type_symbol.h"
#endif
#ifndef __EXPRESSIONS_H__
#include "core_expressions.h"
#endif

#include "extensions_data.h"

struct core_function_definition
{
    struct atom_hash_node *          function_handle;
    char *                           function_name;
    char                             return_type;
    int                              (*functionPointer)(void);
    struct core_expression *         (*parser)(void *, struct core_expression *, char *);
    char *                           restrictions;
    short int                        overloadable;
    short int                        sequential_usage_allowed;
    short int                        environment_aware;
    short int                        id;
    struct core_function_definition *next;
    struct ext_data *                ext_data;
    void *                           context;
};

#define core_get_return_type(target)                (((struct core_function_definition *)target)->return_type)
#define core_get_expression_return_type(target)     (((struct core_function_definition *)((target)->value))->return_type)
#define core_get_expression_function_ptr(target)    (((struct core_function_definition *)((target)->value))->functionPointer)
#define core_get_expression_function_handle(target) (((struct core_function_definition *)((target)->value))->function_handle)
#define core_get_expression_function_name(target)   (((struct core_function_definition *)((target)->value))->function_name)

#define VOID_FN  (int(*) (void))
#define PTR_FN   (int(*) (void *))

/*==================
 * ENVIRONMENT DATA
 *==================*/

#define EXTERNAL_FUNCTION_DATA_INDEX 50

struct core_function_data
{
    struct core_function_definition *ListOfFunctions;
    struct core_function_hash **     FunctionHashtable;
};

#define core_get_function_data(env) ((struct core_function_data *)core_get_environment_data(env, EXTERNAL_FUNCTION_DATA_INDEX))

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef __CORE_FUNCTIONS_SOURCE__
#define LOCALE
#else
#define LOCALE extern
#endif

#ifdef LOCALE
struct core_function_hash
{
    struct core_function_definition *function_definition;
    struct core_function_hash *      next;
};

#define FUNCTION_HASH_SZ 517
#endif

LOCALE void core_init_function_data(void *);
LOCALE int core_define_function(void *, char *, int, int(*) (void *), char *, char *);
LOCALE int core_add_function_parser(void *, char *, struct core_expression *(*)(void *, struct core_expression *, char *));
LOCALE int                                   core_set_function_overload(void *, char *, int, int);
LOCALE struct core_function_definition     * core_get_function_list(void *);
LOCALE struct core_function_definition     * core_lookup_function(void *, char *);
LOCALE int                                   core_get_function_arg_restriction(struct core_function_definition *, int);
LOCALE char*                                 core_arg_type_of(int);
LOCALE int                                   core_undefine_function(void *, char *);
LOCALE int                                   core_get_min_args(struct core_function_definition *);
LOCALE int                                   core_get_max_args(struct core_function_definition *);

#endif
