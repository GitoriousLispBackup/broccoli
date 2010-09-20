#ifndef __CLASSES_INSTANCES_MODIFY_H__
#define __CLASSES_INSTANCES_MODIFY_H__

#define DIRECT_MODIFY_STRING    "direct-modify"
#define MSG_MODIFY_STRING       "message-modify"
#define DIRECT_DUPLICATE_STRING "direct-duplicate"
#define MSG_DUPLICATE_STRING    "message-duplicate"

#ifndef __CORE_EVALUATION_H__
#include "core_evaluation.h"
#endif

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef __CLASSES_INSTANCES_SOURCE_H__
#define LOCALE
#else
#define LOCALE extern
#endif

LOCALE void SetupInstanceModDupCommands(void *);

LOCALE void ModifyInstance(void *, core_data_object *);
LOCALE void MsgModifyInstance(void *, core_data_object *);
LOCALE void DuplicateInstance(void *, core_data_object *);
LOCALE void MsgDuplicateInstance(void *, core_data_object *);
LOCALE void DirectModifyMsgHandler(void *, core_data_object *);
LOCALE void MsgModifyMsgHandler(void *, core_data_object *);
LOCALE void DirectDuplicateMsgHandler(void *, core_data_object *);
LOCALE void MsgDuplicateMsgHandler(void *, core_data_object *);

#ifndef _INSMODDP_SOURCE_
#endif

#endif
