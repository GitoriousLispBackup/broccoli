/* Purpose: Provides functions for parsing the default
 *   attribute and determining default values based on
 *   slot constraints.                                       */

#define __CLASSES_MEMBERS_SOURCE__

#include "setup.h"

#include <stdio.h>
#define _STDIO_INCLUDED_
#include <stdlib.h>
#include <string.h>

#include "constant.h"
#include "constraints_kernel.h"
#include "constraints_query.h"
#include "type_list.h"
#include "classes_instances_kernel.h"
#include "parser_expressions.h"
#include "core_scanner.h"
#include "router.h"
#include "constraints_operators.h"
#include "core_environment.h"

#include "classes_members.h"

/**************************************
 * LOCAL FUNCTION PROTOTYPES
 ***************************************/

static void                    *FindDefaultValue(void *, int, CONSTRAINT_META *, void *);

/*******************************************************
 * DeriveDefaultFromConstraints: Returns an appropriate
 *   default value for the supplied constraints.
 ********************************************************/
globle void DeriveDefaultFromConstraints(void *theEnv, CONSTRAINT_META *constraints, core_data_object *theDefault, int list, int garbageList)
{
    unsigned short theType;
    unsigned long minFields;
    void *theValue;

    /*=============================================================
     * If no constraints are specified, then use the symbol nil as
     * a default for single field slots and a list of length
     * 0 as a default for list slots.
     *=============================================================*/

    if( constraints == NULL )
    {
        if( list )
        {
            SetpType(theDefault, LIST);
            SetpDOBegin(theDefault, 1);
            SetpDOEnd(theDefault, 0);

            if( garbageList )
            {
                SetpValue(theDefault, (void *)create_list(theEnv, 0L));
            }
            else
            {
                SetpValue(theDefault, (void *)create_sized_list(theEnv, 0L));
            }
        }
        else
        {
            theDefault->type = ATOM;
            theDefault->value = store_atom(theEnv, "nil");
        }

        return;
    }

    /*=========================================
     * Determine the default's type and value.
     *=========================================*/

    if( constraints->allow_any || constraints->allow_atom )
    {
        theType = ATOM;
        theValue = FindDefaultValue(theEnv, ATOM, constraints, store_atom(theEnv, "nil"));
    }

    else if( constraints->allow_string )
    {
        theType = STRING;
        theValue = FindDefaultValue(theEnv, STRING, constraints, store_atom(theEnv, ""));
    }

    else if( constraints->allow_integer )
    {
        theType = INTEGER;
        theValue = FindDefaultValue(theEnv, INTEGER, constraints, store_long(theEnv, 0LL));
    }

    else if( constraints->allow_float )
    {
        theType = FLOAT;
        theValue = FindDefaultValue(theEnv, FLOAT, constraints, store_double(theEnv, 0.0));
    }

#if OBJECT_SYSTEM
    else if( constraints->allow_instance_name )
    {
        theType = INSTANCE_NAME;
        theValue = FindDefaultValue(theEnv, INSTANCE_NAME, constraints, store_atom(theEnv, "nil"));
    }

    else if( constraints->allow_instance_pointer )
    {
        theType = INSTANCE_ADDRESS;
        theValue = (void *)&InstanceData(theEnv)->DummyInstance;
    }
#endif
    else if( constraints->allow_external_pointer )
    {
        theType = EXTERNAL_ADDRESS;
        theValue = store_external_address(theEnv, NULL, 0);
    }

    else
    {
        theType = ATOM;
        theValue = store_atom(theEnv, "nil");
    }

    /*=========================================================
     * If the default is for a list slot, then create a
     * list default value that satisfies the cardinality
     * constraints for the slot. The default value for a
     * list slot is a list of length 0.
     *=========================================================*/

    if( list )
    {
        if( constraints->min_fields == NULL )
        {
            minFields = 0;
        }
        else if( constraints->min_fields->value == SymbolData(theEnv)->negative_inf_atom )
        {
            minFields = 0;
        }
        else
        {
            minFields = (unsigned long)ValueToLong(constraints->min_fields->value);
        }

        SetpType(theDefault, LIST);
        SetpDOBegin(theDefault, 1);
        SetpDOEnd(theDefault, (long)minFields);

        if( garbageList )
        {
            SetpValue(theDefault, (void *)create_list(theEnv, minFields));
        }
        else
        {
            SetpValue(theDefault, (void *)create_sized_list(theEnv, minFields));
        }

        for(; minFields > 0; minFields-- )
        {
            SetListNodeType(GetpValue(theDefault), minFields, theType);
            SetListNodeValue(GetpValue(theDefault), minFields, theValue);
        }
    }
    else
    {
        theDefault->type = theType;
        theDefault->value = theValue;
    }
}

/**********************************************************************
 * FindDefaultValue: Searches the list of restriction values for a
 *   constraint to find a default value of the specified type. For
 *   example, if the attribute (allowed-symbols on off) was specified,
 *   then the symbol "on" would be used as a default value rather than
 *   the symbol "nil". For integers and floats, the range attribute is
 *   also used to select a suitable default value. If a minimum value
 *   was specified, then this value is used first followed by the
 *   maximum value.
 ************************************************************************/
static void *FindDefaultValue(void *theEnv, int theType, CONSTRAINT_META *theConstraints, void *standardDefault)
{
    struct core_expression *theList;

    /*=====================================================
     * Look on the the allowed values list to see if there
     * is a value of the requested type. Return the first
     * value found of the requested type.
     *=====================================================*/

    theList = theConstraints->restriction_list;

    while( theList != NULL )
    {
        if( theList->type == theType )
        {
            return(theList->value);
        }

        theList = theList->next_arg;
    }

    /*=============================================================
     * If no specific values were available for the default value,
     * and the type requested is a float or integer, then use the
     * range attribute to select a default value.
     *=============================================================*/

    if( theType == INTEGER )
    {
        if( theConstraints->min_value->type == INTEGER )
        {
            return(theConstraints->min_value->value);
        }
        else if( theConstraints->min_value->type == FLOAT )
        {
            return(store_long(theEnv, (long long)ValueToDouble(theConstraints->min_value->value)));
        }
        else if( theConstraints->max_value->type == INTEGER )
        {
            return(theConstraints->max_value->value);
        }
        else if( theConstraints->max_value->type == FLOAT )
        {
            return(store_long(theEnv, (long long)ValueToDouble(theConstraints->max_value->value)));
        }
    }
    else if( theType == FLOAT )
    {
        if( theConstraints->min_value->type == FLOAT )
        {
            return(theConstraints->min_value->value);
        }
        else if( theConstraints->min_value->type == INTEGER )
        {
            return(store_double(theEnv, (double)ValueToLong(theConstraints->min_value->value)));
        }
        else if( theConstraints->max_value->type == FLOAT )
        {
            return(theConstraints->max_value->value);
        }
        else if( theConstraints->max_value->type == INTEGER )
        {
            return(store_double(theEnv, (double)ValueToLong(theConstraints->max_value->value)));
        }
    }

    /*======================================
     * Use the standard default value (such
     * as nil if symbols are allowed).
     *======================================*/

    return(standardDefault);
}

/*********************************************
 * ParseDefault: Parses a default value list.
 **********************************************/
globle struct core_expression *ParseDefault(void *theEnv, char *readSource, int list, int dynamic, int evalStatic, int *noneSpecified, int *deriveSpecified, int *error)
{
    struct core_expression *defaultList = NULL, *lastDefault = NULL;
    struct core_expression *newItem, *tmpItem;
    struct token theToken;
    core_data_object theValue;
    CONSTRAINT_META *rv;
    int specialVarCode;

    *noneSpecified = FALSE;
    *deriveSpecified = FALSE;

    core_save_pp_buffer(theEnv, " ");
    core_get_token(theEnv, readSource, &theToken);

    /*===================================================
     * Read the items contained in the default attribute
     * until a closing right parenthesis is encountered.
     *===================================================*/

    while( theToken.type != RPAREN )
    {
        /*========================================
         * Get the next item in the default list.
         *========================================*/

        newItem = parse_atom_or_expression(theEnv, readSource, &theToken);

        if( newItem == NULL )
        {
            core_return_expression(theEnv, defaultList);
            *error = TRUE;
            return(NULL);
        }

        /*===========================================================
         * Check for invalid variable usage. With the expection of
         * ?NONE for the default attribute, local variables may not
         * be used within the default or default-dynamic attributes.
         *===========================================================*/

        if((newItem->type == SCALAR_VARIABLE) || (newItem->type == LIST_VARIABLE))
        {
            if( strcmp(to_string(newItem->value), "NONE") == 0 )
            {
                specialVarCode = 0;
            }
            else if( strcmp(to_string(newItem->value), "DERIVE") == 0 )
            {
                specialVarCode = 1;
            }
            else
            {
                specialVarCode = -1;
            }

            if((dynamic) ||
               (newItem->type == LIST_VARIABLE) ||
               (specialVarCode == -1) ||
               ((specialVarCode != -1) && (defaultList != NULL)))
            {
                if( dynamic ) {error_syntax(theEnv, "default-dynamic attribute");}
                else {error_syntax(theEnv, "default attribute");}

                core_return_expression(theEnv, newItem);
                core_return_expression(theEnv, defaultList);
                *error = TRUE;
                return(NULL);
            }

            core_return_expression(theEnv, newItem);

            /*============================================
             * Check for the closing right parenthesis of
             * the default or default dynamic attribute.
             *============================================*/

            core_get_token(theEnv, readSource, &theToken);

            if( theToken.type != RPAREN )
            {
                if( dynamic ) {error_syntax(theEnv, "default-dynamic attribute");}
                else {error_syntax(theEnv, "default attribute");}

                core_backup_pp_buffer(theEnv);
                core_save_pp_buffer(theEnv, " ");
                core_save_pp_buffer(theEnv, theToken.pp);
                *error = TRUE;
            }

            if( specialVarCode == 0 )
            {
                *noneSpecified = TRUE;
            }
            else
            {
                *deriveSpecified = TRUE;
            }

            return(NULL);
        }

        /*====================================================
         * Look to see if any variables have been used within
         * expressions contained within the default list.
         *====================================================*/

        if( core_does_expression_have_variables(newItem, FALSE) == TRUE )
        {
            core_return_expression(theEnv, defaultList);
            core_return_expression(theEnv, newItem);
            *error = TRUE;

            if( dynamic ) {error_syntax(theEnv, "default-dynamic attribute");}
            else {error_syntax(theEnv, "default attribute");}

            return(NULL);
        }

        /*============================================
         * Add the default value to the default list.
         *============================================*/

        if( lastDefault == NULL )
        {
            defaultList = newItem;
        }
        else
        {
            lastDefault->next_arg = newItem;
        }

        lastDefault = newItem;

        /*=======================================
         * Begin parsing the next default value.
         *=======================================*/

        core_save_pp_buffer(theEnv, " ");
        core_get_token(theEnv, readSource, &theToken);
    }

    /*=====================================
     * Fix up pretty print representation.
     *=====================================*/

    core_backup_pp_buffer(theEnv);
    core_backup_pp_buffer(theEnv);
    core_save_pp_buffer(theEnv, ")");

    /*=========================================
     * A single field slot's default attribute
     * must contain a single value.
     *=========================================*/

    if( list == FALSE )
    {
        if( defaultList == NULL )
        {
            *error = TRUE;
        }
        else if( defaultList->next_arg != NULL )
        {
            *error = TRUE;
        }
        else
        {
            rv = convert_expression_to_constraint(theEnv, defaultList);
            rv->allow_list = FALSE;

            if( is_satisfiable(rv)) {*error = TRUE;}

            removeConstraint(theEnv, rv);
        }

        if( *error )
        {
            error_print_id(theEnv, "DEFAULT", 1, TRUE);
            print_router(theEnv, WERROR, "The default value for a single field slot must be a single field value\n");
            core_return_expression(theEnv, defaultList);
            return(NULL);
        }
    }

    /*=======================================================
     * If the dynamic-default attribute is not being parsed,
     * evaluate the expressions to make the default value.
     *=======================================================*/

    if( dynamic || (!evalStatic) || (defaultList == NULL)) {return(defaultList);}

    tmpItem = defaultList;
    newItem = defaultList;

    defaultList = NULL;

    while( newItem != NULL )
    {
        core_set_eval_error(theEnv, FALSE);

        if( core_eval_expression(theEnv, newItem, &theValue)) {*error = TRUE;}

        if((theValue.type == LIST) &&
           (list == FALSE) &&
           (*error == FALSE))
        {
            error_print_id(theEnv, "DEFAULT", 1, TRUE);
            print_router(theEnv, WERROR, "The default value for a single field slot must be a single field value\n");
            *error = TRUE;
        }

        if( *error )
        {
            core_return_expression(theEnv, tmpItem);
            core_return_expression(theEnv, defaultList);
            *error = TRUE;
            return(NULL);
        }

        lastDefault = core_convert_data_to_expression(theEnv, &theValue);

        defaultList = AppendExpressions(defaultList, lastDefault);

        newItem = newItem->next_arg;
    }

    core_return_expression(theEnv, tmpItem);

    /*==========================
     * Return the default list.
     *==========================*/

    return(defaultList);
}
