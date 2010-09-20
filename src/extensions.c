#include "broccoli.h"

void ext_function_hook(void);
void ext_environment_aware_function_hook(void *);

/********************************************************
 * ext_function_hook: Informs the expert system environment
 *   of any user defined functions. In the default case,
 *   there are no user defined functions. To define
 *   functions, either this function must be replaced by
 *   a function with the same name within this file, or
 *   this function can be deleted from this file and
 *   included in another file.
 *********************************************************/
void ext_function_hook()
{
}

/**********************************************************
 * ext_environment_aware_function_hook: Informs the expert system environment
 *   of any user defined functions. In the default case,
 *   there are no user defined functions. To define
 *   functions, either this function must be replaced by
 *   a function with the same name within this file, or
 *   this function can be deleted from this file and
 *   included in another file.
 ***********************************************************/
#if WIN_BTC
#pragma argsused
#endif
void ext_environment_aware_function_hook(void *env)
{
#if MAC_MCW || WIN_MCW || MAC_XCD
#pragma unused(env)
#endif
}
