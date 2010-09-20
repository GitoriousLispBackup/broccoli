/* Purpose: Isolation of system dependent routines.          */

#ifndef _H_sysdep
#define _H_sysdep

#ifndef __TYPE_ATOM_H__
#include "type_symbol.h"
#endif

#ifndef _STDIO_INCLUDED_
#define _STDIO_INCLUDED_
#include <stdio.h>
#endif

#include <setjmp.h>

#if WIN_BTC || WIN_MVC
#include <dos.h>
#endif

#if WIN_BTC || LINUX
#define LLONG_MAX 0x7fffffffffffffffLL
#define LLONG_MIN (~LLONG_MAX)
#endif

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef _ARCH_SOURCE_
#define LOCALE
#else
#define LOCALE extern
#endif

LOCALE void init_system(void *, struct atom_hash_node **, struct float_hash_node **, struct integer_hash_node **, struct bitmap_hash_node **, struct external_address_hash_node **);
LOCALE int sysdep_route_stdin(void *, int, char *[]);
LOCALE double sysdep_time(void);
LOCALE void   sysdep_system(void *env);
LOCALE int    sysdep_open_r_binary(void *, char *, char *);
LOCALE void   sysdep_seek_r_binary(void *, long);
LOCALE void   sysdep_seek_set_r_binary(void *, long);
LOCALE void   sysdep_tell_r_binary(void *, long *);
LOCALE void   sysdep_close_r_binary(void *);
LOCALE void sysdep_read_r_binary(void *, void *, size_t);
LOCALE FILE * sysdep_open_file(void *, char *, char *);
LOCALE int    sysdep_close_file(void *, FILE *);
LOCALE void   sysdep_exit(void *, int);
LOCALE int    sysdep_rand(void);
LOCALE void   sysdep_seed_rand(int);
LOCALE int    sysdep_remove_file(char *);
LOCALE int    sysdep_rename_file(char *, char *);
LOCALE char * sysdep_pwd(char *, int);
LOCALE void sysdep_write_to_file(void *, size_t, FILE *);
LOCALE int(*sysdep_set_before_open_fn(void *, int(*) (void *))) (void *);
LOCALE int(*sysdep_set_after_open_fn(void *, int(*) (void *))) (void *);
LOCALE int    sysdep_sprintf(char *, const char *, ...);
LOCALE char * sysdep_strcpy(char *, const char *);
LOCALE char *sysdep_strncpy(char *, const char *, size_t);
LOCALE char *sysdep_strcat(char *, const char *);
LOCALE char *sysdep_strncat(char *, const char *, size_t);
LOCALE void sysdep_set_jump_buffer(void *, jmp_buf *);
#if WIN_BTC
LOCALE __int64 _RTLENTRY _EXPFUNC strtoll(const char *, char **, int);
LOCALE __int64 _RTLENTRY _EXPFUNC llabs(__int64 val);
#endif

#endif
