#ifndef __PARSER_FLOW_CONTROL_H__
#define __PARSER_FLOW_CONTROL_H__

#ifndef __CONSTRAINTS_KERNEL_H__
#include "constraints_kernel.h"
#endif

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef __PARSER_FLOW_CONTROL_SOURCE__
#define LOCALE
#else
#define LOCALE extern
#endif

struct binding
{
    struct atom_hash_node *name;
    CONSTRAINT_META *      constraints;
    struct binding *       next;
};

LOCALE void                           init_flow_control_parsers(void *);
LOCALE struct binding               * get_parsed_bindings(void *);
LOCALE void                           set_parsed_bindings(void *, struct binding *);
LOCALE void                           clear_parsed_bindings(void *);
LOCALE BOOLEAN                        are_bindings_empty(void *);
LOCALE int                            find_parsed_binding(void *, struct atom_hash_node *);
LOCALE int                            get_binding_count_in_current_context(void *);
LOCALE void                           remove_binding(void *, struct atom_hash_node *);

#endif
