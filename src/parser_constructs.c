/* Purpose: Parsing routines and utilities for parsing
 *   constructs.                                             */

#define __CONSTRUCTS_PARSER_SOURCE__

#include "setup.h"


#include <stdio.h>
#define _STDIO_INCLUDED_
#include <stdlib.h>

#include "core_environment.h"
#include "router.h"
#include "core_watch.h"
#include "core_constructs.h"
#include "parser_flow_control.h"
#include "parser_expressions.h"
#include "modules_query.h"
#include "parser_modules.h"
#include "sysdep.h"
#include "core_gc.h"

#include "parser_constructs.h"

/**************************************
 * LOCAL FUNCTION PROTOTYPES
 ***************************************/

static int _find_construct_start(void *, char *, struct token *, int, int *);
static int _load_from_logical_name(void *, char *);

/***********************************************************
 * load_parse: C access routine for the load command. Returns
 *   0 if the file couldn't be opened, -1 if the file was
 *   opened but an error occurred while loading constructs,
 *   and 1 if the file was opened and no errors occured
 *   while loading.
 ************************************************************/
int load_parse(void *env, char *fileName)
{
    FILE *theFile;
    int noErrorsDetected;

    /*=======================================
     * Open the file specified by file name.
     *=======================================*/

    if((theFile = sysdep_open_file(env, fileName, "r")) == NULL )
    {
        return(0);
    }

    /*===================================================
     * Read in the constructs. Enabling fast load allows
     * the router system to be bypassed for quicker load
     * times.
     *===================================================*/

    set_fast_load(env, theFile);
    noErrorsDetected = _load_from_logical_name(env, (char *)theFile);
    set_fast_load(env, NULL);

    /*=================
     * Close the file.
     *=================*/

    sysdep_close_file(env, theFile);

    /*========================================
     * If no errors occurred during the load,
     * return 1, otherwise return -1.
     *========================================*/

    if( noErrorsDetected )
    {
        return(1);
    }

    return(-1);
}

/****************************************************************
 * LoadConstructsFromLogicalName: Loads a set of constructs into
 *   the current environment from a specified logical name.
 *****************************************************************/
int _load_from_logical_name(void *env, char *readSource)
{
    int constructFlag;
    struct token theToken;
    int noErrors = TRUE;
    int foundConstruct;

    /*=========================================
     * Reset the halt execution and evaluation
     * error flags in preparation for parsing.
     *=========================================*/

    if( core_get_evaluation_data(env)->eval_depth == 0 )
    {
        core_set_halt_eval(env, FALSE);
    }

    core_set_eval_error(env, FALSE);

    /*========================================================
     * Find the beginning of the first construct in the file.
     *========================================================*/

    core_get_evaluation_data(env)->eval_depth++;
    core_get_token(env, readSource, &theToken);
    foundConstruct = _find_construct_start(env, readSource, &theToken, FALSE, &noErrors);

    /*==================================================
     * Parse the file until the end of file is reached.
     *==================================================*/

    while((foundConstruct == TRUE) && (core_get_halt_eval(env) == FALSE))
    {
        /*===========================================================
         * Clear the pretty print buffer in preparation for parsing.
         *===========================================================*/

        core_flush_pp_buffer(env);

        /*======================
         * Parse the construct.
         *======================*/

        constructFlag = parse_construct(env, to_string(theToken.value), readSource);

        /*==============================================================
         * If an error occurred while parsing, then find the beginning
         * of the next construct (but don't generate any more error
         * messages--in effect, skip everything until another construct
         * is found).
         *==============================================================*/

        if( constructFlag == 1 )
        {
            print_router(env, WERROR, "\nERROR:\n");
            core_print_chunkify(env, WERROR, core_get_pp_buffer_handle(env));
            print_router(env, WERROR, "\n");
            noErrors = FALSE;
            core_get_token(env, readSource, &theToken);
            foundConstruct = _find_construct_start(env, readSource, &theToken, TRUE, &noErrors);
        }

        /*======================================================
         * Otherwise, find the beginning of the next construct.
         *======================================================*/

        else
        {
            core_get_token(env, readSource, &theToken);
            foundConstruct = _find_construct_start(env, readSource, &theToken, FALSE, &noErrors);
        }

        /*=====================================================
         * Yield time if necessary to foreground applications.
         *=====================================================*/

        if( foundConstruct )
        {
            inc_atom_count(theToken.value);
        }

        core_get_evaluation_data(env)->eval_depth--;
        core_gc_periodic_cleanup(env, FALSE, TRUE);
        core_gc_yield_time(env);
        core_get_evaluation_data(env)->eval_depth++;

        if( foundConstruct )
        {
            dec_atom_count(env, (ATOM_HN *)theToken.value);
        }
    }

    core_get_evaluation_data(env)->eval_depth--;

    /*========================================================
     * Print a carriage return if a single character is being
     * printed to indicate constructs are being processed.
     *========================================================*/

#if DEBUGGING_FUNCTIONS

    if((EnvGetWatchItem(env, "compilations") != TRUE) && core_is_verbose_loading(env))
#else

    if( core_is_verbose_loading(env))
#endif
    {print_router(env, WDIALOG, "\n");}

    /*=============================================================
     * Once the load is complete, destroy the pretty print buffer.
     * This frees up any memory that was used to create the pretty
     * print forms for constructs during parsing. Thus calls to
     * the mem-used function will accurately reflect the amount of
     * memory being used after a load command.
     *=============================================================*/

    core_delete_pp_buffer(env);

    /*==========================================================
     * Return a boolean flag which indicates whether any errors
     * were encountered while loading the constructs.
     *==========================================================*/

    return(noErrors);
}

/*******************************************************************
 * FindConstructBeginning: Searches for a left parenthesis followed
 *   by the name of a valid construct. Used by the load command to
 *   find the next construct to be parsed. Returns TRUE is the
 *   beginning of a construct was found, otherwise FALSE.
 ********************************************************************/
static int _find_construct_start(void *env, char *readSource, struct token *theToken, int errorCorrection, int *noErrors)
{
    int leftParenthesisFound = FALSE;
    int firstAttempt = TRUE;

    /*===================================================
     * Process tokens until the beginning of a construct
     * is found or there are no more tokens.
     *===================================================*/

    while( theToken->type != STOP )
    {
        /*=====================================================
         * Constructs begin with a left parenthesis. Make note
         * that the opening parenthesis has been found.
         *=====================================================*/

        if( theToken->type == LPAREN )
        {
            leftParenthesisFound = TRUE;
        }

        /*=================================================================
         * The name of the construct follows the opening left parenthesis.
         * If it is the name of a valid construct, then return TRUE.
         * Otherwise, reset the flags to look for the beginning of a
         * construct. If error correction is being performed (i.e. the
         * last construct parsed had an error in it), then don't bother to
         * print an error message, otherwise, print an error message.
         *=================================================================*/

        else if((theToken->type == ATOM) && (leftParenthesisFound == TRUE))
        {
            /*===========================================================
             * Is this a valid construct name
             *===========================================================*/

            if( core_lookup_construct_by_name(env, to_string(theToken->value)) != NULL )
            {
                return(TRUE);
            }

            /*===============================================
             * The construct name is invalid. Print an error
             * message if one hasn't already been printed.
             *===============================================*/

            if( firstAttempt && (!errorCorrection))
            {
                errorCorrection = TRUE;
                *noErrors = FALSE;
                error_print_id(env, "PARSER", 1, TRUE);
                print_router(env, WERROR, "Expected the beginning of a construct.\n");
            }

            /*======================================================
             * Indicate that an error has been found and that we're
             * looking for a left parenthesis again.
             *======================================================*/

            firstAttempt = FALSE;
            leftParenthesisFound = FALSE;
        }

        /*====================================================================
         * Any token encountered other than a left parenthesis or a construct
         * name following a left parenthesis is illegal. Again, if error
         * correction is in progress, no error message is printed, otherwise,
         *  an error message is printed.
         *====================================================================*/

        else
        {
            if( firstAttempt && (!errorCorrection))
            {
                errorCorrection = TRUE;
                *noErrors = FALSE;
                error_print_id(env, "PARSER", 1, TRUE);
                print_router(env, WERROR, "Expected the beginning of a construct.\n");
            }

            firstAttempt = FALSE;
            leftParenthesisFound = FALSE;
        }

        /*============================================
         * Move on to the next token to be processed.
         *============================================*/

        core_get_token(env, readSource, theToken);
    }

    /*===================================================================
     * Couldn't find the beginning of a construct, so FALSE is returned.
     *===================================================================*/

    return(FALSE);
}

/**********************************************************
 * parse_construct: Parses a construct. Returns an integer.
 *   -1 if the construct name has no parsing function, 0
 *   if the construct was parsed successfully, and 1 if
 *   the construct was parsed unsuccessfully.
 ***********************************************************/
int parse_construct(void *env, char *name, char *logicalName)
{
    struct construct *currentPtr;
    int rv, ov;

    /*=================================
     * Look for a valid construct name
     *=================================*/

    currentPtr = core_lookup_construct_by_name(env, name);

    if( currentPtr == NULL )
    {
        return(-1);
    }

    /*==================================
     * Prepare the parsing environment.
     *==================================*/

    ov = core_get_halt_eval(env);
    core_set_eval_error(env, FALSE);
    core_set_halt_eval(env, FALSE);
    clear_parsed_bindings(env);
    push_break_contexts(env);
    core_get_expression_data(env)->return_context = FALSE;
    core_get_expression_data(env)->break_context = FALSE;
    core_get_evaluation_data(env)->eval_depth++;

    /*=======================================
     * Call the construct's parsing routine.
     *=======================================*/

    core_construct_data(env)->parsing = TRUE;
    rv = (*currentPtr->parser)(env, logicalName);
    core_construct_data(env)->parsing = FALSE;

    /*===============================
     * Restore environment settings.
     *===============================*/

    core_get_evaluation_data(env)->eval_depth--;
    pop_break_contexts(env);

    clear_parsed_bindings(env);
    core_set_pp_buffer_status(env, OFF);
    core_set_halt_eval(env, ov);

    /*==============================
     * Return the status of parsing
     * the construct.
     *==============================*/

    return(rv);
}

/********************************************************
 * parse_construct_head: Get the name and comment
 *   field of a construct. Returns name of the construct
 *   if no errors are detected, otherwise returns NULL.
 *********************************************************/
#if WIN_BTC && (!DEBUGGING_FUNCTIONS)
#pragma argsused
#endif
ATOM_HN *parse_construct_head(void *env, char *readSource, struct token *inputToken, char *constructName, void *(*findFunction)(void *, char *), int (*deleteFunction)(void *, void *), char *constructSymbol, int fullMessageCR, int getComment, int moduleNameAllowed)
{
#if (MAC_MCW || WIN_MCW || MAC_XCD) && (!DEBUGGING_FUNCTIONS)
#pragma unused(fullMessageCR)
#endif
    ATOM_HN *name, *moduleName;
    int redefining = FALSE;
    void *cons;
    unsigned separatorPosition;
    struct module_definition *theModule;

    /*==========================
     * Next token should be the
     * name of the construct.
     *==========================*/

    core_get_token(env, readSource, inputToken);

    if( inputToken->type != ATOM )
    {
        error_print_id(env, "PARSER", 2, TRUE);
        print_router(env, WERROR, "Missing name for ");
        print_router(env, WERROR, constructName);
        print_router(env, WERROR, " construct\n");
        return(NULL);
    }

    name = (ATOM_HN *)inputToken->value;

    /*===============================
     * Determine the current module.
     *===============================*/

    separatorPosition = find_module_separator(to_string(name));

    if( separatorPosition )
    {
        if( moduleNameAllowed == FALSE )
        {
            error_syntax(env, "module specifier");
            return(NULL);
        }

        moduleName = garner_module_name(env, separatorPosition, to_string(name));

        if( moduleName == NULL )
        {
            error_syntax(env, "construct name");
            return(NULL);
        }

        theModule = (struct module_definition *)lookup_module(env, to_string(moduleName));

        if( theModule == NULL )
        {
            error_lookup(env, "defmodule", to_string(moduleName));
            return(NULL);
        }

        set_current_module(env, (void *)theModule);
        name = garner_construct_name(env, separatorPosition, to_string(name));

        if( name == NULL )
        {
            error_syntax(env, "construct name");
            return(NULL);
        }
    }

    /*=====================================================
     * If the module was not specified, record the current
     * module name as part of the pretty-print form.
     *=====================================================*/

    else
    {
        theModule = ((struct module_definition *)get_current_module(env));

        if( moduleNameAllowed )
        {
            core_backup_pp_buffer(env);
            core_save_pp_buffer(env, get_module_name(env, theModule));
            core_save_pp_buffer(env, "::");
            core_save_pp_buffer(env, to_string(name));
        }
    }

    /*==================================================================
     * Check for import/export conflicts from the construct definition.
     *==================================================================*/

#if DEFMODULE_CONSTRUCT

    if( FindImportExportConflict(env, construct_name, module_def, to_string(name)))
    {
        error_import_export_conflict(env, construct_name, to_string(name), NULL, NULL);
        return(NULL);
    }

#endif

    /*========================================================
     * Remove the construct if it is already in the knowledge
     * base and we're not just checking syntax.
     *========================================================*/

    if((findFunction != NULL) && (!core_construct_data(env)->check_syntax))
    {
        cons = (*findFunction)(env, to_string(name));

        if( cons != NULL )
        {
            redefining = TRUE;

            if( deleteFunction != NULL )
            {
                if((*deleteFunction)(env, cons) == FALSE )
                {
                    error_print_id(env, "PARSER", 4, TRUE);
                    print_router(env, WERROR, "Cannot redefine ");
                    print_router(env, WERROR, constructName);
                    print_router(env, WERROR, " ");
                    print_router(env, WERROR, to_string(name));
                    print_router(env, WERROR, " while it is in use.\n");
                    return(NULL);
                }
            }
        }
    }

    /*=============================================
     * If compilations are being watched, indicate
     * that a construct is being compiled.
     *=============================================*/

#if DEBUGGING_FUNCTIONS

    if((EnvGetWatchItem(env, "compilations") == TRUE) &&
       core_is_verbose_loading(env) && (!core_construct_data(env)->check_syntax))
    {
        if( redefining )
        {
            warning_print_id(env, "PARSER", 1, TRUE);
            print_router(env, WDIALOG, "Redefining ");
        }
        else
        {
            print_router(env, WDIALOG, "Defining ");
        }

        print_router(env, WDIALOG, construct_name);
        print_router(env, WDIALOG, ": ");
        print_router(env, WDIALOG, to_string(name));

        if( fullMessageCR )
        {
            print_router(env, WDIALOG, "\n");
        }
        else
        {
            print_router(env, WDIALOG, " ");
        }
    }
    else
#endif
    {
        if( core_is_verbose_loading(env) && (!core_construct_data(env)->check_syntax))
        {
            print_router(env, WDIALOG, constructSymbol);
        }
    }

    /*===============================
     * Get the comment if it exists.
     *===============================*/

    core_get_token(env, readSource, inputToken);

    if((inputToken->type == STRING) && getComment )
    {
        core_backup_pp_buffer(env);
        core_save_pp_buffer(env, " ");
        core_save_pp_buffer(env, inputToken->pp);
        //printf( "DOC: %s\n", inputToken->pp);

        core_get_token(env, readSource, inputToken);

        if( inputToken->type != RPAREN )
        {
            core_backup_pp_buffer(env);
            core_save_pp_buffer(env, "\n   ");
            core_save_pp_buffer(env, inputToken->pp);
        }
    }
    else if( inputToken->type != RPAREN )
    {
        core_backup_pp_buffer(env);
        core_save_pp_buffer(env, "\n   ");
        core_save_pp_buffer(env, inputToken->pp);
    }

    /*===================================
     * Return the name of the construct.
     *===================================*/

    return(name);
}

/***************************************
 * undefine_module_construct: Removes a
 *   construct from its module's list
 ****************************************/
void undefine_module_construct(void *env, struct construct_metadata *cons)
{
    struct construct_metadata *lastConstruct, *currentConstruct;

    /*==============================
     * Find the specified construct
     * in the module's list.
     *==============================*/

    lastConstruct = NULL;
    currentConstruct = cons->my_module->first_item;

    while( currentConstruct != cons )
    {
        lastConstruct = currentConstruct;
        currentConstruct = currentConstruct->next;
    }

    /*========================================
     * If it wasn't there, something's wrong.
     *========================================*/

    if( currentConstruct == NULL )
    {
        error_system(env, "PARSER", 1);
        exit_router(env, EXIT_FAILURE);
    }

    /*==========================
     * Remove it from the list.
     *==========================*/

    if( lastConstruct == NULL )
    {
        cons->my_module->first_item = cons->next;
    }
    else
    {
        lastConstruct->next = cons->next;
    }

    /*=================================================
     * Update the pointer to the last item in the list
     * if the construct just deleted was at the end.
     *=================================================*/

    if( cons == cons->my_module->last_item )
    {
        cons->my_module->last_item = lastConstruct;
    }
}

/*****************************************************
 * error_import_export_conflict: Generic error message
 *   for an import/export module conflict detected
 *   when a construct is being defined.
 ******************************************************/
void error_import_export_conflict(void *env, char *constructName, char *itemName, char *causedByConstruct, char *causedByName)
{
    error_print_id(env, "PARSER", 3, TRUE);
    print_router(env, WERROR, "Cannot define ");
    print_router(env, WERROR, constructName);
    print_router(env, WERROR, " ");
    print_router(env, WERROR, itemName);
    print_router(env, WERROR, " because of an import/export conflict");

    if( causedByConstruct == NULL )
    {
        print_router(env, WERROR, ".\n");
    }
    else
    {
        print_router(env, WERROR, " caused by the ");
        print_router(env, WERROR, causedByConstruct);
        print_router(env, WERROR, " ");
        print_router(env, WERROR, causedByName);
        print_router(env, WERROR, ".\n");
    }
}
