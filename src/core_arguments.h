/* Purpose: Provides access routines for accessing arguments
 *   passed to user or system functions defined using the
 *   DefineFunction protocol.                                */

#ifndef __ARGUMENTS_H__
#define __ARGUMENTS_H__

#ifndef __EXPRESSIONS_H__
#include "core_expressions.h"
#endif
#ifndef __CORE_EVALUATION_H__
#include "core_evaluation.h"
#endif
#ifndef __MODULES_INIT_H__
#include "modules_init.h"
#endif

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef __CORE_ARGUMENTS_SOURCE__
#define LOCALE
#else
#define LOCALE extern
#endif

LOCALE int                             core_get_arg_count(void *);
LOCALE int                             core_check_arg_count(void *, char *, int, int);
LOCALE int                             core_check_arg_range(void *, char *, int, int);
LOCALE struct core_data             *  core_get_arg_at(void *, int, struct core_data *);
LOCALE int                             core_check_arg_type(void *, char *, int, int, struct core_data *);
LOCALE BOOLEAN                         core_get_numeric_arg(void *, struct core_expression *, char *, struct core_data *, int, int);
LOCALE char                          * core_lookup_logical_name(void *, int, char *);
LOCALE char                          * core_get_filename(void *, char *, int);
LOCALE char                          * core_get_atom(void *, char *, char *);
LOCALE void                            report_arg_count_error(void *, char *, int, int);
LOCALE void                            report_file_open_error(void *, char *, char *);
LOCALE BOOLEAN                         core_check_function_arg_count(void *, char *, char *, int);
LOCALE void                            report_return_type_error(char *, char *);
LOCALE void                            report_explicit_type_error(void *, char *, int, char *);
LOCALE void                            report_implicit_type_error(void *, char *, int);
LOCALE void                            report_logical_name_lookup_error(void *, char *);

#if DEFMODULE_CONSTRUCT
LOCALE struct module_definition              *core_get_module_name(void *, char *, int, int *);
#endif

#define ERROR_TAG_ARGUMENTS          "Args Error"
#define ERROR_MSG_ARG_RANGE          "Invalid number of arguments for "
#define ERROR_MSG_ARG_TYPE           " received wrong type for arg #"
#define ERROR_MSG_ARG_TYPE_FROM      " received wrong type from "
#define ERROR_MSG_FOR_ARG            " for arg #"
#define ERROR_MSG_FILE_OPEN          " unable to open file "
#define ERROR_MSG_NO_EXISTS          " received a non-existent argument from "
#define ERROR_MSG_EXACTLY            " requires exactly "
#define ERROR_MSG_AT_LEAST           " requires at least "
#define ERROR_MSG_NO_MORE            " requires no more than "
#define ERROR_MSG_GENERAL            ": Illegal argument check "

#endif
