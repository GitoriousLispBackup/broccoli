#ifndef __CORE_CONSTRUCTS_QUERY_H__

#define __CORE_CONSTRUCTS_QUERY_H__

#ifndef __MODULES_INIT_H__
#include "modules_init.h"
#endif
#ifndef __CORE_CONSTRUCTS_H__
#include "core_constructs.h"
#endif

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef __CORE_CONSTRUCTS_QUERY_SOURCE__
#define LOCALE
#else
#define LOCALE extern
#endif

LOCALE void                            core_add_to_module(struct construct_metadata *);
LOCALE void                          * core_lookup_named_construct(void *, char *, struct construct *);
LOCALE void                            core_uninstall_construct_command(void *, char *, struct construct *);
LOCALE ATOM_HN                       * core_query_module_name(void *, char *, struct construct *);
LOCALE void                            core_save_construct(void *, void *, char *, struct construct *);
LOCALE char                          * core_get_atom_string(struct construct_metadata *);
LOCALE char                          * core_garner_module(struct construct_metadata *);
LOCALE ATOM_HN                       * core_get_atom_pointer(struct construct_metadata *);
LOCALE void core_garner_construct_list(void *, char *, core_data_object_ptr, struct construct *);
LOCALE void core_get_module_constructs_list(void *, core_data_object_ptr, struct construct *, struct module_definition *);
LOCALE void                             core_garner_module_constructs_command(void *, char *, struct construct *);
LOCALE void                             core_get_module_constructs(void *, struct construct *, char *, struct module_definition *);
LOCALE void                             core_set_next_construct(struct construct_metadata *, struct construct_metadata *);
LOCALE struct module_header          *  core_query_module_header(struct construct_metadata *);
LOCALE char                          *  core_get_construct_pp(void *, struct construct_metadata *);
LOCALE void                             core_construct_pp_command(void *, char *, struct construct *);
LOCALE struct construct_metadata      * core_get_next_construct_metadata(void *, struct construct_metadata *, int);
LOCALE void                             core_free_construct_header_module(void *, struct module_header *, struct construct *);
LOCALE long core_do_for_all_constructs(void *, void(*) (void *, struct construct_metadata *, void *), int, int, void *);
LOCALE void core_do_for_all_constructs_in_module(void *, void *, void(*) (void *, struct construct_metadata *, void *), int, int, void *);
LOCALE void core_init_construct_header(void *, char *, struct construct_metadata *, ATOM_HN *);
LOCALE void core_set_construct_pp(void *, struct construct_metadata *, char *);
LOCALE void                          *core_lookup_construct(void *, struct construct *, char *, BOOLEAN);
LOCALE BOOLEAN core_are_constructs_deleteable(void *);

#endif
