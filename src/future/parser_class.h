#ifndef __PARSER_CLASS_H__
#define __PARSER_CLASS_H__

#if OBJECT_SYSTEM

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef _CLASSPSR_SOURCE_
#define LOCALE
#else
#define LOCALE extern
#endif

LOCALE int ParseDefclass(void *, char *);

#endif

#endif
