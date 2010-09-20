/* Purpose: Contains routines for creating, deleting,
 *   compacting, installing, and hashing expressions.        */

#define _EXPRESSN_SOURCE_

#include "setup.h"

#include <stdio.h>
#define _STDIO_INCLUDED_
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "core_memory.h"
#include "core_environment.h"
#include "router.h"
#include "core_functions.h"
#include "core_expressions_operators.h"
#include "core_utilities.h"
#include "core_evaluation.h"
#include "funcs_comparison.h"
#include "funcs_logic.h"

#include "core_expressions.h"

#define PRIME_ONE   257
#define PRIME_TWO   263
#define PRIME_THREE 269

/***************************************
 * LOCAL FUNCTION PROTOTYPES
 ****************************************/

static long                     _pack_list(struct core_expression *, struct core_expression *, long);
static EXPRESSION_HASH        * _find_expression_hash(void *, core_expression_object *, unsigned *, EXPRESSION_HASH **);
static unsigned                 _hash_expression(core_expression_object *);
static void                     _delete_expression_data(void *);
static void                     _init_expression_pointers(void *);

/*************************************************
 * core_init_expression_data: Initializes the function
 *   pointers used in generating some expressions
 *   and the expression hash table.
 **************************************************/
void core_init_expression_data(void *env)
{
    register unsigned i;

    core_allocate_environment_data(env, EXPRESSION_DATA_INDEX, sizeof(struct core_expression_data), _delete_expression_data);

    _init_expression_pointers(env);

    core_get_expression_data(env)->expression_hash_table = (EXPRESSION_HASH **)
                                                           core_mem_alloc_no_init(env, (int)(sizeof(EXPRESSION_HASH *) * EXPRESSION_HASH_SZ));

    for( i = 0 ; i < EXPRESSION_HASH_SZ ; i++ )
    {
        core_get_expression_data(env)->expression_hash_table[i] = NULL;
    }
}

/****************************************
 * DeallocateExpressionData: Deallocates
 *    environment data for expressions.
 *****************************************/
static void _delete_expression_data(void *env)
{
    int i;
    EXPRESSION_HASH *tmpPtr, *nextPtr;

    {
        for( i = 0; i < EXPRESSION_HASH_SZ; i++ )
        {
            tmpPtr = core_get_expression_data(env)->expression_hash_table[i];

            while( tmpPtr != NULL )
            {
                nextPtr = tmpPtr->next;
                core_return_packed_expression(env, tmpPtr->expression);
                core_mem_return_struct(env, core_expression_hash, tmpPtr);
                tmpPtr = nextPtr;
            }
        }
    }

    core_mem_release(env, core_get_expression_data(env)->expression_hash_table,
                     (int)(sizeof(EXPRESSION_HASH *) * EXPRESSION_HASH_SZ));
}

/***************************************************
 * core_init_expression_pointers: Initializes the function
 *   pointers used in generating some expressions.
 ****************************************************/
void _init_expression_pointers(void *env)
{
    core_get_expression_data(env)->fn_and = (void *)core_lookup_function(env, FUNC_NAME_AND);
    core_get_expression_data(env)->fn_or = (void *)core_lookup_function(env, FUNC_NAME_OR);
    core_get_expression_data(env)->fn_equal = (void *)core_lookup_function(env, FUNC_NAME_EQUAL);
    core_get_expression_data(env)->fn_not_equal = (void *)core_lookup_function(env, FUNC_NAME_NOT_EQUAL);
    core_get_expression_data(env)->fn_not = (void *)core_lookup_function(env, FUNC_NAME_NOT);

    if((core_get_expression_data(env)->fn_and == NULL) || (core_get_expression_data(env)->fn_or == NULL) ||
       (core_get_expression_data(env)->fn_equal == NULL) || (core_get_expression_data(env)->fn_not_equal == NULL) || (core_get_expression_data(env)->fn_not == NULL))
    {
        error_system(env, "EXPRESSN", 1);
        exit_router(env, EXIT_FAILURE);
    }
}

/**************************************************
 * core_increment_expression: Increments the busy count of
 *   atomic data values found in an expression.
 ***************************************************/
void core_increment_expression(void *env, struct core_expression *expression)
{
    if( expression == NULL )
    {
        return;
    }

    while( expression != NULL )
    {
        core_install_data(env, expression->type, expression->value);
        core_increment_expression(env, expression->args);
        expression = expression->next_arg;
    }
}

/****************************************************
 * core_decrement_expression: Decrements the busy count of
 *   atomic data values found in an expression.
 *****************************************************/
void core_decrement_expression(void *env, struct core_expression *expression)
{
    if( expression == NULL )
    {
        return;
    }

    while( expression != NULL )
    {
        core_decrement_atom(env, expression->type, expression->value);
        core_decrement_expression(env, expression->args);
        expression = expression->next_arg;
    }
}

/**********************************************************************
 * core_pack_expression: Copies an expression (created using multiple memory
 *   requests) into an array (created using a single memory request)
 *   while maintaining all appropriate links in the expression. A
 *   packed expression requires less total memory because it reduces
 *   the overhead required for multiple memory allocations.
 ***********************************************************************/
struct core_expression *core_pack_expression(void *env, struct core_expression *original)
{
    struct core_expression *packPtr;

    if( original == NULL ) {return(NULL);}

    packPtr = (struct core_expression *)
              core_mem_alloc_large_no_init(env, (long)sizeof(struct core_expression) *
                                           (long)core_calculate_expression_size(original));
    _pack_list(original, packPtr, 0L);
    return(packPtr);
}

/**********************************************************
 * ListToPacked: Copies a list of expressions to an array.
 ***********************************************************/
static long _pack_list(struct core_expression *original, struct core_expression *destination, long count)
{
    long i;

    if( original == NULL )
    {
        return(count);
    }

    while( original != NULL )
    {
        i = count;
        count++;

        destination[i].type = original->type;
        destination[i].value = original->value;

        if( original->args == NULL )
        {
            destination[i].args = NULL;
        }
        else
        {
            destination[i].args =
                (struct core_expression *)&destination[(long)count];
            count = _pack_list(original->args, destination, count);
        }

        if( original->next_arg == NULL )
        {
            destination[i].next_arg = NULL;
        }
        else
        {
            destination[i].next_arg =
                (struct core_expression *)&destination[(long)count];
        }

        original = original->next_arg;
    }

    return(count);
}

/**************************************************************
 * core_return_packed_expression: Returns a packed expression created
 *   using core_pack_expression to the memory manager.
 ***************************************************************/
void core_return_packed_expression(void *env, struct core_expression *packPtr)
{
    if( packPtr != NULL )
    {
        core_mem_release_sized(env, (void *)packPtr, (long)sizeof(struct core_expression) *
                               core_calculate_expression_size(packPtr));
    }
}

/**********************************************
 * core_return_expression: Returns a multiply linked
 *   list of core_expression data structures.
 ***********************************************/
void core_return_expression(void *env, struct core_expression *waste)
{
    register struct core_expression *tmp;

    while( waste != NULL )
    {
        if( waste->args != NULL )
        {
            core_return_expression(env, waste->args);
        }

        tmp = waste;
        waste = waste->next_arg;
        core_mem_return_struct(env, core_expression, tmp);
    }
}

/***************************************************
 *  NAME         : FindHashedExpression
 *  DESCRIPTION  : Determines if a given expression
 *              is in the expression hash table
 *  INPUTS       : 1) The expression
 *              2) A buffer to hold the hash
 *                 value
 *              3) A buffer to hold the previous
 *                 node in the hash chain
 *  RETURNS      : The expression hash table entry
 *              (NULL if not found)
 *  SIDE EFFECTS : None
 *  NOTES        : None
 ***************************************************/
static EXPRESSION_HASH *_find_expression_hash(void *env, core_expression_object *theExp, unsigned *hashval, EXPRESSION_HASH **prv)
{
    EXPRESSION_HASH *exphash;

    if( theExp == NULL )
    {
        return(NULL);
    }

    *hashval = _hash_expression(theExp);
    *prv = NULL;
    exphash = core_get_expression_data(env)->expression_hash_table[*hashval];

    while( exphash != NULL )
    {
        if( core_are_expressions_equivalent(exphash->expression, theExp))
        {
            return(exphash);
        }

        *prv = exphash;
        exphash = exphash->next;
    }

    return(NULL);
}

/***************************************************
 *  NAME         : HashExpression
 *  DESCRIPTION  : Assigns a deterministic number to
 *              an expression
 *  INPUTS       : The expression
 *  RETURNS      : The "value" of the expression
 *  SIDE EFFECTS : None
 *  NOTES        : None
 ***************************************************/
static unsigned _hash_expression(core_expression_object *theExp)
{
    unsigned long tally = PRIME_THREE;

    union
    {
        void *        vv;
        unsigned long uv;
    } fis;

    if( theExp->args != NULL )
    {
        tally += _hash_expression(theExp->args) * PRIME_ONE;
    }

    while( theExp != NULL )
    {
        tally += (unsigned long)(theExp->type * PRIME_TWO);
        fis.uv = 0;
        fis.vv = theExp->value;
        tally += fis.uv;
        theExp = theExp->next_arg;
    }

    return((unsigned)(tally % EXPRESSION_HASH_SZ));
}

/***************************************************
 *  NAME         : core_remove_expression_hash
 *  DESCRIPTION  : Removes a hashed expression from
 *              the hash table
 *  INPUTS       : The expression
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Hash node removed (or use count
 *              decremented).  If the hash node
 *              is removed, the expression is
 *              deinstalled and deleted
 *  NOTES        : If the expression is in use by
 *              others, then the use count is
 *              merely decremented
 ***************************************************/
void core_remove_expression_hash(void *env, core_expression_object *theExp)
{
    EXPRESSION_HASH *exphash, *prv;
    unsigned hashval;

    exphash = _find_expression_hash(env, theExp, &hashval, &prv);

    if( exphash == NULL )
    {
        return;
    }

    if( --exphash->count != 0 )
    {
        return;
    }

    if( prv == NULL )
    {
        core_get_expression_data(env)->expression_hash_table[hashval] = exphash->next;
    }
    else
    {
        prv->next = exphash->next;
    }

    core_decrement_expression(env, exphash->expression);
    core_return_packed_expression(env, exphash->expression);
    core_mem_return_struct(env, core_expression_hash, exphash);
}

/*****************************************************
 *  NAME         : core_add_expression_hash
 *  DESCRIPTION  : Adds a new expression to the
 *              expression hash table (or increments
 *              the use count if it is already there)
 *  INPUTS       : The (new) expression
 *  RETURNS      : A pointer to the (new) hash node
 *  SIDE EFFECTS : Adds the new hash node or increments
 *              the count of an existing one
 *  NOTES        : It is the caller's responsibility to
 *              delete the passed expression.  This
 *              routine copies, packs and installs
 *              the given expression
 *****************************************************/
core_expression_object *core_add_expression_hash(void *env, core_expression_object *theExp)
{
    EXPRESSION_HASH *prv, *exphash;
    unsigned hashval;

    if( theExp == NULL )
    {
        return(NULL);
    }

    exphash = _find_expression_hash(env, theExp, &hashval, &prv);

    if( exphash != NULL )
    {
        exphash->count++;
        return(exphash->expression);
    }

    exphash = core_mem_get_struct(env, core_expression_hash);
    exphash->value = hashval;
    exphash->count = 1;
    exphash->expression = core_pack_expression(env, theExp);
    core_increment_expression(env, exphash->expression);
    exphash->next = core_get_expression_data(env)->expression_hash_table[exphash->value];
    core_get_expression_data(env)->expression_hash_table[exphash->value] = exphash;
    exphash->id = 0L;
    return(exphash->expression);
}
