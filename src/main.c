#include <stdio.h>
#include "setup.h"
#include "broccoli.h"

int main(int, char *[]);
void ext_function_hook(void);
void ext_environment_aware_function_hook(void *);

/***************************************
 * main: Starts execution of the expert
 *   system development environment.
 ****************************************/
int main(int argc, char *argv[])
{
    void *env;
    int clMode = TRUE;

    env = core_create_environment();
    clMode = sysdep_route_stdin(env, argc, argv);

    if( clMode == TRUE )
    {
        core_repl(env);
    }
    else
    {
        core_repl_oneshot(env);
    }

    return(-1);
}
