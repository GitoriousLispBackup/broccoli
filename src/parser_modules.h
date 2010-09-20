#ifndef __PARSER_MODULES_H__
#define __PARSER_MODULES_H__

struct portConstructItem
{
    char *                    constructName;
    int                       typeExpected;
    struct portConstructItem *next;
};

#ifndef __TYPE_ATOM_H__
#include "type_symbol.h"
#endif
#ifndef __CORE_EVALUATION_H__
#include "core_evaluation.h"
#endif
#ifndef __MODULES_INIT_H__
#include "modules_init.h"
#endif

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef __PARSER_MODULES_SOURCE__
#define LOCALE
#else
#define LOCALE extern
#endif

#if DEFMODULE_CONSTRUCT
LOCALE long GetNumberOfDefmodules(void *);
LOCALE void SetNumberOfDefmodules(void *, long);
LOCALE void AddAfterModuleDefinedFunction(void *, char *, void(*) (void *), int);
LOCALE int                             ParseDefmodule(void *, char *);
LOCALE void                            AddPortConstructItem(void *, char *, int);
LOCALE struct portConstructItem      * ValidPortConstructItem(void *, char *);
LOCALE int                             FindImportExportConflict(void *, char *, struct module_definition *, char *);

#endif
#endif
