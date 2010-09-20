/* Purpose: Contains the code for file commands including
 *   batch, dribble-on, dribble-off, save, load, bsave, and
 *   bload.
 * Purpose: Contains the code for several I/O functions
 *   including printout, read, open, close, remove, rename,
 *   format, and readline.                                   */

#define __FUNCS_IO_BASIC_SOURCE__

#include <stdio.h>
#include <locale.h>

#define _STDIO_INCLUDED_
#include <string.h>

#include "setup.h"

#include "core_arguments.h"
#include "core_constructs.h"
#include "core_command_prompt.h"
#include "parser_constructs.h"
#include "core_environment.h"
#include "core_functions.h"
#include "core_memory.h"
#include "funcs_flow_control.h"
#include "router.h"
#include "router_string.h"
#include "sysdep.h"
#include "core_gc.h"
#include "router_file.h"
#include "core_scanner.h"
#include "constant.h"

#include "funcs_io_basic.h"

/**************
 * STRUCTURES
 ***************/

struct batch_item
{
    int                type;
    void *             source;
    char *             representation;
    struct batch_item *next;
};

/**************
 * DEFINITIONS
 ***************/

#define FILE_SOURCE      0
#define STRING_SOURCE    1

#define BUFFER_SZ   120

#define IO_DATA_INDEX 14

struct io_data
{
    int                BatchType;
    void *             BatchSource;
    char *             BatchBuffer;
    size_t             BatchCurrentPosition;
    size_t             BatchMaximumPosition;
    struct batch_item *TopOfBatchList;
    struct batch_item *BottomOfBatchList;
};

#define get_io_data(env) ((struct io_data *)core_get_environment_data(env, IO_DATA_INDEX))

/**************************************
 * LOCAL FUNCTION PROTOTYPES
 ***************************************/

static int  _find_batch(void *, char *);
static int  _get_ch_batch(void *, char *);
static int  _unget_ch_batch(void *, int, char *);
static int  _quit_batch(void *, int);
static void _add_batch(void *, int, void *, int, char *);
static void _delete_io_data(void *);

/**************************************
 * init_io_functions: Initializes
 *   file commands.
 ***************************************/
void init_io_functions(void *env)
{
    core_allocate_environment_data(env, IO_DATA_INDEX, sizeof(struct io_data), _delete_io_data);

    core_define_function(env, "run", RT_BOOL, PTR_FN broccoli_run, "BatchCommand", "11k");

#if OFF
    core_define_function(env, "load", 'b', PTR_FN LoadCommand, "LoadCommand", "11k");
#endif

    core_define_function(env, "import", 'b', PTR_FN broccoli_import, "LoadStarCommand", "11k");
}

/*****************************************************
 * DeallocateFileCommandData: Deallocates environment
 *    data for file commands.
 ******************************************************/
static void _delete_io_data(void *env)
{
    struct batch_item *theEntry, *nextEntry;

    theEntry = get_io_data(env)->TopOfBatchList;

    while( theEntry != NULL )
    {
        nextEntry = theEntry->next;

        if( theEntry->type == FILE_SOURCE )
        {
            sysdep_close_file(env, (FILE *)get_io_data(env)->TopOfBatchList->source);
        }
        else
        {
            core_mem_release(env, theEntry->representation, strlen(theEntry->representation) + 1);
        }

        core_mem_return_struct(env, batch_item, theEntry);

        theEntry = nextEntry;
    }

    if( get_io_data(env)->BatchBuffer != NULL )
    {
        core_mem_release(env, get_io_data(env)->BatchBuffer, get_io_data(env)->BatchMaximumPosition);
    }
}

/************************************************
 * FindBatch: Find routine for the batch router.
 *************************************************/
#if WIN_BTC
#pragma argsused
#endif
static int _find_batch(void *env, char *logicalName)
{
#if MAC_MCW || WIN_MCW || MAC_XCD
#pragma unused(env)
#endif

    if( strcmp(logicalName, "stdin") == 0 )
    {
        return(TRUE);
    }

    return(FALSE);
}

/************************************************
 * GetcBatch: Getc routine for the batch router.
 *************************************************/
static int _get_ch_batch(void *env, char *logicalName)
{
    return(io_get_ch_from_batch(env, logicalName, FALSE));
}

/**************************************************
 * io_get_ch_from_batch: Lower level routine for retrieving
 *   a character when a batch file is active.
 ***************************************************/
int io_get_ch_from_batch(void *env, char *logicalName, int returnOnEOF)
{
    int rv = EOF, flag = 1;

    /*=================================================
     * Get a character until a valid character appears
     * or no more batch files are left.
     *=================================================*/

    while((rv == EOF) && (flag == 1))
    {
        if( get_io_data(env)->BatchType == FILE_SOURCE )
        {
            rv = getc((FILE *)get_io_data(env)->BatchSource);
        }
        else
        {
            rv = get_ch_router(env, (char *)get_io_data(env)->BatchSource);
        }

        if( rv == EOF )
        {
            if( get_io_data(env)->BatchCurrentPosition > 0 )
            {
                print_router(env, "stdout", (char *)get_io_data(env)->BatchBuffer);
            }

            flag = io_pop_batch(env);
        }
    }

    /*=========================================================
     * If the character retrieved is an end-of-file character,
     * then there are no batch files with character input
     * remaining. Remove the batch router.
     *=========================================================*/

    if( rv == EOF )
    {
        if( get_io_data(env)->BatchCurrentPosition > 0 )
        {
            print_router(env, "stdout", (char *)get_io_data(env)->BatchBuffer);
        }

        delete_router(env, "run");
        io_pop_batch(env);

        if( returnOnEOF == TRUE )
        {
            return(EOF);
        }
        else
        {
            return(get_ch_router(env, logicalName));
        }
    }

    /*========================================
     * Add the character to the batch buffer.
     *========================================*/

    get_io_data(env)->BatchBuffer = core_gc_expand_string(env, (char)rv, get_io_data(env)->BatchBuffer, &get_io_data(env)->BatchCurrentPosition,
                                                          &get_io_data(env)->BatchMaximumPosition, get_io_data(env)->BatchMaximumPosition + BUFFER_SZ);

    /*======================================
     * If a carriage return is encountered,
     * then flush the batch buffer.
     *======================================*/

    if((char)rv == '\n' )
    {
        print_router(env, "stdout", (char *)get_io_data(env)->BatchBuffer);
        get_io_data(env)->BatchCurrentPosition = 0;

        if((get_io_data(env)->BatchBuffer != NULL) && (get_io_data(env)->BatchMaximumPosition > BUFFER_SZ))
        {
            core_mem_release(env, get_io_data(env)->BatchBuffer, get_io_data(env)->BatchMaximumPosition);
            get_io_data(env)->BatchMaximumPosition = 0;
            get_io_data(env)->BatchBuffer = NULL;
        }
    }

    /*=====================================================
     * Return the character retrieved from the batch file.
     *=====================================================*/

    return(rv);
}

/****************************************************
 * UngetcBatch: Ungetc routine for the batch router.
 *****************************************************/
#if WIN_BTC
#pragma argsused
#endif
static int _unget_ch_batch(void *env, int ch, char *logicalName)
{
#if MAC_MCW || WIN_MCW || MAC_XCD
#pragma unused(logicalName)
#endif

    if( get_io_data(env)->BatchCurrentPosition > 0 )
    {
        get_io_data(env)->BatchCurrentPosition--;
    }

    if( get_io_data(env)->BatchBuffer != NULL )
    {
        get_io_data(env)->BatchBuffer[get_io_data(env)->BatchCurrentPosition] = EOS;
    }

    if( get_io_data(env)->BatchType == FILE_SOURCE )
    {
        return(ungetc(ch, (FILE *)get_io_data(env)->BatchSource));
    }

    return(unget_ch_router(env, ch, (char *)get_io_data(env)->BatchSource));
}

/************************************************
 * ExitBatch: Exit routine for the batch router.
 *************************************************/
#if WIN_BTC
#pragma argsused
#endif
static int _quit_batch(void *env, int num)
{
#if MAC_MCW || WIN_MCW || MAC_XCD
#pragma unused(num)
#endif
    io_close_all_batches(env);
    return(1);
}

/*************************************
 * broccoli_run: H/L access routine
 *   for the batch command.
 **************************************/
int broccoli_run(void *env)
{
    char *fileName;

    if( core_check_arg_count(env, "run", EXACTLY, 1) == -1 )
    {
        return(FALSE);
    }

    if((fileName = core_get_filename(env, "run", 1)) == NULL )
    {
        return(FALSE);
    }

    return(io_open_batch(env, fileName, FALSE));
}

/**********************************************
 * io_open_batch: Adds a file to the list of files
 *   opened with the batch command.
 ***********************************************/
int io_open_batch(void *env, char *fileName, int placeAtEnd)
{
    FILE *theFile;

    /*======================
     * Open the batch file.
     *======================*/

    theFile = sysdep_open_file(env, fileName, "r");

    if( theFile == NULL )
    {
        report_file_open_error(env, "run", fileName);
        return(FALSE);
    }

    /*============================
     * Create the batch router if
     * it doesn't already exist.
     *============================*/

    if( get_io_data(env)->TopOfBatchList == NULL )
    {
        add_router(env, "run", 20,
                   _find_batch, NULL,
                   _get_ch_batch, _unget_ch_batch,
                   _quit_batch);
    }

    /*====================================
     * Add the newly opened batch file to
     * the list of batch files opened.
     *====================================*/

    _add_batch(env, placeAtEnd, (void *)theFile, FILE_SOURCE, NULL);

    /*===================================
     * Return TRUE to indicate the batch
     * file was successfully opened.
     *===================================*/

    return(TRUE);
}

/******************************************************
 * AddBatch: Creates the batch file data structure and
 *   adds it to the list of opened batch files.
 *******************************************************/
static void _add_batch(void *env, int placeAtEnd, void *theSource, int type, char *theString)
{
    struct batch_item *bptr;

    /*=========================
     * Create the batch entry.
     *=========================*/

    bptr = core_mem_get_struct(env, batch_item);
    bptr->type = type;
    bptr->source = theSource;
    bptr->representation = theString;
    bptr->next = NULL;

    /*============================
     * Add the entry to the list.
     *============================*/

    if( get_io_data(env)->TopOfBatchList == NULL )
    {
        get_io_data(env)->TopOfBatchList = bptr;
        get_io_data(env)->BottomOfBatchList = bptr;
        get_io_data(env)->BatchType = type;
        get_io_data(env)->BatchSource = theSource;
        get_io_data(env)->BatchCurrentPosition = 0;
    }
    else if( placeAtEnd == FALSE )
    {
        bptr->next = get_io_data(env)->TopOfBatchList;
        get_io_data(env)->TopOfBatchList = bptr;
        get_io_data(env)->BatchType = type;
        get_io_data(env)->BatchSource = theSource;
        get_io_data(env)->BatchCurrentPosition = 0;
    }
    else
    {
        get_io_data(env)->BottomOfBatchList->next = bptr;
        get_io_data(env)->BottomOfBatchList = bptr;
    }
}

/*****************************************************************
 * io_pop_batch: Removes the top entry on the list of batch files.
 ******************************************************************/
int io_pop_batch(void *env)
{
    struct batch_item *bptr;
    int rv;

    if( get_io_data(env)->TopOfBatchList == NULL )
    {
        return(FALSE);
    }

    /*==================================================
     * Close the source from which batch input is read.
     *==================================================*/

    if( get_io_data(env)->TopOfBatchList->type == FILE_SOURCE )
    {
        sysdep_close_file(env, (FILE *)get_io_data(env)->TopOfBatchList->source);
    }
    else
    {
        close_string_source(env, (char *)get_io_data(env)->TopOfBatchList->source);
        core_mem_release(env, get_io_data(env)->TopOfBatchList->representation, strlen(get_io_data(env)->TopOfBatchList->representation) + 1);
    }

    /*=================================
     * Remove the entry from the list.
     *=================================*/

    bptr = get_io_data(env)->TopOfBatchList;
    get_io_data(env)->TopOfBatchList = get_io_data(env)->TopOfBatchList->next;

    core_mem_return_struct(env, batch_item, bptr);

    /*========================================================
     * If there are no batch files remaining to be processed,
     * then free the space used by the batch buffer.
     *========================================================*/

    if( get_io_data(env)->TopOfBatchList == NULL )
    {
        get_io_data(env)->BottomOfBatchList = NULL;
        get_io_data(env)->BatchSource = NULL;

        if( get_io_data(env)->BatchBuffer != NULL )
        {
            core_mem_release(env, get_io_data(env)->BatchBuffer, get_io_data(env)->BatchMaximumPosition);
            get_io_data(env)->BatchBuffer = NULL;
        }

        get_io_data(env)->BatchCurrentPosition = 0;
        get_io_data(env)->BatchMaximumPosition = 0;
        rv = 0;
    }

    /*===========================================
     * Otherwise move on to the next batch file.
     *===========================================*/

    else
    {
        get_io_data(env)->BatchType = get_io_data(env)->TopOfBatchList->type;
        get_io_data(env)->BatchSource = get_io_data(env)->TopOfBatchList->source;
        get_io_data(env)->BatchCurrentPosition = 0;
        rv = 1;
    }

    /*====================================================
     * Return TRUE if a batch file if there are remaining
     * batch files to be processed, otherwise FALSE.
     *====================================================*/

    return(rv);
}

/***************************************
 * io_is_batching: Returns TRUE if a batch
 *   file is open, otherwise FALSE.
 ****************************************/
BOOLEAN io_is_batching(void *env)
{
    if( get_io_data(env)->TopOfBatchList != NULL )
    {
        return(TRUE);
    }

    return(FALSE);
}

/*****************************************************
 * io_close_all_batches: Closes all open batch files.
 ******************************************************/
void io_close_all_batches(void *env)
{
    /*================================================
     * Free the batch buffer if it contains anything.
     *================================================*/

    if( get_io_data(env)->BatchBuffer != NULL )
    {
        if( get_io_data(env)->BatchCurrentPosition > 0 )
        {
            print_router(env, "stdout", (char *)get_io_data(env)->BatchBuffer);
        }

        core_mem_release(env, get_io_data(env)->BatchBuffer, get_io_data(env)->BatchMaximumPosition);
        get_io_data(env)->BatchBuffer = NULL;
        get_io_data(env)->BatchCurrentPosition = 0;
        get_io_data(env)->BatchMaximumPosition = 0;
    }

    /*==========================
     * Delete the batch router.
     *==========================*/

    delete_router(env, "run");

    /*=====================================
     * Close each of the open batch files.
     *=====================================*/

    while( io_pop_batch(env))
    {     /* Do Nothing */
    }
}

/*********************************************************
 * broccoli_run_silent: C access routine for the batch* command.
 **********************************************************/
int broccoli_run_silent(void *env, char *fileName)
{
    int inchar;
    FILE *theFile;
    char *theString = NULL;
    size_t position = 0;
    size_t maxChars = 0;

    /*======================
     * Open the batch file.
     *======================*/

    theFile = sysdep_open_file(env, fileName, "r");

    if( theFile == NULL )
    {
        report_file_open_error(env, "batch", fileName);
        return(FALSE);
    }

    /*========================
     * Reset the error state.
     *========================*/

    core_set_halt_eval(env, FALSE);
    core_set_eval_error(env, FALSE);

    /*=============================================
     * Evaluate commands from the file one by one.
     *=============================================*/

    while((inchar = getc(theFile)) != EOF )
    {
        theString = core_gc_expand_string(env, inchar, theString, &position,
                                          &maxChars, maxChars + 80);

        if( core_complete_command(theString) != 0 )
        {
            core_flush_pp_buffer(env);
            core_set_pp_buffer_status(env, OFF);
            core_route_command(env, theString, FALSE);
            core_flush_pp_buffer(env);
            core_set_halt_eval(env, FALSE);
            core_set_eval_error(env, FALSE);
            flush_bindings(env);
            core_mem_free(env, theString, (unsigned)maxChars);
            theString = NULL;
            maxChars = 0;
            position = 0;
        }
    }

    if( theString != NULL )
    {
        core_mem_free(env, theString, (unsigned)maxChars);
    }

    /*=======================
     * Close the batch file.
     *=======================*/

    sysdep_close_file(env, theFile);
    return(TRUE);
}

#if OFF

/**********************************************************
 * LoadCommand: H/L access routine for the load command.
 ***********************************************************/
int LoadCommand(void *env)
{
    char *theFileName;
    int rv;

    if( core_check_arg_count(env, "load", EXACTLY, 1) == -1 )
    {
        return(FALSE);
    }

    if((theFileName = core_get_filename(env, "load", 1)) == NULL )
    {
        return(FALSE);
    }

    core_set_verbose_loading(env, TRUE);

    if((rv = load_parse(env, theFileName)) == FALSE )
    {
        core_set_verbose_loading(env, FALSE);
        report_file_open_error(env, "load", theFileName);
        return(FALSE);
    }

    core_set_verbose_loading(env, FALSE);

    if( rv == -1 )
    {
        return(FALSE);
    }

    return(TRUE);
}

#endif

/***************************************************************
 * broccoli_import: H/L access routine for the load* command.
 ****************************************************************/
int broccoli_import(void *env)
{
    char *theFileName;
    int rv;

    if( core_check_arg_count(env, "import", EXACTLY, 1) == -1 )
    {
        return(FALSE);
    }

    if((theFileName = core_get_filename(env, "import", 1)) == NULL )
    {
        return(FALSE);
    }

    if((rv = load_parse(env, theFileName)) == FALSE )
    {
        report_file_open_error(env, "import", theFileName);
        return(FALSE);
    }

    if( rv == -1 )
    {
        return(FALSE);
    }

    return(TRUE);
}

#if DEBUGGING_FUNCTIONS

/**********************************************************
 * SaveCommand: H/L access routine for the save command.
 ***********************************************************/
int SaveCommand(void *env)
{
    char *theFileName;

    if( core_check_arg_count(env, "save", EXACTLY, 1) == -1 )
    {
        return(FALSE);
    }

    if((theFileName = core_get_filename(env, "save", 1)) == NULL )
    {
        return(FALSE);
    }

    if( core_save(env, theFileName) == FALSE )
    {
        report_file_open_error(env, "save", theFileName);
        return(FALSE);
    }

    return(TRUE);
}

#endif

/** IO_FUNCS STUFF **/

/**************
 * DEFINITIONS
 ***************/

#define FORMAT_MAX  512
#define FLAG_MAX    80

/*******************
 * ENVIRONMENT DATA
 ********************/

#define IO_FUNCTION_DATA_INDEX 64

struct io_locale_data
{
    void *  locale;
    BOOLEAN use_crlf;
};

#define get_io_locale_data(env) ((struct io_locale_data *)core_get_environment_data(env, IO_FUNCTION_DATA_INDEX))

/***************************************
 * LOCAL FUNCTION PROTOTYPES
 ****************************************/

#if IO_FUNCTIONS
static void              ReadTokenFromStdin(void *, struct token *);
static char            * ControlStringCheck(void *, int);
static char FindFormatFlag(char *, size_t *, char *, size_t);
static char            * PrintFormatFlag(void *, char *, int, int);
static char            * FillBuffer(void *, char *, size_t *, size_t *);
static void              ReadNumber(void *, char *, struct token *, int);
#endif

/*************************************
 * init_io_all_functions: Initializes
 *   the I/O functions.
 **************************************/
void init_io_all_functions(void *env)
{
    core_allocate_environment_data(env, IO_FUNCTION_DATA_INDEX, sizeof(struct io_locale_data), NULL);

    get_io_locale_data(env)->use_crlf = FALSE;
    get_io_locale_data(env)->locale = (ATOM_HN *)store_atom(env, setlocale(LC_ALL, NULL));
    inc_atom_count(get_io_locale_data(env)->locale);

    core_define_function(env, "print", RT_VOID, PTR_FN broccoli_print, "PrintoutFunction", "1*");

#if IO_FUNCTIONS
    core_define_function(env, "read",       'u', PTR_FN ReadFunction,  "ReadFunction", "*1");
    core_define_function(env, "open",       'b', OpenFunction,  "OpenFunction", "23*k");
    core_define_function(env, "close",      'b', CloseFunction, "CloseFunction", "*1");
    core_define_function(env, "get-char",   'i', GetCharFunction, "GetCharFunction", "*1");
    core_define_function(env, "remove",   'b', RemoveFunction,  "RemoveFunction", "11k");
    core_define_function(env, "rename",   'b', RenameFunction, "RenameFunction", "22k");
    core_define_function(env, "format",   's', PTR_FN FormatFunction, "FormatFunction", "2**us");
    core_define_function(env, "readline", 'k', PTR_FN ReadlineFunction, "ReadlineFunction", "*1");
    core_define_function(env, "set-locale", 'u', PTR_FN SetLocaleFunction,  "SetLocaleFunction", "*1");
    core_define_function(env, "read-number",       'u', PTR_FN ReadNumberFunction,  "ReadNumberFunction", "*1");
#endif
}

/*****************************************
 * broccoli_print: H/L access routine
 *   for the printout function.
 ******************************************/
void broccoli_print(void *env)
{
    char dummyid[] = "stdout";
    int i, argCount;
    core_data_object arg;

    /*=======================================================
     * The printout function requires at least one argument.
     *=======================================================*/

    if((argCount = core_check_arg_count(env, "printout", AT_LEAST, 1)) == -1 )
    {
        return;
    }

#if OFF

    /*=====================================================
     * Get the logical name to which output is to be sent.
     *=====================================================*/

    dummyid = core_lookup_logical_name(env, 1, "stdout");

    if( dummyid == NULL )
    {
        report_logical_name_lookup_error(env, "printout");
        core_set_halt_eval(env, TRUE);
        core_set_eval_error(env, TRUE);
        return;
    }

    /*============================================================
     * Determine if any router recognizes the output destination.
     *============================================================*/

    if( strcmp(dummyid, "nil") == 0 )
    {
        return;
    }
    else
#endif

    if( query_router(env, dummyid) == FALSE )
    {
        error_unknown_router(env, dummyid);
        return;
    }

    /*===============================================
     * Print each of the arguments sent to printout.
     *===============================================*/

    for( i = 1; i <= argCount; i++ )
    {
        core_get_arg_at(env, i, &arg);

        if( core_get_evaluation_data(env)->halt )
        {
            break;
        }

        switch( core_get_type(arg))
        {
        case ATOM:

            if( strcmp(core_convert_data_to_string(arg), OUT_CRLF) == 0 )
            {
                if( get_io_locale_data(env)->use_crlf )
                {
                    print_router(env, dummyid, "\r\n");
                }
                else
                {
                    print_router(env, dummyid, "\n");
                }
            }
            else if( strcmp(core_convert_data_to_string(arg), OUT_TAB) == 0 )
            {
                print_router(env, dummyid, "\t");
            }
            else
            {
                print_router(env, dummyid, core_convert_data_to_string(arg));
            }

            break;

        case STRING:
            print_router(env, dummyid, core_convert_data_to_string(arg));
            break;

        default:
            core_print_data(env, dummyid, &arg);
            break;
        }
    }
}

#if IO_FUNCTIONS

/****************************************************
 * SetFullCRLF: Set the flag which indicates whether
 *   crlf is treated just as '\n' or '\r\n'.
 *****************************************************/
BOOLEAN SetFullCRLF(void *env, BOOLEAN value)
{
    BOOLEAN oldValue = io_locale_data(env)->use_crlf;

    io_locale_data(env)->use_crlf = value;

    return(oldValue);
}

/************************************************************
 * ReadFunction: H/L access routine for the read function.
 *************************************************************/
void ReadFunction(void *env, core_data_object_ptr ret)
{
    struct token theToken;
    int numberOfArguments;
    char *logical_name = NULL;

    /*===============================================
     * Check for an appropriate number of arguments.
     *===============================================*/

    if((numberOfArguments = core_check_arg_count(env, "read", NO_MORE_THAN, 1)) == -1 )
    {
        ret->type = STRING;
        ret->value = (void *)store_atom(env, "*** READ ERROR ***");
        return;
    }

    /*======================================================
     * Determine the logical name from which input is read.
     *======================================================*/

    if( numberOfArguments == 0 )
    {
        logical_name = "stdin";
    }
    else if( numberOfArguments == 1 )
    {
        logical_name = core_lookup_logical_name(env, 1, "stdin");

        if( logical_name == NULL )
        {
            report_logical_name_lookup_error(env, "read");
            core_set_halt_eval(env, TRUE);
            core_set_eval_error(env, TRUE);
            ret->type = STRING;
            ret->value = (void *)store_atom(env, "*** READ ERROR ***");
            return;
        }
    }

    /*============================================
     * Check to see that the logical name exists.
     *============================================*/

    if( query_router(env, logical_name) == FALSE )
    {
        error_unknown_router(env, logical_name);
        core_set_halt_eval(env, TRUE);
        core_set_eval_error(env, TRUE);
        ret->type = STRING;
        ret->value = (void *)store_atom(env, "*** READ ERROR ***");
        return;
    }

    /*=======================================
     * Collect input into string if the read
     * source is stdin, else just get token.
     *=======================================*/

    if( strcmp(logical_name, "stdin") == 0 )
    {
        ReadTokenFromStdin(env, &theToken);
    }
    else
    {
        core_get_token(env, logical_name, &theToken);
    }

    get_router_data(env)->command_buffer_input_count = 0;
    get_router_data(env)->is_waiting = FALSE;

    /*====================================================
     * Copy the token to the return value data structure.
     *====================================================*/

    ret->type = theToken.type;

    if((theToken.type == FLOAT) || (theToken.type == STRING) ||
#if OBJECT_SYSTEM
       (theToken.type == INSTANCE_NAME) ||
#endif
       (theToken.type == ATOM) || (theToken.type == INTEGER))
    {
        ret->value = theToken.value;
    }
    else if( theToken.type == STOP )
    {
        ret->type = ATOM;
        ret->value = (void *)store_atom(env, "EOF");
    }
    else if( theToken.type == UNKNOWN_VALUE )
    {
        ret->type = STRING;
        ret->value = (void *)store_atom(env, "*** READ ERROR ***");
    }
    else
    {
        ret->type = STRING;
        ret->value = (void *)store_atom(env, theToken.pp);
    }

    return;
}

/*******************************************************
 * ReadTokenFromStdin: Special routine used by the read
 *   function to read a token from standard input.
 ********************************************************/
static void ReadTokenFromStdin(void *env, struct token *theToken)
{
    char *inputString;
    size_t inputStringSize;
    int inchar;

    /*=============================================
     * Continue processing until a token is found.
     *=============================================*/

    theToken->type = STOP;

    while( theToken->type == STOP )
    {
        /*===========================================
         * Initialize the variables used for storing
         * the characters retrieved from stdin.
         *===========================================*/

        inputString = NULL;
        get_router_data(env)->command_buffer_input_count = 0;
        get_router_data(env)->is_waiting = TRUE;
        inputStringSize = 0;
        inchar = get_ch_router(env, "stdin");

        /*========================================================
         * Continue reading characters until a carriage return is
         * entered or the user halts execution (usually with
         * control-c). Waiting for the carriage return prevents
         * the input from being prematurely parsed (such as when
         * a space is entered after a type_symbol.has been typed).
         *========================================================*/

        while((inchar != '\n') && (inchar != '\r') && (inchar != EOF) &&
              (!core_get_halt_eval(env)))
        {
            inputString = core_gc_expand_string(env, inchar, inputString, &get_router_data(env)->command_buffer_input_count,
                                                &inputStringSize, inputStringSize + 80);
            inchar = get_ch_router(env, "stdin");
        }

        /*==================================================
         * Open a string input source using the characters
         * retrieved from stdin and extract the first token
         * contained in the string.
         *==================================================*/

        open_string_source(env, "read", inputString, 0);
        core_get_token(env, "read", theToken);
        close_string_source(env, "read");

        if( inputStringSize > 0 )
        {
            core_mem_release(env, inputString, inputStringSize);
        }

        /*===========================================
         * Pressing control-c (or comparable action)
         * aborts the read function.
         *===========================================*/

        if( core_get_halt_eval(env))
        {
            theToken->type = STRING;
            theToken->value = (void *)store_atom(env, "*** READ ERROR ***");
        }

        /*====================================================
         * Return the EOF symbol if the end of file for stdin
         * has been encountered. This typically won't occur,
         * but is possible (for example by pressing control-d
         * in the UNIX operating system).
         *====================================================*/

        if((theToken->type == STOP) && (inchar == EOF))
        {
            theToken->type = ATOM;
            theToken->value = (void *)store_atom(env, "EOF");
        }
    }
}

/************************************************************
 * OpenFunction: H/L access routine for the open function.
 *************************************************************/
int OpenFunction(void *env)
{
    int numberOfArguments;
    char *fileName, *logical_name, *accessMode = NULL;
    core_data_object arg;

    /*========================================
     * Check for a valid number of arguments.
     *========================================*/

    if((numberOfArguments = core_check_arg_range(env, "open", 2, 3)) == -1 )
    {
        return(0);
    }

    /*====================
     * Get the file name.
     *====================*/

    if((fileName = core_get_filename(env, "open", 1)) == NULL )
    {
        return(0);
    }

    /*=======================================
     * Get the logical name to be associated
     * with the opened file.
     *=======================================*/

    logical_name = core_lookup_logical_name(env, 2, NULL);

    if( logical_name == NULL )
    {
        core_set_halt_eval(env, TRUE);
        core_set_eval_error(env, TRUE);
        report_logical_name_lookup_error(env, "open");
        return(0);
    }

    /*==================================
     * Check to see if the logical name
     * is already in use.
     *==================================*/

    if( find_file(env, logical_name))
    {
        core_set_halt_eval(env, TRUE);
        core_set_eval_error(env, TRUE);
        error_print_id(env, "IO", 2, FALSE);
        print_router(env, WERROR, "Logical name ");
        print_router(env, WERROR, logical_name);
        print_router(env, WERROR, " already in use.\n");
        return(0);
    }

    /*===========================
     * Get the file access mode.
     *===========================*/

    if( numberOfArguments == 2 )
    {
        accessMode = "r";
    }
    else if( numberOfArguments == 3 )
    {
        if( core_check_arg_type(env, "open", 3, STRING, &arg) == FALSE )
        {
            return(0);
        }

        accessMode = core_convert_data_to_string(arg);
    }

    /*=====================================
     * Check for a valid file access mode.
     *=====================================*/

    if((strcmp(accessMode, "r") != 0) &&
       (strcmp(accessMode, "r+") != 0) &&
       (strcmp(accessMode, "w") != 0) &&
       (strcmp(accessMode, "a") != 0) &&
       (strcmp(accessMode, "wb") != 0))
    {
        core_set_halt_eval(env, TRUE);
        core_set_eval_error(env, TRUE);
        report_explicit_type_error(env, "open", 3, "string with value \"r\", \"r+\", \"w\", \"wb\", or \"a\"");
        return(0);
    }

    /*================================================
     * Open the named file and associate it with the
     * specified logical name. Return TRUE if the
     * file was opened successfully, otherwise FALSE.
     *================================================*/

    return(open_file(env, fileName, accessMode, logical_name));
}

/**************************************************************
 * CloseFunction: H/L access routine for the close function.
 ***************************************************************/
int CloseFunction(void *env)
{
    int numberOfArguments;
    char *logical_name;

    /*======================================
     * Check for valid number of arguments.
     *======================================*/

    if((numberOfArguments = core_check_arg_count(env, "close", NO_MORE_THAN, 1)) == -1 )
    {
        return(0);
    }

    /*=====================================================
     * If no arguments are specified, then close all files
     * opened with the open command. Return TRUE if all
     * files were closed successfully, otherwise FALSE.
     *=====================================================*/

    if( numberOfArguments == 0 )
    {
        return(close_all_files(env));
    }

    /*================================
     * Get the logical name argument.
     *================================*/

    logical_name = core_lookup_logical_name(env, 1, NULL);

    if( logical_name == NULL )
    {
        report_logical_name_lookup_error(env, "close");
        core_set_halt_eval(env, TRUE);
        core_set_eval_error(env, TRUE);
        return(0);
    }

    /*========================================================
     * Close the file associated with the specified logical
     * name. Return TRUE if the file was closed successfully,
     * otherwise false.
     *========================================================*/

    return(close_file(env, logical_name));
}

/**************************************
 * GetCharFunction: H/L access routine
 *   for the get-char function.
 ***************************************/
int GetCharFunction(void *env)
{
    int numberOfArguments;
    char *logical_name;

    if((numberOfArguments = core_check_arg_count(env, "get-char", NO_MORE_THAN, 1)) == -1 )
    {
        return(-1);
    }

    if( numberOfArguments == 0 )
    {
        logical_name = "stdin";
    }
    else
    {
        logical_name = core_lookup_logical_name(env, 1, "stdin");

        if( logical_name == NULL )
        {
            report_logical_name_lookup_error(env, "get-char");
            core_set_halt_eval(env, TRUE);
            core_set_eval_error(env, TRUE);
            return(-1);
        }
    }

    if( query_router(env, logical_name) == FALSE )
    {
        error_unknown_router(env, logical_name);
        core_set_halt_eval(env, TRUE);
        core_set_eval_error(env, TRUE);
        return(-1);
    }

    return(get_ch_router(env, logical_name));
}

/***************************************
 * RemoveFunction: H/L access routine
 *   for the remove function.
 ****************************************/
int RemoveFunction(void *env)
{
    char *theFileName;

    /*======================================
     * Check for valid number of arguments.
     *======================================*/

    if( core_check_arg_count(env, "remove", EXACTLY, 1) == -1 )
    {
        return(FALSE);
    }

    /*====================
     * Get the file name.
     *====================*/

    if((theFileName = core_get_filename(env, "remove", 1)) == NULL )
    {
        return(FALSE);
    }

    /*==============================================
     * Remove the file. Return TRUE if the file was
     * sucessfully removed, otherwise FALSE.
     *==============================================*/

    return(sysdep_remove_file(theFileName));
}

/***************************************
 * RenameFunction: H/L access routine
 *   for the rename function.
 ****************************************/
int RenameFunction(void *env)
{
    char *oldFileName, *newFileName;

    /*========================================
     * Check for a valid number of arguments.
     *========================================*/

    if( core_check_arg_count(env, "rename", EXACTLY, 2) == -1 )
    {
        return(FALSE);
    }

    /*===========================
     * Check for the file names.
     *===========================*/

    if((oldFileName = core_get_filename(env, "rename", 1)) == NULL )
    {
        return(FALSE);
    }

    if((newFileName = core_get_filename(env, "rename", 2)) == NULL )
    {
        return(FALSE);
    }

    /*==============================================
     * Rename the file. Return TRUE if the file was
     * sucessfully renamed, otherwise FALSE.
     *==============================================*/

    return(sysdep_rename_file(oldFileName, newFileName));
}

/***************************************
 * FormatFunction: H/L access routine
 *   for the format function.
 ****************************************/
void *FormatFunction(void *env)
{
    int argCount;
    size_t start_pos;
    char *formatString, *logical_name;
    char formatFlagType;
    int f_cur_arg = 3;
    size_t form_pos = 0;
    char percentBuffer[FLAG_MAX];
    char *fstr = NULL;
    size_t fmaxm = 0;
    size_t fpos = 0;
    void *hptr;
    char *representation;

    /*======================================
     * Set default return value for errors.
     *======================================*/

    hptr = store_atom(env, "");

    /*=========================================
     * Format requires at least two arguments:
     * a logical name and a format string.
     *=========================================*/

    if((argCount = core_check_arg_count(env, "format", AT_LEAST, 2)) == -1 )
    {
        return(hptr);
    }

    /*========================================
     * First argument must be a logical name.
     *========================================*/

    if((logical_name = core_lookup_logical_name(env, 1, "stdout")) == NULL )
    {
        report_logical_name_lookup_error(env, "format");
        core_set_halt_eval(env, TRUE);
        core_set_eval_error(env, TRUE);
        return(hptr);
    }

    if( strcmp(logical_name, "nil") == 0 )
    {     /* do nothing */
    }
    else if( query_router(env, logical_name) == FALSE )
    {
        error_unknown_router(env, logical_name);
        return(hptr);
    }

    /*=====================================================
     * Second argument must be a string.  The appropriate
     * number of arguments specified by the string must be
     * present in the argument list.
     *=====================================================*/

    if((formatString = ControlStringCheck(env, argCount)) == NULL )
    {
        return(hptr);
    }

    /*========================================
     * Search the format string, printing the
     * format flags as they are encountered.
     *========================================*/

    while( formatString[form_pos] != '\0' )
    {
        if( formatString[form_pos] != '%' )
        {
            start_pos = form_pos;

            while((formatString[form_pos] != '%') &&
                  (formatString[form_pos] != '\0'))
            {
                form_pos++;
            }

            fstr = core_gc_append_subset_to_string(env, &formatString[start_pos], fstr, form_pos - start_pos, &fpos, &fmaxm);
        }
        else
        {
            form_pos++;
            formatFlagType = FindFormatFlag(formatString, &form_pos, percentBuffer, FLAG_MAX);

            if( formatFlagType != ' ' )
            {
                if((representation = PrintFormatFlag(env, percentBuffer, f_cur_arg, formatFlagType)) == NULL )
                {
                    if( fstr != NULL )
                    {
                        core_mem_release(env, fstr, fmaxm);
                    }

                    return(hptr);
                }

                fstr = core_gc_append_to_string(env, representation, fstr, &fpos, &fmaxm);

                if( fstr == NULL )
                {
                    return(hptr);
                }

                f_cur_arg++;
            }
            else
            {
                fstr = core_gc_append_to_string(env, percentBuffer, fstr, &fpos, &fmaxm);

                if( fstr == NULL )
                {
                    return(hptr);
                }
            }
        }
    }

    if( fstr != NULL )
    {
        hptr = store_atom(env, fstr);

        if( strcmp(logical_name, "nil") != 0 )
        {
            print_router(env, logical_name, fstr);
        }

        core_mem_release(env, fstr, fmaxm);
    }
    else
    {
        hptr = store_atom(env, "");
    }

    return(hptr);
}

/********************************************************************
 * ControlStringCheck:  Checks the 2nd parameter which is the format
 *   control string to see if there are enough matching arguments.
 *********************************************************************/
static char *ControlStringCheck(void *env, int argCount)
{
    core_data_object t_ptr;
    char *str_array;
    char print_buff[FLAG_MAX];
    size_t i;
    int per_count;
    char formatFlag;

    if( core_check_arg_type(env, "format", 2, STRING, &t_ptr) == FALSE )
    {
        return(NULL);
    }

    per_count = 0;
    str_array = to_string(t_ptr.value);

    for( i = 0 ; str_array[i] != '\0' ;)
    {
        if( str_array[i] == '%' )
        {
            i++;
            formatFlag = FindFormatFlag(str_array, &i, print_buff, FLAG_MAX);

            if( formatFlag == '-' )
            {
                error_print_id(env, "IO", 3, FALSE);
                print_router(env, WERROR, "Invalid format flag \"");
                print_router(env, WERROR, print_buff);
                print_router(env, WERROR, "\" specified in format function.\n");
                core_set_eval_error(env, TRUE);
                return(NULL);
            }
            else if( formatFlag != ' ' )
            {
                per_count++;
            }
        }
        else
        {
            i++;
        }
    }

    if( per_count != (argCount - 2))
    {
        report_arg_count_error(env, "format", EXACTLY, per_count + 2);
        core_set_eval_error(env, TRUE);
        return(NULL);
    }

    return(str_array);
}

/**********************************************
 * FindFormatFlag:  This function searches for
 *   a format flag in the format string.
 ***********************************************/
static char FindFormatFlag(char *formatString, size_t *a, char *formatBuffer, size_t bufferMax)
{
    char inchar, formatFlagType;
    size_t copy_pos = 0;

    /*====================================================
     * Set return values to the default value. A blank
     * character indicates that no format flag was found
     * which requires a parameter.
     *====================================================*/

    formatFlagType = ' ';

    /*=====================================================
     * The format flags for carriage returns, line feeds,
     * horizontal and vertical tabs, and the percent sign,
     * do not require a parameter.
     *=====================================================*/

    if( formatString[*a] == 'n' )
    {
        sysdep_sprintf(formatBuffer, "\n");
        (*a)++;
        return(formatFlagType);
    }
    else if( formatString[*a] == 'r' )
    {
        sysdep_sprintf(formatBuffer, "\r");
        (*a)++;
        return(formatFlagType);
    }
    else if( formatString[*a] == 't' )
    {
        sysdep_sprintf(formatBuffer, "\t");
        (*a)++;
        return(formatFlagType);
    }
    else if( formatString[*a] == 'v' )
    {
        sysdep_sprintf(formatBuffer, "\v");
        (*a)++;
        return(formatFlagType);
    }
    else if( formatString[*a] == '%' )
    {
        sysdep_sprintf(formatBuffer, "%%");
        (*a)++;
        return(formatFlagType);
    }

    /*======================================================
     * Identify the format flag which requires a parameter.
     *======================================================*/

    formatBuffer[copy_pos++] = '%';
    formatBuffer[copy_pos] = '\0';

    while((formatString[*a] != '%') &&
          (formatString[*a] != '\0') &&
          (copy_pos < (bufferMax - 5)))
    {
        inchar = formatString[*a];
        (*a)++;

        if( (inchar == 'd') ||
            (inchar == 'o') ||
            (inchar == 'x') ||
            (inchar == 'u'))
        {
            formatFlagType = inchar;
            formatBuffer[copy_pos++] = 'l';
            formatBuffer[copy_pos++] = 'l';
            formatBuffer[copy_pos++] = inchar;
            formatBuffer[copy_pos] = '\0';
            return(formatFlagType);
        }
        else if( (inchar == 'c') ||
                 (inchar == 's') ||
                 (inchar == 'e') ||
                 (inchar == 'f') ||
                 (inchar == 'g') )
        {
            formatBuffer[copy_pos++] = inchar;
            formatBuffer[copy_pos] = '\0';
            formatFlagType = inchar;
            return(formatFlagType);
        }

        /*=======================================================
         * If the type hasn't been read, then this should be the
         * -M.N part of the format specification (where M and N
         * are integers).
         *=======================================================*/

        if( (!isdigit(inchar)) &&
            (inchar != '.') &&
            (inchar != '-') )
        {
            formatBuffer[copy_pos++] = inchar;
            formatBuffer[copy_pos] = '\0';
            return('-');
        }

        formatBuffer[copy_pos++] = inchar;
        formatBuffer[copy_pos] = '\0';
    }

    return(formatFlagType);
}

/*********************************************************************
 * PrintFormatFlag:  Prints out part of the total format string along
 *   with the argument for that part of the format string.
 **********************************************************************/
static char *PrintFormatFlag(void *env, char *formatString, int whichArg, int formatType)
{
    core_data_object theResult;
    char *representation, *printBuffer;
    size_t theLength;
    void *oldLocale;

    /*=================
     * String argument
     *=================*/

    switch( formatType )
    {
    case 's':

        if( core_check_arg_type(env, "format", whichArg, ATOM_OR_STRING, &theResult) == FALSE )
        {
            return(NULL);
        }

        theLength = strlen(formatString) + strlen(to_string(theResult.value)) + 200;
        printBuffer = (char *)core_mem_alloc_no_init(env, (sizeof(char) * theLength));
        sysdep_sprintf(printBuffer, formatString, to_string(theResult.value));
        break;

    case 'c':
        core_get_arg_at(env, whichArg, &theResult);

        if((core_get_type(theResult) == STRING) ||
           (core_get_type(theResult) == ATOM))
        {
            theLength = strlen(formatString) + 200;
            printBuffer = (char *)core_mem_alloc_no_init(env, (sizeof(char) * theLength));
            sysdep_sprintf(printBuffer, formatString, (to_string(theResult.value))[0]);
        }
        else if( core_get_type(theResult) == INTEGER )
        {
            theLength = strlen(formatString) + 200;
            printBuffer = (char *)core_mem_alloc_no_init(env, (sizeof(char) * theLength));
            sysdep_sprintf(printBuffer, formatString, (char)core_convert_data_to_long(theResult));
        }
        else
        {
            report_explicit_type_error(env, "format", whichArg, "symbol, string, or integer");
            return(NULL);
        }

        break;

    case 'd':
    case 'x':
    case 'o':
    case 'u':

        if( core_check_arg_type(env, "format", whichArg, INTEGER_OR_FLOAT, &theResult) == FALSE )
        {
            return(NULL);
        }

        theLength = strlen(formatString) + 200;
        printBuffer = (char *)core_mem_alloc_no_init(env, (sizeof(char) * theLength));

        oldLocale = store_atom(env, setlocale(LC_NUMERIC, NULL));
        setlocale(LC_NUMERIC, to_string(io_locale_data(env)->locale));

        if( core_get_type(theResult) == FLOAT )
        {
            sysdep_sprintf(printBuffer, formatString, (long long)to_double(theResult.value));
        }
        else
        {
            sysdep_sprintf(printBuffer, formatString, (long long)to_long(theResult.value));
        }

        setlocale(LC_NUMERIC, to_string(oldLocale));
        break;

    case 'f':
    case 'g':
    case 'e':

        if( core_check_arg_type(env, "format", whichArg, INTEGER_OR_FLOAT, &theResult) == FALSE )
        {
            return(NULL);
        }

        theLength = strlen(formatString) + 200;
        printBuffer = (char *)core_mem_alloc_no_init(env, (sizeof(char) * theLength));

        oldLocale = store_atom(env, setlocale(LC_NUMERIC, NULL));

        setlocale(LC_NUMERIC, to_string(io_locale_data(env)->locale));

        if( core_get_type(theResult) == FLOAT )
        {
            sysdep_sprintf(printBuffer, formatString, to_double(theResult.value));
        }
        else
        {
            sysdep_sprintf(printBuffer, formatString, (double)to_long(theResult.value));
        }

        setlocale(LC_NUMERIC, to_string(oldLocale));

        break;

    default:
        print_router(env, WERROR, " Error in format, the conversion character");
        print_router(env, WERROR, " for formatted output is not valid\n");
        return(FALSE);
    }

    representation = to_string(store_atom(env, printBuffer));
    core_mem_release(env, printBuffer, sizeof(char) * theLength);
    return(representation);
}

/*****************************************
 * ReadlineFunction: H/L access routine
 *   for the readline function.
 ******************************************/
void ReadlineFunction(void *env, core_data_object_ptr ret)
{
    char *buffer;
    size_t line_max = 0;
    int numberOfArguments;
    char *logical_name;

    ret->type = STRING;

    if((numberOfArguments = core_check_arg_count(env, "readline", NO_MORE_THAN, 1)) == -1 )
    {
        ret->value = (void *)store_atom(env, "*** READ ERROR ***");
        return;
    }

    if( numberOfArguments == 0 )
    {
        logical_name = "stdin";
    }
    else
    {
        logical_name = core_lookup_logical_name(env, 1, "stdin");

        if( logical_name == NULL )
        {
            report_logical_name_lookup_error(env, "readline");
            core_set_halt_eval(env, TRUE);
            core_set_eval_error(env, TRUE);
            ret->value = (void *)store_atom(env, "*** READ ERROR ***");
            return;
        }
    }

    if( query_router(env, logical_name) == FALSE )
    {
        error_unknown_router(env, logical_name);
        core_set_halt_eval(env, TRUE);
        core_set_eval_error(env, TRUE);
        ret->value = (void *)store_atom(env, "*** READ ERROR ***");
        return;
    }

    get_router_data(env)->command_buffer_input_count = 0;
    get_router_data(env)->is_waiting = TRUE;
    buffer = FillBuffer(env, logical_name, &get_router_data(env)->command_buffer_input_count, &line_max);
    get_router_data(env)->command_buffer_input_count = 0;
    get_router_data(env)->is_waiting = FALSE;

    if( core_get_halt_eval(env))
    {
        ret->value = (void *)store_atom(env, "*** READ ERROR ***");

        if( buffer != NULL )
        {
            core_mem_release(env, buffer, (int)sizeof(char) * line_max);
        }

        return;
    }

    if( buffer == NULL )
    {
        ret->value = (void *)store_atom(env, "EOF");
        ret->type = ATOM;
        return;
    }

    ret->value = (void *)store_atom(env, buffer);
    core_mem_release(env, buffer, (int)sizeof(char) * line_max);
    return;
}

/************************************************************
 * FillBuffer: Read characters from a specified logical name
 *   and places them into a buffer until a carriage return
 *   or end-of-file character is read.
 *************************************************************/
static char *FillBuffer(void *env, char *logical_name, size_t *position, size_t *maximumSize)
{
    int c;
    char *buf = NULL;

    /*================================
     * Read until end of line or eof.
     *================================*/

    c = get_ch_router(env, logical_name);

    if( c == EOF )
    {
        return(NULL);
    }

    /*==================================
     * Grab characters until cr or eof.
     *==================================*/

    while((c != '\n') && (c != '\r') && (c != EOF) &&
          (!core_get_halt_eval(env)))
    {
        buf = core_gc_expand_string(env, c, buf, position, maximumSize, *maximumSize + 80);
        c = get_ch_router(env, logical_name);
    }

    /*==================
     * Add closing EOS.
     *==================*/

    buf = core_gc_expand_string(env, EOS, buf, position, maximumSize, *maximumSize + 80);
    return(buf);
}

/****************************************
 * SetLocaleFunction: H/L access routine
 *   for the set-locale function.
 *****************************************/
void SetLocaleFunction(void *env, core_data_object_ptr ret)
{
    core_data_object theResult;
    int numArgs;

    /*======================================
     * Check for valid number of arguments.
     *======================================*/

    if((numArgs = core_check_arg_count(env, "set-locale", NO_MORE_THAN, 1)) == -1 )
    {
        ret->type = ATOM;
        ret->value = get_false(env);
        return;
    }

    /*=================================
     * If there are no arguments, just
     * return the current locale.
     *=================================*/

    if( numArgs == 0 )
    {
        ret->type = STRING;
        ret->value = io_locale_data(env)->locale;
        return;
    }

    /*=================
     * Get the locale.
     *=================*/

    if( core_check_arg_type(env, "set-locale", 1, STRING, &theResult) == FALSE )
    {
        ret->type = ATOM;
        ret->value = get_false(env);
        return;
    }

    /*=====================================
     * Return the old value of the locale.
     *=====================================*/

    ret->type = STRING;
    ret->value = io_locale_data(env)->locale;

    /*======================================================
     * Change the value of the locale to the one specified.
     *======================================================*/

    dec_atom_count(env, (struct atom_hash_node *)io_locale_data(env)->locale);
    io_locale_data(env)->locale = core_convert_data_to_pointe(theResult);
    inc_atom_count(io_locale_data(env)->locale);
}

/*****************************************
 * ReadNumberFunction: H/L access routine
 *   for the read-number function.
 ******************************************/
void ReadNumberFunction(void *env, core_data_object_ptr ret)
{
    struct token theToken;
    int numberOfArguments;
    char *logical_name = NULL;

    /*===============================================
     * Check for an appropriate number of arguments.
     *===============================================*/

    if((numberOfArguments = core_check_arg_count(env, "read", NO_MORE_THAN, 1)) == -1 )
    {
        ret->type = STRING;
        ret->value = (void *)store_atom(env, "*** READ ERROR ***");
        return;
    }

    /*======================================================
     * Determine the logical name from which input is read.
     *======================================================*/

    if( numberOfArguments == 0 )
    {
        logical_name = "stdin";
    }
    else if( numberOfArguments == 1 )
    {
        logical_name = core_lookup_logical_name(env, 1, "stdin");

        if( logical_name == NULL )
        {
            report_logical_name_lookup_error(env, "read");
            core_set_halt_eval(env, TRUE);
            core_set_eval_error(env, TRUE);
            ret->type = STRING;
            ret->value = (void *)store_atom(env, "*** READ ERROR ***");
            return;
        }
    }

    /*============================================
     * Check to see that the logical name exists.
     *============================================*/

    if( query_router(env, logical_name) == FALSE )
    {
        error_unknown_router(env, logical_name);
        core_set_halt_eval(env, TRUE);
        core_set_eval_error(env, TRUE);
        ret->type = STRING;
        ret->value = (void *)store_atom(env, "*** READ ERROR ***");
        return;
    }

    /*=======================================
     * Collect input into string if the read
     * source is stdin, else just get token.
     *=======================================*/

    if( strcmp(logical_name, "stdin") == 0 )
    {
        ReadNumber(env, logical_name, &theToken, TRUE);
    }
    else
    {
        ReadNumber(env, logical_name, &theToken, FALSE);
    }

    get_router_data(env)->command_buffer_input_count = 0;
    get_router_data(env)->is_waiting = FALSE;

    /*====================================================
     * Copy the token to the return value data structure.
     *====================================================*/

    ret->type = theToken.type;

    if((theToken.type == FLOAT) || (theToken.type == STRING) ||
#if OBJECT_SYSTEM
       (theToken.type == INSTANCE_NAME) ||
#endif
       (theToken.type == ATOM) || (theToken.type == INTEGER))
    {
        ret->value = theToken.value;
    }
    else if( theToken.type == STOP )
    {
        ret->type = ATOM;
        ret->value = (void *)store_atom(env, "EOF");
    }
    else if( theToken.type == UNKNOWN_VALUE )
    {
        ret->type = STRING;
        ret->value = (void *)store_atom(env, "*** READ ERROR ***");
    }
    else
    {
        ret->type = STRING;
        ret->value = (void *)store_atom(env, theToken.pp);
    }

    return;
}

/*******************************************
 * ReadNumber: Special routine used by the
 *   read-number function to read a number.
 ********************************************/
static void ReadNumber(void *env, char *logical_name, struct token *theToken, int isStdin)
{
    char *inputString;
    char *charPtr = NULL;
    size_t inputStringSize;
    int inchar;
    long long theLong;
    double theDouble;
    void *oldLocale;

    theToken->type = STOP;

    /*===========================================
     * Initialize the variables used for storing
     * the characters retrieved from stdin.
     *===========================================*/

    inputString = NULL;
    get_router_data(env)->command_buffer_input_count = 0;
    get_router_data(env)->is_waiting = TRUE;
    inputStringSize = 0;
    inchar = get_ch_router(env, logical_name);

    /*====================================
     * Skip whitespace before any number.
     *====================================*/

    while( isspace(inchar) && (inchar != EOF) &&
           (!core_get_halt_eval(env)))
    {
        inchar = get_ch_router(env, logical_name);
    }

    /*=============================================================
     * Continue reading characters until whitespace is found again
     * (for anything other than stdin) or a CR/LF (for stdin).
     *=============================================================*/

    while((((!isStdin) && (!isspace(inchar))) ||
           (isStdin && (inchar != '\n') && (inchar != '\r'))) &&
          (inchar != EOF) &&
          (!core_get_halt_eval(env)))
    {
        inputString = core_gc_expand_string(env, inchar, inputString, &get_router_data(env)->command_buffer_input_count,
                                            &inputStringSize, inputStringSize + 80);
        inchar = get_ch_router(env, logical_name);
    }

    /*===========================================
     * Pressing control-c (or comparable action)
     * aborts the read-number function.
     *===========================================*/

    if( core_get_halt_eval(env))
    {
        theToken->type = STRING;
        theToken->value = (void *)store_atom(env, "*** READ ERROR ***");

        if( inputStringSize > 0 )
        {
            core_mem_release(env, inputString, inputStringSize);
        }

        return;
    }

    /*====================================================
     * Return the EOF symbol if the end of file for stdin
     * has been encountered. This typically won't occur,
     * but is possible (for example by pressing control-d
     * in the UNIX operating system).
     *====================================================*/

    if( inchar == EOF )
    {
        theToken->type = ATOM;
        theToken->value = (void *)store_atom(env, "EOF");

        if( inputStringSize > 0 )
        {
            core_mem_release(env, inputString, inputStringSize);
        }

        return;
    }

    /*==================================================
     * Open a string input source using the characters
     * retrieved from stdin and extract the first token
     * contained in the string.
     *==================================================*/

    /*=======================================
     * Change the locale so that numbers are
     * converted using the localized format.
     *=======================================*/

    oldLocale = store_atom(env, setlocale(LC_NUMERIC, NULL));
    setlocale(LC_NUMERIC, to_string(io_locale_data(env)->locale));

    /*========================================
     * Try to parse the number as a long. The
     * terminating character must either be
     * white space or the string terminator.
     *========================================*/

#if WIN_MVC
    theLong = _strtoi64(inputString, &charPtr, 10);
#else
    theLong = strtoll(inputString, &charPtr, 10);
#endif

    if((charPtr != inputString) &&
       (isspace(*charPtr) || (*charPtr == '\0')))
    {
        theToken->type = INTEGER;
        theToken->value = (void *)store_long(env, theLong);

        if( inputStringSize > 0 )
        {
            core_mem_release(env, inputString, inputStringSize);
        }

        setlocale(LC_NUMERIC, to_string(oldLocale));
        return;
    }

    /*==========================================
     * Try to parse the number as a double. The
     * terminating character must either be
     * white space or the string terminator.
     *==========================================*/

    theDouble = strtod(inputString, &charPtr);

    if((charPtr != inputString) &&
       (isspace(*charPtr) || (*charPtr == '\0')))
    {
        theToken->type = FLOAT;
        theToken->value = (void *)store_double(env, theDouble);

        if( inputStringSize > 0 )
        {
            core_mem_release(env, inputString, inputStringSize);
        }

        setlocale(LC_NUMERIC, to_string(oldLocale));
        return;
    }

    /*============================================
     * Restore the "C" locale so that any parsing
     * of numbers uses the C format.
     *============================================*/

    setlocale(LC_NUMERIC, to_string(oldLocale));

    /*=========================================
     * Return "*** READ ERROR ***" to indicate
     * a number was not successfully parsed.
     *=========================================*/

    theToken->type = STRING;
    theToken->value = (void *)store_atom(env, "*** READ ERROR ***");
}

#endif
