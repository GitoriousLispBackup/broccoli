#ifndef __PARSER_CLASS_METHODS_H__
#define __PARSER_CLASS_METHODS_H__

#if OBJECT_SYSTEM

#define SELF_STRING     "self"

#ifndef __CLASSES_H__
#include "classes.h"
#endif

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef __PARSER_CLASS_METHODS_SOURCE__
#define LOCALE
#else
#define LOCALE extern
#endif

LOCALE int  ParseDefmessageHandler(void *, char *);
LOCALE void CreateGetAndPutHandlers(void *, SLOT_DESC *);

#endif

#endif
