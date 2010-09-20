/* Purpose: Contains the code for several string functions
 *   including str-cat, sym-cat, str-length, str-compare,
 *   upcase, lowcase, sub-string, str-index, eval, and
 *   build.                                                  */

#define __FUNCS_STRING_SOURCE__

#include "setup.h"

#if STRING_FUNCTIONS

#include <stdio.h>
#define _STDIO_INCLUDED_
#include <ctype.h>
#include <string.h>

#include "core_arguments.h"
#include "core_command_prompt.h"
#include "core_constructs.h"
#include "parser_constructs.h"
#include "core_environment.h"
#include "parser_expressions.h"
#include "core_functions.h"
#include "core_memory.h"
#include "parser_flow_control.h"
#include "router.h"
#include "router_string.h"
#include "core_scanner.h"
#include "sysdep.h"


#include "funcs_string.h"

/**************************************
 * LOCAL FUNCTION PROTOTYPES
 ***************************************/

static void StrOrSymCatFunction(void *, core_data_object_ptr, unsigned short);

/*****************************************
 * StringFunctionDefinitions: Initializes
 *   the string manipulation functions.
 ******************************************/
void StringFunctionDefinitions(void *env)
{
    core_define_function(env, "str-cat", 'k', PTR_FN StrCatFunction, "StrCatFunction", "1*");
    core_define_function(env, "sym-cat", 'k', PTR_FN SymCatFunction, "SymCatFunction", "1*");
    core_define_function(env, "str-length", 'g', PTR_FN StrLengthFunction, "StrLengthFunction", "11j");
    core_define_function(env, "str-compare", 'g', PTR_FN StrCompareFunction, "StrCompareFunction", "23*jji");
    core_define_function(env, "upcase", 'j', PTR_FN UpcaseFunction, "UpcaseFunction", "11j");
    core_define_function(env, "lowcase", 'j', PTR_FN LowcaseFunction, "LowcaseFunction", "11j");
    core_define_function(env, "sub-string", 's', PTR_FN SubStringFunction, "SubStringFunction", "33*iij");
    core_define_function(env, "str-index", 'u', PTR_FN StrIndexFunction, "StrIndexFunction", "22j");

    core_define_function(env, "build", 'b', PTR_FN BuildFunction, "BuildFunction", "11k");
    core_define_function(env, "string-to-field", 'u', PTR_FN StringToFieldFunction, "StringToFieldFunction", "11j");
}

/***************************************
 * StrCatFunction: H/L access routine
 *   for the str-cat function.
 ****************************************/
void StrCatFunction(void *env, core_data_object_ptr ret)
{
    StrOrSymCatFunction(env, ret, STRING);
}

/***************************************
 * SymCatFunction: H/L access routine
 *   for the sym-cat function.
 ****************************************/
void SymCatFunction(void *env, core_data_object_ptr ret)
{
    StrOrSymCatFunction(env, ret, ATOM);
}

/*******************************************************
 * StrOrSymCatFunction: Driver routine for implementing
 *   the str-cat and sym-cat functions.
 ********************************************************/
static void StrOrSymCatFunction(void *env, core_data_object_ptr ret, unsigned short returnType)
{
    core_data_object theArg;
    int numArgs, i, total, j;
    char *representation;
    ATOM_HN **arrayOfStrings;
    ATOM_HN *hashPtr;
    char *functionName;

    /*============================================
     * Determine the calling function name.
     * Store the null string or the symbol nil as
     * the return value in the event of an error.
     *============================================*/

    core_set_pointer_type(ret, returnType);

    if( returnType == STRING )
    {
        functionName = "str-cat";
        core_set_pointer_value(ret, (void *)store_atom(env, ""));
    }
    else
    {
        functionName = "sym-cat";
        core_set_pointer_value(ret, (void *)store_atom(env, "nil"));
    }

    /*===============================================
     * Determine the number of arguments as create a
     * string array which is large enough to store
     * the string representation of each argument.
     *===============================================*/

    numArgs = core_get_arg_count(env);
    arrayOfStrings = (ATOM_HN **)core_mem_alloc_and_init(env, (int)sizeof(ATOM_HN *) * numArgs);

    for( i = 0; i < numArgs; i++ )
    {
        arrayOfStrings[i] = NULL;
    }

    /*=============================================
     * Evaluate each argument and store its string
     * representation in the string array.
     *=============================================*/

    total = 1;

    for( i = 1 ; i <= numArgs ; i++ )
    {
        core_get_arg_at(env, i, &theArg);

        switch( core_get_type(theArg))
        {
        case STRING:
#if OBJECT_SYSTEM
        case INSTANCE_NAME:
#endif
        case ATOM:
            hashPtr = (ATOM_HN *)core_get_value(theArg);
            arrayOfStrings[i - 1] = hashPtr;
            inc_atom_count(hashPtr);
            break;

        case FLOAT:
            hashPtr = (ATOM_HN *)store_atom(env, core_convert_float_to_string(env, to_double(core_get_value(theArg))));
            arrayOfStrings[i - 1] = hashPtr;
            inc_atom_count(hashPtr);
            break;

        case INTEGER:
            hashPtr = (ATOM_HN *)store_atom(env, core_conver_long_to_string(env, to_long(core_get_value(theArg))));
            arrayOfStrings[i - 1] = hashPtr;
            inc_atom_count(hashPtr);
            break;

        default:
            report_explicit_type_error(env, functionName, i, "string, instance name, symbol, float, or integer");
            core_set_eval_error(env, TRUE);
            break;
        }

        if( core_get_evaluation_data(env)->eval_error )
        {
            for( i = 0; i < numArgs; i++ )
            {
                if( arrayOfStrings[i] != NULL )
                {
                    dec_atom_count(env, arrayOfStrings[i]);
                }
            }

            core_mem_release(env, arrayOfStrings, sizeof(ATOM_HN *) * numArgs);
            return;
        }

        total += (int)strlen(to_string(arrayOfStrings[i - 1]));
    }

    /*=========================================================
     * Allocate the memory to store the concatenated string or
     * symbol, then copy the values in the string array to the
     * memory just allocated.
     *=========================================================*/

    representation = (char *)core_mem_alloc_no_init(env, (sizeof(char) * total));

    j = 0;

    for( i = 0 ; i < numArgs ; i++ )
    {
        sysdep_sprintf(&representation[j], "%s", to_string(arrayOfStrings[i]));
        j += (int)strlen(to_string(arrayOfStrings[i]));
    }

    /*=========================================
     * Return the concatenated value and clean
     * up the temporary memory used.
     *=========================================*/

    core_set_pointer_value(ret, (void *)store_atom(env, representation));
    core_mem_release(env, representation, sizeof(char) * total);

    for( i = 0; i < numArgs; i++ )
    {
        if( arrayOfStrings[i] != NULL )
        {
            dec_atom_count(env, arrayOfStrings[i]);
        }
    }

    core_mem_release(env, arrayOfStrings, sizeof(ATOM_HN *) * numArgs);
}

/******************************************
 * StrLengthFunction: H/L access routine
 *   for the str-length function.
 *******************************************/
long long StrLengthFunction(void *env)
{
    core_data_object theArg;

    /*===================================================
     * Function str-length expects exactly one argument.
     *===================================================*/

    if( core_check_arg_count(env, "str-length", EXACTLY, 1) == -1 )
    {
        return(-1LL);
    }

    /*==================================================
     * The argument should be of type symbol or string.
     *==================================================*/

    if( core_check_arg_type(env, "str-length", 1, ATOM_OR_STRING, &theArg) == FALSE )
    {
        return(-1LL);
    }

    /*============================================
     * Return the length of the string or symbol.
     *============================================*/

    return( (long long)strlen(core_convert_data_to_string(theArg)));
}

/***************************************
 * UpcaseFunction: H/L access routine
 *   for the upcase function.
 ****************************************/
void UpcaseFunction(void *env, core_data_object_ptr ret)
{
    core_data_object theArg;
    unsigned i;
    size_t slen;
    char *osptr, *nsptr;

    /*===============================================
     * Function upcase expects exactly one argument.
     *===============================================*/

    if( core_check_arg_count(env, "upcase", EXACTLY, 1) == -1 )
    {
        core_set_pointer_type(ret, STRING);
        core_set_pointer_value(ret, (void *)store_atom(env, ""));
        return;
    }

    /*==================================================
     * The argument should be of type symbol or string.
     *==================================================*/

    if( core_check_arg_type(env, "upcase", 1, ATOM_OR_STRING, &theArg) == FALSE )
    {
        core_set_pointer_type(ret, STRING);
        core_set_pointer_value(ret, (void *)store_atom(env, ""));
        return;
    }

    /*======================================================
     * Allocate temporary memory and then copy the original
     * string or symbol to that memory, while uppercasing
     * lower case alphabetic characters.
     *======================================================*/

    osptr = core_convert_data_to_string(theArg);
    slen = strlen(osptr) + 1;
    nsptr = (char *)core_mem_alloc_no_init(env, slen);

    for( i = 0  ; i < slen ; i++ )
    {
        if( islower(osptr[i]))
        {
            nsptr[i] = (char)toupper(osptr[i]);
        }
        else
        {
            nsptr[i] = osptr[i];
        }
    }

    /*========================================
     * Return the uppercased string and clean
     * up the temporary memory used.
     *========================================*/

    core_set_pointer_type(ret, core_get_type(theArg));
    core_set_pointer_value(ret, (void *)store_atom(env, nsptr));
    core_mem_release(env, nsptr, slen);
}

/****************************************
 * LowcaseFunction: H/L access routine
 *   for the lowcase function.
 *****************************************/
void LowcaseFunction(void *env, core_data_object_ptr ret)
{
    core_data_object theArg;
    unsigned i;
    size_t slen;
    char *osptr, *nsptr;

    /*================================================
     * Function lowcase expects exactly one argument.
     *================================================*/

    if( core_check_arg_count(env, "lowcase", EXACTLY, 1) == -1 )
    {
        core_set_pointer_type(ret, STRING);
        core_set_pointer_value(ret, (void *)store_atom(env, ""));
        return;
    }

    /*==================================================
     * The argument should be of type symbol or string.
     *==================================================*/

    if( core_check_arg_type(env, "lowcase", 1, ATOM_OR_STRING, &theArg) == FALSE )
    {
        core_set_pointer_type(ret, STRING);
        core_set_pointer_value(ret, (void *)store_atom(env, ""));
        return;
    }

    /*======================================================
     * Allocate temporary memory and then copy the original
     * string or symbol to that memory, while lowercasing
     * upper case alphabetic characters.
     *======================================================*/

    osptr = core_convert_data_to_string(theArg);
    slen = strlen(osptr) + 1;
    nsptr = (char *)core_mem_alloc_no_init(env, slen);

    for( i = 0  ; i < slen ; i++ )
    {
        if( isupper(osptr[i]))
        {
            nsptr[i] = (char)tolower(osptr[i]);
        }
        else
        {
            nsptr[i] = osptr[i];
        }
    }

    /*========================================
     * Return the lowercased string and clean
     * up the temporary memory used.
     *========================================*/

    core_set_pointer_type(ret, core_get_type(theArg));
    core_set_pointer_value(ret, (void *)store_atom(env, nsptr));
    core_mem_release(env, nsptr, slen);
}

/*******************************************
 * StrCompareFunction: H/L access routine
 *   for the str-compare function.
 ********************************************/
long long StrCompareFunction(void *env)
{
    int numArgs, length;
    core_data_object arg1, arg2, arg3;
    long long ret;

    /*=======================================================
     * Function str-compare expects either 2 or 3 arguments.
     *=======================================================*/

    if((numArgs = core_check_arg_range(env, "str-compare", 2, 3)) == -1 )
    {
        return(0L);
    }

    /*=============================================================
     * The first two arguments should be of type symbol or string.
     *=============================================================*/

    if( core_check_arg_type(env, "str-compare", 1, ATOM_OR_STRING, &arg1) == FALSE )
    {
        return(0L);
    }

    if( core_check_arg_type(env, "str-compare", 2, ATOM_OR_STRING, &arg2) == FALSE )
    {
        return(0L);
    }

    /*===================================================
     * Compare the strings. Use the 3rd argument for the
     * maximum length of comparison, if it is provided.
     *===================================================*/

    if( numArgs == 3 )
    {
        if( core_check_arg_type(env, "str-compare", 3, INTEGER, &arg3) == FALSE )
        {
            return(0L);
        }

        length = core_coerce_to_integer(core_get_type(arg3), core_get_value(arg3));
        ret = strncmp(core_convert_data_to_string(arg1), core_convert_data_to_string(arg2),
                      (STD_SIZE)length);
    }
    else
    {
        ret = strcmp(core_convert_data_to_string(arg1), core_convert_data_to_string(arg2));
    }

    /*========================================================
     * Return Values are as follows:
     * -1 is returned if <string-1> is less than <string-2>.
     *  1 is return if <string-1> is greater than <string-2>.
     *  0 is returned if <string-1> is equal to <string-2>.
     *========================================================*/

    if( ret < 0 )
    {
        ret = -1;
    }
    else if( ret > 0 )
    {
        ret = 1;
    }

    return(ret);
}

/******************************************
 * SubStringFunction: H/L access routine
 *   for the sub-string function.
 *******************************************/
void *SubStringFunction(void *env)
{
    core_data_object arg;
    char *tempString, *returnString;
    int start, end, i, j;
    void *ret;

    /*===================================
     * Check and retrieve the arguments.
     *===================================*/

    if( core_check_arg_count(env, "sub-string", EXACTLY, 3) == -1 )
    {
        return((void *)store_atom(env, ""));
    }

    if( core_check_arg_type(env, "sub-string", 1, INTEGER, &arg) == FALSE )
    {
        return((void *)store_atom(env, ""));
    }

    start = core_coerce_to_integer(arg.type, arg.value) - 1;

    if( core_check_arg_type(env, "sub-string", 2, INTEGER, &arg) == FALSE )
    {
        return((void *)store_atom(env, ""));
    }

    end = core_coerce_to_integer(arg.type, arg.value) - 1;

    if( core_check_arg_type(env, "sub-string", 3, ATOM_OR_STRING, &arg) == FALSE )
    {
        return((void *)store_atom(env, ""));
    }

    /*================================================
     * If parameters are out of range return an error
     *================================================*/

    if( start < 0 )
    {
        start = 0;
    }

    if( end > (int)strlen(core_convert_data_to_string(arg)))
    {
        end = (int)strlen(core_convert_data_to_string(arg));
    }

    /*==================================
     * If the start is greater than the
     * end, return a null string.
     *==================================*/

    if( start > end )
    {
        return((void *)store_atom(env, ""));
    }

    /*=============================================
     * Otherwise, allocate the string and copy the
     * designated portion of the old string to the
     * new string.
     *=============================================*/

    else
    {
        returnString = (char *)core_mem_alloc_no_init(env, (unsigned)(end - start + 2));  /* (end - start) inclusive + EOS */
        tempString = core_convert_data_to_string(arg);

        for( j = 0, i = start; i <= end; i++, j++ )
        {
            *(returnString + j) = *(tempString + i);
        }

        *(returnString + j) = '\0';
    }

    /*========================
     * Return the new string.
     *========================*/

    ret = (void *)store_atom(env, returnString);
    core_mem_release(env, returnString, (unsigned)(end - start + 2));
    return(ret);
}

/*****************************************
 * StrIndexFunction: H/L access routine
 *   for the sub-index function.
 ******************************************/
void StrIndexFunction(void *env, core_data_object_ptr result)
{
    core_data_object arg1, arg2;
    char *strg1, *strg2;
    size_t i, j;

    result->type = ATOM;
    result->value = get_false(env);

    /*===================================
     * Check and retrieve the arguments.
     *===================================*/

    if( core_check_arg_count(env, "str-index", EXACTLY, 2) == -1 )
    {
        return;
    }

    if( core_check_arg_type(env, "str-index", 1, ATOM_OR_STRING, &arg1) == FALSE )
    {
        return;
    }

    if( core_check_arg_type(env, "str-index", 2, ATOM_OR_STRING, &arg2) == FALSE )
    {
        return;
    }

    strg1 = core_convert_data_to_string(arg1);
    strg2 = core_convert_data_to_string(arg2);

    /*=================================
     * Find the position in string2 of
     * string1 (counting from 1).
     *=================================*/

    if( strlen(strg1) == 0 )
    {
        result->type = INTEGER;
        result->value = (void *)store_long(env, (long long)strlen(strg2) + 1LL);
        return;
    }

    for( i = 1; *strg2; i++, strg2++ )
    {
        for( j = 0; *(strg1 + j) && *(strg1 + j) == *(strg2 + j); j++ )
        {     /* Do Nothing */
        }

        if( *(strg1 + j) == '\0' )
        {
            result->type = INTEGER;
            result->value = (void *)store_long(env, (long long)i);
            return;
        }
    }

    return;
}

/*******************************************
 * StringToFieldFunction: H/L access routine
 *   for the string-to-field function.
 ********************************************/
void StringToFieldFunction(void *env, core_data_object *ret)
{
    core_data_object theArg;

    /*========================================================
     * Function string-to-field expects exactly one argument.
     *========================================================*/

    if( core_check_arg_count(env, "string-to-field", EXACTLY, 1) == -1 )
    {
        ret->type = STRING;
        ret->value = (void *)store_atom(env, "*** ERROR ***");
        return;
    }

    /*==================================================
     * The argument should be of type symbol or string.
     *==================================================*/

    if( core_check_arg_type(env, "string-to-field", 1, ATOM_OR_STRING, &theArg) == FALSE )
    {
        ret->type = STRING;
        ret->value = (void *)store_atom(env, "*** ERROR ***");
        return;
    }

    /*================================
     * Convert the string to an atom.
     *================================*/

    StringToField(env, core_convert_data_to_string(theArg), ret);
}

/************************************************************
 * StringToField: Converts a string to an atomic data value.
 *************************************************************/
void StringToField(void *env, char *representation, core_data_object *ret)
{
    struct token theToken;

    /*====================================
     * Open the string as an input source
     * and retrieve the first value.
     *====================================*/

    open_string_source(env, "string-to-field-str", representation, 0);
    core_get_token(env, "string-to-field-str", &theToken);
    close_string_source(env, "string-to-field-str");

    /*====================================================
     * Copy the token to the return value data structure.
     *====================================================*/

    ret->type = theToken.type;

    if((theToken.type == FLOAT) || (theToken.type == STRING) ||
#if OBJECT_SYSTEM
       (theToken.type == INSTANCE_NAME) ||
#endif
       (theToken.type == ATOM) || (theToken.type == INTEGER))
    {
        ret->value = theToken.value;
    }
    else if( theToken.type == STOP )
    {
        ret->type = ATOM;
        ret->value = (void *)store_atom(env, "EOF");
    }
    else if( theToken.type == UNKNOWN_VALUE )
    {
        ret->type = STRING;
        ret->value = (void *)store_atom(env, "*** ERROR ***");
    }
    else
    {
        ret->type = STRING;
        ret->value = (void *)store_atom(env, theToken.pp);
    }
}

/**************************************
 * BuildFunction: H/L access routine
 *   for the build function.
 ***************************************/
int BuildFunction(void *env)
{
    core_data_object theArg;

    /*==============================================
     * Function build expects exactly one argument.
     *==============================================*/

    if( core_check_arg_count(env, "build", EXACTLY, 1) == -1 )
    {
        return(FALSE);
    }

    /*==================================================
     * The argument should be of type ATOM or STRING.
     *==================================================*/

    if( core_check_arg_type(env, "build", 1, ATOM_OR_STRING, &theArg) == FALSE )
    {
        return(FALSE);
    }

    /*======================
     * Build the construct.
     *======================*/

    return(EnvBuild(env, core_convert_data_to_string(theArg)));
}

/*****************************
 * EnvBuild: C access routine
 *   for the build function.
 ******************************/
int EnvBuild(void *env, char *representation)
{
    char *type;
    struct token theToken;
    int errorFlag;

    /*===========================================
     * Create a string source router so that the
     * string can be used as an input source.
     *===========================================*/

    if( open_string_source(env, "build", representation, 0) == 0 )
    {
        return(FALSE);
    }

    /*================================
     * The first token of a construct
     * must be a left parenthesis.
     *================================*/

    core_get_token(env, "build", &theToken);

    if( theToken.type != LPAREN )
    {
        close_string_source(env, "build");
        return(FALSE);
    }

    /*==============================================
     * The next token should be the construct type.
     *==============================================*/

    core_get_token(env, "build", &theToken);

    if( theToken.type != ATOM )
    {
        close_string_source(env, "build");
        return(FALSE);
    }

    type = to_string(theToken.value);

    /*======================
     * Parse the construct.
     *======================*/

    errorFlag = parse_construct(env, type, "build");

    /*=================================
     * Close the string source router.
     *=================================*/

    close_string_source(env, "build");

    /*=========================================
     * If an error occured while parsing the
     * construct, then print an error message.
     *=========================================*/

    if( errorFlag == 1 )
    {
        print_router(env, WERROR, "\nERROR:\n");
        core_print_chunkify(env, WERROR, core_get_pp_buffer_handle(env));
        print_router(env, WERROR, "\n");
    }

    core_delete_pp_buffer(env);

    /*===========================================
     * Perform periodic cleanup if the build was
     * issued from an embedded controller.
     *===========================================*/

    if((core_get_evaluation_data(env)->eval_depth == 0) && (!CommandLineData(env)->EvaluatingTopLevelCommand) &&
       (core_get_evaluation_data(env)->current_expression == NULL))
    {
        core_gc_periodic_cleanup(env, TRUE, FALSE);
    }

    /*===============================================
     * Return TRUE if the construct was successfully
     * parsed, otherwise return FALSE.
     *===============================================*/

    if( errorFlag == 0 )
    {
        return(TRUE);
    }

    return(FALSE);
}

#endif /* STRING_FUNCTIONS */
