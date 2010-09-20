/* Purpose: Provides functions for parsing constraint
 *   declarations.                                           */

#define __PARSER_CONSTRAINTS_SOURCE__

#include <stdio.h>
#define _STDIO_INCLUDED_
#include <stdlib.h>

#include "setup.h"

#include "constant.h"
#include "core_environment.h"
#include "core_memory.h"
#include "router.h"
#include "core_scanner.h"
#include "constraints_operators.h"
#include "constraints_query.h"
#include "sysdep.h"

#include "parser_constraints.h"

/**************************************
 * LOCAL FUNCTION PROTOTYPES
 ***************************************/
#if OFF
static BOOLEAN ParseRangeCardinalityAttribute(void *, char *, CONSTRAINT_META *, CONSTRAINT_PARSE_RECORD *, char *, int);
static BOOLEAN ParseTypeAttribute(void *, char *, CONSTRAINT_META *);
static void    AddToRestrictionList(void *, int, CONSTRAINT_META *, CONSTRAINT_META *);
static BOOLEAN ParseAllowedValuesAttribute(void *, char *, char *, CONSTRAINT_META *, CONSTRAINT_PARSE_RECORD *);
static int     GetConstraintTypeFromAllowedName(char *);
static int     GetConstraintTypeFromTypeName(char *);
static int     GetAttributeParseValue(char *, CONSTRAINT_PARSE_RECORD *);
static void    SetRestrictionFlag(int, CONSTRAINT_META *, int);
static void    SetParseFlag(CONSTRAINT_PARSE_RECORD *, char *);
static void    NoConjunctiveUseError(void *, char *, char *);


/*******************************************************************
 * CheckConstraintParseConflicts: Determines if a constraint record
 *   has any conflicts in the attribute specifications. Returns
 *   TRUE if no conflicts were detected, otherwise FALSE.
 ********************************************************************/
BOOLEAN CheckConstraintParseConflicts(void *env, CONSTRAINT_META *constraints)
{
    /*===================================================
     * Check to see if any of the allowed-... attributes
     * conflict with the type attribute.
     *===================================================*/

    if( constraints->allow_any == TRUE )
    {     /* Do Nothing */
    }
    else if( constraints->restrict_atom &&
             (constraints->allow_atom == FALSE))
    {
        AttributeConflictErrorMessage(env, "type", "allowed-symbols");
        return(FALSE);
    }
    else if( constraints->restrict_string &&
             (constraints->allow_string == FALSE))
    {
        AttributeConflictErrorMessage(env, "type", "allowed-strings");
        return(FALSE);
    }
    else if( constraints->restrict_integer &&
             (constraints->allow_integer == FALSE))
    {
        AttributeConflictErrorMessage(env, "type", "allowed-integers/numbers");
        return(FALSE);
    }
    else if( constraints->restrict_float &&
             (constraints->allow_float == FALSE))
    {
        AttributeConflictErrorMessage(env, "type", "allowed-floats/numbers");
        return(FALSE);
    }
    else if( constraints->restrict_class &&
             (constraints->allow_instance_pointer == FALSE) &&
             (constraints->allow_instance_name == FALSE))
    {
        AttributeConflictErrorMessage(env, "type", "allowed-classes");
        return(FALSE);
    }
    else if( constraints->restrict_instance_name &&
             (constraints->allow_instance_name == FALSE))
    {
        AttributeConflictErrorMessage(env, "type", "allowed-instance-names");
        return(FALSE);
    }
    else if( constraints->restrict_any )
    {
        struct core_expression *theExp;

        for( theExp = constraints->restriction_list;
             theExp != NULL;
             theExp = theExp->next_arg )
        {
            if( garner_value_violations(env, theExp->type, theExp->value, constraints) != NO_VIOLATION )
            {
                AttributeConflictErrorMessage(env, "type", "allowed-values");
                return(FALSE);
            }
        }
    }

    /*================================================================
     * Check to see if range attribute conflicts with type attribute.
     *================================================================*/

    if((constraints->max_value != NULL) &&
       (constraints->allow_any == FALSE))
    {
        if(((constraints->max_value->type == INTEGER) &&
            (constraints->allow_integer == FALSE)) ||
           ((constraints->max_value->type == FLOAT) &&
            (constraints->allow_float == FALSE)))
        {
            AttributeConflictErrorMessage(env, "type", "range");
            return(FALSE);
        }
    }

    if((constraints->min_value != NULL) &&
       (constraints->allow_any == FALSE))
    {
        if(((constraints->min_value->type == INTEGER) &&
            (constraints->allow_integer == FALSE)) ||
           ((constraints->min_value->type == FLOAT) &&
            (constraints->allow_float == FALSE)))
        {
            AttributeConflictErrorMessage(env, "type", "range");
            return(FALSE);
        }
    }

    /*=========================================
     * Check to see if allowed-class attribute
     * conflicts with type attribute.
     *=========================================*/

    if((constraints->class_list != NULL) &&
       (constraints->allow_any == FALSE) &&
       (constraints->allow_instance_name == FALSE) &&
       (constraints->allow_instance_pointer == FALSE))
    {
        AttributeConflictErrorMessage(env, "type", "allowed-class");
        return(FALSE);
    }

    /*=====================================================
     * Return TRUE to indicate no conflicts were detected.
     *=====================================================*/

    return(TRUE);
}

/*******************************************************
 * AttributeConflictErrorMessage: Generic error message
 *   for a constraint attribute conflict.
 ********************************************************/
void AttributeConflictErrorMessage(void *env, char *attribute1, char *attribute2)
{
    error_print_id(env, "PARSER", 1, TRUE);
    print_router(env, WERROR, "The ");
    print_router(env, WERROR, attribute1);
    print_router(env, WERROR, " attribute conflicts with the ");
    print_router(env, WERROR, attribute2);
    print_router(env, WERROR, " attribute.\n");
}

/**************************************************************************
 * InitializeConstraintParseRecord: Initializes the values of a constraint
 *   parse record which is used to determine whether one of the standard
 *   constraint specifications has already been parsed.
 ***************************************************************************/
void InitializeConstraintParseRecord(CONSTRAINT_PARSE_RECORD *parsedConstraints)
{
    parsedConstraints->type = FALSE;
    parsedConstraints->range = FALSE;
    parsedConstraints->allowedSymbols = FALSE;
    parsedConstraints->allowedStrings = FALSE;
    parsedConstraints->allowedLexemes = FALSE;
    parsedConstraints->allowedIntegers = FALSE;
    parsedConstraints->allowedFloats = FALSE;
    parsedConstraints->allowedNumbers = FALSE;
    parsedConstraints->allowedValues = FALSE;
    parsedConstraints->allowedInstanceNames = FALSE;
    parsedConstraints->allowedClasses = FALSE;
    parsedConstraints->cardinality = FALSE;
}

/***********************************************************************
 * StandardConstraint: Returns TRUE if the specified name is one of the
 *   standard constraints parseable by the routines in this module.
 ************************************************************************/
BOOLEAN StandardConstraint(char *constraintName)
{
    if((strcmp(constraintName, "type") == 0) ||
       (strcmp(constraintName, "range") == 0) ||
       (strcmp(constraintName, "cardinality") == 0) ||
       (strcmp(constraintName, "allowed-symbols") == 0) ||
       (strcmp(constraintName, "allowed-strings") == 0) ||
       (strcmp(constraintName, "allowed-lexemes") == 0) ||
       (strcmp(constraintName, "allowed-integers") == 0) ||
       (strcmp(constraintName, "allowed-floats") == 0) ||
       (strcmp(constraintName, "allowed-numbers") == 0) ||
       (strcmp(constraintName, "allowed-instance-names") == 0) ||
       (strcmp(constraintName, "allowed-classes") == 0) ||
       (strcmp(constraintName, "allowed-values") == 0))

    {
        return(TRUE);
    }

    return(FALSE);
}

/**********************************************************************
 * ParseStandardConstraint: Parses a standard constraint. Returns TRUE
 *   if the constraint was successfully parsed, otherwise FALSE.
 ***********************************************************************/
BOOLEAN ParseStandardConstraint(void *env, char *readSource, char *constraintName, CONSTRAINT_META *constraints, CONSTRAINT_PARSE_RECORD *parsedConstraints, int multipleValuesAllowed)
{
    int rv = FALSE;

    /*=====================================================
     * Determine if the attribute has already been parsed.
     *=====================================================*/

    if( GetAttributeParseValue(constraintName, parsedConstraints))
    {
        error_reparse(env, constraintName, " attribute");
        return(FALSE);
    }

    /*==========================================
     * If specified, parse the range attribute.
     *==========================================*/

    if( strcmp(constraintName, "range") == 0 )
    {
        rv = ParseRangeCardinalityAttribute(env, readSource, constraints, parsedConstraints,
                                            constraintName, multipleValuesAllowed);
    }

    /*================================================
     * If specified, parse the cardinality attribute.
     *================================================*/

    else if( strcmp(constraintName, "cardinality") == 0 )
    {
        rv = ParseRangeCardinalityAttribute(env, readSource, constraints, parsedConstraints,
                                            constraintName, multipleValuesAllowed);
    }

    /*=========================================
     * If specified, parse the type attribute.
     *=========================================*/

    else if( strcmp(constraintName, "type") == 0 )
    {
        rv = ParseTypeAttribute(env, readSource, constraints);
    }

    /*================================================
     * If specified, parse the allowed-... attribute.
     *================================================*/

    else if((strcmp(constraintName, "allowed-symbols") == 0) ||
            (strcmp(constraintName, "allowed-strings") == 0) ||
            (strcmp(constraintName, "allowed-lexemes") == 0) ||
            (strcmp(constraintName, "allowed-integers") == 0) ||
            (strcmp(constraintName, "allowed-floats") == 0) ||
            (strcmp(constraintName, "allowed-numbers") == 0) ||
            (strcmp(constraintName, "allowed-instance-names") == 0) ||
            (strcmp(constraintName, "allowed-classes") == 0) ||
            (strcmp(constraintName, "allowed-values") == 0))
    {
        rv = ParseAllowedValuesAttribute(env, readSource, constraintName,
                                         constraints, parsedConstraints);
    }

    /*=========================================
     * Remember which constraint attribute was
     * parsed and return the error status.
     *=========================================*/

    SetParseFlag(parsedConstraints, constraintName);
    return(rv);
}

/**********************************************************
 * OverlayConstraint: Overlays fields of source constraint
 * record on destination based on which fields are set in
 * the parsed constraint record. Assumes addConstraint has
 * not yet been called for the destination constraint
 * record.
 ***********************************************************/
void OverlayConstraint(void *env, CONSTRAINT_PARSE_RECORD *pc, CONSTRAINT_META *cdst, CONSTRAINT_META *csrc)
{
    if( pc->type == 0 )
    {
        cdst->allow_any = csrc->allow_any;
        cdst->allow_atom = csrc->allow_atom;
        cdst->allow_string = csrc->allow_string;
        cdst->allow_float = csrc->allow_float;
        cdst->allow_integer = csrc->allow_integer;
        cdst->allow_instance_name = csrc->allow_instance_name;
        cdst->allow_instance_pointer = csrc->allow_instance_pointer;
        cdst->allow_external_pointer = csrc->allow_external_pointer;
        cdst->allow_void = csrc->allow_void;
    }

    if( pc->range == 0 )
    {
        core_return_expression(env, cdst->min_value);
        core_return_expression(env, cdst->max_value);
        cdst->min_value = core_copy_expression(env, csrc->min_value);
        cdst->max_value = core_copy_expression(env, csrc->max_value);
    }

    if( pc->allowedClasses == 0 )
    {
        core_return_expression(env, cdst->class_list);
        cdst->class_list = core_copy_expression(env, csrc->class_list);
    }

    if( pc->allowedValues == 0 )
    {
        if((pc->allowedSymbols == 0) &&
           (pc->allowedStrings == 0) &&
           (pc->allowedLexemes == 0) &&
           (pc->allowedIntegers == 0) &&
           (pc->allowedFloats == 0) &&
           (pc->allowedNumbers == 0) &&
           (pc->allowedInstanceNames == 0))
        {
            cdst->restrict_any = csrc->restrict_any;
            cdst->restrict_atom = csrc->restrict_atom;
            cdst->restrict_string = csrc->restrict_string;
            cdst->restrict_float = csrc->restrict_float;
            cdst->restrict_integer = csrc->restrict_integer;
            cdst->restrict_class = csrc->restrict_class;
            cdst->restrict_instance_name = csrc->restrict_instance_name;
            cdst->restriction_list = core_copy_expression(env, csrc->restriction_list);
        }
        else
        {
            if((pc->allowedSymbols == 0) && csrc->restrict_atom )
            {
                cdst->restrict_atom = 1;
                AddToRestrictionList(env, ATOM, cdst, csrc);
            }

            if((pc->allowedStrings == 0) && csrc->restrict_string )
            {
                cdst->restrict_string = 1;
                AddToRestrictionList(env, STRING, cdst, csrc);
            }

            if((pc->allowedLexemes == 0) && csrc->restrict_atom && csrc->restrict_string )
            {
                cdst->restrict_atom = 1;
                cdst->restrict_string = 1;
                AddToRestrictionList(env, ATOM, cdst, csrc);
                AddToRestrictionList(env, STRING, cdst, csrc);
            }

            if((pc->allowedIntegers == 0) && csrc->restrict_integer )
            {
                cdst->restrict_integer = 1;
                AddToRestrictionList(env, INTEGER, cdst, csrc);
            }

            if((pc->allowedFloats == 0) && csrc->restrict_float )
            {
                cdst->restrict_float = 1;
                AddToRestrictionList(env, FLOAT, cdst, csrc);
            }

            if((pc->allowedNumbers == 0) && csrc->restrict_integer && csrc->restrict_float )
            {
                cdst->restrict_integer = 1;
                cdst->restrict_float = 1;
                AddToRestrictionList(env, INTEGER, cdst, csrc);
                AddToRestrictionList(env, FLOAT, cdst, csrc);
            }

            if((pc->allowedInstanceNames == 0) && csrc->restrict_instance_name )
            {
                cdst->restrict_instance_name = 1;
                AddToRestrictionList(env, INSTANCE_NAME, cdst, csrc);
            }
        }
    }

    if( pc->cardinality == 0 )
    {
        core_return_expression(env, cdst->min_fields);
        core_return_expression(env, cdst->max_fields);
        cdst->min_fields = core_copy_expression(env, csrc->min_fields);
        cdst->max_fields = core_copy_expression(env, csrc->max_fields);
    }
}

/*********************************************
 * OverlayConstraintParseRecord: Performs a
 *   field-wise "or" of the destination parse
 *   record with the source parse record.
 **********************************************/
void OverlayConstraintParseRecord(CONSTRAINT_PARSE_RECORD *dst, CONSTRAINT_PARSE_RECORD *src)
{
    if( src->type )
    {
        dst->type = TRUE;
    }

    if( src->range )
    {
        dst->range = TRUE;
    }

    if( src->allowedSymbols )
    {
        dst->allowedSymbols = TRUE;
    }

    if( src->allowedStrings )
    {
        dst->allowedStrings = TRUE;
    }

    if( src->allowedLexemes )
    {
        dst->allowedLexemes = TRUE;
    }

    if( src->allowedIntegers )
    {
        dst->allowedIntegers = TRUE;
    }

    if( src->allowedFloats )
    {
        dst->allowedFloats = TRUE;
    }

    if( src->allowedNumbers )
    {
        dst->allowedNumbers = TRUE;
    }

    if( src->allowedValues )
    {
        dst->allowedValues = TRUE;
    }

    if( src->allowedInstanceNames )
    {
        dst->allowedInstanceNames = TRUE;
    }

    if( src->allowedClasses )
    {
        dst->allowedClasses = TRUE;
    }

    if( src->cardinality )
    {
        dst->cardinality = TRUE;
    }
}

/***********************************************************
 * AddToRestrictionList: Prepends atoms of the specified
 * type from the source restriction list to the destination
 ************************************************************/
static void AddToRestrictionList(void *env, int type, CONSTRAINT_META *cdst, CONSTRAINT_META *csrc)
{
    struct core_expression *theExp, *tmp;

    for( theExp = csrc->restriction_list; theExp != NULL; theExp = theExp->next_arg )
    {
        if( theExp->type == type )
        {
            tmp = core_generate_constant(env, theExp->type, theExp->value);
            tmp->next_arg = cdst->restriction_list;
            cdst->restriction_list = tmp;
        }
    }
}

/******************************************************************
 * ParseAllowedValuesAttribute: Parses the allowed-... attributes.
 *******************************************************************/
static BOOLEAN ParseAllowedValuesAttribute(void *env, char *readSource, char *constraintName, CONSTRAINT_META *constraints, CONSTRAINT_PARSE_RECORD *parsedConstraints)
{
    struct token inputToken;
    int expectedType, restrictionType, error = FALSE;
    struct core_expression *newValue, *lastValue;
    int constantParsed = FALSE, variableParsed = FALSE;
    char *tempPtr = NULL;

    /*======================================================
     * The allowed-values attribute is not allowed if other
     * allowed-... attributes have already been parsed.
     *======================================================*/

    if((strcmp(constraintName, "allowed-values") == 0) &&
       ((parsedConstraints->allowedSymbols) ||
        (parsedConstraints->allowedStrings) ||
        (parsedConstraints->allowedLexemes) ||
        (parsedConstraints->allowedIntegers) ||
        (parsedConstraints->allowedFloats) ||
        (parsedConstraints->allowedNumbers) ||
        (parsedConstraints->allowedInstanceNames)))
    {
        if( parsedConstraints->allowedSymbols )
        {
            tempPtr = "allowed-symbols";
        }
        else if( parsedConstraints->allowedStrings )
        {
            tempPtr = "allowed-strings";
        }
        else if( parsedConstraints->allowedLexemes )
        {
            tempPtr = "allowed-lexemes";
        }
        else if( parsedConstraints->allowedIntegers )
        {
            tempPtr = "allowed-integers";
        }
        else if( parsedConstraints->allowedFloats )
        {
            tempPtr = "allowed-floats";
        }
        else if( parsedConstraints->allowedNumbers )
        {
            tempPtr = "allowed-numbers";
        }
        else if( parsedConstraints->allowedInstanceNames )
        {
            tempPtr = "allowed-instance-names";
        }

        NoConjunctiveUseError(env, "allowed-values", tempPtr);
        return(FALSE);
    }

    /*=======================================================
     * The allowed-values/numbers/integers/floats attributes
     * are not allowed with the range attribute.
     *=======================================================*/

    if(((strcmp(constraintName, "allowed-values") == 0) ||
        (strcmp(constraintName, "allowed-numbers") == 0) ||
        (strcmp(constraintName, "allowed-integers") == 0) ||
        (strcmp(constraintName, "allowed-floats") == 0)) &&
       (parsedConstraints->range))
    {
        NoConjunctiveUseError(env, constraintName, "range");
        return(FALSE);
    }

    /*===================================================
     * The allowed-... attributes are not allowed if the
     * allowed-values attribute has already been parsed.
     *===================================================*/

    if((strcmp(constraintName, "allowed-values") != 0) &&
       (parsedConstraints->allowedValues))
    {
        NoConjunctiveUseError(env, constraintName, "allowed-values");
        return(FALSE);
    }

    /*==================================================
     * The allowed-numbers attribute is not allowed if
     * the allowed-integers or allowed-floats attribute
     * has already been parsed.
     *==================================================*/

    if((strcmp(constraintName, "allowed-numbers") == 0) &&
       ((parsedConstraints->allowedFloats) || (parsedConstraints->allowedIntegers)))
    {
        if( parsedConstraints->allowedFloats )
        {
            tempPtr = "allowed-floats";
        }
        else
        {
            tempPtr = "allowed-integers";
        }

        NoConjunctiveUseError(env, "allowed-numbers", tempPtr);
        return(FALSE);
    }

    /*============================================================
     * The allowed-integers/floats attributes are not allowed if
     * the allowed-numbers attribute has already been parsed.
     *============================================================*/

    if(((strcmp(constraintName, "allowed-integers") == 0) ||
        (strcmp(constraintName, "allowed-floats") == 0)) &&
       (parsedConstraints->allowedNumbers))
    {
        NoConjunctiveUseError(env, constraintName, "allowed-number");
        return(FALSE);
    }

    /*==================================================
     * The allowed-lexemes attribute is not allowed if
     * the allowed-symbols or allowed-strings attribute
     * has already been parsed.
     *==================================================*/

    if((strcmp(constraintName, "allowed-lexemes") == 0) &&
       ((parsedConstraints->allowedSymbols) || (parsedConstraints->allowedStrings)))
    {
        if( parsedConstraints->allowedSymbols )
        {
            tempPtr = "allowed-symbols";
        }
        else
        {
            tempPtr = "allowed-strings";
        }

        NoConjunctiveUseError(env, "allowed-lexemes", tempPtr);
        return(FALSE);
    }

    /*===========================================================
     * The allowed-symbols/strings attributes are not allowed if
     * the allowed-lexemes attribute has already been parsed.
     *===========================================================*/

    if(((strcmp(constraintName, "allowed-symbols") == 0) ||
        (strcmp(constraintName, "allowed-strings") == 0)) &&
       (parsedConstraints->allowedLexemes))
    {
        NoConjunctiveUseError(env, constraintName, "allowed-lexemes");
        return(FALSE);
    }

    /*========================
     * Get the expected type.
     *========================*/

    restrictionType = GetConstraintTypeFromAllowedName(constraintName);
    SetRestrictionFlag(restrictionType, constraints, TRUE);

    if( strcmp(constraintName, "allowed-classes") == 0 )
    {
        expectedType = ATOM;
    }
    else
    {
        expectedType = restrictionType;
    }

    /*=================================================
     * Get the last value in the restriction list (the
     * allowed values will be appended there).
     *=================================================*/

    if( strcmp(constraintName, "allowed-classes") == 0 )
    {
        lastValue = constraints->class_list;
    }
    else
    {
        lastValue = constraints->restriction_list;
    }

    if( lastValue != NULL )
    {
        while( lastValue->next_arg != NULL )
        {
            lastValue = lastValue->next_arg;
        }
    }

    /*==================================================
     * Read the allowed values and add them to the list
     * until a right parenthesis is encountered.
     *==================================================*/

    core_save_pp_buffer(env, " ");
    core_get_token(env, readSource, &inputToken);

    while( inputToken.type != RPAREN )
    {
        core_save_pp_buffer(env, " ");

        /*=============================================
         * Determine the type of the token just parsed
         * and if it is an appropriate value.
         *=============================================*/

        switch( inputToken.type )
        {
        case INTEGER:

            if((expectedType != UNKNOWN_VALUE) &&
               (expectedType != INTEGER) &&
               (expectedType != INTEGER_OR_FLOAT))
            {
                error = TRUE;
            }

            constantParsed = TRUE;
            break;

        case FLOAT:

            if((expectedType != UNKNOWN_VALUE) &&
               (expectedType != FLOAT) &&
               (expectedType != INTEGER_OR_FLOAT))
            {
                error = TRUE;
            }

            constantParsed = TRUE;
            break;

        case STRING:

            if((expectedType != UNKNOWN_VALUE) &&
               (expectedType != STRING) &&
               (expectedType != ATOM_OR_STRING))
            {
                error = TRUE;
            }

            constantParsed = TRUE;
            break;

        case ATOM:

            if((expectedType != UNKNOWN_VALUE) &&
               (expectedType != ATOM) &&
               (expectedType != ATOM_OR_STRING))
            {
                error = TRUE;
            }

            constantParsed = TRUE;
            break;

#if OBJECT_SYSTEM
        case INSTANCE_NAME:

            if((expectedType != UNKNOWN_VALUE) &&
               (expectedType != INSTANCE_NAME))
            {
                error = TRUE;
            }

            constantParsed = TRUE;
            break;
#endif

        case SCALAR_VARIABLE:

            if( strcmp(inputToken.pp, "?VARIABLE") == 0 )
            {
                variableParsed = TRUE;
            }
            else
            {
                char tempBuffer[120];
                sysdep_sprintf(tempBuffer, "%s attribute", constraintName);
                error_syntax(env, tempBuffer);
                return(FALSE);
            }

            break;

        default:
        {
            char tempBuffer[120];
            sysdep_sprintf(tempBuffer, "%s attribute", constraintName);
            error_syntax(env, tempBuffer);
        }
            return(FALSE);
        }

        /*=====================================
         * Signal an error if an inappropriate
         * value was found.
         *=====================================*/

        if( error )
        {
            error_print_id(env, "PARSER", 4, TRUE);
            print_router(env, WERROR, "Value does not match the expected type for the ");
            print_router(env, WERROR, constraintName);
            print_router(env, WERROR, " attribute\n");
            return(FALSE);
        }

        /*======================================
         * The ?VARIABLE argument can't be used
         * in conjunction with constants.
         *======================================*/

        if( constantParsed && variableParsed )
        {
            char tempBuffer[120];
            sysdep_sprintf(tempBuffer, "%s attribute", constraintName);
            error_syntax(env, tempBuffer);
            return(FALSE);
        }

        /*===========================================
         * Add the constant to the restriction list.
         *===========================================*/

        newValue = core_generate_constant(env, inputToken.type, inputToken.value);

        if( lastValue == NULL )
        {
            if( strcmp(constraintName, "allowed-classes") == 0 )
            {
                constraints->class_list = newValue;
            }
            else
            {
                constraints->restriction_list = newValue;
            }
        }
        else
        {
            lastValue->next_arg = newValue;
        }

        lastValue = newValue;

        /*=======================================
         * Begin parsing the next allowed value.
         *=======================================*/

        core_get_token(env, readSource, &inputToken);
    }

    /*======================================================
     * There must be at least one value for this attribute.
     *======================================================*/

    if((!constantParsed) && (!variableParsed))
    {
        char tempBuffer[120];
        sysdep_sprintf(tempBuffer, "%s attribute", constraintName);
        error_syntax(env, tempBuffer);
        return(FALSE);
    }

    /*======================================
     * If ?VARIABLE was parsed, then remove
     * the restrictions for the type being
     * restricted.
     *======================================*/

    if( variableParsed )
    {
        switch( restrictionType )
        {
        case UNKNOWN_VALUE:
            constraints->restrict_any = FALSE;
            break;

        case ATOM:
            constraints->restrict_atom = FALSE;
            break;

        case STRING:
            constraints->restrict_string = FALSE;
            break;

        case INTEGER:
            constraints->restrict_integer = FALSE;
            break;

        case FLOAT:
            constraints->restrict_float = FALSE;
            break;

        case INTEGER_OR_FLOAT:
            constraints->restrict_float = FALSE;
            constraints->restrict_integer = FALSE;
            break;

        case ATOM_OR_STRING:
            constraints->restrict_atom = FALSE;
            constraints->restrict_string = FALSE;
            break;

        case INSTANCE_NAME:
            constraints->restrict_instance_name = FALSE;
            break;

        case INSTANCE_OR_INSTANCE_NAME:
            constraints->restrict_class = FALSE;
            break;
        }
    }

    /*=====================================
     * Fix up pretty print representation.
     *=====================================*/

    core_backup_pp_buffer(env);
    core_backup_pp_buffer(env);
    core_save_pp_buffer(env, ")");

    /*=======================================
     * Return TRUE to indicate the attribute
     * was successfully parsed.
     *=======================================*/

    return(TRUE);
}

/**********************************************************
 * NoConjunctiveUseError: Generic error message indicating
 *   that two attributes can't be used in conjunction.
 ***********************************************************/
static void NoConjunctiveUseError(void *env, char *attribute1, char *attribute2)
{
    error_print_id(env, "PARSER", 3, TRUE);
    print_router(env, WERROR, "The ");
    print_router(env, WERROR, attribute1);
    print_router(env, WERROR, " attribute cannot be used\n");
    print_router(env, WERROR, "in conjunction with the ");
    print_router(env, WERROR, attribute2);
    print_router(env, WERROR, " attribute.\n");
}

/*************************************************
 * ParseTypeAttribute: Parses the type attribute.
 **************************************************/
static BOOLEAN ParseTypeAttribute(void *env, char *readSource, CONSTRAINT_META *constraints)
{
    int typeParsed = FALSE;
    int variableParsed = FALSE;
    int theType;
    struct token inputToken;

    /*======================================
     * Continue parsing types until a right
     * parenthesis is encountered.
     *======================================*/

    core_save_pp_buffer(env, " ");

    for( core_get_token(env, readSource, &inputToken);
         inputToken.type != RPAREN;
         core_get_token(env, readSource, &inputToken))
    {
        core_save_pp_buffer(env, " ");

        /*==================================
         * If the token is a symbol then...
         *==================================*/

        if( inputToken.type == ATOM )
        {
            /*==============================================
             * ?VARIABLE can't be used with type constants.
             *==============================================*/

            if( variableParsed == TRUE )
            {
                error_syntax(env, "type attribute");
                return(FALSE);
            }

            /*========================================
             * Check for an appropriate type constant
             * (e.g. ATOM, FLOAT, INTEGER, etc.).
             *========================================*/

            theType = GetConstraintTypeFromTypeName(to_string(inputToken.value));

            if( theType < 0 )
            {
                error_syntax(env, "type attribute");
                return(FALSE);
            }

            /*==================================================
             * Change the type restriction flags to reflect the
             * type restriction. If the type restriction was
             * already specified, then a error is generated.
             *==================================================*/

            if( set_constraint_type(theType, constraints))
            {
                error_syntax(env, "type attribute");
                return(FALSE);
            }

            constraints->allow_any = FALSE;

            /*===========================================
             * Remember that a type constant was parsed.
             *===========================================*/

            typeParsed = TRUE;
        }

        /*==============================================
         * Otherwise if the token is a variable then...
         *==============================================*/

        else if( inputToken.type == SCALAR_VARIABLE )
        {
            /*========================================
             * The only variable allowd is ?VARIABLE.
             *========================================*/

            if( strcmp(inputToken.pp, "?VARIABLE") != 0 )
            {
                error_syntax(env, "type attribute");
                return(FALSE);
            }

            /*===================================
             * ?VARIABLE can't be used more than
             * once or with type constants.
             *===================================*/

            if( typeParsed || variableParsed )
            {
                error_syntax(env, "type attribute");
                return(FALSE);
            }

            /*======================================
             * Remember that a variable was parsed.
             *======================================*/

            variableParsed = TRUE;
        }

        /*====================================
         * Otherwise this is an invalid value
         * for the type attribute.
         *====================================*/

        else
        {
            error_syntax(env, "type attribute");
            return(FALSE);
        }
    }

    /*=====================================
     * Fix up pretty print representation.
     *=====================================*/

    core_backup_pp_buffer(env);
    core_backup_pp_buffer(env);
    core_save_pp_buffer(env, ")");

    /*=======================================
     * The type attribute must have a value.
     *=======================================*/

    if((!typeParsed) && (!variableParsed))
    {
        error_syntax(env, "type attribute");
        return(FALSE);
    }

    /*===========================================
     * Return TRUE indicating the type attibuted
     * was successfully parsed.
     *===========================================*/

    return(TRUE);
}

/**************************************************************************
 * ParseRangeCardinalityAttribute: Parses the range/cardinality attribute.
 ***************************************************************************/
static BOOLEAN ParseRangeCardinalityAttribute(void *env, char *readSource, CONSTRAINT_META *constraints, CONSTRAINT_PARSE_RECORD *parsedConstraints, char *constraintName, int multipleValuesAllowed)
{
    struct token inputToken;
    int range;
    char *tempPtr = NULL;

    /*=================================
     * Determine if we're parsing the
     * range or cardinality attribute.
     *=================================*/

    if( strcmp(constraintName, "range") == 0 )
    {
        parsedConstraints->range = TRUE;
        range = TRUE;
    }
    else
    {
        parsedConstraints->cardinality = TRUE;
        range = FALSE;
    }

    /*===================================================================
     * The cardinality attribute can only be used with list slots.
     *===================================================================*/

    if((range == FALSE) &&
       (multipleValuesAllowed == FALSE))
    {
        error_print_id(env, "PARSER", 5, TRUE);
        print_router(env, WERROR, "The cardinality attribute ");
        print_router(env, WERROR, "can only be used with list slots.\n");
        return(FALSE);
    }

    /*====================================================
     * The range attribute is not allowed with the
     * allowed-values/numbers/integers/floats attributes.
     *====================================================*/

    if((range == TRUE) &&
       (parsedConstraints->allowedValues ||
        parsedConstraints->allowedNumbers ||
        parsedConstraints->allowedIntegers ||
        parsedConstraints->allowedFloats))
    {
        if( parsedConstraints->allowedValues )
        {
            tempPtr = "allowed-values";
        }
        else if( parsedConstraints->allowedIntegers )
        {
            tempPtr = "allowed-integers";
        }
        else if( parsedConstraints->allowedFloats )
        {
            tempPtr = "allowed-floats";
        }
        else if( parsedConstraints->allowedNumbers )
        {
            tempPtr = "allowed-numbers";
        }

        NoConjunctiveUseError(env, "range", tempPtr);
        return(FALSE);
    }

    /*==========================
     * Parse the minimum value.
     *==========================*/

    core_save_pp_buffer(env, " ");
    core_get_token(env, readSource, &inputToken);

    if((inputToken.type == INTEGER) || ((inputToken.type == FLOAT) && range))
    {
        if( range )
        {
            core_return_expression(env, constraints->min_value);
            constraints->min_value = core_generate_constant(env, inputToken.type, inputToken.value);
        }
        else
        {
            core_return_expression(env, constraints->min_fields);
            constraints->min_fields = core_generate_constant(env, inputToken.type, inputToken.value);
        }
    }
    else if((inputToken.type == SCALAR_VARIABLE) && (strcmp(inputToken.pp, "?VARIABLE") == 0))
    {     /* Do nothing. */
    }
    else
    {
        char tempBuffer[120];
        sysdep_sprintf(tempBuffer, "%s attribute", constraintName);
        error_syntax(env, tempBuffer);
        return(FALSE);
    }

    /*==========================
     * Parse the maximum value.
     *==========================*/

    core_save_pp_buffer(env, " ");
    core_get_token(env, readSource, &inputToken);

    if((inputToken.type == INTEGER) || ((inputToken.type == FLOAT) && range))
    {
        if( range )
        {
            core_return_expression(env, constraints->max_value);
            constraints->max_value = core_generate_constant(env, inputToken.type, inputToken.value);
        }
        else
        {
            core_return_expression(env, constraints->max_fields);
            constraints->max_fields = core_generate_constant(env, inputToken.type, inputToken.value);
        }
    }
    else if((inputToken.type == SCALAR_VARIABLE) && (strcmp(inputToken.pp, "?VARIABLE") == 0))
    {     /* Do nothing. */
    }
    else
    {
        char tempBuffer[120];
        sysdep_sprintf(tempBuffer, "%s attribute", constraintName);
        error_syntax(env, tempBuffer);
        return(FALSE);
    }

    /*================================
     * Parse the closing parenthesis.
     *================================*/

    core_get_token(env, readSource, &inputToken);

    if( inputToken.type != RPAREN )
    {
        error_syntax(env, "range attribute");
        return(FALSE);
    }

    /*====================================================
     * Minimum value must be less than the maximum value.
     *====================================================*/

    if( range )
    {
        if( core_numerical_compare(env, constraints->min_value->type,
                                   constraints->min_value->value,
                                   constraints->max_value->type,
                                   constraints->max_value->value) == GREATER_THAN )
        {
            error_print_id(env, "PARSER", 2, TRUE);
            print_router(env, WERROR, "Minimum range value must be less than\n");
            print_router(env, WERROR, "or equal to the maximum range value\n");
            return(FALSE);
        }
    }
    else
    {
        if( core_numerical_compare(env, constraints->min_fields->type,
                                   constraints->min_fields->value,
                                   constraints->max_fields->type,
                                   constraints->max_fields->value) == GREATER_THAN )
        {
            error_print_id(env, "PARSER", 2, TRUE);
            print_router(env, WERROR, "Minimum cardinality value must be less than\n");
            print_router(env, WERROR, "or equal to the maximum cardinality value\n");
            return(FALSE);
        }
    }

    /*====================================
     * Return TRUE to indicate that the
     * attribute was successfully parsed.
     *====================================*/

    return(TRUE);
}

/*****************************************************************
 * GetConstraintTypeFromAllowedName: Returns the type restriction
 *   associated with an allowed-... attribute.
 ******************************************************************/
static int GetConstraintTypeFromAllowedName(char *constraintName)
{
    if( strcmp(constraintName, "allowed-values") == 0 )
    {
        return(UNKNOWN_VALUE);
    }
    else if( strcmp(constraintName, "allowed-symbols") == 0 )
    {
        return(ATOM);
    }
    else if( strcmp(constraintName, "allowed-strings") == 0 )
    {
        return(STRING);
    }
    else if( strcmp(constraintName, "allowed-lexemes") == 0 )
    {
        return(ATOM_OR_STRING);
    }
    else if( strcmp(constraintName, "allowed-integers") == 0 )
    {
        return(INTEGER);
    }
    else if( strcmp(constraintName, "allowed-numbers") == 0 )
    {
        return(INTEGER_OR_FLOAT);
    }
    else if( strcmp(constraintName, "allowed-instance-names") == 0 )
    {
        return(INSTANCE_NAME);
    }
    else if( strcmp(constraintName, "allowed-classes") == 0 )
    {
        return(INSTANCE_OR_INSTANCE_NAME);
    }
    else if( strcmp(constraintName, "allowed-floats") == 0 )
    {
        return(FLOAT);
    }

    return(-1);
}

/******************************************************
 * GetConstraintTypeFromTypeName: Converts a type name
 *   to its equivalent integer type restriction.
 *******************************************************/
static int GetConstraintTypeFromTypeName(char *constraintName)
{
    if( strcmp(constraintName, "ATOM") == 0 )
    {
        return(ATOM);
    }
    else if( strcmp(constraintName, "STRING") == 0 )
    {
        return(STRING);
    }
    else if( strcmp(constraintName, "LEXEME") == 0 )
    {
        return(ATOM_OR_STRING);
    }
    else if( strcmp(constraintName, "INTEGER") == 0 )
    {
        return(INTEGER);
    }
    else if( strcmp(constraintName, "FLOAT") == 0 )
    {
        return(FLOAT);
    }
    else if( strcmp(constraintName, "NUMBER") == 0 )
    {
        return(INTEGER_OR_FLOAT);
    }
    else if( strcmp(constraintName, "INSTANCE-NAME") == 0 )
    {
        return(INSTANCE_NAME);
    }
    else if( strcmp(constraintName, "INSTANCE-ADDRESS") == 0 )
    {
        return(INSTANCE_ADDRESS);
    }
    else if( strcmp(constraintName, "INSTANCE") == 0 )
    {
        return(INSTANCE_OR_INSTANCE_NAME);
    }
    else if( strcmp(constraintName, "EXTERNAL-ADDRESS") == 0 )
    {
        return(EXTERNAL_ADDRESS);
    }

    return(-1);
}

/*************************************************************
 * GetAttributeParseValue: Returns a boolean value indicating
 *   whether a specific attribute has already been parsed.
 **************************************************************/
static int GetAttributeParseValue(char *constraintName, CONSTRAINT_PARSE_RECORD *parsedConstraints)
{
    if( strcmp(constraintName, "type") == 0 )
    {
        return(parsedConstraints->type);
    }
    else if( strcmp(constraintName, "range") == 0 )
    {
        return(parsedConstraints->range);
    }
    else if( strcmp(constraintName, "cardinality") == 0 )
    {
        return(parsedConstraints->cardinality);
    }
    else if( strcmp(constraintName, "allowed-values") == 0 )
    {
        return(parsedConstraints->allowedValues);
    }
    else if( strcmp(constraintName, "allowed-symbols") == 0 )
    {
        return(parsedConstraints->allowedSymbols);
    }
    else if( strcmp(constraintName, "allowed-strings") == 0 )
    {
        return(parsedConstraints->allowedStrings);
    }
    else if( strcmp(constraintName, "allowed-lexemes") == 0 )
    {
        return(parsedConstraints->allowedLexemes);
    }
    else if( strcmp(constraintName, "allowed-instance-names") == 0 )
    {
        return(parsedConstraints->allowedInstanceNames);
    }
    else if( strcmp(constraintName, "allowed-classes") == 0 )
    {
        return(parsedConstraints->allowedClasses);
    }
    else if( strcmp(constraintName, "allowed-integers") == 0 )
    {
        return(parsedConstraints->allowedIntegers);
    }
    else if( strcmp(constraintName, "allowed-floats") == 0 )
    {
        return(parsedConstraints->allowedFloats);
    }
    else if( strcmp(constraintName, "allowed-numbers") == 0 )
    {
        return(parsedConstraints->allowedNumbers);
    }

    return(TRUE);
}

/*********************************************************
 * SetRestrictionFlag: Sets the restriction flag of a
 *   constraint record indicating whether a specific
 *   type has an associated allowed-... restriction list.
 **********************************************************/
static void SetRestrictionFlag(int restriction, CONSTRAINT_META *constraints, int value)
{
    switch( restriction )
    {
    case UNKNOWN_VALUE:
        constraints->restrict_any = value;
        break;

    case ATOM:
        constraints->restrict_atom = value;
        break;

    case STRING:
        constraints->restrict_string = value;
        break;

    case INTEGER:
        constraints->restrict_integer = value;
        break;

    case FLOAT:
        constraints->restrict_float = value;
        break;

    case INTEGER_OR_FLOAT:
        constraints->restrict_integer = value;
        constraints->restrict_float = value;
        break;

    case ATOM_OR_STRING:
        constraints->restrict_atom = value;
        constraints->restrict_string = value;
        break;

    case INSTANCE_NAME:
        constraints->restrict_instance_name = value;
        break;

    case INSTANCE_OR_INSTANCE_NAME:
        constraints->restrict_class = value;
        break;
    }
}

/*******************************************************************
 * SetParseFlag: Sets the flag in a parsed constraints data
 *  structure indicating that a specific attribute has been parsed.
 ********************************************************************/
static void SetParseFlag(CONSTRAINT_PARSE_RECORD *parsedConstraints, char *constraintName)
{
    if( strcmp(constraintName, "range") == 0 )
    {
        parsedConstraints->range = TRUE;
    }
    else if( strcmp(constraintName, "type") == 0 )
    {
        parsedConstraints->type = TRUE;
    }
    else if( strcmp(constraintName, "cardinality") == 0 )
    {
        parsedConstraints->cardinality = TRUE;
    }
    else if( strcmp(constraintName, "allowed-symbols") == 0 )
    {
        parsedConstraints->allowedSymbols = TRUE;
    }
    else if( strcmp(constraintName, "allowed-strings") == 0 )
    {
        parsedConstraints->allowedStrings = TRUE;
    }
    else if( strcmp(constraintName, "allowed-lexemes") == 0 )
    {
        parsedConstraints->allowedLexemes = TRUE;
    }
    else if( strcmp(constraintName, "allowed-integers") == 0 )
    {
        parsedConstraints->allowedIntegers = TRUE;
    }
    else if( strcmp(constraintName, "allowed-floats") == 0 )
    {
        parsedConstraints->allowedFloats = TRUE;
    }
    else if( strcmp(constraintName, "allowed-numbers") == 0 )
    {
        parsedConstraints->allowedNumbers = TRUE;
    }
    else if( strcmp(constraintName, "allowed-values") == 0 )
    {
        parsedConstraints->allowedValues = TRUE;
    }
    else if( strcmp(constraintName, "allowed-classes") == 0 )
    {
        parsedConstraints->allowedClasses = TRUE;
    }
}

#endif
