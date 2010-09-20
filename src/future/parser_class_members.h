#ifndef __PARSER_CLASS_MEMBERS_H__
#define __PARSER_CLASS_MEMBERS_H__

#if OBJECT_SYSTEM

#define MATCH_RLN            "pattern-match"
#define REACTIVE_RLN         "reactive"
#define NONREACTIVE_RLN      "non-reactive"

#ifndef __CLASSES_H__
#include "classes.h"
#endif

typedef struct tempSlotLink
{
    SLOT_DESC *desc;
    struct tempSlotLink *nxt;
} TEMP_SLOT_LINK;

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef _CLSLTPSR_SOURCE_
#define LOCALE
#else
#define LOCALE extern
#endif

LOCALE TEMP_SLOT_LINK * ParseSlot(void *, char *, TEMP_SLOT_LINK *, PACKED_CLASS_LINKS *, int, int);
LOCALE void             DeleteSlots(void *, TEMP_SLOT_LINK *);

#ifndef _CLSLTPSR_SOURCE_
#endif

#endif

#endif
