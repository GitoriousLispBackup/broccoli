/* Purpose: Provides functions for parsing constraint
 *   declarations.                                           */

#ifndef __PARSER_CONSTRAINTS_H__
#define __PARSER_CONSTRAINTS_H__

#ifndef __CONSTRAINTS_KERNEL_H__
#include "constraints_kernel.h"
#endif

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef _PARSER_SOURCE_
#define LOCALE
#else
#define LOCALE extern
#endif
#if OFF
struct constraintParseRecord
{
    unsigned int type :
    1;
    unsigned int range :
    1;
    unsigned int allowedSymbols :
    1;
    unsigned int allowedStrings :
    1;
    unsigned int allowedLexemes :
    1;
    unsigned int allowedFloats :
    1;
    unsigned int allowedIntegers :
    1;
    unsigned int allowedNumbers :
    1;
    unsigned int allowedValues :
    1;
    unsigned int allowedClasses :
    1;
    unsigned int allowedInstanceNames :
    1;
    unsigned int cardinality :
    1;
};

typedef struct constraintParseRecord CONSTRAINT_PARSE_RECORD;

LOCALE BOOLEAN CheckConstraintParseConflicts(void *, CONSTRAINT_META *);
LOCALE void    AttributeConflictErrorMessage(void *, char *, char *);
LOCALE void    InitializeConstraintParseRecord(CONSTRAINT_PARSE_RECORD *);
LOCALE BOOLEAN StandardConstraint(char *);
LOCALE BOOLEAN ParseStandardConstraint(void *, char *, char *, CONSTRAINT_META *, CONSTRAINT_PARSE_RECORD *, int);
LOCALE void    OverlayConstraint(void *, CONSTRAINT_PARSE_RECORD *, CONSTRAINT_META *, CONSTRAINT_META *);
LOCALE void    OverlayConstraintParseRecord(CONSTRAINT_PARSE_RECORD *, CONSTRAINT_PARSE_RECORD *);

#endif
#endif
