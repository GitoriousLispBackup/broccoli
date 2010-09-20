/* Purpose: I/O Router routines which allow strings to be
 *   used as input and output sources.                       */

#ifndef __ROUTER_STRING_H__
#define __ROUTER_STRING_H__

#ifndef _STDIO_INCLUDED_
#define _STDIO_INCLUDED_
#include <stdio.h>
#endif

#define STRING_ROUTER_DATA_INDEX 48

struct string_router
{
    char *                name;
    char *                str;
    size_t                position;
    size_t                max_position;
    int                   rw_type;
    struct string_router *next;
};

struct string_router_data
{
    struct string_router *string_router_list;
};

#define get_string_router_data(env) ((struct string_router_data *)core_get_environment_data(env, STRING_ROUTER_DATA_INDEX))

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef __ROUTER_STRING_SOURCE__
#define LOCALE
#else
#define LOCALE extern
#endif

/*************************
 * I/O ROUTER DEFINITIONS
 **************************/

LOCALE void init_string_routers(void *);
LOCALE int open_string_source(void *, char *, char *, size_t);
LOCALE int open_text_source(void *, char *, char *, size_t, size_t);
LOCALE int close_string_source(void *, char *);
LOCALE int open_string_dest(void *, char *, char *, size_t);
LOCALE int close_string_dest(void *, char *);

#endif
