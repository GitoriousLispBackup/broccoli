/* Purpose: Provides a set of routines for processing
 *   commands entered at the top level prompt.               */

#ifndef __CORE_COMMAND_PROMPT_H__

#define __CORE_COMMAND_PROMPT_H__

#define COMMAND_PROMPT_DATA_INDEX 40

struct commandLineData
{
    int                     EvaluatingTopLevelCommand;
    int                     HaltREPLBatch;
    struct core_expression *CurrentCommand;
    char *                  CommandString;
    size_t                  MaximumCharacters;
    int                     ParsingTopLevelCommand;
    char *                  BannerString;
    int                     (*EventFunction)(void *);
};

#define CommandLineData(env) ((struct commandLineData *)core_get_environment_data(env, COMMAND_PROMPT_DATA_INDEX))

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef __CORE_COMMAND_LINE_SOURCE__
#define LOCALE
#else
#define LOCALE extern
#endif

LOCALE void    core_init_command_prompt(void *);
LOCALE int     core_complete_command(char *);
LOCALE void    core_repl(void *);
LOCALE BOOLEAN core_route_command(void *, char *, int);
LOCALE int(*core_set_event_listener(void *, int(*) (void *))) (void *);
LOCALE void core_repl_oneshot(void *env);

#endif
