/* Purpose: Implements core commands for the deffacts
 *   construct such as clear, reset, save, undeffacts,
 *   ppdeffacts, list-deffacts, and get-deffacts-list.       */

#ifndef __MODULES_KERNEL_H__
#define __MODULES_KERNEL_H__

#ifndef __CORE_EVALUATION_H__
#include "core_evaluation.h"
#endif

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef __MODULES_KERNEL_SOURCE__
#define LOCALE
#else
#define LOCALE extern
#endif

LOCALE void init_module_functions(void *);

#endif
