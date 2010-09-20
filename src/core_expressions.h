#ifndef __EXPRESSIONS_H__
#define __EXPRESSIONS_H__


struct core_expression;
struct core_expression_hash;

#ifndef __CORE_EXPRESSIONS_OPERATORS_H__
#include "core_expressions_operators.h"
#endif

/*****************************
 * Expression Data Structures
 ******************************/

struct core_expression
{
    unsigned short          type;
    void *                  value;
    struct core_expression *args;
    struct core_expression *next_arg;
};

typedef struct core_expression core_expression_object;

typedef struct core_expression_hash
{
    unsigned value;
    unsigned count;
    struct core_expression *expression;
    struct core_expression_hash *next;
    long id;
} EXPRESSION_HASH;

#define EXPRESSION_HASH_SZ 503

/************************
 * Type and Value Macros
 *************************/

#define core_get_type(target)                 ((target).type)
#define core_get_pointer_type(target)         ((target)->type)
#define core_set_type(target, val)            ((target).type = (unsigned short)(val))
#define core_set_pointer_type(target, val)    ((target)->type = (unsigned short)(val))
#define core_get_value(target)                ((target).value)
#define core_get_pointer_value(target)        ((target)->value)
#define core_set_value(target, val)           ((target).value = (void *)(val))
#define core_set_pointer_value(target, val)   ((target)->value = (void *)(val))

/*******************
 * ENVIRONMENT DATA
 ********************/

#ifndef __PARSER_EXPRESSIONS_H__
#include "parser_expressions.h"
#endif

#define EXPRESSION_DATA_INDEX 45

struct core_expression_data
{
    void *            fn_and;
    void *            fn_or;
    void *            fn_equal;
    void *            fn_not_equal;
    void *            fn_not;
    EXPRESSION_HASH **expression_hash_table;
    SAVED_CONTEXTS *  saved_contexts;
    int               return_context;
    int               break_context;
    BOOLEAN           sequential_operation;
};

#define core_get_expression_data(env) ((struct core_expression_data *)core_get_environment_data(env, EXPRESSION_DATA_INDEX))

/*******************
 * Global Functions
 ********************/

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef _EXPRESSN_SOURCE_
#define LOCALE
#else
#define LOCALE extern
#endif

LOCALE void                                        core_return_expression(void *, struct core_expression *);
LOCALE void                                        core_increment_expression(void *, struct core_expression *);
LOCALE void                                        core_decrement_expression(void *, struct core_expression *);
LOCALE struct core_expression        *             core_pack_expression(void *, struct core_expression *);
LOCALE void                                        core_return_packed_expression(void *, struct core_expression *);
LOCALE void                                        core_init_expression_data(void *);
LOCALE core_expression_object                    * core_add_expression_hash(void *, core_expression_object *);
LOCALE void                                        core_remove_expression_hash(void *, core_expression_object *);

#endif
