/* Purpose: Utility routines for printing various items
 *   and messages.                                           */

#define __CORE_UTILITIES_SOURCE__

#include <stdio.h>
#define _STDIO_INCLUDED_
#include <string.h>

#include "setup.h"

#include "constant.h"
#include "core_environment.h"
#include "type_symbol.h"
#include "type_list.h"
#include "core_gc.h"
#include "core_evaluation.h"
#include "core_arguments.h"
#include "router.h"
#include "funcs_list.h"
#if OBJECT_SYSTEM
#include "classes_instances_kernel.h"
#include "classes_instances_init.h"
#endif
#include "core_memory.h"
#include "sysdep.h"

#include "core_utilities.h"

/****************************************************
 * core_init_utility_data: Allocates environment
 *    data for print utility routines.
 *****************************************************/
void core_init_utility_data(void *env)
{
    core_allocate_environment_data(env, PRINT_UTILITY_DATA_INDEX, sizeof(struct core_utility_data), NULL);
}

/**********************************************************
 * core_print_chunkify:  Prints a string in chunks to accomodate
 *   systems which have a limit on the maximum size of a
 *   string which can be printed.
 ***********************************************************/
void core_print_chunkify(void *env, char *logicalName, char *bigString)
{
    char tc, *subString;

    subString = bigString;

    if( subString == NULL )
    {
        return;
    }

    while(((int)strlen(subString)) > 500 )
    {
        if( core_get_evaluation_data(env)->halt )
        {
            return;
        }

        tc = subString[500];
        subString[500] = EOS;
        print_router(env, logicalName, subString);
        subString[500] = tc;
        subString += 500;
    }

    print_router(env, logicalName, subString);
}

/***********************************************************
 * core_print_float: Controls printout of floating point numbers.
 ************************************************************/
void core_print_float(void *env, char *fileid, double number)
{
    char *theString;

    theString = core_convert_float_to_string(env, number);
    print_router(env, fileid, theString);
}

/***************************************************
 * core_print_long: Controls printout of integers.
 ****************************************************/
void core_print_long(void *env, char *logicalName, long long number)
{
    char printBuffer[32];

    sysdep_sprintf(printBuffer, "%lld", number);
    print_router(env, logicalName, printBuffer);
}

/***************************************************
 * core_print_long: Controls printout of integers.
 ****************************************************/
void core_print_hex(void *env, char *logicalName, int number)
{
    char printBuffer[32];

    sysdep_sprintf(printBuffer, "%#x", number);
    print_router(env, logicalName, printBuffer);
}

/*************************************
 * core_print_atom: Prints an atomic value.
 **************************************/
void core_print_atom(void *env, char *logicalName, int type, void *value)
{
    struct external_address_hash_node *theAddress;
    char buffer[20];

    switch( type )
    {
    case FLOAT:
        core_print_float(env, logicalName, to_double(value));
        break;
    case INTEGER:
        core_print_long(env, logicalName, to_long(value));
        break;
    case ATOM:
        print_router(env, logicalName, to_string(value));
        break;
    case STRING:

        if( core_get_utility_data(env)->preserving_escape )
        {
            print_router(env, logicalName, core_gc_string_printform(env, to_string(value)));
        }
        else
        {
            print_router(env, logicalName, "\"");
            print_router(env, logicalName, to_string(value));
            print_router(env, logicalName, "\"");
        }

        break;

    case EXTERNAL_ADDRESS:
        theAddress = (struct external_address_hash_node *)value;

        if( core_get_utility_data(env)->stringifying_pointer )
        {
            print_router(env, logicalName, "\"");
        }

        if((core_get_evaluation_data(env)->ext_address_types[theAddress->type] != NULL) &&
           (core_get_evaluation_data(env)->ext_address_types[theAddress->type]->long_print_fn != NULL))
        {
            (*core_get_evaluation_data(env)->ext_address_types[theAddress->type]->long_print_fn)(env, logicalName, value);
        }
        else
        {
            print_router(env, logicalName, "<Pointer-");

            sysdep_sprintf(buffer, "%d-", theAddress->type);
            print_router(env, logicalName, buffer);

            sysdep_sprintf(buffer, "%p", to_external_address(value));
            print_router(env, logicalName, buffer);
            print_router(env, logicalName, ">");
        }

        if( core_get_utility_data(env)->stringifying_pointer )
        {
            print_router(env, logicalName, "\"");
        }

        break;

#if OBJECT_SYSTEM
    case INSTANCE_NAME:
        print_router(env, logical_name, "[");
        print_router(env, logical_name, to_string(value));
        print_router(env, logical_name, "]");
        break;
#endif

    case RVOID:
        break;

    default:

        if( core_get_evaluation_data(env)->primitives[type] == NULL )
        {
            break;
        }

        if( core_get_evaluation_data(env)->primitives[type]->long_print_fn == NULL )
        {
            print_router(env, logicalName, "<unknown atom type>");
            break;
        }

        (*core_get_evaluation_data(env)->primitives[type]->long_print_fn)(env, logicalName, value);
        break;
    }
}

/*********************************************************
 * core_print_tally: Prints a tally count indicating the number
 *   of items that have been displayed.
 **********************************************************/
void core_print_tally(void *env, char *logicalName, long long count, char *singular, char *plural)
{
    if( count == 0 )
    {
        return;
    }

    print_router(env, logicalName, "For a total of ");
    core_print_long(env, logicalName, count);
    print_router(env, logicalName, " ");

    if( count == 1 )
    {
        print_router(env, logicalName, singular);
    }
    else
    {
        print_router(env, logicalName, plural);
    }

    print_router(env, logicalName, ".\n");
}

/*******************************************
 * error_print_id: Prints the module name and
 *   error ID for an error message.
 ********************************************/
void error_print_id(void *env, char *module, int errorID, int printCR)
{
    if( printCR )
    {
        print_router(env, WERROR, "\n");
    }

    print_router(env, WERROR, module);
    print_router(env, WERROR, "[code ");
    core_print_hex(env, WERROR, errorID);
    print_router(env, WERROR, "]: ");
}

/*********************************************
 * warning_print_id: Prints the module name and
 *   warning ID for a warning message.
 **********************************************/
void warning_print_id(void *env, char *module, int warningID, int printCR)
{
    if( printCR )
    {
        print_router(env, WWARNING, "\n");
    }

    print_router(env, WWARNING, module);
    print_router(env, WWARNING, "[code ");
    core_print_hex(env, WWARNING, warningID);
    print_router(env, WWARNING, "]: ");
}

/**************************************************
 * error_lookup: Generic error message
 *  when an "item" can not be found.
 ***************************************************/
void error_lookup(void *env, char *itemType, char *itemName)
{
    error_print_id(env, "ERROR", 1, FALSE);
    print_router(env, WERROR, "Unable to find ");
    print_router(env, WERROR, itemType);
    print_router(env, WERROR, " ");
    print_router(env, WERROR, itemName);
    print_router(env, WERROR, ".\n");
}

/****************************************************
 * error_deletion: Generic error message
 *  when an "item" can not be deleted.
 *****************************************************/
void error_deletion(void *env, char *itemType, char *itemName)
{
    error_print_id(env, "ERROR", 4, FALSE);
    print_router(env, WERROR, "Unable to delete ");
    print_router(env, WERROR, itemType);
    print_router(env, WERROR, " ");
    print_router(env, WERROR, itemName);
    print_router(env, WERROR, ".\n");
}

/***************************************************
 * error_reparse: Generic error message
 *  when an "item" has already been parsed.
 ****************************************************/
void error_reparse(void *env, char *itemType, char *itemName)
{
    error_print_id(env, "ERROR", 5, TRUE);
    print_router(env, WERROR, "The ");

    if( itemType != NULL )
    {
        print_router(env, WERROR, itemType);
    }

    if( itemName != NULL )
    {
        print_router(env, WERROR, itemName);
    }

    print_router(env, WERROR, " has already been parsed.\n");
}

/********************************************************
 * error_syntax: Generalized syntax error message.
 *********************************************************/
void error_syntax(void *env, char *location)
{
    error_print_id(env, "ERROR", 2, TRUE);
    print_router(env, WERROR, "Syntax Error");

    if( location != NULL )
    {
        print_router(env, WERROR, ":  Syntax error ");
        print_router(env, WERROR, location);
    }

    print_router(env, WERROR, ".\n");
    core_set_eval_error(env, TRUE);
}

/***************************************************
 * error_local_variable: Generic error message
 *  when a local variable is accessed by an "item"
 *  which can not access local variables.
 ****************************************************/
void error_local_variable(void *env, char *byWhat)
{
    error_print_id(env, "ERROR", 6, TRUE);
    print_router(env, WERROR, "Local variables inaccessible to ");
    print_router(env, WERROR, byWhat);
    print_router(env, WERROR, ".\n");
}

/*****************************************
 * error_system: Generalized error message
 *   for major internal errors.
 ******************************************/
void error_system(void *env, char *module, int errorID)
{
    error_print_id(env, "Software Failure.  ", 3, TRUE);
    print_router(env, WERROR, APPLICATION_NAME);
    print_router(env, WERROR, "\n");

    print_router(env, WERROR, " Guru Meditation (#");

    print_router(env, WERROR, module);
    core_print_hex(env, WERROR, errorID);
    print_router(env, WERROR, ")\n");

    print_router(env, WERROR, "Please contact ");
    print_router(env, WERROR, EMAIL_STRING);
    print_router(env, WERROR, " with this error message and the broccoli.dump file.\n\n");
}

/******************************************************
 * error_divide_by_zero: Generalized error message
 *   for when a function attempts to divide by zero.
 *******************************************************/
void error_divide_by_zero(void *env, char *functionName)
{
    error_print_id(env, "ERROR", 7, FALSE);
    print_router(env, WERROR, "Divide by zero in ");
    print_router(env, WERROR, functionName);
    print_router(env, WERROR, " function.\n");
}

/******************************************************
 * core_convert_float_to_string: Converts number to KB string format.
 *******************************************************/
char *core_convert_float_to_string(void *env, double number)
{
    char floatString[40];
    int i;
    char x;
    void *thePtr;

    sysdep_sprintf(floatString, "%.15g", number);

    for( i = 0; (x = floatString[i]) != '\0'; i++ )
    {
        if((x == '.') || (x == 'e'))
        {
            thePtr = store_atom(env, floatString);
            return(to_string(thePtr));
        }
    }

    sysdep_strcat(floatString, ".0");

    thePtr = store_atom(env, floatString);
    return(to_string(thePtr));
}

/******************************************************************
 * core_conver_long_to_string: Converts long integer to KB string format.
 *******************************************************************/
char *core_conver_long_to_string(void *env, long long number)
{
    char buffer[50];
    void *thePtr;

    sysdep_sprintf(buffer, "%lld", number);

    thePtr = store_atom(env, buffer);
    return(to_string(thePtr));
}

/******************************************************************
 * core_convert_data_object_to_string: Converts a core_data_object to KB string format.
 *******************************************************************/
char *core_convert_data_object_to_string(void *env, core_data_object *theDO)
{
    void *thePtr;
    char *theString, *newString;
    char *prefix, *postfix;
    size_t length;
    struct external_address_hash_node *theAddress;
    char buffer[30];

    switch( core_get_pointer_type(theDO))
    {
    case LIST:
        prefix = "(";
        theString = to_string(implode_list(env, theDO));
        postfix = ")";
        break;

    case STRING:
        prefix = "\"";
        theString = core_convert_data_ptr_to_string(theDO);
        postfix = "\"";
        break;

    case INSTANCE_NAME:
        prefix = "[";
        theString = core_convert_data_ptr_to_string(theDO);
        postfix = "]";
        break;

    case ATOM:
        return(core_convert_data_ptr_to_string(theDO));

    case FLOAT:
        return(core_convert_float_to_string(env, core_convert_data_ptr_to_double(theDO)));

    case INTEGER:
        return(core_conver_long_to_string(env, core_convert_data_ptr_to_long(theDO)));

    case RVOID:
        return("");

#if OBJECT_SYSTEM
    case INSTANCE_ADDRESS:
        thePtr = core_convert_data_ptr_to_pointer(theDO);

        if( thePtr == (void *)&InstanceData(env)->DummyInstance )
        {
            return("<Dummy Instance>");
        }

        if(((struct instance *)thePtr)->garbage )
        {
            prefix = "<Stale Instance-";
            representation = to_string(((struct instance *)thePtr)->name);
            postfix = ">";
        }
        else
        {
            prefix = "<Instance-";
            representation = to_string(GetFullInstanceName(env, (INSTANCE_TYPE *)thePtr));
            postfix = ">";
        }

        break;
#endif

    case EXTERNAL_ADDRESS:
        theAddress = (struct external_address_hash_node *)core_convert_data_ptr_to_pointer(theDO);
        /* TBD Need specific routine for creating name string. */
        sysdep_sprintf(buffer, "<Pointer-%d-%p>", (int)theAddress->type, core_convert_data_ptr_to_ext_address(theDO));
        thePtr = store_atom(env, buffer);
        return(to_string(thePtr));


    default:
        return("UNK");
    }

    length = strlen(prefix) + strlen(theString) + strlen(postfix) + 1;
    newString = (char *)core_mem_alloc(env, length);
    newString[0] = '\0';
    sysdep_strcat(newString, prefix);
    sysdep_strcat(newString, theString);
    sysdep_strcat(newString, postfix);
    thePtr = store_atom(env, newString);
    core_mem_free(env, newString, length);
    return(to_string(thePtr));
}

/**************************************************
 * error_method_duplication: Prints out an appropriate error
 *   message when a slot cannot be found for a
 *   function. Input to the function is the slot
 *   name and the function name.
 ***************************************************/
void error_method_duplication(void *env, char *sname, char *func)
{
    error_print_id(env, "FUNCTIONS", 3, FALSE);
    print_router(env, WERROR, "No such slot ");
    print_router(env, WERROR, sname);
    print_router(env, WERROR, " in function ");
    print_router(env, WERROR, func);
    print_router(env, WERROR, ".\n");
    core_set_eval_error(env, TRUE);
}

/************************************************************
 * core_numerical_compare: Given two numbers (which can be integers,
 *   floats, or the symbols for positive/negative infinity)
 *   returns the relationship between the numbers (greater
 *   than, less than or equal).
 *************************************************************/
int core_numerical_compare(void *env, int type1, void *vptr1, int type2, void *vptr2)
{
    /*============================================
     * Handle the situation in which the values
     * are exactly equal (same type, same value).
     *============================================*/

    if( vptr1 == vptr2 )
    {
        return(EQUAL);
    }

    /*=======================================
     * Handle the special cases for positive
     * and negative infinity.
     *=======================================*/

    if( vptr1 == get_atom_data(env)->positive_inf_atom )
    {
        return(GREATER_THAN);
    }

    if( vptr1 == get_atom_data(env)->negative_inf_atom )
    {
        return(LESS_THAN);
    }

    if( vptr2 == get_atom_data(env)->positive_inf_atom )
    {
        return(LESS_THAN);
    }

    if( vptr2 == get_atom_data(env)->negative_inf_atom )
    {
        return(GREATER_THAN);
    }

    /*=======================
     * Compare two integers.
     *=======================*/

    if((type1 == INTEGER) && (type2 == INTEGER))
    {
        if( to_long(vptr1) < to_long(vptr2))
        {
            return(LESS_THAN);
        }
        else if( to_long(vptr1) > to_long(vptr2))
        {
            return(GREATER_THAN);
        }

        return(EQUAL);
    }

    /*=====================
     * Compare two floats.
     *=====================*/

    if((type1 == FLOAT) && (type2 == FLOAT))
    {
        if( to_double(vptr1) < to_double(vptr2))
        {
            return(LESS_THAN);
        }
        else if( to_double(vptr1) > to_double(vptr2))
        {
            return(GREATER_THAN);
        }

        return(EQUAL);
    }

    /*================================
     * Compare an integer to a float.
     *================================*/

    if((type1 == INTEGER) && (type2 == FLOAT))
    {
        if(((double)to_long(vptr1)) < to_double(vptr2))
        {
            return(LESS_THAN);
        }
        else if(((double)to_long(vptr1)) > to_double(vptr2))
        {
            return(GREATER_THAN);
        }

        return(EQUAL);
    }

    /*================================
     * Compare a float to an integer.
     *================================*/

    if((type1 == FLOAT) && (type2 == INTEGER))
    {
        if( to_double(vptr1) < ((double)to_long(vptr2)))
        {
            return(LESS_THAN);
        }
        else if( to_double(vptr1) > ((double)to_long(vptr2)))
        {
            return(GREATER_THAN);
        }

        return(EQUAL);
    }

    /*===================================
     * One of the arguments was invalid.
     * Return -1 to indicate an error.
     *===================================*/

    return(-1);
}
