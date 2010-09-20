/* Purpose: Routines for scanning lexical tokens from an
 *   input source.                                           */

#ifndef __CORE_SCANNER_H__
#define __CORE_SCANNER_H__

struct token;

#ifndef __CORE_PRETTY_PRINT_H__
#include "core_pretty_print.h"
#endif

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef __CORE_SCANNER_SOURCE__
#define LOCALE
#else
#define LOCALE extern
#endif

struct token
{
    unsigned short type;
    void *         value;
    char *         pp;
};

#define SCANNER_DATA_INDEX 57

struct core_scanner_data
{
    char * global_str;
    size_t global_max;
    size_t global_position;
    long   line_count;
    int    ignoring_completion_error;
};

#define core_get_scanner_data(env) ((struct core_scanner_data *)core_get_environment_data(env, SCANNER_DATA_INDEX))

LOCALE void core_init_scanner_data(void *);
LOCALE void core_get_token(void *, char *, struct token *);
LOCALE void core_copy_token(struct token *, struct token *);
LOCALE void core_reset_line_count(void *);
LOCALE long core_get_line_count(void *);
LOCALE void core_inc_line_count(void *);
LOCALE void core_dec_line_count(void *);

#endif
