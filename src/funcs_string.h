#ifndef __FUNCS_STRING_H__
#define __FUNCS_STRING_H__

#ifndef __CORE_EVALUATION_H__
#include "core_evaluation.h"
#endif

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef __FUNCS_STRING_SOURCE__
#define LOCALE
#else
#define LOCALE extern
#endif
#if STRING_FUNCTIONS
LOCALE int  EnvBuild(void *, char *);
LOCALE void StringFunctionDefinitions(void *);
LOCALE void StrCatFunction(void *, core_data_object_ptr);
LOCALE void SymCatFunction(void *, core_data_object_ptr);
LOCALE long long StrLengthFunction(void *);
LOCALE void UpcaseFunction(void *, core_data_object_ptr);
LOCALE void LowcaseFunction(void *, core_data_object_ptr);
LOCALE long long                       StrCompareFunction(void *);
LOCALE void                          * SubStringFunction(void *);
LOCALE void StrIndexFunction(void *, core_data_object_ptr);
LOCALE int  BuildFunction(void *);
LOCALE void StringToFieldFunction(void *, core_data_object *);
LOCALE void StringToField(void *, char *, core_data_object *);

#endif
#endif
