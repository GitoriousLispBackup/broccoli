/* Purpose:  Instance Function Parsing Routines              */

/* =========================================
 *****************************************
 *            EXTERNAL DEFINITIONS
 *  =========================================
 ***************************************** */
#include "setup.h"

#if OBJECT_SYSTEM

#ifndef _STDIO_INCLUDED_
#define _STDIO_INCLUDED_
#include <stdio.h>
#endif

#include <string.h>

#include "classes_kernel.h"
#include "funcs_class.h"
#include "classes_info.h"
#include "constant.h"
#include "core_environment.h"
#include "core_evaluation.h"
#include "parser_expressions.h"
#include "core_functions.h"
#include "modules_init.h"
#include "core_utilities.h"
#include "router.h"

#define __PARSER_CLASS_INSTANCES_SOURCE__
#include "parser_class_instances.h"

/* =========================================
 *****************************************
 *                CONSTANTS
 *  =========================================
 ***************************************** */
#define MAKE_TYPE       0
#define INITIALIZE_TYPE 1
#define MODIFY_TYPE     2
#define DUPLICATE_TYPE  3

#define CLASS_RLN          "of"
#define DUPLICATE_NAME_REF "to"

/* =========================================
 *****************************************
 *   INTERNALLY VISIBLE FUNCTION HEADERS
 *  =========================================
 ***************************************** */

static BOOLEAN ReplaceClassNameWithReference(void *, core_expression_object *);

/* =========================================
 *****************************************
 *       EXTERNALLY VISIBLE FUNCTIONS
 *  =========================================
 ***************************************** */


/*************************************************************************************
 *  NAME         : ParseInitializeInstance
 *  DESCRIPTION  : Parses initialize-instance and make-instance function
 *                calls into an core_expression_object form that
 *                can later be evaluated with core_eval_expression(theEnv,)
 *  INPUTS       : 1) The address of the top node of the expression
 *                 containing the initialize-instance function call
 *              2) The logical name of the input source
 *  RETURNS      : The address of the modified expression, or NULL
 *                 if there is an error
 *  SIDE EFFECTS : The expression is enhanced to include all
 *                 aspects of the initialize-instance call
 *                 (slot-overrides etc.)
 *              The "top" expression is deleted on errors.
 *  NOTES        : This function parses a initialize-instance call into
 *              an expression of the following form :
 *
 *              (initialize-instance <instance-name> <slot-override>*)
 *               where <slot-override> ::= (<slot-name> <expression>+)
 *
 *               goes to -->
 *
 *               initialize-instance
 |
 *                   V
 *               <instance or name>-><slot-name>-><dummy-node>...
 |
 *                                                   V
 *                                            <value-expression>...
 *
 *               (make-instance <instance> of <class> <slot-override>*)
 *               goes to -->
 *
 *               make-instance
 |
 *                   V
 *               <instance-name>-><class-name>-><slot-name>-><dummy-node>...
 |
 *                                                              V
 *                                                       <value-expression>...
 *
 *               (make-instance of <class> <slot-override>*)
 *               goes to -->
 *
 *               make-instance
 |
 *                   V
 *               (gensym*)-><class-name>-><slot-name>-><dummy-node>...
 |
 *                                                              V
 *                                                       <value-expression>...
 *
 *               (modify-instance <instance> <slot-override>*)
 *               goes to -->
 *
 *               modify-instance
 |
 *                   V
 *               <instance or name>-><slot-name>-><dummy-node>...
 |
 *                                                   V
 *                                            <value-expression>...
 *
 *               (duplicate-instance <instance> [to <new-name>] <slot-override>*)
 *               goes to -->
 *
 *               duplicate-instance
 |
 *                   V
 *               <instance or name>-><new-name>-><slot-name>-><dummy-node>...
 *                                       OR                         |
 *                                   (gensym*)                      V
 *                                                        <value-expression>...
 *
 *************************************************************************************/
globle core_expression_object *ParseInitializeInstance(void *theEnv, core_expression_object *top, char *readSource)
{
    int error, fcalltype, readclass;

    if((top->value == (void *)core_lookup_function(theEnv, "make-instance")) ||
       (top->value == (void *)core_lookup_function(theEnv, "active-make-instance")))
    {
        fcalltype = MAKE_TYPE;
    }
    else if((top->value == (void *)core_lookup_function(theEnv, "initialize-instance")) ||
            (top->value == (void *)core_lookup_function(theEnv, "active-initialize-instance")))
    {
        fcalltype = INITIALIZE_TYPE;
    }
    else if((top->value == (void *)core_lookup_function(theEnv, "modify-instance")) ||
            (top->value == (void *)core_lookup_function(theEnv, "active-modify-instance")) ||
            (top->value == (void *)core_lookup_function(theEnv, "message-modify-instance")) ||
            (top->value == (void *)core_lookup_function(theEnv, "active-message-modify-instance")))
    {
        fcalltype = MODIFY_TYPE;
    }
    else
    {
        fcalltype = DUPLICATE_TYPE;
    }

    core_increment_pp_buffer_depth(theEnv, 3);
    error = FALSE;

    if( top->type == UNKNOWN_VALUE )
    {
        top->type = FCALL;
    }
    else
    {
        core_save_pp_buffer(theEnv, " ");
    }

    top->args = parse_args(theEnv, readSource, &error);

    if( error )
    {
        goto ParseInitializeInstanceError;
    }
    else if( top->args == NULL )
    {
        error_syntax(theEnv, "instance");
        goto ParseInitializeInstanceError;
    }

    core_save_pp_buffer(theEnv, " ");

    if( fcalltype == MAKE_TYPE )
    {
        /* ======================================
         *  Handle the case of anonymous instances
         *  where the name was not specified
         *  ====================================== */
        if((top->args->type != ATOM) ? FALSE :
           (strcmp(to_string(top->args->value), CLASS_RLN) == 0))
        {
            top->args->next_arg = parse_args(theEnv, readSource, &error);

            if( error == TRUE )
            {
                goto ParseInitializeInstanceError;
            }

            if( top->args->next_arg == NULL )
            {
                error_syntax(theEnv, "instance class");
                goto ParseInitializeInstanceError;
            }

            if((top->args->next_arg->type != ATOM) ? TRUE :
               (strcmp(to_string(top->args->next_arg->value), CLASS_RLN) != 0))
            {
                top->args->type = FCALL;
                top->args->value = (void *)core_lookup_function(theEnv, "gensym*");
                readclass = FALSE;
            }
            else
            {
                readclass = TRUE;
            }
        }
        else
        {
            core_get_token(theEnv, readSource, &DefclassData(theEnv)->ObjectParseToken);

            if((core_get_type(DefclassData(theEnv)->ObjectParseToken) != ATOM) ? TRUE :
               (strcmp(CLASS_RLN, core_convert_data_to_string(DefclassData(theEnv)->ObjectParseToken)) != 0))
            {
                error_syntax(theEnv, "make-instance");
                goto ParseInitializeInstanceError;
            }

            core_save_pp_buffer(theEnv, " ");
            readclass = TRUE;
        }

        if( readclass )
        {
            top->args->next_arg = parse_args(theEnv, readSource, &error);

            if( error )
            {
                goto ParseInitializeInstanceError;
            }

            if( top->args->next_arg == NULL )
            {
                error_syntax(theEnv, "instance class");
                goto ParseInitializeInstanceError;
            }
        }

        /* ==============================================
         *  If the class name is a constant, go ahead and
         *  look it up now and replace it with the pointer
         *  ============================================== */
        if( ReplaceClassNameWithReference(theEnv, top->args->next_arg) == FALSE )
        {
            goto ParseInitializeInstanceError;
        }

        core_inject_ws_into_pp_buffer(theEnv);
        core_get_token(theEnv, readSource, &DefclassData(theEnv)->ObjectParseToken);
        top->args->next_arg->next_arg =
            ParseSlotOverrides(theEnv, readSource, &error);
    }
    else
    {
        core_inject_ws_into_pp_buffer(theEnv);
        core_get_token(theEnv, readSource, &DefclassData(theEnv)->ObjectParseToken);

        if( fcalltype == DUPLICATE_TYPE )
        {
            if((DefclassData(theEnv)->ObjectParseToken.type != ATOM) ? FALSE :
               (strcmp(core_convert_data_to_string(DefclassData(theEnv)->ObjectParseToken), DUPLICATE_NAME_REF) == 0))
            {
                core_backup_pp_buffer(theEnv);
                core_backup_pp_buffer(theEnv);
                core_save_pp_buffer(theEnv, DefclassData(theEnv)->ObjectParseToken.pp);
                core_save_pp_buffer(theEnv, " ");
                top->args->next_arg = parse_args(theEnv, readSource, &error);

                if( error )
                {
                    goto ParseInitializeInstanceError;
                }

                if( top->args->next_arg == NULL )
                {
                    error_syntax(theEnv, "instance name");
                    goto ParseInitializeInstanceError;
                }

                core_inject_ws_into_pp_buffer(theEnv);
                core_get_token(theEnv, readSource, &DefclassData(theEnv)->ObjectParseToken);
            }
            else
            {
                top->args->next_arg = core_generate_constant(theEnv, FCALL, (void *)core_lookup_function(theEnv, "gensym*"));
            }

            top->args->next_arg->next_arg = ParseSlotOverrides(theEnv, readSource, &error);
        }
        else
        {
            top->args->next_arg = ParseSlotOverrides(theEnv, readSource, &error);
        }
    }

    if( error )
    {
        goto ParseInitializeInstanceError;
    }

    if( core_get_type(DefclassData(theEnv)->ObjectParseToken) != RPAREN )
    {
        error_syntax(theEnv, "slot-override");
        goto ParseInitializeInstanceError;
    }

    core_decrement_pp_buffer_depth(theEnv, 3);
    return(top);

ParseInitializeInstanceError:
    core_set_eval_error(theEnv, TRUE);
    core_return_expression(theEnv, top);
    core_decrement_pp_buffer_depth(theEnv, 3);
    return(NULL);
}

/********************************************************************************
 *  NAME         : ParseSlotOverrides
 *  DESCRIPTION  : Forms expressions for slot-overrides
 *  INPUTS       : 1) The logical name of the input
 *              2) Caller's buffer for error flkag
 *  RETURNS      : Address override expressions, NULL
 *                if none or error.
 *  SIDE EFFECTS : Slot-expression built
 *              Caller's error flag set
 *  NOTES        : <slot-override> ::= (<slot-name> <value>*)*
 *
 *              goes to
 *
 *              <slot-name> --> <dummy-node> --> <slot-name> --> <dummy-node>...
 |
 *                                    V
 *                            <value-expression> --> <value-expression> --> ...
 *
 *              Assumes first token has already been scanned
 ********************************************************************************/
globle core_expression_object *ParseSlotOverrides(void *theEnv, char *readSource, int *error)
{
    core_expression_object *top = NULL, *bot = NULL, *theExp;

    while( core_get_type(DefclassData(theEnv)->ObjectParseToken) == LPAREN )
    {
        *error = FALSE;
        theExp = parse_args(theEnv, readSource, error);

        if( *error == TRUE )
        {
            core_return_expression(theEnv, top);
            return(NULL);
        }
        else if( theExp == NULL )
        {
            error_syntax(theEnv, "slot-override");
            *error = TRUE;
            core_return_expression(theEnv, top);
            core_set_eval_error(theEnv, TRUE);
            return(NULL);
        }

        theExp->next_arg = core_generate_constant(theEnv, ATOM, get_true(theEnv));

        if( collect_args(theEnv, theExp->next_arg, readSource) == NULL )
        {
            *error = TRUE;
            core_return_expression(theEnv, top);
            return(NULL);
        }

        if( top == NULL )
        {
            top = theExp;
        }
        else
        {
            bot->next_arg = theExp;
        }

        bot = theExp->next_arg;
        core_inject_ws_into_pp_buffer(theEnv);
        core_get_token(theEnv, readSource, &DefclassData(theEnv)->ObjectParseToken);
    }

    core_backup_pp_buffer(theEnv);
    core_backup_pp_buffer(theEnv);
    core_save_pp_buffer(theEnv, DefclassData(theEnv)->ObjectParseToken.pp);
    return(top);
}

/****************************************************************************
 *  NAME         : ParseSimpleInstance
 *  DESCRIPTION  : Parses instances from file for load-instances
 *                into an core_expression_object forms that
 *                can later be evaluated with core_eval_expression(theEnv,)
 *  INPUTS       : 1) The address of the top node of the expression
 *                 containing the make-instance function call
 *              2) The logical name of the input source
 *  RETURNS      : The address of the modified expression, or NULL
 *                 if there is an error
 *  SIDE EFFECTS : The expression is enhanced to include all
 *                 aspects of the make-instance call
 *                 (slot-overrides etc.)
 *              The "top" expression is deleted on errors.
 *  NOTES        : The name, class, values etc. must be constants.
 *
 *              This function parses a make-instance call into
 *              an expression of the following form :
 *
 *               (make-instance <instance> of <class> <slot-override>*)
 *               where <slot-override> ::= (<slot-name> <expression>+)
 *
 *               goes to -->
 *
 *               make-instance
 |
 *                   V
 *               <instance-name>-><class-name>-><slot-name>-><dummy-node>...
 |
 *                                                              V
 *                                                       <value-expression>...
 *
 ****************************************************************************/
globle core_expression_object *ParseSimpleInstance(void *theEnv, core_expression_object *top, char *readSource)
{
    core_expression_object *theExp, *vals = NULL, *vbot, *tval;
    unsigned short type;

    core_get_token(theEnv, readSource, &DefclassData(theEnv)->ObjectParseToken);

    if((core_get_type(DefclassData(theEnv)->ObjectParseToken) != INSTANCE_NAME) &&
       (core_get_type(DefclassData(theEnv)->ObjectParseToken) != ATOM))
    {
        goto MakeInstanceError;
    }

    if((core_get_type(DefclassData(theEnv)->ObjectParseToken) == ATOM) &&
       (strcmp(CLASS_RLN, core_convert_data_to_string(DefclassData(theEnv)->ObjectParseToken)) == 0))
    {
        top->args = core_generate_constant(theEnv, FCALL,
                                   (void *)core_lookup_function(theEnv, "gensym*"));
    }
    else
    {
        top->args = core_generate_constant(theEnv, INSTANCE_NAME,
                                   (void *)core_get_value(DefclassData(theEnv)->ObjectParseToken));
        core_get_token(theEnv, readSource, &DefclassData(theEnv)->ObjectParseToken);

        if((core_get_type(DefclassData(theEnv)->ObjectParseToken) != ATOM) ? TRUE :
           (strcmp(CLASS_RLN, core_convert_data_to_string(DefclassData(theEnv)->ObjectParseToken)) != 0))
        {
            goto MakeInstanceError;
        }
    }

    core_get_token(theEnv, readSource, &DefclassData(theEnv)->ObjectParseToken);

    if( core_get_type(DefclassData(theEnv)->ObjectParseToken) != ATOM )
    {
        goto MakeInstanceError;
    }

    top->args->next_arg =
        core_generate_constant(theEnv, ATOM, (void *)core_get_value(DefclassData(theEnv)->ObjectParseToken));
    theExp = top->args->next_arg;

    if( ReplaceClassNameWithReference(theEnv, theExp) == FALSE )
    {
        goto MakeInstanceError;
    }

    core_get_token(theEnv, readSource, &DefclassData(theEnv)->ObjectParseToken);

    while( core_get_type(DefclassData(theEnv)->ObjectParseToken) == LPAREN )
    {
        core_get_token(theEnv, readSource, &DefclassData(theEnv)->ObjectParseToken);

        if( core_get_type(DefclassData(theEnv)->ObjectParseToken) != ATOM )
        {
            goto SlotOverrideError;
        }

        theExp->next_arg = core_generate_constant(theEnv, ATOM, (void *)core_get_value(DefclassData(theEnv)->ObjectParseToken));
        theExp->next_arg->next_arg = core_generate_constant(theEnv, ATOM, get_true(theEnv));
        theExp = theExp->next_arg->next_arg;
        core_get_token(theEnv, readSource, &DefclassData(theEnv)->ObjectParseToken);
        vbot = NULL;

        while( core_get_type(DefclassData(theEnv)->ObjectParseToken) != RPAREN )
        {
            type = core_get_type(DefclassData(theEnv)->ObjectParseToken);

            if( type == LPAREN )
            {
                core_get_token(theEnv, readSource, &DefclassData(theEnv)->ObjectParseToken);

                if((core_get_type(DefclassData(theEnv)->ObjectParseToken) != ATOM) ? TRUE :
                   (strcmp(to_string(DefclassData(theEnv)->ObjectParseToken.value), FUNC_NAME_CREATE_LIST) != 0))
                {
                    goto SlotOverrideError;
                }

                core_get_token(theEnv, readSource, &DefclassData(theEnv)->ObjectParseToken);

                if( core_get_type(DefclassData(theEnv)->ObjectParseToken) != RPAREN )
                {
                    goto SlotOverrideError;
                }

                tval = core_generate_constant(theEnv, FCALL, (void *)core_lookup_function(theEnv, FUNC_NAME_CREATE_LIST));
            }
            else
            {
                if((type != ATOM) && (type != STRING) &&
                   (type != FLOAT) && (type != INTEGER) && (type != INSTANCE_NAME))
                {
                    goto SlotOverrideError;
                }

                tval = core_generate_constant(theEnv, type, (void *)core_get_value(DefclassData(theEnv)->ObjectParseToken));
            }

            if( vals == NULL )
            {
                vals = tval;
            }
            else
            {
                vbot->next_arg = tval;
            }

            vbot = tval;
            core_get_token(theEnv, readSource, &DefclassData(theEnv)->ObjectParseToken);
        }

        theExp->args = vals;
        core_get_token(theEnv, readSource, &DefclassData(theEnv)->ObjectParseToken);
        vals = NULL;
    }

    if( core_get_type(DefclassData(theEnv)->ObjectParseToken) != RPAREN )
    {
        goto SlotOverrideError;
    }

    return(top);

MakeInstanceError:
    error_syntax(theEnv, "make-instance");
    core_set_eval_error(theEnv, TRUE);
    core_return_expression(theEnv, top);
    return(NULL);

SlotOverrideError:
    error_syntax(theEnv, "slot-override");
    core_set_eval_error(theEnv, TRUE);
    core_return_expression(theEnv, top);
    core_return_expression(theEnv, vals);
    return(NULL);
}

/* =========================================
 *****************************************
 *       INTERNALLY VISIBLE FUNCTIONS
 *  =========================================
 ***************************************** */

/***************************************************
 *  NAME         : ReplaceClassNameWithReference
 *  DESCRIPTION  : In parsing a make instance call,
 *              this function replaces a constant
 *              class name with an actual pointer
 *              to the class
 *  INPUTS       : The expression
 *  RETURNS      : TRUE if all OK, FALSE
 *              if class cannot be found
 *  SIDE EFFECTS : The expression type and value are
 *              modified if class is found
 *  NOTES        : Searches current nd imported
 *              modules for reference
 ***************************************************/
static BOOLEAN ReplaceClassNameWithReference(void *theEnv, core_expression_object *theExp)
{
    char *theClassName;
    void *theDefclass;

    if( theExp->type == ATOM )
    {
        theClassName = to_string(theExp->value);
        theDefclass = (void *)LookupDefclassInScope(theEnv, theClassName);

        if( theDefclass == NULL )
        {
            error_lookup(theEnv, "class", theClassName);
            return(FALSE);
        }

        if( EnvClassAbstractP(theEnv, theDefclass))
        {
            error_print_id(theEnv, "INSMNGR", 3, FALSE);
            print_router(theEnv, WERROR, "Cannot create instances of abstract class ");
            print_router(theEnv, WERROR, theClassName);
            print_router(theEnv, WERROR, ".\n");
            return(FALSE);
        }

        theExp->type = DEFCLASS_PTR;
        theExp->value = theDefclass;
    }

    return(TRUE);
}

#endif
