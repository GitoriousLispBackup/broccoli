/* Purpose: Parsing Routines for Defclass Construct           */

/* =========================================
 *****************************************
 *            EXTERNAL DEFINITIONS
 *  =========================================
 ***************************************** */
#include "setup.h"

#if OBJECT_SYSTEM

#include "classes_kernel.h"
#include "funcs_class.h"
#include "parser_class_members.h"
#include "parser_constructs.h"
#include "core_environment.h"
#include "parser_class_inheritance.h"
#include "core_memory.h"
#include "parser_modules.h"
#include "modules_query.h"
#include "parser_class_methods.h"
#include "router.h"
#include "core_scanner.h"

#define __PARSER_CLASS_SOURCE__
#include "parser_class.h"

/* =========================================
 *****************************************
 *                CONSTANTS
 *  =========================================
 ***************************************** */
#define ROLE_RLN             "role"
#define ABSTRACT_RLN         "abstract"
#define CONCRETE_RLN         "concrete"

#define HANDLER_DECL         "message-handler"

#define SLOT_RLN             "slot"
#define SGL_SLOT_RLN         "single-slot"
#define MLT_SLOT_RLN         "multislot"

#define DIRECT               0
#define INHERIT              1

/* =========================================
 *****************************************
 *   INTERNALLY VISIBLE FUNCTION HEADERS
 *  =========================================
 ***************************************** */

static BOOLEAN          ValidClassName(void *, char *, DEFCLASS **);
static BOOLEAN          ParseSimpleQualifier(void *, char *, char *, char *, char *, BOOLEAN *, BOOLEAN *);
static BOOLEAN          ReadUntilClosingParen(void *, char *, struct token *);
static void             AddClass(void *, DEFCLASS *);
static void             BuildSubclassLinks(void *, DEFCLASS *);
static void             FormInstanceTemplate(void *, DEFCLASS *);
static void             FormSlotNameMap(void *, DEFCLASS *);
static TEMP_SLOT_LINK * MergeSlots(void *, TEMP_SLOT_LINK *, DEFCLASS *, short *, int);
static void             PackSlots(void *, DEFCLASS *, TEMP_SLOT_LINK *);
#if DEFMODULE_CONSTRUCT
static void CreateClassScopeMap(void *, DEFCLASS *);
#endif
static void CreatePublicSlotMessageHandlers(void *, DEFCLASS *);

/* =========================================
 *****************************************
 *       EXTERNALLY VISIBLE FUNCTIONS
 *  =========================================
 ***************************************** */

/***************************************************************************************
 *  NAME         : ParseDefclass
 *  DESCRIPTION  : (defclass ...) is a construct (as
 *              opposed to a function), thus no variables
 *              may be used.  This means classes may only
 *              be STATICALLY defined (like rules).
 *  INPUTS       : The logical name of the router
 *                 for the parser input
 *  RETURNS      : FALSE if successful parse, TRUE otherwise
 *  SIDE EFFECTS : Inserts valid class definition into
 *              Class Table.
 *  NOTES        : H/L Syntax :
 *              (defclass <name> [<comment>]
 *                 (is-a <superclass-name>+)
 *                 <class-descriptor>*)
 *
 *              <class-descriptor> :== (slot <name> <slot-descriptor>*) |
 *                                     (role abstract|concrete) |
 *                                     (pattern-match reactive|non-reactive)
 *
 *                                     These are for documentation only:
 *                                     (message-handler <name> [<type>])
 *
 *              <slot-descriptor>  :== (default <default-expression>) |
 *                                     (default-dynamic <default-expression>) |
 *                                     (storage shared|local) |
 *                                     (access read-only|read-write|initialize-only) |
 *                                     (propagation no-inherit|inherit) |
 *                                     (source composite|exclusive)
 *                                     (pattern-match reactive|non-reactive)
 *                                     (visibility public|private)
 *                                     (override-message <message-name>)
 *                                     (type ...) |
 *                                     (cardinality ...) |
 *                                     (allowed-symbols ...) |
 *                                     (allowed-strings ...) |
 *                                     (allowed-numbers ...) |
 *                                     (allowed-integers ...) |
 *                                     (allowed-floats ...) |
 *                                     (allowed-values ...) |
 *                                     (allowed-instance-names ...) |
 *                                     (allowed-classes ...) |
 *                                     (range ...)
 *
 *            <default-expression> ::= ?NONE | ?VARIABLE | <expression>*
 ***************************************************************************************/
globle int ParseDefclass(void *theEnv, char *readSource)
{
    ATOM_HN *cname;
    DEFCLASS *cls;
    PACKED_CLASS_LINKS *sclasses, *preclist;
    TEMP_SLOT_LINK *slots = NULL;
    int roleSpecified = FALSE,
        abstract = FALSE,
        parseError;


    core_set_pp_buffer_status(theEnv, ON);
    core_flush_pp_buffer(theEnv);
    core_pp_indent_depth(theEnv, 3);
    core_save_pp_buffer(theEnv, "(defclass ");


    cname = parse_construct_head(theEnv, readSource, &DefclassData(theEnv)->ObjectParseToken, "defclass",
                                       EnvFindDefclass, NULL, "#", TRUE,
                                       TRUE, TRUE);

    if( cname == NULL )
    {
        return(TRUE);
    }

    if( ValidClassName(theEnv, to_string(cname), &cls) == FALSE )
    {
        return(TRUE);
    }

    sclasses = ParseSuperclasses(theEnv, readSource, cname);

    if( sclasses == NULL )
    {
        return(TRUE);
    }

    preclist = FindPrecedenceList(theEnv, cls, sclasses);

    if( preclist == NULL )
    {
        DeletePackedClassLinks(theEnv, sclasses, TRUE);
        return(TRUE);
    }

    parseError = FALSE;
    core_get_token(theEnv, readSource, &DefclassData(theEnv)->ObjectParseToken);

    while( core_get_type(DefclassData(theEnv)->ObjectParseToken) != RPAREN )
    {
        if( core_get_type(DefclassData(theEnv)->ObjectParseToken) != LPAREN )
        {
            error_syntax(theEnv, "defclass");
            parseError = TRUE;
            break;
        }

        core_backup_pp_buffer(theEnv);
        core_inject_ws_into_pp_buffer(theEnv);
        core_save_pp_buffer(theEnv, "(");
        core_get_token(theEnv, readSource, &DefclassData(theEnv)->ObjectParseToken);

        if( core_get_type(DefclassData(theEnv)->ObjectParseToken) != ATOM )
        {
            error_syntax(theEnv, "defclass");
            parseError = TRUE;
            break;
        }

        if( strcmp(core_convert_data_to_string(DefclassData(theEnv)->ObjectParseToken), ROLE_RLN) == 0 )
        {
            if( ParseSimpleQualifier(theEnv, readSource, ROLE_RLN, CONCRETE_RLN, ABSTRACT_RLN,
                                     &roleSpecified, &abstract) == FALSE )
            {
                parseError = TRUE;
                break;
            }
        }

        else if( strcmp(core_convert_data_to_string(DefclassData(theEnv)->ObjectParseToken), SLOT_RLN) == 0 )
        {
            slots = ParseSlot(theEnv, readSource, slots, preclist, FALSE, FALSE);

            if( slots == NULL )
            {
                parseError = TRUE;
                break;
            }
        }
        else if( strcmp(core_convert_data_to_string(DefclassData(theEnv)->ObjectParseToken), SGL_SLOT_RLN) == 0 )
        {
            slots = ParseSlot(theEnv, readSource, slots, preclist, FALSE, TRUE);

            if( slots == NULL )
            {
                parseError = TRUE;
                break;
            }
        }
        else if( strcmp(core_convert_data_to_string(DefclassData(theEnv)->ObjectParseToken), MLT_SLOT_RLN) == 0 )
        {
            slots = ParseSlot(theEnv, readSource, slots, preclist, TRUE, TRUE);

            if( slots == NULL )
            {
                parseError = TRUE;
                break;
            }
        }
        else if( strcmp(core_convert_data_to_string(DefclassData(theEnv)->ObjectParseToken), HANDLER_DECL) == 0 )
        {
            if( ReadUntilClosingParen(theEnv, readSource, &DefclassData(theEnv)->ObjectParseToken) == FALSE )
            {
                parseError = TRUE;
                break;
            }
        }
        else
        {
            error_syntax(theEnv, "defclass");
            parseError = TRUE;
            break;
        }

        core_get_token(theEnv, readSource, &DefclassData(theEnv)->ObjectParseToken);
    }

    if((core_get_type(DefclassData(theEnv)->ObjectParseToken) != RPAREN) || (parseError == TRUE))
    {
        DeletePackedClassLinks(theEnv, sclasses, TRUE);
        DeletePackedClassLinks(theEnv, preclist, TRUE);
        DeleteSlots(theEnv, slots);
        return(TRUE);
    }

    core_save_pp_buffer(theEnv, "\n");

    /* =========================================================================
     *  The abstract/reactive qualities of a class are inherited if not specified
     *  ========================================================================= */
    if( roleSpecified == FALSE )
    {
        if( preclist->classArray[1]->system &&                             /* Change to cause         */
            (DefclassData(theEnv)->ClassDefaultsMode == CONVENIENCE_MODE))     /* default role of         */
        {
            abstract = FALSE;    /* classes to be concrete. */
        }
        else
        {
            abstract = preclist->classArray[1]->abstract;
        }
    }


    /* =======================================================
     *  If we're only checking syntax, don't add the
     *  successfully parsed defclass to the KB.
     *  ======================================================= */

    if( core_construct_data(theEnv)->check_syntax )
    {
        DeletePackedClassLinks(theEnv, sclasses, TRUE);
        DeletePackedClassLinks(theEnv, preclist, TRUE);
        DeleteSlots(theEnv, slots);
        return(FALSE);
    }

    cls = NewClass(theEnv, cname);
    cls->abstract = abstract;
    cls->directSuperclasses.classCount = sclasses->classCount;
    cls->directSuperclasses.classArray = sclasses->classArray;

    /* =======================================================
     *  This is a hack to let functions which need to iterate
     *  over a class AND its superclasses to conveniently do so
     *
     *  The real precedence list starts in position 1
     *  ======================================================= */
    preclist->classArray[0] = cls;
    cls->allSuperclasses.classCount = preclist->classCount;
    cls->allSuperclasses.classArray = preclist->classArray;
    core_mem_return_struct(theEnv, packedClassLinks, sclasses);
    core_mem_return_struct(theEnv, packedClassLinks, preclist);

    /* =================================
     *  Shove slots into contiguous array
     *  ================================= */
    if( slots != NULL )
    {
        PackSlots(theEnv, cls, slots);
    }

    AddClass(theEnv, cls);

    return(FALSE);
}

/* =========================================
 *****************************************
 *       INTERNALLY VISIBLE FUNCTIONS
 *  =========================================
 ***************************************** */

/***********************************************************
 *  NAME         : ValidClassName
 *  DESCRIPTION  : Determines if a new class of the given
 *              name can be defined in the current module
 *  INPUTS       : 1) The new class name
 *              2) Buffer to hold class address
 *  RETURNS      : TRUE if OK, FALSE otherwise
 *  SIDE EFFECTS : Error message printed if not OK
 *  NOTES        : parse_construct_head() (called before
 *              this function) ensures that the defclass
 *              name does not conflict with one from
 *              another module
 ***********************************************************/
static BOOLEAN ValidClassName(void *theEnv, char *theClassName, DEFCLASS **theDefclass)
{
    *theDefclass = (DEFCLASS *)EnvFindDefclass(theEnv, theClassName);

    if( *theDefclass != NULL )
    {
        /* ===================================
         *  System classes (which are visible
         *  in all modules) cannot be redefined
         *  =================================== */
        if((*theDefclass)->system )
        {
            error_print_id(theEnv, "CLASSPSR", 2, FALSE);
            print_router(theEnv, WERROR, "Cannot redefine a predefined system class.\n");
            return(FALSE);
        }

        /* ===============================================
         *  A class in the current module can only be
         *  redefined if it is not in use, e.g., instances,
         *  generic function method restrictions, etc.
         *  =============================================== */
        if((EnvIsDefclassDeletable(theEnv, (void *)*theDefclass) == FALSE) &&
           (!core_construct_data(theEnv)->check_syntax))
        {
            error_print_id(theEnv, "CLASSPSR", 3, FALSE);
            print_router(theEnv, WERROR, EnvGetDefclassName(theEnv, (void *)*theDefclass));
            print_router(theEnv, WERROR, " class cannot be redefined while\n");
            print_router(theEnv, WERROR, "    outstanding references to it still exist.\n");
            return(FALSE);
        }
    }

    return(TRUE);
}

/***************************************************************
 *  NAME         : ParseSimpleQualifier
 *  DESCRIPTION  : Parses abstract/concrete role and
 *              pattern-matching reactivity for class
 *  INPUTS       : 1) The input logical name
 *              2) The name of the qualifier being parsed
 *              3) The qualifier value indicating that the
 *                 qualifier should be false
 *              4) The qualifier value indicating that the
 *                 qualifier should be TRUE
 *              5) A pointer to a bitmap indicating
 *                 if the qualifier has already been parsed
 *              6) A buffer to store the value of the qualifier
 *  RETURNS      : TRUE if all OK, FALSE otherwise
 *  SIDE EFFECTS : Bitmap and qualifier buffers set
 *              Messages printed on errors
 *  NOTES        : None
 ***************************************************************/
static BOOLEAN ParseSimpleQualifier(void *theEnv, char *readSource, char *classQualifier, char *clearRelation, char *setRelation, BOOLEAN *alreadyTestedFlag, BOOLEAN *binaryFlag)
{
    if( *alreadyTestedFlag )
    {
        error_print_id(theEnv, "CLASSPSR", 4, FALSE);
        print_router(theEnv, WERROR, "Class ");
        print_router(theEnv, WERROR, classQualifier);
        print_router(theEnv, WERROR, " already declared.\n");
        return(FALSE);
    }

    core_save_pp_buffer(theEnv, " ");
    core_get_token(theEnv, readSource, &DefclassData(theEnv)->ObjectParseToken);

    if( core_get_type(DefclassData(theEnv)->ObjectParseToken) != ATOM )
    {
        goto ParseSimpleQualifierError;
    }

    if( strcmp(core_convert_data_to_string(DefclassData(theEnv)->ObjectParseToken), setRelation) == 0 )
    {
        *binaryFlag = TRUE;
    }
    else if( strcmp(core_convert_data_to_string(DefclassData(theEnv)->ObjectParseToken), clearRelation) == 0 )
    {
        *binaryFlag = FALSE;
    }
    else
    {
        goto ParseSimpleQualifierError;
    }

    core_get_token(theEnv, readSource, &DefclassData(theEnv)->ObjectParseToken);

    if( core_get_type(DefclassData(theEnv)->ObjectParseToken) != RPAREN )
    {
        goto ParseSimpleQualifierError;
    }

    *alreadyTestedFlag = TRUE;
    return(TRUE);

ParseSimpleQualifierError:
    error_syntax(theEnv, "defclass");
    return(FALSE);
}

/***************************************************
 *  NAME         : ReadUntilClosingParen
 *  DESCRIPTION  : Skips over tokens until a ')' is
 *              encountered.
 *  INPUTS       : 1) The logical input source
 *              2) A buffer for scanned tokens
 *  RETURNS      : TRUE if ')' read, FALSE
 *              otherwise
 *  SIDE EFFECTS : Tokens read
 *  NOTES        : Expects first token after opening
 *              paren has already been scanned
 ***************************************************/
static BOOLEAN ReadUntilClosingParen(void *theEnv, char *readSource, struct token *inputToken)
{
    int cnt = 1, lparen_read = FALSE;

    do
    {
        if( lparen_read == FALSE )
        {
            core_save_pp_buffer(theEnv, " ");
        }

        core_get_token(theEnv, readSource, inputToken);

        if( inputToken->type == STOP )
        {
            error_syntax(theEnv, "message-handler declaration");
            return(FALSE);
        }
        else if( inputToken->type == LPAREN )
        {
            lparen_read = TRUE;
            cnt++;
        }
        else if( inputToken->type == RPAREN )
        {
            cnt--;

            if( lparen_read == FALSE )
            {
                core_backup_pp_buffer(theEnv);
                core_backup_pp_buffer(theEnv);
                core_save_pp_buffer(theEnv, ")");
            }

            lparen_read = FALSE;
        }
        else
        {
            lparen_read = FALSE;
        }
    } while( cnt > 0 );

    return(TRUE);
}

/*****************************************************************************
 *  NAME         : AddClass
 *  DESCRIPTION  : Determines the precedence list of the new class.
 *              If it is valid, the routine checks to see if the class
 *              already exists.  If it does not, all the subclass
 *              links are made from the class's direct superclasses,
 *              and the class is inserted in the hash table.  If it
 *              does, all sublclasses are deleted. An error will occur
 *              if any instances of the class (direct or indirect) exist.
 *              If all checks out, the old definition is replaced by the new.
 *  INPUTS       : The new class description
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : The class is deleted if there is an error.
 *  NOTES        : No change in the class graph state will occur
 *              if there were any errors.
 *              Assumes class is not busy!!!
 *****************************************************************************/
static void AddClass(void *theEnv, DEFCLASS *cls)
{
    DEFCLASS *ctmp;

#if DEBUGGING_FUNCTIONS
    int oldTraceInstances = FALSE,
        oldTraceSlots = FALSE;
#endif

    /* ===============================================
     *  If class does not already exist, insert and
     *  form progeny links with all direct superclasses
     *  =============================================== */
    cls->hashTableIndex = HashClass(GetDefclassNamePointer((void *)cls));
    ctmp = (DEFCLASS *)EnvFindDefclass(theEnv, EnvGetDefclassName(theEnv, (void *)cls));

    if( ctmp != NULL )
    {
#if DEBUGGING_FUNCTIONS
        oldTraceInstances = ctmp->traceInstances;
        oldTraceSlots = ctmp->traceSlots;
#endif
        DeleteClassUAG(theEnv, ctmp);
    }

    PutClassInTable(theEnv, cls);

    BuildSubclassLinks(theEnv, cls);
    InstallClass(theEnv, cls, TRUE);
    core_add_to_module((struct construct_metadata *)cls);

    FormInstanceTemplate(theEnv, cls);
    FormSlotNameMap(theEnv, cls);

    AssignClassID(theEnv, cls);

#if DEBUGGING_FUNCTIONS

    if( cls->abstract )
    {
        cls->traceInstances = FALSE;
        cls->traceSlots = FALSE;
    }
    else
    {
        if( oldTraceInstances )
        {
            cls->traceInstances = TRUE;
        }

        if( oldTraceSlots )
        {
            cls->traceSlots = TRUE;
        }
    }

#endif

#if DEBUGGING_FUNCTIONS

    if( EnvGetConserveMemory(theEnv) == FALSE )
    {
        SetDefclassPPForm((void *)cls, core_copy_pp_buffer(theEnv));
    }

#endif

#if DEFMODULE_CONSTRUCT

    /* =========================================
     *  Create a bitmap indicating whether this
     *  class is in scope or not for every module
     *  ========================================= */
    CreateClassScopeMap(theEnv, cls);

#endif

    /* ==============================================
     *  Define get- and put- handlers for public slots
     *  ============================================== */
    CreatePublicSlotMessageHandlers(theEnv, cls);
}

/*******************************************************
 *  NAME         : BuildSubclassLinks
 *  DESCRIPTION  : Follows the list of superclasses
 *              for a class and puts the class in
 *              each of the superclasses' subclass
 *              list.
 *  INPUTS       : The address of the class
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : The subclass lists for every superclass
 *              are modified.
 *  NOTES        : Assumes the superclass list is formed.
 *******************************************************/
static void BuildSubclassLinks(void *theEnv, DEFCLASS *cls)
{
    long i;

    for( i = 0 ; i < cls->directSuperclasses.classCount ; i++ )
    {
        AddClassLink(theEnv, &cls->directSuperclasses.classArray[i]->directSubclasses, cls, -1);
    }
}

/**********************************************************
 *  NAME         : FormInstanceTemplate
 *  DESCRIPTION  : Forms a contiguous array of instance
 *               slots for use in creating instances later
 *              Also used in determining instance slot
 *               indices a priori during handler defns
 *  INPUTS       : The class
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Contiguous array of instance slots formed
 *  NOTES        : None
 **********************************************************/
static void FormInstanceTemplate(void *theEnv, DEFCLASS *cls)
{
    TEMP_SLOT_LINK *islots = NULL, *stmp;
    short scnt = 0;
    long i;

    /* ========================
     *  Get direct class's slots
     *  ======================== */
    islots = MergeSlots(theEnv, islots, cls, &scnt, DIRECT);

    /* ===================================================================
     *  Get all inherited slots - a more specific slot takes precedence
     *  over more general, i.e. the first class in the precedence list with
     *  a particular slot gets to specify its default value
     *  =================================================================== */
    for( i = 1 ; i < cls->allSuperclasses.classCount ; i++ )
    {
        islots = MergeSlots(theEnv, islots, cls->allSuperclasses.classArray[i], &scnt, INHERIT);
    }

    /* ===================================================
     *  Allocate a contiguous array to store all the slots.
     *  =================================================== */
    cls->instanceSlotCount = scnt;
    cls->localInstanceSlotCount = 0;

    if( scnt > 0 )
    {
        cls->instanceTemplate = (SLOT_DESC **)core_mem_alloc_no_init(theEnv, (scnt * sizeof(SLOT_DESC *)));
    }

    for( i = 0 ; i < scnt ; i++ )
    {
        stmp = islots;
        islots = islots->nxt;
        cls->instanceTemplate[i] = stmp->desc;

        if( stmp->desc->shared == 0 )
        {
            cls->localInstanceSlotCount++;
        }

        core_mem_return_struct(theEnv, tempSlotLink, stmp);
    }
}

/**********************************************************
 *  NAME         : FormSlotNameMap
 *  DESCRIPTION  : Forms a mapping of the slot name ids into
 *              the instance template.  Given the slot
 *              name id, this map provides a much faster
 *              lookup of a slot.  The id is stored
 *              statically in object patterns and can
 *              be looked up via a hash table at runtime
 *              as well.
 *  INPUTS       : The class
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Contiguous array of integers formed
 *              The position in the array corresponding
 *              to a slot name id holds an the index
 *              into the instance template array holding
 *              the slot
 *              The max slot name id for the class is
 *              also stored to make deletion of the slots
 *              easier
 *  NOTES        : Assumes the instance template has already
 *              been formed
 **********************************************************/
static void FormSlotNameMap(void *theEnv, DEFCLASS *cls)
{
    long i;

    cls->maxSlotNameID = 0;
    cls->slotNameMap = NULL;

    if( cls->instanceSlotCount == 0 )
    {
        return;
    }

    for( i = 0 ; i < cls->instanceSlotCount ; i++ )
    {
        if( cls->instanceTemplate[i]->slotName->id > cls->maxSlotNameID )
        {
            cls->maxSlotNameID = cls->instanceTemplate[i]->slotName->id;
        }
    }

    cls->slotNameMap = (unsigned *)core_mem_alloc_no_init(theEnv, (sizeof(unsigned) * (cls->maxSlotNameID + 1)));

    for( i = 0 ; i <= cls->maxSlotNameID ; i++ )
    {
        cls->slotNameMap[i] = 0;
    }

    for( i = 0 ; i < cls->instanceSlotCount ; i++ )
    {
        cls->slotNameMap[cls->instanceTemplate[i]->slotName->id] = i + 1;
    }
}

/********************************************************************
 * NAME         : MergeSlots
 * DESCRIPTION  : Adds non-duplicate slots to list and increments
 *               slot count for the class instance template
 * INPUTS       : 1) The old slot list
 *             2) The address of class containing new slots
 *             3) Caller's buffer for # of slots
 *             4) A flag indicating whether the new list of slots
 *                is from the direct parent-class or not.
 * RETURNS      : The address of the new expanded list, or NULL
 *               for an empty list
 * SIDE EFFECTS : The list is expanded
 *             Caller's slot count is adjusted.
 * NOTES        : Lists are assumed to contain no duplicates
 *******************************************************************/
static TEMP_SLOT_LINK *MergeSlots(void *theEnv, TEMP_SLOT_LINK *old, DEFCLASS *cls, short *scnt, int src)
{
    TEMP_SLOT_LINK *cur, *tmp;
    register int i;
    SLOT_DESC *newSlot;

    /* ======================================
     *  Process the slots in reverse order
     *  since we are pushing them onto a stack
     *  ====================================== */
    for( i = (int)(cls->slotCount - 1) ; i >= 0 ; i-- )
    {
        newSlot = &cls->slots[i];

        /* ==========================================
         *  A class can prevent it slots from being
         *  propagated to all but its direct instances
         *  ========================================== */
        if((newSlot->noInherit == 0) ? TRUE : (src == DIRECT))
        {
            cur = old;

            while((cur != NULL) ? (newSlot->slotName != cur->desc->slotName) : FALSE )
            {
                cur = cur->nxt;
            }

            if( cur == NULL )
            {
                tmp = core_mem_get_struct(theEnv, tempSlotLink);
                tmp->desc = newSlot;
                tmp->nxt = old;
                old = tmp;
                (*scnt)++;
            }
        }
    }

    return(old);
}

/***********************************************************************
 *  NAME         : PackSlots
 *  DESCRIPTION  : Groups class-slots into a contiguous array
 *               "slots" field points to array
 *               "slotCount" field set
 *  INPUTS       : 1) The class
 *              2) The list of slots
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Temporary list deallocated, contiguous array allocated,
 *                and nxt pointers linked
 *              Class pointer set for slots
 *  NOTES        : Assumes class->slotCount == 0 && class->slots == NULL
 ***********************************************************************/
static void PackSlots(void *theEnv, DEFCLASS *cls, TEMP_SLOT_LINK *slots)
{
    TEMP_SLOT_LINK *stmp, *sprv;
    long i;

    stmp = slots;

    while( stmp != NULL )
    {
        stmp->desc->cls = cls;
        cls->slotCount++;
        stmp = stmp->nxt;
    }

    cls->slots = (SLOT_DESC *)core_mem_alloc_no_init(theEnv, (sizeof(SLOT_DESC) * cls->slotCount));
    stmp = slots;

    for( i = 0 ; i < cls->slotCount ; i++ )
    {
        sprv = stmp;
        stmp = stmp->nxt;
        core_mem_copy_memory(SLOT_DESC, 1, &(cls->slots[i]), sprv->desc);
        cls->slots[i].sharedValue.desc = &(cls->slots[i]);
        cls->slots[i].sharedValue.value = NULL;
        core_mem_return_struct(theEnv, slotDescriptor, sprv->desc);
        core_mem_return_struct(theEnv, tempSlotLink, sprv);
    }
}

#if DEFMODULE_CONSTRUCT

/********************************************************
 *  NAME         : CreateClassScopeMap
 *  DESCRIPTION  : Creates a bitmap where each bit position
 *              corresponds to a module id. If the bit
 *              is set, the class is in scope for that
 *              module, otherwise it is not.
 *  INPUTS       : The class
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Scope bitmap created and attached
 *  NOTES        : Uses lookup_imported_construct()
 ********************************************************/
static void CreateClassScopeMap(void *theEnv, DEFCLASS *theDefclass)
{
    unsigned scopeMapSize;
    char *scopeMap;
    char *className;
    struct module_definition *matchModule,
    *module_def;
    int moduleID, count;

    className = to_string(theDefclass->header.name);
    matchModule = theDefclass->header.my_module->module_def;

    scopeMapSize = (sizeof(char) * ((GetNumberOfDefmodules(theEnv) / BITS_PER_BYTE) + 1));
    scopeMap = (char *)core_mem_alloc_no_init(theEnv, scopeMapSize);

    init_bitmap((void *)scopeMap, scopeMapSize);
    save_current_module(theEnv);

    for( module_def = (struct module_definition *)get_next_module(theEnv, NULL) ;
         module_def != NULL ;
         module_def = (struct module_definition *)get_next_module(theEnv, (void *)module_def))
    {
        set_current_module(theEnv, (void *)module_def);
        moduleID = (int)module_def->id;

        if( lookup_imported_construct(theEnv, "defclass", matchModule,
                                  className, &count, TRUE, NULL) != NULL )
        {
            core_bitmap_set(scopeMap, moduleID);
        }
    }

    restore_current_module(theEnv);
    theDefclass->scopeMap = (BITMAP_HN *)store_bitmap(theEnv, scopeMap, scopeMapSize);
    inc_bitmap_count(theDefclass->scopeMap);
    core_mem_release(theEnv, (void *)scopeMap, scopeMapSize);
}

#endif

/*****************************************************************************
 * NAME         : CreatePublicSlotMessageHandlers
 * DESCRIPTION  : Creates a get-<slot-name> and
 *             put-<slot-name> handler for every
 *             public slot in a class.
 *
 *             The syntax of the message-handlers
 *             created are:
 *
 *             (defmessage-handler <class> get-<slot-name> primary ()
 *                ?self:<slot-name>)
 *
 *             For single-field slots:
 *
 *             (defmessage-handler <class> put-<slot-name> primary (?value)
 *                (bind ?self:<slot-name> ?value))
 *
 *             For list slots:
 *
 *             (defmessage-handler <class> put-<slot-name> primary ($?value)
 *                (bind ?self:<slot-name> ?value))
 * INPUTS       : The defclass
 * RETURNS      : Nothing useful
 * SIDE EFFECTS : Message-handlers created
 * NOTES        : None
 ******************************************************************************/
static void CreatePublicSlotMessageHandlers(void *theEnv, DEFCLASS *theDefclass)
{
    long i;
    register SLOT_DESC *sd;

    for( i = 0 ; i < theDefclass->slotCount ; i++ )
    {
        sd = &theDefclass->slots[i];
        CreateGetAndPutHandlers(theEnv, sd);
    }

    for( i = 0 ; i < theDefclass->handlerCount ; i++ )
    {
        theDefclass->handlers[i].system = TRUE;
    }
}

#endif
