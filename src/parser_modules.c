/* Purpose: Parses a module_definition construct.                    */

#define __PARSER_MODULES_SOURCE__

#include "setup.h"

#if DEFMODULE_CONSTRUCT

#include <stdio.h>
#include <string.h>
#define _STDIO_INCLUDED_

#include "core_memory.h"
#include "constant.h"
#include "router.h"
#include "core_functions.h"
#include "core_arguments.h"
#include "parser_constructs.h"
#include "core_constructs.h"
#include "modules_query.h"
#include "core_gc.h"
#include "core_environment.h"

#include "parser_modules.h"

/**************************************
 * LOCAL FUNCTION PROTOTYPES
 ***************************************/

static int     ParsePortSpecifications(void *, char *, struct token *, struct module_definition *);
static int     ParseImportSpec(void *, char *, struct token *, struct module_definition *);
static int     ParseExportSpec(void *, char *, struct token *, struct module_definition *, struct module_definition *);
static BOOLEAN DeleteDefmodule(void *, void *);
static int     FindMultiImportConflict(void *, struct module_definition *);
static void    NotExportedErrorMessage(void *, char *, char *, char *);

/********************************************
 * GetNumberOfDefmodules: Returns the number
 *   of defmodules currently defined.
 *********************************************/
long GetNumberOfDefmodules(void *env)
{
    return(get_module_data(env)->submodules_count);
}

/*****************************************
 * SetNumberOfDefmodules: Sets the number
 *   of defmodules currently defined.
 ******************************************/
void SetNumberOfDefmodules(void *env, long value)
{
    get_module_data(env)->submodules_count = value;
}

/***************************************************
 * add_module_change_listener: Adds a function to
 *   the list of functions that are to be called
 *   after a module change occurs.
 ****************************************************/
void AddAfterModuleDefinedFunction(void *env, char *name, void (*func)(void *), int priority)
{
    get_module_data(env)->module_define_listeners =
        core_gc_add_call_function(env, name, priority, func, get_module_data(env)->module_define_listeners, TRUE);
}

/*****************************************************
 * AddPortConstructItem: Adds an item to the list of
 *   items that can be imported/exported by a module.
 ******************************************************/
void AddPortConstructItem(void *env, char *theName, int theType)
{
    struct portConstructItem *newItem;

    newItem = core_mem_get_struct(env, portConstructItem);
    newItem->construct_name = theName;
    newItem->typeExpected = theType;
    newItem->next = get_module_data(env)->exportable_items;
    get_module_data(env)->exportable_items = newItem;
}

/*****************************************************
 * ParseDefmodule: Coordinates all actions necessary
 *   for the parsing and creation of a module_definition into
 *   the current environment.
 ******************************************************/
int ParseDefmodule(void *env, char *readSource)
{
    ATOM_HN *defmoduleName;
    struct module_definition *newDefmodule;
    struct token inputToken;
    int i;
    struct module_item *theItem;
    struct portable_item *portSpecs, *nextSpec;
    struct module_header *theHeader;
    struct core_gc_call_function *defineFunctions;
    struct module_definition *redefiningMainModule = NULL;
    int parseError;
    struct portable_item *oldImportList = NULL, *oldExportList = NULL;
    short overwrite = FALSE;

    /*================================================
     * Flush the buffer which stores the pretty print
     * representation for a module.  Add the already
     * parsed keyword module_definition to this buffer.
     *================================================*/

    core_set_pp_buffer_status(env, ON);
    core_flush_pp_buffer(env);
    core_pp_indent_depth(env, 3);
    core_save_pp_buffer(env, "(defmodule ");

    /*===============================
     * Modules cannot be loaded when
     * a binary load is in effect.
     *===============================*/


    /*=====================================================
     * Parse the name and comment fields of the module_definition.
     * Remove the module_definition if it already exists.
     *=====================================================*/

    defmoduleName = parse_construct_head(env, readSource, &inputToken, "defmodule",
                                         lookup_module, DeleteDefmodule, "+",
                                         TRUE, TRUE, FALSE);

    if( defmoduleName == NULL )
    {
        return(TRUE);
    }

    if( strcmp(to_string(defmoduleName), "MAIN") == 0 )
    {
        redefiningMainModule = (struct module_definition *)lookup_module(env, "MAIN");
    }

    /*==============================================
     * Create the module_definition structure if necessary.
     *==============================================*/

    if( redefiningMainModule == NULL )
    {
        newDefmodule = (struct module_definition *)lookup_module(env, to_string(defmoduleName));

        if( newDefmodule )
        {
            overwrite = TRUE;
        }
        else
        {
            newDefmodule = core_mem_get_struct(env, module_definition);
            newDefmodule->name = defmoduleName;
            newDefmodule->ext_data = NULL;
            newDefmodule->next = NULL;
        }
    }
    else
    {
        overwrite = TRUE;
        newDefmodule = redefiningMainModule;
    }

    if( overwrite )
    {
        oldImportList = newDefmodule->imports;
        oldExportList = newDefmodule->exports;
    }

    newDefmodule->imports = NULL;
    newDefmodule->exports = NULL;

    /*===================================
     * Finish parsing the module_definition (its
     * import/export specifications).
     *===================================*/

    parseError = ParsePortSpecifications(env, readSource, &inputToken, newDefmodule);

    /*====================================
     * Check for import/export conflicts.
     *====================================*/

    if( !parseError )
    {
        parseError = FindMultiImportConflict(env, newDefmodule);
    }

    /*======================================================
     * If an error occured in parsing or an import conflict
     * was detected, abort the definition of the module_definition.
     * If we're only checking syntax, then we want to exit
     * at this point as well.
     *======================================================*/

    if( parseError || core_construct_data(env)->check_syntax )
    {
        while( newDefmodule->imports != NULL )
        {
            nextSpec = newDefmodule->imports->next;
            core_mem_return_struct(env, portable_item, newDefmodule->imports);
            newDefmodule->imports = nextSpec;
        }

        while( newDefmodule->exports != NULL )
        {
            nextSpec = newDefmodule->exports->next;
            core_mem_return_struct(env, portable_item, newDefmodule->exports);
            newDefmodule->exports = nextSpec;
        }

        if((redefiningMainModule == NULL) && (!overwrite))
        {
            core_mem_return_struct(env, module_definition, newDefmodule);
        }

        if( overwrite )
        {
            newDefmodule->imports = oldImportList;
            newDefmodule->exports = oldExportList;
        }

        if( parseError )
        {
            return(TRUE);
        }

        return(FALSE);
    }

    /*===============================================
     * Increment the symbol table counts for symbols
     * used in the module_definition data structures.
     *===============================================*/

    if( redefiningMainModule == NULL )
    {
        inc_atom_count(newDefmodule->name);
    }
    else
    {
        if((newDefmodule->imports != NULL) ||
           (newDefmodule->exports != NULL))
        {
            get_module_data(env)->redefinable = FALSE;
        }
    }

    for( portSpecs = newDefmodule->imports; portSpecs != NULL; portSpecs = portSpecs->next )
    {
        if( portSpecs->parent_module != NULL )
        {
            inc_atom_count(portSpecs->parent_module);
        }

        if( portSpecs->type != NULL )
        {
            inc_atom_count(portSpecs->type);
        }

        if( portSpecs->construct_name != NULL )
        {
            inc_atom_count(portSpecs->construct_name);
        }
    }

    for( portSpecs = newDefmodule->exports; portSpecs != NULL; portSpecs = portSpecs->next )
    {
        if( portSpecs->parent_module != NULL )
        {
            inc_atom_count(portSpecs->parent_module);
        }

        if( portSpecs->type != NULL )
        {
            inc_atom_count(portSpecs->type);
        }

        if( portSpecs->construct_name != NULL )
        {
            inc_atom_count(portSpecs->construct_name);
        }
    }

    /*====================================================
     * Allocate storage for the module's construct lists.
     *====================================================*/

    if( redefiningMainModule != NULL )
    {                                   /* Do nothing */
    }
    else if( get_module_data(env)->items_count == 0 )
    {
        newDefmodule->items = NULL;
    }
    else
    {
        newDefmodule->items = (struct module_header **)core_mem_alloc_no_init(env, sizeof(void *) * get_module_data(env)->items_count);

        for( i = 0, theItem = get_module_data(env)->items;
             (i < get_module_data(env)->items_count) && (theItem != NULL);
             i++, theItem = theItem->next )
        {
            if( theItem->fn_allocate == NULL )
            {
                newDefmodule->items[i] = NULL;
            }
            else
            {
                newDefmodule->items[i] = (struct module_header *)
                                         (*theItem->fn_allocate)(env);
                theHeader = (struct module_header *)newDefmodule->items[i];
                theHeader->module_def = newDefmodule;
                theHeader->first_item = NULL;
                theHeader->last_item = NULL;
            }
        }
    }

    /*=======================================
     * Save the pretty print representation.
     *=======================================*/

    core_save_pp_buffer(env, "\n");

    if( EnvGetConserveMemory(env) == TRUE )
    {
        newDefmodule->pp = NULL;
    }
    else
    {
        newDefmodule->pp = core_copy_pp_buffer(env);
    }

    /*==============================================
     * Add the module_definition to the list of defmodules.
     *==============================================*/

    if( redefiningMainModule == NULL )
    {
        if( get_module_data(env)->last_submodule == NULL )
        {
            get_module_data(env)->submodules = newDefmodule;
        }
        else
        {
            get_module_data(env)->last_submodule->next = newDefmodule;
        }

        get_module_data(env)->last_submodule = newDefmodule;
        newDefmodule->id = get_module_data(env)->submodules_count++;
    }

    set_current_module(env, (void *)newDefmodule);

    /*=========================================
     * Call any functions required by other
     * constructs when a new module is defined
     *=========================================*/

    for( defineFunctions = get_module_data(env)->module_define_listeners;
         defineFunctions != NULL;
         defineFunctions = defineFunctions->next )
    {
        (*(void(*) (void *))defineFunctions->func)(env);
    }

    /*===============================================
     * Defmodule successfully parsed with no errors.
     *===============================================*/

    return(FALSE);
}

/************************************************************
 * DeleteDefmodule: Used by the parsing routine to determine
 *   if a module can be redefined. Only the MAIN module can
 *   be redefined (and it can only be redefined once).
 *************************************************************/
static BOOLEAN DeleteDefmodule(void *env, void *cons)
{
    if( strcmp(get_module_name(env, cons), "MAIN") == 0 )
    {
        return(get_module_data(env)->redefinable);
    }

    return(FALSE);
}

/********************************************************
 * ParsePortSpecifications: Parses the import and export
 *   specifications found in a module_definition construct.
 *********************************************************/
static int ParsePortSpecifications(void *env, char *readSource, struct token *theToken, struct module_definition *theDefmodule)
{
    int error;

    /*=============================
     * The import and export lists
     * are initially empty.
     *=============================*/

    theDefmodule->imports = NULL;
    theDefmodule->exports = NULL;

    /*==========================================
     * Parse import/export specifications until
     * a right parenthesis is encountered.
     *==========================================*/

    while( theToken->type != RPAREN )
    {
        /*========================================
         * Look for the opening left parenthesis.
         *========================================*/

        if( theToken->type != LPAREN )
        {
            error_syntax(env, "defmodule");
            return(TRUE);
        }

        /*====================================
         * Look for the import/export keyword
         * and call the appropriate functions
         * for parsing the specification.
         *====================================*/

        core_get_token(env, readSource, theToken);

        if( theToken->type != ATOM )
        {
            error_syntax(env, "defmodule");
            return(TRUE);
        }

        if( strcmp(to_string(theToken->value), "import") == 0 )
        {
            error = ParseImportSpec(env, readSource, theToken, theDefmodule);
        }
        else if( strcmp(to_string(theToken->value), "export") == 0 )
        {
            error = ParseExportSpec(env, readSource, theToken, theDefmodule, NULL);
        }
        else
        {
            error_syntax(env, "defmodule");
            return(TRUE);
        }

        if( error )
        {
            return(TRUE);
        }

        /*============================================
         * Begin parsing the next port specification.
         *============================================*/

        core_inject_ws_into_pp_buffer(env);
        core_get_token(env, readSource, theToken);

        if( theToken->type == RPAREN )
        {
            core_backup_pp_buffer(env);
            core_backup_pp_buffer(env);
            core_save_pp_buffer(env, ")");
        }
    }

    /*===================================
     * Return FALSE to indicate no error
     * occurred while parsing the
     * import/export specifications.
     *===================================*/

    return(FALSE);
}

/*********************************************************
 * ParseImportSpec: Parses import specifications found in
 *   a module_definition construct.
 *
 * <import-spec> ::= (import <module-name> <port-item>)
 *
 * <port-item>   ::= ?ALL |
 *                   ?NONE |
 *                   <construct-name> ?ALL |
 *                   <construct-name> ?NONE |
 *                   <construct-name> <names>*
 **********************************************************/
static int ParseImportSpec(void *env, char *readSource, struct token *theToken, struct module_definition *newModule)
{
    struct module_definition *module_def;
    struct portable_item *thePort, *oldImportSpec;
    int found, count;

    /*===========================
     * Look for the module name.
     *===========================*/

    core_save_pp_buffer(env, " ");

    core_get_token(env, readSource, theToken);

    if( theToken->type != ATOM )
    {
        error_syntax(env, "defmodule import specification");
        return(TRUE);
    }

    /*=====================================
     * Verify the existence of the module.
     *=====================================*/

    if((module_def = (struct module_definition *)
                     lookup_module(env, to_string(theToken->value))) == NULL )
    {
        error_lookup(env, "defmodule", to_string(theToken->value));
        return(TRUE);
    }

    /*========================================
     * If the specified module doesn't export
     * any constructs, then the import
     * specification is meaningless.
     *========================================*/

    if( module_def->exports == NULL )
    {
        NotExportedErrorMessage(env, get_module_name(env, module_def), NULL, NULL);
        return(TRUE);
    }

    /*==============================================
     * Parse the remaining portion of the import
     * specification and return if an error occurs.
     *==============================================*/

    oldImportSpec = newModule->imports;

    if( ParseExportSpec(env, readSource, theToken, newModule, module_def))
    {
        return(TRUE);
    }

    /*========================================================
     * If the ?NONE keyword was used with the import spec,
     * then no constructs were actually imported and the
     * import spec does not need to be checked for conflicts.
     *========================================================*/

    if( newModule->imports == oldImportSpec )
    {
        return(FALSE);
    }

    /*======================================================
     * Check to see if the construct being imported can be
     * by the specified module. This check exported doesn't
     * guarantee that a specific named construct actually
     * exists. It just checks that it could be exported if
     * it does exists.
     *======================================================*/

    if( newModule->imports->type != NULL )
    {
        /*=============================
         * Look for the construct in
         * the module that exports it.
         *=============================*/

        found = FALSE;

        for( thePort = module_def->exports;
             (thePort != NULL) && (!found);
             thePort = thePort->next )
        {
            if( thePort->type == NULL )
            {
                found = TRUE;
            }
            else if( thePort->type == newModule->imports->type )
            {
                if( newModule->imports->construct_name == NULL )
                {
                    found = TRUE;
                }
                else if( thePort->construct_name == NULL )
                {
                    found = TRUE;
                }
                else if( thePort->construct_name == newModule->imports->construct_name )
                {
                    found = TRUE;
                }
            }
        }

        /*=======================================
         * If it's not exported by the specified
         * module, print an error message.
         *=======================================*/

        if( !found )
        {
            if( newModule->imports->construct_name == NULL )
            {
                NotExportedErrorMessage(env, get_module_name(env, module_def),
                                        to_string(newModule->imports->type),
                                        NULL);
            }
            else
            {
                NotExportedErrorMessage(env, get_module_name(env, module_def),
                                        to_string(newModule->imports->type),
                                        to_string(newModule->imports->construct_name));
            }

            return(TRUE);
        }
    }

    /*======================================================
     * Verify that specific named constructs actually exist
     * and can be seen from the module importing them.
     *======================================================*/

    save_current_module(env);
    set_current_module(env, (void *)newModule);

    for( thePort = newModule->imports;
         thePort != NULL;
         thePort = thePort->next )
    {
        if((thePort->type == NULL) || (thePort->construct_name == NULL))
        {
            continue;
        }

        module_def = (struct module_definition *)
                     lookup_module(env, to_string(thePort->parent_module));
        set_current_module(env, module_def);

        if( lookup_imported_construct(env, to_string(thePort->type), NULL,
                                      to_string(thePort->construct_name), &count,
                                      TRUE, FALSE) == NULL )
        {
            NotExportedErrorMessage(env, get_module_name(env, module_def),
                                    to_string(thePort->type),
                                    to_string(thePort->construct_name));
            restore_current_module(env);
            return(TRUE);
        }
    }

    restore_current_module(env);

    /*===============================================
     * The import list has been successfully parsed.
     *===============================================*/

    return(FALSE);
}

/*********************************************************
 * ParseExportSpec: Parses export specifications found in
 *   a module_definition construct. This includes parsing the
 *   remaining specification found in an import
 *   specification after the module name.
 **********************************************************/
static int ParseExportSpec(void *env, char *readSource, struct token *theToken, struct module_definition *newModule, struct module_definition *importModule)
{
    struct portable_item *newPort;
    ATOM_HN *cons, *parent_module;
    struct portConstructItem *thePortConstruct;
    char *errorMessage;

    /*===========================================
     * Set up some variables for error messages.
     *===========================================*/

    if( importModule != NULL )
    {
        errorMessage = "defmodule import specification";
        parent_module = importModule->name;
    }
    else
    {
        errorMessage = "defmodule export specification";
        parent_module = NULL;
    }

    /*=============================================
     * Handle the special variables ?ALL and ?NONE
     * in the import/export specification.
     *=============================================*/

    core_save_pp_buffer(env, " ");
    core_get_token(env, readSource, theToken);

    if( theToken->type == SCALAR_VARIABLE )
    {
        /*==============================
         * Check to see if the variable
         * is either ?ALL or ?NONE.
         *==============================*/

        if( strcmp(to_string(theToken->value), "ALL") == 0 )
        {
            newPort = (struct portable_item *)core_mem_get_struct(env, portable_item);
            newPort->parent_module = parent_module;
            newPort->type = NULL;
            newPort->construct_name = NULL;
            newPort->next = NULL;
        }
        else if( strcmp(to_string(theToken->value), "NONE") == 0 )
        {
            newPort = NULL;
        }
        else
        {
            error_syntax(env, errorMessage);
            return(TRUE);
        }

        /*=======================================================
         * The export/import specification must end with a right
         * parenthesis after ?ALL or ?NONE at this point.
         *=======================================================*/

        core_get_token(env, readSource, theToken);

        if( theToken->type != RPAREN )
        {
            if( newPort != NULL )
            {
                core_mem_return_struct(env, portable_item, newPort);
            }

            core_backup_pp_buffer(env);
            core_save_pp_buffer(env, " ");
            core_save_pp_buffer(env, theToken->pp);
            error_syntax(env, errorMessage);
            return(TRUE);
        }

        /*=====================================
         * Add the new specification to either
         * the import or export list.
         *=====================================*/

        if( newPort != NULL )
        {
            if( importModule != NULL )
            {
                newPort->next = newModule->imports;
                newModule->imports = newPort;
            }
            else
            {
                newPort->next = newModule->exports;
                newModule->exports = newPort;
            }
        }

        /*============================================
         * Return FALSE to indicate the import/export
         * specification was successfully parsed.
         *============================================*/

        return(FALSE);
    }

    /*========================================================
     * If the ?ALL and ?NONE keywords were not used, then the
     * token must be the name of an importable construct.
     *========================================================*/

    if( theToken->type != ATOM )
    {
        error_syntax(env, errorMessage);
        return(TRUE);
    }

    cons = (ATOM_HN *)theToken->value;

    if((thePortConstruct = ValidPortConstructItem(env, to_string(cons))) == NULL )
    {
        error_syntax(env, errorMessage);
        return(TRUE);
    }

    /*=============================================================
     * If the next token is the special variable ?ALL, then all
     * constructs of the specified type are imported/exported. If
     * the next token is the special variable ?NONE, then no
     * constructs of the specified type will be imported/exported.
     *=============================================================*/

    core_save_pp_buffer(env, " ");
    core_get_token(env, readSource, theToken);

    if( theToken->type == SCALAR_VARIABLE )
    {
        /*==============================
         * Check to see if the variable
         * is either ?ALL or ?NONE.
         *==============================*/

        if( strcmp(to_string(theToken->value), "ALL") == 0 )
        {
            newPort = (struct portable_item *)core_mem_get_struct(env, portable_item);
            newPort->parent_module = parent_module;
            newPort->type = cons;
            newPort->construct_name = NULL;
            newPort->next = NULL;
        }
        else if( strcmp(to_string(theToken->value), "NONE") == 0 )
        {
            newPort = NULL;
        }
        else
        {
            error_syntax(env, errorMessage);
            return(TRUE);
        }

        /*=======================================================
         * The export/import specification must end with a right
         * parenthesis after ?ALL or ?NONE at this point.
         *=======================================================*/

        core_get_token(env, readSource, theToken);

        if( theToken->type != RPAREN )
        {
            if( newPort != NULL )
            {
                core_mem_return_struct(env, portable_item, newPort);
            }

            core_backup_pp_buffer(env);
            core_save_pp_buffer(env, " ");
            core_save_pp_buffer(env, theToken->pp);
            error_syntax(env, errorMessage);
            return(TRUE);
        }

        /*=====================================
         * Add the new specification to either
         * the import or export list.
         *=====================================*/

        if( newPort != NULL )
        {
            if( importModule != NULL )
            {
                newPort->next = newModule->imports;
                newModule->imports = newPort;
            }
            else
            {
                newPort->next = newModule->exports;
                newModule->exports = newPort;
            }
        }

        /*============================================
         * Return FALSE to indicate the import/export
         * specification was successfully parsed.
         *============================================*/

        return(FALSE);
    }

    /*============================================
     * There must be at least one named construct
     * in the import/export list at this point.
     *============================================*/

    if( theToken->type == RPAREN )
    {
        error_syntax(env, errorMessage);
        return(TRUE);
    }

    /*=====================================
     * Read in the list of imported items.
     *=====================================*/

    while( theToken->type != RPAREN )
    {
        if( theToken->type != thePortConstruct->typeExpected )
        {
            error_syntax(env, errorMessage);
            return(TRUE);
        }

        /*========================================
         * Create the data structure to represent
         * the import/export specification for
         * the named construct.
         *========================================*/

        newPort = (struct portable_item *)core_mem_get_struct(env, portable_item);
        newPort->parent_module = parent_module;
        newPort->type = cons;
        newPort->construct_name = (ATOM_HN *)theToken->value;

        /*=====================================
         * Add the new specification to either
         * the import or export list.
         *=====================================*/

        if( importModule != NULL )
        {
            newPort->next = newModule->imports;
            newModule->imports = newPort;
        }
        else
        {
            newPort->next = newModule->exports;
            newModule->exports = newPort;
        }

        /*===================================
         * Move on to the next import/export
         * specification.
         *===================================*/

        core_save_pp_buffer(env, " ");
        core_get_token(env, readSource, theToken);
    }

    /*=============================
     * Fix up pretty print buffer.
     *=============================*/

    core_backup_pp_buffer(env);
    core_backup_pp_buffer(env);
    core_save_pp_buffer(env, ")");

    /*============================================
     * Return FALSE to indicate the import/export
     * specification was successfully parsed.
     *============================================*/

    return(FALSE);
}

/************************************************************
 * ValidPortConstructItem: Returns TRUE if a given construct
 *   name is in the list of constructs which can be exported
 *   and imported, otherwise FALSE is returned.
 *************************************************************/
struct portConstructItem *ValidPortConstructItem(void *env, char *theName)
{
    struct portConstructItem *theItem;

    for( theItem = get_module_data(env)->exportable_items;
         theItem != NULL;
         theItem = theItem->next )
    {
        if( strcmp(theName, theItem->construct_name) == 0 ) {return(theItem);}
    }

    return(NULL);
}

/**********************************************************
 * FindMultiImportConflict: Determines if a module imports
 *   the same named construct from more than one module
 *   (i.e. an ambiguous reference which is not allowed).
 ***********************************************************/
static int FindMultiImportConflict(void *env, struct module_definition *module_def)
{
    struct module_definition *testModule;
    int count;
    struct portConstructItem *thePCItem;
    struct construct *cons;
    void *theCItem;

    /*==========================
     * Save the current module.
     *==========================*/

    save_current_module(env);

    /*============================
     * Loop through every module.
     *============================*/

    for( testModule = (struct module_definition *)get_next_module(env, NULL);
         testModule != NULL;
         testModule = (struct module_definition *)get_next_module(env, testModule))
    {
        /*========================================
         * Loop through every construct type that
         * can be imported/exported by a module.
         *========================================*/

        for( thePCItem = get_module_data(env)->exportable_items;
             thePCItem != NULL;
             thePCItem = thePCItem->next )
        {
            set_current_module(env, (void *)testModule);

            /*=====================================================
             * Loop through every construct of the specified type.
             *=====================================================*/

            cons = core_lookup_construct_by_name(env, thePCItem->construct_name);

            for( theCItem = (*cons->next_item_geter)(env, NULL);
                 theCItem != NULL;
                 theCItem = (*cons->next_item_geter)(env, theCItem))
            {
                /*===============================================
                 * Check to see if the specific construct in the
                 * module can be imported with more than one
                 * reference into the module we're examining for
                 * ambiguous import  specifications.
                 *===============================================*/

                set_current_module(env, (void *)module_def);
                lookup_imported_construct(env, thePCItem->construct_name, NULL,
                                          to_string((*cons->namer)
                                                    ((struct construct_metadata *)theCItem)),
                                          &count, FALSE, NULL);

                if( count > 1 )
                {
                    error_import_export_conflict(env, "defmodule", get_module_name(env, module_def),
                                                 thePCItem->construct_name,
                                                 to_string((*cons->namer)
                                                           ((struct construct_metadata *)theCItem)));
                    restore_current_module(env);
                    return(TRUE);
                }

                set_current_module(env, (void *)testModule);
            }
        }
    }

    /*=============================
     * Restore the current module.
     *=============================*/

    restore_current_module(env);

    /*=======================================
     * Return FALSE to indicate no ambiguous
     * references were found.
     *=======================================*/

    return(FALSE);
}

/*****************************************************
 * NotExportedErrorMessage: Generalized error message
 *  for indicating that a construct type or specific
 *  named construct is not exported.
 ******************************************************/
static void NotExportedErrorMessage(void *env, char *module_def, char *cons, char *theName)
{
    error_print_id(env, "PARSER", 1, TRUE);
    print_router(env, WERROR, "Module ");
    print_router(env, WERROR, module_def);
    print_router(env, WERROR, " does not export ");

    if( cons == NULL )
    {
        print_router(env, WERROR, "any constructs");
    }
    else if( theName == NULL )
    {
        print_router(env, WERROR, "any ");
        print_router(env, WERROR, cons);
        print_router(env, WERROR, " constructs");
    }
    else
    {
        print_router(env, WERROR, "the ");
        print_router(env, WERROR, cons);
        print_router(env, WERROR, " ");
        print_router(env, WERROR, theName);
    }

    print_router(env, WERROR, ".\n");
}

/************************************************************
 * FindImportExportConflict: Determines if the definition of
 *   a construct would cause an import/export conflict. The
 *   construct is not yet defined when this function is
 *   called. TRUE is returned if an import/export conflicts
 *   is found, otherwise FALSE is returned.
 *************************************************************/
int FindImportExportConflict(void *env, char *construct_name, struct module_definition *matchModule, char *findName)
{
    struct module_definition *module_def;
    struct module_item *theModuleItem;
    int count;

    /*===========================================================
     * If the construct type can't be imported or exported, then
     * it's not possible to have an import/export conflict.
     *===========================================================*/

    if( ValidPortConstructItem(env, construct_name) == NULL )
    {
        return(FALSE);
    }

    /*============================================
     * There module name should already have been
     * separated fromthe construct's name.
     *============================================*/

    if( find_module_separator(findName))
    {
        return(FALSE);
    }

    /*===============================================================
     * The construct must be capable of being stored within a module
     * (this test should never fail). The construct must also have
     * a find function associated with it so we can actually look
     * for import/export conflicts.
     *===============================================================*/

    if((theModuleItem = lookup_module_item(env, construct_name)) == NULL )
    {
        return(FALSE);
    }

    if( theModuleItem->finder == NULL )
    {
        return(FALSE);
    }

    /*==========================
     * Save the current module.
     *==========================*/

    save_current_module(env);

    /*================================================================
     * Look at each module and count each definition of the specified
     * construct which is visible to the module. If more than one
     * definition is visible, then an import/export conflict exists
     * and TRUE is returned.
     *================================================================*/

    for( module_def = (struct module_definition *)get_next_module(env, NULL);
         module_def != NULL;
         module_def = (struct module_definition *)get_next_module(env, module_def))
    {
        set_current_module(env, (void *)module_def);

        lookup_imported_construct(env, construct_name, NULL, findName, &count, TRUE, matchModule);

        if( count > 1 )
        {
            restore_current_module(env);
            return(TRUE);
        }
    }

    /*==========================================
     * Restore the current module. No conflicts
     * were detected so FALSE is returned.
     *==========================================*/

    restore_current_module(env);
    return(FALSE);
}

#endif
