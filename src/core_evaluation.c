/* Purpose: Provides routines for evaluating expressions.    */

#define __CORE_EVALUATION_SOURCE__

#include <stdio.h>
#define _STDIO_INCLUDED_
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "setup.h"

#include "core_arguments.h"
#include "core_command_prompt.h"
#include "constant.h"
#include "core_environment.h"
#include "core_memory.h"
#include "router.h"
#include "core_functions.h"
#include "funcs_flow_control.h"
#include "type_list.h"
#include "core_utilities.h"
#include "parser_expressions.h"
#include "core_gc.h"
#include "funcs_profiling.h"
#include "sysdep.h"

#if DEFFUNCTION_CONSTRUCT
#include "funcs_function.h"
#endif

#if DEFGENERIC_CONSTRUCT
#include "generics_kernel.h"
#endif

#if OBJECT_SYSTEM
#include "classes.h"
#endif

#include "core_evaluation.h"

/**************************************
 * LOCAL FUNCTION PROTOTYPES
 ***************************************/

static void _pass_atom(void *, int, void *);
static void _delete_evaluation_data(void *);
static void _print_pointer(void *, char *, void *);
static void _new_ext_address(void *, core_data_object *);

/*
 * static BOOLEAN                 DiscardCAddress(void *,void *);
 */

/*************************************************
 * core_init_evaluation_data: Allocates environment
 *    data for expression evaluation.
 **************************************************/
void core_init_evaluation_data(void *env)
{
    struct core_external_address_type cPointer = {"C", _print_pointer, _print_pointer, NULL, _new_ext_address, NULL};

    core_allocate_environment_data(env, EVALUATION_DATA, sizeof(struct core_evaluation_data), _delete_evaluation_data);

    core_install_ext_address_type(env, &cPointer);
}

/****************************************************
 * DeallocateEvaluationData: Deallocates environment
 *    data for evaluation data.
 *****************************************************/
static void _delete_evaluation_data(void *env)
{
    int i;

    for( i = 0; i < core_get_evaluation_data(env)->address_type_count; i++ )
    {
        core_mem_return_struct(env, core_external_address_type, core_get_evaluation_data(env)->ext_address_types[i]);
    }
}

/*************************************************************
 * core_eval_expression: Evaluates an expression. Returns FALSE
 *   if no errors occurred during evaluation, otherwise TRUE.
 **************************************************************/
int core_eval_expression(void *env, struct core_expression *problem, core_data_object_ptr ret)
{
    struct core_expression *oldArgument;
    void *oldContext;
    struct core_function_definition *fptr;

#if PROFILING_FUNCTIONS
    struct profileFrameInfo profileFrame;
#endif

    if( problem == NULL )
    {
        ret->type = ATOM;
        ret->value = get_false(env);
        return(core_get_evaluation_data(env)->eval_error);
    }

    switch( problem->type )
    {
    case STRING:
    case ATOM:
    case FLOAT:
    case INTEGER:
#if OBJECT_SYSTEM
    case INSTANCE_NAME:
    case INSTANCE_ADDRESS:
#endif
    case EXTERNAL_ADDRESS:
        ret->type = problem->type;
        ret->value = problem->value;
        break;

    case DATA_OBJECT_ARRAY: /* TBD Remove with AddPrimitive */
        ret->type = problem->type;
        ret->value = problem->value;
        break;

    case FCALL:
    {
        fptr = (struct core_function_definition *)problem->value;
        oldContext = core_set_environment_function_context(env, fptr->context);

#if PROFILING_FUNCTIONS
        StartProfile(env, &profileFrame,
                     &fptr->ext_data,
                     ProfileFunctionData(env)->ProfileUserFunctions);
#endif

        oldArgument = core_get_evaluation_data(env)->current_expression;
        core_get_evaluation_data(env)->current_expression = problem;

        switch( fptr->return_type )
        {
        case RT_VOID:

            if( fptr->environment_aware )
            {
                (*(void(*) (void *))fptr->functionPointer)(env);
            }
            else
            {
                (*(void(*) (void))fptr->functionPointer)();
            }

            ret->type = RVOID;
            ret->value = get_false(env);
            break;
        case RT_BOOL:
            ret->type = ATOM;

            if( fptr->environment_aware )
            {
                if((*(int(*) (void *))fptr->functionPointer)(env))
                {
                    ret->value = get_true(env);
                }
                else
                {
                    ret->value = get_false(env);
                }
            }
            else
            {
                if((*(int(*) (void))fptr->functionPointer)())
                {
                    ret->value = get_true(env);
                }
                else
                {
                    ret->value = get_false(env);
                }
            }

            break;
        case RT_EXT_ADDRESS:
            ret->type = EXTERNAL_ADDRESS;

            if( fptr->environment_aware )
            {
                ret->value = (*(void *(*)(void *))fptr->functionPointer)(env);
            }
            else
            {
                ret->value = (*(void *(*)(void))fptr->functionPointer)();
            }

            break;
        case RT_LONG_LONG:
            ret->type = INTEGER;

            if( fptr->environment_aware )
            {
                ret->value = (void *) store_long(env, (*(long long(*) (void *))fptr->functionPointer)(env));
            }
            else
            {
                ret->value = (void *) store_long(env, (*(long long(*) (void))fptr->functionPointer)());
            }

            break;
        case RT_INT:
            ret->type = INTEGER;

            if( fptr->environment_aware )
            {
                ret->value = (void *) store_long(env, (long long)(*(int(*) (void *))fptr->functionPointer)(env));
            }
            else
            {
                ret->value = (void *) store_long(env, (long long)(*(int(*) (void))fptr->functionPointer)());
            }

            break;
        case RT_LONG:
            ret->type = INTEGER;

            if( fptr->environment_aware )
            {
                ret->value = (void *) store_long(env, (long long)(*(long int(*) (void *))fptr->functionPointer)(env));
            }
            else
            {
                ret->value = (void *) store_long(env, (long long)(*(long int(*) (void))fptr->functionPointer)());
            }

            break;
        case RT_FLOAT:
            ret->type = FLOAT;

            if( fptr->environment_aware )
            {
                ret->value = (void *) store_double(env, (double)(*(float(*) (void *))fptr->functionPointer)(env));
            }
            else
            {
                ret->value = (void *) store_double(env, (double)(*(float(*) (void))fptr->functionPointer)());
            }

            break;
        case RT_DOUBLE:
            ret->type = FLOAT;

            if( fptr->environment_aware )
            {
                ret->value = (void *) store_double(env, (*(double(*) (void *))fptr->functionPointer)(env));
            }
            else
            {
                ret->value = (void *) store_double(env, (*(double(*) (void))fptr->functionPointer)());
            }

            break;
        case RT_STRING:
            ret->type = STRING;

            if( fptr->environment_aware )
            {
                ret->value = (void *)
                (*(ATOM_HN * (*)(void *))fptr->functionPointer)(env);
            }
            else
            {
                ret->value = (void *)
                (*(ATOM_HN * (*)(void))fptr->functionPointer)();
            }

            break;
        case RT_ATOM:
            ret->type = ATOM;

            if( fptr->environment_aware )
            {
                ret->value = (void *)
                (*(ATOM_HN * (*)(void *))fptr->functionPointer)(env);
            }
            else
            {
                ret->value = (void *)
                (*(ATOM_HN * (*)(void))fptr->functionPointer)();
            }

            break;
#if OBJECT_SYSTEM
        case RT_INST_ADDRESS:
            ret->type = INSTANCE_ADDRESS;

            if( fptr->environment_aware )
            {
                ret->value =
                    (*(void *(*)(void *))fptr->functionPointer)(env);
            }
            else
            {
                ret->value =
                    (*(void *(*)(void))fptr->functionPointer)();
            }

            break;
        case RT_INSTANCE:
            ret->type = INSTANCE_NAME;

            if( fptr->environment_aware )
            {
                ret->value = (void *)
                (*(ATOM_HN * (*)(void *))fptr->functionPointer)(env);
            }
            else
            {
                ret->value = (void *)
                (*(ATOM_HN * (*)(void))fptr->functionPointer)();
            }

            break;
#endif
        case RT_CHAR:
        {
            char cbuff[2];

            if( fptr->environment_aware )
            {
                cbuff[0] = (*(char(*) (void *))fptr->functionPointer)(env);
            }
            else
            {
                cbuff[0] = (*(char(*) (void))fptr->functionPointer)();
            }

            cbuff[1] = EOS;
            ret->type = ATOM;
            ret->value = (void *)store_atom(env, cbuff);
            break;
        }

        case RT_ATOM_STRING_INST:
        case RT_ATOM_STRING:
        case RT_LIST:
        case RT_INT_FLOAT:
        case RT_UNKNOWN:

            if( fptr->environment_aware )
            {
                (*(void(*) (void *, core_data_object_ptr))fptr->functionPointer)(env, ret);
            }
            else
            {
                (*(void(*) (core_data_object_ptr))fptr->functionPointer)(ret);
            }

            break;

        default:
            error_system(env, ERROR_TAG_EVALUATION, 2);
            exit_router(env, EXIT_FAILURE);
            break;
        }

#if PROFILING_FUNCTIONS
        EndProfile(env, &profileFrame);
#endif

        core_set_environment_function_context(env, oldContext);
        core_get_evaluation_data(env)->current_expression = oldArgument;

        break;
    }

    case LIST:
        ret->type = LIST;
        ret->value = ((core_data_object_ptr)(problem->value))->value;
        ret->begin = ((core_data_object_ptr)(problem->value))->begin;
        ret->end = ((core_data_object_ptr)(problem->value))->end;
        break;

    case LIST_VARIABLE:
    case SCALAR_VARIABLE:

        if( lookup_binding(env, ret, (ATOM_HN *)problem->value) == FALSE )
        {
            error_print_id(env, ERROR_TAG_EVALUATION, 1, FALSE);
            print_router(env, WERROR, ERROR_MSG_VAR_UNBOUND);
            print_router(env, WERROR, to_string(problem->value));
            print_router(env, WERROR, ".\n");
            ret->type = ATOM;
            ret->value = get_false(env);
            core_set_eval_error(env, TRUE);
        }

        break;

    default:

        if( core_get_evaluation_data(env)->primitives[problem->type] == NULL )
        {
            error_system(env, ERROR_TAG_EVALUATION, 3);
            exit_router(env, EXIT_FAILURE);
        }

        if( core_get_evaluation_data(env)->primitives[problem->type]->lazy )
        {
            ret->type = problem->type;
            ret->value = problem->value;
            break;
        }

        if( core_get_evaluation_data(env)->primitives[problem->type]->eval_fn == NULL )
        {
            error_system(env, ERROR_TAG_EVALUATION, 4);
            exit_router(env, EXIT_FAILURE);
        }

        oldArgument = core_get_evaluation_data(env)->current_expression;
        core_get_evaluation_data(env)->current_expression = problem;

#if PROFILING_FUNCTIONS
        StartProfile(env, &profileFrame,
                     &core_get_evaluation_data(env)->primitives[problem->type]->ext_data,
                     ProfileFunctionData(env)->ProfileUserFunctions);
#endif

        (*core_get_evaluation_data(env)->primitives[problem->type]->eval_fn)(env, problem->value, ret);

#if PROFILING_FUNCTIONS
        EndProfile(env, &profileFrame);
#endif

        core_get_evaluation_data(env)->current_expression = oldArgument;
        break;
    }

    core_pass_return_value(env, ret);

    return(core_get_evaluation_data(env)->eval_error);
}

/*****************************************
 * core_install_primitive: Installs a primitive
 *   data type in the primitives array.
 ******************************************/
void core_install_primitive(void *env, struct core_data_entity *thePrimitive, int whichPosition)
{
    if( core_get_evaluation_data(env)->primitives[whichPosition] != NULL )
    {
        error_system(env, ERROR_TAG_EVALUATION, 5);
        exit_router(env, EXIT_FAILURE);
    }

    core_get_evaluation_data(env)->primitives[whichPosition] = thePrimitive;
}

/*****************************************************
 * core_install_ext_address_type: Installs an external
 *   address type in the external address type array.
 ******************************************************/
int core_install_ext_address_type(void *env, struct core_external_address_type *theAddressType)
{
    struct core_external_address_type *copyEAT;

    int rv = core_get_evaluation_data(env)->address_type_count;

    if( core_get_evaluation_data(env)->address_type_count == MAXIMUM_EXTERNAL_ADDRESS_TYPES )
    {
        error_system(env, ERROR_TAG_EVALUATION, 6);
        exit_router(env, EXIT_FAILURE);
    }

    copyEAT = (struct core_external_address_type *)core_mem_alloc(env, sizeof(struct core_external_address_type));
    memcpy(copyEAT, theAddressType, sizeof(struct core_external_address_type));
    core_get_evaluation_data(env)->ext_address_types[core_get_evaluation_data(env)->address_type_count++] = copyEAT;

    return(rv);
}

/*****************************************************
 * core_set_eval_error: Sets the eval_error flag.
 ******************************************************/
void core_set_eval_error(void *env, int value)
{
    core_get_evaluation_data(env)->eval_error = value;

    if( value == TRUE )
    {
        core_get_evaluation_data(env)->halt = TRUE;
    }
}

/********************************************************
 * core_get_eval_error: Returns the eval_error flag.
 *********************************************************/
int core_get_eval_error(void *env)
{
    return(core_get_evaluation_data(env)->eval_error);
}

/*************************************************
 * core_set_halt_eval: Sets the halt flag.
 **************************************************/
void core_set_halt_eval(void *env, int value)
{
    core_get_evaluation_data(env)->halt = value;
}

/****************************************************
 * core_get_halt_eval: Returns the halt flag.
 *****************************************************/
int core_get_halt_eval(void *env)
{
    return(core_get_evaluation_data(env)->halt);
}

/*****************************************************
 * core_release_data: Returns a linked list of core_data_object
 *   structures to the pool of free memory.
 ******************************************************/
void core_release_data(void *env, core_data_object_ptr garbagePtr, BOOLEAN decrementSupplementalInfo)
{
    core_data_object_ptr nextPtr;

    while( garbagePtr != NULL )
    {
        nextPtr = garbagePtr->next;
        core_value_decrement(env, garbagePtr);

        if((garbagePtr->metadata != NULL) && decrementSupplementalInfo )
        {
            dec_atom_count(env, (struct atom_hash_node *)garbagePtr->metadata);
        }

        core_mem_return_struct(env, core_data, garbagePtr);
        garbagePtr = nextPtr;
    }
}

/**************************************************
 * core_print_data: Prints a core_data_object structure
 *   to the specified logical name.
 ***************************************************/
void core_print_data(void *env, char *fileid, core_data_object_ptr argPtr)
{
    switch( argPtr->type )
    {
    case RVOID:
    case ATOM:
    case STRING:
    case INTEGER:
    case FLOAT:
    case EXTERNAL_ADDRESS:
#if OBJECT_SYSTEM
    case INSTANCE_NAME:
    case INSTANCE_ADDRESS:
#endif
        core_print_atom(env, fileid, argPtr->type, argPtr->value);
        break;

    case LIST:
        print_list(env, fileid, (struct list *)argPtr->value,
                   argPtr->begin, argPtr->end, TRUE);
        break;

    default:

        if( core_get_evaluation_data(env)->primitives[argPtr->type] != NULL )
        {
            if( core_get_evaluation_data(env)->primitives[argPtr->type]->long_print_fn )
            {
                (*core_get_evaluation_data(env)->primitives[argPtr->type]->long_print_fn)(env, fileid, argPtr->value);
                break;
            }
            else if( core_get_evaluation_data(env)->primitives[argPtr->type]->short_print_fn )
            {
                (*core_get_evaluation_data(env)->primitives[argPtr->type]->short_print_fn)(env, fileid, argPtr->value);
                break;
            }
        }

        print_router(env, fileid, "<UnknownPrintType");
        core_print_long(env, fileid, (long int)argPtr->type);
        print_router(env, fileid, ">");
        core_set_halt_eval(env, TRUE);
        core_set_eval_error(env, TRUE);
        break;
    }
}

/***************************************************
 * core_create_error_list: Creates a list
 *   value of length zero for error returns.
 ****************************************************/
void core_create_error_list(void *env, core_data_object_ptr ret)
{
    ret->type = LIST;
    ret->value = create_list(env, 0L);
    ret->begin = 1;
    ret->end = 0;
}

/*************************************************
 * core_value_increment: Increments the appropriate count
 *   (in use) values for a core_data_object structure.
 **************************************************/
void core_value_increment(void *env, core_data_object *vPtr)
{
    if( vPtr->type == LIST )
    {
        install_list(env, (struct list *)vPtr->value);
    }
    else
    {
        core_install_data(env, vPtr->type, vPtr->value);
    }
}

/***************************************************
 * core_value_decrement: Decrements the appropriate count
 *   (in use) values for a core_data_object structure.
 ****************************************************/
void core_value_decrement(void *env, core_data_object *vPtr)
{
    if( vPtr->type == LIST )
    {
        uninstall_list(env, (struct list *)vPtr->value);
    }
    else
    {
        core_decrement_atom(env, vPtr->type, vPtr->value);
    }
}

/****************************************
 * core_install_data: Increments the reference
 *   count of an atomic data type.
 *****************************************/
void core_install_data(void *env, int type, void *vPtr)
{
    switch( type )
    {
    case ATOM:
    case STRING:
#if OBJECT_SYSTEM
    case INSTANCE_NAME:
#endif
        inc_atom_count(vPtr);
        break;

    case FLOAT:
        inc_float_count(vPtr);
        break;

    case INTEGER:
        inc_integer_count(vPtr);
        break;

    case EXTERNAL_ADDRESS:
        inc_external_address_count(vPtr);
        break;

    case LIST:
        install_list(env, (struct list *)vPtr);
        break;

    case RVOID:
        break;

    default:

        if( core_get_evaluation_data(env)->primitives[type] == NULL )
        {
            break;
        }

        if( core_get_evaluation_data(env)->primitives[type]->bitmap )
        {
            inc_bitmap_count(vPtr);
        }
        else if( core_get_evaluation_data(env)->primitives[type]->increment_busy )
        {
            (*core_get_evaluation_data(env)->primitives[type]->increment_busy)(env, vPtr);
        }

        break;
    }
}

/******************************************
 * core_decrement_atom: Decrements the reference
 *   count of an atomic data type.
 *******************************************/
void core_decrement_atom(void *env, int type, void *vPtr)
{
    switch( type )
    {
    case ATOM:
    case STRING:
#if OBJECT_SYSTEM
    case INSTANCE_NAME:
#endif
        dec_atom_count(env, (ATOM_HN *)vPtr);
        break;

    case FLOAT:
        dec_float_count(env, (FLOAT_HN *)vPtr);
        break;

    case INTEGER:
        dec_integer_count(env, (INTEGER_HN *)vPtr);
        break;

    case EXTERNAL_ADDRESS:
        dec_external_address_count(env, (EXTERNAL_ADDRESS_HN *)vPtr);
        break;

    case LIST:
        uninstall_list(env, (struct list *)vPtr);
        break;

    case RVOID:
        break;

    default:

        if( core_get_evaluation_data(env)->primitives[type] == NULL )
        {
            break;
        }

        if( core_get_evaluation_data(env)->primitives[type]->bitmap )
        {
            dec_bitmap_count(env, (BITMAP_HN *)vPtr);
        }
        else if( core_get_evaluation_data(env)->primitives[type]->decrement_busy )
        {
            (*core_get_evaluation_data(env)->primitives[type]->decrement_busy)(env, vPtr);
        }
    }
}

/********************************************************************
 * core_pass_return_value: Decrements the associated depth for a value
 *   stored in a core_data_object structure. In effect, the values
 *   returned by certain evaluations (such as a deffunction call)
 *   are passed up to the previous depth of evaluation. The return
 *   value's depth is decremented so that it will not be garbage
 *   collected along with other items that are no longer is_needed from
 *   the evaluation that generated the return value.
 *********************************************************************/
void core_pass_return_value(void *env, core_data_object *vPtr)
{
    long i;
    struct list *list_segment;
    struct node *list;

    if( vPtr->type != LIST )
    {
        _pass_atom(env, vPtr->type, vPtr->value);
    }
    else
    {
        list_segment = (struct list *)vPtr->value;

        if( list_segment->depth > core_get_evaluation_data(env)->eval_depth )
        {
            list_segment->depth = (short)core_get_evaluation_data(env)->eval_depth;
        }

        list = list_segment->cell;

        for( i = 0; i < list_segment->length; i++ )
        {
            _pass_atom(env, list[i].type, list[i].value);
        }
    }
}

/****************************************
 * PropagateReturnAtom: Support function
 *   for core_pass_return_value.
 *****************************************/
static void _pass_atom(void *env, int type, void *value)
{
    switch( type )
    {
    case INTEGER:
    case FLOAT:
    case ATOM:
    case STRING:
    case EXTERNAL_ADDRESS:
#if OBJECT_SYSTEM
    case INSTANCE_NAME:
#endif

        if(((ATOM_HN *)value)->depth > core_get_evaluation_data(env)->eval_depth )
        {
            ((ATOM_HN *)value)->depth = core_get_evaluation_data(env)->eval_depth;
        }

        break;

#if OBJECT_SYSTEM
    case INSTANCE_ADDRESS:

        if(((INSTANCE_TYPE *)value)->depth > core_get_evaluation_data(env)->eval_depth )
        {
            ((INSTANCE_TYPE *)value)->depth = core_get_evaluation_data(env)->eval_depth;
        }

        break;
#endif
    }
}

/**********************************************
 * core_copy_data: Copies the values
 *   directly from a source core_data_object to a
 *   destination core_data_object.
 ***********************************************/
void core_copy_data(core_data_object *dst, core_data_object *src)
{
    dst->type = src->type;
    dst->value = src->value;
    dst->begin = src->begin;
    dst->end = src->end;
    dst->metadata = src->metadata;
    dst->next = src->next;
}

/***********************************************************************
 * core_convert_data_to_expression: Converts the value stored in a data object
 *   into an expression. For list values, a chain of expressions
 *   is generated and the chain is linked by the next_arg field. For a
 *   single field value, a single expression is created.
 ************************************************************************/
struct core_expression *core_convert_data_to_expression(void *env, core_data_object *val)
{
    long i;
    struct core_expression *head = NULL, *last = NULL, *newItem;

    if( core_get_pointer_type(val) != LIST )
    {
        return(core_generate_constant(env, core_get_pointer_type(val), core_get_pointer_value(val)));
    }

    for( i = core_get_data_ptr_start(val); i <= core_get_data_ptr_end(val); i++ )
    {
        newItem = core_generate_constant(env, get_list_node_type(core_get_pointer_value(val), i),
                                         get_list_node_value(core_get_pointer_value(val), i));

        if( last == NULL ) {head = newItem;}
        else {last->next_arg = newItem;}

        last = newItem;
    }

    if( head == NULL )
    {
        return(core_generate_constant(env, FCALL, (void *)core_lookup_function(env, FUNC_NAME_CREATE_LIST)));
    }

    return(head);
}

/***************************************
 * core_hash_atom: Returns the hash
 *   value for an atomic data type.
 ****************************************/
unsigned long core_hash_atom(unsigned short type, void *value, int position)
{
    unsigned long tvalue;

    union
    {
        double        fv;
        void *        vv;
        unsigned long liv;
    } fis;

    switch( type )
    {
    case FLOAT:
        fis.fv = to_double(value);
        tvalue = fis.liv;
        break;

    case INTEGER:
        tvalue = (unsigned long)to_long(value);
        break;

    case EXTERNAL_ADDRESS:
        fis.liv = 0;
        fis.vv = to_external_address(value);
        tvalue = (unsigned long)fis.liv;
        break;

#if OBJECT_SYSTEM
    case INSTANCE_ADDRESS:

        fis.liv = 0;
        fis.vv = value;
        tvalue = (unsigned long)fis.liv;
        break;
#endif

    case STRING:
#if OBJECT_SYSTEM
    case INSTANCE_NAME:
#endif
    case ATOM:
        tvalue = ((ATOM_HN *)value)->bucket;
        break;

    default:
        tvalue = type;
    }

    if( position < 0 )
    {
        return(tvalue);
    }

    return((unsigned long)(tvalue * (((unsigned long)position) + 29)));
}

/**********************************************************
 * core_convert_expression_to_function: Returns an expression with
 *   an appropriate expression reference to the specified
 *   name if it is the name of a deffunction, defgeneric,
 *   or user/system defined function.
 ***********************************************************/
struct core_expression *core_convert_expression_to_function(void *env, char *name)
{
#if DEFGENERIC_CONSTRUCT
    void *gfunc;
#endif
#if DEFFUNCTION_CONSTRUCT
    void *dptr;
#endif
    struct core_function_definition *fptr;

    /*=====================================================
     * Check to see if the function call is a deffunction.
     *=====================================================*/

#if DEFFUNCTION_CONSTRUCT

    if((dptr = (void *)lookup_function_in_scope(env, name)) != NULL )
    {
        return(core_generate_constant(env, PCALL, dptr));
    }

#endif

    /*====================================================
     * Check to see if the function call is a defgeneric.
     *====================================================*/

#if DEFGENERIC_CONSTRUCT

    if((gfunc = (void *)LookupDefgenericInScope(env, name)) != NULL )
    {
        return(core_generate_constant(env, GCALL, gfunc));
    }

#endif

    /*======================================
     * Check to see if the function call is
     * a system or user defined function.
     *======================================*/

    if((fptr = core_lookup_function(env, name)) != NULL )
    {
        return(core_generate_constant(env, FCALL, fptr));
    }

    /*===================================================
     * The specified function name is not a deffunction,
     * defgeneric, or user/system defined function.
     *===================================================*/

    return(NULL);
}

/*****************************************************************
 * core_get_function_referrence: Fills an expression with an appropriate
 *   expression reference to the specified name if it is the
 *   name of a deffunction, defgeneric, or user/system defined
 *   function.
 ******************************************************************/
BOOLEAN core_get_function_referrence(void *env, char *name, FUNCTION_REFERENCE *ref)
{
#if DEFGENERIC_CONSTRUCT
    void *gfunc;
#endif
#if DEFFUNCTION_CONSTRUCT
    void *dptr;
#endif
    struct core_function_definition *fptr;

    ref->next_arg = NULL;
    ref->args = NULL;
    ref->type = RVOID;
    ref->value = NULL;

    /*=====================================================
     * Check to see if the function call is a deffunction.
     *=====================================================*/

#if DEFFUNCTION_CONSTRUCT

    if((dptr = (void *)lookup_function_in_scope(env, name)) != NULL )
    {
        ref->type = PCALL;
        ref->value = dptr;
        return(TRUE);
    }

#endif

    /*====================================================
     * Check to see if the function call is a defgeneric.
     *====================================================*/

#if DEFGENERIC_CONSTRUCT

    if((gfunc = (void *)LookupDefgenericInScope(env, name)) != NULL )
    {
        ref->type = GCALL;
        ref->value = gfunc;
        return(TRUE);
    }

#endif

    /*======================================
     * Check to see if the function call is
     * a system or user defined function.
     *======================================*/

    if((fptr = core_lookup_function(env, name)) != NULL )
    {
        ref->type = FCALL;
        ref->value = fptr;
        return(TRUE);
    }

    /*===================================================
     * The specified function name is not a deffunction,
     * defgeneric, or user/system defined function.
     *===================================================*/

    return(FALSE);
}

/******************************************************
 * PrintCAddress:
 *******************************************************/
static void _print_pointer(void *env, char *logicalName, void *val)
{
    char buffer[20];

    print_router(env, logicalName, "<Pointer-C-");

    sysdep_sprintf(buffer, "%p", to_external_address(val));
    print_router(env, logicalName, buffer);
    print_router(env, logicalName, ">");
}

/******************************************************
 * NewCAddress:
 *******************************************************/
static void _new_ext_address(void *env, core_data_object *rv)
{
    int numberOfArguments;

    numberOfArguments = core_get_arg_count(env);

    if( numberOfArguments != 1 )
    {
        error_print_id(env, "NEW", 1, FALSE);
        print_router(env, WERROR, "Function new expected no additional arguments for the C external language type.\n");
        core_set_eval_error(env, TRUE);
        return;
    }

    core_set_pointer_type(rv, EXTERNAL_ADDRESS);
    core_set_pointer_value(rv, store_external_address(env, NULL, 0));
}
