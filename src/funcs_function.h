#ifndef __FUNCS_FUNCTIONS_H__
#define __FUNCS_FUNCTIONS_H__

#define get_function_name(env, x)     core_get_atom_string((struct construct_metadata *)x)
#define get_function_pp(env, x)       core_get_construct_pp(env, (struct construct_metadata *)x)
#define get_function_name_ptr(x)      core_get_atom_pointer((struct construct_metadata *)x)
#define set_function_pp(d, ppf)       core_set_construct_pp(env, (struct construct_metadata *)d, ppf)
#define get_function_metadata(env, x) core_garner_module((struct construct_metadata *)x)

typedef struct function_definition FUNCTION_DEFINITION;
typedef struct function_module     FUNCTION_MODULE;

#ifndef __CORE_CONSTRUCTS_QUERY_H__
#include "core_constructs_query.h"
#endif
#ifndef __MODULES_INIT_H__
#include "modules_init.h"
#endif
#ifndef __CORE_EVALUATION_H__
#include "core_evaluation.h"
#endif
#ifndef __EXPRESSIONS_H__
#include "core_expressions.h"
#endif
#ifndef __TYPE_ATOM_H__
#include "type_symbol.h"
#endif

#ifdef LOCALE
#undef LOCALE
#endif
#ifdef __FUNCS_FUNCTIONS_SOURCE__
#define LOCALE
#else
#define LOCALE extern
#endif

struct function_module
{
    struct module_header header;
};

struct function_definition
{
    struct construct_metadata header;
    unsigned                  busy;
    unsigned                  executing;
    unsigned short            trace;
    core_expression_object *  code;
    int                       min_args,
                              max_args,
                              local_variable_count;
};

#define FUNCTION_DATA_INDEX 23

struct function_data
{
    struct construct *      function_construct;
    int                     module_index;
    core_data_entity_object entity_record;
#if DEBUGGING_FUNCTIONS
    unsigned is_watching;
#endif
    struct CodeGeneratorItem *code_generation_item;
    FUNCTION_DEFINITION *     executing_function;
    struct token              input_token;
};

#define get_function_data(env) ((struct function_data *)core_get_environment_data(env, FUNCTION_DATA_INDEX))

LOCALE void                  init_functions(void *);
LOCALE void *                lookup_function(void *, char *);
LOCALE FUNCTION_DEFINITION * lookup_function_in_scope(void *, char *);
LOCALE BOOLEAN               undefine_function(void *, void *);
LOCALE void *                get_next_function(void *, void *);
LOCALE int                   is_function_deletable(void *, void *);
LOCALE void                  broccoli_undefine_function(void *);
LOCALE void *                broccoli_lookup_function_module(void *);
LOCALE int                   verify_function_call(void *, void *, int);
LOCALE void                  remove_function(void *, void *);
LOCALE void                  broccoli_list_functions(void *, core_data_object *);

#define FUNCTIONS_GROUP_NAME            "functions"

#endif
