/* Purpose: Provides functions for creating and removing
 *   constraint records, adding them to the contraint hash
 *   table, and enabling and disabling static and dynamic
 *   constraint checking.                                    */

#define __CONSTRAINTS_SOURCE__

#include <stdio.h>
#define _STDIO_INCLUDED_
#include <stdlib.h>

#include "setup.h"

#include "core_arguments.h"
#include "constant.h"
#include "core_environment.h"
#include "core_functions.h"
#include "core_memory.h"
#include "type_list.h"
#include "router.h"
#include "core_scanner.h"

#include "constraints_kernel.h"

/**************************************
 * LOCAL FUNCTION PROTOTYPES
 ***************************************/

static void          _install_constraint(void *, CONSTRAINT_META *);
static int           _compare_constraint(struct constraint_metadata *, struct constraint_metadata *);
static void          _free_constraint(void *, CONSTRAINT_META *);
static void          _uninstall_constraint(void *, CONSTRAINT_META *);
static void          _clear_constraints(void *);
static unsigned long _hash_constraint(struct constraint_metadata *);

/****************************************************
 * init_constraints: Initializes the constraint
 *   hash table to NULL and defines the static and
 *   dynamic constraint access functions.
 *****************************************************/
void init_constraints(void *env)
{
    int i;

    core_allocate_environment_data(env, CONSTRAINT_DATA_INDEX, sizeof(struct constraint_data), _clear_constraints);

    lookup_constraint_data(env)->is_static = TRUE;
    lookup_constraint_data(env)->hashtable = (struct constraint_metadata **)core_mem_alloc_no_init(env, (int)sizeof(struct constraint_metadata *) *CONSTRAINT_HASH_SZ);

    if( lookup_constraint_data(env)->hashtable == NULL )
    {
        exit_router(env, EXIT_FAILURE);
    }

    for( i = 0; i < CONSTRAINT_HASH_SZ; i++ )
    {
        lookup_constraint_data(env)->hashtable[i] = NULL;
    }

#if METAOBJECT_PROTOCOL
    core_define_function(env, "get-dynamic-constraint-checking", 'b', GDCCommand, "GDCCommand", "00");
    core_define_function(env, "set-dynamic-constraint-checking", 'b', SDCCommand, "SDCCommand", "11");
    core_define_function(env, "get-static-constraint-checking", 'b', GSCCommand, "GSCCommand", "00");
    core_define_function(env, "set-static-constraint-checking", 'b', SSCCommand, "SSCCommand", "11");
#endif
}

/*****************************************
 * remove_constraint: Removes a constraint
 *   from the constraint hash table.
 ******************************************/
void remove_constraint(void *env, struct constraint_metadata *theConstraint)
{
    struct constraint_metadata *tmpPtr, *prevPtr = NULL;

    if( theConstraint == NULL )
    {
        return;
    }

    /*========================================
     * If the bucket value is less than zero,
     * then the constraint wasn't stored in
     * the hash table.
     *========================================*/

    if( theConstraint->bucket < 0 )
    {
        _free_constraint(env, theConstraint);
        return;
    }

    /*================================
     * Find and remove the constraint
     * from the contraint hash table.
     *================================*/

    tmpPtr = lookup_constraint_data(env)->hashtable[theConstraint->bucket];

    while( tmpPtr != NULL )
    {
        if( tmpPtr == theConstraint )
        {
            theConstraint->count--;

            if( theConstraint->count == 0 )
            {
                if( prevPtr == NULL )
                {
                    lookup_constraint_data(env)->hashtable[theConstraint->bucket] = theConstraint->next;
                }
                else
                {
                    prevPtr->next = theConstraint->next;
                }

                _uninstall_constraint(env, theConstraint);
                _free_constraint(env, theConstraint);
            }

            return;
        }

        prevPtr = tmpPtr;
        tmpPtr = tmpPtr->next;
    }

    return;
}

/**********************************
 * hashConstraint: Returns a hash
 *   value for a given constraint.
 ***********************************/
unsigned long _hash_constraint(struct constraint_metadata *theConstraint)
{
    int i = 0;
    unsigned long count = 0;
    unsigned long hashValue;
    struct core_expression *tmpPtr;

    count += (unsigned long)
             (theConstraint->allow_any * 17) +
             (theConstraint->allow_atom * 5) +
             (theConstraint->allow_string * 23) +
             (theConstraint->allow_float * 19) +
             (theConstraint->allow_integer * 29) +
             (theConstraint->allow_instance_name * 31) +
             (theConstraint->allow_instance_pointer * 17);

    count += (unsigned long)
             (theConstraint->allow_external_pointer * 29) +
             (theConstraint->allow_void * 29) +
             (theConstraint->allow_list * 29) +
             (theConstraint->restrict_any * 59) +
             (theConstraint->restrict_atom * 61);

    count += (unsigned long)
             (theConstraint->restrict_string * 3) +
             (theConstraint->restrict_float * 37) +
             (theConstraint->restrict_integer * 9) +
             (theConstraint->restrict_class * 11) +
             (theConstraint->restrict_instance_name * 7);

    for( tmpPtr = theConstraint->class_list; tmpPtr != NULL; tmpPtr = tmpPtr->next_arg )
    {
        count += core_hash_atom(tmpPtr->type, tmpPtr->value, i++);
    }

    for( tmpPtr = theConstraint->restriction_list; tmpPtr != NULL; tmpPtr = tmpPtr->next_arg )
    {
        count += core_hash_atom(tmpPtr->type, tmpPtr->value, i++);
    }

    for( tmpPtr = theConstraint->min_value; tmpPtr != NULL; tmpPtr = tmpPtr->next_arg )
    {
        count += core_hash_atom(tmpPtr->type, tmpPtr->value, i++);
    }

    for( tmpPtr = theConstraint->max_value; tmpPtr != NULL; tmpPtr = tmpPtr->next_arg )
    {
        count += core_hash_atom(tmpPtr->type, tmpPtr->value, i++);
    }

    for( tmpPtr = theConstraint->min_fields; tmpPtr != NULL; tmpPtr = tmpPtr->next_arg )
    {
        count += core_hash_atom(tmpPtr->type, tmpPtr->value, i++);
    }

    for( tmpPtr = theConstraint->max_fields; tmpPtr != NULL; tmpPtr = tmpPtr->next_arg )
    {
        count += core_hash_atom(tmpPtr->type, tmpPtr->value, i++);
    }

    if( theConstraint->list != NULL )
    {
        count += _hash_constraint(theConstraint->list);
    }

    hashValue = (unsigned long)(count % CONSTRAINT_HASH_SZ);

    return(hashValue);
}

#if METAOBJECT_PROTOCOL

/*********************************************
 * SDCCommand: H/L access routine for the
 *   set-dynamic-constraint-checking command.
 **********************************************/
int SDCCommand(void *env)
{
    int oldValue;
    core_data_object arg_ptr;

    oldValue = getDynamicConstraints(env);

    if( core_check_arg_count(env, "set-dynamic-constraint-checking", EXACTLY, 1) == -1 )
    {
        return(oldValue);
    }

    core_get_arg_at(env, 1, &arg_ptr);

    if((arg_ptr.value == get_false(env)) && (arg_ptr.type == ATOM))
    {
        setDynamicConstraints(env, FALSE);
    }
    else
    {
        setDynamicConstraints(env, TRUE);
    }

    return(oldValue);
}

/*********************************************
 * GDCCommand: H/L access routine for the
 *   get-dynamic-constraint-checking command.
 **********************************************/
int GDCCommand(void *env)
{
    int oldValue;

    oldValue = getDynamicConstraints(env);

    if( core_check_arg_count(env, "get-dynamic-constraint-checking", EXACTLY, 0) == -1 )
    {
        return(oldValue);
    }

    return(oldValue);
}

/********************************************
 * SSCCommand: H/L access routine for the
 *   set-static-constraint-checking command.
 *********************************************/
int SSCCommand(void *env)
{
    int oldValue;
    core_data_object arg_ptr;

    oldValue = get_constraints(env);

    if( core_check_arg_count(env, "set-static-constraint-checking", EXACTLY, 1) == -1 )
    {
        return(oldValue);
    }

    core_get_arg_at(env, 1, &arg_ptr);

    if((arg_ptr.value == get_false(env)) && (arg_ptr.type == ATOM))
    {
        setStaticConstraints(env, FALSE);
    }
    else
    {
        setStaticConstraints(env, TRUE);
    }

    return(oldValue);
}

/********************************************
 * GSCCommand: H/L access routine for the
 *   get-static-constraint-checking command.
 *********************************************/
int GSCCommand(void *env)
{
    int oldValue;

    oldValue = get_constraints(env);

    if( core_check_arg_count(env, "get-static-constraint-checking", EXACTLY, 0) == -1 )
    {
        return(oldValue);
    }

    return(oldValue);
}

#endif

/****************************************************
 * get_constraints: C access routine
 *   for the get-static-constraint-checking command.
 *****************************************************/
BOOLEAN get_constraints(void *env)
{
    return(lookup_constraint_data(env)->is_static);
}

/**************************************
 * LOCAL FUNCTIONS
 ***************************************/

/****************************************************
 * clearConstraints: Deallocates environment
 *    data for constraints.
 *****************************************************/
static void _clear_constraints(void *env)
{
    struct constraint_metadata *tmpPtr, *nextPtr;
    int i;

    for( i = 0; i < CONSTRAINT_HASH_SZ; i++ )
    {
        tmpPtr = lookup_constraint_data(env)->hashtable[i];

        while( tmpPtr != NULL )
        {
            nextPtr = tmpPtr->next;
            _free_constraint(env, tmpPtr);
            tmpPtr = nextPtr;
        }
    }

    core_mem_release(env, lookup_constraint_data(env)->hashtable, (int)sizeof(struct constraint_metadata *) *CONSTRAINT_HASH_SZ);
}

/************************************************************
 * freeConstraint: Frees the data structures used by
 *   a constraint record. If the returnOnlyFields argument
 *   is FALSE, then the constraint record is also freed.
 *************************************************************/
static void _free_constraint(void *env, CONSTRAINT_META *constraints)
{
    if( constraints == NULL )
    {
        return;
    }

    if( constraints->bucket < 0 )
    {
        core_return_expression(env, constraints->class_list);
        core_return_expression(env, constraints->restriction_list);
        core_return_expression(env, constraints->max_value);
        core_return_expression(env, constraints->min_value);
        core_return_expression(env, constraints->min_fields);
        core_return_expression(env, constraints->max_fields);
    }

    _free_constraint(env, constraints->list);

    core_mem_return_struct(env, constraint_metadata, constraints);
}

/**************************************************
 * uninstallConstraint: Decrements the count
 *   values of all occurrences of primitive data
 *   types found in a constraint record.
 ***************************************************/
static void _uninstall_constraint(void *env, CONSTRAINT_META *constraints)
{
    if( constraints->bucket >= 0 )
    {
        core_remove_expression_hash(env, constraints->class_list);
        core_remove_expression_hash(env, constraints->restriction_list);
        core_remove_expression_hash(env, constraints->max_value);
        core_remove_expression_hash(env, constraints->min_value);
        core_remove_expression_hash(env, constraints->min_fields);
        core_remove_expression_hash(env, constraints->max_fields);
    }
    else
    {
        core_decrement_expression(env, constraints->class_list);
        core_decrement_expression(env, constraints->restriction_list);
        core_decrement_expression(env, constraints->max_value);
        core_decrement_expression(env, constraints->min_value);
        core_decrement_expression(env, constraints->min_fields);
        core_decrement_expression(env, constraints->max_fields);
    }

    if( constraints->list != NULL )
    {
        _uninstall_constraint(env, constraints->list);
    }
}

/*********************************************
 * compareConstraint: Compares two constraint
 *   records and returns TRUE if they are
 *   identical, otherwise FALSE.
 **********************************************/
static int _compare_constraint(struct constraint_metadata *constraint1, struct constraint_metadata *constraint2)
{
    struct core_expression *tmpPtr1, *tmpPtr2;

    if((constraint1->allow_any != constraint2->allow_any) ||
       (constraint1->allow_atom != constraint2->allow_atom) ||
       (constraint1->allow_string != constraint2->allow_string) ||
       (constraint1->allow_float != constraint2->allow_float) ||
       (constraint1->allow_integer != constraint2->allow_integer) ||
       (constraint1->allow_instance_name != constraint2->allow_instance_name) ||
       (constraint1->allow_instance_pointer != constraint2->allow_instance_pointer) ||
       (constraint1->allow_external_pointer != constraint2->allow_external_pointer) ||
       (constraint1->allow_void != constraint2->allow_void) ||
       (constraint1->allow_list != constraint2->allow_list) ||
       (constraint1->allow_scalar != constraint2->allow_scalar) ||
       (constraint1->restrict_any != constraint2->restrict_any) ||
       (constraint1->restrict_atom != constraint2->restrict_atom) ||
       (constraint1->restrict_string != constraint2->restrict_string) ||
       (constraint1->restrict_float != constraint2->restrict_float) ||
       (constraint1->restrict_integer != constraint2->restrict_integer) ||
       (constraint1->restrict_class != constraint2->restrict_class) ||
       (constraint1->restrict_instance_name != constraint2->restrict_instance_name))
    {
        return(FALSE);
    }

    for( tmpPtr1 = constraint1->class_list, tmpPtr2 = constraint2->class_list;
         (tmpPtr1 != NULL) && (tmpPtr2 != NULL);
         tmpPtr1 = tmpPtr1->next_arg, tmpPtr2 = tmpPtr2->next_arg )
    {
        if((tmpPtr1->type != tmpPtr2->type) || (tmpPtr1->value != tmpPtr2->value))
        {
            return(FALSE);
        }
    }

    if( tmpPtr1 != tmpPtr2 )
    {
        return(FALSE);
    }

    for( tmpPtr1 = constraint1->restriction_list, tmpPtr2 = constraint2->restriction_list;
         (tmpPtr1 != NULL) && (tmpPtr2 != NULL);
         tmpPtr1 = tmpPtr1->next_arg, tmpPtr2 = tmpPtr2->next_arg )
    {
        if((tmpPtr1->type != tmpPtr2->type) || (tmpPtr1->value != tmpPtr2->value))
        {
            return(FALSE);
        }
    }

    if( tmpPtr1 != tmpPtr2 )
    {
        return(FALSE);
    }

    for( tmpPtr1 = constraint1->min_value, tmpPtr2 = constraint2->min_value;
         (tmpPtr1 != NULL) && (tmpPtr2 != NULL);
         tmpPtr1 = tmpPtr1->next_arg, tmpPtr2 = tmpPtr2->next_arg )
    {
        if((tmpPtr1->type != tmpPtr2->type) || (tmpPtr1->value != tmpPtr2->value))
        {
            return(FALSE);
        }
    }

    if( tmpPtr1 != tmpPtr2 )
    {
        return(FALSE);
    }

    for( tmpPtr1 = constraint1->max_value, tmpPtr2 = constraint2->max_value;
         (tmpPtr1 != NULL) && (tmpPtr2 != NULL);
         tmpPtr1 = tmpPtr1->next_arg, tmpPtr2 = tmpPtr2->next_arg )
    {
        if((tmpPtr1->type != tmpPtr2->type) || (tmpPtr1->value != tmpPtr2->value))
        {
            return(FALSE);
        }
    }

    if( tmpPtr1 != tmpPtr2 )
    {
        return(FALSE);
    }

    for( tmpPtr1 = constraint1->min_fields, tmpPtr2 = constraint2->min_fields;
         (tmpPtr1 != NULL) && (tmpPtr2 != NULL);
         tmpPtr1 = tmpPtr1->next_arg, tmpPtr2 = tmpPtr2->next_arg )
    {
        if((tmpPtr1->type != tmpPtr2->type) || (tmpPtr1->value != tmpPtr2->value))
        {
            return(FALSE);
        }
    }

    if( tmpPtr1 != tmpPtr2 )
    {
        return(FALSE);
    }

    for( tmpPtr1 = constraint1->max_fields, tmpPtr2 = constraint2->max_fields;
         (tmpPtr1 != NULL) && (tmpPtr2 != NULL);
         tmpPtr1 = tmpPtr1->next_arg, tmpPtr2 = tmpPtr2->next_arg )
    {
        if((tmpPtr1->type != tmpPtr2->type) || (tmpPtr1->value != tmpPtr2->value))
        {
            return(FALSE);
        }
    }

    if( tmpPtr1 != tmpPtr2 )
    {
        return(FALSE);
    }

    if(((constraint1->list == NULL) && (constraint2->list != NULL)) ||
       ((constraint1->list != NULL) && (constraint2->list == NULL)))
    {
        return(FALSE);
    }
    else if( constraint1->list == constraint2->list )
    {
        return(TRUE);
    }

    return(_compare_constraint(constraint1->list, constraint2->list));
}

/************************************************
 * installConstraint: Increments the count
 *   values of all occurrences of primitive data
 *   types found in a constraint record.
 *************************************************/
static void _install_constraint(void *env, CONSTRAINT_META *constraints)
{
    struct core_expression *tempExpr;

    tempExpr = core_add_expression_hash(env, constraints->class_list);
    core_return_expression(env, constraints->class_list);
    constraints->class_list = tempExpr;

    tempExpr = core_add_expression_hash(env, constraints->restriction_list);
    core_return_expression(env, constraints->restriction_list);
    constraints->restriction_list = tempExpr;

    tempExpr = core_add_expression_hash(env, constraints->max_value);
    core_return_expression(env, constraints->max_value);
    constraints->max_value = tempExpr;

    tempExpr = core_add_expression_hash(env, constraints->min_value);
    core_return_expression(env, constraints->min_value);
    constraints->min_value = tempExpr;

    tempExpr = core_add_expression_hash(env, constraints->min_fields);
    core_return_expression(env, constraints->min_fields);
    constraints->min_fields = tempExpr;

    tempExpr = core_add_expression_hash(env, constraints->max_fields);
    core_return_expression(env, constraints->max_fields);
    constraints->max_fields = tempExpr;

    if( constraints->list != NULL )
    {
        _install_constraint(env, constraints->list);
    }
}
