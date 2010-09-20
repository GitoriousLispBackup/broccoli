/* Purpose: Provides routines for parsing module/construct
 *   names and searching through modules for specific
 *   constructs.                                             */

#define __MODULES_QUERY_SOURCE__

#include "setup.h"

#include "core_memory.h"
#include "router.h"
#include "core_environment.h"
#include "sysdep.h"

#include "parser_modules.h"
#include "modules_query.h"

/**************************************
 * LOCAL FUNCTION PROTOTYPES
 ***************************************/

static void                      *SearchImportedConstructModules(void *, struct atom_hash_node *, struct module_definition *, struct module_item *, struct atom_hash_node *, int *, int, struct module_definition *);

/*******************************************************************
 * find_module_separator: Finds the :: separator which delineates the
 *   boundary between a module name and a construct name. The value
 *   zero is returned if the separator is not found, otherwise the
 *   position of the second colon within the string is returned.
 ********************************************************************/
unsigned find_module_separator(char *theString)
{
    unsigned i, foundColon;

    for( i = 0, foundColon = FALSE; theString[i] != EOS; i++ )
    {
        if( theString[i] == ':' )
        {
            if( foundColon )
            {
                return(i);
            }

            foundColon = TRUE;
        }
        else
        {
            foundColon = FALSE;
        }
    }

    return(FALSE);
}

/******************************************************************
 * garner_module_name: Given the position of the :: separator and a
 *   module/construct name joined using the separator, returns a
 *   symbol reference to the module name (or NULL if a module name
 *   cannot be extracted).
 *******************************************************************/
ATOM_HN *garner_module_name(void *env, unsigned thePosition, char *theString)
{
    char *newString;
    ATOM_HN *ret;

    /*=============================================
     * Return NULL if the :: is in a position such
     * that a module name can't be extracted.
     *=============================================*/

    if( thePosition <= 1 )
    {
        return(NULL);
    }

    /*==========================================
     * Allocate storage for a temporary string.
     *==========================================*/

    newString = (char *)core_mem_alloc_no_init(env, thePosition);

    /*======================================================
     * Copy the entire module/construct name to the string.
     *======================================================*/

    sysdep_strncpy(newString, theString,
                   (STD_SIZE)thePosition - 1);

    /*========================================================
     * Place an end of string marker where the :: is located.
     *========================================================*/

    newString[thePosition - 1] = EOS;

    /*=====================================================
     * Add the module name (the truncated module/construct
     * name) to the symbol table.
     *=====================================================*/

    ret = (ATOM_HN *)store_atom(env, newString);

    /*=============================================
     * Return the storage of the temporary string.
     *=============================================*/

    core_mem_release(env, newString, thePosition);

    /*=============================================
     * Return a pointer to the module name symbol.
     *=============================================*/

    return(ret);
}

/*******************************************************************
 * garner_construct_name: Given the position of the :: separator and
 *   a module/construct name joined using the separator, returns a
 *   symbol reference to the construct name (or NULL if a construct
 *   name cannot be extracted).
 ********************************************************************/
ATOM_HN *garner_construct_name(void *env, unsigned thePosition, char *theString)
{
    size_t theLength;
    char *newString;
    ATOM_HN *ret;

    /*======================================
     * Just return the string if it doesn't
     * contain the :: symbol.
     *======================================*/

    if( thePosition == 0 )
    {
        return((ATOM_HN *)store_atom(env, theString));
    }

    /*=====================================
     * Determine the length of the string.
     *=====================================*/

    theLength = strlen(theString);

    /*=================================================
     * Return NULL if the :: is at the very end of the
     * string (and thus there is no construct name).
     *=================================================*/

    if( theLength <= (thePosition + 1))
    {
        return(NULL);
    }

    /*====================================
     * Allocate a temporary string large
     * enough to hold the construct name.
     *====================================*/

    newString = (char *)core_mem_alloc_no_init(env, theLength - thePosition);

    /*================================================
     * Copy the construct name portion of the
     * module/construct name to the temporary string.
     *================================================*/

    sysdep_strncpy(newString, &theString[thePosition + 1],
                   (STD_SIZE)theLength - thePosition);

    /*=============================================
     * Add the construct name to the symbol table.
     *=============================================*/

    ret = (ATOM_HN *)store_atom(env, newString);

    /*=============================================
     * Return the storage of the temporary string.
     *=============================================*/

    core_mem_release(env, newString, theLength - thePosition);

    /*================================================
     * Return a pointer to the construct name symbol.
     *================================================*/

    return(ret);
}

/***************************************************
 * garner_module_reference: Extracts both the
 *   module and construct name from a string. Sets
 *   the current module to the specified module.
 ****************************************************/
char *garner_module_reference(void *env, char *theName)
{
    unsigned separatorPosition;
    ATOM_HN *moduleName, *shortName;
    struct module_definition *theModule;

    /*========================
     * Find the :: separator.
     *========================*/

    separatorPosition = find_module_separator(theName);

    if( !separatorPosition )
    {
        return(theName);
    }

    /*==========================
     * Extract the module name.
     *==========================*/

    moduleName = garner_module_name(env, separatorPosition, theName);

    if( moduleName == NULL )
    {
        return(NULL);
    }

    /*====================================
     * Check to see if the module exists.
     *====================================*/

    theModule = (struct module_definition *)lookup_module(env, to_string(moduleName));

    if( theModule == NULL )
    {
        return(NULL);
    }

    /*============================
     * Change the current module.
     *============================*/

    set_current_module(env, (void *)theModule);

    /*=============================
     * Extract the construct name.
     *=============================*/

    shortName = garner_construct_name(env, separatorPosition, theName);

    if( shortName == NULL )
    {
        return(NULL);
    }

    return(to_string(shortName));
}

/***********************************************************
 * lookup_imported_construct: High level routine which searches
 *   a module and other modules from which it imports
 *   constructs for a specified construct.
 ************************************************************/
void *lookup_imported_construct(void *env, char *constructName, struct module_definition *matchModule, char *findName, int *count, int searchCurrent, struct module_definition *notYetDefinedInModule)
{
    void *rv;
    struct module_item *theModuleItem;

    /*=============================================
     * Set the number of references found to zero.
     *=============================================*/

    *count = 0;

    /*===============================
     * The :: should not be included
     * in the construct's name.
     *===============================*/

    if( find_module_separator(findName))
    {
        return(NULL);
    }

    /*=============================================
     * Remember the current module since we'll be
     * changing it during the search and will want
     * to restore it once the search is completed.
     *=============================================*/

    save_current_module(env);

    /*==========================================
     * Find the module related access functions
     * for the construct type being sought.
     *==========================================*/

    if((theModuleItem = lookup_module_item(env, constructName)) == NULL )
    {
        restore_current_module(env);
        return(NULL);
    }

    /*===========================================
     * If the construct type doesn't have a find
     * function, then we can't look for it.
     *===========================================*/

    if( theModuleItem->fn_find == NULL )
    {
        restore_current_module(env);
        return(NULL);
    }

    /*==================================
     * Initialize the search by marking
     * all modules as unvisited.
     *==================================*/

    set_module_unvisited(env);

    /*===========================
     * Search for the construct.
     *===========================*/

    rv = SearchImportedConstructModules(env, (ATOM_HN *)store_atom(env, constructName),
                                        matchModule, theModuleItem,
                                        (ATOM_HN *)store_atom(env, findName), count,
                                        searchCurrent, notYetDefinedInModule);

    /*=============================
     * Restore the current module.
     *=============================*/

    restore_current_module(env);

    /*====================================
     * Return a pointer to the construct.
     *====================================*/

    return(rv);
}

/********************************************************
 * error_ambiguous_reference: Error message printed
 *   when a reference to a specific construct can be
 *   imported from more than one module.
 *********************************************************/
void error_ambiguous_reference(void *env, char *constructName, char *findName)
{
    print_router(env, WERROR, "Ambiguous reference to ");
    print_router(env, WERROR, constructName);
    print_router(env, WERROR, " ");
    print_router(env, WERROR, findName);
    print_router(env, WERROR, ".\n");
}

/***************************************************
 * set_module_unvisited: Used for initializing a
 *   search through the module heirarchies. Sets
 *   the visited flag of each module to FALSE.
 ****************************************************/
void set_module_unvisited(void *env)
{
    struct module_definition *theModule;

    get_module_data(env)->current_submodule->visited = FALSE;

    for( theModule = (struct module_definition *)get_next_module(env, NULL);
         theModule != NULL;
         theModule = (struct module_definition *)get_next_module(env, theModule))
    {
        theModule->visited = FALSE;
    }
}

/**********************************************************
 * SearchImportedConstructModules: Low level routine which
 *   searches a module and other modules from which it
 *   imports constructs for a specified construct.
 ***********************************************************/
static void *SearchImportedConstructModules(void *env, struct atom_hash_node *constructType, struct module_definition *matchModule, struct module_item *theModuleItem, struct atom_hash_node *findName, int *count, int searchCurrent, struct module_definition *notYetDefinedInModule)
{
    struct module_definition *theModule;
    struct portable_item *theImportList, *theExportList;
    void *rv, *arv = NULL;
    int searchModule, exported;
    struct module_definition *currentModule;

    /*=========================================
     * Start the search in the current module.
     * If the current module has already been
     * visited, then return.
     *=========================================*/

    currentModule = ((struct module_definition *)get_current_module(env));

    if( currentModule->visited )
    {
        return(NULL);
    }

    /*=======================================================
     * The searchCurrent flag indicates whether the current
     * module should be included in the search. In addition,
     * if matchModule is non-NULL, the current module will
     * only be searched if it is the specific module from
     * which we want the construct imported.
     *=======================================================*/

    if((searchCurrent) &&
       ((matchModule == NULL) || (currentModule == matchModule)))
    {
        /*===============================================
         * Look for the construct in the current module.
         *===============================================*/

        rv = (*theModuleItem->fn_find)(env, to_string(findName));

        /*========================================================
         * If we're in the process of defining the construct in
         * the module we're searching then go ahead and increment
         * the count indicating the number of modules in which
         * the construct was found.
         *========================================================*/

        if( notYetDefinedInModule == currentModule )
        {
            (*count)++;
            arv = rv;
        }

        /*=========================================================
         * Otherwise, if the construct is in the specified module,
         * increment the count only if the construct actually
         * belongs to the module. [Some constructs, like the COOL
         * system classes, can be found in any module, but they
         * actually belong to the MAIN module.]
         *=========================================================*/

        else if( rv != NULL )
        {
            if(((struct construct_metadata *)rv)->my_module->module_def == currentModule )
            {
                (*count)++;
            }

            arv = rv;
        }
    }

    /*=====================================
     * Mark the current module as visited.
     *=====================================*/

    currentModule->visited = TRUE;

    /*===================================
     * Search through all of the modules
     * imported by the current module.
     *===================================*/

    theModule = ((struct module_definition *)get_current_module(env));
    theImportList = theModule->imports;

    while( theImportList != NULL )
    {
        /*===================================================
         * Determine if the module should be searched (based
         * upon whether the entire module, all constructs of
         * a specific type, or specifically named constructs
         * are imported).
         *===================================================*/

        searchModule = FALSE;

        if((theImportList->type == NULL) ||
           (theImportList->type == constructType))
        {
            if((theImportList->name == NULL) ||
               (theImportList->name == findName))
            {
                searchModule = TRUE;
            }
        }

        /*=================================
         * Determine if the module exists.
         *=================================*/

        if( searchModule )
        {
            theModule = (struct module_definition *)
                        lookup_module(env, to_string(theImportList->parent_module));

            if( theModule == NULL )
            {
                searchModule = FALSE;
            }
        }

        /*=======================================================
         * Determine if the construct is exported by the module.
         *=======================================================*/

        if( searchModule )
        {
            exported = FALSE;
            theExportList = theModule->exports;

            while((theExportList != NULL) && (!exported))
            {
                if((theExportList->type == NULL) ||
                   (theExportList->type == constructType))
                {
                    if((theExportList->name == NULL) ||
                       (theExportList->name == findName))
                    {
                        exported = TRUE;
                    }
                }

                theExportList = theExportList->next;
            }

            if( !exported )
            {
                searchModule = FALSE;
            }
        }

        /*=================================
         * Search in the specified module.
         *=================================*/

        if( searchModule )
        {
            set_current_module(env, (void *)theModule);

            if((rv = SearchImportedConstructModules(env, constructType, matchModule,
                                                    theModuleItem, findName,
                                                    count, TRUE,
                                                    notYetDefinedInModule)) != NULL )
            {
                arv = rv;
            }
        }

        /*====================================
         * Move on to the next imported item.
         *====================================*/

        theImportList = theImportList->next;
    }

    /*=========================
     * Return a pointer to the
     * last construct found.
     *=========================*/

    return(arv);
}

/*************************************************************
 * is_construct_exported: Returns TRUE if the specified construct
 *   is exported from the specified module.
 **************************************************************/
BOOLEAN is_construct_exported(void *env, char *constructTypeStr, struct atom_hash_node *moduleName, struct atom_hash_node *findName)
{
    struct atom_hash_node *constructType;
    struct module_definition *theModule;
    struct portable_item *theExportList;

    constructType = lookup_atom(env, constructTypeStr);
    theModule = (struct module_definition *)lookup_module(env, to_string(moduleName));

    if((constructType == NULL) || (theModule == NULL) || (findName == NULL))
    {
        return(FALSE);
    }

    theExportList = theModule->exports;

    while( theExportList != NULL )
    {
        if((theExportList->type == NULL) ||
           (theExportList->type == constructType))
        {
            if((theExportList->name == NULL) ||
               (theExportList->name == findName))
            {
                return(TRUE);
            }
        }

        theExportList = theExportList->next;
    }

    return(FALSE);
}
