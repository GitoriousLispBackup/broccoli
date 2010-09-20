/* Purpose: Provides a set of routines for processing
 *   commands entered at the top level prompt.               */

#define __CORE_COMMAND_LINE_SOURCE__

#include <stdio.h>
#define _STDIO_INCLUDED_
#include <string.h>
#include <ctype.h>

#include "setup.h"
#include "constant.h"

#include "core_arguments.h"
#include "core_constructs.h"
#include "parser_constructs.h"
#include "core_environment.h"
#include "parser_expressions.h"
#include "funcs_io_basic.h"
#include "core_memory.h"
#include "funcs_flow_control.h"
#include "parser_flow_control.h"
#include "router.h"
#include "core_scanner.h"
#include "router_string.h"
#include "type_symbol.h"
#include "sysdep.h"
#include "core_gc.h"

#include "core_command_prompt.h"

/**************************************
 * LOCAL FUNCTION PROTOTYPES
 ***************************************/

static int     _skip_string(char *, int, int *);
static int     _skip_comment(char *, int);
static int     _skip_ws(char *, int);
static int     _next_event(void *);
static void    _clear_prompt(void *);
static int     _expand(void *, int);
static void    _flush(void *);
static void    _prompt(void *env);
static BOOLEAN _execute(void *env);
static BOOLEAN _execute_silently(void *env);
static char *  _completion(void *env, char *theString, size_t maxPosition);
static void    _load_base(void*);

/***************************************************
 * core_init_command_prompt: Allocates environment
 *    data for command line functionality.
 ****************************************************/
void core_init_command_prompt(void *env)
{
    core_allocate_environment_data(env, COMMAND_PROMPT_DATA_INDEX, sizeof(struct commandLineData), _clear_prompt);

    CommandLineData(env)->BannerString = BANNER_STRING;
    CommandLineData(env)->EventFunction = _next_event;
}

/******************************************************
 * clearCommandPrompt: Deallocates environment
 *    data for the command line functionality.
 ******************************************************/
static void _clear_prompt(void *env)
{
    if( CommandLineData(env)->CommandString != NULL )
    {
        core_mem_release(env, CommandLineData(env)->CommandString, CommandLineData(env)->MaximumCharacters);
    }

    if( CommandLineData(env)->CurrentCommand != NULL )
    {
        core_return_expression(env, CommandLineData(env)->CurrentCommand);
    }
}

/**************************************************
 * expand: Appends a character to the
 *   command string. Returns TRUE if the command
 *   string was successfully expanded, otherwise
 *   FALSE. Expanding the string also includes
 *   adding a backspace character which reduces
 *   string's length.
 ***************************************************/
static int _expand(void *env, int inchar)
{
    size_t k;

    k = get_router_data(env)->command_buffer_input_count;
    CommandLineData(env)->CommandString = core_gc_expand_string(env, inchar, CommandLineData(env)->CommandString, &get_router_data(env)->command_buffer_input_count,
                                                                &CommandLineData(env)->MaximumCharacters, CommandLineData(env)->MaximumCharacters + 80);
    return((get_router_data(env)->command_buffer_input_count != k) ? TRUE : FALSE);
}

/*****************************************************************
 * flush: Empties the contents of the CommandString.
 ******************************************************************/
static void _flush(void *env)
{
    if( CommandLineData(env)->CommandString != NULL )
    {
        core_mem_release(env, CommandLineData(env)->CommandString, CommandLineData(env)->MaximumCharacters);
    }

    CommandLineData(env)->CommandString = NULL;
    CommandLineData(env)->MaximumCharacters = 0;
    get_router_data(env)->command_buffer_input_count = 0;
    get_router_data(env)->is_waiting = TRUE;
}

/*************************************************************************
 * core_complete_command: Determines whether a string forms a complete command.
 *   A complete command is either a constant, a variable, or a function
 *   call which is followed (at some point) by a carriage return. Once a
 *   complete command is found (not including the parenthesis),
 *   extraneous parenthesis and other tokens are ignored. If a complete
 *   command exists, then 1 is returned. 0 is returned if the command was
 *   not complete and without errors. -1 is returned if the command
 *   contains an error.
 **************************************************************************/
int core_complete_command(char *mstring)
{
    int i;
    char inchar;
    int depth = 0;
    int moreThanZero = 0;
    int complete;
    int error = 0;

    if( mstring == NULL )
    {
        return(0);
    }

    /*===================================================
     * Loop through each character of the command string
     * to determine if there is a complete command.
     *===================================================*/

    i = 0;

    while((inchar = mstring[i++]) != EOS )
    {
        switch( inchar )
        {
            /*======================================================
             * If a carriage return or line feed is found, there is
             * at least one completed token in the command buffer,
             * and parentheses are balanced, then a complete
             * command has been found. Otherwise, remove all white
             * space beginning with the current character.
             *======================================================*/

        case '\n':
        case '\r':

            if( error )
            {
                return(-1);
            }

            if( moreThanZero && (depth == 0))
            {
                return(1);
            }

            i = _skip_ws(mstring, i);
            break;

            /*=====================
             * Remove white space.
             *=====================*/

        case ' ':
        case '\f':
        case '\t':
            i = _skip_ws(mstring, i);
            break;

            /*======================================================
             * If the opening quotation of a string is encountered,
             * determine if the closing quotation of the string is
             * in the command buffer. Until the closing quotation
             * is found, a complete command can not be made.
             *======================================================*/

        case '"':
            i = _skip_string(mstring, i, &complete);

            if((depth == 0) && complete )
            {
                moreThanZero = TRUE;
            }

            break;

            /*====================
             * Process a comment.
             *====================*/

        case ';':
            i = _skip_comment(mstring, i);

            if( moreThanZero && (depth == 0) && (mstring[i] != EOS))
            {
                if( error )
                {
                    return(-1);
                }
                else
                {
                    return(1);
                }
            }
            else if( mstring[i] != EOS )
            {
                i++;
            }

            break;

            /*====================================================
             * A left parenthesis increases the nesting depth of
             * the current command by 1. Don't bother to increase
             * the depth if the first token encountered was not
             * a parenthesis (e.g. for the command string
             * "red (+ 3 4", the symbol red already forms a
             * complete command, so the next carriage return will
             * cause evaluation of red--the closing parenthesis
             * for "(+ 3 4" does not have to be found).
             *====================================================*/

        case '(':

            if((depth > 0) || (moreThanZero == FALSE))
            {
                depth++;
                moreThanZero = TRUE;
            }

            break;

            /*====================================================
             * A right parenthesis decreases the nesting depth of
             * the current command by 1. If the parenthesis is
             * the first token of the command, then an error is
             * generated.
             *====================================================*/

        case ')':

            if( depth > 0 )
            {
                depth--;
            }
            else if( moreThanZero == FALSE )
            {
                error = TRUE;
            }

            break;

            /*=====================================================
             * If the command begins with any other character and
             * an opening parenthesis hasn't yet been found, then
             * skip all characters on the same line. If a carriage
             * return or line feed is found, then a complete
             * command exists.
             *=====================================================*/

        default:

            if( depth == 0 )
            {
                if( isprint(inchar))
                {
                    while((inchar = mstring[i++]) != EOS )
                    {
                        if((inchar == '\n') || (inchar == '\r'))
                        {
                            if( error )
                            {
                                return(-1);
                            }
                            else
                            {
                                return(1);
                            }
                        }
                    }

                    return(0);
                }
            }

            break;
        }
    }

    /*====================================================
     * Return 0 because a complete command was not found.
     *====================================================*/

    return(0);
}

/**********************************************************
 * skipString: Skips over a string contained within a string
 *   until the closing quotation mark is encountered.
 ***********************************************************/
static int _skip_string(char *str, int pos, int *complete)
{
    int inchar;

    /*=================================================
     * Process the string character by character until
     * the closing quotation mark is found.
     *=================================================*/

    inchar = str[pos];

    while( inchar  != '"' )
    {
        /*=====================================================
         * If a \ is found, then the next character is ignored
         * even if it is a closing quotation mark.
         *=====================================================*/

        if( inchar == '\\' )
        {
            pos++;
            inchar = str[pos];
        }

        /*===================================================
         * If the end of input is reached before the closing
         * quotation mark is found, the return the last
         * position that was reached and indicate that a
         * complete string was not found.
         *===================================================*/

        if( inchar == EOS )
        {
            *complete = FALSE;
            return(pos);
        }

        /*================================
         * Move on to the next character.
         *================================*/

        pos++;
        inchar = str[pos];
    }

    /*======================================================
     * Indicate that a complete string was found and return
     * the position of the closing quotation mark.
     *======================================================*/

    pos++;
    *complete = TRUE;
    return(pos);
}

/************************************************************
 * skipComment: Skips over a comment contained within a string
 *   until a line feed or carriage return is encountered.
 *************************************************************/
static int _skip_comment(char *str, int pos)
{
    int inchar;

    inchar = str[pos];

    while((inchar != '\n') && (inchar != '\r'))
    {
        if( inchar == EOS )
        {
            return(pos);
        }

        pos++;
        inchar = str[pos];
    }

    return(pos);
}

/*************************************************************
 * skipWhitespace: Skips over white space consisting of spaces,
 *   tabs, and form feeds that is contained within a string.
 **************************************************************/
static int _skip_ws(char *str, int pos)
{
    int inchar;

    inchar = str[pos];

    while((inchar == ' ') || (inchar == '\f') || (inchar == '\t'))
    {
        pos++;
        inchar = str[pos];
    }

    return(pos);
}

static void _load_base(void*env)
{
    broccoli_run_silent(env, "broccoli.brocc");
}

/*******************************************************************
 * core_repl: Endless loop which waits for user commands and then
 *   executes them. The command loop will bypass the EventFunction
 *   if there is an active batch file.
 ********************************************************************/
void core_repl(void *env)
{
    int inchar;

    print_router(env, WPROMPT, CommandLineData(env)->BannerString);
    core_set_halt_eval(env, FALSE);
    core_set_eval_error(env, FALSE);
    core_gc_periodic_cleanup(env, TRUE, FALSE);

    _load_base(env);

    _prompt(env);
    get_router_data(env)->command_buffer_input_count = 0;
    get_router_data(env)->is_waiting = TRUE;

    while( TRUE )
    {
        /*===================================================
         * If a batch file is active, grab the command input
         * directly from the batch file, otherwise call the
         * event function.
         *===================================================*/

        if( io_is_batching(env) == TRUE )
        {
            inchar = io_get_ch_from_batch(env, "stdin", TRUE);

            if( inchar == EOF )
            {
                (*CommandLineData(env)->EventFunction)(env);
            }
            else
            {
                _expand(env, (char)inchar);
            }
        }
        else
        {
            (*CommandLineData(env)->EventFunction)(env);
        }

        /*=================================================
         * If execution was halted, then remove everything
         * from the command buffer.
         *=================================================*/

        if( core_get_halt_eval(env) == TRUE )
        {
            core_set_halt_eval(env, FALSE);
            core_set_eval_error(env, FALSE);
            _flush(env);
            fflush(stdin);
            print_router(env, WPROMPT, "\n");
            _prompt(env);
        }

        /*=========================================
         * If a complete command is in the command
         * buffer, then execute it.
         *=========================================*/

        _execute(env);
    }
}

/*********************************************************
 * execute: Checks to determine if there
 *   is a completed command and if so executes it.
 **********************************************************/
static BOOLEAN _execute(void *env)
{
    if((core_complete_command(CommandLineData(env)->CommandString) == 0) ||
       (get_router_data(env)->command_buffer_input_count == 0) ||
       (get_router_data(env)->is_waiting == FALSE))
    {
        return(FALSE);
    }

    core_flush_pp_buffer(env);
    core_set_pp_buffer_status(env, OFF);
    get_router_data(env)->command_buffer_input_count = 0;
    get_router_data(env)->is_waiting = FALSE;
    core_route_command(env, CommandLineData(env)->CommandString, TRUE);
    core_flush_pp_buffer(env);
    core_set_halt_eval(env, FALSE);
    core_set_eval_error(env, FALSE);
    _flush(env);
    core_gc_periodic_cleanup(env, TRUE, FALSE);
    _prompt(env);

    return(TRUE);
}

/******************************************
 * prompt: Prints the command prompt.
 *******************************************/
static void _prompt(void *env)
{
    print_router(env, WPROMPT, COMMAND_PROMPT);
}

/***********************************************
 * core_route_command: Processes a completed command.
 ************************************************/
BOOLEAN core_route_command(void *env, char *command, int printResult)
{
    core_data_object result;
    struct core_expression *top;
    char *commandName;
    struct token theToken;

    if( command == NULL )
    {
        return(0);
    }

    /*========================================
     * Open a string input source and get the
     * first token from that source.
     *========================================*/

    open_string_source(env, "command", command, 0);

    core_get_token(env, "command", &theToken);

    /*=====================
     * Evaluate constants.
     *=====================*/

    if((theToken.type == ATOM) || (theToken.type == STRING) ||
       (theToken.type == FLOAT) || (theToken.type == INTEGER) ||
       (theToken.type == INSTANCE_NAME))
    {
        close_string_source(env, "command");

        if( printResult )
        {
            core_print_atom(env, "stdout", theToken.type, theToken.value);
            print_router(env, "stdout", "\n");
        }

        return(1);
    }

    /*=====================
     * Evaluate variables.
     *=====================*/

    if((theToken.type == SCALAR_VARIABLE) || (theToken.type == LIST_VARIABLE))
    {
        close_string_source(env, "command");
        top = core_generate_constant(env, theToken.type, theToken.value);
        core_eval_expression(env, top, &result);
        core_mem_return_struct(env, core_expression, top);

        if( printResult )
        {
            core_print_data(env, "stdout", &result);
            print_router(env, "stdout", "\n");
        }

        return(1);
    }

    /*========================================================
     * If the next token isn't the beginning left parenthesis
     * of a command or construct, then whatever was entered
     * cannot be evaluated at the command prompt.
     *========================================================*/

    if( theToken.type != LPAREN )
    {
        error_print_id(env, "PROMPT", 1, FALSE);
        print_router(env, WERROR, "Expected a '(', constant, or variable\n");
        close_string_source(env, "command");
        return(0);
    }

    /*===========================================================
     * The next token must be a function name or construct type.
     *===========================================================*/

    core_get_token(env, "command", &theToken);

    if( theToken.type != ATOM )
    {
        error_print_id(env, "PROMPT", 2, FALSE);
        print_router(env, WERROR, "Expected a command.\n");
        close_string_source(env, "command");
        return(0);
    }

    commandName = to_string(theToken.value);

    /*======================
     * Evaluate constructs.
     *======================*/

    {
        int errorFlag;

        errorFlag = parse_construct(env, commandName, "command");

        if( errorFlag != -1 )
        {
            close_string_source(env, "command");

            if( errorFlag == 1 )
            {
                print_router(env, WERROR, "\nERROR:\n");
                core_print_chunkify(env, WERROR, core_get_pp_buffer_handle(env));
                print_router(env, WERROR, "\n");
            }

            core_delete_pp_buffer(env);
            return(errorFlag);
        }
    }

    /*========================
     * Parse a function call.
     *========================*/

    CommandLineData(env)->ParsingTopLevelCommand = TRUE;
    top = parse_function_body(env, "command", commandName);
    CommandLineData(env)->ParsingTopLevelCommand = FALSE;
    clear_parsed_bindings(env);

    /*================================
     * Close the string input source.
     *================================*/

    close_string_source(env, "command");

    /*=========================
     * Evaluate function call.
     *=========================*/

    if( top == NULL )
    {
        return(0);
    }

    core_increment_expression(env, top);

    CommandLineData(env)->EvaluatingTopLevelCommand = TRUE;
    CommandLineData(env)->CurrentCommand = top;
    core_eval_expression(env, top, &result);
    CommandLineData(env)->CurrentCommand = NULL;
    CommandLineData(env)->EvaluatingTopLevelCommand = FALSE;

    core_decrement_expression(env, top);
    core_return_expression(env, top);

    if((result.type != RVOID) && printResult )
    {
        core_print_data(env, "stdout", &result);
        print_router(env, "stdout", "\n");
    }

    return(1);
}

/****************************************************************
 * nextEvent: Default event-handling function. Handles
 *   only keyboard events by first calling GetcRouter to get a
 *   character and then calling expand to add the
 *   character to the CommandString.
 *****************************************************************/
static int _next_event(void *env)
{
    int inchar;

    inchar = get_ch_router(env, "stdin");

    if( inchar == EOF )
    {
        inchar = '\n';
    }

    _expand(env, (char)inchar);

    return(0);
}

/************************************
 * core_set_event_listener: Replaces the
 *   current value of EventFunction.
 *************************************/
int(*core_set_event_listener(void *env, int (*func)(void *))) (void *)
{
    int (*tmp_ptr)(void *);

    tmp_ptr = CommandLineData(env)->EventFunction;
    CommandLineData(env)->EventFunction = func;
    return(tmp_ptr);
}

/**********************************************************
 * completion: Returns the last token in a
 *   string if it is a valid token for command completion.
 ***********************************************************/
static char *_completion(void *env, char *theString, size_t maxPosition)
{
    struct token lastToken;
    struct token theToken;
    char lastChar;
    char *rs;
    size_t length;

    /*=========================
     * Get the command string.
     *=========================*/

    if( theString == NULL )
    {
        return("");
    }

    /*=========================================================================
     * If the last character in the command string is a space, character
     * return, or quotation mark, then the command completion can be anything.
     *=========================================================================*/

    lastChar = theString[maxPosition - 1];

    if((lastChar == ' ') || (lastChar == '"') ||
       (lastChar == '\t') || (lastChar == '\f') ||
       (lastChar == '\n') || (lastChar == '\r'))
    {
        return("");
    }

    /*============================================
     * Find the last token in the command string.
     *============================================*/

    open_text_source(env, "CommandCompletion", theString, 0, maxPosition);
    core_get_scanner_data(env)->ignoring_completion_error = TRUE;
    core_get_token(env, "CommandCompletion", &theToken);
    core_copy_token(&lastToken, &theToken);

    while( theToken.type != STOP )
    {
        core_copy_token(&lastToken, &theToken);
        core_get_token(env, "CommandCompletion", &theToken);
    }

    close_string_source(env, "CommandCompletion");
    core_get_scanner_data(env)->ignoring_completion_error = FALSE;

    /*===============================================
     * Determine if the last token can be completed.
     *===============================================*/

    if( lastToken.type == ATOM )
    {
        rs = to_string(lastToken.value);

        if( rs[0] == '[' )
        {
            return(&rs[1]);
        }

        return(to_string(lastToken.value));
    }
    else if( lastToken.type == SCALAR_VARIABLE )
    {
        return(to_string(lastToken.value));
    }
    else if( lastToken.type == LIST_VARIABLE )
    {
        return(to_string(lastToken.value));
    }
    else if((lastToken.type == INSTANCE_NAME))
    {
        return(NULL);
    }
    else if( lastToken.type == STRING )
    {
        length = strlen(to_string(lastToken.value));
        return(_completion(env, to_string(lastToken.value), length));
    }
    else if((lastToken.type == FLOAT) || (lastToken.type == INTEGER))
    {
        return(NULL);
    }

    return("");
}

static BOOLEAN _execute_silently(void *env)
{
    if((core_complete_command(CommandLineData(env)->CommandString) == 0) ||
       (get_router_data(env)->command_buffer_input_count == 0) ||
       (get_router_data(env)->is_waiting == FALSE))
    {
        return(FALSE);
    }

    core_set_pp_buffer_status(env, OFF);
    get_router_data(env)->command_buffer_input_count = 0;
    get_router_data(env)->is_waiting = FALSE;
    core_route_command(env, CommandLineData(env)->CommandString, TRUE);
    core_set_halt_eval(env, FALSE);
    core_set_eval_error(env, FALSE);
    _flush(env);
    core_gc_periodic_cleanup(env, TRUE, FALSE);

    return(TRUE);
}

/***********************************************************
 * CommandLoopOnceThenBatch: Loop which waits for commands
 *   from a batch file and then executes them. Returns when
 *   there are no longer any active batch files.
 ************************************************************/
void core_repl_oneshot(void *env)
{
    int inchar;

    _load_base(env);

    while( TRUE )
    {
        if( CommandLineData(env)->HaltREPLBatch == TRUE )
        {
            io_close_all_batches(env);
            CommandLineData(env)->HaltREPLBatch = FALSE;
        }

        /*===================================================
         * If a batch file is active, grab the command input
         * directly from the batch file, otherwise call the
         * event function.
         *===================================================*/

        if( io_is_batching(env) == TRUE )
        {
            inchar = io_get_ch_from_batch(env, "stdin", TRUE);

            if( inchar == EOF )
            {
                return;
            }
            else
            {
                _expand(env, (char)inchar);
            }
        }
        else
        {
            return;
        }

        /*=================================================
         * If execution was halted, then remove everything
         * from the command buffer.
         *=================================================*/

        if( core_get_halt_eval(env) == TRUE )
        {
            core_set_halt_eval(env, FALSE);
            core_set_eval_error(env, FALSE);
            _flush(env);
            fflush(stdin);
        }

        /*=========================================
         * If a complete command is in the command
         * buffer, then execute it.
         *=========================================*/

        _execute_silently(env);
    }
}
