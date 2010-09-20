/* Purpose: Provides functions for performing operations on
 *   constraint records including computing the intersection
 *   and union of constraint records.                        */

#define __CONSTRAINTS_OPERATORS_SOURCE__

#include "setup.h"

#include <stdio.h>
#define _STDIO_INCLUDED_
#include <stdlib.h>


#include "constant.h"
#include "core_environment.h"
#include "core_memory.h"
#include "router.h"
#include "core_functions.h"
#include "core_scanner.h"
#include "type_list.h"
#include "constraints_kernel.h"
#include "constraints_query.h"
#include "constraints_operators.h"

/**************************************
 * LOCAL FUNCTION PROTOTYPES
 ***************************************/

static void                                       _intersect_numbers(void *, CONSTRAINT_META *, CONSTRAINT_META *, CONSTRAINT_META *, int);
static void                                       _intersect_allowed_values(void *, CONSTRAINT_META *, CONSTRAINT_META *, CONSTRAINT_META *);
static void                                       _intersect_allowed_classes(void *, CONSTRAINT_META *, CONSTRAINT_META *, CONSTRAINT_META *);
static int                                        _lookup_constant(int, void *, int, struct core_expression *);
static void                                       _update_restrictions(CONSTRAINT_META *);
static void                                       _union_numbers_with_list(void *, struct core_expression *, struct core_expression *, struct core_expression **, struct core_expression **);
static void                                       _union_numbers(void *, CONSTRAINT_META *, CONSTRAINT_META *, CONSTRAINT_META *, int);
static struct core_expression                   * _add_to_union(void *, struct core_expression *, struct core_expression *, CONSTRAINT_META *);
static void                                       _union_allowed_values(void *, CONSTRAINT_META *, CONSTRAINT_META *, CONSTRAINT_META *);
static void                                       _union_allowed_classes(void *, CONSTRAINT_META *, CONSTRAINT_META *, CONSTRAINT_META *);
static int                                        _has_type_restriction(int, CONSTRAINT_META *);
static struct constraint_metadata   *             _clone_constraint(void *, CONSTRAINT_META *);
static void                                       _allow_all(CONSTRAINT_META *, int);
static void                                       _restrict_all(CONSTRAINT_META *, int);
static CONSTRAINT_META              *             _convert_function_to_constraint(void *, void *);

/*************************************************************
 * constraint_intersection: Creates a new constraint record that
 *   is the intersection of two other constraint records.
 **************************************************************/
struct constraint_metadata *constraint_intersection(void *env, CONSTRAINT_META *c1, CONSTRAINT_META *c2)
{
    struct constraint_metadata *rv;
    int c1Changed = FALSE, c2Changed = FALSE;

    /*=================================================
     * If both constraint records are NULL,then create
     * a constraint record that allows any value.
     *=================================================*/

    if((c1 == NULL) && (c2 == NULL))
    {
        rv = create_constraint(env);
        rv->allow_list = TRUE;
        return(rv);
    }

    /*=================================================
     * If one of the constraint records is NULL, then
     * the intersection is the other constraint record
     * (a NULL value means no constraints).
     *=================================================*/

    if( c1 == NULL ) {return(_clone_constraint(env, c2));}

    if( c2 == NULL ) {return(_clone_constraint(env, c1));}

    /*=================================
     * Create a new constraint record.
     *=================================*/

    rv = create_constraint(env);

    /*==============================
     * Intersect the allowed types.
     *==============================*/

    if((c1->allow_list != c2->allow_list) &&
       (c1->allow_scalar != c2->allow_scalar))
    {
        rv->allow_any = FALSE;
        return(rv);
    }

    if( c1->allow_list && c2->allow_list )
    {
        rv->allow_list = TRUE;
    }
    else
    {
        rv->allow_list = FALSE;
    }

    if( c1->allow_scalar && c2->allow_scalar )
    {
        rv->allow_scalar = TRUE;
    }
    else
    {
        rv->allow_scalar = FALSE;
    }

    if( c1->allow_any && c2->allow_any ) {rv->allow_any = TRUE;}
    else
    {
        if( c1->allow_any )
        {
            c1Changed = TRUE;
            _allow_all(c1, FALSE);
        }
        else if( c2->allow_any )
        {
            c2Changed = TRUE;
            _allow_all(c2, FALSE);
        }

        rv->allow_any = FALSE;
        rv->allow_atom = (c1->allow_atom && c2->allow_atom);
        rv->allow_string = (c1->allow_string && c2->allow_string);
        rv->allow_float = (c1->allow_float && c2->allow_float);
        rv->allow_integer = (c1->allow_integer && c2->allow_integer);
        rv->allow_instance_name = (c1->allow_instance_name && c2->allow_instance_name);
        rv->allow_instance_pointer = (c1->allow_instance_pointer && c2->allow_instance_pointer);
        rv->allow_external_pointer = (c1->allow_external_pointer && c2->allow_external_pointer);
        rv->allow_void = (c1->allow_void && c2->allow_void);
        rv->allow_list = (c1->allow_list && c2->allow_list);

        if( c1Changed ) {_allow_all(c1, TRUE);}

        if( c2Changed ) {_allow_all(c2, TRUE);}
    }

    /*=====================================
     * Intersect the allowed-values flags.
     *=====================================*/

    if( c1->restrict_any || c2->restrict_any ) {rv->restrict_any = TRUE;}
    else
    {
        rv->restrict_any = FALSE;
        rv->restrict_atom = (c1->restrict_atom || c2->restrict_atom);
        rv->restrict_string = (c1->restrict_string || c2->restrict_string);
        rv->restrict_float = (c1->restrict_float || c2->restrict_float);
        rv->restrict_integer = (c1->restrict_integer || c2->restrict_integer);
        rv->restrict_class = (c1->restrict_class || c2->restrict_class);
        rv->restrict_instance_name = (c1->restrict_instance_name || c2->restrict_instance_name);
    }

    /*==================================================
     * Intersect the allowed values list, allowed class
     * list, min and max values, and the range values.
     *==================================================*/

    _intersect_allowed_values(env, c1, c2, rv);
    _intersect_allowed_classes(env, c1, c2, rv);
    _intersect_numbers(env, c1, c2, rv, TRUE);
    _intersect_numbers(env, c1, c2, rv, FALSE);

    /*==========================================
     * Update the allowed-values flags based on
     * the previous intersection for allowed,
     * min and max, and range values.
     *==========================================*/

    _update_restrictions(rv);

    /*============================================
     * If lists are allowed, then intersect
     * the constraint record for them.
     *============================================*/

    if( rv->allow_list )
    {
        rv->list = constraint_intersection(env, c1->list, c2->list);

        if( is_satisfiable(rv->list))
        {
            rv->allow_list = FALSE;
        }
    }

    /*========================
     * Return the intersected
     * constraint record.
     *========================*/

    return(rv);
}

/************************************************
 * intersectAllowedValues: Creates the
 *   intersection of two allowed-values lists.
 *************************************************/
static void _intersect_allowed_values(void *env, CONSTRAINT_META *constraint1, CONSTRAINT_META *constraint2, CONSTRAINT_META *newConstraint)
{
    struct core_expression *list1, *list2;
    struct core_expression *theHead = NULL, *tmpExpr;

    /*===========================================
     * Loop through each value in allowed-values
     * list of the first constraint record. Add
     * each value to a list if it satisfies the
     * restrictions for both constraint records.
     *===========================================*/

    for( list1 = constraint1->restriction_list;
         list1 != NULL;
         list1 = list1->next_arg )
    {
        if( is_type_allowed(list1->type, list1->value, constraint1) &&
            is_type_allowed(list1->type, list1->value, constraint2))
        {
            tmpExpr = core_generate_constant(env, list1->type, list1->value);
            tmpExpr->next_arg = theHead;
            theHead = tmpExpr;
        }
    }

    /*===========================================
     * Loop through each value in allowed-values
     * list of the second constraint record. Add
     * each value to a list if it satisfies the
     * restrictions for both constraint records.
     *===========================================*/

    for( list2 = constraint2->restriction_list;
         list2 != NULL;
         list2 = list2->next_arg )
    {
        if( _lookup_constant(list2->type, list2->value, TRUE, theHead))
        {     /* The value is already in the list--Do nothing */
        }
        else if( is_type_allowed(list2->type, list2->value, constraint1) &&
                 is_type_allowed(list2->type, list2->value, constraint2))
        {
            tmpExpr = core_generate_constant(env, list2->type, list2->value);
            tmpExpr->next_arg = theHead;
            theHead = tmpExpr;
        }
    }

    /*================================================
     * Set the allowed values list for the constraint
     * record to the intersected values of the two
     * other constraint records.
     *================================================*/

    newConstraint->restriction_list = theHead;
}

/************************************************
 * intersectAllowedClasses: Creates the
 *   intersection of two allowed-classes lists.
 *************************************************/
static void _intersect_allowed_classes(void *env, CONSTRAINT_META *constraint1, CONSTRAINT_META *constraint2, CONSTRAINT_META *newConstraint)
{
#if OBJECT_SYSTEM
    struct core_expression *list1, *list2;
    struct core_expression *theHead = NULL, *tmpExpr;

    /*============================================
     * Loop through each value in allowed-classes
     * list of the first constraint record. Add
     * each value to a list if it satisfies the
     * restrictions for both constraint records.
     *============================================*/

    for( list1 = constraint1->class_list;
         list1 != NULL;
         list1 = list1->next_arg )
    {
        if( is_class_allowed(env, list1->type, list1->value, constraint1) &&
            is_class_allowed(env, list1->type, list1->value, constraint2))
        {
            tmpExpr = core_generate_constant(env, list1->type, list1->value);
            tmpExpr->next_arg = theHead;
            theHead = tmpExpr;
        }
    }

    /*============================================
     * Loop through each value in allowed-classes
     * list of the second constraint record. Add
     * each value to a list if it satisfies the
     * restrictions for both constraint records.
     *============================================*/

    for( list2 = constraint2->class_list;
         list2 != NULL;
         list2 = list2->next_arg )
    {
        if( _lookup_constant(list2->type, list2->value, TRUE, theHead))
        {     /* The value is already in the list--Do nothing */
        }
        else if( is_class_allowed(env, list2->type, list2->value, constraint1) &&
                 is_class_allowed(env, list2->type, list2->value, constraint2))
        {
            tmpExpr = core_generate_constant(env, list2->type, list2->value);
            tmpExpr->next_arg = theHead;
            theHead = tmpExpr;
        }
    }

    /*=================================================
     * Set the allowed classes list for the constraint
     * record to the intersected values of the two
     * other constraint records.
     *=================================================*/

    newConstraint->class_list = theHead;
#endif
}

/********************************************************
 * intersectNumerals: Creates the intersection
 *   of two range or two min/max-fields constraints.
 *********************************************************/
static void _intersect_numbers(void *env, CONSTRAINT_META *constraint1, CONSTRAINT_META *constraint2, CONSTRAINT_META *newConstraint, int range)
{
    struct core_expression *tmpmin1, *tmpmax1, *tmpmin2, *tmpmax2, *theMin, *theMax;
    struct core_expression *theMinList, *theMaxList, *lastMin = NULL, *lastMax = NULL;
    int cmaxmax, cminmin, cmaxmin, cminmax;

    /*==========================================
     * Initialize the new range/min/max values
     * for the intersection of the constraints.
     *==========================================*/

    theMinList = NULL;
    theMaxList = NULL;

    /*=================================
     * Determine the min/max values of
     * the first constraint record.
     *=================================*/

    if( range )
    {
        tmpmin1 = constraint1->min_value;
        tmpmax1 = constraint1->max_value;
    }
    else
    {
        tmpmin1 = constraint1->min_fields;
        tmpmax1 = constraint1->max_fields;
    }

    /*===========================================
     * Loop through each of range/min/max values
     * from the first constraint record.
     *===========================================*/

    for(;
        tmpmin1 != NULL;
        tmpmin1 = tmpmin1->next_arg, tmpmax1 = tmpmax1->next_arg )
    {
        /*============================================
         * Get the appropriate values from the second
         * constraint record for comparison.
         *============================================*/

        if( range )
        {
            tmpmin2 = constraint2->min_value;
            tmpmax2 = constraint2->max_value;
        }
        else
        {
            tmpmin2 = constraint2->min_fields;
            tmpmax2 = constraint2->max_fields;
        }

        /*================================================
         * Loop through each of range/min/max values from
         * the second constraint record comparing it to
         * the values from the first constraint record.
         *================================================*/

        for(;
            tmpmin2 != NULL;
            tmpmin2 = tmpmin2->next_arg, tmpmax2 = tmpmax2->next_arg )
        {
            /*==============================================
             * Determine the relationship between the four
             * combinations of min/max values (>, <, or =).
             *==============================================*/

            cmaxmax = core_numerical_compare(env, tmpmax1->type, tmpmax1->value,
                                             tmpmax2->type, tmpmax2->value);

            cminmin = core_numerical_compare(env, tmpmin1->type, tmpmin1->value,
                                             tmpmin2->type, tmpmin2->value);

            cmaxmin = core_numerical_compare(env, tmpmax1->type, tmpmax1->value,
                                             tmpmin2->type, tmpmin2->value);

            cminmax = core_numerical_compare(env, tmpmin1->type, tmpmin1->value,
                                             tmpmax2->type, tmpmax2->value);

            /*============================================
             * If the range/min/max values don't overlap,
             * then proceed to the next pair of numbers
             * to see if they overlap.
             *============================================*/

            if((cmaxmin == LESS_THAN) || (cminmax == GREATER_THAN))
            {
                continue;
            }

            /*=======================================
             * Compute the new minimum value for the
             * intersected range/min/max values.
             *=======================================*/

            if( cminmin == GREATER_THAN )
            {
                theMin = core_generate_constant(env, tmpmin1->type, tmpmin1->value);
            }
            else
            {
                theMin = core_generate_constant(env, tmpmin2->type, tmpmin2->value);
            }

            /*=======================================
             * Compute the new maximum value for the
             * intersected range/min/max values.
             *=======================================*/

            if( cmaxmax == LESS_THAN )
            {
                theMax = core_generate_constant(env, tmpmax1->type, tmpmax1->value);
            }
            else
            {
                theMax = core_generate_constant(env, tmpmax2->type, tmpmax2->value);
            }

            /*==================================
             * Add the new range/min/max values
             * to the intersection list.
             *==================================*/

            if( lastMin == NULL )
            {
                theMinList = theMin;
                theMaxList = theMax;
            }
            else
            {
                lastMin->next_arg = theMin;
                lastMax->next_arg = theMax;
            }

            lastMin = theMin;
            lastMax = theMax;
        }
    }

    /*============================================================
     * If the intersection produced a pair of valid range/min/max
     * values, then replace the previous values of the constraint
     * record to the new intersected values.
     *============================================================*/

    if( theMinList != NULL )
    {
        if( range )
        {
            core_return_expression(env, newConstraint->min_value);
            core_return_expression(env, newConstraint->max_value);
            newConstraint->min_value = theMinList;
            newConstraint->max_value = theMaxList;
        }
        else
        {
            core_return_expression(env, newConstraint->min_fields);
            core_return_expression(env, newConstraint->max_fields);
            newConstraint->min_fields = theMinList;
            newConstraint->max_fields = theMaxList;
        }
    }

    /*===============================================================
     * Otherwise, the intersection produced no valid range/min/max
     * values. For the range attribute, this means that no numbers
     * can satisfy the constraint. For the min/max fields attribute,
     * it means that no value can satisfy the constraint.
     *===============================================================*/

    else
    {
        if( range )
        {
            if( newConstraint->allow_any )
            {
                _allow_all(newConstraint, FALSE);
            }

            newConstraint->allow_integer = FALSE;
            newConstraint->allow_float = FALSE;
        }
        else
        {
            _allow_all(newConstraint, TRUE);
            newConstraint->allow_scalar = FALSE;
            newConstraint->allow_list = FALSE;
            newConstraint->allow_any = FALSE;
        }
    }
}

/***********************************************************
 * updateRestrictions: Updates the types allowed flags
 *   based on the allowed values in a constraint record.
 *   Intended to be called after the allowed values list
 *   has been changed (for example after intersecting the
 *   allowed-values list there may no be any values of a
 *   particular type left even though the type is allowed).
 ************************************************************/
static void _update_restrictions(CONSTRAINT_META *rv)
{
    if((rv->restrict_any) && (rv->restriction_list == NULL))
    {
        _allow_all(rv, TRUE);
        rv->allow_any = FALSE;
    }

    if((rv->restrict_atom) && (rv->allow_atom))
    {
        rv->allow_atom = _lookup_constant(ATOM, NULL, FALSE, rv->restriction_list);
    }

    if((rv->restrict_string)  && (rv->allow_string))
    {
        rv->allow_string = _lookup_constant(STRING, NULL, FALSE, rv->restriction_list);
    }

    if((rv->restrict_float) && (rv->allow_float))
    {
        rv->allow_float = _lookup_constant(FLOAT, NULL, FALSE, rv->restriction_list);
    }

    if((rv->restrict_integer) && (rv->allow_integer))
    {
        rv->allow_integer = _lookup_constant(INTEGER, NULL, FALSE, rv->restriction_list);
    }

    if((rv->restrict_instance_name) && (rv->allow_instance_name))
    {
        rv->allow_instance_name = _lookup_constant(INSTANCE_NAME, NULL, FALSE, rv->restriction_list);
    }
}

/************************************************************
 * lookupConstant: Determines if a particular constant
 *   (such as 27) or a class of constants (such as integers)
 *   can be found in a list of constants. Returns TRUE if
 *   such a constant can be found, otherwise FALSE.
 *************************************************************/
static int _lookup_constant(int theType, void *val, int useValue, struct core_expression *list)
{
    while( list != NULL )
    {
        if( list->type == theType )
        {
            if( !useValue )
            {
                return(TRUE);
            }
            else if( list->value == val )
            {
                return(TRUE);
            }
        }

        list = list->next_arg;
    }

    return(FALSE);
}

/*************************************************
 * hasTypeRestriction: Determines if a restriction
 *   is present for a specific type. Returns TRUE
 *   if there is, otherwise FALSE.
 **************************************************/
static int _has_type_restriction(int theType, CONSTRAINT_META *theConstraint)
{
    if( theConstraint == NULL )
    {
        return(FALSE);
    }

    if((theConstraint->restrict_any) ||
       (theConstraint->restrict_atom && (theType == ATOM)) ||
       (theConstraint->restrict_string && (theType == STRING)) ||
       (theConstraint->restrict_float && (theType == FLOAT)) ||
       (theConstraint->restrict_integer && (theType == INTEGER)) ||
       (theConstraint->restrict_class && ((theType == INSTANCE_ADDRESS) ||
                                          (theType == INSTANCE_NAME))) ||
       (theConstraint->restrict_instance_name && (theType == INSTANCE_NAME)))
    {
        return(TRUE);
    }

    return(FALSE);
}

/*********************************************************
 * constraint_union: Creates a new constraint record that
 *   is the union of two other constraint records.
 **********************************************************/
struct constraint_metadata *constraint_union(void *env, CONSTRAINT_META *c1, CONSTRAINT_META *c2)
{
    struct constraint_metadata *rv;
    int c1Changed = FALSE, c2Changed = FALSE;

    /*=================================================
     * If both constraint records are NULL,then create
     * a constraint record that allows any value.
     *=================================================*/

    if((c1 == NULL) && (c2 == NULL)) {return(create_constraint(env));}

    /*=====================================================
     * If one of the constraint records is NULL, then the
     * union is the other constraint record. Note that
     * this is different from the way that intersections
     * were handled (a NULL constraint record implied that
     *  any value was legal which in turn would imply that
     * the union would allow any value as well).
     *=====================================================*/

    if( c1 == NULL ) {return(_clone_constraint(env, c2));}

    if( c2 == NULL ) {return(_clone_constraint(env, c1));}

    /*=================================
     * Create a new constraint record.
     *=================================*/

    rv = create_constraint(env);

    /*==========================
     * Union the allowed types.
     *==========================*/

    if( c1->allow_list || c2->allow_list )
    {
        rv->allow_list = TRUE;
    }

    if( c1->allow_scalar || c2->allow_scalar )
    {
        rv->allow_scalar = TRUE;
    }

    if( c1->allow_any || c2->allow_any ) {rv->allow_any = TRUE;}
    else
    {
        rv->allow_any = FALSE;
        rv->allow_atom = (c1->allow_atom || c2->allow_atom);
        rv->allow_string = (c1->allow_string || c2->allow_string);
        rv->allow_float = (c1->allow_float || c2->allow_float);
        rv->allow_integer = (c1->allow_integer || c2->allow_integer);
        rv->allow_instance_name = (c1->allow_instance_name || c2->allow_instance_name);
        rv->allow_instance_pointer = (c1->allow_instance_pointer || c2->allow_instance_pointer);
        rv->allow_external_pointer = (c1->allow_external_pointer || c2->allow_external_pointer);
        rv->allow_void = (c1->allow_void || c2->allow_void);
    }

    /*=================================
     * Union the allowed-values flags.
     *=================================*/

    if( c1->restrict_any && c2->restrict_any ) {rv->restrict_any = TRUE;}
    else
    {
        if( c1->restrict_any )
        {
            c1Changed = TRUE;
            _restrict_all(c1, FALSE);
        }
        else if( c2->restrict_any )
        {
            c2Changed = TRUE;
            _restrict_all(c2, FALSE);
        }

        rv->restrict_any = FALSE;
        rv->restrict_atom = (c1->restrict_atom && c2->restrict_atom);
        rv->restrict_string = (c1->restrict_string && c2->restrict_string);
        rv->restrict_float = (c1->restrict_float && c2->restrict_float);
        rv->restrict_integer = (c1->restrict_integer && c2->restrict_integer);
        rv->restrict_class = (c1->restrict_class && c2->restrict_class);
        rv->restrict_instance_name = (c1->restrict_instance_name && c2->restrict_instance_name);

        if( c1Changed ) {_restrict_all(c1, FALSE);}
        else if( c2Changed ) {_restrict_all(c2, FALSE);}
    }

    /*========================================
     * Union the allowed values list, the min
     * and max values, and the range values.
     *========================================*/

    _union_allowed_values(env, c1, c2, rv);
    _union_allowed_classes(env, c1, c2, rv);
    _union_numbers(env, c1, c2, rv, TRUE);
    _union_numbers(env, c1, c2, rv, FALSE);

    /*========================================
     * If lists are allowed, then union
     * the constraint record for them.
     *========================================*/

    if( rv->allow_list )
    {
        rv->list = constraint_union(env, c1->list, c2->list);
    }

    /*====================
     * Return the unioned
     * constraint record.
     *====================*/

    return(rv);
}

/*************************************************
 * unionNumerals: Creates the union of
 *   two range or two min/max-fields constraints.
 **************************************************/
static void _union_numbers(void *env, CONSTRAINT_META *constraint1, CONSTRAINT_META *constraint2, CONSTRAINT_META *newConstraint, int range)
{
    struct core_expression *tmpmin, *tmpmax;
    struct core_expression *theMinList, *theMaxList;

    /*=========================================
     * Initialize the new range/min/max values
     * for the union of the constraints.
     *=========================================*/

    theMinList = NULL;
    theMaxList = NULL;

    /*=================================
     * Determine the min/max values of
     * the first constraint record.
     *=================================*/

    if( range )
    {
        tmpmin = constraint1->min_value;
        tmpmax = constraint1->max_value;
    }
    else
    {
        tmpmin = constraint1->min_fields;
        tmpmax = constraint1->max_fields;
    }

    /*============================================
     * Add each range/min/max pair from the first
     * constraint record to the union list.
     *============================================*/

    for(;
        tmpmin != NULL;
        tmpmin = tmpmin->next_arg, tmpmax = tmpmax->next_arg )
    {
        _union_numbers_with_list(env, tmpmin, tmpmax, &theMinList, &theMaxList);
    }

    /*=================================
     * Determine the min/max values of
     * the second constraint record.
     *=================================*/

    if( range )
    {
        tmpmin = constraint2->min_value;
        tmpmax = constraint2->max_value;
    }
    else
    {
        tmpmin = constraint2->min_fields;
        tmpmax = constraint2->max_fields;
    }

    /*=============================================
     * Add each range/min/max pair from the second
     * constraint record to the union list.
     *=============================================*/

    for(;
        tmpmin != NULL;
        tmpmin = tmpmin->next_arg, tmpmax = tmpmax->next_arg )
    {
        _union_numbers_with_list(env, tmpmin, tmpmax, &theMinList, &theMaxList);
    }

    /*=====================================================
     * If the union produced a pair of valid range/min/max
     * values, then replace the previous values of the
     * constraint record to the new unioned values.
     *=====================================================*/

    if( theMinList != NULL )
    {
        if( range )
        {
            core_return_expression(env, newConstraint->min_value);
            core_return_expression(env, newConstraint->max_value);
            newConstraint->min_value = theMinList;
            newConstraint->max_value = theMaxList;
        }
        else
        {
            core_return_expression(env, newConstraint->min_fields);
            core_return_expression(env, newConstraint->max_fields);
            newConstraint->min_fields = theMinList;
            newConstraint->max_fields = theMaxList;
        }
    }

    /*==============================================================
     * Otherwise, the union produced no valid range/min/max values.
     * For the range attribute, this means that no numbers can
     * satisfy the constraint. For the min/max fields attribute, it
     * means that no value can satisfy the constraint.
     *==============================================================*/

    else
    {
        if( range )
        {
            if( newConstraint->allow_any )
            {
                _allow_all(newConstraint, FALSE);
            }

            newConstraint->allow_integer = FALSE;
            newConstraint->allow_float = FALSE;
        }
        else
        {
            _allow_all(newConstraint, TRUE);
            newConstraint->allow_any = TRUE;
        }
    }
}

/********************************************************
 * unionNumeralsWithList: Unions a range/min/max
 *   pair of values with a list of such values.
 *********************************************************/
static void _union_numbers_with_list(void *env, struct core_expression *addmin, struct core_expression *addmax, struct core_expression **theMinList, struct core_expression **theMaxList)
{
    struct core_expression *tmpmin, *tmpmax, *lastmin, *lastmax;
    struct core_expression *themin, *themax, *nextmin, *nextmax;
    int cmaxmin, cmaxmax, cminmin, cminmax;

    /*=========================================================
     * If no values are on the lists, then use the new values.
     *=========================================================*/

    if( *theMinList == NULL )
    {
        *theMinList = core_generate_constant(env, addmin->type, addmin->value);
        *theMaxList = core_generate_constant(env, addmax->type, addmax->value);
        return;
    }

    lastmin = NULL;
    lastmax = NULL;
    tmpmin = (*theMinList);
    tmpmax = (*theMaxList);

    while( tmpmin != NULL )
    {
        cmaxmax = core_numerical_compare(env, addmax->type, addmax->value,
                                         tmpmax->type, tmpmax->value);

        cminmin = core_numerical_compare(env, addmin->type, addmin->value,
                                         tmpmin->type, tmpmin->value);

        cmaxmin = core_numerical_compare(env, addmax->type, addmax->value,
                                         tmpmin->type, tmpmin->value);

        cminmax = core_numerical_compare(env, addmin->type, addmin->value,
                                         tmpmax->type, tmpmax->value);

        /*=================================
         * Check to see if the range is
         * contained within another range.
         *=================================*/

        if(((cmaxmax == LESS_THAN) || (cmaxmax == EQUAL)) &&
           ((cminmin == GREATER_THAN) || (cminmin == EQUAL)))
        {
            return;
        }

        /*================================
         * Extend the greater than range.
         *================================*/

        if((cmaxmax == GREATER_THAN) &&
           ((cminmax == LESS_THAN) || (cminmax == EQUAL)))
        {
            tmpmax->type = addmax->type;
            tmpmax->value = addmax->value;
        }

        /*=============================
         * Extend the less than range.
         *=============================*/

        if((cminmin == LESS_THAN) &&
           ((cmaxmin == GREATER_THAN) || (cmaxmin == EQUAL)))
        {
            tmpmin->type = addmin->type;
            tmpmin->value = addmin->value;
        }

        /*====================
         * Handle insertions.
         *====================*/

        if( cmaxmin == LESS_THAN )
        {
            if( lastmax == NULL )
            {
                themin = core_generate_constant(env, addmin->type, addmin->value);
                themax = core_generate_constant(env, addmax->type, addmax->value);
                themin->next_arg = *theMinList;
                themax->next_arg = *theMaxList;
                *theMinList = themin;
                *theMaxList = themax;
                return;
            }

            if( core_numerical_compare(env, addmin->type, addmin->value,
                                       lastmax->type, lastmax->value) == GREATER_THAN )
            {
                themin = core_generate_constant(env, addmin->type, addmin->value);
                themax = core_generate_constant(env, addmax->type, addmax->value);

                themin->next_arg = lastmin->next_arg;
                themax->next_arg = lastmax->next_arg;

                lastmin->next_arg = themin;
                lastmax->next_arg = themax;
                return;
            }
        }

        /*==========================
         * Move on to the next one.
         *==========================*/

        tmpmin = tmpmin->next_arg;
        tmpmax = tmpmax->next_arg;
    }

    /*===========================
     * Merge overlapping ranges.
     *===========================*/

    tmpmin = (*theMinList);
    tmpmax = (*theMaxList);

    while( tmpmin != NULL )
    {
        nextmin = tmpmin->next_arg;
        nextmax = tmpmax->next_arg;

        if( nextmin != NULL )
        {
            cmaxmin = core_numerical_compare(env, tmpmax->type, tmpmax->value,
                                             nextmin->type, nextmin->value);

            if((cmaxmin == GREATER_THAN) || (cmaxmin == EQUAL))
            {
                tmpmax->type = nextmax->type;
                tmpmax->value = nextmax->value;
                tmpmax->next_arg = nextmax->next_arg;
                tmpmin->next_arg = nextmin->next_arg;

                core_mem_return_struct(env, core_expression, nextmin);
                core_mem_return_struct(env, core_expression, nextmax);
            }
            else
            {
                tmpmin = tmpmin->next_arg;
                tmpmax = tmpmax->next_arg;
            }
        }
        else
        {
            tmpmin = nextmin;
            tmpmax = nextmax;
        }
    }
}

/**************************************************
 * unionAllowedClasses: Creates the union
 *   of two sets of allowed-classes expressions.
 ***************************************************/
static void _union_allowed_classes(void *env, CONSTRAINT_META *constraint1, CONSTRAINT_META *constraint2, CONSTRAINT_META *newConstraint)
{
    struct core_expression *theHead = NULL;

    theHead = _add_to_union(env, constraint1->class_list, theHead, newConstraint);
    theHead = _add_to_union(env, constraint2->class_list, theHead, newConstraint);

    newConstraint->class_list = theHead;
}

/**************************************************
 * unionAllowedValues: Creates the union
 *   of two sets of allowed value expressions.
 ***************************************************/
static void _union_allowed_values(void *env, CONSTRAINT_META *constraint1, CONSTRAINT_META *constraint2, CONSTRAINT_META *newConstraint)
{
    struct core_expression *theHead = NULL;

    theHead = _add_to_union(env, constraint1->restriction_list, theHead, newConstraint);
    theHead = _add_to_union(env, constraint2->restriction_list, theHead, newConstraint);

    newConstraint->restriction_list = theHead;
}

/***********************************************************
 * addToUnion: Adds a list of values to a unioned list
 *   making sure that duplicates are not added and that any
 *   value added satisfies the constraints for the list.
 ************************************************************/
static struct core_expression *_add_to_union(void *env, struct core_expression *list1, struct core_expression *theHead, CONSTRAINT_META *theConstraint)
{
    struct core_expression *list2;
    int flag;

    /*======================================
     * Loop through each value in the list
     * being added to the unioned set.
     *======================================*/

    for(; list1 != NULL; list1 = list1->next_arg )
    {
        /*===================================
         * Determine if the value is already
         * in the unioned list.
         *===================================*/

        flag = TRUE;

        for( list2 = theHead;
             list2 != NULL;
             list2 = list2->next_arg )
        {
            if((list1->type == list2->type) &&
               (list1->value == list2->value))
            {
                flag = FALSE;
                break;
            }
        }

        /*=====================================================
         * If the value wasn't in the unioned list and doesn't
         * violate any of the unioned list's constraints, then
         * add it to the list.
         *=====================================================*/

        if( flag )
        {
            if( _has_type_restriction(list1->type, theConstraint))
            {
                list2 = core_generate_constant(env, list1->type, list1->value);
                list2->next_arg = theHead;
                theHead = list2;
            }
        }
    }

    /*==============================
     * Return the new unioned list.
     *==============================*/

    return(theHead);
}

/***********************************************
 * newConstraint: Creates and initializes
 *   the values of a constraint record.
 ************************************************/
struct constraint_metadata *create_constraint(void *env)
{
    CONSTRAINT_META *constraints;
    unsigned i;

    constraints = core_mem_get_struct(env, constraint_metadata);

    for( i = 0 ; i < sizeof(CONSTRAINT_META) ; i++ )
    {
        ((char *)constraints)[i] = '\0';
    }

    _allow_all(constraints, TRUE);

    constraints->allow_list = FALSE;
    constraints->allow_scalar = TRUE;

    constraints->restrict_any = FALSE;
    constraints->restrict_atom = FALSE;
    constraints->restrict_string = FALSE;
    constraints->restrict_float = FALSE;
    constraints->restrict_integer = FALSE;
    constraints->restrict_class = FALSE;
    constraints->restrict_instance_name = FALSE;
    constraints->class_list = NULL;
    constraints->restriction_list = NULL;
    constraints->min_value = core_generate_constant(env, ATOM, get_atom_data(env)->negative_inf_atom);
    constraints->max_value = core_generate_constant(env, ATOM, get_atom_data(env)->positive_inf_atom);
    constraints->min_fields = core_generate_constant(env, INTEGER, get_atom_data(env)->zero_atom);
    constraints->max_fields = core_generate_constant(env, ATOM, get_atom_data(env)->positive_inf_atom);
    constraints->bucket = -1;
    constraints->count = 0;
    constraints->list = NULL;
    constraints->next = NULL;

    return(constraints);
}

/*******************************************************
 * allowAll: Sets the allowed type flags of a
 *   constraint record to allow all types. If passed an
 *   argument of TRUE, just the "any allowed" flag is
 *   set to TRUE. If passed an argument of FALSE, then
 *   all of the individual type flags are set to TRUE.
 ********************************************************/
static void _allow_all(CONSTRAINT_META *theConstraint, int justOne)
{
    int flag1, flag2;

    if( justOne )
    {
        flag1 = TRUE;
        flag2 = FALSE;
    }
    else
    {
        flag1 = FALSE;
        flag2 = TRUE;
    }

    theConstraint->allow_any = flag1;
    theConstraint->allow_atom = flag2;
    theConstraint->allow_string = flag2;
    theConstraint->allow_float = flag2;
    theConstraint->allow_integer = flag2;
    theConstraint->allow_instance_name = flag2;
    theConstraint->allow_instance_pointer = flag2;
    theConstraint->allow_external_pointer = flag2;
    theConstraint->allow_void = flag2;
}

/****************************************************
 * cloneConstraint: Copies a constraint record.
 *****************************************************/
static struct constraint_metadata *_clone_constraint(void *env, CONSTRAINT_META *sourceConstraint)
{
    CONSTRAINT_META *theConstraint;

    if( sourceConstraint == NULL ) {return(NULL);}

    theConstraint = core_mem_get_struct(env, constraint_metadata);

    theConstraint->allow_any = sourceConstraint->allow_any;
    theConstraint->allow_atom = sourceConstraint->allow_atom;
    theConstraint->allow_string = sourceConstraint->allow_string;
    theConstraint->allow_float = sourceConstraint->allow_float;
    theConstraint->allow_integer = sourceConstraint->allow_integer;
    theConstraint->allow_instance_name = sourceConstraint->allow_instance_name;
    theConstraint->allow_instance_pointer = sourceConstraint->allow_instance_pointer;
    theConstraint->allow_external_pointer = sourceConstraint->allow_external_pointer;
    theConstraint->allow_void = sourceConstraint->allow_void;
    theConstraint->allow_list = sourceConstraint->allow_list;
    theConstraint->allow_scalar = sourceConstraint->allow_scalar;
    theConstraint->restrict_any = sourceConstraint->restrict_any;
    theConstraint->restrict_atom = sourceConstraint->restrict_atom;
    theConstraint->restrict_string = sourceConstraint->restrict_string;
    theConstraint->restrict_float = sourceConstraint->restrict_float;
    theConstraint->restrict_integer = sourceConstraint->restrict_integer;
    theConstraint->restrict_class = sourceConstraint->restrict_class;
    theConstraint->restrict_instance_name = sourceConstraint->restrict_instance_name;
    theConstraint->class_list = core_copy_expression(env, sourceConstraint->class_list);
    theConstraint->restriction_list = core_copy_expression(env, sourceConstraint->restriction_list);
    theConstraint->min_value = core_copy_expression(env, sourceConstraint->min_value);
    theConstraint->max_value = core_copy_expression(env, sourceConstraint->max_value);
    theConstraint->min_fields = core_copy_expression(env, sourceConstraint->min_fields);
    theConstraint->max_fields = core_copy_expression(env, sourceConstraint->max_fields);
    theConstraint->bucket = -1;
    theConstraint->count = 0;
    theConstraint->list = _clone_constraint(env, sourceConstraint->list);
    theConstraint->next = NULL;

    return(theConstraint);
}

/*************************************************************
 * restrictAll: Sets the restriction type flags of
 *   a constraint record to indicate there are restriction on
 *   all types. If passed an argument of TRUE, just the
 *   "any restriction" flag is set to TRUE. If passed an
 *   argument of FALSE, then all of the individual type
 *   restriction flags are set to TRUE.
 **************************************************************/
static void _restrict_all(CONSTRAINT_META *theConstraint, int justOne)
{
    int flag1, flag2;

    if( justOne )
    {
        flag1 = TRUE;
        flag2 = FALSE;
    }
    else
    {
        flag1 = FALSE;
        flag2 = TRUE;
    }

    theConstraint->restrict_any = flag1;
    theConstraint->restrict_atom = flag2;
    theConstraint->restrict_string = flag2;
    theConstraint->restrict_float = flag2;
    theConstraint->restrict_integer = flag2;
    theConstraint->restrict_instance_name = flag2;
}

/****************************************************
 * set_constraint_type: Given a constraint type and a
 *   constraint, sets the allowed type flags for the
 *   specified type in the constraint to TRUE.
 *****************************************************/
int set_constraint_type(int theType, CONSTRAINT_META *constraints)
{
    int rv = TRUE;

    switch( theType )
    {
    case UNKNOWN_VALUE:
        rv = constraints->allow_any;
        constraints->allow_any = TRUE;
        break;

    case ATOM:
        rv = constraints->allow_atom;
        constraints->allow_atom = TRUE;
        break;

    case STRING:
        rv = constraints->allow_string;
        constraints->allow_string = TRUE;
        break;

    case ATOM_OR_STRING:
        rv = (constraints->allow_string | constraints->allow_atom);
        constraints->allow_atom = TRUE;
        constraints->allow_string = TRUE;
        break;

    case INTEGER:
        rv = constraints->allow_integer;
        constraints->allow_integer = TRUE;
        break;

    case FLOAT:
        rv = constraints->allow_float;
        constraints->allow_float = TRUE;
        break;

    case INTEGER_OR_FLOAT:
        rv = (constraints->allow_integer | constraints->allow_float);
        constraints->allow_integer = TRUE;
        constraints->allow_float = TRUE;
        break;

    case INSTANCE_ADDRESS:
        rv = constraints->allow_instance_pointer;
        constraints->allow_instance_pointer = TRUE;
        break;

    case INSTANCE_NAME:
        rv = constraints->allow_instance_name;
        constraints->allow_instance_name = TRUE;
        break;

    case INSTANCE_OR_INSTANCE_NAME:
        rv = (constraints->allow_instance_name | constraints->allow_instance_pointer);
        constraints->allow_instance_name = TRUE;
        constraints->allow_instance_pointer = TRUE;
        break;

    case EXTERNAL_ADDRESS:
        rv = constraints->allow_external_pointer;
        constraints->allow_external_pointer = TRUE;
        break;

    case RVOID:
        rv = constraints->allow_void;
        constraints->allow_void = TRUE;
        break;

    case LIST:
        rv = constraints->allow_list;
        constraints->allow_list = TRUE;
        break;
    }

    if( theType != UNKNOWN_VALUE )
    {
        constraints->allow_any = FALSE;
    }

    return(rv);
}

/***************************************************************
 * convert_expression_to_constraint: Converts an expression into a
 *   constraint record. For example, an expression representing
 *   the symbol BLUE would be converted to a  record with
 *   allowed types ATOM and allow-values BLUE.
 ****************************************************************/
CONSTRAINT_META *convert_expression_to_constraint(void *env, struct core_expression *expr)
{
    CONSTRAINT_META *rv;

    /*================================================
     * A NULL expression is converted to a constraint
     * record with no values allowed.
     *================================================*/

    if( expr == NULL )
    {
        rv = create_constraint(env);
        rv->allow_any = FALSE;
        return(rv);
    }

    /*=============================================================
     * Convert variables and function calls to constraint records.
     *=============================================================*/

    if((expr->type == SCALAR_VARIABLE) ||
#if DEFGENERIC_CONSTRUCT
       (expr->type == GCALL) ||
#endif
#if DEFFUNCTION_CONSTRUCT
       (expr->type == PCALL) ||
#endif
       (expr->type == LIST_VARIABLE))
    {
        rv = create_constraint(env);
        rv->allow_list = TRUE;
        return(rv);
    }
    else if( expr->type == FCALL )
    {
        return(_convert_function_to_constraint(env, expr->value));
    }

    /*============================================
     * Convert a constant to a constraint record.
     *============================================*/

    rv = create_constraint(env);
    rv->allow_any = FALSE;

    if( expr->type == FLOAT )
    {
        rv->restrict_float = TRUE;
        rv->allow_float = TRUE;
    }
    else if( expr->type == INTEGER )
    {
        rv->restrict_integer = TRUE;
        rv->allow_integer = TRUE;
    }
    else if( expr->type == ATOM )
    {
        rv->restrict_atom = TRUE;
        rv->allow_atom = TRUE;
    }
    else if( expr->type == STRING )
    {
        rv->restrict_string = TRUE;
        rv->allow_string = TRUE;
    }
    else if( expr->type == INSTANCE_NAME )
    {
        rv->restrict_instance_name = TRUE;
        rv->allow_instance_name = TRUE;
    }
    else if( expr->type == INSTANCE_ADDRESS )
    {
        rv->allow_instance_pointer = TRUE;
    }

    if( rv->allow_float || rv->allow_integer || rv->allow_atom ||
        rv->allow_string || rv->allow_instance_name )
    {
        rv->restriction_list = core_generate_constant(env, expr->type, expr->value);
    }

    return(rv);
}

/******************************************************
 * functionToConstraint: Converts a function
 *   call to a constraint record. For example, the +
 *   function when converted would be a constraint
 *   record with allowed types INTEGER and FLOAT.
 *******************************************************/
static CONSTRAINT_META *_convert_function_to_constraint(void *env, void *func)
{
    CONSTRAINT_META *rv;

    rv = create_constraint(env);
    rv->allow_any = FALSE;

    switch((char)core_get_return_type(func))
    {
    case RT_EXT_ADDRESS:
        rv->allow_external_pointer = TRUE;
        break;

    case RT_FLOAT:
    case RT_DOUBLE:
        rv->allow_float = TRUE;
        break;

    case RT_INT:
    case RT_LONG:
    case RT_LONG_LONG:
        rv->allow_integer = TRUE;
        break;

    case RT_ATOM_STRING_INST:
        rv->allow_instance_name = TRUE;
        rv->allow_atom = TRUE;
        rv->allow_string = TRUE;
        break;

    case RT_ATOM_STRING:
        rv->allow_atom = TRUE;
        rv->allow_string = TRUE;
        break;

    case RT_LIST:
        rv->allow_scalar = FALSE;
        rv->allow_list = TRUE;
        break;

    case RT_INT_FLOAT:
        rv->allow_float = TRUE;
        rv->allow_integer = TRUE;
        break;

    case RT_INSTANCE:
        rv->allow_instance_name = TRUE;
        break;

    case RT_STRING:
        rv->allow_string = TRUE;
        break;

    case RT_UNKNOWN:
        rv->allow_any = TRUE;
        rv->allow_list = TRUE;
        break;

    case RT_ATOM:
    case RT_CHAR:
    case RT_BOOL:
        rv->allow_atom = TRUE;
        break;

    case RT_INST_ADDRESS:
        rv->allow_instance_pointer = TRUE;
        break;

    case RT_VOID:
        rv->allow_void = TRUE;
        break;
    }

    return(rv);
}

/******************************************************
 * convert_arg_type_to_constraint: Converts one of the
 *   function argument types (used by DefineFunction2)
 *   to a constraint record.
 *******************************************************/
CONSTRAINT_META *convert_arg_type_to_constraint(void *env, int theRestriction)
{
    CONSTRAINT_META *rv;

    rv = create_constraint(env);
    rv->allow_any = FALSE;

    switch( theRestriction )
    {
    case RT_EXT_ADDRESS:
        rv->allow_external_pointer = TRUE;
        break;

    case RT_INSTANCE_NAME:
        rv->allow_atom = TRUE;
        rv->allow_instance_name = TRUE;
        rv->allow_instance_pointer = TRUE;
        break;

    case RT_DOUBLE:
    case RT_FLOAT:
        rv->allow_float = TRUE;
        break;

    case RT_LONG_LONG:
        rv->allow_integer = TRUE;
        rv->allow_float = TRUE;
        rv->allow_atom = TRUE;
        break;

    case RT_UNUSED_0:
        /* Unused */
        break;

    case RT_INT:
    case RT_LONG:
        rv->allow_integer = TRUE;
        break;

    case RT_ATOM_STRING_INST:
        rv->allow_atom = TRUE;
        rv->allow_string = TRUE;
        rv->allow_instance_name = TRUE;
        break;

    case RT_ATOM_STRING:
        rv->allow_atom = TRUE;
        rv->allow_string = TRUE;
        break;

    case RT_LIST:
        rv->allow_scalar = FALSE;
        rv->allow_list = TRUE;
        break;

    case RT_INT_FLOAT:
        rv->allow_float = TRUE;
        rv->allow_integer = TRUE;
        break;

    case RT_INSTANCE:
        rv->allow_instance_name = TRUE;
        break;

    case RT_INSTANCE_NAME_ATOM:
        rv->allow_instance_name = TRUE;
        rv->allow_atom = TRUE;
        break;

    case RT_LIST_ATOM_STRING:
        rv->allow_atom = TRUE;
        rv->allow_string = TRUE;
        rv->allow_list = TRUE;
        break;

    case RT_STRING:
        rv->allow_string = TRUE;
        break;

    case RT_ATOM:
        rv->allow_atom = TRUE;
        break;

    case RT_INST_ADDRESS:
        rv->allow_instance_pointer = TRUE;
        break;

    case RT_UNUSED_3:
        /* Unused */
        break;

    case RT_UNUSED_4:
        /* Unused */
        break;

    case RT_UNKNOWN:
        rv->allow_any = TRUE;
        rv->allow_list = TRUE;
        break;

    case RT_VOID:
        rv->allow_void = TRUE;
        break;
    }

    return(rv);
}
