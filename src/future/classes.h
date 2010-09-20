#ifndef __CLASSES_H__
#define __CLASSES_H__

typedef struct defclassModule   DEFCLASS_MODULE;
typedef struct defclass         DEFCLASS;
typedef struct packedClassLinks PACKED_CLASS_LINKS;
typedef struct classLink        CLASS_LINK;
typedef struct slotName         SLOT_NAME;
typedef struct slotDescriptor   SLOT_DESC;
typedef struct messageHandler   HANDLER;
typedef struct instance         INSTANCE_TYPE;
typedef struct instanceSlot     INSTANCE_SLOT;

/* Maximum # of simultaneous class hierarchy traversals
 * should be a multiple of BITS_PER_BYTE and less than MAX_INT      */

#define MAX_TRAVERSALS  256
#define TRAVERSAL_BYTES 32       /* (MAX_TRAVERSALS / BITS_PER_BYTE) */

#define VALUE_REQUIRED     0
#define VALUE_PROHIBITED   1
#define VALUE_NOT_REQUIRED 2

#ifndef __CORE_CONSTRUCTS_H__
#include "core_constructs.h"
#endif
#ifndef __CONSTRAINTS_KERNEL_H__
#include "constraints_kernel.h"
#endif
#ifndef __MODULES_INIT_H__
#include "modules_init.h"
#endif
#ifndef __CORE_EVALUATION_H__
#include "core_evaluation.h"
#endif
#ifndef __EXPRESSIONS_H__
#include "core_expressions.h"
#endif
#ifndef __TYPE_LIST_H__
#include "type_list.h"
#endif
#ifndef __TYPE_ATOM_H__
#include "type_symbol.h"
#endif

#define GetInstanceSlotLength(sp) get_list_length(sp->value)

struct packedClassLinks
{
    long       classCount;
    DEFCLASS **classArray;
};

struct defclassModule
{
    struct module_header header;
};

struct defclass
{
    struct construct_metadata header;
    unsigned               installed      :
    1;
    unsigned system         :
    1;
    unsigned abstract       :
    1;
    unsigned reactive       :
    1;
    unsigned traceInstances :
    1;
    unsigned traceSlots     :
    1;
    unsigned id;
    unsigned busy,
             hashTableIndex;
    PACKED_CLASS_LINKS directSuperclasses,
                       directSubclasses,
                       allSuperclasses;
    SLOT_DESC *slots,
    **instanceTemplate;
    unsigned *     slotNameMap;
    short          slotCount;
    short          localInstanceSlotCount;
    short          instanceSlotCount;
    short          maxSlotNameID;
    INSTANCE_TYPE *instanceList,
    *instanceListBottom;
    HANDLER *  handlers;
    unsigned * handlerOrderMap;
    short      handlerCount;
    DEFCLASS * nxtHash;
    BITMAP_HN *scopeMap;
    char       traversalRecord[TRAVERSAL_BYTES];
};

struct classLink
{
    DEFCLASS *        cls;
    struct classLink *nxt;
};

struct slotName
{
    unsigned hashTableIndex,
             use;
    short    id;
    ATOM_HN *name,
    *putHandlerName;
    struct slotName *nxt;
    long             bsaveIndex;
};

struct instanceSlot
{
    SLOT_DESC *desc;
    unsigned   valueRequired :
    1;
    unsigned override      :
    1;
    unsigned short type;
    void *         value;
};

struct slotDescriptor
{
    unsigned shared                   :
    1;
    unsigned multiple                 :
    1;
    unsigned composite                :
    1;
    unsigned noInherit                :
    1;
    unsigned noWrite                  :
    1;
    unsigned initializeOnly           :
    1;
    unsigned dynamicDefault           :
    1;
    unsigned defaultSpecified         :
    1;
    unsigned noDefault                :
    1;
    unsigned reactive                 :
    1;
    unsigned publicVisibility         :
    1;
    unsigned createReadAccessor       :
    1;
    unsigned createWriteAccessor      :
    1;
    unsigned overrideMessageSpecified :
    1;
    DEFCLASS *         cls;
    SLOT_NAME *        slotName;
    ATOM_HN *          overrideMessage;
    void *             defaultValue;
    CONSTRAINT_META *constraint;
    unsigned           sharedCount;
    long               bsaveIndex;
    INSTANCE_SLOT      sharedValue;
};

struct instance
{
    struct patternEntity header;
    void *               partialMatchList;
    INSTANCE_SLOT *      basisSlots;
    unsigned             installed            :
    1;
    unsigned garbage              :
    1;
    unsigned initSlotsCalled      :
    1;
    unsigned initializeInProgress :
    1;
    unsigned reteSynchronized     :
    1;
    ATOM_HN *      name;
    int            depth;
    unsigned       hashTableIndex;
    unsigned       busy;
    DEFCLASS *     cls;
    INSTANCE_TYPE *prvClass, *nxtClass,
    *prvHash, *nxtHash,
    *prvList, *nxtList;
    INSTANCE_SLOT **slotAddresses,
    *slots;
};

struct messageHandler
{
    unsigned system         :
    1;
    unsigned type           :
    2;
    unsigned mark           :
    1;
    unsigned trace          :
    1;
    unsigned         busy;
    ATOM_HN *        name;
    DEFCLASS *       cls;
    short            minParams;
    short            maxParams;
    short            localVarCount;
    core_expression_object *     actions;
    char *           ppForm;
    struct ext_data *usrData;
};

#endif
