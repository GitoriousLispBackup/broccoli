/* Purpose: Parsing routines and utilities for parsing
 *   constructs.                                             */

#ifndef __CONSTRUCTS_PARSER_H__
#define __CONSTRUCTS_PARSER_H__

#ifndef __CORE_EVALUATION_H__
#include "core_evaluation.h"
#endif
#ifndef __CORE_SCANNER_H__
#include "core_scanner.h"
#endif
#ifndef __CORE_CONSTRUCTS_H__
#include "core_constructs.h"
#endif

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef __CONSTRUCTS_PARSER_SOURCE__
#define LOCALE
#else
#define LOCALE extern
#endif

LOCALE int  load_parse(void *, char *);
LOCALE int  parse_construct(void *, char *, char *);
LOCALE void undefine_module_construct(void *, struct construct_metadata *);
LOCALE ATOM_HN         *parse_construct_head(void *, char *,
                                             struct token *, char *,
                                             void *(*)(void *, char *),
                                             int(*) (void *, void *),
                                             char *, int, int, int);
LOCALE void error_import_export_conflict(void *, char *, char *, char *, char *);

#endif
