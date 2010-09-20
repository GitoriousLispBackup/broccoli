/* Purpose: Deffunction Parsing Routines                     */

/* =========================================
 *****************************************
 *            EXTERNAL DEFINITIONS
 *  =========================================
 ***************************************** */
#include "setup.h"

#if DEFFUNCTION_CONSTRUCT


#if DEFGENERIC_CONSTRUCT
#include "generics_kernel.h"
#endif

#include "constant.h"
#include "parser_constraints.h"
#include "parser_constructs.h"
#include "core_constructs.h"
#include "funcs_function.h"
#include "core_environment.h"
#include "core_expressions.h"
#include "parser_expressions.h"
#include "core_functions.h"
#include "core_memory.h"
#include "core_functions_util.h"
#include "router.h"
#include "core_scanner.h"
#include "type_symbol.h"

#define __PARSER_FUNCTIONS_SOURCE__
#include "parser_functions.h"

/* =========================================
 *****************************************
 *   INTERNALLY VISIBLE FUNCTION HEADERS
 *  =========================================
 ***************************************** */

static BOOLEAN               _is_valid_function_name(void *, char *);
static FUNCTION_DEFINITION * _add_function(void *, ATOM_HN *, core_expression_object *, int, int, int, int);

/* =========================================
 *****************************************
 *       EXTERNALLY VISIBLE FUNCTIONS
 *  =========================================
 ***************************************** */

/***************************************************************************
 *  NAME         : parse_function
 *  DESCRIPTION  : Parses the deffunction construct
 *  INPUTS       : The input logical name
 *  RETURNS      : FALSE if successful parse, TRUE otherwise
 *  SIDE EFFECTS : Creates valid deffunction definition
 *  NOTES        : H/L Syntax :
 *              (deffunction <name> [<comment>]
 *                 (<single-field-varible>* [<list-variable>])
 *                 <action>*)
 ***************************************************************************/
BOOLEAN parse_function(void *env, char *readSource)
{
    ATOM_HN *deffunctionName;
    core_expression_object *actions;
    core_expression_object *parameterList;
    ATOM_HN *wildcard;
    int min, max, lvars, DeffunctionError = FALSE;
    short overwrite = FALSE, owMin = 0, owMax = 0;
    FUNCTION_DEFINITION *dptr;

    core_set_pp_buffer_status(env, ON);

    core_flush_pp_buffer(env);
    core_pp_indent_depth(env, 3);
    core_save_pp_buffer(env, "(deffunction ");


    /* =====================================================
     *  Parse the name and comment fields of the deffunction.
     *  ===================================================== */
    deffunctionName = parse_construct_head(env, readSource, &get_function_data(env)->input_token, FUNC_NAME_CREATE_FUNC,
                                           lookup_function, NULL,
                                           "!", TRUE, TRUE, TRUE);

    if( deffunctionName == NULL )
    {
        return(TRUE);
    }

    if( _is_valid_function_name(env, to_string(deffunctionName)) == FALSE )
    {
        return(TRUE);
    }

    /*==========================
     * Parse the argument list.
     *==========================*/
    parameterList = core_parse_function_args(env, readSource, &get_function_data(env)->input_token, NULL, &wildcard,
                                             &min, &max, &DeffunctionError, NULL);

    if( DeffunctionError )
    {
        return(TRUE);
    }

    /*===================================================================
     * Go ahead and add the deffunction so it can be recursively called.
     *===================================================================*/

    if( core_construct_data(env)->check_syntax )
    {
        dptr = (FUNCTION_DEFINITION *)lookup_function(env, to_string(deffunctionName));

        if( dptr == NULL )
        {
            dptr = _add_function(env, deffunctionName, NULL, min, max, 0, TRUE);
        }
        else
        {
            overwrite = TRUE;
            owMin = (short)dptr->min_args;
            owMax = (short)dptr->max_args;
            dptr->min_args = min;
            dptr->max_args = max;
        }
    }
    else
    {
        dptr = _add_function(env, deffunctionName, NULL, min, max, 0, TRUE);
    }

    if( dptr == NULL )
    {
        core_return_expression(env, parameterList);
        return(TRUE);
    }

    /*==================================================
     * Parse the actions contained within the function.
     *==================================================*/

    core_inject_ws_into_pp_buffer(env);

    core_get_expression_data(env)->return_context = TRUE;
    actions = core_parse_function_actions(env, FUNC_NAME_CREATE_FUNC, readSource,
                                          &get_function_data(env)->input_token, parameterList, wildcard,
                                          NULL, NULL, &lvars, NULL);

    /*=============================================================
     * Check for the closing right parenthesis of the deffunction.
     *=============================================================*/

    if((get_function_data(env)->input_token.type != RPAREN) && /* DR0872 */
       (actions != NULL))
    {
        error_syntax(env, FUNC_NAME_CREATE_FUNC);

        core_return_expression(env, parameterList);
        core_return_packed_expression(env, actions);

        if( overwrite )
        {
            dptr->min_args = owMin;
            dptr->max_args = owMax;
        }

        if((dptr->busy == 0) && (!overwrite))
        {
            undefine_module_construct(env, (struct construct_metadata *)dptr);
            remove_function(env, dptr);
        }

        return(TRUE);
    }

    if( actions == NULL )
    {
        core_return_expression(env, parameterList);

        if( overwrite )
        {
            dptr->min_args = owMin;
            dptr->max_args = owMax;
        }

        if((dptr->busy == 0) && (!overwrite))
        {
            undefine_module_construct(env, (struct construct_metadata *)dptr);
            remove_function(env, dptr);
        }

        return(TRUE);
    }

    /*==============================================
     * If we're only checking syntax, don't add the
     * successfully parsed deffunction to the KB.
     *==============================================*/

    if( core_construct_data(env)->check_syntax )
    {
        core_return_expression(env, parameterList);
        core_return_packed_expression(env, actions);

        if( overwrite )
        {
            dptr->min_args = owMin;
            dptr->max_args = owMax;
        }
        else
        {
            undefine_module_construct(env, (struct construct_metadata *)dptr);
            remove_function(env, dptr);
        }

        return(FALSE);
    }

    /*=============================
     * Reformat the closing token.
     *=============================*/

    core_backup_pp_buffer(env);
    core_backup_pp_buffer(env);
    core_save_pp_buffer(env, get_function_data(env)->input_token.pp);
    core_save_pp_buffer(env, "\n");

    /*======================
     * Add the deffunction.
     *======================*/

    _add_function(env, deffunctionName, actions, min, max, lvars, FALSE);

    core_return_expression(env, parameterList);

    return(DeffunctionError);
}

/* =========================================
 *****************************************
 *       INTERNALLY VISIBLE FUNCTIONS
 *  =========================================
 ***************************************** */

/************************************************************
 *  NAME         : ValidDeffunctionName
 *  DESCRIPTION  : Determines if a new deffunction of the given
 *              name can be defined in the current module
 *  INPUTS       : The new deffunction name
 *  RETURNS      : TRUE if OK, FALSE otherwise
 *  SIDE EFFECTS : Error message printed if not OK
 *  NOTES        : parse_construct_head() (called before
 *              this function) ensures that the deffunction
 *              name does not conflict with one from
 *              another module
 ************************************************************/
static BOOLEAN _is_valid_function_name(void *env, char *theDeffunctionName)
{
    struct construct_metadata *theDeffunction;

#if DEFGENERIC_CONSTRUCT
    struct module_definition *module_def;
    struct construct_metadata *theDefgeneric;
#endif

    /* ============================================
     *  A deffunction cannot be named the same as a
     *  construct type
     *  ============================================ */
    if( core_lookup_construct_by_name(env, theDeffunctionName) != NULL )
    {
        error_print_id(env, "PARSER", 1, FALSE);
        print_router(env, WERROR, "Deffunctions are not allowed to replace constructs.\n");
        return(FALSE);
    }

    /* ============================================
     *  A deffunction cannot be named the same as a
     *  pre-defined system function, e.g, watch
     *  ============================================ */
    if( core_lookup_function(env, theDeffunctionName) != NULL )
    {
        error_print_id(env, "PARSER", 2, FALSE);
        print_router(env, WERROR, "Deffunctions are not allowed to replace external functions.\n");
        return(FALSE);
    }

#if DEFGENERIC_CONSTRUCT

    /* ============================================
     *  A deffunction cannot be named the same as a
     *  generic function (either in this module or
     *  imported from another)
     *  ============================================ */
    theDefgeneric =
        (struct construct_metadata *)LookupDefgenericInScope(env, theDeffunctionName);

    if( theDefgeneric != NULL )
    {
        module_def = core_query_module_header(theDefgeneric)->module_def;

        if( module_def != ((struct module_definition *)get_current_module(env)))
        {
            error_print_id(env, "PARSER", 5, FALSE);
            print_router(env, WERROR, "Defgeneric ");
            print_router(env, WERROR, EnvGetDefgenericName(env, (void *)theDefgeneric));
            print_router(env, WERROR, " imported from module ");
            print_router(env, WERROR, get_module_name(env, (void *)module_def));
            print_router(env, WERROR, " conflicts with this deffunction.\n");
            return(FALSE);
        }
        else
        {
            error_print_id(env, "PARSER", 3, FALSE);
            print_router(env, WERROR, "Deffunctions are not allowed to replace generic functions.\n");
        }

        return(FALSE);
    }

#endif

    theDeffunction = (struct construct_metadata *)lookup_function(env, theDeffunctionName);

    if( theDeffunction != NULL )
    {
        /* ===========================================
         *  And a deffunction in the current module can
         *  only be redefined if it is not executing.
         *  =========================================== */
        if(((FUNCTION_DEFINITION *)theDeffunction)->executing )
        {
            error_print_id(env, "PARSER", 4, FALSE);
            print_router(env, WERROR, "Deffunction ");
            print_router(env, WERROR, get_function_name(env, (void *)theDeffunction));
            print_router(env, WERROR, " may not be redefined while it is executing.\n");
            return(FALSE);
        }
    }

    return(TRUE);
}

/****************************************************
 *  NAME         : AddDeffunction
 *  DESCRIPTION  : Adds a deffunction to the list of
 *              deffunctions
 *  INPUTS       : 1) The symbolic name
 *              2) The action expressions
 *              3) The minimum number of arguments
 *              4) The maximum number of arguments
 *                 (can be -1)
 *              5) The number of local variables
 *              6) A flag indicating if this is
 *                 a header call so that the
 *                 deffunction can be recursively
 *                 called
 *  RETURNS      : The new deffunction (NULL on errors)
 *  SIDE EFFECTS : Deffunction structures allocated
 *  NOTES        : Assumes deffunction is not executing
 ****************************************************/
#if WIN_BTC
#pragma argsused
#endif
static FUNCTION_DEFINITION *_add_function(void *env, ATOM_HN *name, core_expression_object *actions, int min, int max, int lvars, int headerp)
{
    FUNCTION_DEFINITION *dfuncPtr;
    unsigned oldbusy;

#if DEBUGGING_FUNCTIONS
    unsigned DFHadWatch = FALSE;
#else
#if MAC_MCW || WIN_MCW || MAC_XCD
#pragma unused(headerp)
#endif
#endif

    /*===============================================================
     * If the deffunction doesn't exist, create a new structure to
     * contain it and add it to the List of deffunctions. Otherwise,
     * use the existing structure and remove the pretty print form
     * and interpretive code.
     *===============================================================*/
    dfuncPtr = (FUNCTION_DEFINITION *)lookup_function(env, to_string(name));

    if( dfuncPtr == NULL )
    {
        dfuncPtr = core_mem_get_struct(env, function_definition);
        core_init_construct_header(env, FUNC_NAME_CREATE_FUNC, (struct construct_metadata *)dfuncPtr, name);
        inc_atom_count(name);
        dfuncPtr->code = NULL;
        dfuncPtr->min_args = min;
        dfuncPtr->max_args = max;
        dfuncPtr->local_variable_count = lvars;
        dfuncPtr->busy = 0;
        dfuncPtr->executing = 0;
    }
    else
    {
#if DEBUGGING_FUNCTIONS
        DFHadWatch = EnvGetDeffunctionWatch(env, (void *)dfuncPtr);
#endif
        dfuncPtr->min_args = min;
        dfuncPtr->max_args = max;
        dfuncPtr->local_variable_count = lvars;
        oldbusy = dfuncPtr->busy;
        core_decrement_expression(env, dfuncPtr->code);
        dfuncPtr->busy = oldbusy;
        core_return_packed_expression(env, dfuncPtr->code);
        dfuncPtr->code = NULL;
        set_function_pp((void *)dfuncPtr, NULL);

        /* =======================================
         *  Remove the deffunction from the list so
         *  that it can be added at the end
         *  ======================================= */
        undefine_module_construct(env, (struct construct_metadata *)dfuncPtr);
    }

    core_add_to_module((struct construct_metadata *)dfuncPtr);

    /* ==================================
     *  Install the new interpretive code.
     *  ================================== */

    if( actions != NULL )
    {
        /* ===============================
         *  If a deffunction is recursive,
         *  do not increment its busy count
         *  based on self-references
         *  =============================== */
        oldbusy = dfuncPtr->busy;
        core_increment_expression(env, actions);
        dfuncPtr->busy = oldbusy;
        dfuncPtr->code = actions;
    }

    /* ===============================================================
     *  Install the pretty print form if memory is not being conserved.
     *  =============================================================== */

#if DEBUGGING_FUNCTIONS
    EnvSetDeffunctionWatch(env, DFHadWatch ? TRUE : get_function_data(env)->is_watching, (void *)dfuncPtr);

    if((EnvGetConserveMemory(env) == FALSE) && (headerp == FALSE))
    {
        set_function_pp((void *)dfuncPtr, core_copy_pp_buffer(env));
    }

#endif
    return(dfuncPtr);
}

#endif
