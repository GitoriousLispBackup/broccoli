/* Purpose: Parsing Routines for Defclass Construct           */

/* =========================================
 *****************************************
 *            EXTERNAL DEFINITIONS
 *  =========================================
 ***************************************** */
#include "setup.h"

#if OBJECT_SYSTEM

#include <string.h>

#include "classes_kernel.h"
#include "funcs_class.h"
#include "constraints_query.h"
#include "parser_constraints.h"
#include "constraints_operators.h"
#include "classes_members.h"
#include "core_environment.h"
#include "funcs_instance.h"
#include "core_memory.h"
#include "core_utilities.h"
#include "router.h"
#include "core_scanner.h"

#define __PARSER_CLASS_MEMBERS_SOURCE__
#include "parser_class_members.h"

/* =========================================
 *****************************************
 *                CONSTANTS
 *  =========================================
 ***************************************** */
#define DEFAULT_FACET         "default"
#define DYNAMIC_FACET         "default-dynamic"
#define VARIABLE_VAR          "VARIABLE"

#define STORAGE_FACET         "storage"
#define SLOT_SHARE_RLN        "shared"
#define SLOT_LOCAL_RLN        "local"

#define ACCESS_FACET          "access"
#define SLOT_RDONLY_RLN       "read-only"
#define SLOT_RDWRT_RLN        "read-write"
#define SLOT_INIT_RLN         "initialize-only"

#define PROPAGATION_FACET     "propagation"
#define SLOT_NO_INH_RLN       "no-inherit"
#define SLOT_INH_RLN          "inherit"

#define SOURCE_FACET          "source"
#define SLOT_COMPOSITE_RLN    "composite"
#define SLOT_EXCLUSIVE_RLN    "exclusive"

#define MATCH_FACET           MATCH_RLN
#define SLOT_REACTIVE_RLN     REACTIVE_RLN
#define SLOT_NONREACTIVE_RLN  NONREACTIVE_RLN

#define VISIBILITY_FACET      "visibility"
#define SLOT_PUBLIC_RLN       "public"
#define SLOT_PRIVATE_RLN      "private"

#define CREATE_ACCESSOR_FACET "create-accessor"
#define SLOT_READ_RLN         "read"
#define SLOT_WRITE_RLN        "write"
#define SLOT_NONE_RLN         "NONE"

#define OVERRIDE_MSG_FACET    "override-message"
#define SLOT_DEFAULT_RLN      "DEFAULT"

#define STORAGE_BIT           0
#define FIELD_BIT             1
#define ACCESS_BIT            2
#define PROPAGATION_BIT       3
#define SOURCE_BIT            4
#define MATCH_BIT             5
#define DEFAULT_BIT           6
#define DEFAULT_DYNAMIC_BIT   7
#define VISIBILITY_BIT        8
#define CREATE_ACCESSOR_BIT   9
#define OVERRIDE_MSG_BIT      10

/* =========================================
 *****************************************
 *   INTERNALLY VISIBLE FUNCTION HEADERS
 *  =========================================
 ***************************************** */

static SLOT_DESC *      NewSlot(void *, ATOM_HN *);
static TEMP_SLOT_LINK * InsertSlot(void *, TEMP_SLOT_LINK *, SLOT_DESC *);
static int              ParseSimpleFacet(void *, char *, char*, char *, int, char *, char *, char *, char *, ATOM_HN **);
static BOOLEAN          ParseDefaultFacet(void *, char *, char *, SLOT_DESC *);
static void             BuildCompositeFacets(void *, SLOT_DESC *, PACKED_CLASS_LINKS *, char *, CONSTRAINT_PARSE_RECORD *);
static BOOLEAN          CheckForFacetConflicts(void *, SLOT_DESC *, CONSTRAINT_PARSE_RECORD *);
static BOOLEAN          EvaluateSlotDefaultValue(void *, SLOT_DESC *, char *);

/* =========================================
 *****************************************
 *       EXTERNALLY VISIBLE FUNCTIONS
 *  =========================================
 ***************************************** */

/************************************************************
 *  NAME         : ParseSlot
 *  DESCRIPTION  : Parses slot definitions for a
 *                defclass statement
 *  INPUTS       : 1) The logical name of the input source
 *              2) The current slot list
 *              3) The class precedence list for the class
 *                 to which this slot is being attached
 *                 (used to find facets for composite slots)
 *              4) A flag indicating if this is a list
 *                 slot or not
 *              5) A flag indicating if the type of slot
 *                 (single or multi) was explicitly
 *                 specified or not
 *  RETURNS      : The address of the list of slots,
 *                NULL if there was an error
 *  SIDE EFFECTS : The slot list is allocated
 *  NOTES        : Assumes "(slot" has already been parsed.
 ************************************************************/
globle TEMP_SLOT_LINK *ParseSlot(void *theEnv, char *readSource, TEMP_SLOT_LINK *slist, PACKED_CLASS_LINKS *preclist, int multiSlot, int fieldSpecified)
{
    SLOT_DESC *slot;
    CONSTRAINT_PARSE_RECORD parsedConstraint;
    char specbits[2];
    int rtnCode;
    ATOM_HN *newOverrideMsg;

    /* ===============================================================
     *  Bits in specbits are when slot qualifiers are specified so that
     *  duplicate or conflicting qualifiers can be detected.
     *
     *  Shared/local                          bit-0
     *  Single/multiple                       bit-1
     *  Read-only/Read-write/Initialize-Only  bit-2
     *  Inherit/No-inherit                    bit-3
     *  Composite/Exclusive                   bit-4
     *  Reactive/Nonreactive                  bit-5
     *  Default                               bit-6
     *  Default-dynamic                       bit-7
     *  Visibility                            bit-8
     *  Override-message                      bit-9
     *  =============================================================== */
    core_save_pp_buffer(theEnv, " ");
    specbits[0] = specbits[1] = '\0';
    core_get_token(theEnv, readSource, &DefclassData(theEnv)->ObjectParseToken);

    if( core_get_type(DefclassData(theEnv)->ObjectParseToken) != ATOM )
    {
        DeleteSlots(theEnv, slist);
        error_syntax(theEnv, "defclass slot");
        return(NULL);
    }

    if((DefclassData(theEnv)->ObjectParseToken.value == (void *)DefclassData(theEnv)->ISA_ATOM) ||
       (DefclassData(theEnv)->ObjectParseToken.value == (void *)DefclassData(theEnv)->NAME_ATOM))
    {
        DeleteSlots(theEnv, slist);
        error_syntax(theEnv, "defclass slot");
        return(NULL);
    }

    slot = NewSlot(theEnv, (ATOM_HN *)core_get_value(DefclassData(theEnv)->ObjectParseToken));
    slist = InsertSlot(theEnv, slist, slot);

    if( slist == NULL )
    {
        return(NULL);
    }

    if( multiSlot )
    {
        slot->multiple = TRUE;
    }

    if( fieldSpecified )
    {
        core_bitmap_set(specbits, FIELD_BIT);
    }

    core_get_token(theEnv, readSource, &DefclassData(theEnv)->ObjectParseToken);
    core_increment_pp_buffer_depth(theEnv, 3);
    InitializeConstraintParseRecord(&parsedConstraint);

    while( core_get_type(DefclassData(theEnv)->ObjectParseToken) == LPAREN )
    {
        core_backup_pp_buffer(theEnv);
        core_inject_ws_into_pp_buffer(theEnv);
        core_save_pp_buffer(theEnv, "(");
        core_get_token(theEnv, readSource, &DefclassData(theEnv)->ObjectParseToken);

        if( core_get_type(DefclassData(theEnv)->ObjectParseToken) != ATOM )
        {
            error_syntax(theEnv, "defclass slot");
            goto ParseSlotError;
        }
        else if( strcmp(core_convert_data_to_string(DefclassData(theEnv)->ObjectParseToken), DEFAULT_FACET) == 0 )
        {
            if( ParseDefaultFacet(theEnv, readSource, specbits, slot) == FALSE )
            {
                goto ParseSlotError;
            }
        }
        else if( strcmp(core_convert_data_to_string(DefclassData(theEnv)->ObjectParseToken), DYNAMIC_FACET) == 0 )
        {
            core_bitmap_set(specbits, DEFAULT_DYNAMIC_BIT);

            if( ParseDefaultFacet(theEnv, readSource, specbits, slot) == FALSE )
            {
                goto ParseSlotError;
            }
        }
        else if( strcmp(core_convert_data_to_string(DefclassData(theEnv)->ObjectParseToken), ACCESS_FACET) == 0 )
        {
            rtnCode = ParseSimpleFacet(theEnv, readSource, specbits, ACCESS_FACET, ACCESS_BIT,
                                       SLOT_RDWRT_RLN, SLOT_RDONLY_RLN, SLOT_INIT_RLN,
                                       NULL, NULL);

            if( rtnCode == -1 )
            {
                goto ParseSlotError;
            }
            else if( rtnCode == 1 )
            {
                slot->noWrite = 1;
            }
            else if( rtnCode == 2 )
            {
                slot->initializeOnly = 1;
            }
        }
        else if( strcmp(core_convert_data_to_string(DefclassData(theEnv)->ObjectParseToken), STORAGE_FACET) == 0 )
        {
            rtnCode = ParseSimpleFacet(theEnv, readSource, specbits, STORAGE_FACET, STORAGE_BIT,
                                       SLOT_LOCAL_RLN, SLOT_SHARE_RLN, NULL, NULL, NULL);

            if( rtnCode == -1 )
            {
                goto ParseSlotError;
            }

            slot->shared = rtnCode;
        }
        else if( strcmp(core_convert_data_to_string(DefclassData(theEnv)->ObjectParseToken), PROPAGATION_FACET) == 0 )
        {
            rtnCode = ParseSimpleFacet(theEnv, readSource, specbits, PROPAGATION_FACET, PROPAGATION_BIT,
                                       SLOT_INH_RLN, SLOT_NO_INH_RLN, NULL, NULL, NULL);

            if( rtnCode == -1 )
            {
                goto ParseSlotError;
            }

            slot->noInherit = rtnCode;
        }
        else if( strcmp(core_convert_data_to_string(DefclassData(theEnv)->ObjectParseToken), SOURCE_FACET) == 0 )
        {
            rtnCode = ParseSimpleFacet(theEnv, readSource, specbits, SOURCE_FACET, SOURCE_BIT,
                                       SLOT_EXCLUSIVE_RLN, SLOT_COMPOSITE_RLN, NULL, NULL, NULL);

            if( rtnCode == -1 )
            {
                goto ParseSlotError;
            }

            slot->composite = rtnCode;
        }

        else if( strcmp(core_convert_data_to_string(DefclassData(theEnv)->ObjectParseToken), VISIBILITY_FACET) == 0 )
        {
            rtnCode = ParseSimpleFacet(theEnv, readSource, specbits, VISIBILITY_FACET, VISIBILITY_BIT,
                                       SLOT_PRIVATE_RLN, SLOT_PUBLIC_RLN, NULL, NULL, NULL);

            if( rtnCode == -1 )
            {
                goto ParseSlotError;
            }

            slot->publicVisibility = rtnCode;
        }
        else if( strcmp(core_convert_data_to_string(DefclassData(theEnv)->ObjectParseToken), CREATE_ACCESSOR_FACET) == 0 )
        {
            rtnCode = ParseSimpleFacet(theEnv, readSource, specbits, CREATE_ACCESSOR_FACET,
                                       CREATE_ACCESSOR_BIT,
                                       SLOT_READ_RLN, SLOT_WRITE_RLN, SLOT_RDWRT_RLN,
                                       SLOT_NONE_RLN, NULL);

            if( rtnCode == -1 )
            {
                goto ParseSlotError;
            }

            if((rtnCode == 0) || (rtnCode == 2))
            {
                slot->createReadAccessor = TRUE;
            }

            if((rtnCode == 1) || (rtnCode == 2))
            {
                slot->createWriteAccessor = TRUE;
            }
        }
        else if( strcmp(core_convert_data_to_string(DefclassData(theEnv)->ObjectParseToken), OVERRIDE_MSG_FACET) == 0 )
        {
            rtnCode = ParseSimpleFacet(theEnv, readSource, specbits, OVERRIDE_MSG_FACET, OVERRIDE_MSG_BIT,
                                       NULL, NULL, NULL, SLOT_DEFAULT_RLN, &newOverrideMsg);

            if( rtnCode == -1 )
            {
                goto ParseSlotError;
            }

            if( rtnCode == 4 )
            {
                dec_atom_count(theEnv, slot->overrideMessage);
                slot->overrideMessage = newOverrideMsg;
                inc_atom_count(slot->overrideMessage);
            }

            slot->overrideMessageSpecified = TRUE;
        }
        else if( StandardConstraint(core_convert_data_to_string(DefclassData(theEnv)->ObjectParseToken)))
        {
            if( ParseStandardConstraint(theEnv, readSource, core_convert_data_to_string(DefclassData(theEnv)->ObjectParseToken),
                                        slot->constraint, &parsedConstraint, TRUE) == FALSE )
            {
                goto ParseSlotError;
            }
        }
        else
        {
            error_syntax(theEnv, "defclass slot");
            goto ParseSlotError;
        }

        core_get_token(theEnv, readSource, &DefclassData(theEnv)->ObjectParseToken);
    }

    if( core_get_type(DefclassData(theEnv)->ObjectParseToken) != RPAREN )
    {
        error_syntax(theEnv, "defclass slot");
        goto ParseSlotError;
    }

    if( DefclassData(theEnv)->ClassDefaultsMode == CONVENIENCE_MODE )
    {
        if( !core_bitmap_test(specbits, CREATE_ACCESSOR_BIT))
        {
            slot->createReadAccessor = TRUE;

            if( !slot->noWrite )
            {
                slot->createWriteAccessor = TRUE;
            }
        }
    }

    if( slot->composite )
    {
        BuildCompositeFacets(theEnv, slot, preclist, specbits, &parsedConstraint);
    }

    if( CheckForFacetConflicts(theEnv, slot, &parsedConstraint) == FALSE )
    {
        goto ParseSlotError;
    }

    if( CheckConstraintParseConflicts(theEnv, slot->constraint) == FALSE )
    {
        goto ParseSlotError;
    }

    if( EvaluateSlotDefaultValue(theEnv, slot, specbits) == FALSE )
    {
        goto ParseSlotError;
    }

    if((slot->dynamicDefault == 0) && (slot->noWrite == 1) &&
       (slot->initializeOnly == 0))
    {
        slot->shared = 1;
    }

    slot->constraint = addConstraint(theEnv, slot->constraint);
    core_decrement_pp_buffer_depth(theEnv, 3);
    return(slist);

ParseSlotError:
    core_decrement_pp_buffer_depth(theEnv, 3);
    DeleteSlots(theEnv, slist);
    return(NULL);
}

/***************************************************
 *  NAME         : DeleteSlots
 *  DESCRIPTION  : Deallocates a list of slots and
 *                their values
 *  INPUTS       : The address of the slot list
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : The slot list is destroyed
 *  NOTES        : None
 ***************************************************/
globle void DeleteSlots(void *theEnv, TEMP_SLOT_LINK *slots)
{
    TEMP_SLOT_LINK *stmp;

    while( slots != NULL )
    {
        stmp = slots;
        slots = slots->nxt;
        DeleteSlotName(theEnv, stmp->desc->slotName);
        dec_atom_count(theEnv, stmp->desc->overrideMessage);
        removeConstraint(theEnv, stmp->desc->constraint);

        if( stmp->desc->dynamicDefault == 1 )
        {
            core_decrement_expression(theEnv, (core_expression_object *)stmp->desc->defaultValue);
            core_return_packed_expression(theEnv, (core_expression_object *)stmp->desc->defaultValue);
        }
        else if( stmp->desc->defaultValue != NULL )
        {
            core_value_decrement(theEnv, (core_data_object *)stmp->desc->defaultValue);
            core_mem_return_struct(theEnv, core_data, stmp->desc->defaultValue);
        }

        core_mem_return_struct(theEnv, slotDescriptor, stmp->desc);
        core_mem_return_struct(theEnv, tempSlotLink, stmp);
    }
}

/* =========================================
 *****************************************
 *       INTERNALLY VISIBLE FUNCTIONS
 *  =========================================
 ***************************************** */

/**************************************************************
 *  NAME         : NewSlot
 *  DESCRIPTION  : Allocates and initalizes a new slot structure
 *  INPUTS       : The symbolic name of the new slot
 *  RETURNS      : The address of the new slot
 *  SIDE EFFECTS : None
 *  NOTES        : Also adds symbols of the form get-<name> and
 *                put-<name> for slot accessors
 **************************************************************/
static SLOT_DESC *NewSlot(void *theEnv, ATOM_HN *name)
{
    SLOT_DESC *slot;

    slot = core_mem_get_struct(theEnv, slotDescriptor);
    slot->dynamicDefault = 1;
    slot->defaultSpecified = 0;
    slot->noDefault = 0;
    slot->noInherit = 0;
    slot->noWrite = 0;
    slot->initializeOnly = 0;
    slot->shared = 0;
    slot->multiple = 0;
    slot->composite = 0;
    slot->sharedCount = 0;
    slot->publicVisibility = 0;
    slot->createReadAccessor = FALSE;
    slot->createWriteAccessor = FALSE;
    slot->overrideMessageSpecified = 0;
    slot->cls = NULL;
    slot->defaultValue = NULL;
    slot->constraint = newConstraint(theEnv);
    slot->slotName = AddSlotName(theEnv, name, 0, FALSE);
    slot->overrideMessage = slot->slotName->putHandlerName;
    inc_atom_count(slot->overrideMessage);
    return(slot);
}

/**********************************************************
 *  NAME         : InsertSlot
 *  DESCRIPTION  : Inserts a slot into the list of slots
 *  INPUTS       : 1) The current head of the slot list
 *              2) The slot to be inserted
 *  RETURNS      : The head of the slot list
 *  SIDE EFFECTS : The slot is inserted if no errors,
 *                otherwise the original list and the
 *                new slot are destroyed
 *  NOTES        : None
 **********************************************************/
static TEMP_SLOT_LINK *InsertSlot(void *theEnv, TEMP_SLOT_LINK *slist, SLOT_DESC *slot)
{
    TEMP_SLOT_LINK *stmp, *sprv, *tmp;

    tmp = core_mem_get_struct(theEnv, tempSlotLink);
    tmp->desc = slot;
    tmp->nxt = NULL;

    if( slist == NULL )
    {
        slist = tmp;
    }
    else
    {
        stmp = slist;
        sprv = NULL;

        while( stmp != NULL )
        {
            if( stmp->desc->slotName == slot->slotName )
            {
                tmp->nxt = slist;
                DeleteSlots(theEnv, tmp);
                error_print_id(theEnv, "CLSLTPSR", 1, FALSE);
                print_router(theEnv, WERROR, "Duplicate slots not allowed.\n");
                return(NULL);
            }

            sprv = stmp;
            stmp = stmp->nxt;
        }

        sprv->nxt = tmp;
    }

    return(slist);
}

/****************************************************************
 * NAME         : ParseSimpleFacet
 * DESCRIPTION  : Parses the following facets for a slot:
 *               access, source, propagation, storage,
 *               pattern-match, visibility and override-message
 * INPUTS       : 1) The input logical name
 *             2) The bitmap indicating which facets have
 *                already been parsed
 *             3) The name of the facet
 *             4) The bit to test/set in arg #2 for this facet
 *             5) The facet value string which indicates the
 *                facet should be false
 *             6) The facet value string which indicates the
 *                facet should be TRUE
 *             7) An alternate value string for use when the
 *                first two don't match (can be NULL)
 *             7) An alternate value string for use when the
 *                first three don't match (can be NULL)
 *                (will be an SCALAR_VARIABLE type)
 *             9) A buffer to hold the facet value symbol
 *                (can be NULL - only set if args #5 and #6
 *                 are both NULL)
 * RETURNS      : -1 on errors
 *              0 if first value string matched
 *              1 if second value string matched
 *              2 if alternate value string matched
 *              3 if variable value string matched
 *              4 if facet value buffer was set
 * SIDE EFFECTS : Messages printed on errors
 *             Bitmap marked indicating facet was parsed
 *             Facet value symbol buffer set, if appropriate
 * NOTES        : None
 *****************************************************************/
static int ParseSimpleFacet(void *theEnv, char *readSource, char *specbits, char *facetName, int testBit, char *clearRelation, char *setRelation, char *alternateRelation, char *varRelation, ATOM_HN **facetSymbolicValue)
{
    int rtnCode;

    if( core_bitmap_test(specbits, testBit))
    {
        error_print_id(theEnv, "CLSLTPSR", 2, FALSE);
        print_router(theEnv, WERROR, facetName);
        print_router(theEnv, WERROR, " facet already specified.\n");
        return(-1);
    }

    core_bitmap_set(specbits, testBit);
    core_save_pp_buffer(theEnv, " ");
    core_get_token(theEnv, readSource, &DefclassData(theEnv)->ObjectParseToken);

    /* ===============================
     *  Check for the variable relation
     *  =============================== */
    if( DefclassData(theEnv)->ObjectParseToken.type == SCALAR_VARIABLE )
    {
        if((varRelation == NULL) ? FALSE :
           (strcmp(core_convert_data_to_string(DefclassData(theEnv)->ObjectParseToken), varRelation) == 0))
        {
            rtnCode = 3;
        }
        else
        {
            goto ParseSimpleFacetError;
        }
    }
    else
    {
        if( DefclassData(theEnv)->ObjectParseToken.type != ATOM )
        {
            goto ParseSimpleFacetError;
        }

        /* ===================================================
         *  If the facet value buffer is non-NULL
         *  simply get the value and do not check any relations
         *  =================================================== */
        if( facetSymbolicValue == NULL )
        {
            if( strcmp(core_convert_data_to_string(DefclassData(theEnv)->ObjectParseToken), clearRelation) == 0 )
            {
                rtnCode = 0;
            }
            else if( strcmp(core_convert_data_to_string(DefclassData(theEnv)->ObjectParseToken), setRelation) == 0 )
            {
                rtnCode = 1;
            }
            else if((alternateRelation == NULL) ? FALSE :
                    (strcmp(core_convert_data_to_string(DefclassData(theEnv)->ObjectParseToken), alternateRelation) == 0))
            {
                rtnCode = 2;
            }
            else
            {
                goto ParseSimpleFacetError;
            }
        }
        else
        {
            rtnCode = 4;
            *facetSymbolicValue = (ATOM_HN *)DefclassData(theEnv)->ObjectParseToken.value;
        }
    }

    core_get_token(theEnv, readSource, &DefclassData(theEnv)->ObjectParseToken);

    if( DefclassData(theEnv)->ObjectParseToken.type != RPAREN )
    {
        goto ParseSimpleFacetError;
    }

    return(rtnCode);

ParseSimpleFacetError:
    error_syntax(theEnv, "slot facet");
    return(-1);
}

/*************************************************************
 *  NAME         : ParseDefaultFacet
 *  DESCRIPTION  : Parses the facet for a slot
 *  INPUTS       : 1) The input logical name
 *              2) The bitmap indicating which facets have
 *                 already been parsed
 *              3) The slot descriptor to set
 *  RETURNS      : TRUE if all OK, FALSE otherwise
 *  SIDE EFFECTS : Slot  set and parsed facet bitmap set
 *  NOTES        : Syntax: (default ?NONE|<expression>*)
 *                      (default-dynamic <expression>*)
 *************************************************************/
static BOOLEAN ParseDefaultFacet(void *theEnv, char *readSource, char *specbits, SLOT_DESC *slot)
{
    core_expression_object *tmp;
    int error, noneSpecified, deriveSpecified;

    if( core_bitmap_test(specbits, DEFAULT_BIT))
    {
        error_print_id(theEnv, "CLSLTPSR", 2, FALSE);
        print_router(theEnv, WERROR, "default facet already specified.\n");
        return(FALSE);
    }

    core_bitmap_set(specbits, DEFAULT_BIT);
    error = FALSE;
    tmp = ParseDefault(theEnv, readSource, 1, (int)core_bitmap_test(specbits, DEFAULT_DYNAMIC_BIT),
                       0, &noneSpecified, &deriveSpecified, &error);

    if( error == TRUE )
    {
        return(FALSE);
    }

    if( noneSpecified || deriveSpecified )
    {
        if( noneSpecified )
        {
            slot->noDefault = 1;
            slot->defaultSpecified = 1;
        }
        else
        {
            core_bitmap_clear(specbits, DEFAULT_BIT);
        }
    }
    else
    {
        slot->defaultValue = (void *)core_pack_expression(theEnv, tmp);
        core_return_expression(theEnv, tmp);
        core_increment_expression(theEnv, (core_expression_object *)slot->defaultValue);
        slot->defaultSpecified = 1;
    }

    return(TRUE);
}

/**************************************************************************
 * NAME         : BuildCompositeFacets
 * DESCRIPTION  : Composite slots are ones that get their facets
 *               from more than one class.  By default, the most
 *               specific class in object's precedence list specifies
 *               the complete set of facets for a slot.  The composite
 *               facet in a slot allows facets that are not overridden
 *               by the most specific class to be obtained from other
 *               classes.
 *
 *             Since all superclasses are predetermined before creating
 *               a new class based on them, this routine need only
 *               examine the immediately next most specific class for
 *               extra facets.  Even if that slot is also composite, the
 *               other facets have already been filtered down.  If the
 *               slot is no-inherit, the next most specific class must
 *               be examined.
 * INPUTS       : 1) The slot descriptor
 *             2) The class precedence list
 *             3) The bitmap marking which facets were specified in
 *                the original slot definition
 * RETURNS      : Nothing useful
 * SIDE EFFECTS : Composite slot is updated to reflect facets from
 *               a less specific class
 * NOTES        : Assumes slot is composite
 *************************************************************************/
static void BuildCompositeFacets(void *theEnv, SLOT_DESC *sd, PACKED_CLASS_LINKS *preclist, char *specbits, CONSTRAINT_PARSE_RECORD *parsedConstraint)
{
    SLOT_DESC *compslot = NULL;
    long i;

    for( i = 1 ; i < preclist->classCount ; i++ )
    {
        compslot = FindClassSlot(preclist->classArray[i], sd->slotName->name);

        if((compslot != NULL) ? (compslot->noInherit == 0) : FALSE )
        {
            break;
        }
    }

    if( compslot != NULL )
    {
        if((sd->defaultSpecified == 0) && (compslot->defaultSpecified == 1))
        {
            sd->dynamicDefault = compslot->dynamicDefault;
            sd->noDefault = compslot->noDefault;
            sd->defaultSpecified = 1;

            if( compslot->defaultValue != NULL )
            {
                if( sd->dynamicDefault )
                {
                    sd->defaultValue = (void *)core_pack_expression(theEnv, (core_expression_object *)compslot->defaultValue);
                    core_increment_expression(theEnv, (core_expression_object *)sd->defaultValue);
                }
                else
                {
                    sd->defaultValue = (void *)core_mem_get_struct(theEnv, core_data);
                    core_mem_copy_memory(core_data_object, 1, sd->defaultValue, compslot->defaultValue);
                    core_value_increment(theEnv, (core_data_object *)sd->defaultValue);
                }
            }
        }

        if( core_bitmap_test(specbits, FIELD_BIT) == 0 )
        {
            sd->multiple = compslot->multiple;
        }

        if( core_bitmap_test(specbits, STORAGE_BIT) == 0 )
        {
            sd->shared = compslot->shared;
        }

        if( core_bitmap_test(specbits, ACCESS_BIT) == 0 )
        {
            sd->noWrite = compslot->noWrite;
            sd->initializeOnly = compslot->initializeOnly;
        }


        if( core_bitmap_test(specbits, VISIBILITY_BIT) == 0 )
        {
            sd->publicVisibility = compslot->publicVisibility;
        }

        if( core_bitmap_test(specbits, CREATE_ACCESSOR_BIT) == 0 )
        {
            sd->createReadAccessor = compslot->createReadAccessor;
            sd->createWriteAccessor = compslot->createWriteAccessor;
        }

        if((core_bitmap_test(specbits, OVERRIDE_MSG_BIT) == 0) &&
           compslot->overrideMessageSpecified )
        {
            dec_atom_count(theEnv, sd->overrideMessage);
            sd->overrideMessage = compslot->overrideMessage;
            inc_atom_count(sd->overrideMessage);
            sd->overrideMessageSpecified = TRUE;
        }

        OverlayConstraint(theEnv, parsedConstraint, sd->constraint, compslot->constraint);
    }
}

/***************************************************
 *  NAME         : CheckForFacetConflicts
 *  DESCRIPTION  : Determines if all facets specified
 *              (and inherited) for a slot are
 *              consistent
 *  INPUTS       : 1) The slot descriptor
 *              2) The parse record for the
 *                 type constraints on the slot
 *  RETURNS      : TRUE if all OK,
 *              FALSE otherwise
 *  SIDE EFFECTS : Min and Max fields replaced in
 *              constraint for single-field slot
 *  NOTES        : None
 ***************************************************/
static BOOLEAN CheckForFacetConflicts(void *theEnv, SLOT_DESC *sd, CONSTRAINT_PARSE_RECORD *parsedConstraint)
{
    if( sd->multiple == 0 )
    {
        if( parsedConstraint->cardinality )
        {
            error_print_id(theEnv, "CLSLTPSR", 3, TRUE);
            print_router(theEnv, WERROR, "Cardinality facet can only be used with list slots\n");
            return(FALSE);
        }
        else
        {
            core_return_expression(theEnv, sd->constraint->min_fields);
            core_return_expression(theEnv, sd->constraint->max_fields);
            sd->constraint->min_fields = core_generate_constant(theEnv, INTEGER, store_long(theEnv, 1LL));
            sd->constraint->max_fields = core_generate_constant(theEnv, INTEGER, store_long(theEnv, 1LL));
        }
    }

    if( sd->noDefault && sd->noWrite )
    {
        error_print_id(theEnv, "CLSLTPSR", 4, TRUE);
        print_router(theEnv, WERROR, "read-only slots must have a default value\n");
        return(FALSE);
    }

    if( sd->noWrite && (sd->createWriteAccessor || sd->overrideMessageSpecified))
    {
        error_print_id(theEnv, "CLSLTPSR", 5, TRUE);
        print_router(theEnv, WERROR, "read-only slots cannot have a write accessor\n");
        return(FALSE);
    }

    if( sd->noInherit && sd->publicVisibility )
    {
        error_print_id(theEnv, "CLSLTPSR", 6, TRUE);
        print_router(theEnv, WERROR, "no-inherit slots cannot also be public\n");
        return(FALSE);
    }

    return(TRUE);
}

/********************************************************************
 *  NAME         : EvaluateSlotDefaultValue
 *  DESCRIPTION  : Checks the default value against the slot
 *              constraints and evaluates static default values
 *  INPUTS       : 1) The slot descriptor
 *              2) The bitmap marking which facets were specified in
 *                 the original slot definition
 *  RETURNS      : TRUE if all OK, FALSE otherwise
 *  SIDE EFFECTS : Static default value expressions deleted and
 *              replaced with data object evaluation
 *  NOTES        : On errors, slot is marked as dynamix so that
 *              DeleteSlots() will erase the slot expression
 ********************************************************************/
static BOOLEAN EvaluateSlotDefaultValue(void *theEnv, SLOT_DESC *sd, char *specbits)
{
    core_data_object temp;
    int oldce, olddcc, vCode;

    /* ===================================================================
     *  Slot default value expression is marked as dynamic until now so
     *  that DeleteSlots() would erase in the event of an error.  The delay
     *  was so that the evaluation of a static default value could be
     *  delayed until all the constraints were parsed.
     *  =================================================================== */
    if( core_bitmap_test(specbits, DEFAULT_DYNAMIC_BIT) == 0 )
    {
        sd->dynamicDefault = 0;
    }

    if( sd->noDefault )
    {
        return(TRUE);
    }

    if( sd->dynamicDefault == 0 )
    {
        if( core_bitmap_test(specbits, DEFAULT_BIT))
        {
            oldce = core_is_construct_executing(theEnv);
            core_set_construct_executing(theEnv, TRUE);
            olddcc = setDynamicConstraints(theEnv, getStaticConstraints(theEnv));
            vCode = EvaluateAndStoreInDataObject(theEnv, (int)sd->multiple,
                                                 (core_expression_object *)sd->defaultValue, &temp, TRUE);

            if( vCode != FALSE )
            {
                vCode = ValidSlotValue(theEnv, &temp, sd, NULL, "slot default value");
            }

            setDynamicConstraints(theEnv, olddcc);
            core_set_construct_executing(theEnv, oldce);

            if( vCode )
            {
                core_decrement_expression(theEnv, (core_expression_object *)sd->defaultValue);
                core_return_packed_expression(theEnv, (core_expression_object *)sd->defaultValue);
                sd->defaultValue = (void *)core_mem_get_struct(theEnv, core_data);
                core_mem_copy_memory(core_data_object, 1, sd->defaultValue, &temp);
                core_value_increment(theEnv, (core_data_object *)sd->defaultValue);
            }
            else
            {
                sd->dynamicDefault = 1;
                return(FALSE);
            }
        }
        else if( sd->defaultSpecified == 0 )
        {
            sd->defaultValue = (void *)core_mem_get_struct(theEnv, core_data);
            DeriveDefaultFromConstraints(theEnv, sd->constraint,
                                         (core_data_object *)sd->defaultValue, (int)sd->multiple, TRUE);
            core_value_increment(theEnv, (core_data_object *)sd->defaultValue);
        }
    }
    else if( getStaticConstraints(theEnv))
    {
        vCode = garner_expression_violations(theEnv, (core_expression_object *)sd->defaultValue, sd->constraint);

        if( vCode != NO_VIOLATION )
        {
            error_print_id(theEnv, "CSTRNCHK", 1, FALSE);
            print_router(theEnv, WERROR, "Expression for ");
            PrintSlot(theEnv, WERROR, sd, NULL, "dynamic default value");
            report_constraint_violation_error(theEnv, NULL, NULL, 0, 0, NULL, 0,
                                            vCode, sd->constraint, FALSE);
            return(FALSE);
        }
    }

    return(TRUE);
}

#endif
