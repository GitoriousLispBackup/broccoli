/* Purpose: Provides a centralized mechanism for handling
 *   input and output requests.                              */

#ifndef _H_router
#define _H_router

#ifndef __CORE_UTILITIES_H__
#include "core_utilities.h"
#endif

#ifndef _STDIO_INCLUDED_
#define _STDIO_INCLUDED_
#include <stdio.h>
#endif

#define WWARNING "wwarning"
#define WERROR   "werror"
#define WTRACE   "wtrace"
#define WDIALOG  "wdialog"
#define WPROMPT  WPROMPT_STRING
#define WDISPLAY "wdisplay"

#define ROUTER_DATA_INDEX 46

struct router
{
    char *         name;
    int            active;
    int            priority;
    short int      environment_aware;
    void *         context;
    int            (*query)(void *, char *);
    int            (*printer)(void *, char *, char *);
    int            (*exiter)(void *, int);
    int            (*charget)(void *, char *);
    int            (*charunget)(void *, int, char *);
    struct router *next;
};

struct router_data
{
    size_t         command_buffer_input_count;
    int            is_waiting;
    char *         line_count_router;
    char *         fast_get_ch;
    char *         fast_get_str;
    long           fast_get_inde;
    struct router *routers_list;
    FILE *         fast_load_file_ptr;
    FILE *         fast_save_file_ptr;
    int            abort;
};

#define get_router_data(env) ((struct router_data *)core_get_environment_data(env, ROUTER_DATA_INDEX))

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef _ROUTER_SOURCE_
#define LOCALE
#else
#define LOCALE extern
#endif

LOCALE void init_routers(void *);
LOCALE int  print_router(void *, char *, char *);
LOCALE int  get_ch_router(void *, char *);
LOCALE int  unget_ch_router(void *, int, char *);
LOCALE void exit_router(void *, int);
LOCALE void abort_exit(void *);

LOCALE BOOLEAN add_router(void *,
                          char *, int,
                          int(*) (void *, char *),
                          int(*) (void *, char *, char *),
                          int(*) (void *, char *),
                          int(*) (void *, int, char *),
                          int(*) (void *, int));

LOCALE int                             delete_router(void *, char *);
LOCALE int                             query_router(void *, char *);
LOCALE int                             deactivate_router(void *, char *);
LOCALE int                             activate_router(void *, char *);
LOCALE void                            set_fast_load(void *, FILE *);
LOCALE void                            set_fast_save(void *, FILE *);
LOCALE FILE                          * get_fast_load(void *);
LOCALE FILE                          * get_fast_save(void *);
LOCALE void                            error_unknown_router(void *, char *);
LOCALE void                            broccoli_quit(void *);

/***
 * Function names and constraints
 **/
#define FUNC_NAME_QUIT              "quit"
#define FUNC_CNSTR_QUIT             "*1i"

#endif
