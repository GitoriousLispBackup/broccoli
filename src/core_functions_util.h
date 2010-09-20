#ifndef __CORE_WILDCARDS_H__
#define __CORE_WILDCARDS_H__

#ifndef __EXPRESSIONS_H__
#include "core_expressions.h"
#endif
#ifndef __CORE_EVALUATION_H__
#include "core_evaluation.h"
#endif
#ifndef __MODULES_INIT_H__
#include "modules_init.h"
#endif
#ifndef __CORE_SCANNER_H__
#include "core_scanner.h"
#endif
#ifndef __TYPE_ATOM_H__
#include "type_symbol.h"
#endif

typedef struct core_function_parameter_stack
{
    core_data_object                        *arguments;

#if DEFGENERIC_CONSTRUCT
    core_expression_object                  *generic_args;
#endif

    int                                      arguments_sz;
    core_data_object                        *optional_argument;
    void (*fn_unbound_error)(void *);
    struct core_function_parameter_stack    *nxt;
} FUNCTION_ARGUMENT_STACK;

#define PROCEDURAL_PRIMITIVE_DATA_INDEX 37

struct core_function_primitive_data
{
    void *                   null_arg_value;
    core_data_object        *arguments;
    int                      arguments_sz;
    core_expression_object * actions;
#if DEFGENERIC_CONSTRUCT
    core_expression_object *function_generic_args;
#endif
    FUNCTION_ARGUMENT_STACK *arg_stack;
    core_data_object *       optional_argument;
    core_data_object *       local_variables;
    void                     (*fn_unbound_error)(void *);
    core_data_entity_object  function_arg_info;
    core_data_entity_object  function_optional_arg_info;
    core_data_entity_object  function_get_info;
    core_data_entity_object  function_bind_info;
    core_data_entity_object  generic_info;
    int                      old_index;
};

#define core_get_function_primitive_data(env) ((struct core_function_primitive_data *)core_get_environment_data(env, PROCEDURAL_PRIMITIVE_DATA_INDEX))

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef __CORE_WILDCARDS_SOURCE__
#define LOCALE
#else
#define LOCALE extern
#endif

LOCALE void core_install_function_primitive(void *);


LOCALE core_expression_object *core_parse_function_args(void *, char *, struct token *, core_expression_object *,
                                                        ATOM_HN * *, int *, int *, int *,
                                                        int(*) (void *, char *));

LOCALE core_expression_object *core_parse_function_actions(void *, char *, char *, struct token *, core_expression_object *, ATOM_HN *,
                                                           int(*) (void *, core_expression_object *, void *),
                                                           int(*) (void *, core_expression_object *, void *),
                                                           int *, void *);

LOCALE BOOLEAN core_replace_function_variable(void *, char *, core_expression_object *, core_expression_object *, ATOM_HN *,
                                              int(*) (void *, core_expression_object *, void *), void *);

#if DEFGENERIC_CONSTRUCT
LOCALE core_expression_object *core_generate_generic_function_reference(void *, int);
#endif

LOCALE void core_push_function_args(void *, core_expression_object *, int, char *, char *, void(*) (void *));
LOCALE void core_pop_function_args(void *);

#if DEFGENERIC_CONSTRUCT
LOCALE core_expression_object *core_garner_generic_args(void *);
#endif

LOCALE void core_eval_function_actions(void *, struct module_definition *, core_expression_object *, int, core_data_object *, void(*) (void *));
LOCALE void core_print_function_args(void *, char *);
LOCALE void core_garner_optional_args(void *, core_data_object *, int);

#endif
