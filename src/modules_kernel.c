/* Purpose: Implements core commands for the module_definition
 *   construct such as clear, reset, save, ppdefmodule
 *   list-defmodules, and get-module_definition-list.                */

#define __MODULES_KERNEL_SOURCE__

#include "setup.h"

#include <stdio.h>
#define _STDIO_INCLUDED_
#include <string.h>

#include "core_constructs.h"
#include "core_functions.h"
#include "core_utilities.h"
#include "router.h"
#include "core_arguments.h"
#include "core_environment.h"

#include "modules_kernel.h"

/**************************************
 * LOCAL FUNCTION PROTOTYPES
 ***************************************/

static void _clear_modules(void *);
#if DEFMODULE_CONSTRUCT
static void SaveDefmodules(void *, void *, char *);
#endif

/****************************************************************
 * init_module_functions: Initializes basic module_definition commands.
 *****************************************************************/
void init_module_functions(void *env)
{
    core_add_clear_fn(env, "defmodule", _clear_modules, 2000);

#if DEFMODULE_CONSTRUCT
    core_add_saver(env, "defmodule", SaveDefmodules, 1100);

    core_define_function(env, "get-defmodule-list", 'm', PTR_FN EnvGetDefmoduleList, "EnvGetDefmoduleList", "00");

#if DEBUGGING_FUNCTIONS
    core_define_function(env, "list-defmodules", 'v', PTR_FN ListDefmodulesCommand, "ListDefmodulesCommand", "00");
    core_define_function(env, "ppdefmodule", 'v', PTR_FN PPDefmoduleCommand, "PPDefmoduleCommand", "11w");
#endif
#endif
}

/********************************************************
 * ClearDefmodules: Defmodule clear routine for use with
 *   the clear command. Creates the MAIN module.
 *********************************************************/
static void _clear_modules(void *env)
{
    remove_all_modules(env);

    create_main_module(env);
    get_module_data(env)->redefinable = TRUE;
}

#if DEFMODULE_CONSTRUCT

/*****************************************
 * SaveDefmodules: Defmodule save routine
 *   for use with the save command.
 ******************************************/
static void SaveDefmodules(void *env, void *module_def, char *logical_name)
{
    char *ppform;

    ppform = get_module_pp(env, module_def);

    if( ppform != NULL )
    {
        core_print_chunkify(env, logical_name, ppform);
        print_router(env, logical_name, "\n");
    }
}

/************************************************
 * EnvGetDefmoduleList: H/L and C access routine
 *   for the get-module_definition-list function.
 *************************************************/
void EnvGetDefmoduleList(void *env, core_data_object_ptr ret)
{
    core_garner_construct_list(env, ret, get_next_module, get_module_name);
}

#if DEBUGGING_FUNCTIONS

/*******************************************
 * PPDefmoduleCommand: H/L access routine
 *   for the ppdefmodule command.
 ********************************************/
void PPDefmoduleCommand(void *env)
{
    char *defmoduleName;

    defmoduleName = core_get_atom(env, "ppdefmodule", "defmodule name");

    if( defmoduleName == NULL )
    {
        return;
    }

    PPDefmodule(env, defmoduleName, WDISPLAY);

    return;
}

/************************************
 * PPDefmodule: C access routine for
 *   the ppdefmodule command.
 *************************************/
int PPDefmodule(void *env, char *defmoduleName, char *logical_name)
{
    void *defmodulePtr;

    defmodulePtr = lookup_module(env, defmoduleName);

    if( defmodulePtr == NULL )
    {
        error_lookup(env, "defmodule", defmoduleName);
        return(FALSE);
    }

    if( get_module_pp(env, defmodulePtr) == NULL )
    {
        return(TRUE);
    }

    core_print_chunkify(env, logical_name, get_module_pp(env, defmodulePtr));
    return(TRUE);
}

/**********************************************
 * ListDefmodulesCommand: H/L access routine
 *   for the list-defmodules command.
 ***********************************************/
void ListDefmodulesCommand(void *env)
{
    if( core_check_arg_count(env, "list-defmodules", EXACTLY, 0) == -1 )
    {
        return;
    }

    EnvListDefmodules(env, WDISPLAY);
}

/**************************************
 * EnvListDefmodules: C access routine
 *   for the list-defmodules command.
 ***************************************/
void EnvListDefmodules(void *env, char *logical_name)
{
    void *module_def;
    int count = 0;

    for( module_def = get_next_module(env, NULL);
         module_def != NULL;
         module_def = get_next_module(env, module_def))
    {
        print_router(env, logical_name, get_module_name(env, module_def));
        print_router(env, logical_name, "\n");
        count++;
    }

    core_print_tally(env, logical_name, count, "defmodule", "defmodules");
}

#endif /* DEBUGGING_FUNCTIONS */

#endif /* DEFMODULE_CONSTRUCT */
