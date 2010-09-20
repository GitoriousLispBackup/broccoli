/* Purpose: Provides routines for evaluating expressions.    */

#ifndef __CORE_EVALUATION_H__

#define __CORE_EVALUATION_H__

struct core_data_entity;
struct core_data;

#ifndef _H_constant
#include "constant.h"
#endif
#ifndef __TYPE_ATOM_H__
#include "type_symbol.h"
#endif
#ifndef __EXPRESSIONS_H__
#include "core_expressions.h"
#endif

struct core_data
{
    void *             metadata;
    unsigned short     type;
    void *             value;
    long               begin;
    long               end;
    struct core_data  *next;
};

typedef struct core_data   core_data_object;
typedef struct core_data * core_data_object_ptr;

typedef struct core_expression FUNCTION_REFERENCE;

#define DATA_OBJECT_PTR_ARG core_data_object_ptr

#include "extensions_data.h"

struct core_data_entity
{
    char *       name;
    unsigned int type :
    13;
    unsigned int lazy :
    1;
    unsigned int bitmap :
    1;
    unsigned int unused :
    1;
    void             (*short_print_fn)(void *, char *, void *);
    void             (*long_print_fn)(void *, char *, void *);
    BOOLEAN          (*delete_fn)(void *, void *);
    BOOLEAN          (*eval_fn)(void *, void *, core_data_object *);
    void *           (*get_next_fn)(void *, void *);
    void             (*decrement_busy)(void *, void *);
    void             (*increment_busy)(void *, void *);
    void             (*propagate_depth)(void *, void *);
    void             (*needs_mark)(void *, void *);
    void             (*install)(void *, void *);
    void             (*deinstall)(void *, void *);
    struct ext_data *ext_data;
};

struct core_external_address_type
{
    char *  name;
    void    (*short_print_fn)(void *, char *, void *);
    void    (*long_print_fn)(void *, char *, void *);
    BOOLEAN (*discard_fn)(void *, void *);
    void    (*new_fn)(void *, core_data_object *);
    BOOLEAN (*call_fn)(void *, core_data_object *, core_data_object *);
};

typedef struct core_data_entity   core_data_entity_object;
typedef struct core_data_entity * core_data_entity_object_pt;

#define core_get_data_length(target)           (((target).end - (target).begin) + 1)
#define core_get_data_ptr_length(target)       (((target)->end - (target)->begin) + 1)
#define core_get_data_start(target)            ((target).begin + 1)
#define core_get_data_end(target)              ((target).end + 1)
#define core_get_data_ptr_start(target)        ((target)->begin + 1)
#define core_get_data_ptr_end(target)          ((target)->end + 1)
#define core_set_data_start(target, val)       ((target).begin = (long)((val) - 1))
#define core_set_data_end(target, val)         ((target).end = (long)((val) - 1))
#define core_set_data_ptr_start(target, val)   ((target)->begin = (long)((val) - 1))
#define core_set_data_ptr_end(target, val)     ((target)->end = (long)((val) - 1))

#define core_convert_data_ptr_to_string(target)          (((struct atom_hash_node *)((target)->value))->contents)
#define core_convert_data_ptr_to_double(target)          (((struct float_hash_node *)((target)->value))->contents)
#define core_convert_data_ptr_to_float(target)           ((float)(((struct float_hash_node *)((target)->value))->contents))
#define core_convert_data_ptr_to_long(target)            (((struct integer_hash_node *)((target)->value))->contents)
#define core_convert_data_ptr_to_integer(target)         ((int)(((struct integer_hash_node *)((target)->value))->contents))
#define core_convert_data_ptr_to_pointer(target)         ((target)->value)
#define core_convert_data_ptr_to_ext_address(target)     (((struct external_address_hash_node *)((target)->value))->address)

#define core_convert_data_to_string(target)          (((struct atom_hash_node *)((target).value))->contents)
#define core_convert_data_to_double(target)          (((struct float_hash_node *)((target).value))->contents)
#define core_convert_data_to_float(target)           ((float)(((struct float_hash_node *)((target).value))->contents))
#define core_convert_data_to_long(target)            (((struct integer_hash_node *)((target).value))->contents)
#define core_convert_data_to_integer(target)         ((int)(((struct integer_hash_node *)((target).value))->contents))
#define core_convert_data_to_pointe(target)          ((target).value)
#define core_convert_data_to_ext_address(target)     (((struct external_address_hash_node *)((target).value))->address))

/***
 * HERE
 **/

#define core_coerce_to_long(t, v)        ((t == INTEGER) ? to_long(v) : (long int)to_double(v))
#define core_coerce_to_integer(t, v)     ((t == INTEGER) ? (int)to_long(v) : (int)to_double(v))
#define core_coerce_to_double(t, v)      ((t == INTEGER) ? (double)to_long(v) : to_double(v))

#define core_get_first_arg()           (core_get_evaluation_data(env)->current_expression->args)
#define core_get_next_arg(ep)          (ep->next_arg)

#define MAXIMUM_PRIMITIVES             150
#define MAXIMUM_EXTERNAL_ADDRESS_TYPES 10
#define EVALUATION_DATA                44
#define BITS_PER_BYTE                  8

#define core_bit_test(n, b)   ((n) & (char)(1 << (b)))
#define core_bit_set(n, b)    (n |= (char)(1 << (b)))
#define core_bit_clear(n, b)  (n &= (char)~(1 << (b)))

#define core_bitmap_test(map, id)  core_bit_test(map[(id) / BITS_PER_BYTE], (id) % BITS_PER_BYTE)
#define core_bitmap_set(map, id)   core_bit_set(map[(id) / BITS_PER_BYTE], (id) % BITS_PER_BYTE)
#define core_bitmap_clear(map, id) core_bit_clear(map[(id) / BITS_PER_BYTE], (id) % BITS_PER_BYTE)

struct core_evaluation_data
{
    struct core_expression *           current_expression;
    int                                eval_error;
    int                                halt;
    int                                eval_depth;
    int                                address_type_count;
    struct core_data_entity *          primitives[MAXIMUM_PRIMITIVES];
    struct core_external_address_type *ext_address_types[MAXIMUM_EXTERNAL_ADDRESS_TYPES];
};

#define core_get_evaluation_data(env) ((struct core_evaluation_data *)core_get_environment_data(env, EVALUATION_DATA))

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef __CORE_EVALUATION_SOURCE__
#define LOCALE
#else
#define LOCALE extern
#endif

LOCALE void core_init_evaluation_data(void *);
LOCALE int  core_eval_expression(void *, struct core_expression *, struct core_data *);
LOCALE void core_set_eval_error(void *, BOOLEAN);
LOCALE int  core_get_eval_error(void *);
LOCALE void core_set_halt_eval(void *, int);
LOCALE int  core_get_halt_eval(void *);
LOCALE void core_release_data(void *, struct core_data *, BOOLEAN);
LOCALE void                     core_print_data(void *, char *, struct core_data *);
LOCALE void                     core_create_error_list(void *, struct core_data *);
LOCALE void                     core_value_increment(void *, struct core_data *);
LOCALE void                     core_value_decrement(void *, struct core_data *);
LOCALE void                     core_pass_return_value(void *, struct core_data *);
LOCALE void                     core_install_data(void *, int, void *);
LOCALE void                     core_decrement_atom(void *, int, void *);
LOCALE struct core_expression*  core_convert_data_to_expression(void *, core_data_object *);
LOCALE unsigned long            core_hash_atom(unsigned short, void *, int);
LOCALE void                     core_install_primitive(void *, struct core_data_entity *, int);
LOCALE int                      core_install_ext_address_type(void *, struct core_external_address_type *);
LOCALE void                     core_copy_data(core_data_object *, core_data_object *);
LOCALE struct core_expression * core_convert_expression_to_function(void *, char *);
LOCALE BOOLEAN                  core_get_function_referrence(void *, char *, FUNCTION_REFERENCE *);

#define ERROR_TAG_EVALUATION            "Evaluation Error "
#define ERROR_MSG_FUNC_NOT_FOUND        "Function not found "
#define ERROR_MSG_VAR_UNBOUND           "Variable undefined "

#endif
