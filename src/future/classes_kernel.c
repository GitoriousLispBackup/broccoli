/* Purpose: Kernel Interface Commands for Object System       */

/* =========================================
 *****************************************
 *            EXTERNAL DEFINITIONS
 *  =========================================
 ***************************************** */

#include <string.h>

#include "setup.h"

#if OBJECT_SYSTEM

#include "core_arguments.h"
#include "funcs_class.h"
#include "classes_init.h"
#include "core_environment.h"
#include "modules_query.h"
#include "classes_methods_kernel.h"
#include "router.h"

#define __KERNEL_CLASS_SOURCE__
#include "classes_kernel.h"

/* =========================================
 *****************************************
 *   INTERNALLY VISIBLE FUNCTION HEADERS
 *  =========================================
 ***************************************** */

#if DEBUGGING_FUNCTIONS
static void SaveDefclass(void *, struct construct_metadata *, void *);
#endif
static char *GetClassDefaultsModeName(unsigned short);

/* =========================================
 *****************************************
 *       EXTERNALLY VISIBLE FUNCTIONS
 *  =========================================
 ***************************************** */

/*******************************************************************
 * NAME         : EnvFindDefclass
 * DESCRIPTION  : Looks up a specified class in the class hash table
 *             (Only looks in current or specified module)
 * INPUTS       : The name-string of the class (including module)
 * RETURNS      : The address of the found class, NULL otherwise
 * SIDE EFFECTS : None
 * NOTES        : None
 ******************************************************************/
globle void *EnvFindDefclass(void *theEnv, char *classAndModuleName)
{
    ATOM_HN *classSymbol = NULL;
    DEFCLASS *cls;
    struct module_definition *module_def = NULL;
    char *className;

    save_current_module(theEnv);
    className = garner_module_reference(theEnv, classAndModuleName);

    if( className != NULL )
    {
        classSymbol = lookup_atom(theEnv, garner_module_reference(theEnv, classAndModuleName));
        module_def = ((struct module_definition *)get_current_module(theEnv));
    }

    restore_current_module(theEnv);

    if( classSymbol == NULL )
    {
        return(NULL);
    }

    cls = DefclassData(theEnv)->ClassTable[HashClass(classSymbol)];

    while( cls != NULL )
    {
        if( cls->header.name == classSymbol )
        {
            if( cls->system || (cls->header.my_module->module_def == module_def))
            {
                return(cls->installed ? (void *)cls : NULL);
            }
        }

        cls = cls->nxtHash;
    }

    return(NULL);
}

/***************************************************
 *  NAME         : LookupDefclassByMdlOrScope
 *  DESCRIPTION  : Finds a class anywhere (if module
 *              is specified) or in current or
 *              imported modules
 *  INPUTS       : The class name
 *  RETURNS      : The class (NULL if not found)
 *  SIDE EFFECTS : Error message printed on
 *               ambiguous references
 *  NOTES        : Assumes no two classes of the same
 *              name are ever in the same scope
 ***************************************************/
globle DEFCLASS *LookupDefclassByMdlOrScope(void *theEnv, char *classAndModuleName)
{
    DEFCLASS *cls;
    char *className;
    ATOM_HN *classSymbol;
    struct module_definition *module_def;

    if( find_module_separator(classAndModuleName) == FALSE )
    {
        return(LookupDefclassInScope(theEnv, classAndModuleName));
    }

    save_current_module(theEnv);
    className = garner_module_reference(theEnv, classAndModuleName);
    module_def = ((struct module_definition *)get_current_module(theEnv));
    restore_current_module(theEnv);

    if( className == NULL )
    {
        return(NULL);
    }

    if((classSymbol = lookup_atom(theEnv, className)) == NULL )
    {
        return(NULL);
    }

    cls = DefclassData(theEnv)->ClassTable[HashClass(classSymbol)];

    while( cls != NULL )
    {
        if((cls->header.name == classSymbol) &&
           (cls->header.my_module->module_def == module_def))
        {
            return(cls->installed ? cls : NULL);
        }

        cls = cls->nxtHash;
    }

    return(NULL);
}

/****************************************************
 *  NAME         : LookupDefclassInScope
 *  DESCRIPTION  : Finds a class in current or imported
 *                modules (module specifier
 *                is not allowed)
 *  INPUTS       : The class name
 *  RETURNS      : The class (NULL if not found)
 *  SIDE EFFECTS : Error message printed on
 *               ambiguous references
 *  NOTES        : Assumes no two classes of the same
 *              name are ever in the same scope
 ****************************************************/
globle DEFCLASS *LookupDefclassInScope(void *theEnv, char *className)
{
    DEFCLASS *cls;
    ATOM_HN *classSymbol;

    if((classSymbol = lookup_atom(theEnv, className)) == NULL )
    {
        return(NULL);
    }

    cls = DefclassData(theEnv)->ClassTable[HashClass(classSymbol)];

    while( cls != NULL )
    {
        if((cls->header.name == classSymbol) && DefclassInScope(theEnv, cls, NULL))
        {
            return(cls->installed ? cls : NULL);
        }

        cls = cls->nxtHash;
    }

    return(NULL);
}

/******************************************************
 *  NAME         : LookupDefclassAnywhere
 *  DESCRIPTION  : Finds a class in specified
 *              (or any) module
 *  INPUTS       : 1) The module (NULL if don't care)
 *              2) The class name (module specifier
 *                 in name not allowed)
 *  RETURNS      : The class (NULL if not found)
 *  SIDE EFFECTS : None
 *  NOTES        : Does *not* generate an error if
 *              multiple classes of the same name
 *              exist as do the other lookup functions
 ******************************************************/
globle DEFCLASS *LookupDefclassAnywhere(void *theEnv, struct module_definition *module_def, char *className)
{
    DEFCLASS *cls;
    ATOM_HN *classSymbol;

    if((classSymbol = lookup_atom(theEnv, className)) == NULL )
    {
        return(NULL);
    }

    cls = DefclassData(theEnv)->ClassTable[HashClass(classSymbol)];

    while( cls != NULL )
    {
        if((cls->header.name == classSymbol) &&
           ((module_def == NULL) ||
            (cls->header.my_module->module_def == module_def)))
        {
            return(cls->installed ? cls : NULL);
        }

        cls = cls->nxtHash;
    }

    return(NULL);
}

/***************************************************
 *  NAME         : DefclassInScope
 *  DESCRIPTION  : Determines if a defclass is in
 *              scope of the given module
 *  INPUTS       : 1) The defclass
 *              2) The module (NULL for current
 *                 module)
 *  RETURNS      : TRUE if in scope,
 *              FALSE otherwise
 *  SIDE EFFECTS : None
 *  NOTES        : None
 ***************************************************/
#if WIN_BTC && (!DEFMODULE_CONSTRUCT)
#pragma argsused
#endif
globle BOOLEAN DefclassInScope(void *theEnv, DEFCLASS *theDefclass, struct module_definition *module_def)
{
#if DEFMODULE_CONSTRUCT
    int moduleID;
    char *scopeMap;

    scopeMap = (char *)to_bitmap(theDefclass->scopeMap);

    if( module_def == NULL )
    {
        module_def = ((struct module_definition *)get_current_module(theEnv));
    }

    moduleID = (int)module_def->id;
    return(core_bitmap_test(scopeMap, moduleID) ? TRUE : FALSE);

#else
#if MAC_MCW || WIN_MCW || MAC_XCD
#pragma unused(theEnv,theDefclass,theModule)
#endif
    return(TRUE);

#endif
}

/***********************************************************
 *  NAME         : EnvGetNextDefclass
 *  DESCRIPTION  : Finds first or next defclass
 *  INPUTS       : The address of the current defclass
 *  RETURNS      : The address of the next defclass
 *                (NULL if none)
 *  SIDE EFFECTS : None
 *  NOTES        : If ptr == NULL, the first defclass
 *                 is returned.
 ***********************************************************/
globle void *EnvGetNextDefclass(void *theEnv, void *ptr)
{
    return((void *)core_get_next_construct_metadata(theEnv, (struct construct_metadata *)ptr, DefclassData(theEnv)->DefclassModuleIndex));
}

/***************************************************
 *  NAME         : EnvIsDefclassDeletable
 *  DESCRIPTION  : Determines if a defclass
 *                can be deleted
 *  INPUTS       : Address of the defclass
 *  RETURNS      : TRUE if deletable,
 *              FALSE otherwise
 *  SIDE EFFECTS : None
 *  NOTES        : None
 ***************************************************/
globle BOOLEAN EnvIsDefclassDeletable(void *theEnv, void *ptr)
{
    DEFCLASS *cls;

    if( !core_are_constructs_deleteable(theEnv))
    {
        return(FALSE);
    }

    cls = (DEFCLASS *)ptr;

    if( cls->system == 1 )
    {
        return(FALSE);
    }

    return((IsClassBeingUsed(cls) == FALSE) ? TRUE : FALSE);
}

/*************************************************************
 *  NAME         : UndefclassCommand
 *  DESCRIPTION  : Deletes a class and its subclasses, as
 *              well as their associated instances
 *  INPUTS       : None
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : None
 *  NOTES        : Syntax : (undefclass <class-name> | *)
 *************************************************************/
globle void UndefclassCommand(void *theEnv)
{
    core_uninstall_construct_command(theEnv, "undefclass", DefclassData(theEnv)->DefclassConstruct);
}

/********************************************************
 *  NAME         : EnvUndefclass
 *  DESCRIPTION  : Deletes the named defclass
 *  INPUTS       : None
 *  RETURNS      : TRUE if deleted, or FALSE
 *  SIDE EFFECTS : Defclass and handlers removed
 *  NOTES        : Interface for core_define_construct()
 ********************************************************/
globle BOOLEAN EnvUndefclass(void *theEnv, void *theDefclass)
{
    DEFCLASS *cls;

    cls = (DEFCLASS *)theDefclass;

    if( cls == NULL )
    {
        return(RemoveAllUserClasses(theEnv));
    }

    return(DeleteClassUAG(theEnv, cls));
}

#if DEBUGGING_FUNCTIONS

/*********************************************************
 *  NAME         : PPDefclassCommand
 *  DESCRIPTION  : Displays the pretty print form of
 *              a class to the wdialog router.
 *  INPUTS       : None
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : None
 *  NOTES        : Syntax : (ppdefclass <class-name>)
 *********************************************************/
globle void PPDefclassCommand(void *theEnv)
{
    core_construct_pp_command(theEnv, "ppdefclass", DefclassData(theEnv)->DefclassConstruct);
}

/***************************************************
 *  NAME         : ListDefclassesCommand
 *  DESCRIPTION  : Displays all defclass names
 *  INPUTS       : None
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Defclass names printed
 *  NOTES        : H/L Interface
 ***************************************************/
globle void ListDefclassesCommand(void *theEnv)
{
    core_garner_module_constructs_command(theEnv, "list-defclasses", DefclassData(theEnv)->DefclassConstruct);
}

/***************************************************
 *  NAME         : EnvListDefclasses
 *  DESCRIPTION  : Displays all defclass names
 *  INPUTS       : 1) The logical name of the output
 *              2) The module
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Defclass names printed
 *  NOTES        : C Interface
 ***************************************************/
globle void EnvListDefclasses(void *theEnv, char *logical_name, struct module_definition *module_def)
{
    core_get_module_constructs_list(theEnv, DefclassData(theEnv)->DefclassConstruct, logical_name, module_def);
}

/*********************************************************
 *  NAME         : EnvGetDefclassWatchInstances
 *  DESCRIPTION  : Determines if deletions/creations of
 *              instances of this class will generate
 *              trace messages or not
 *  INPUTS       : A pointer to the class
 *  RETURNS      : TRUE if a trace is active,
 *              FALSE otherwise
 *  SIDE EFFECTS : None
 *  NOTES        : None
 *********************************************************/
#if WIN_BTC
#pragma argsused
#endif
globle unsigned EnvGetDefclassWatchInstances(void *theEnv, void *theClass)
{
#if MAC_MCW || WIN_MCW || MAC_XCD
#pragma unused(theEnv)
#endif

    return(((DEFCLASS *)theClass)->traceInstances);
}

/*********************************************************
 *  NAME         : EnvSetDefclassWatchInstances
 *  DESCRIPTION  : Sets the trace to ON/OFF for the
 *              creation/deletion of instances
 *              of the class
 *  INPUTS       : 1) TRUE to set the trace on,
 *                 FALSE to set it off
 *              2) A pointer to the class
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Watch flag for the class set
 *  NOTES        : None
 *********************************************************/
#if WIN_BTC
#pragma argsused
#endif
globle void EnvSetDefclassWatchInstances(void *theEnv, unsigned newState, void *theClass)
{
#if MAC_MCW || WIN_MCW || MAC_XCD
#pragma unused(theEnv)
#endif

    if(((DEFCLASS *)theClass)->abstract )
    {
        return;
    }

    ((DEFCLASS *)theClass)->traceInstances = newState;
}

/*********************************************************
 *  NAME         : EnvGetDefclassWatchSlots
 *  DESCRIPTION  : Determines if changes to slots of
 *              instances of this class will generate
 *              trace messages or not
 *  INPUTS       : A pointer to the class
 *  RETURNS      : TRUE if a trace is active,
 *              FALSE otherwise
 *  SIDE EFFECTS : None
 *  NOTES        : None
 *********************************************************/
#if WIN_BTC
#pragma argsused
#endif
globle unsigned EnvGetDefclassWatchSlots(void *theEnv, void *theClass)
{
#if MAC_MCW || WIN_MCW || MAC_XCD
#pragma unused(theEnv)
#endif

    return(((DEFCLASS *)theClass)->traceSlots);
}

/**********************************************************
 *  NAME         : EnvSetDefclassWatchSlots
 *  DESCRIPTION  : Sets the trace to ON/OFF for the
 *              changes to slots of instances of the class
 *  INPUTS       : 1) TRUE to set the trace on,
 *                 FALSE to set it off
 *              2) A pointer to the class
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Watch flag for the class set
 *  NOTES        : None
 **********************************************************/
#if WIN_BTC
#pragma argsused
#endif
globle void EnvSetDefclassWatchSlots(void *theEnv, unsigned newState, void *theClass)
{
#if MAC_MCW || WIN_MCW || MAC_XCD
#pragma unused(theEnv)
#endif

    ((DEFCLASS *)theClass)->traceSlots = newState;
}

/******************************************************************
 *  NAME         : DefclassWatchAccess
 *  DESCRIPTION  : Parses a list of class names passed by
 *              AddWatchItem() and sets the traces accordingly
 *  INPUTS       : 1) A code indicating which trace flag is to be set
 *                 0 - Watch instance creation/deletion
 *                 1 - Watch slot changes to instances
 *              2) The value to which to set the trace flags
 *              3) A list of expressions containing the names
 *                 of the classes for which to set traces
 *  RETURNS      : TRUE if all OK, FALSE otherwise
 *  SIDE EFFECTS : Watch flags set in specified classes
 *  NOTES        : Accessory function for AddWatchItem()
 ******************************************************************/
globle unsigned DefclassWatchAccess(void *theEnv, int code, unsigned newState, core_expression_object *argExprs)
{
    if( code )
    {
        return(ConstructSetWatchAccess(theEnv, DefclassData(theEnv)->DefclassConstruct, newState, argExprs,
                                       EnvGetDefclassWatchSlots, EnvSetDefclassWatchSlots));
    }
    else
    {
        return(ConstructSetWatchAccess(theEnv, DefclassData(theEnv)->DefclassConstruct, newState, argExprs,
                                       EnvGetDefclassWatchInstances, EnvSetDefclassWatchInstances));
    }
}

/***********************************************************************
 *  NAME         : DefclassWatchPrint
 *  DESCRIPTION  : Parses a list of class names passed by
 *              AddWatchItem() and displays the traces accordingly
 *  INPUTS       : 1) The logical name of the output
 *              2) A code indicating which trace flag is to be examined
 *                 0 - Watch instance creation/deletion
 *                 1 - Watch slot changes to instances
 *              3) A list of expressions containing the names
 *                 of the classes for which to examine traces
 *  RETURNS      : TRUE if all OK, FALSE otherwise
 *  SIDE EFFECTS : Watch flags displayed for specified classes
 *  NOTES        : Accessory function for AddWatchItem()
 ***********************************************************************/
globle unsigned DefclassWatchPrint(void *theEnv, char *logName, int code, core_expression_object *argExprs)
{
    if( code )
    {
        return(ConstructPrintWatchAccess(theEnv, DefclassData(theEnv)->DefclassConstruct, logName, argExprs,
                                         EnvGetDefclassWatchSlots, EnvSetDefclassWatchSlots));
    }
    else
    {
        return(ConstructPrintWatchAccess(theEnv, DefclassData(theEnv)->DefclassConstruct, logName, argExprs,
                                         EnvGetDefclassWatchInstances, EnvSetDefclassWatchInstances));
    }
}

#endif

/*********************************************************
 *  NAME         : GetDefclassListFunction
 *  DESCRIPTION  : Groups names of all defclasses into
 *                a list variable
 *  INPUTS       : A data object buffer
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : List set to list of classes
 *  NOTES        : None
 *********************************************************/
globle void GetDefclassListFunction(void *theEnv, core_data_object_ptr returnValue)
{
    core_garner_construct_list(theEnv, "get-defclass-list", returnValue, DefclassData(theEnv)->DefclassConstruct);
}

/***************************************************************
 *  NAME         : EnvGetDefclassList
 *  DESCRIPTION  : Groups all defclass names into
 *              a list list
 *  INPUTS       : 1) A data object buffer to hold
 *                 the list result
 *              2) The module from which to obtain defclasses
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : List allocated and filled
 *  NOTES        : External C access
 ***************************************************************/
globle void EnvGetDefclassList(void *theEnv, core_data_object *returnValue, struct module_definition *module_def)
{
    core_get_module_constructs_list(theEnv, returnValue, DefclassData(theEnv)->DefclassConstruct, module_def);
}

/*****************************************************
 *  NAME         : HasSuperclass
 *  DESCRIPTION  : Determines if class-2 is a superclass
 *                of class-1
 *  INPUTS       : 1) Class-1
 *              2) Class-2
 *  RETURNS      : TRUE if class-2 is a superclass of
 *                class-1, FALSE otherwise
 *  SIDE EFFECTS : None
 *  NOTES        : None
 *****************************************************/
globle int HasSuperclass(DEFCLASS *c1, DEFCLASS *c2)
{
    long i;

    for( i = 1 ; i < c1->allSuperclasses.classCount ; i++ )
    {
        if( c1->allSuperclasses.classArray[i] == c2 )
        {
            return(TRUE);
        }
    }

    return(FALSE);
}

/********************************************************************
 *  NAME         : CheckClassAndSlot
 *  DESCRIPTION  : Checks class and slot argument for various functions
 *  INPUTS       : 1) Name of the calling function
 *              2) Buffer for class address
 *  RETURNS      : Slot symbol, NULL on errors
 *  SIDE EFFECTS : None
 *  NOTES        : None
 ********************************************************************/
globle ATOM_HN *CheckClassAndSlot(void *theEnv, char *func, DEFCLASS **cls)
{
    core_data_object temp;

    if( core_check_arg_type(theEnv, func, 1, ATOM, &temp) == FALSE )
    {
        return(NULL);
    }

    *cls = LookupDefclassByMdlOrScope(theEnv, core_convert_data_to_string(temp));

    if( *cls == NULL )
    {
        ClassExistError(theEnv, func, core_convert_data_to_string(temp));
        return(NULL);
    }

    if( core_check_arg_type(theEnv, func, 2, ATOM, &temp) == FALSE )
    {
        return(NULL);
    }

    return((ATOM_HN *)core_get_value(temp));
}

/***************************************************
 *  NAME         : SaveDefclasses
 *  DESCRIPTION  : Prints pretty print form of
 *                defclasses to specified output
 *  INPUTS       : The  logical name of the output
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : None
 *  NOTES        : None
 ***************************************************/
#if WIN_BTC && (!DEBUGGING_FUNCTIONS)
#pragma argsused
#endif
globle void SaveDefclasses(void *theEnv, void *module_def, char *logName)
{
#if DEBUGGING_FUNCTIONS
    core_do_for_all_constructs_in_module(theEnv, module_def, SaveDefclass, DefclassData(theEnv)->DefclassModuleIndex, FALSE, (void *)logName);
#else
#if MAC_MCW || WIN_MCW || MAC_XCD
#pragma unused(theEnv,theModule,logName)
#endif
#endif
}

/* =========================================
 *****************************************
 *       INTERNALLY VISIBLE FUNCTIONS
 *  =========================================
 ***************************************** */

#if DEBUGGING_FUNCTIONS

/***************************************************
 *  NAME         : SaveDefclass
 *  DESCRIPTION  : Writes out the pretty-print forms
 *              of a class and all its handlers
 *  INPUTS       : 1) The class
 *              2) The logical name of the output
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Class and handlers written
 *  NOTES        : None
 ***************************************************/
static void SaveDefclass(void *theEnv, struct construct_metadata *theDefclass, void *userBuffer)
{
    char *logName = (char *)userBuffer;
    unsigned hnd;
    char *pp;

    pp = EnvGetDefclassPPForm(theEnv, (void *)theDefclass);

    if( pp != NULL )
    {
        core_print_chunkify(theEnv, logName, pp);
        print_router(theEnv, logName, "\n");
        hnd = EnvGetNextDefmessageHandler(theEnv, (void *)theDefclass, 0);

        while( hnd != 0 )
        {
            pp = EnvGetDefmessageHandlerPPForm(theEnv, (void *)theDefclass, hnd);

            if( pp != NULL )
            {
                core_print_chunkify(theEnv, logName, pp);
                print_router(theEnv, logName, "\n");
            }

            hnd = EnvGetNextDefmessageHandler(theEnv, (void *)theDefclass, hnd);
        }
    }
}

#endif

/**********************************************
 * EnvSetClassDefaultsMode: Allows the setting
 *    of the class defaults mode.
 ***********************************************/
globle unsigned short EnvSetClassDefaultsMode(void *theEnv, unsigned short value)
{
    unsigned short ov;

    ov = DefclassData(theEnv)->ClassDefaultsMode;
    DefclassData(theEnv)->ClassDefaultsMode = value;
    return(ov);
}

/***************************************
 * EnvGetClassDefaultsMode: Returns the
 *    value of the class defaults mode.
 ****************************************/
globle unsigned short EnvGetClassDefaultsMode(void *theEnv)
{
    return(DefclassData(theEnv)->ClassDefaultsMode);
}

/**************************************************
 * GetClassDefaultsModeCommand: H/L access routine
 *   for the get-class-defaults-mode command.
 ***************************************************/
globle void *GetClassDefaultsModeCommand(void *theEnv)
{
    core_check_arg_count(theEnv, "get-class-defaults-mode", EXACTLY, 0);

    return((ATOM_HN *)store_atom(theEnv, GetClassDefaultsModeName(EnvGetClassDefaultsMode(theEnv))));
}

/**************************************************
 * SetClassDefaultsModeCommand: H/L access routine
 *   for the set-class-defaults-mode command.
 ***************************************************/
globle void *SetClassDefaultsModeCommand(void *theEnv)
{
    core_data_object argPtr;
    char *argument;
    unsigned short oldMode;

    oldMode = DefclassData(theEnv)->ClassDefaultsMode;

    /*=====================================================
     * Check for the correct number and type of arguments.
     *=====================================================*/

    if( core_check_arg_count(theEnv, "set-class-defaults-mode", EXACTLY, 1) == -1 )
    {
        return((ATOM_HN *)store_atom(theEnv, GetClassDefaultsModeName(EnvGetClassDefaultsMode(theEnv))));
    }

    if( core_check_arg_type(theEnv, "set-class-defaults-mode", 1, ATOM, &argPtr) == FALSE )
    {
        return((ATOM_HN *)store_atom(theEnv, GetClassDefaultsModeName(EnvGetClassDefaultsMode(theEnv))));
    }

    argument = core_convert_data_to_string(argPtr);

    /*=============================================
     * Set the strategy to the specified strategy.
     *=============================================*/

    if( strcmp(argument, "conservation") == 0 )
    {
        EnvSetClassDefaultsMode(theEnv, CONSERVATION_MODE);
    }
    else if( strcmp(argument, "convenience") == 0 )
    {
        EnvSetClassDefaultsMode(theEnv, CONVENIENCE_MODE);
    }
    else
    {
        report_explicit_type_error(theEnv, "set-class-defaults-mode", 1,
                           "symbol with value conservation or convenience");
        return((ATOM_HN *)store_atom(theEnv, GetClassDefaultsModeName(EnvGetClassDefaultsMode(theEnv))));
    }

    /*===================================
     * Return the old value of the mode.
     *===================================*/

    return((ATOM_HN *)store_atom(theEnv, GetClassDefaultsModeName(oldMode)));
}

/******************************************************************
 * GetClassDefaultsModeName: Given the integer value corresponding
 *   to a specified class defaults mode, return a character string
 *   of the class defaults mode's name.
 *******************************************************************/
static char *GetClassDefaultsModeName(unsigned short mode)
{
    char *sname;

    switch( mode )
    {
    case CONSERVATION_MODE:
        sname = "conservation";
        break;
    case CONVENIENCE_MODE:
        sname = "convenience";
        break;
    default:
        sname = "unknown";
        break;
    }

    return(sname);
}

#endif
