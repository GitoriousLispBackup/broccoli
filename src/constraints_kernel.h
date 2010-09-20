/* Purpose: Provides functions for creating and removing
 *   constraint records, adding them to the contraint hash
 *   table, and enabling and disabling static and dynamic
 *   constraint checking.                                    */

#ifndef __CONSTRAINTS_KERNEL_H__
#define __CONSTRAINTS_KERNEL_H__

struct constraint_metadata;

#ifndef __CORE_EVALUATION_H__
#include "core_evaluation.h"
#endif

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef __CONSTRAINTS_SOURCE__
#define LOCALE
#else
#define LOCALE extern
#endif

struct constraint_metadata
{
    unsigned int allow_any :
    1;
    unsigned int allow_atom :
    1;
    unsigned int allow_string :
    1;
    unsigned int allow_float :
    1;
    unsigned int allow_integer :
    1;
    unsigned int allow_instance_name :
    1;
    unsigned int allow_instance_pointer :
    1;
    unsigned int allow_external_pointer :
    1;
    unsigned int allow_void :
    1;
    unsigned int restrict_any :
    1;
    unsigned int restrict_atom :
    1;
    unsigned int restrict_string :
    1;
    unsigned int restrict_float :
    1;
    unsigned int restrict_integer :
    1;
    unsigned int restrict_class :
    1;
    unsigned int restrict_instance_name :
    1;
    unsigned int allow_list :
    1;
    unsigned int allow_scalar :
    1;
    unsigned short              save_index;
    struct core_expression *    class_list;
    struct core_expression *    restriction_list;
    struct core_expression *    min_value;
    struct core_expression *    max_value;
    struct core_expression *    min_fields;
    struct core_expression *    max_fields;
    struct constraint_metadata *list;
    struct constraint_metadata *next;
    int                         bucket;
    int                         count;
};

typedef struct constraint_metadata CONSTRAINT_META;

#define CONSTRAINT_HASH_SZ            167
#define CONSTRAINT_DATA_INDEX         43

struct constraint_data
{
    struct constraint_metadata**hashtable;
    BOOLEAN                     is_static;
    BOOLEAN                     is_dynamic;
};

#define lookup_constraint_data(env)   ((struct constraint_data *)core_get_environment_data(env, CONSTRAINT_DATA_INDEX))

LOCALE void init_constraints(void *);

#if METAOBJECT_PROTOCOL
LOCALE int GDCCommand(void *);
LOCALE int SDCCommand(void *d);
LOCALE int GSCCommand(void *);
LOCALE int SSCCommand(void *);
#endif

LOCALE BOOLEAN get_constraints(void *);
LOCALE void    remove_constraint(void *, struct constraint_metadata *);

#endif
