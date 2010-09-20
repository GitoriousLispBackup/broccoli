#ifndef __CLASSES_INSTANCES_FILE_H__
#define __CLASSES_INSTANCES_FILE_H__

#ifndef __EXPRESSIONS_H__
#include "core_expressions.h"
#endif

#define INSTANCE_FILE_DATA 30


#ifdef LOCALE
#undef LOCALE
#endif

#ifdef __CLASSES_INSTANCES_FILE_SOURCE__
#define LOCALE
#else
#define LOCALE extern
#endif

#define BinaryLoadInstances(a)           EnvBinaryLoadInstances(GetCurrentEnvironment(), a)
#define BinarySaveInstances(a, b, c, d)  EnvBinarySaveInstances(GetCurrentEnvironment(), a, b, c, d)
#define LoadInstances(a)                 EnvLoadInstances(GetCurrentEnvironment(), a)
#define LoadInstancesFromString(a, b)    EnvLoadInstancesFromString(GetCurrentEnvironment(), a, b)
#define RestoreInstances(a)              EnvRestoreInstances(GetCurrentEnvironment(), a)
#define RestoreInstancesFromString(a, b) EnvRestoreInstancesFromString(GetCurrentEnvironment(), a, b)
#define SaveInstances(a, b, c, d)        coreSaveInstances(GetCurrentEnvironment(), a, b, c, d)

LOCALE void SetupInstanceFileCommands(void *);

LOCALE long SaveInstancesCommand(void *);
LOCALE long LoadInstancesCommand(void *);
LOCALE long RestoreInstancesCommand(void *);
LOCALE long coreSaveInstances(void *, char *, int, core_expression_object *, BOOLEAN);



LOCALE long EnvLoadInstances(void *, char *);
LOCALE long EnvLoadInstancesFromString(void *, char *, int);
LOCALE long EnvRestoreInstances(void *, char *);
LOCALE long EnvRestoreInstancesFromString(void *, char *, int);

#ifndef _INSFILE_SOURCE_
#endif

#endif
