#ifndef __CLASSES_INSTANCES_INIT_H__
#define __CLASSES_INSTANCES_INIT_H__

#ifndef __CLASSES_H__
#include "classes.h"
#endif

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef __CLASSES_INSTANCES_INIT_SOURCE__
#define LOCALE
#else
#define LOCALE extern
#endif

LOCALE void      InitializeInstanceCommand(void *, core_data_object *);
LOCALE void      MakeInstanceCommand(void *, core_data_object *);
LOCALE ATOM_HN * GetFullInstanceName(void *, INSTANCE_TYPE *);
LOCALE INSTANCE_TYPE *BuildInstance(void *, ATOM_HN *, DEFCLASS *, BOOLEAN);
LOCALE void    InitSlotsCommand(void *, core_data_object *);
LOCALE BOOLEAN QuashInstance(void *, INSTANCE_TYPE *);

#if OBJECT_SYSTEM
LOCALE void InactiveInitializeInstance(void *, core_data_object *);
LOCALE void InactiveMakeInstance(void *, core_data_object *);
#endif

#endif
