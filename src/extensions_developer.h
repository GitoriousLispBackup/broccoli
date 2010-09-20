#ifndef __EXTENSIONS_DEVELOPER_H__
#define __EXTENSIONS_DEVELOPER_H__

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef __EXTENSIONS_DEVELOPER_SOURCE__
#define LOCALE
#else
#define LOCALE extern
#endif

#if DEVELOPER
LOCALE void DeveloperCommands(void *);
LOCALE void PrimitiveTablesInfo(void *);
LOCALE void PrimitiveTablesUsage(void *);
LOCALE void EnableGCHeuristics(void *);
LOCALE void DisableGCHeuristics(void *);

#if OBJECT_SYSTEM
LOCALE void InstanceTableUsage(void *);
#endif
#endif
#endif
