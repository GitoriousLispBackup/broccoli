#ifndef __CLASSES_META_H__
#define __CLASSES_META_H__

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef __CLASSES_META_SOURCE__
#define LOCALE
#else
#define LOCALE extern
#endif

#define BrowseClasses(a, b)       EnvBrowseClasses(GetCurrentEnvironment(), a, b)
#define DescribeClass(a, b)       EnvDescribeClass(GetCurrentEnvironment(), a, b)
#define SlotDirectAccessP(a, b)   EnvSlotDirectAccessP(GetCurrentEnvironment(), a, b)
#define SlotExistP(a, b, c)       EnvSlotExistP(GetCurrentEnvironment(), a, b, c)
#define SlotInitableP(a, b)       EnvSlotInitableP(GetCurrentEnvironment(), a, b)
#define SlotPublicP(a, b)         EnvSlotPublicP(GetCurrentEnvironment(), a, b)
#define SlotWritableP(a, b)       EnvSlotWritableP(GetCurrentEnvironment(), a, b)
#define SubclassP(a, b)           EnvSubclassP(GetCurrentEnvironment(), a, b)
#define SuperclassP(a, b)         EnvSuperclassP(GetCurrentEnvironment(), a, b)
#define SlotDefaultValue(a, b, c) EnvSlotDefaultValue(GetCurrentEnvironment(), a, b, c)

#if DEBUGGING_FUNCTIONS
LOCALE void BrowseClassesCommand(void *);
LOCALE void EnvBrowseClasses(void *, char *, void *);
LOCALE void DescribeClassCommand(void *);
LOCALE void EnvDescribeClass(void *, char *, void *);
#endif

LOCALE char *GetCreateAccessorString(void *);

LOCALE void *  GetDefclassModuleCommand(void *);
LOCALE BOOLEAN SuperclassPCommand(void *);
LOCALE BOOLEAN EnvSuperclassP(void *, void *, void *);
LOCALE BOOLEAN SubclassPCommand(void *);
LOCALE BOOLEAN EnvSubclassP(void *, void *, void *);
LOCALE int     SlotExistPCommand(void *);
LOCALE BOOLEAN EnvSlotExistP(void *, void *, char *, BOOLEAN);
LOCALE int     MessageHandlerExistPCommand(void *);
LOCALE BOOLEAN SlotWritablePCommand(void *);
LOCALE BOOLEAN EnvSlotWritableP(void *, void *, char *);
LOCALE BOOLEAN SlotInitablePCommand(void *);
LOCALE BOOLEAN EnvSlotInitableP(void *, void *, char *);
LOCALE BOOLEAN SlotPublicPCommand(void *);
LOCALE BOOLEAN EnvSlotPublicP(void *, void *, char *);
LOCALE BOOLEAN SlotDirectAccessPCommand(void *);
LOCALE BOOLEAN EnvSlotDirectAccessP(void *, void *, char *);
LOCALE void SlotDefaultValueCommand(void *, core_data_object_ptr);
LOCALE BOOLEAN EnvSlotDefaultValue(void *, void *, char *, core_data_object_ptr);
LOCALE int ClassExistPCommand(void *);

#ifndef _CLASSEXM_SOURCE_
#endif

#endif
