#ifndef __PARSER_CLASS_INSTANCES_H__
#define __PARSER_CLASS_INSTANCES_H__

#ifndef __EXPRESSIONS_H__
#include "core_expressions.h"
#endif

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef __PARSER_CLASS_INSTANCES_SOURCE__
#define LOCALE
#else
#define LOCALE extern
#endif

LOCALE core_expression_object * ParseInitializeInstance(void *, core_expression_object *, char *);
LOCALE core_expression_object * ParseSlotOverrides(void *, char *, int *);

LOCALE core_expression_object *ParseSimpleInstance(void *, core_expression_object *, char *);

#ifndef _INSCOM_SOURCE_
#endif

#endif
