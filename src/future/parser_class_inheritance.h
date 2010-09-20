#ifndef __PARSER_CLASS_INHERITANCE_H__
#define __PARSER_CLASS_INHERITANCE_H__

#if OBJECT_SYSTEM

#ifndef __CLASSES_H__
#include "classes.h"
#endif

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef __PARSER_CLASS_INHERITANCE_SOURCE__
#define LOCALE
#else
#define LOCALE extern
#endif

LOCALE PACKED_CLASS_LINKS * ParseSuperclasses(void *, char *, ATOM_HN *);
LOCALE PACKED_CLASS_LINKS * FindPrecedenceList(void *, DEFCLASS *, PACKED_CLASS_LINKS *);
LOCALE void                 PackClassLinks(void *, PACKED_CLASS_LINKS *, CLASS_LINK *);

#ifndef _INHERPSR_SOURCE_
#endif

#endif

#endif
