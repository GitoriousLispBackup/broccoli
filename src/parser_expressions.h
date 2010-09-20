/* Purpose: Provides routines for parsing expressions.       */

#ifndef __PARSER_EXPRESSIONS_H__
#define __PARSER_EXPRESSIONS_H__


typedef struct saved_contexts
{
    int rtn;
    int brk;
    struct saved_contexts *nxt;
} SAVED_CONTEXTS;


#ifndef __CORE_FUNCTIONS_H__
#include "core_functions.h"
#endif
#ifndef __CORE_SCANNER_H__
#include "core_scanner.h"
#endif

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef __PARSER_EXPRESSIONS_SOURCE__
#define LOCALE
#else
#define LOCALE extern
#endif

LOCALE struct core_expression                 * parse_virgin_function(void *, char *);
LOCALE struct core_expression                 * parse_function_body(void *, char *, char *);
LOCALE void                                     push_break_contexts(void *);
LOCALE void                                     pop_break_contexts(void *);
LOCALE struct core_expression                 * collect_args(void *, struct core_expression *, char *);
LOCALE struct core_expression                 * parse_args(void *, char *, int *);
LOCALE struct core_expression                 * parse_atom_or_expression(void *, char *, struct token *);
LOCALE core_expression_object                 * parse_constants(void *, char *, int *);
LOCALE struct core_expression                 * group_actions(void *, char *, struct token *, int, char *, int);
LOCALE struct core_expression                 * strip_frivolous_progn(void *, struct core_expression *);

#define ERROR_TAG_EXPRESSION_PARSER         "Parse Error "
#define ERROR_MSG_INVALID_FUNC_NAME         "Function names must be symbols"
#define ERROR_MSG_MISSIN_FUNC_DECL          "Function not defined "
#define ERROR_MSG_LIST_INVALID_ARG          "Invalid argument for "
#define ERROR_MSG_INVALID_ARG               "Invalid argument: expected one of literal, variable, or expression"
#define ERROR_MSG_EXT_CALL_ARGS             "Unable to read arguments"
#define ERROR_MSG_EXT_CALL_CONST            "Invalid argument: expected one of symbol, literal, or instance name"

#endif
