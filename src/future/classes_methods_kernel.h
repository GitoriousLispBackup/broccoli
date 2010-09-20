#ifndef __CLASSES_METHODS_KERNEL_H__
#define __CLASSES_METHODS_KERNEL_H__

#ifndef __CLASSES_H__
#include "classes.h"
#endif

#ifndef __CLASSES_METHODS_CALL_H__
#include "msgpass.h"
#endif

#define MESSAGE_HANDLER_DATA 32

struct messageHandlerData
{
    core_data_entity_object HandlerGetInfo;
    core_data_entity_object HandlerPutInfo;
    ATOM_HN *     INIT_ATOM;
    ATOM_HN *     DELETE_ATOM;
    ATOM_HN *     CREATE_ATOM;
#if DEBUGGING_FUNCTIONS
    unsigned WatchHandlers;
    unsigned WatchMessages;
#endif
    char *        hndquals[4];
    ATOM_HN *     SELF_ATOM;
    ATOM_HN *     CurrentMessageName;
    HANDLER_LINK *CurrentCore;
    HANDLER_LINK *TopOfCore;
    HANDLER_LINK *NextInCore;
    HANDLER_LINK *OldCore;
};

#define MessageHandlerData(theEnv) ((struct messageHandlerData *)core_get_environment_data(theEnv, MESSAGE_HANDLER_DATA))


#ifdef LOCALE
#undef LOCALE
#endif

#ifdef __CLASSES_METHODS_KERNEL_SOURCE__
#define LOCALE
#else
#define LOCALE extern
#endif

#define INIT_STRING   "init"
#define DELETE_STRING "delete"
#define PRINT_STRING  "print"
#define CREATE_STRING "create"

#define FindDefmessageHandler(a, b, c)     EnvFindDefmessageHandler(GetCurrentEnvironment(), a, b, c)
#define GetDefmessageHandlerName(a, b)     EnvGetDefmessageHandlerName(GetCurrentEnvironment(), a, b)
#define GetDefmessageHandlerPPForm(a, b)   EnvGetDefmessageHandlerPPForm(GetCurrentEnvironment(), a, b)
#define GetDefmessageHandlerType(a, b)     EnvGetDefmessageHandlerType(GetCurrentEnvironment(), a, b)
#define GetDefmessageHandlerWatch(a, b)    EnvGetDefmessageHandlerWatch(GetCurrentEnvironment(), a, b)
#define GetNextDefmessageHandler(a, b)     EnvGetNextDefmessageHandler(GetCurrentEnvironment(), a, b)
#define IsDefmessageHandlerDeletable(a, b) EnvIsDefmessageHandlerDeletable(GetCurrentEnvironment(), a, b)
#define ListDefmessageHandlers(a, b, c)    EnvListDefmessageHandlers(GetCurrentEnvironment(), a, b, c)
#define PreviewSend(a, b, c)               EnvPreviewSend(GetCurrentEnvironment(), a, b, c)
#define SetDefmessageHandlerWatch(a, b, c) EnvSetDefmessageHandlerWatch(GetCurrentEnvironment(), a, b, c)
#define UndefmessageHandler(a, b)          EnvUndefmessageHandler(GetCurrentEnvironment(), a, b)

LOCALE void              SetupMessageHandlers(void *);
LOCALE char            * EnvGetDefmessageHandlerName(void *, void *, int);
LOCALE char            * EnvGetDefmessageHandlerType(void *, void *, int);
LOCALE int               EnvGetNextDefmessageHandler(void *, void *, int);
LOCALE HANDLER         * GetDefmessageHandlerPointer(void *, int);
#if DEBUGGING_FUNCTIONS
LOCALE unsigned EnvGetDefmessageHandlerWatch(void *, void *, int);
LOCALE void     EnvSetDefmessageHandlerWatch(void *, int, void *, int);
#endif
LOCALE unsigned EnvFindDefmessageHandler(void *, void *, char *, char *);
LOCALE int      EnvIsDefmessageHandlerDeletable(void *, void *, int);
LOCALE void     UndefmessageHandlerCommand(void *);
LOCALE int      EnvUndefmessageHandler(void *, void *, int);

#if DEBUGGING_FUNCTIONS
LOCALE void              PPDefmessageHandlerCommand(void *);
LOCALE void              ListDefmessageHandlersCommand(void *);
LOCALE void              PreviewSendCommand(void *);
LOCALE char            * EnvGetDefmessageHandlerPPForm(void *, void *, int);
LOCALE void              EnvListDefmessageHandlers(void *, char *, void *, int);
LOCALE void              EnvPreviewSend(void *, char *, void *, char *);
LOCALE long              DisplayHandlersInLinks(void *, char *, PACKED_CLASS_LINKS *, int);
#endif

#endif
