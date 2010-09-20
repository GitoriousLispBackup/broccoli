/* Purpose: Parsing routines for Implicit System Methods     */

/* =========================================
 *****************************************
 *            EXTERNAL DEFINITIONS
 *  =========================================
 ***************************************** */
#include "setup.h"

#if DEFGENERIC_CONSTRUCT

#include <stdlib.h>

#if OBJECT_SYSTEM
#include "classes_kernel.h"
#include "funcs_class.h"
#endif

#include "core_environment.h"
#include "core_memory.h"
#include "constraints_operators.h"
#include "core_functions.h"
#include "parser_generics.h"
#include "core_functions_util.h"

#define __PARSER_GENERICS_IMPLICIT_SOURCE__
#include "parser_generics_implicit.h"

/* =========================================
 *****************************************
 *   INTERNALLY VISIBLE FUNCTION HEADERS
 *  =========================================
 ***************************************** */

static void          FormMethodsFromRestrictions(void *, DEFGENERIC *, char *, core_expression_object *);
static RESTRICTION * ParseRestrictionType(void *, int);
static core_expression_object *  GenTypeExpression(void *, core_expression_object *, int, int, char *);

/* =========================================
 *****************************************
 *       EXTERNALLY VISIBLE FUNCTIONS
 *  =========================================
 ***************************************** */

/********************************************************
 *  NAME         : AddImplicitMethods
 *  DESCRIPTION  : Adds a method(s) for a generic function
 *                for an overloaded system function
 *  INPUTS       : A pointer to a gneeric function
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Method added
 *  NOTES        : Method marked as system
 *              Assumes no other methods already present
 ********************************************************/
globle void AddImplicitMethods(void *theEnv, DEFGENERIC *gfunc)
{
    struct core_function_definition *sysfunc;
    core_expression_object action;

    sysfunc = core_lookup_function(theEnv, to_string(gfunc->header.name));

    if( sysfunc == NULL )
    {
        return;
    }

    action.type = FCALL;
    action.value = (void *)sysfunc;
    action.next_arg = NULL;
    action.args = NULL;
    FormMethodsFromRestrictions(theEnv, gfunc, sysfunc->restrictions, &action);
}

/* =========================================
 *****************************************
 *       INTERNALLY VISIBLE FUNCTIONS
 *  =========================================
 ***************************************** */

/**********************************************************************
 *  NAME         : FormMethodsFromRestrictions
 *  DESCRIPTION  : Uses restriction string given in DefineFunction2()
 *                for system function to create an equivalent method
 *  INPUTS       : 1) The generic function for the new methods
 *              2) System function restriction string
 *                 (see DefineFunction2() last argument)
 *              3) The actions to attach to a new method(s)
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Implicit method(s) created
 *  NOTES        : None
 **********************************************************************/
static void FormMethodsFromRestrictions(void *theEnv, DEFGENERIC *gfunc, char *rstring, core_expression_object *actions)
{
    DEFMETHOD *meth;
    core_expression_object *plist, *tmp, *bot, *svBot;
    RESTRICTION *rptr;
    char theChar[2], defaultc;
    int min, max, mposn, needMinimumMethod;
    register int i, j;

    /* ===================================
     *  The system function will accept any
     *  number of any type of arguments
     *  =================================== */
    if( rstring == NULL )
    {
        tmp = core_mem_get_struct(theEnv, core_expression);
        rptr = core_mem_get_struct(theEnv, restriction);
        PackRestrictionTypes(theEnv, rptr, NULL);
        rptr->query = NULL;
        tmp->args = (core_expression_object *)rptr;
        tmp->next_arg = NULL;
        meth = AddMethod(theEnv, gfunc, NULL, 0, 0, tmp, 1, 0, (ATOM_HN *)get_true(theEnv),
                         core_pack_expression(theEnv, actions), NULL, FALSE);
        meth->system = 1;
        DeleteTempRestricts(theEnv, tmp);
        return;
    }

    /* ==============================
     *  Extract the range of arguments
     *  from the restriction string
     *  ============================== */
    theChar[1] = '\0';

    if( rstring[0] == '*' )
    {
        min = 0;
    }
    else
    {
        theChar[0] = rstring[0];
        min = atoi(theChar);
    }

    if( rstring[1] == '*' )
    {
        max = -1;
    }
    else
    {
        theChar[0] = rstring[1];
        max = atoi(theChar);
    }

    if( rstring[2] != '\0' )
    {
        defaultc = rstring[2];
        j = 3;
    }
    else
    {
        defaultc = 'u';
        j = 2;
    }

    /* ================================================
     *  Form a list of method restrictions corresponding
     *  to the minimum number of arguments
     *  ================================================ */
    plist = bot = NULL;

    for( i = 0 ; i < min ; i++ )
    {
        theChar[0] = (rstring[j] != '\0') ? rstring[j++] : defaultc;
        rptr = ParseRestrictionType(theEnv, (int)theChar[0]);
        tmp = core_mem_get_struct(theEnv, core_expression);
        tmp->args = (core_expression_object *)rptr;
        tmp->next_arg = NULL;

        if( plist == NULL )
        {
            plist = tmp;
        }
        else
        {
            bot->next_arg = tmp;
        }

        bot = tmp;
    }

    /* ===============================
     *  Remember where restrictions end
     *  for minimum number of arguments
     *  =============================== */
    svBot = bot;
    needMinimumMethod = TRUE;

    /* =======================================================
     *  Attach one or more new methods to correspond
     *  to the possible variations of the extra arguments
     *
     *  Add a separate method for each specified extra argument
     *  ======================================================= */
    i = 0;

    while( rstring[j] != '\0' )
    {
        if((rstring[j + 1] == '\0') && ((min + i + 1) == max))
        {
            defaultc = rstring[j];
            break;
        }

        rptr = ParseRestrictionType(theEnv, (int)rstring[j]);
        tmp = core_mem_get_struct(theEnv, core_expression);
        tmp->args = (core_expression_object *)rptr;
        tmp->next_arg = NULL;

        if( plist == NULL )
        {
            plist = tmp;
        }
        else
        {
            bot->next_arg = tmp;
        }

        bot = tmp;
        i++;
        j++;

        if((rstring[j] != '\0') || ((min + i) == max))
        {
            FindMethodByRestrictions(gfunc, plist, min + i, NULL, &mposn);
            meth = AddMethod(theEnv, gfunc, NULL, mposn, 0, plist, min + i, 0, NULL,
                             core_pack_expression(theEnv, actions), NULL, TRUE);
            meth->system = 1;
        }
    }

    /* ==============================================
     *  Add a method to account for wildcard arguments
     *  and attach a query in case there is a limit
     *  ============================================== */
    if((min + i) != max )
    {
        /* ================================================
         *  If a wildcard is present immediately after the
         *  minimum number of args - then the minimum case
         *  will already be handled by this method. We don't
         *  need to add an extra method for that case
         *  ================================================ */
        if( i == 0 )
        {
            needMinimumMethod = FALSE;
        }

        rptr = ParseRestrictionType(theEnv, (int)defaultc);

        if( max != -1 )
        {
            rptr->query = core_generate_constant(theEnv, FCALL, (void *)core_lookup_function(theEnv, "<="));
            rptr->query->args = core_generate_constant(theEnv, FCALL, (void *)core_lookup_function(theEnv, "length$"));
            rptr->query->args->args = core_generate_generic_function_reference(theEnv, min + i + 1);
            rptr->query->args->next_arg =
                core_generate_constant(theEnv, INTEGER, (void *)store_long(theEnv, (long long)(max - min - i)));
        }

        tmp = core_mem_get_struct(theEnv, core_expression);
        tmp->args = (core_expression_object *)rptr;
        tmp->next_arg = NULL;

        if( plist == NULL )
        {
            plist = tmp;
        }
        else
        {
            bot->next_arg = tmp;
        }

        FindMethodByRestrictions(gfunc, plist, min + i + 1, (ATOM_HN *)get_true(theEnv), &mposn);
        meth = AddMethod(theEnv, gfunc, NULL, mposn, 0, plist, min + i + 1, 0, (ATOM_HN *)get_true(theEnv),
                         core_pack_expression(theEnv, actions), NULL, FALSE);
        meth->system = 1;
    }

    /* ===================================================
     *  When extra methods had to be added because of
     *  different restrictions on the optional arguments OR
     *  the system function accepts a fixed number of args,
     *  we must add a specific method for the minimum case.
     *  Otherwise, the method with the wildcard covers it.
     *  =================================================== */
    if( needMinimumMethod )
    {
        if( svBot != NULL )
        {
            bot = svBot->next_arg;
            svBot->next_arg = NULL;
            DeleteTempRestricts(theEnv, bot);
        }

        FindMethodByRestrictions(gfunc, plist, min, NULL, &mposn);
        meth = AddMethod(theEnv, gfunc, NULL, mposn, 0, plist, min, 0, NULL,
                         core_pack_expression(theEnv, actions), NULL, TRUE);
        meth->system = 1;
    }

    DeleteTempRestricts(theEnv, plist);
}

/*******************************************************************
 *  NAME         : ParseRestrictionType
 *  DESCRIPTION  : Takes a string of type character codes (as given in
 *              DefineFunction2()) and converts it into a method
 *              restriction structure
 *  INPUTS       : The type character code
 *  RETURNS      : The restriction
 *  SIDE EFFECTS : Restriction allocated
 *  NOTES        : None
 *******************************************************************/
static RESTRICTION *ParseRestrictionType(void *theEnv, int code)
{
    RESTRICTION *rptr;
    CONSTRAINT_META *rv;
    core_expression_object *types = NULL;

    rptr = core_mem_get_struct(theEnv, restriction);
    rptr->query = NULL;
    rv = convert_arg_type_to_constraint(theEnv, code);

    if( rv->allow_any == FALSE )
    {
        if( rv->allow_atom && rv->allow_string )
        {
            types = GenTypeExpression(theEnv, types, LEXEME_TYPE_CODE, -1, LEXEME_TYPE_NAME);
        }
        else if( rv->allow_atom )
        {
            types = GenTypeExpression(theEnv, types, ATOM, ATOM, NULL);
        }
        else if( rv->allow_string )
        {
            types = GenTypeExpression(theEnv, types, STRING, STRING, NULL);
        }

        if( rv->allow_float && rv->allow_integer )
        {
            types = GenTypeExpression(theEnv, types, NUMBER_TYPE_CODE, -1, NUMBER_TYPE_NAME);
        }
        else if( rv->allow_integer )
        {
            types = GenTypeExpression(theEnv, types, INTEGER, INTEGER, NULL);
        }
        else if( rv->allow_float )
        {
            types = GenTypeExpression(theEnv, types, FLOAT, FLOAT, NULL);
        }

        if( rv->allow_instance_name && rv->allow_instance_pointer )
        {
            types = GenTypeExpression(theEnv, types, INSTANCE_TYPE_CODE, -1, INSTANCE_TYPE_NAME);
        }
        else if( rv->allow_instance_name )
        {
            types = GenTypeExpression(theEnv, types, INSTANCE_NAME, INSTANCE_NAME, NULL);
        }
        else if( rv->allow_instance_pointer )
        {
            types = GenTypeExpression(theEnv, types, INSTANCE_ADDRESS, INSTANCE_ADDRESS, NULL);
        }

        if( rv->allow_external_pointer && rv->allow_instance_pointer )
        {
            types = GenTypeExpression(theEnv, types, ADDRESS_TYPE_CODE, -1, ADDRESS_TYPE_NAME);
        }
        else
        {
            if( rv->allow_external_pointer )
            {
                types = GenTypeExpression(theEnv, types, EXTERNAL_ADDRESS, EXTERNAL_ADDRESS, NULL);
            }

            if( rv->allow_instance_pointer && (rv->allow_instance_name == 0))
            {
                types = GenTypeExpression(theEnv, types, INSTANCE_ADDRESS, INSTANCE_ADDRESS, NULL);
            }
        }

        if( rv->allow_list )
        {
            types = GenTypeExpression(theEnv, types, LIST, LIST, NULL);
        }
    }

    removeConstraint(theEnv, rv);
    PackRestrictionTypes(theEnv, rptr, types);
    return(rptr);
}

/***************************************************
 *  NAME         : GenTypeExpression
 *  DESCRIPTION  : Creates an expression corresponding
 *              to the type specified and adds it
 *              to the front of a temporary type
 *              list for a method restriction
 *  INPUTS       : 1) The top of the current type list
 *              2) The type code when COOL is
 *                 not installed
 *              3) The primitive type (-1 if not
 *                 a primitive type)
 *              4) The name of the COOL class if
 *                 it is not a primitive type
 *  RETURNS      : The new top of the types list
 *  SIDE EFFECTS : Type node allocated and attached
 *  NOTES        : Restriction types in a non-COOL
 *              environment are the type codes
 *              given in CONSTANT.H.  In a COOL
 *              environment, they are pointers
 *              to classes
 ***************************************************/
#if WIN_BTC
#pragma argsused
#endif
static core_expression_object *GenTypeExpression(void *theEnv, core_expression_object *top, int nonCOOLCode, int primitiveCode, char *COOLName)
{
#if OBJECT_SYSTEM
#if MAC_MCW || WIN_MCW || MAC_XCD
#pragma unused(nonCOOLCode)
#endif
#else
#if MAC_MCW || WIN_MCW || MAC_XCD
#pragma unused(primitiveCode)
#pragma unused(COOLName)
#endif
#endif
    core_expression_object *tmp;

#if OBJECT_SYSTEM

    if( primitiveCode != -1 )
    {
        tmp = core_generate_constant(theEnv, 0, (void *)DefclassData(theEnv)->PrimitiveClassMap[primitiveCode]);
    }
    else
    {
        tmp = core_generate_constant(theEnv, 0, (void *)LookupDefclassByMdlOrScope(theEnv, COOLName));
    }

#else
    tmp = core_generate_constant(theEnv, 0, store_long(theEnv, (long long)nonCOOLCode));
#endif
    tmp->next_arg = top;
    return(tmp);
}

#endif
