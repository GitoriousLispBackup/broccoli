/* Purpose: Provides functions for parsing the default
 *   attribute and determining default values based on
 *   slot constraints.                                       */

#ifndef __CLASSES_MEMBERS_H__
#define __CLASSES_MEMBERS_H__

#ifndef __CONSTRAINTS_KERNEL_H__
#include "constraints_kernel.h"
#endif
#ifndef __CORE_EVALUATION_H__
#include "core_evaluation.h"
#endif

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef __CLASSES_MEMBERS_SOURCE__
#define LOCALE
#else
#define LOCALE extern
#endif

LOCALE void                            DeriveDefaultFromConstraints(void *, CONSTRAINT_META *, core_data_object *, int, int);
LOCALE struct core_expression                   * ParseDefault(void *, char *, int, int, int, int *, int *, int *);

#endif
