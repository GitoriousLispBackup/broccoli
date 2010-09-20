#ifndef __CLASSES_INIT_H__
#define __CLASSES_INIT_H__

#ifndef __CORE_CONSTRUCTS_H__
#include "core_constructs.h"
#endif
#ifndef __CLASSES_H__
#include "classes.h"
#endif

#if OBJECT_SYSTEM

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef __CLASSES_INIT_SOURCE__
#define LOCALE
#else
#define LOCALE extern
#endif

LOCALE void SetupObjectSystem(void *);
LOCALE void CreateSystemClasses(void *);

#endif

#endif
