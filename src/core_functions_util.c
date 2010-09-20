/* Purpose: Procedural Code Support Routines for Deffunctions,
 *          Generic Function Methods,Message-Handlers
 *          and Rules                                          */

/* =========================================
 *****************************************
 *            EXTERNAL DEFINITIONS
 *  =========================================
 ***************************************** */
#include "setup.h"

#ifndef _STDIO_INCLUDED_
#include <stdio.h>
#define _STDIO_INCLUDED_
#endif

#include <stdlib.h>
#include <ctype.h>

#include "core_memory.h"
#include "constant.h"
#include "core_environment.h"
#include "parser_expressions.h"
#include "type_list.h"
#if OBJECT_SYSTEM
#include "classes.h"
#endif
#include "parser_flow_control.h"
#include "router.h"
#include "core_gc.h"

#define __CORE_WILDCARDS_SOURCE__
#include "core_functions_util.h"

/* =========================================
 *****************************************
 *            MACROS AND TYPES
 *  =========================================
 ***************************************** */
typedef struct
{
    unsigned firstFlag  :
    1;
    unsigned first      :
    15;
    unsigned secondFlag :
    1;
    unsigned second     :
    15;
} PACKED_FUNCTION_VARIABLE;

/* =========================================
 *****************************************
 *   INTERNALLY VISIBLE FUNCTION HEADERS
 *  =========================================
 ***************************************** */

static void    _eval_function_args(void *, core_expression_object *, int, char *, char *);
static BOOLEAN _get_function_arg(void *, void *, core_data_object *);
static BOOLEAN _lookup_function_binding(void *, void *, core_data_object *);
static BOOLEAN _set_function_binding(void *, void *, core_data_object *);
static BOOLEAN _get_function_optional_arg(void *, void *, core_data_object *);
static void    _delete_function_primitive_data(void *);
static void    _release_function_args(void *);

static int _lookup_function_arg(ATOM_HN *, core_expression_object *, ATOM_HN *);
static int _release_function_binding(void *, core_expression_object *, int(*) (void *, core_expression_object *, void *), void *);
static core_expression_object * _pack_function_actions(void *, core_expression_object *);
static BOOLEAN                  _dummy_function_call(void *, void *, core_data_object *);

/* =========================================
 *****************************************
 *       EXTERNALLY VISIBLE FUNCTIONS
 *  =========================================
 ***************************************** */

/****************************************************
 *  NAME         : core_install_function_primitive
 *  DESCRIPTION  : Installs primitive function handlers
 *              for accessing parameters and local
 *              variables within the bodies of
 *              message-handlers, methods, rules and
 *              deffunctions.
 *  INPUTS       : None
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Primitive entities installed
 *  NOTES        : None
 ****************************************************/
void core_install_function_primitive(void *env)
{
    core_data_entity_object procParameterInfo = {"FUNCTION_ARG",    FUNCTION_ARG, 0,            1,            0,          NULL,          NULL,    NULL,
                                                 _get_function_arg, NULL,         NULL,         NULL,         NULL,       NULL,          NULL,    NULL,    NULL},
                            procWildInfo =      {"FUNCTION_OPTIONAL_ARG",    FUNCTION_OPTIONAL_ARG, 0,                     1,                     0,               NULL,               NULL,    NULL,
                                                 _get_function_optional_arg, NULL,                  NULL,                  NULL,                  NULL,            NULL,               NULL,    NULL,    NULL},
                            procGetInfo =       {"FUNCTION_VAR_META",      FUNCTION_VAR_META, 0,                 1,                 0,             NULL,             NULL,    NULL,
                                                 _lookup_function_binding, NULL,              NULL,              NULL,              NULL,          NULL,             NULL,    NULL,    NULL},
                            procBindInfo =      {"FUNCTION_VAR",        FUNCTION_VAR, 0,            1,            0,         NULL,         NULL,    NULL,
                                                 _set_function_binding, NULL,         NULL,         NULL,         NULL,      NULL,         NULL,    NULL,    NULL};

#if !DEFFUNCTION_CONSTRUCT
    core_data_entity_object deffunctionEntityRecord =
    {"PCALL",              PCALL,           0,               0,               1,
     NULL,                 NULL,            NULL,
     _dummy_function_call,
     NULL,                 NULL,            NULL,            NULL,            NULL,              NULL,NULL, NULL};
#endif
#if !DEFGENERIC_CONSTRUCT
    core_data_entity_object genericEntityRecord =
    {"GCALL",              GCALL,           0,               0,               1,
     NULL,                 NULL,            NULL,
     _dummy_function_call,
     NULL,                 NULL,            NULL,            NULL,            NULL,              NULL,NULL, NULL};
#endif

    core_allocate_environment_data(env, PROCEDURAL_PRIMITIVE_DATA_INDEX, sizeof(struct core_function_primitive_data), _delete_function_primitive_data);

    memcpy(&core_get_function_primitive_data(env)->function_arg_info, &procParameterInfo, sizeof(struct core_data_entity));
    memcpy(&core_get_function_primitive_data(env)->function_optional_arg_info, &procWildInfo, sizeof(struct core_data_entity));
    memcpy(&core_get_function_primitive_data(env)->function_get_info, &procGetInfo, sizeof(struct core_data_entity));
    memcpy(&core_get_function_primitive_data(env)->function_bind_info, &procBindInfo, sizeof(struct core_data_entity));

    core_install_primitive(env, &core_get_function_primitive_data(env)->function_arg_info, FUNCTION_ARG);
    core_install_primitive(env, &core_get_function_primitive_data(env)->function_optional_arg_info, FUNCTION_OPTIONAL_ARG);
    core_install_primitive(env, &core_get_function_primitive_data(env)->function_get_info, FUNCTION_VAR_META);
    core_install_primitive(env, &core_get_function_primitive_data(env)->function_bind_info, FUNCTION_VAR);

    core_get_function_primitive_data(env)->old_index = -1;

    /* ===============================================
     *  Make sure a default evaluation function is
     *  in place for deffunctions and generic functions
     *  in the event that a binary image containing
     *  these items is loaded into a configuration
     *  that does not support them.
     *  =============================================== */

#if !DEFFUNCTION_CONSTRUCT
    memcpy(&core_get_function_primitive_data(env)->entity_record, &deffunctionEntityRecord, sizeof(struct core_data_entity));
    core_install_primitive(env, &core_get_function_primitive_data(env)->entity_record, PCALL);
#endif

#if !DEFGENERIC_CONSTRUCT
    memcpy(&core_get_function_primitive_data(env)->generic_info, &genericEntityRecord, sizeof(struct core_data_entity));
    core_install_primitive(env, &core_get_function_primitive_data(env)->generic_info, GCALL);
#endif

    /* =============================================
     *  Install the special empty list to
     *  let callers distinguish between no parameters
     *  and zero-length list parameters
     *  ============================================= */
    core_get_function_primitive_data(env)->null_arg_value = create_sized_list(env, 0L);
    install_list(env, (LIST_PTR)core_get_function_primitive_data(env)->null_arg_value);
}

/*************************************************************
 * DeallocateProceduralPrimitiveData: Deallocates environment
 *    data for the procedural primitives functionality.
 **************************************************************/
static void _delete_function_primitive_data(void *env)
{
    release_list(env, (struct list *)core_get_function_primitive_data(env)->null_arg_value);
    _release_function_args(env);
}

#if DEFFUNCTION_CONSTRUCT || OBJECT_SYSTEM

/************************************************************
 *  NAME         : core_parse_function_args
 *  DESCRIPTION  : Parses a parameter list for a
 *               procedural routine, such as a
 *               deffunction or message-handler
 *  INPUTS       : 1) The logical name of the input
 *              2) A buffer for scanned tokens
 *              3) The partial list of parameters so far
 *                 (can be NULL)
 *              3) A buffer for a wildcard symbol (if any)
 *              4) A buffer for a minimum of parameters
 *              5) A buffer for a maximum of parameters
 *                 (will be set to -1 if there is a wilcard)
 *              6) A buffer for an error flag
 *              7) The address of a function to do specialized
 *                 checking on a parameter (can be NULL)
 *                 The function should accept a string and
 *                 return FALSE if the parameter is OK, TRUE
 *                 otherwise.
 *  RETURNS      : A list of expressions containing the
 *                parameter names
 *  SIDE EFFECTS : Parameters parsed and expressions formed
 *  NOTES        : None
 ************************************************************/
core_expression_object *core_parse_function_args(void *env, char *readSource, struct token *tkn, core_expression_object *parameterList, ATOM_HN **wildcard, int *min, int *max, int *error, int (*checkfunc)(void *, char *))
{
    core_expression_object *nextOne, *lastOne, *check;
    int paramprintp = 0;

    *wildcard = NULL;
    *min = 0;
    *error = TRUE;
    lastOne = nextOne = parameterList;

    while( nextOne != NULL )
    {
        (*min)++;
        lastOne = nextOne;
        nextOne = nextOne->next_arg;
    }

    if( tkn->type != LPAREN )
    {
        error_syntax(env, "parameter list");
        core_return_expression(env, parameterList);
        return(NULL);
    }

    core_get_token(env, readSource, tkn);

    while((tkn->type == SCALAR_VARIABLE) || (tkn->type == LIST_VARIABLE))
    {
        for( check = parameterList ; check != NULL ; check = check->next_arg )
        {
            if( check->value == tkn->value )
            {
                error_print_id(env, "FLOW", 7, FALSE);
                print_router(env, WERROR, "Duplicate parameter names not allowed.\n");
                core_return_expression(env, parameterList);
                return(NULL);
            }
        }

        if( *wildcard != NULL )
        {
            error_print_id(env, "FLOW", 8, FALSE);
            print_router(env, WERROR, "No parameters allowed after wildcard parameter.\n");
            core_return_expression(env, parameterList);
            return(NULL);
        }

        if((checkfunc != NULL) ? (*checkfunc)(env, to_string(tkn->value)) : FALSE )
        {
            core_return_expression(env, parameterList);
            return(NULL);
        }

        nextOne = core_generate_constant(env, tkn->type, tkn->value);

        if( tkn->type == LIST_VARIABLE )
        {
            *wildcard = (ATOM_HN *)tkn->value;
        }
        else
        {
            (*min)++;
        }

        if( lastOne == NULL )
        {
            parameterList = nextOne;
        }
        else
        {
            lastOne->next_arg = nextOne;
        }

        lastOne = nextOne;
        core_save_pp_buffer(env, " ");
        paramprintp = 1;
        core_get_token(env, readSource, tkn);
    }

    if( tkn->type != RPAREN )
    {
        error_syntax(env, "parameter list");
        core_return_expression(env, parameterList);
        return(NULL);
    }

    if( paramprintp )
    {
        core_backup_pp_buffer(env);
        core_backup_pp_buffer(env);
        core_save_pp_buffer(env, ")");
    }

    *error = FALSE;
    *max = (*wildcard != NULL) ? -1 : *min;
    return(parameterList);
}

#endif

/*************************************************************************
 *  NAME         : core_parse_function_actions
 *  DESCRIPTION  : Parses the bodies of deffunctions, generic function
 *              methods and message-handlers.  Replaces parameter
 *              and local variable references with appropriate
 *              runtime access functions
 *  INPUTS       : 1) The type of procedure body being parsed
 *              2) The logical name of the input
 *              3) A buffer for scanned tokens
 *              4) A list of expressions containing the names
 *                 of the parameters
 *              5) The wilcard parameter symbol (NULL if none)
 *              6) A pointer to a function to parse variables not
 *                 recognized by the standard parser
 *                 The function should accept the variable
 *                 expression and a generic pointer for special
 *                 data (can be NULL) as arguments.  If the variable
 *                 is recognized, the function should modify the
 *                 expression to access this variable.  Return 1
 *                 if recognized, 0 if not, -1 on errors
 *                 This argument can be NULL.
 *              7) A pointer to a function to handle binds in a
 *                 special way. The function should accept the
 *                 bind function call expression as an argument.
 *                 If the variable is recognized and treated specially,
 *                 the function should modify the expression
 *                 appropriately (including attaching/removing
 *                 any necessary argument expressions).  Return 1
 *                 if recognized, 0 if not, -1 on errors.
 *                 This argument can be NULL.
 *              8) A buffer for holding the number of local vars
 *                 used by this procedure body.
 *              9) Special user data buffer to pass to variable
 *                 reference and bind replacement functions
 *  RETURNS      : A packed expression containing the body, NULL on
 *                errors.
 *  SIDE EFFECTS : Variable references replaced with runtime calls
 *               to access the paramter and local variable array
 *  NOTES        : None
 *************************************************************************/
core_expression_object *core_parse_function_actions(void *env, char *bodytype, char *readSource, struct token *tkn, core_expression_object *params, ATOM_HN *wildcard, int (*altvarfunc)(void *, core_expression_object *, void *), int (*altbindfunc)(void *, core_expression_object *, void *), int *lvarcnt, void *userBuffer)
{
    core_expression_object *actions, *pactions;

    /* ====================================================================
     *  Clear parsed bind list - so that only local vars from this body will
     *  be on it.  The position of vars on thsi list are used to generate
     *  indices into the local_variables at runtime.  The parsing of the
     *  FUNC_NAME_ASSIGNMENT function adds vars to this list.
     *  ==================================================================== */
    clear_parsed_bindings(env);
    actions = group_actions(env, readSource, tkn, TRUE, NULL, FALSE);

    if( actions == NULL )
    {
        return(NULL);
    }

    /* ====================================================================
     *  Replace any bind functions with special functions before replacing
     *  any variable references.  This allows those bind names to be removed
     *  before they can be seen by variable replacement and thus generate
     *  incorrect indices.
     *  ==================================================================== */
    if( altbindfunc != NULL )
    {
        if( _release_function_binding(env, actions, altbindfunc, userBuffer))
        {
            clear_parsed_bindings(env);
            core_return_expression(env, actions);
            return(NULL);
        }
    }

    /* ======================================================================
     *  The number of names left on the bind list is the number of local
     *  vars for this procedure body.  Replace all variable reference with
     *  runtime access functions for arguments, local_variables or
     *  other special items, such as direct slot references,
     *  or fact field references.
     *  ====================================================================== */
    *lvarcnt = get_binding_count_in_current_context(env);

    if( core_replace_function_variable(env, bodytype, actions, params, wildcard, altvarfunc, userBuffer))
    {
        clear_parsed_bindings(env);
        core_return_expression(env, actions);
        return(NULL);
    }

    /* =======================================================================
     *  Normally, actions are grouped in a progn.  If there is only one action,
     *  the progn is unnecessary and can be removed.  Also, the actions are
     *  packed into a contiguous array to save on memory overhead.  The
     *  intermediate parsed bind names are freed to avoid tying up memory.
     *  ======================================================================= */
    actions = _pack_function_actions(env, actions);
    pactions = core_pack_expression(env, actions);
    core_return_expression(env, actions);
    clear_parsed_bindings(env);
    return(pactions);
}

/*************************************************************************
 *  NAME         : core_replace_function_variable
 *  DESCRIPTION  : Examines an expression for variables
 *                and replaces any that correspond to
 *                procedure parameters or globals
 *                with function calls that get these
 *                variables' values at run-time.
 *                For example, procedure arguments
 *                are stored an array at run-time, so at
 *                parse-time, parameter-references are replaced
 *                with function calls referencing this array at
 *                the appropriate position.
 *  INPUTS       : 1) The type of procedure being parsed
 *              2) The expression-actions to be examined
 *              3) The parameter list
 *              4) The wildcard parameter symbol (NULL if none)
 *              5) A pointer to a function to parse variables not
 *                 recognized by the standard parser
 *                 The function should accept the variable
 *                 expression and a generic pointer for special
 *                 data (can be NULL) as arguments.  If the variable
 *                 is recognized, the function should modify the
 *                 expression to access this variable.  Return 1
 *                 if recognized, 0 if not, -1 on errors
 *                 This argument can be NULL.
 *              6) Data buffer to be passed to alternate parsing
 *                 function
 *  RETURNS      : FALSE if OK, TRUE on errors
 *  SIDE EFFECTS : Variable references replaced with function calls
 *  NOTES        : This function works from the ParsedBindNames list in
 *                 SPCLFORM.C to access local binds.  Make sure that
 *                 the list accurately reflects the binds by calling
 *                 clear_parsed_bindings(env) before the parse of the body
 *                 in which variables are being replaced.
 *************************************************************************/
int core_replace_function_variable(void *env, char *bodytype, core_expression_object *actions, core_expression_object *parameterList, ATOM_HN *wildcard, int (*altvarfunc)(void *, core_expression_object *, void *), void *specdata)
{
    int position, altcode;
    BOOLEAN boundPosn;
    core_expression_object *arg_lvl, *altvarexp;
    ATOM_HN *bindName;
    PACKED_FUNCTION_VARIABLE pvar;

    while( actions != NULL )
    {
        if( actions->type == SCALAR_VARIABLE )
        {
            /*===============================================
             * See if the variable is in the parameter list.
             *===============================================*/
            bindName = (ATOM_HN *)actions->value;
            position = _lookup_function_arg(bindName, parameterList, wildcard);

            /*=============================================================
             * Check to see if the variable is bound within the procedure.
             *=============================================================*/
            boundPosn = find_parsed_binding(env, bindName);

            /*=============================================
             * If variable is not defined in the parameter
             * list or as part of a bind action then...
             *=============================================*/

            if((position == 0) && (boundPosn == 0))
            {
                /*================================================================
                 * Check to see if the variable has a special access function,
                 * such as direct slot reference or a rule RHS pattern reference.
                 *================================================================*/
                if((altvarfunc != NULL) ? ((*altvarfunc)(env, actions, specdata) != 1) : TRUE )
                {
                    error_print_id(env, "FLOW", 3, TRUE);
                    print_router(env, WERROR, "Undefined variable ");
                    print_router(env, WERROR, to_string(bindName));
                    print_router(env, WERROR, " referenced in ");
                    print_router(env, WERROR, bodytype);
                    print_router(env, WERROR, ".\n");
                    return(TRUE);
                }
            }

            /*===================================================
             * Else if variable is defined in the parameter list
             * and not rebound within the procedure then...
             *===================================================*/

            else if((position > 0) && (boundPosn == 0))
            {
                actions->type = (unsigned short)((bindName != wildcard) ? FUNCTION_ARG : FUNCTION_OPTIONAL_ARG);
                actions->value = store_bitmap(env, (void *)&position, (int)sizeof(int));
            }

            /*=========================================================
             * Else the variable is rebound within the procedure so...
             *=========================================================*/

            else
            {
                if( altvarfunc != NULL )
                {
                    altvarexp = core_generate_constant(env, actions->type, actions->value);
                    altcode = (*altvarfunc)(env, altvarexp, specdata);

                    if( altcode == 0 )
                    {
                        core_mem_return_struct(env, core_expression, altvarexp);
                        altvarexp = NULL;
                    }
                    else if( altcode == -1 )
                    {
                        core_mem_return_struct(env, core_expression, altvarexp);
                        return(TRUE);
                    }
                }
                else
                {
                    altvarexp = NULL;
                }

                actions->type = FUNCTION_VAR_META;
                init_bitmap((void *)&pvar, (int)sizeof(PACKED_FUNCTION_VARIABLE));
                pvar.first = boundPosn;
                pvar.second = position;
                pvar.secondFlag = (bindName != wildcard) ? 0 : 1;
                actions->value = store_bitmap(env, (void *)&pvar, (int)sizeof(PACKED_FUNCTION_VARIABLE));
                actions->args = core_generate_constant(env, ATOM, (void *)bindName);
                actions->args->next_arg = altvarexp;
            }
        }

        if((altvarfunc != NULL) ? ((*altvarfunc)(env, actions, specdata) == -1) : FALSE )
        {
            return(TRUE);
        }

        if( actions->args != NULL )
        {
            if( core_replace_function_variable(env, bodytype, actions->args, parameterList,
                                               wildcard, altvarfunc, specdata))
            {
                return(TRUE);
            }

            /* ====================================================================
             *  Check to see if this is a call to the bind function.  If so (and the
             *  second argument is a symbol) then it is a locally bound variable
             *  (as opposed to a global).
             *
             *  Replace the call to FUNC_NAME_ASSIGNMENT with a call to FUNCTION_VAR - the
             *  special internal function for procedure local variables.
             *  ==================================================================== */
            if((actions->value == (void *)core_lookup_function(env, FUNC_NAME_ASSIGNMENT)) &&
               (actions->args->type == ATOM))
            {
                actions->type = FUNCTION_VAR;
                boundPosn = find_parsed_binding(env, (ATOM_HN *)actions->args->value);
                actions->value = store_bitmap(env, (void *)&boundPosn, (int)sizeof(BOOLEAN));
                arg_lvl = actions->args->next_arg;
                core_mem_return_struct(env, core_expression, actions->args);
                actions->args = arg_lvl;
            }
        }

        actions = actions->next_arg;
    }

    return(FALSE);
}

#if DEFGENERIC_CONSTRUCT

/*****************************************************
 *  NAME         : core_generate_generic_function_reference
 *  DESCRIPTION  : Returns an expression to access the
 *                 wildcard parameter for a method
 *  INPUTS       : The starting index of the wildcard
 *  RETURNS      : An expression containing the wildcard
 *              reference
 *  SIDE EFFECTS : Expression allocated
 *  NOTES        : None
 *****************************************************/
core_expression_object *core_generate_generic_function_reference(void *env, int theIndex)
{
    return(core_generate_constant(env, FUNCTION_OPTIONAL_ARG, store_bitmap(env, (void *)&theIndex, (int)sizeof(int))));
}

#endif


/*******************************************************************
 *  NAME         : core_push_function_args
 *  DESCRIPTION  : Given a list of parameter expressions,
 *                this function evaluates each expression
 *                and stores the results in a contiguous
 *                array of DATA_OBJECTS.  Used in creating a new
 *                arguments for the execution of a
 *                procedure
 *              The current arrays are saved on a stack.
 *  INPUTS       : 1) The paramter expression list
 *              2) The number of parameters in the list
 *              3) The name of the procedure for which
 *                 these parameters are being evaluated
 *              4) The type of procedure
 *              5) A pointer to a function to print out a trace
 *                 message about the currently executing
 *                 procedure when unbound variables are detected
 *                 at runtime (The function should take no
 *                 arguments and have no return value.  The
 *                 function should print its synopsis to WERROR
 *                 and include the final carriage-return.)
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Any side-effects of the evaluation of the
 *                parameter expressions
 *              core_data_object array allocated (deallocated on errors)
 *              arguments set
 *  NOTES        : eval_error set on errors
 *******************************************************************/
void core_push_function_args(void *env, core_expression_object *parameterList, int numberOfParameters, char *pname, char *bodytype, void (*UnboundErrFunc)(void *))
{
    register FUNCTION_ARGUMENT_STACK *ptmp;

    ptmp = core_mem_get_struct(env, core_function_parameter_stack);
    ptmp->arguments = core_get_function_primitive_data(env)->arguments;
    ptmp->arguments_sz = core_get_function_primitive_data(env)->arguments_sz;
    ptmp->fn_unbound_error = core_get_function_primitive_data(env)->fn_unbound_error;
    ptmp->nxt = core_get_function_primitive_data(env)->arg_stack;
    core_get_function_primitive_data(env)->arg_stack = ptmp;
    _eval_function_args(env, parameterList, numberOfParameters, pname, bodytype);

    if( core_get_evaluation_data(env)->eval_error )
    {
        ptmp = core_get_function_primitive_data(env)->arg_stack;
        core_get_function_primitive_data(env)->arg_stack = core_get_function_primitive_data(env)->arg_stack->nxt;
        core_mem_return_struct(env, core_function_parameter_stack, ptmp);
        return;
    }

    /* ================================================================
     *  Record function_generic_args and optional_argument for previous frame
     *  AFTER evaluating arguments for the new frame, because they could
     *  have gone from NULL to non-NULL (if they were already non-NULL,
     *  they would remain unchanged.)
     *  ================================================================ */
#if DEFGENERIC_CONSTRUCT
    ptmp->generic_args = core_get_function_primitive_data(env)->function_generic_args;
    core_get_function_primitive_data(env)->function_generic_args = NULL;
#endif
    ptmp->optional_argument = core_get_function_primitive_data(env)->optional_argument;
    core_get_function_primitive_data(env)->optional_argument = NULL;
    core_get_function_primitive_data(env)->fn_unbound_error = UnboundErrFunc;
}

/******************************************************************
 *  NAME         : core_pop_function_args
 *  DESCRIPTION  : Restores old procedure arrays
 *  INPUTS       : None
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Stack popped and globals restored
 *  NOTES        : Assumes arg_stack != NULL
 ******************************************************************/
void core_pop_function_args(void *env)
{
    register FUNCTION_ARGUMENT_STACK *ptmp;

    if( core_get_function_primitive_data(env)->arguments != NULL )
    {
        core_mem_release(env, (void *)core_get_function_primitive_data(env)->arguments, (sizeof(core_data_object) * core_get_function_primitive_data(env)->arguments_sz));
    }

#if DEFGENERIC_CONSTRUCT

    if( core_get_function_primitive_data(env)->function_generic_args != NULL )
    {
        core_mem_release(env, (void *)core_get_function_primitive_data(env)->function_generic_args, (sizeof(core_expression_object) * core_get_function_primitive_data(env)->arguments_sz));
    }

#endif

    ptmp = core_get_function_primitive_data(env)->arg_stack;
    core_get_function_primitive_data(env)->arg_stack = core_get_function_primitive_data(env)->arg_stack->nxt;
    core_get_function_primitive_data(env)->arguments = ptmp->arguments;
    core_get_function_primitive_data(env)->arguments_sz = ptmp->arguments_sz;

#if DEFGENERIC_CONSTRUCT
    core_get_function_primitive_data(env)->function_generic_args = ptmp->generic_args;
#endif

    if( core_get_function_primitive_data(env)->optional_argument != NULL )
    {
        uninstall_list(env, (LIST_PTR)core_get_function_primitive_data(env)->optional_argument->value);

        if( core_get_function_primitive_data(env)->optional_argument->value != core_get_function_primitive_data(env)->null_arg_value )
        {
            track_list(env, (LIST_PTR)core_get_function_primitive_data(env)->optional_argument->value);
        }

        core_mem_return_struct(env, core_data, core_get_function_primitive_data(env)->optional_argument);
    }

    core_get_function_primitive_data(env)->optional_argument = ptmp->optional_argument;
    core_get_function_primitive_data(env)->fn_unbound_error = ptmp->fn_unbound_error;
    core_mem_return_struct(env, core_function_parameter_stack, ptmp);
}

/******************************************************************
 *  NAME         : ReleaseProcParameters
 *  DESCRIPTION  : Restores old procedure arrays
 *  INPUTS       : None
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Stack popped and globals restored
 *  NOTES        : Assumes arg_stack != NULL
 ******************************************************************/
static void _release_function_args(void *env)
{
    register FUNCTION_ARGUMENT_STACK *ptmp, *next;

    if( core_get_function_primitive_data(env)->arguments != NULL )
    {
        core_mem_release(env, (void *)core_get_function_primitive_data(env)->arguments, (sizeof(core_data_object) * core_get_function_primitive_data(env)->arguments_sz));
    }


    if( core_get_function_primitive_data(env)->optional_argument != NULL )
    {
        if( core_get_function_primitive_data(env)->optional_argument->value != core_get_function_primitive_data(env)->null_arg_value )
        {
            release_list(env, (struct list *)core_get_function_primitive_data(env)->optional_argument->value);
        }

        core_mem_return_struct(env, core_data, core_get_function_primitive_data(env)->optional_argument);
    }

#if DEFGENERIC_CONSTRUCT

    if( core_get_function_primitive_data(env)->function_generic_args != NULL )
    {
        core_mem_release(env, (void *)core_get_function_primitive_data(env)->function_generic_args, (sizeof(core_expression_object) * core_get_function_primitive_data(env)->arguments_sz));
    }

#endif

    ptmp = core_get_function_primitive_data(env)->arg_stack;

    while( ptmp != NULL )
    {
        next = ptmp->nxt;

        if( ptmp->arguments != NULL )
        {
            core_mem_release(env, (void *)ptmp->arguments, (sizeof(core_data_object) * ptmp->arguments_sz));
        }

#if DEFGENERIC_CONSTRUCT

        if( ptmp->generic_args != NULL )
        {
            core_mem_release(env, (void *)ptmp->generic_args, (sizeof(core_expression_object) * ptmp->arguments_sz));
        }

#endif

        if( ptmp->optional_argument != NULL )
        {
            if( ptmp->optional_argument->value != core_get_function_primitive_data(env)->null_arg_value )
            {
                release_list(env, (struct list *)ptmp->optional_argument->value);
            }

            core_mem_return_struct(env, core_data, ptmp->optional_argument);
        }

        core_mem_return_struct(env, core_function_parameter_stack, ptmp);
        ptmp = next;
    }
}

#if DEFGENERIC_CONSTRUCT

/***********************************************************
 *  NAME         : core_garner_generic_args
 *  DESCRIPTION  : Forms an array of expressions equivalent to
 *              the current procedure paramter array.  Used
 *              to conveniently attach these parameters as
 *              arguments to a H/L system function call
 *              (used by the generic dispatch).
 *  INPUTS       : None
 *  RETURNS      : A pointer to an array of expressions
 *  SIDE EFFECTS : Expression array created
 *  NOTES        : None
 ***********************************************************/
core_expression_object *core_garner_generic_args(void *env)
{
    register int i;

    if((core_get_function_primitive_data(env)->arguments == NULL) || (core_get_function_primitive_data(env)->function_generic_args != NULL))
    {
        return(core_get_function_primitive_data(env)->function_generic_args);
    }

    core_get_function_primitive_data(env)->function_generic_args = (core_expression_object *)
                                                                   core_mem_alloc_no_init(env, (sizeof(core_expression_object) * core_get_function_primitive_data(env)->arguments_sz));

    for( i = 0 ; i < core_get_function_primitive_data(env)->arguments_sz ; i++ )
    {
        core_get_function_primitive_data(env)->function_generic_args[i].type = core_get_function_primitive_data(env)->arguments[i].type;

        if( core_get_function_primitive_data(env)->arguments[i].type != LIST )
        {
            core_get_function_primitive_data(env)->function_generic_args[i].value = core_get_function_primitive_data(env)->arguments[i].value;
        }
        else
        {
            core_get_function_primitive_data(env)->function_generic_args[i].value = (void *)&core_get_function_primitive_data(env)->arguments[i];
        }

        core_get_function_primitive_data(env)->function_generic_args[i].args = NULL;
        core_get_function_primitive_data(env)->function_generic_args[i].next_arg =
            ((i + 1) != core_get_function_primitive_data(env)->arguments_sz) ? &core_get_function_primitive_data(env)->function_generic_args[i + 1] : NULL;
    }

    return(core_get_function_primitive_data(env)->function_generic_args);
}

#endif

/***********************************************************
 *  NAME         : core_eval_function_actions
 *  DESCRIPTION  : Evaluates the actions of a deffunction,
 *              generic function method or message-handler.
 *  INPUTS       : 1) The module where the actions should be
 *                 executed
 *              2) The actions (linked by next_arg fields)
 *              3) The number of local variables to reserve
 *                 space for.
 *              4) A buffer to hold the result of evaluating
 *                 the actions.
 *              5) A function which prints out the name of
 *                 the currently executing body for error
 *                 messages (can be NULL).
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Allocates and deallocates space for
 *              local variable array.
 *  NOTES        : None
 ***********************************************************/
void core_eval_function_actions(void *env, struct module_definition *theModule, core_expression_object *actions, int lvarcnt, core_data_object *result, void (*crtproc)(void *))
{
    core_data_object *oldLocalVarArray;
    register int i;
    struct module_definition *oldModule;
    core_expression_object *oldActions;
    struct core_gc_tracked_memory *theTM;

    oldLocalVarArray = core_get_function_primitive_data(env)->local_variables;
    core_get_function_primitive_data(env)->local_variables = (lvarcnt == 0) ? NULL :
                                                             (core_data_object *)core_mem_alloc_no_init(env, (sizeof(core_data_object) * lvarcnt));

    if( lvarcnt != 0 )
    {
        theTM = core_gc_add_tracked_memory(env, core_get_function_primitive_data(env)->local_variables, sizeof(core_data_object) * lvarcnt);
    }
    else
    {
        theTM = NULL;
    }

    for( i = 0 ; i < lvarcnt ; i++ )
    {
        core_get_function_primitive_data(env)->local_variables[i].metadata = get_false(env);
    }

    oldModule = ((struct module_definition *)get_current_module(env));

    if( oldModule != theModule )
    {
        set_current_module(env, (void *)theModule);
    }

    oldActions = core_get_function_primitive_data(env)->actions;
    core_get_function_primitive_data(env)->actions = actions;

    if( core_eval_expression(env, actions, result))
    {
        result->type = ATOM;
        result->value = get_false(env);
    }

    core_get_function_primitive_data(env)->actions = oldActions;

    if( oldModule != ((struct module_definition *)get_current_module(env)))
    {
        set_current_module(env, (void *)oldModule);
    }

    if((crtproc != NULL) ? core_get_evaluation_data(env)->halt : FALSE )
    {
        error_print_id(env, "FLOW", 4, FALSE);
        print_router(env, WERROR, "Execution halted during the actions of ");
        (*crtproc)(env);
    }

    if((core_get_function_primitive_data(env)->optional_argument != NULL) ? (result->value == core_get_function_primitive_data(env)->optional_argument->value) : FALSE )
    {
        uninstall_list(env, (LIST_PTR)core_get_function_primitive_data(env)->optional_argument->value);

        if( core_get_function_primitive_data(env)->optional_argument->value != core_get_function_primitive_data(env)->null_arg_value )
        {
            track_list(env, (LIST_PTR)core_get_function_primitive_data(env)->optional_argument->value);
        }

        core_mem_return_struct(env, core_data, core_get_function_primitive_data(env)->optional_argument);
        core_get_function_primitive_data(env)->optional_argument = NULL;
    }

    if( lvarcnt != 0 )
    {
        core_gc_remove_tracked_memory(env, theTM);

        for( i = 0 ; i < lvarcnt ; i++ )
        {
            if( core_get_function_primitive_data(env)->local_variables[i].metadata == get_true(env))
            {
                core_value_decrement(env, &core_get_function_primitive_data(env)->local_variables[i]);
            }
        }

        core_mem_release(env, (void *)core_get_function_primitive_data(env)->local_variables, (sizeof(core_data_object) * lvarcnt));
    }

    core_get_function_primitive_data(env)->local_variables = oldLocalVarArray;
}

/****************************************************
 *  NAME         : core_print_function_args
 *  DESCRIPTION  : Displays the contents of the
 *              current procedure parameter array
 *  INPUTS       : The logical name of the output
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : None
 *  NOTES        : None
 ****************************************************/
void core_print_function_args(void *env, char *logName)
{
    register int i;

    print_router(env, logName, " (");

    for( i = 0 ; i < core_get_function_primitive_data(env)->arguments_sz ; i++ )
    {
        core_print_data(env, logName, &core_get_function_primitive_data(env)->arguments[i]);

        if( i != core_get_function_primitive_data(env)->arguments_sz - 1 )
        {
            print_router(env, logName, " ");
        }
    }

    print_router(env, logName, ")\n");
}

/****************************************************************
 *  NAME         : core_garner_optional_args
 *  DESCRIPTION  : Groups a portion of the arguments
 *                into a list variable
 *  INPUTS       : 1) Starting index in arguments
 *                   for grouping of arguments into
 *                   list variable
 *              2) Caller's result value buffer
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Multi-field variable allocated and set
 *                with corresponding values of arguments
 *  NOTES        : Multi-field is NOT on list of is_ephemeral segments
 ****************************************************************/
void core_garner_optional_args(void *env, core_data_object *result, int theIndex)
{
    register int i, j;
    long k; /* 6.04 Bug Fix */
    long size;
    core_data_object *val;

    result->type = LIST;
    result->begin = 0;

    if( core_get_function_primitive_data(env)->optional_argument == NULL )
    {
        core_get_function_primitive_data(env)->optional_argument = core_mem_get_struct(env, core_data);
        core_get_function_primitive_data(env)->optional_argument->begin = 0;
    }
    else if( theIndex == core_get_function_primitive_data(env)->old_index )
    {
        result->end = core_get_function_primitive_data(env)->optional_argument->end;
        result->value = core_get_function_primitive_data(env)->optional_argument->value;
        return;
    }
    else
    {
        uninstall_list(env, (LIST_PTR)core_get_function_primitive_data(env)->optional_argument->value);

        if( core_get_function_primitive_data(env)->optional_argument->value != core_get_function_primitive_data(env)->null_arg_value )
        {
            track_list(env, (LIST_PTR)core_get_function_primitive_data(env)->optional_argument->value);
        }
    }

    core_get_function_primitive_data(env)->old_index = theIndex;
    size = core_get_function_primitive_data(env)->arguments_sz - theIndex + 1;

    if( size <= 0 )
    {
        result->end = core_get_function_primitive_data(env)->optional_argument->end = -1;
        result->value = core_get_function_primitive_data(env)->optional_argument->value = core_get_function_primitive_data(env)->null_arg_value;
        install_list(env, (LIST_PTR)core_get_function_primitive_data(env)->optional_argument->value);
        return;
    }

    for( i = theIndex - 1 ; i < core_get_function_primitive_data(env)->arguments_sz ; i++ )
    {
        if( core_get_function_primitive_data(env)->arguments[i].type == LIST )
        {
            size += core_get_function_primitive_data(env)->arguments[i].end - core_get_function_primitive_data(env)->arguments[i].begin;
        }
    }

    result->end = core_get_function_primitive_data(env)->optional_argument->end = size - 1;
    result->value = core_get_function_primitive_data(env)->optional_argument->value = (void *)create_sized_list(env, (unsigned long)size);

    for( i = theIndex - 1, j = 1 ; i < core_get_function_primitive_data(env)->arguments_sz ; i++ )
    {
        if( core_get_function_primitive_data(env)->arguments[i].type != LIST )
        {
            set_list_node_type(result->value, j, (short)core_get_function_primitive_data(env)->arguments[i].type);
            set_list_node_value(result->value, j, core_get_function_primitive_data(env)->arguments[i].value);
            j++;
        }
        else
        {
            val = &core_get_function_primitive_data(env)->arguments[i];

            for( k = val->begin + 1 ; k <= val->end + 1 ; k++, j++ )
            {
                set_list_node_type(result->value, j, get_list_node_type(val->value, k));
                set_list_node_value(result->value, j, get_list_node_value(val->value, k));
            }
        }
    }

    install_list(env, (LIST_PTR)core_get_function_primitive_data(env)->optional_argument->value);
}

/* =========================================
 *****************************************
 *       INTERNALLY VISIBLE FUNCTIONS
 *  =========================================
 ***************************************** */

/*******************************************************************
 *  NAME         : EvaluateProcParameters
 *  DESCRIPTION  : Given a list of parameter expressions,
 *                this function evaluates each expression
 *                and stores the results in a contiguous
 *                array of DATA_OBJECTS.  Used in creating a new
 *                arguments for the execution of a
 *                procedure
 *  INPUTS       : 1) The paramter expression list
 *              2) The number of parameters in the list
 *              3) The name of the procedure for which
 *                 these parameters are being evaluated
 *              4) The type of procedure
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Any side-effects of the evaluation of the
 *                parameter expressions
 *              core_data_object array allocated (deallocated on errors)
 *              arguments set
 *  NOTES        : eval_error set on errors
 *******************************************************************/
static void _eval_function_args(void *env, core_expression_object *parameterList, int numberOfParameters, char *pname, char *bodytype)
{
    core_data_object *rva, temp;
    int i = 0;

    if( numberOfParameters == 0 )
    {
        core_get_function_primitive_data(env)->arguments = NULL;
        core_get_function_primitive_data(env)->arguments_sz = 0;
        return;
    }

    rva = (core_data_object *)core_mem_alloc_no_init(env, (sizeof(core_data_object) * numberOfParameters));

    while( parameterList != NULL )
    {
        if((core_eval_expression(env, parameterList, &temp) == TRUE) ? TRUE :
           (temp.type == RVOID))
        {
            if( temp.type == RVOID )
            {
                error_print_id(env, "FLOW", 2, FALSE);
                print_router(env, WERROR, "Functions without a return value are illegal as ");
                print_router(env, WERROR, bodytype);
                print_router(env, WERROR, " arguments.\n");
                core_set_eval_error(env, TRUE);
            }

            error_print_id(env, "FLOW", 6, FALSE);
            print_router(env, WERROR, "This error occurred while evaluating arguments ");
            print_router(env, WERROR, "for the ");
            print_router(env, WERROR, bodytype);
            print_router(env, WERROR, " ");
            print_router(env, WERROR, pname);
            print_router(env, WERROR, ".\n");
            core_mem_release(env, (void *)rva, (sizeof(core_data_object) * numberOfParameters));
            return;
        }

        rva[i].type = temp.type;
        rva[i].value = temp.value;
        rva[i].begin = temp.begin;
        rva[i].end = temp.end;
        parameterList = parameterList->next_arg;
        i++;
    }

    core_get_function_primitive_data(env)->arguments_sz = numberOfParameters;
    core_get_function_primitive_data(env)->arguments = rva;
}

/***************************************************
 *  NAME         : RtnProcParam
 *  DESCRIPTION  : Internal function for getting the
 *                value of an argument passed to
 *                a procedure
 *  INPUTS       : 1) Expression to evaluate
 *                 (FUNCTION_ARG index)
 *              2) Caller's result value buffer
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Caller's buffer set to specified
 *                node of arguments
 *  NOTES        : None
 ***************************************************/
static BOOLEAN _get_function_arg(void *env, void *value, core_data_object *result)
{
    register core_data_object *src;

    src = &core_get_function_primitive_data(env)->arguments[*((int *)to_bitmap(value)) - 1];
    result->type = src->type;
    result->value = src->value;
    result->begin = src->begin;
    result->end = src->end;
    return(TRUE);
}

/**************************************************************
 *  NAME         : GetProcBind
 *  DESCRIPTION  : Internal function for looking up the
 *                 values of parameters or bound variables
 *                 within procedures
 *  INPUTS       : 1) Expression to evaluate
 *                 (FUNCTION_VAR_META index)
 *              2) Caller's result value buffer
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Caller's buffer set to parameter value in
 *                arguments or the value in local_variables
 *  NOTES        : None
 **************************************************************/
static BOOLEAN _lookup_function_binding(void *env, void *value, core_data_object *result)
{
    register core_data_object *src;
    PACKED_FUNCTION_VARIABLE *pvar;

    pvar = (PACKED_FUNCTION_VARIABLE *)to_bitmap(value);
    src = &core_get_function_primitive_data(env)->local_variables[pvar->first - 1];

    if( src->metadata == get_true(env))
    {
        result->type = src->type;
        result->value = src->value;
        result->begin = src->begin;
        result->end = src->end;
        return(TRUE);
    }

    if( core_get_first_arg()->next_arg != NULL )
    {
        core_eval_expression(env, core_get_first_arg()->next_arg, result);
        return(TRUE);
    }

    if( pvar->second == 0 )
    {
        error_print_id(env, "FLOW", 5, FALSE);
        core_set_eval_error(env, TRUE);
        print_router(env, WERROR, "Variable ");
        print_router(env, WERROR, to_string(core_get_first_arg()->value));

        if( core_get_function_primitive_data(env)->fn_unbound_error != NULL )
        {
            print_router(env, WERROR, " unbound in ");
            (*core_get_function_primitive_data(env)->fn_unbound_error)(env);
        }
        else
        {
            print_router(env, WERROR, " unbound.\n");
        }

        result->type = ATOM;
        result->value = get_false(env);
        return(TRUE);
    }

    if( pvar->secondFlag == 0 )
    {
        src = &core_get_function_primitive_data(env)->arguments[pvar->second - 1];
        result->type = src->type;
        result->value = src->value;
        result->begin = src->begin;
        result->end = src->end;
    }
    else
    {
        core_garner_optional_args(env, result, (int)pvar->second);
    }

    return(TRUE);
}

/**************************************************************
 *  NAME         : PutProcBind
 *  DESCRIPTION  : Internal function for setting the values of
 *              of locally bound variables within procedures
 *  INPUTS       : 1) Expression to evaluate
 *                 (FUNCTION_ARG index)
 *              2) Caller's result value buffer
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Bound variable in local_variables set to
 *                value in caller's buffer.
 *  NOTES        : None
 **************************************************************/
static BOOLEAN _set_function_binding(void *env, void *value, core_data_object *result)
{
    register core_data_object *dst;

    dst = &core_get_function_primitive_data(env)->local_variables[*((int *)to_bitmap(value)) - 1];

    if( core_get_first_arg() == NULL )
    {
        if( dst->metadata == get_true(env))
        {
            core_value_decrement(env, dst);
        }

        dst->metadata = get_false(env);
        result->type = ATOM;
        result->value = get_false(env);
    }
    else
    {
        if( core_get_first_arg()->next_arg != NULL )
        {
            add_to_list(env, result, core_get_first_arg(), TRUE);
        }
        else
        {
            core_eval_expression(env, core_get_first_arg(), result);
        }

        if( dst->metadata == get_true(env))
        {
            core_value_decrement(env, dst);
        }

        dst->metadata = get_true(env);
        dst->type = result->type;
        dst->value = result->value;
        dst->begin = result->begin;
        dst->end = result->end;
        core_value_increment(env, dst);
    }

    return(TRUE);
}

/****************************************************************
 *  NAME         : RtnProcWild
 *  DESCRIPTION  : Groups a portion of the arguments
 *                into a list variable
 *  INPUTS       : 1) Starting index in arguments
 *                   for grouping of arguments into
 *                   list variable (expression value)
 *              2) Caller's result value buffer
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Multi-field variable allocated and set
 *                with corresponding values of arguments
 *  NOTES        : Multi-field is NOT on list of is_ephemeral segments
 ****************************************************************/
static BOOLEAN _get_function_optional_arg(void *env, void *value, core_data_object *result)
{
    core_garner_optional_args(env, result, *(int *)to_bitmap(value));
    return(TRUE);
}

/***************************************************
 *  NAME         : FindProcParameter
 *  DESCRIPTION  : Determines the relative position in
 *                an n-element list of a certain
 *                parameter.  The index is 1..n.
 *  INPUTS       : 1) Parameter name
 *              2) Parameter list
 *              3) Wildcard symbol (NULL if none)
 *  RETURNS      : Index of parameter in list, 0 if
 *                not found
 *  SIDE EFFECTS : None
 *  NOTES        : None
 ***************************************************/
static int _lookup_function_arg(ATOM_HN *name, core_expression_object *parameterList, ATOM_HN *wildcard)
{
    int i = 1;

    while( parameterList != NULL )
    {
        if( parameterList->value == (void *)name )
        {
            return(i);
        }

        i++;
        parameterList = parameterList->next_arg;
    }

    /* ===================================================================
     *  Wildcard may not be stored in actual list but know is always at end
     *  =================================================================== */
    if( name == wildcard )
    {
        return(i);
    }

    return(0);
}

/*************************************************************************
 *  NAME         : ReplaceProcBinds
 *  DESCRIPTION  : Examines an expression and replaces calls to the
 *              FUNC_NAME_ASSIGNMENT function which are specially recognized
 *
 *              For example, in a message-handler,
 *
 *                (bind ?self <value>) would be illegal
 *
 *                and
 *
 *                (bind ?self:<slot-name> <value>) would be
 *                replaced with
 *                (put <slot-name> <value>)
 *
 *  INPUTS       : 1) The actions in which to replace special binds
 *              2) A pointer to a function to handle binds in a
 *                 special way. The function should accept the
 *                 bind function call expression and a specialized
 *                 data buffer (can be NULL) as arguments.
 *                 If the variable is recognized and treated specially,
 *                 the function should modify the expression
 *                 appropriately (including attaching/removing
 *                 any necessary argument expressions).  Return 1
 *                 if recognized, 0 if not, -1 on errors.
 *                 This argument CANNOT be NULL.
 *              3) Specialized user data buffer
 *  RETURNS      : FALSE if OK, TRUE on errors
 *  SIDE EFFECTS : Some binds replaced with specialized calls
 *  NOTES        : Local variable binds are replaced in core_replace_function_variable
 *              (after this routine has had a chance to replace all
 *               special binds and remove the names from the parsed
 *               bind list)
 *************************************************************************/
static int _release_function_binding(void *env, core_expression_object *actions, int (*altbindfunc)(void *, core_expression_object *, void *), void *userBuffer)
{
    int bcode;
    ATOM_HN *bname;

    while( actions != NULL )
    {
        if( actions->args != NULL )
        {
            if( _release_function_binding(env, actions->args, altbindfunc, userBuffer))
            {
                return(TRUE);
            }

            if((actions->value == (void *)core_lookup_function(env, FUNC_NAME_ASSIGNMENT)) &&
               (actions->args->type == ATOM))
            {
                bname = (ATOM_HN *)actions->args->value;
                bcode = (*altbindfunc)(env, actions, userBuffer);

                if( bcode == -1 )
                {
                    return(TRUE);
                }

                if( bcode == 1 )
                {
                    remove_binding(env, bname);
                }
            }
        }

        actions = actions->next_arg;
    }

    return(FALSE);
}

/*****************************************************
 *  NAME         : CompactActions
 *  DESCRIPTION  : Examines a progn expression chain,
 *              and if there is only one action,
 *              the progn header is deallocated and
 *              the action is returned.  If there are
 *              no actions, the progn expression is
 *              modified to be the FALSE symbol
 *              and returned.  Otherwise, the progn
 *              is simply returned.
 *  INPUTS       : The action expression
 *  RETURNS      : The compacted expression
 *  SIDE EFFECTS : Some expressions possibly deallocated
 *  NOTES        : Assumes actions is a progn expression
 *              and actions->next_arg == NULL
 *****************************************************/
static core_expression_object *_pack_function_actions(void *env, core_expression_object *actions)
{
    register struct core_expression *tmp;

    if( actions->args == NULL )
    {
        actions->type = ATOM;
        actions->value = get_false(env);
    }
    else if( actions->args->next_arg == NULL )
    {
        tmp = actions;
        actions = actions->args;
        core_mem_return_struct(env, core_expression, tmp);
    }

    return(actions);
}

#if (!DEFFUNCTION_CONSTRUCT) || (!DEFGENERIC_CONSTRUCT)

/******************************************************
 *  NAME         : EvaluateBadCall
 *  DESCRIPTION  : Default evaluation function for
 *              deffunctions and gneric functions
 *              in configurations where either
 *              capability is not present.
 *  INPUTS       : 1) The function (ignored)
 *              2) A data object buffer for the result
 *  RETURNS      : FALSE
 *  SIDE EFFECTS : Data object buffer set to the
 *              symbol FALSE and evaluation error set
 *  NOTES        : Used for binary images which
 *              contain deffunctions and generic
 *              functions which cannot be used
 ******************************************************/
#if WIN_BTC
#pragma argsused
#endif
static BOOLEAN _dummy_function_call(void *env, void *value, core_data_object *result)
{
#if MAC_MCW || WIN_MCW || MAC_XCD
#pragma unused(value)
#endif
    error_print_id(env, "FLOW", 1, FALSE);
    print_router(env, WERROR, "Attempted to call a deffunction/generic function ");
    print_router(env, WERROR, "which does not exist.\n");
    core_set_eval_error(env, TRUE);
    core_set_pointer_type(result, ATOM);
    core_set_pointer_value(result, get_false(env));
    return(FALSE);
}

#endif
