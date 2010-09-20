#ifndef __PARSER_GENERICS_IMPLICIT_H__
#define __PARSER_GENERICS_IMPLICIT_H__

#if DEFGENERIC_CONSTRUCT

#include "funcs_generics.h"

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef __PARSER_GENERICS_IMPLICIT_SOURCE__
#define LOCALE
#else
#define LOCALE extern
#endif

LOCALE void AddImplicitMethods(void *, DEFGENERIC *);

#ifndef _IMMTHPSR_SOURCE_
#endif

#endif

#endif
