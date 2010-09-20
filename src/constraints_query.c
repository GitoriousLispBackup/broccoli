/* Purpose: Provides functions for constraint checking of
 *   data types.                                             */

#define __CONSTRAINTS_QUERY_SOURCE__

#include <stdio.h>
#define _STDIO_INCLUDED_
#include <stdlib.h>

#include "setup.h"

#include "router.h"
#include "type_list.h"
#include "core_environment.h"
#include "core_functions.h"
#include "constraints_operators.h"
#if OBJECT_SYSTEM
#include "classes_instances_kernel.h"
#include "funcs_instance.h"
#include "classes_kernel.h"
#include "classes_meta.h"
#endif

#include "constraints_query.h"

/**************************************
 * LOCAL FUNCTION PROTOTYPES
 ***************************************/

static BOOLEAN _is_valid_range(void *, int, int, CONSTRAINT_META *);
static BOOLEAN _is_valid_return_type(int, CONSTRAINT_META *);
static BOOLEAN _is_valid_type(int, CONSTRAINT_META *);
static BOOLEAN _is_in_range(void *, int, void *, CONSTRAINT_META *);
static BOOLEAN _is_valid_cardinality(void *, long, CONSTRAINT_META *);

/*****************************************************
 * isValidReturnType: Checks a functions return
 *   type against a set of permissable return values.
 *   Returns TRUE if the return type is included
 *   among the permissible values, otherwise FALSE.
 ******************************************************/
static BOOLEAN _is_valid_return_type(int functionReturnType, CONSTRAINT_META *constraints)
{
    if( constraints == NULL )
    {
        return(TRUE);
    }

    if( constraints->allow_any )
    {
        return(TRUE);
    }

    switch( functionReturnType )
    {
    case RT_CHAR:
    case RT_ATOM:
    case RT_BOOL:

        if( constraints->allow_atom )
        {
            return(TRUE);
        }
        else
        {
            return(FALSE);
        }

    case RT_STRING:

        if( constraints->allow_string )
        {
            return(TRUE);
        }
        else
        {
            return(FALSE);
        }

    case RT_ATOM_STRING_INST:

        if((constraints->allow_atom) ||
           (constraints->allow_string) ||
           (constraints->allow_instance_name))
        {
            return(TRUE);
        }
        else
        {
            return(FALSE);
        }

    case RT_ATOM_STRING:

        if((constraints->allow_atom) || (constraints->allow_string))
        {
            return(TRUE);
        }
        else
        {
            return(FALSE);
        }

    case RT_DOUBLE:
    case RT_FLOAT:

        if( constraints->allow_float )
        {
            return(TRUE);
        }
        else
        {
            return(FALSE);
        }

    case RT_INT:
    case RT_LONG:

        if( constraints->allow_integer )
        {
            return(TRUE);
        }
        else
        {
            return(FALSE);
        }

    case RT_INT_FLOAT:

        if((constraints->allow_integer) || (constraints->allow_float))
        {
            return(TRUE);
        }
        else
        {
            return(FALSE);
        }

    case RT_LIST:

        if( constraints->allow_list )
        {
            return(TRUE);
        }
        else
        {
            return(FALSE);
        }

    case RT_EXT_ADDRESS:

        if( constraints->allow_external_pointer )
        {
            return(TRUE);
        }
        else
        {
            return(FALSE);
        }

    case RT_INST_ADDRESS:

        if( constraints->allow_instance_pointer )
        {
            return(TRUE);
        }
        else
        {
            return(FALSE);
        }

    case RT_INSTANCE:

        if( constraints->allow_instance_name )
        {
            return(TRUE);
        }
        else
        {
            return(FALSE);
        }

    case RT_UNKNOWN:
        return(TRUE);

    case RT_VOID:

        if( constraints->allow_void )
        {
            return(TRUE);
        }
    }

    return(TRUE);
}

/***************************************************
 * isValidType: Determines if a primitive
 *   data type satisfies the type constraint fields
 *   of aconstraint record.
 ****************************************************/
static BOOLEAN _is_valid_type(int type, CONSTRAINT_META *constraints)
{
    if( type == RVOID )
    {
        return(FALSE);
    }

    if( constraints == NULL )
    {
        return(TRUE);
    }

    if( constraints->allow_any == TRUE )
    {
        return(TRUE);
    }

    if((type == ATOM) && (constraints->allow_atom != TRUE))
    {
        return(FALSE);
    }

    if((type == STRING) && (constraints->allow_string != TRUE))
    {
        return(FALSE);
    }

    if((type == FLOAT) && (constraints->allow_float != TRUE))
    {
        return(FALSE);
    }

    if((type == INTEGER) && (constraints->allow_integer != TRUE))
    {
        return(FALSE);
    }

#if OBJECT_SYSTEM

    if((type == INSTANCE_NAME) && (constraints->allow_instance_name != TRUE))
    {
        return(FALSE);
    }

    if((type == INSTANCE_ADDRESS) && (constraints->allow_instance_pointer != TRUE))
    {
        return(FALSE);
    }

#endif

    if((type == EXTERNAL_ADDRESS) && (constraints->allow_external_pointer != TRUE))
    {
        return(FALSE);
    }

    if((type == RVOID) && (constraints->allow_void != TRUE))
    {
        return(FALSE);
    }

    return(TRUE);
}

/*******************************************************
 * isValidCardinality: Determines if an integer
 *   falls within the range of allowed cardinalities
 *   for a constraint record.
 ********************************************************/
static BOOLEAN _is_valid_cardinality(void *env, long number, CONSTRAINT_META *constraints)
{
    /*=========================================
     * If the constraint record is NULL, there
     * are no cardinality restrictions.
     *=========================================*/

    if( constraints == NULL )
    {
        return(TRUE);
    }

    /*==================================
     * Determine if the integer is less
     * than the minimum cardinality.
     *==================================*/

    if( constraints->min_fields != NULL )
    {
        if( constraints->min_fields->value != get_atom_data(env)->negative_inf_atom )
        {
            if( number < to_long(constraints->min_fields->value))
            {
                return(FALSE);
            }
        }
    }

    /*=====================================
     * Determine if the integer is greater
     * than the maximum cardinality.
     *=====================================*/

    if( constraints->max_fields != NULL )
    {
        if( constraints->max_fields->value != get_atom_data(env)->positive_inf_atom )
        {
            if( number > to_long(constraints->max_fields->value))
            {
                return(FALSE);
            }
        }
    }

    /*=========================================================
     * The integer falls within the allowed cardinality range.
     *=========================================================*/

    return(TRUE);
}

/****************************************************************
 * isValidRange: Determines if a range
 *   of numbers could possibly fall within the range of allowed
 *   cardinalities for a constraint record. Returns TRUE if at
 *   least one of the numbers in the range is within the allowed
 *   cardinality, otherwise FALSE is returned.
 *****************************************************************/
static BOOLEAN _is_valid_range(void *env, int min, int max, CONSTRAINT_META *constraints)
{
    /*=========================================
     * If the constraint record is NULL, there
     * are no cardinality restrictions.
     *=========================================*/

    if( constraints == NULL )
    {
        return(TRUE);
    }

    /*===============================================================
     * If the minimum value of the range is greater than the maximum
     * value of the cardinality, then there are no numbers in the
     * range which could fall within the cardinality range, and so
     * FALSE is returned.
     *===============================================================*/

    if( constraints->max_fields != NULL )
    {
        if( constraints->max_fields->value != get_atom_data(env)->positive_inf_atom )
        {
            if( min > to_long(constraints->max_fields->value))
            {
                return(FALSE);
            }
        }
    }

    /*===============================================================
     * If the maximum value of the range is less than the minimum
     * value of the cardinality, then there are no numbers in the
     * range which could fall within the cardinality range, and so
     * FALSE is returned. A maximum range value of -1 indicates that
     * the maximum possible value of the range is positive infinity.
     *===============================================================*/

    if((constraints->min_fields != NULL) && (max != -1))
    {
        if( constraints->min_fields->value != get_atom_data(env)->negative_inf_atom )
        {
            if( max < to_long(constraints->min_fields->value))
            {
                return(FALSE);
            }
        }
    }

    /*=============================================
     * At least one number in the specified range
     * falls within the allowed cardinality range.
     *=============================================*/

    return(TRUE);
}

/*********************************************************************
 * is_type_allowed: Determines if a primitive data type
 *   satisfies the allowed-... constraint fields of a constraint
 *   record. Returns TRUE if the constraints are satisfied, otherwise
 *   FALSE is returned.
 **********************************************************************/
BOOLEAN is_type_allowed(int type, void *vPtr, CONSTRAINT_META *constraints)
{
    struct core_expression *tmpPtr;

    /*=========================================
     * If the constraint record is NULL, there
     * are no allowed-... restrictions.
     *=========================================*/

    if( constraints == NULL )
    {
        return(TRUE);
    }

    /*=====================================================
     * Determine if there are any allowed-... restrictions
     * for the type of the value being checked.
     *=====================================================*/

    switch( type )
    {
    case ATOM:

        if((constraints->restrict_atom == FALSE) &&
           (constraints->restrict_any == FALSE))
        {
            return(TRUE);
        }

        break;

#if OBJECT_SYSTEM
    case INSTANCE_NAME:

        if((constraints->restrict_instance_name == FALSE) &&
           (constraints->restrict_any == FALSE))
        {
            return(TRUE);
        }

        break;
#endif

    case STRING:

        if((constraints->restrict_string == FALSE) &&
           (constraints->restrict_any == FALSE))
        {
            return(TRUE);
        }

        break;

    case INTEGER:

        if((constraints->restrict_integer == FALSE) &&
           (constraints->restrict_any == FALSE))
        {
            return(TRUE);
        }

        break;

    case FLOAT:

        if((constraints->restrict_float == FALSE) &&
           (constraints->restrict_any == FALSE))
        {
            return(TRUE);
        }

        break;

    default:
        return(TRUE);
    }

    /*=========================================================
     * Search through the restriction list to see if the value
     * matches one of the allowed values in the list.
     *=========================================================*/

    for( tmpPtr = constraints->restriction_list;
         tmpPtr != NULL;
         tmpPtr = tmpPtr->next_arg )
    {
        if((tmpPtr->type == type) && (tmpPtr->value == vPtr))
        {
            return(TRUE);
        }
    }

    /*====================================================
     * If the value wasn't found in the list, then return
     * FALSE because the constraint has been violated.
     *====================================================*/

    return(FALSE);
}

/*********************************************************************
 * is_class_allowed: Determines if a primitive data type
 *   satisfies the allowed-classes constraint fields of a constraint
 *   record. Returns TRUE if the constraints are satisfied, otherwise
 *   FALSE is returned.
 **********************************************************************/
BOOLEAN is_class_allowed(void *env, int type, void *vPtr, CONSTRAINT_META *constraints)
{
#if OBJECT_SYSTEM
    struct core_expression *tmpPtr;
    INSTANCE_TYPE *ins;
    DEFCLASS *insClass, *cmpClass;

    /*=========================================
     * If the constraint record is NULL, there
     * is no allowed-classes restriction.
     *=========================================*/

    if( constraints == NULL )
    {
        return(TRUE);
    }

    /*======================================
     * The constraint is satisfied if there
     * aren't any class restrictions.
     *======================================*/

    if( constraints->class_list == NULL )
    {
        return(TRUE);
    }

    /*==================================
     * Class restrictions only apply to
     * instances and instance names.
     *==================================*/

    if((type != INSTANCE_ADDRESS) && (type != INSTANCE_NAME))
    {
        return(TRUE);
    }

    /*=============================================
     * If an instance name is specified, determine
     * whether the instance exists.
     *=============================================*/

    if( type == INSTANCE_ADDRESS )
    {
        ins = (INSTANCE_TYPE *)vPtr;
    }
    else
    {
        ins = FindInstanceBySymbol(env, (ATOM_HN *)vPtr);
    }

    if( ins == NULL )
    {
        return(FALSE);
    }

    /*======================================================
     * Search through the class list to see if the instance
     * belongs to one of the allowed classes in the list.
     *======================================================*/

    insClass = (DEFCLASS *)EnvGetInstanceClass(env, ins);

    for( tmpPtr = constraints->class_list;
         tmpPtr != NULL;
         tmpPtr = tmpPtr->next_arg )
    {
        cmpClass = (DEFCLASS *)EnvFindDefclass(env, to_string(tmpPtr->value));

        if( cmpClass == NULL )
        {
            continue;
        }

        if( cmpClass == insClass )
        {
            return(TRUE);
        }

        if( EnvSubclassP(env, insClass, cmpClass))
        {
            return(TRUE);
        }
    }

    /*=========================================================
     * If a parent class wasn't found in the list, then return
     * FALSE because the constraint has been violated.
     *=========================================================*/

    return(FALSE);

#else

#if MAC_MCW || WIN_MCW || MAC_XCD
#pragma unused(env)
#pragma unused(type)
#pragma unused(vPtr)
#pragma unused(constraints)
#endif

    return(TRUE);

#endif
}

/************************************************************
 * doesFallInRange: Determines if a primitive data type
 *   satisfies the range constraint of a constraint record.
 *************************************************************/
static BOOLEAN _is_in_range(void *env, int type, void *vPtr, CONSTRAINT_META *constraints)
{
    struct core_expression *minList, *maxList;

    /*===================================
     * If the constraint record is NULL,
     * there are no range restrictions.
     *===================================*/

    if( constraints == NULL )
    {
        return(TRUE);
    }

    /*============================================
     * If the value being checked isn't a number,
     * then the range restrictions don't apply.
     *============================================*/

    if((type != INTEGER) && (type != FLOAT))
    {
        return(TRUE);
    }

    /*=====================================================
     * Check each of the range restrictions to see if the
     * number falls within at least one of the allowed
     * ranges. If it falls within one of the ranges, then
     * return TRUE since the constraint is satisifed.
     *=====================================================*/

    minList = constraints->min_value;
    maxList = constraints->max_value;

    while( minList != NULL )
    {
        if( core_numerical_compare(env, type, vPtr, minList->type, minList->value) == LESS_THAN )
        {
            minList = minList->next_arg;
            maxList = maxList->next_arg;
        }
        else if( core_numerical_compare(env, type, vPtr, maxList->type, maxList->value) == GREATER_THAN )
        {
            minList = minList->next_arg;
            maxList = maxList->next_arg;
        }
        else
        {
            return(TRUE);
        }
    }

    /*===========================================
     * Return FALSE since the number didn't fall
     * within one of the allowed numeric ranges.
     *===========================================*/

    return(FALSE);
}

/***********************************************
 * report_constraint_violation_error: Generalized
 *   error message for constraint violations.
 ************************************************/
void report_constraint_violation_error(void *env, char *theWhat, char *thePlace, int command, int thePattern, struct atom_hash_node *theSlot, int theField, int violationType, CONSTRAINT_META *theConstraint, int printPrelude)
{
    /*======================================================
     * Don't print anything other than the tail explanation
     * of the error unless asked to do so.
     *======================================================*/

    if( printPrelude )
    {
        /*===================================
         * Print the name of the thing which
         * caused the constraint violation.
         *===================================*/

        if( violationType == FUNCTION_RETURN_TYPE_VIOLATION )
        {
            error_print_id(env, "CONSTRAINTS", 1, TRUE);
            print_router(env, WERROR, "The function return value ");
        }
        else if( theWhat != NULL )
        {
            error_print_id(env, "CONSTRAINTS", 1, TRUE);
            print_router(env, WERROR, theWhat);
            print_router(env, WERROR, " ");
        }

        /*=======================================
         * Print the location of the thing which
         * caused the constraint violation.
         *=======================================*/

        if( thePlace != NULL )
        {
            print_router(env, WERROR, "found in ");

            if( command )
            {
                print_router(env, WERROR, "the ");
            }

            print_router(env, WERROR, thePlace);

            if( command )
            {
                print_router(env, WERROR, " command");
            }
        }

        /*================================================
         * If the violation occured in the LHS of a rule,
         * indicate which pattern was at fault.
         *================================================*/

        if( thePattern > 0 )
        {
            print_router(env, WERROR, "found in CE #");
            core_print_long(env, WERROR, (long int)thePattern);
        }
    }

    /*===============================================================
     * Indicate the type of constraint violation (type, range, etc).
     *===============================================================*/

    if((violationType == TYPE_VIOLATION) ||
       (violationType == FUNCTION_RETURN_TYPE_VIOLATION))
    {
        print_router(env, WERROR, "\ndoes not match the allowed types");
    }
    else if( violationType == RANGE_VIOLATION )
    {
        print_router(env, WERROR, "\ndoes not fall in the allowed range ");
    }
    else if( violationType == ALLOWED_VALUES_VIOLATION )
    {
        print_router(env, WERROR, "\ndoes not match the allowed values");
    }
    else if( violationType == CARDINALITY_VIOLATION )
    {
        print_router(env, WERROR, "\ndoes not satisfy the cardinality restrictions");
    }
    else if( violationType == ALLOWED_CLASSES_VIOLATION )
    {
        print_router(env, WERROR, "\ndoes not match the allowed classes");
    }

    /*==============================================
     * Print either the slot name or field position
     * where the constraint violation occured.
     *==============================================*/

    if( theSlot != NULL )
    {
        print_router(env, WERROR, " for slot ");
        print_router(env, WERROR, to_string(theSlot));
    }
    else if( theField > 0 )
    {
        print_router(env, WERROR, " for field #");
        core_print_long(env, WERROR, (long long)theField);
    }

    print_router(env, WERROR, ".\n");
}

/************************************************************
 * garner_violations: Given a value stored in a data
 *   object structure and a constraint record, determines if
 *   the data object satisfies the constraint record.
 *************************************************************/
int garner_violations(void *env, core_data_object *theData, CONSTRAINT_META *theConstraints)
{
    long i; /* 6.04 Bug Fix */
    int rv;
    struct node *list;

    if( theConstraints == NULL )
    {
        return(NO_VIOLATION);
    }

    if( theData->type == LIST )
    {
        if( _is_valid_cardinality(env, (theData->end - theData->begin) + 1,
                                  theConstraints) == FALSE )
        {
            return(CARDINALITY_VIOLATION);
        }

        list = ((struct list *)theData->value)->cell;

        for( i = theData->begin; i <= theData->end; i++ )
        {
            if((rv = garner_value_violations(env, list[i].type,
                                             list[i].value,
                                             theConstraints)) != NO_VIOLATION )
            {
                return(rv);
            }
        }

        return(NO_VIOLATION);
    }

    if( _is_valid_cardinality(env, 1L, theConstraints) == FALSE )
    {
        return(CARDINALITY_VIOLATION);
    }

    return(garner_value_violations(env, theData->type, theData->value, theConstraints));
}

/***************************************************************
 * garner_value_violations: Given a value and a constraint record,
 *   determines if the value satisfies the constraint record.
 ****************************************************************/
int garner_value_violations(void *env, int theType, void *val, CONSTRAINT_META *theConstraints)
{
    if( _is_valid_type(theType, theConstraints) == FALSE )
    {
        return(TYPE_VIOLATION);
    }

    else if( is_type_allowed(theType, val, theConstraints) == FALSE )
    {
        return(ALLOWED_VALUES_VIOLATION);
    }

    else if( is_class_allowed(env, theType, val, theConstraints) == FALSE )
    {
        return(ALLOWED_CLASSES_VIOLATION);
    }

    else if( _is_in_range(env, theType, val, theConstraints) == FALSE )
    {
        return(RANGE_VIOLATION);
    }

    else if( theType == FCALL )
    {
        if( _is_valid_return_type((int)core_get_return_type(val), theConstraints) == FALSE )
        {
            return(FUNCTION_RETURN_TYPE_VIOLATION);
        }
    }

    return(NO_VIOLATION);
}

/*******************************************************************
 * garner_expression_violations: Checks an expression and next_arg
 * links for constraint conflicts (args is not followed).
 ********************************************************************/
int garner_expression_violations(void *env, struct core_expression *expr, CONSTRAINT_META *theConstraints)
{
    struct core_expression *theExp;
    int min = 0, max = 0, vCode;

    /*===========================================================
     * Determine the minimum and maximum number of value which
     * can be derived from the expression chain (max of -1 means
     * positive infinity).
     *===========================================================*/

    for( theExp = expr ; theExp != NULL ; theExp = theExp->next_arg )
    {
        if( core_is_constant(theExp->type))
        {
            min++;
        }
        else if( theExp->type == FCALL )
        {
            if((core_get_expression_return_type(theExp) != RT_LIST) &&
               (core_get_expression_return_type(theExp) != RT_UNKNOWN))
            {
                min++;
            }
            else
            {
                max = -1;
            }
        }
        else
        {
            max = -1;
        }
    }

    /*====================================
     * Check for a cardinality violation.
     *====================================*/

    if( max == 0 )
    {
        max = min;
    }

    if( _is_valid_range(env, min, max, theConstraints) == FALSE )
    {
        return(CARDINALITY_VIOLATION);
    }

    /*========================================
     * Check for other constraint violations.
     *========================================*/

    for( theExp = expr ; theExp != NULL ; theExp = theExp->next_arg )
    {
        vCode = garner_value_violations(env, theExp->type, theExp->value, theConstraints);

        if( vCode != NO_VIOLATION )
        {
            return(vCode);
        }
    }

    return(NO_VIOLATION);
}

/****************************************************
 * is_satisfiable: Determines if a constraint
 *  record can still be satisfied by some value.
 *****************************************************/
BOOLEAN is_satisfiable(CONSTRAINT_META *theConstraint)
{
    if( theConstraint == NULL )
    {
        return(FALSE);
    }

    if((!theConstraint->allow_any) &&
       (!theConstraint->allow_atom) &&
       (!theConstraint->allow_string) &&
       (!theConstraint->allow_float) &&
       (!theConstraint->allow_integer) &&
       (!theConstraint->allow_instance_name) &&
       (!theConstraint->allow_instance_pointer) &&
       (!theConstraint->allow_list) &&
       (!theConstraint->allow_external_pointer) &&
       (!theConstraint->allow_void))
    {
        return(TRUE);
    }

    return(FALSE);
}
