/* Purpose: Provides routines for parsing module/construct
 *   names and searching through modules for specific
 *   constructs.                                             */

#ifndef __MODULES_QUERY_H__
#define __MODULES_QUERY_H__

#ifndef __TYPE_ATOM_H__
#include "type_symbol.h"
#endif
#ifndef __MODULES_INIT_H__
#include "modules_init.h"
#endif

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef __MODULES_QUERY_SOURCE__
#define LOCALE
#else
#define LOCALE extern
#endif

LOCALE unsigned                        find_module_separator(char *);
LOCALE ATOM_HN                       * garner_module_name(void *, unsigned, char *);
LOCALE ATOM_HN                       * garner_construct_name(void *, unsigned, char *);
LOCALE char                          * garner_module_reference(void *, char *);
LOCALE void                          * lookup_imported_construct(void *, char *, struct module_definition *, char *, int *, int, struct module_definition *);
LOCALE void                            error_ambiguous_reference(void *, char *, char *);
LOCALE void                            set_module_unvisited(void *);
LOCALE BOOLEAN                         is_construct_exported(void *, char *, struct atom_hash_node *, struct atom_hash_node *);

#endif
