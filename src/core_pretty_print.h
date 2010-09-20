/* Purpose: Routines for processing the pretty print
 *   representation of constructs.                           */

#ifndef __CORE_PRETTY_PRINT_H__
#define __CORE_PRETTY_PRINT_H__

#define PRETTY_PRINT_DATA_INDE 52

struct core_pp_buffer
{
    int    status;
    int    is_enabled;
    int    space_depth;
    size_t position;
    size_t max;
    size_t lookback1;
    size_t lookback2;
    char * data;
};

#define core_pp_get_buffer(env) ((struct core_pp_buffer *)core_get_environment_data(env, PRETTY_PRINT_DATA_INDE))

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef __CORE_PRETTY_PRINT_SOURCE__
#define LOCALE
#else
#define LOCALE extern
#endif

LOCALE void                            core_init_pp_buffer(void *);
LOCALE void                            core_flush_pp_buffer(void *);
LOCALE void                            core_delete_pp_buffer(void *);
LOCALE void                            core_save_pp_buffer(void *, char *);
LOCALE void                            core_backup_pp_buffer(void *);
LOCALE char                          * core_copy_pp_buffer(void *);
LOCALE char                          * core_get_pp_buffer_handle(void *);
LOCALE void                            core_inject_ws_into_pp_buffer(void *);
LOCALE void                            core_increment_pp_buffer_depth(void *, int);
LOCALE void                            core_decrement_pp_buffer_depth(void *, int);
LOCALE void                            core_pp_indent_depth(void *, int);
LOCALE void                            core_set_pp_buffer_status(void *, int);
LOCALE int                             core_get_pp_buffer_status(void *);
LOCALE int                             core_set_pp_buffer_enabled(void *, int);
LOCALE int                             core_get_pp_buffer_enabled(void *);

#endif
