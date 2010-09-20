/* Purpose: Generic Function Execution Routines              */

/* =========================================
 *****************************************
 *            EXTERNAL DEFINITIONS
 *  =========================================
 ***************************************** */
#include "setup.h"

#if DEFGENERIC_CONSTRUCT

#if OBJECT_SYSTEM
#include "classes_kernel.h"
#include "funcs_class.h"
#include "funcs_instance.h"
#endif

#include "core_arguments.h"
#include "core_constructs.h"
#include "core_environment.h"
#include "generics_kernel.h"
#include "funcs_function.h"
#include "core_functions_util.h"
#include "funcs_profiling.h"
#include "router.h"
#include "core_gc.h"

#define __GENERICS_CALL_SOURCE__
#include "generics_call.h"

/* =========================================
 *****************************************
 *                CONSTANTS
 *  =========================================
 ***************************************** */

#define BEGIN_TRACE     ">>"
#define END_TRACE       "<<"

/* =========================================
 *****************************************
 *   INTERNALLY VISIBLE FUNCTION HEADERS
 *  =========================================
 ***************************************** */

static DEFMETHOD *FindApplicableMethod(void *, DEFGENERIC *, DEFMETHOD *);

#if DEBUGGING_FUNCTIONS
static void WatchGeneric(void *, char *);
static void WatchMethod(void *, char *);
#endif

#if OBJECT_SYSTEM
static DEFCLASS *DetermineRestrictionClass(void *, core_data_object *);
#endif

/* =========================================
 *****************************************
 *       EXTERNALLY VISIBLE FUNCTIONS
 *  =========================================
 ***************************************** */

/***********************************************************************************
 *  NAME         : GenericDispatch
 *  DESCRIPTION  : Executes the most specific applicable method
 *  INPUTS       : 1) The generic function
 *              2) The method to start after in the search for an applicable
 *                 method (ignored if arg #3 is not NULL).
 *              3) A specific method to call (NULL if want highest precedence
 *                 method to be called)
 *              4) The generic function argument expressions
 *              5) The caller's result value buffer
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Any side-effects of evaluating the generic function arguments
 *              Any side-effects of evaluating query functions on method parameter
 *                restrictions when determining the core (see warning #1)
 *              Any side-effects of actual execution of methods (see warning #2)
 *              Caller's buffer set to the result of the generic function call
 *
 *              In case of errors, the result is FALSE, otherwise it is the
 *                result returned by the most specific method (which can choose
 *                to ignore or return the values of more general methods)
 *  NOTES        : WARNING #1: Query functions on method parameter restrictions
 *                 should not have side-effects, for they might be evaluated even
 *                 for methods that aren't applicable to the generic function call.
 *              WARNING #2: Side-effects of method execution should not always rely
 *                 on only being executed once per generic function call.  Every
 *                 time a method calls (shadow-call) the same next-most-specific
 *                 method is executed.  Thus, it is possible for a method to be
 *                 executed multiple times per generic function call.
 ***********************************************************************************/
globle void GenericDispatch(void *theEnv, DEFGENERIC *gfunc, DEFMETHOD *prevmeth, DEFMETHOD *meth, core_expression_object *params, core_data_object *result)
{
    DEFGENERIC *previousGeneric;
    DEFMETHOD *previousMethod;
    int oldce;

#if PROFILING_FUNCTIONS
    struct profileFrameInfo profileFrame;
#endif

    result->type = ATOM;
    result->value = get_false(theEnv);
    core_get_evaluation_data(theEnv)->eval_error = FALSE;

    if( core_get_evaluation_data(theEnv)->halt )
    {
        return;
    }

    oldce = core_is_construct_executing(theEnv);
    core_set_construct_executing(theEnv, TRUE);
    previousGeneric = DefgenericData(theEnv)->CurrentGeneric;
    previousMethod = DefgenericData(theEnv)->CurrentMethod;
    DefgenericData(theEnv)->CurrentGeneric = gfunc;
    core_get_evaluation_data(theEnv)->eval_depth++;
    gfunc->busy++;
    core_push_function_args(theEnv, params, core_count_args(params),
                       EnvGetDefgenericName(theEnv, (void *)gfunc),
                       "generic function", UnboundMethodErr);

    if( core_get_evaluation_data(theEnv)->eval_error )
    {
        gfunc->busy--;
        DefgenericData(theEnv)->CurrentGeneric = previousGeneric;
        DefgenericData(theEnv)->CurrentMethod = previousMethod;
        core_get_evaluation_data(theEnv)->eval_depth--;
        core_gc_periodic_cleanup(theEnv, FALSE, TRUE);
        core_set_construct_executing(theEnv, oldce);
        return;
    }

    if( meth != NULL )
    {
        if( IsMethodApplicable(theEnv, meth))
        {
            meth->busy++;
            DefgenericData(theEnv)->CurrentMethod = meth;
        }
        else
        {
            error_print_id(theEnv, "GENRCEXE", 4, FALSE);
            core_set_eval_error(theEnv, TRUE);
            DefgenericData(theEnv)->CurrentMethod = NULL;
            print_router(theEnv, WERROR, "Generic function ");
            print_router(theEnv, WERROR, EnvGetDefgenericName(theEnv, (void *)gfunc));
            print_router(theEnv, WERROR, " method #");
            core_print_long(theEnv, WERROR, (long long)meth->index);
            print_router(theEnv, WERROR, " is not applicable to the given arguments.\n");
        }
    }
    else
    {
        DefgenericData(theEnv)->CurrentMethod = FindApplicableMethod(theEnv, gfunc, prevmeth);
    }

    if( DefgenericData(theEnv)->CurrentMethod != NULL )
    {
#if DEBUGGING_FUNCTIONS

        if( DefgenericData(theEnv)->CurrentGeneric->trace )
        {
            WatchGeneric(theEnv, BEGIN_TRACE);
        }

        if( DefgenericData(theEnv)->CurrentMethod->trace )
        {
            WatchMethod(theEnv, BEGIN_TRACE);
        }

#endif

        if( DefgenericData(theEnv)->CurrentMethod->system )
        {
            core_expression_object fcall;

            fcall.type = FCALL;
            fcall.value = DefgenericData(theEnv)->CurrentMethod->actions->value;
            fcall.next_arg = NULL;
            fcall.args = core_garner_generic_args(theEnv);
            core_eval_expression(theEnv, &fcall, result);
        }
        else
        {
#if PROFILING_FUNCTIONS
            StartProfile(theEnv, &profileFrame,
                         &DefgenericData(theEnv)->CurrentMethod->ext_data,
                         ProfileFunctionData(theEnv)->ProfileConstructs);
#endif

            core_eval_function_actions(theEnv, DefgenericData(theEnv)->CurrentGeneric->header.my_module->module_def,
                                DefgenericData(theEnv)->CurrentMethod->actions, DefgenericData(theEnv)->CurrentMethod->localVarCount,
                                result, UnboundMethodErr);

#if PROFILING_FUNCTIONS
            EndProfile(theEnv, &profileFrame);
#endif
        }

        DefgenericData(theEnv)->CurrentMethod->busy--;
#if DEBUGGING_FUNCTIONS

        if( DefgenericData(theEnv)->CurrentMethod->trace )
        {
            WatchMethod(theEnv, END_TRACE);
        }

        if( DefgenericData(theEnv)->CurrentGeneric->trace )
        {
            WatchGeneric(theEnv, END_TRACE);
        }

#endif
    }
    else if( !core_get_evaluation_data(theEnv)->eval_error )
    {
        error_print_id(theEnv, "GENRCEXE", 1, FALSE);
        print_router(theEnv, WERROR, "No applicable methods for ");
        print_router(theEnv, WERROR, EnvGetDefgenericName(theEnv, (void *)gfunc));
        print_router(theEnv, WERROR, ".\n");
        core_set_eval_error(theEnv, TRUE);
    }

    gfunc->busy--;
    get_flow_control_data(theEnv)->return_flag = FALSE;
    core_pop_function_args(theEnv);
    DefgenericData(theEnv)->CurrentGeneric = previousGeneric;
    DefgenericData(theEnv)->CurrentMethod = previousMethod;
    core_get_evaluation_data(theEnv)->eval_depth--;
    core_pass_return_value(theEnv, result);
    core_gc_periodic_cleanup(theEnv, FALSE, TRUE);
    core_set_construct_executing(theEnv, oldce);
}

/*******************************************************
 *  NAME         : UnboundMethodErr
 *  DESCRIPTION  : Print out a synopis of the currently
 *                executing method for unbound variable
 *                errors
 *  INPUTS       : None
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Error synopsis printed to WERROR
 *  NOTES        : None
 *******************************************************/
globle void UnboundMethodErr(void *theEnv)
{
    print_router(theEnv, WERROR, "generic function ");
    print_router(theEnv, WERROR, EnvGetDefgenericName(theEnv, (void *)DefgenericData(theEnv)->CurrentGeneric));
    print_router(theEnv, WERROR, " method #");
    core_print_long(theEnv, WERROR, (long long)DefgenericData(theEnv)->CurrentMethod->index);
    print_router(theEnv, WERROR, ".\n");
}

/***********************************************************************
 *  NAME         : IsMethodApplicable
 *  DESCRIPTION  : Tests to see if a method satsifies the arguments of a
 *                generic function
 *              A method is applicable if all its restrictions are
 *                satisfied by the corresponding arguments
 *  INPUTS       : The method address
 *  RETURNS      : TRUE if method is applicable, FALSE otherwise
 *  SIDE EFFECTS : Any query functions are evaluated
 *  NOTES        : Uses globals arguments_sz and arguments
 ***********************************************************************/
globle BOOLEAN IsMethodApplicable(void *theEnv, DEFMETHOD *meth)
{
    core_data_object temp;
    short i, j, k;
    register RESTRICTION *rp;

#if OBJECT_SYSTEM
    void *type;
#else
    int type;
#endif

    if((core_get_function_primitive_data(theEnv)->arguments_sz < meth->minRestrictions) ||
       ((core_get_function_primitive_data(theEnv)->arguments_sz > meth->minRestrictions) && (meth->maxRestrictions != -1)))
    {
        return(FALSE);
    }

    for( i = 0, k = 0 ; i < core_get_function_primitive_data(theEnv)->arguments_sz ; i++ )
    {
        rp = &meth->restrictions[k];

        if( rp->tcnt != 0 )
        {
#if OBJECT_SYSTEM
            type = (void *)DetermineRestrictionClass(theEnv, &core_get_function_primitive_data(theEnv)->arguments[i]);

            if( type == NULL )
            {
                return(FALSE);
            }

            for( j = 0 ; j < rp->tcnt ; j++ )
            {
                if( type == rp->types[j] )
                {
                    break;
                }

                if( HasSuperclass((DEFCLASS *)type, (DEFCLASS *)rp->types[j]))
                {
                    break;
                }

                if( rp->types[j] == (void *)DefclassData(theEnv)->PrimitiveClassMap[INSTANCE_ADDRESS] )
                {
                    if( core_get_function_primitive_data(theEnv)->arguments[i].type == INSTANCE_ADDRESS )
                    {
                        break;
                    }
                }
                else if( rp->types[j] == (void *)DefclassData(theEnv)->PrimitiveClassMap[INSTANCE_NAME] )
                {
                    if( core_get_function_primitive_data(theEnv)->arguments[i].type == INSTANCE_NAME )
                    {
                        break;
                    }
                }
                else if( rp->types[j] ==
                         (void *)DefclassData(theEnv)->PrimitiveClassMap[INSTANCE_NAME]->directSuperclasses.classArray[0] )
                {
                    if((core_get_function_primitive_data(theEnv)->arguments[i].type == INSTANCE_NAME) ||
                       (core_get_function_primitive_data(theEnv)->arguments[i].type == INSTANCE_ADDRESS))
                    {
                        break;
                    }
                }
            }

#else
            type = core_get_function_primitive_data(theEnv)->arguments[i].type;

            for( j = 0 ; j < rp->tcnt ; j++ )
            {
                if( type == to_int(rp->types[j]))
                {
                    break;
                }

                if( SubsumeType(type, to_int(rp->types[j])))
                {
                    break;
                }
            }

#endif

            if( j == rp->tcnt )
            {
                return(FALSE);
            }
        }

        if( rp->query != NULL )
        {
            DefgenericData(theEnv)->GenericCurrentArgument = &core_get_function_primitive_data(theEnv)->arguments[i];
            core_eval_expression(theEnv, rp->query, &temp);

            if((temp.type != ATOM) ? FALSE :
               (temp.value == get_false(theEnv)))
            {
                return(FALSE);
            }
        }

        if(((int)k) != meth->restrictionCount - 1 )
        {
            k++;
        }
    }

    return(TRUE);
}

/***************************************************
 *  NAME         : NextMethodP
 *  DESCRIPTION  : Determines if a shadowed generic
 *                function method is available for
 *                execution
 *  INPUTS       : None
 *  RETURNS      : TRUE if there is a method available,
 *                FALSE otherwise
 *  SIDE EFFECTS : None
 *  NOTES        : H/L Syntax: (next-methodp)
 ***************************************************/
globle int NextMethodP(void *theEnv)
{
    register DEFMETHOD *meth;

    if( DefgenericData(theEnv)->CurrentMethod == NULL )
    {
        return(FALSE);
    }

    meth = FindApplicableMethod(theEnv, DefgenericData(theEnv)->CurrentGeneric, DefgenericData(theEnv)->CurrentMethod);

    if( meth != NULL )
    {
        meth->busy--;
        return(TRUE);
    }

    return(FALSE);
}

/****************************************************
 *  NAME         : CallNextMethod
 *  DESCRIPTION  : Executes the next available method
 *                in the core for a generic function
 *  INPUTS       : Caller's buffer for the result
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Side effects of execution of shadow
 *              eval_error set if no method
 *                is available to execute.
 *  NOTES        : H/L Syntax: (call-next-method)
 ****************************************************/
globle void CallNextMethod(void *theEnv, core_data_object *result)
{
    DEFMETHOD *oldMethod;

#if PROFILING_FUNCTIONS
    struct profileFrameInfo profileFrame;
#endif

    result->type = ATOM;
    result->value = get_false(theEnv);

    if( core_get_evaluation_data(theEnv)->halt )
    {
        return;
    }

    oldMethod = DefgenericData(theEnv)->CurrentMethod;

    if( DefgenericData(theEnv)->CurrentMethod != NULL )
    {
        DefgenericData(theEnv)->CurrentMethod = FindApplicableMethod(theEnv, DefgenericData(theEnv)->CurrentGeneric, DefgenericData(theEnv)->CurrentMethod);
    }

    if( DefgenericData(theEnv)->CurrentMethod == NULL )
    {
        DefgenericData(theEnv)->CurrentMethod = oldMethod;
        error_print_id(theEnv, "GENRCEXE", 2, FALSE);
        print_router(theEnv, WERROR, "Shadowed methods not applicable in current context.\n");
        core_set_eval_error(theEnv, TRUE);
        return;
    }

#if DEBUGGING_FUNCTIONS

    if( DefgenericData(theEnv)->CurrentMethod->trace )
    {
        WatchMethod(theEnv, BEGIN_TRACE);
    }

#endif

    if( DefgenericData(theEnv)->CurrentMethod->system )
    {
        core_expression_object fcall;

        fcall.type = FCALL;
        fcall.value = DefgenericData(theEnv)->CurrentMethod->actions->value;
        fcall.next_arg = NULL;
        fcall.args = core_garner_generic_args(theEnv);
        core_eval_expression(theEnv, &fcall, result);
    }
    else
    {
#if PROFILING_FUNCTIONS
        StartProfile(theEnv, &profileFrame,
                     &DefgenericData(theEnv)->CurrentGeneric->header.ext_data,
                     ProfileFunctionData(theEnv)->ProfileConstructs);
#endif

        core_eval_function_actions(theEnv, DefgenericData(theEnv)->CurrentGeneric->header.my_module->module_def,
                            DefgenericData(theEnv)->CurrentMethod->actions, DefgenericData(theEnv)->CurrentMethod->localVarCount,
                            result, UnboundMethodErr);

#if PROFILING_FUNCTIONS
        EndProfile(theEnv, &profileFrame);
#endif
    }

    DefgenericData(theEnv)->CurrentMethod->busy--;
#if DEBUGGING_FUNCTIONS

    if( DefgenericData(theEnv)->CurrentMethod->trace )
    {
        WatchMethod(theEnv, END_TRACE);
    }

#endif
    DefgenericData(theEnv)->CurrentMethod = oldMethod;
    get_flow_control_data(theEnv)->return_flag = FALSE;
}

/**************************************************************************
 *  NAME         : CallSpecificMethod
 *  DESCRIPTION  : Allows a specific method to be called without regards to
 *                higher precedence methods which might also be applicable
 *                However, shadowed methods can still be called.
 *  INPUTS       : A data object buffer to hold the method evaluation result
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Side-effects of method applicability tests and the
 *              evaluation of methods
 *  NOTES        : H/L Syntax: (call-specific-method
 *                             <generic-function> <method-index> <args>)
 **************************************************************************/
globle void CallSpecificMethod(void *theEnv, core_data_object *result)
{
    core_data_object temp;
    DEFGENERIC *gfunc;
    int mi;

    result->type = ATOM;
    result->value = get_false(theEnv);

    if( core_check_arg_type(theEnv, "call-specific-method", 1, ATOM, &temp) == FALSE )
    {
        return;
    }

    gfunc = CheckGenericExists(theEnv, "call-specific-method", core_convert_data_to_string(temp));

    if( gfunc == NULL )
    {
        return;
    }

    if( core_check_arg_type(theEnv, "call-specific-method", 2, INTEGER, &temp) == FALSE )
    {
        return;
    }

    mi = CheckMethodExists(theEnv, "call-specific-method", gfunc, (long)core_convert_data_to_long(temp));

    if( mi == -1 )
    {
        return;
    }

    gfunc->methods[mi].busy++;
    GenericDispatch(theEnv, gfunc, NULL, &gfunc->methods[mi],
                    core_get_first_arg()->next_arg->next_arg, result);
    gfunc->methods[mi].busy--;
}

/***********************************************************************
 *  NAME         : OverrideNextMethod
 *  DESCRIPTION  : Changes the arguments to shadowed methods, thus the set
 *              of applicable methods to this call may change
 *  INPUTS       : A buffer to hold the result of the call
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Any of evaluating method restrictions and bodies
 *  NOTES        : H/L Syntax: (override-next-method <args>)
 ***********************************************************************/
globle void OverrideNextMethod(void *theEnv, core_data_object *result)
{
    result->type = ATOM;
    result->value = get_false(theEnv);

    if( core_get_evaluation_data(theEnv)->halt )
    {
        return;
    }

    if( DefgenericData(theEnv)->CurrentMethod == NULL )
    {
        error_print_id(theEnv, "GENRCEXE", 2, FALSE);
        print_router(theEnv, WERROR, "Shadowed methods not applicable in current context.\n");
        core_set_eval_error(theEnv, TRUE);
        return;
    }

    GenericDispatch(theEnv, DefgenericData(theEnv)->CurrentGeneric, DefgenericData(theEnv)->CurrentMethod, NULL,
                    core_get_first_arg(), result);
}

/***********************************************************
 *  NAME         : GetGenericCurrentArgument
 *  DESCRIPTION  : Returns the value of the generic function
 *                argument being tested in the method
 *                applicability determination process
 *  INPUTS       : A data-object buffer
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Data-object set
 *  NOTES        : Useful for queries in wildcard restrictions
 ***********************************************************/
globle void GetGenericCurrentArgument(void *theEnv, core_data_object *result)
{
    result->type = DefgenericData(theEnv)->GenericCurrentArgument->type;
    result->value = DefgenericData(theEnv)->GenericCurrentArgument->value;
    result->begin = DefgenericData(theEnv)->GenericCurrentArgument->begin;
    result->end = DefgenericData(theEnv)->GenericCurrentArgument->end;
}

/* =========================================
 *****************************************
 *       INTERNALLY VISIBLE FUNCTIONS
 *  =========================================
 ***************************************** */

/************************************************************
 *  NAME         : FindApplicableMethod
 *  DESCRIPTION  : Finds the first/next applicable
 *                method for a generic function call
 *  INPUTS       : 1) The generic function pointer
 *              2) The address of the current method
 *                 (NULL to find the first)
 *  RETURNS      : The address of the first/next
 *                applicable method (NULL on errors)
 *  SIDE EFFECTS : Any from evaluating query restrictions
 *              Methoid busy count incremented if applicable
 *  NOTES        : None
 ************************************************************/
static DEFMETHOD *FindApplicableMethod(void *theEnv, DEFGENERIC *gfunc, DEFMETHOD *meth)
{
    if( meth != NULL )
    {
        meth++;
    }
    else
    {
        meth = gfunc->methods;
    }

    for(; meth < &gfunc->methods[gfunc->mcnt] ; meth++ )
    {
        meth->busy++;

        if( IsMethodApplicable(theEnv, meth))
        {
            return(meth);
        }

        meth->busy--;
    }

    return(NULL);
}

#if DEBUGGING_FUNCTIONS

/**********************************************************************
 *  NAME         : WatchGeneric
 *  DESCRIPTION  : Prints out a trace of the beginning or end
 *                of the execution of a generic function
 *  INPUTS       : A string to indicate beginning or end of execution
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : None
 *  NOTES        : Uses the globals CurrentGeneric, arguments_sz and
 *                arguments for other trace info
 **********************************************************************/
static void WatchGeneric(void *theEnv, char *tstring)
{
    print_router(theEnv, WTRACE, "GNC ");
    print_router(theEnv, WTRACE, tstring);
    print_router(theEnv, WTRACE, " ");

    if( DefgenericData(theEnv)->CurrentGeneric->header.my_module->module_def != ((struct module_definition *)get_current_module(theEnv)))
    {
        print_router(theEnv, WTRACE, get_module_name(theEnv, (void *)
                                                           DefgenericData(theEnv)->CurrentGeneric->header.my_module->module_def));
        print_router(theEnv, WTRACE, "::");
    }

    print_router(theEnv, WTRACE, to_string((void *)DefgenericData(theEnv)->CurrentGeneric->header.name));
    print_router(theEnv, WTRACE, " ");
    print_router(theEnv, WTRACE, " ED:");
    core_print_long(theEnv, WTRACE, (long long)core_get_evaluation_data(theEnv)->eval_depth);
    core_print_function_args(theEnv, WTRACE);
}

/**********************************************************************
 *  NAME         : WatchMethod
 *  DESCRIPTION  : Prints out a trace of the beginning or end
 *                of the execution of a generic function
 *                method
 *  INPUTS       : A string to indicate beginning or end of execution
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : None
 *  NOTES        : Uses the globals CurrentGeneric, CurrentMethod,
 *                arguments_sz and arguments for
 *                other trace info
 **********************************************************************/
static void WatchMethod(void *theEnv, char *tstring)
{
    print_router(theEnv, WTRACE, "MTH ");
    print_router(theEnv, WTRACE, tstring);
    print_router(theEnv, WTRACE, " ");

    if( DefgenericData(theEnv)->CurrentGeneric->header.my_module->module_def != ((struct module_definition *)get_current_module(theEnv)))
    {
        print_router(theEnv, WTRACE, get_module_name(theEnv, (void *)
                                                           DefgenericData(theEnv)->CurrentGeneric->header.my_module->module_def));
        print_router(theEnv, WTRACE, "::");
    }

    print_router(theEnv, WTRACE, to_string((void *)DefgenericData(theEnv)->CurrentGeneric->header.name));
    print_router(theEnv, WTRACE, ":#");

    if( DefgenericData(theEnv)->CurrentMethod->system )
    {
        print_router(theEnv, WTRACE, "SYS");
    }

    core_print_long(theEnv, WTRACE, (long long)DefgenericData(theEnv)->CurrentMethod->index);
    print_router(theEnv, WTRACE, " ");
    print_router(theEnv, WTRACE, " ED:");
    core_print_long(theEnv, WTRACE, (long long)core_get_evaluation_data(theEnv)->eval_depth);
    core_print_function_args(theEnv, WTRACE);
}

#endif

#if OBJECT_SYSTEM

/***************************************************
 *  NAME         : DetermineRestrictionClass
 *  DESCRIPTION  : Finds the class of an argument in
 *                the arguments
 *  INPUTS       : The argument data object
 *  RETURNS      : The class address, NULL if error
 *  SIDE EFFECTS : eval_error set on errors
 *  NOTES        : None
 ***************************************************/
static DEFCLASS *DetermineRestrictionClass(void *theEnv, core_data_object *dobj)
{
    INSTANCE_TYPE *ins;
    DEFCLASS *cls;

    if( dobj->type == INSTANCE_NAME )
    {
        ins = FindInstanceBySymbol(theEnv, (ATOM_HN *)dobj->value);
        cls = (ins != NULL) ? ins->cls : NULL;
    }
    else if( dobj->type == INSTANCE_ADDRESS )
    {
        ins = (INSTANCE_TYPE *)dobj->value;
        cls = (ins->garbage == 0) ? ins->cls : NULL;
    }
    else
    {
        return(DefclassData(theEnv)->PrimitiveClassMap[dobj->type]);
    }

    if( cls == NULL )
    {
        core_set_eval_error(theEnv, TRUE);
        error_print_id(theEnv, "GENRCEXE", 3, FALSE);
        print_router(theEnv, WERROR, "Unable to determine class of ");
        core_print_data(theEnv, WERROR, dobj);
        print_router(theEnv, WERROR, " in generic function ");
        print_router(theEnv, WERROR, EnvGetDefgenericName(theEnv, (void *)DefgenericData(theEnv)->CurrentGeneric));
        print_router(theEnv, WERROR, ".\n");
    }

    return(cls);
}

#endif

#endif
