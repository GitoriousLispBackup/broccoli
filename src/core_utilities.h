/* Purpose: Utility routines for printing various items
 *   and messages.                                           */

#ifndef __CORE_UTILITIES_H__
#define __CORE_UTILITIES_H__

#ifndef __MODULES_INIT_H__
#include "modules_init.h"
#endif

#ifndef _STDIO_INCLUDED_
#define _STDIO_INCLUDED_
#include <stdio.h>
#endif

#define PRINT_UTILITY_DATA_INDEX 53

struct core_utility_data
{
    BOOLEAN preserving_escape;
    BOOLEAN stringifying_pointer;
    BOOLEAN converting_inst_address_to_name;
};

#define core_get_utility_data(env) ((struct core_utility_data *)core_get_environment_data(env, PRINT_UTILITY_DATA_INDEX))

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef __CORE_UTILITIES_SOURCE__
#define LOCALE
#else
#define LOCALE extern
#endif
LOCALE void                            core_init_utility_data(void *);
LOCALE void                            core_print_chunkify(void *, char *, char *);
LOCALE void                            core_print_float(void *, char *, double);
LOCALE void                            core_print_long(void *, char *, long long);
LOCALE void                            core_print_hex(void *, char *, int);
LOCALE void                            core_print_atom(void *, char *, int, void *);
LOCALE void                            core_print_tally(void *, char *, long long, char *, char *);
LOCALE char                          * core_convert_float_to_string(void *, double);
LOCALE char                          * core_conver_long_to_string(void *, long long);
LOCALE char                          * core_convert_data_object_to_string(void *, core_data_object *);
LOCALE void                            error_syntax(void *, char *);
LOCALE void                            error_system(void *, char *, int);
LOCALE void                            error_print_id(void *, char *, int, int);
LOCALE void                            warning_print_id(void *, char *, int, int);
LOCALE void                            error_lookup(void *, char *, char *);
LOCALE void                            error_deletion(void *, char *, char *);
LOCALE void                            error_reparse(void *, char *, char *);
LOCALE void                            error_local_variable(void *, char *);
LOCALE void                            error_divide_by_zero(void *, char *);
LOCALE void                            error_method_duplication(void *, char *, char *);
LOCALE int                             core_numerical_compare(void *, int, void *, int, void *);

#endif
