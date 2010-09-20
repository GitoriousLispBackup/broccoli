/* Purpose: Routines for creating and manipulating
 *   list values.                                      */

#ifndef __TYPE_LIST_H__
#define __TYPE_LIST_H__

struct node;
struct list;

#ifndef __CORE_EVALUATION_H__
#include "core_evaluation.h"
#endif

struct node
{
    unsigned short type;
    void *         value;
};

struct list
{
    unsigned     busy_count;
    short        depth;
    long         length;
    struct list *next;
    struct node  cell[1];
};

typedef struct list   LIST_SEGMENT;
typedef struct list * LIST_SEGMENT_PTR;
typedef struct list * LIST_PTR;
typedef struct node   NODE;
typedef struct node * NODE_PTR;

#define get_list_length(target)                   (((struct list *)(target))->length)
#define get_list_ptr(target, index)               (&(((struct node *)((struct list *)(target))->cell)[index - 1]))
#define set_list_node_type(target, index, value)  (((struct node *)((struct list *)(target))->cell)[index - 1].type = (unsigned short)(value))
#define set_list_node_value(target, index, val)   (((struct node *)((struct list *)(target))->cell)[index - 1].value = (void *)(val))
#define get_list_node_type(target, index)         (((struct node *)((struct list *)(target))->cell)[index - 1].type)
#define get_list_node_value(target, index)        (((struct node *)((struct list *)(target))->cell)[index - 1].value)

/*==================
 * ENVIRONMENT DATA
 *==================*/

#define LIST_DATA_INDEX 51

struct list_data
{
    struct list *all_lists;
};

#define get_list_data(env) ((struct list_data *)core_get_environment_data(env, LIST_DATA_INDEX))

#ifdef LOCALE
#undef LOCALE
#endif
#ifdef __TYPE_LIST_SOURCE__
#define LOCALE
#else
#define LOCALE extern
#endif

LOCALE void         init_list_data(void *);
LOCALE void *       create_sized_list(void *, long);
LOCALE void         release_list(void *, struct list *);
LOCALE void         install_list(void *, struct list *);
LOCALE void         uninstall_list(void *, struct list *);
LOCALE struct list* convert_str_to_list(void *, char *);
LOCALE void *       create_list(void *, long);
LOCALE void         track_list(void *, struct list *);
LOCALE void         flush_lists(void *);
LOCALE void         clone_list(void *, struct core_data *, struct core_data *);
LOCALE void print_list(void *, char *, LIST_SEGMENT_PTR, long, long, int);
LOCALE BOOLEAN are_list_segments_equal(core_data_object_ptr, core_data_object_ptr);
LOCALE void          add_to_list(void *, core_data_object *, core_expression_object *, int);
LOCALE void *        copy_list(void *, struct list *);
LOCALE BOOLEAN       are_lists_equal(struct list *, struct list *);
LOCALE void *        convert_data_object_to_list(void *, core_data_object *);
LOCALE unsigned long hash_list(struct list *, unsigned long);
LOCALE struct list * get_all_lists(void *);
LOCALE void *        implode_list(void *, core_data_object *);
LOCALE void          concatenate_lists(void *, core_data_object *, core_expression_object *, int);

#endif
