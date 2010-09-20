#ifndef __PARSER_GENERICS_H__
#define __PARSER_GENERICS_H__

#if DEFGENERIC_CONSTRUCT

#include "funcs_generics.h"

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef __PARSER_GENERICS_SOURCE__
#define LOCALE
#else
#define LOCALE extern
#endif

LOCALE BOOLEAN     ParseDefgeneric(void *, char *);
LOCALE BOOLEAN     ParseDefmethod(void *, char *);
LOCALE DEFMETHOD * AddMethod(void *, DEFGENERIC *, DEFMETHOD *, int, short, core_expression_object *, int, int, ATOM_HN *, core_expression_object *, char *, int);
LOCALE void        PackRestrictionTypes(void *, RESTRICTION *, core_expression_object *);
LOCALE void        DeleteTempRestricts(void *, core_expression_object *);
LOCALE DEFMETHOD * FindMethodByRestrictions(DEFGENERIC *, core_expression_object *, int, ATOM_HN *, int *);

#ifndef _GENRCPSR_SOURCE_
#endif

#endif

#endif
