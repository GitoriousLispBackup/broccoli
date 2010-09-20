/* Purpose: I/O Router routines which allow files to be used
 *   as input and output sources.                            */

#ifndef __ROUTER_FILE_H__
#define __ROUTER_FILE_H__

#ifndef _STDIO_INCLUDED_
#define _STDIO_INCLUDED_
#include <stdio.h>
#endif

#define FILE_ROUTER_DATA_INDEX 47

struct file_router
{
    char *              logical_name;
    FILE *              stream;
    struct file_router *next;
};

struct file_router_data
{
    struct file_router *file_routers_list;
};

#define get_file_router_data(env) ((struct file_router_data *)core_get_environment_data(env, FILE_ROUTER_DATA_INDEX))

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef __ROUTER_FILE_SOURCE__
#define LOCALE
#else
#define LOCALE extern
#endif

LOCALE void init_file_router(void *);
LOCALE int  open_file(void *, char *, char *, char *);
LOCALE int  close_all_files(void *);
LOCALE int  close_file(void *, char *);
LOCALE int  find_file(void *, char *);

#endif
