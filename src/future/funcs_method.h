/* Purpose: Message-passing support functions                */

#ifndef __FUNCS_METHOD_H__
#define __FUNCS_METHOD_H__

typedef struct handlerSlotReference
{
    long classID;
    long slotID;
} HANDLER_SLOT_REFERENCE;

#ifndef __CLASSES_H__
#include "classes.h"
#endif
#include "msgpass.h"

#define BEGIN_TRACE ">>"
#define END_TRACE   "<<"

/* =================================================================================
 *  Message-handler types - don't change these values: a string array depends on them
 *  ================================================================================= */
#define MAROUND        0
#define MBEFORE        1
#define MPRIMARY       2
#define MAFTER         3
#define MERROR         4

#define LOOKUP_HANDLER_INDEX   0
#define LOOKUP_HANDLER_ADDRESS 1

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef __FUNCS_METHOD_SOURCE__
#define LOCALE
#else
#define LOCALE extern
#endif

LOCALE void UnboundHandlerErr(void *);
LOCALE void PrintNoHandlerError(void *, char *);
LOCALE int  CheckHandlerArgCount(void *);
LOCALE void SlotAccessViolationError(void *, char *, BOOLEAN, void *);
LOCALE void SlotVisibilityViolationError(void *, SLOT_DESC *, DEFCLASS *);

LOCALE void              NewSystemHandler(void *, char *, char *, char *, int);
LOCALE HANDLER         * InsertHandlerHeader(void *, DEFCLASS *, ATOM_HN *, int);

LOCALE HANDLER         * NewHandler(void);
LOCALE int               HandlersExecuting(DEFCLASS *);
LOCALE int               DeleteHandler(void *, DEFCLASS *, ATOM_HN *, int, int);
LOCALE void              DeallocateMarkedHandlers(void *, DEFCLASS *);
LOCALE unsigned          HandlerType(void *, char *, char *);
LOCALE int               CheckCurrentMessage(void *, char *, int);
LOCALE void              PrintHandler(void *, char *, HANDLER *, int);
LOCALE HANDLER         * FindHandlerByAddress(DEFCLASS *, ATOM_HN *, unsigned);
LOCALE int               FindHandlerByIndex(DEFCLASS *, ATOM_HN *, unsigned);
LOCALE int               FindHandlerNameGroup(DEFCLASS *, ATOM_HN *);
LOCALE void              HandlerDeleteError(void *, char *);

#if DEBUGGING_FUNCTIONS
LOCALE void              DisplayCore(void *, char *, HANDLER_LINK *, int);
LOCALE HANDLER_LINK    * FindPreviewApplicableHandlers(void *, DEFCLASS *, ATOM_HN *);
LOCALE void              WatchMessage(void *, char *, char *);
LOCALE void              WatchHandler(void *, char *, HANDLER_LINK *, char *);
#endif

#endif
