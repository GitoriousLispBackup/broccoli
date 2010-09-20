#ifndef __CLASSES_INSTANCES_LISTS_H__
#define __CLASSES_INSTANCES_LISTS_H__

#ifndef __CORE_EVALUATION_H__
#include "core_evaluation.h"
#endif

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef __CLASSES_INSTANCES_LISTS_SOURCE__
#define LOCALE
#else
#define LOCALE extern
#endif

LOCALE void SetupInstanceListCommands(void *);

LOCALE void    MVSlotReplaceCommand(void *, core_data_object *);
LOCALE void    MVSlotInsertCommand(void *, core_data_object *);
LOCALE void    MVSlotDeleteCommand(void *, core_data_object *);
LOCALE BOOLEAN DirectMVReplaceCommand(void *);
LOCALE BOOLEAN DirectMVInsertCommand(void *);
LOCALE BOOLEAN DirectMVDeleteCommand(void *);

#ifndef _INSMULT_SOURCE_
#endif

#endif
