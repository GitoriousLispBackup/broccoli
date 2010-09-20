#ifndef __PARSER_CLASS_QUERY_H__
#define __PARSER_CLASS_QUERY_H__

#if INSTANCE_SET_QUERIES

#ifndef __EXPRESSIONS_H__
#include "core_expressions.h"
#endif

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef __PARSER_CLASS_QUERY_SOURCE__
#define LOCALE
#else
#define LOCALE extern
#endif

LOCALE core_expression_object * ParseQueryNoAction(void *, core_expression_object *, char *);
LOCALE core_expression_object * ParseQueryAction(void *, core_expression_object *, char *);

#ifndef _INSQYPSR_SOURCE_
#endif

#endif

#endif
