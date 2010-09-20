/* Purpose: Message-passing support functions                */

#ifndef __CLASSES_METHODS_CALL_H__
#define __CLASSES_METHODS_CALL_H__

#define GetActiveInstance(theEnv) ((INSTANCE_TYPE *)GetNthMessageArgument(theEnv, 0)->value)

#ifndef __CLASSES_H__
#include "classes.h"
#endif

typedef struct messageHandlerLink
{
    HANDLER *hnd;
    struct messageHandlerLink *nxt;
    struct messageHandlerLink *nxtInStack;
} HANDLER_LINK;

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef __CLASSES_METHODS_CALL_SOURCE__
#define LOCALE
#else
#define LOCALE extern
#endif

#define Send(a, b, c, d) EnvSend(GetCurrentEnvironment(), a, b, c, d)

LOCALE void              DirectMessage(void *, ATOM_HN *, INSTANCE_TYPE *, core_data_object *, core_expression_object *);
LOCALE void              EnvSend(void *, core_data_object *, char *, char *, core_data_object *);
LOCALE void              DestroyHandlerLinks(void *, HANDLER_LINK *);
LOCALE void              SendCommand(void *, core_data_object *);
LOCALE core_data_object     * GetNthMessageArgument(void *, int);

LOCALE int  NextHandlerAvailable(void *);
LOCALE void CallNextHandler(void *, core_data_object *);

LOCALE void FindApplicableOfName(void *, DEFCLASS *, HANDLER_LINK *[],
                                 HANDLER_LINK *[], ATOM_HN *);
LOCALE HANDLER_LINK    *JoinHandlerLinks(void *, HANDLER_LINK *[], HANDLER_LINK *[], ATOM_HN *);

LOCALE void    PrintHandlerSlotGetFunction(void *, char *, void *);
LOCALE BOOLEAN HandlerSlotGetFunction(void *, void *, core_data_object *);
LOCALE void    PrintHandlerSlotPutFunction(void *, char *, void *);
LOCALE BOOLEAN HandlerSlotPutFunction(void *, void *, core_data_object *);
LOCALE void    DynamicHandlerGetSlot(void *, core_data_object *);
LOCALE void    DynamicHandlerPutSlot(void *, core_data_object *);

#endif
