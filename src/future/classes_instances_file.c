/* Purpose:  File load/save routines for instances           */

/* =========================================
 *****************************************
 *            EXTERNAL DEFINITIONS
 *  =========================================
 ***************************************** */

#include <stdlib.h>

#include "setup.h"

#if OBJECT_SYSTEM

#include "core_arguments.h"
#include "classes_kernel.h"
#include "funcs_class.h"
#include "core_memory.h"
#include "core_functions.h"
#include "classes_instances_kernel.h"
#include "funcs_instance.h"
#include "classes_instances_init.h"
#include "parser_class_instances.h"
#include "classes.h"
#include "router.h"
#include "router_string.h"
#include "sysdep.h"
#include "core_environment.h"

#define __CLASSES_INSTANCES_FILE_SOURCE__
#include "classes_instances_file.h"

/* =========================================
 *****************************************
 *                CONSTANTS
 *  =========================================
 ***************************************** */
#define MAX_BLOCK_SIZE 10240

/* =========================================
 *****************************************
 *            MACROS AND TYPES
 *  =========================================
 ***************************************** */
struct bsaveSlotValue
{
    long     slotName;
    unsigned valueCount;
};

struct bsaveSlotValueAtom
{
    unsigned short type;
    long           value;
};

/* =========================================
 *****************************************
 *   INTERNALLY VISIBLE FUNCTION HEADERS
 *  =========================================
 ***************************************** */

static long InstancesSaveCommandParser(void *, char *, long(*) (void *, char *, int,
                                                                core_expression_object *, BOOLEAN));
static core_data_object *ProcessSaveClassList(void *, char *, core_expression_object *, int, BOOLEAN);
static void ReturnSaveClassList(void *, core_data_object *);
static long SaveOrMarkInstances(void *, void *, int, core_data_object *, BOOLEAN, BOOLEAN,
                                void(*) (void *, void *, INSTANCE_TYPE *));
static long SaveOrMarkInstancesOfClass(void *, void *, struct module_definition *, int, DEFCLASS *,
                                       BOOLEAN, int, void(*) (void *, void *, INSTANCE_TYPE *));
static void SaveSingleInstanceText(void *, void *, INSTANCE_TYPE *);
static void ProcessFileErrorMessage(void *, char *, char *);

static long LoadOrRestoreInstances(void *, char *, int, int);

/* =========================================
 *****************************************
 *       EXTERNALLY VISIBLE FUNCTIONS
 *  =========================================
 ***************************************** */

/***************************************************
 *  NAME         : SetupInstanceFileCommands
 *  DESCRIPTION  : Defines function interfaces for
 *              saving instances to files
 *  INPUTS       : None
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Functions defined to KB
 *  NOTES        : None
 ***************************************************/
globle void SetupInstanceFileCommands(void *theEnv)
{
    core_define_function(theEnv, "save-instances", 'l', PTR_FN SaveInstancesCommand,
                       "SaveInstancesCommand", "1*wk");
    core_define_function(theEnv, "load-instances", 'l', PTR_FN LoadInstancesCommand,
                       "LoadInstancesCommand", "11k");
    core_define_function(theEnv, "restore-instances", 'l', PTR_FN RestoreInstancesCommand,
                       "RestoreInstancesCommand", "11k");
}

/****************************************************************************
 *  NAME         : SaveInstancesCommand
 *  DESCRIPTION  : H/L interface for saving
 *                current instances to a file
 *  INPUTS       : None
 *  RETURNS      : The number of instances saved
 *  SIDE EFFECTS : Instances saved to named file
 *  NOTES        : H/L Syntax :
 *              (save-instances <file> [local|visible [[inherit] <class>+]])
 ****************************************************************************/
globle long SaveInstancesCommand(void *theEnv)
{
    return(InstancesSaveCommandParser(theEnv, "save-instances", coreSaveInstances));
}

/******************************************************
 *  NAME         : LoadInstancesCommand
 *  DESCRIPTION  : H/L interface for loading
 *                instances from a file
 *  INPUTS       : None
 *  RETURNS      : The number of instances loaded
 *  SIDE EFFECTS : Instances loaded from named file
 *  NOTES        : H/L Syntax : (load-instances <file>)
 ******************************************************/
globle long LoadInstancesCommand(void *theEnv)
{
    char *fileFound;
    core_data_object temp;
    long instanceCount;

    if( core_check_arg_type(theEnv, "load-instances", 1, ATOM_OR_STRING, &temp) == FALSE )
    {
        return(0L);
    }

    fileFound = core_convert_data_to_string(temp);

    instanceCount = EnvLoadInstances(theEnv, fileFound);

    if( core_get_evaluation_data(theEnv)->eval_error )
    {
        ProcessFileErrorMessage(theEnv, "load-instances", fileFound);
    }

    return(instanceCount);
}

/***************************************************
 *  NAME         : EnvLoadInstances
 *  DESCRIPTION  : Loads instances from named file
 *  INPUTS       : The name of the input file
 *  RETURNS      : The number of instances loaded
 *  SIDE EFFECTS : Instances loaded from file
 *  NOTES        : None
 ***************************************************/
globle long EnvLoadInstances(void *theEnv, char *file)
{
    return(LoadOrRestoreInstances(theEnv, file, TRUE, TRUE));
}

/***************************************************
 *  NAME         : EnvLoadInstancesFromString
 *  DESCRIPTION  : Loads instances from given string
 *  INPUTS       : 1) The input string
 *              2) Index of char in string after
 *                 last valid char (-1 for all chars)
 *  RETURNS      : The number of instances loaded
 *  SIDE EFFECTS : Instances loaded from string
 *  NOTES        : Uses string routers
 ***************************************************/
globle long EnvLoadInstancesFromString(void *theEnv, char *representation, int theMax)
{
    long theCount;
    char *theStrRouter = "*** load-instances-from-string ***";

    if((theMax == -1) ? (!open_string_source(theEnv, theStrRouter, representation, 0)) :
       (!open_text_source(theEnv, theStrRouter, representation, 0, (unsigned)theMax)))
    {
        return(-1L);
    }

    theCount = LoadOrRestoreInstances(theEnv, theStrRouter, TRUE, FALSE);
    close_string_source(theEnv, theStrRouter);
    return(theCount);
}

/*********************************************************
 *  NAME         : RestoreInstancesCommand
 *  DESCRIPTION  : H/L interface for loading
 *                instances from a file w/o messages
 *  INPUTS       : None
 *  RETURNS      : The number of instances restored
 *  SIDE EFFECTS : Instances loaded from named file
 *  NOTES        : H/L Syntax : (restore-instances <file>)
 *********************************************************/
globle long RestoreInstancesCommand(void *theEnv)
{
    char *fileFound;
    core_data_object temp;
    long instanceCount;

    if( core_check_arg_type(theEnv, "restore-instances", 1, ATOM_OR_STRING, &temp) == FALSE )
    {
        return(0L);
    }

    fileFound = core_convert_data_to_string(temp);

    instanceCount = EnvRestoreInstances(theEnv, fileFound);

    if( core_get_evaluation_data(theEnv)->eval_error )
    {
        ProcessFileErrorMessage(theEnv, "restore-instances", fileFound);
    }

    return(instanceCount);
}

/***************************************************
 *  NAME         : EnvRestoreInstances
 *  DESCRIPTION  : Restores instances from named file
 *  INPUTS       : The name of the input file
 *  RETURNS      : The number of instances restored
 *  SIDE EFFECTS : Instances restored from file
 *  NOTES        : None
 ***************************************************/
globle long EnvRestoreInstances(void *theEnv, char *file)
{
    return(LoadOrRestoreInstances(theEnv, file, FALSE, TRUE));
}

/***************************************************
 *  NAME         : EnvRestoreInstancesFromString
 *  DESCRIPTION  : Restores instances from given string
 *  INPUTS       : 1) The input string
 *              2) Index of char in string after
 *                 last valid char (-1 for all chars)
 *  RETURNS      : The number of instances loaded
 *  SIDE EFFECTS : Instances loaded from string
 *  NOTES        : Uses string routers
 ***************************************************/
globle long EnvRestoreInstancesFromString(void *theEnv, char *representation, int theMax)
{
    long theCount;
    char *theStrRouter = "*** load-instances-from-string ***";

    if((theMax == -1) ? (!open_string_source(theEnv, theStrRouter, representation, 0)) :
       (!open_text_source(theEnv, theStrRouter, representation, 0, (unsigned)theMax)))
    {
        return(-1L);
    }

    theCount = LoadOrRestoreInstances(theEnv, theStrRouter, FALSE, FALSE);
    close_string_source(theEnv, theStrRouter);
    return(theCount);
}

/*******************************************************
 *  NAME         : coreSaveInstances
 *  DESCRIPTION  : Saves current instances to named file
 *  INPUTS       : 1) The name of the output file
 *              2) A flag indicating whether to
 *                 save local (current module only)
 *                 or visible instances
 *                 LOCAL_SAVE or VISIBLE_SAVE
 *              3) A list of expressions containing
 *                 the names of classes for which
 *                 instances are to be saved
 *              4) A flag indicating if the subclasses
 *                 of specified classes shoudl also
 *                 be processed
 *  RETURNS      : The number of instances saved
 *  SIDE EFFECTS : Instances saved to file
 *  NOTES        : None
 *******************************************************/
globle long coreSaveInstances(void *theEnv, char *file, int saveCode, core_expression_object *classExpressionList, BOOLEAN inheritFlag)
{
    FILE *sfile = NULL;
    int oldPEC, oldATS, oldIAN;
    core_data_object *class_list;
    long instanceCount;

    class_list = ProcessSaveClassList(theEnv, "save-instances", classExpressionList,
                                     saveCode, inheritFlag);

    if((class_list == NULL) && (classExpressionList != NULL))
    {
        return(0L);
    }

    SaveOrMarkInstances(theEnv, (void *)sfile, saveCode, class_list,
                        inheritFlag, TRUE, NULL);

    if((sfile = sysdep_open_file(theEnv, file, "w")) == NULL )
    {
        report_file_open_error(theEnv, "save-instances", file);
        ReturnSaveClassList(theEnv, class_list);
        core_set_eval_error(theEnv, TRUE);
        return(0L);
    }

    oldPEC = core_get_utility_data(theEnv)->preserving_escape;
    core_get_utility_data(theEnv)->preserving_escape = TRUE;
    oldATS = core_get_utility_data(theEnv)->stringifying_pointer;
    core_get_utility_data(theEnv)->stringifying_pointer = TRUE;
    oldIAN = core_get_utility_data(theEnv)->converting_inst_address_to_name;
    core_get_utility_data(theEnv)->converting_inst_address_to_name = TRUE;

    set_fast_save(theEnv, sfile);
    instanceCount = SaveOrMarkInstances(theEnv, (void *)sfile, saveCode, class_list,
                                        inheritFlag, TRUE, SaveSingleInstanceText);
    sysdep_close_file(theEnv, sfile);
    set_fast_save(theEnv, NULL);

    core_get_utility_data(theEnv)->preserving_escape = oldPEC;
    core_get_utility_data(theEnv)->stringifying_pointer = oldATS;
    core_get_utility_data(theEnv)->converting_inst_address_to_name = oldIAN;
    ReturnSaveClassList(theEnv, class_list);
    return(instanceCount);
}

/* =========================================
 *****************************************
 *       INTERNALLY VISIBLE FUNCTIONS
 *  =========================================
 ***************************************** */

/******************************************************
 *  NAME         : InstancesSaveCommandParser
 *  DESCRIPTION  : Argument parser for save-instances
 *              and bsave-instances
 *  INPUTS       : 1) The name of the calling function
 *              2) A pointer to the support
 *                 function to call for the save/bsave
 *  RETURNS      : The number of instances saved
 *  SIDE EFFECTS : Instances saved/bsaved
 *  NOTES        : None
 ******************************************************/
static long InstancesSaveCommandParser(void *theEnv, char *functionName, long (*saveFunction)(void *, char *, int, core_expression_object *, BOOLEAN))
{
    char *fileFound;
    core_data_object temp;
    int argCount, saveCode = LOCAL_SAVE;
    core_expression_object *class_list = NULL;
    BOOLEAN inheritFlag = FALSE;

    if( core_check_arg_type(theEnv, functionName, 1, ATOM_OR_STRING, &temp) == FALSE )
    {
        return(0L);
    }

    fileFound = core_convert_data_to_string(temp);

    argCount = core_get_arg_count(theEnv);

    if( argCount > 1 )
    {
        if( core_check_arg_type(theEnv, functionName, 2, ATOM, &temp) == FALSE )
        {
            report_explicit_type_error(theEnv, functionName, 2, "symbol \"local\" or \"visible\"");
            core_set_eval_error(theEnv, TRUE);
            return(0L);
        }

        if( strcmp(core_convert_data_to_string(temp), "local") == 0 )
        {
            saveCode = LOCAL_SAVE;
        }
        else if( strcmp(core_convert_data_to_string(temp), "visible") == 0 )
        {
            saveCode = VISIBLE_SAVE;
        }
        else
        {
            report_explicit_type_error(theEnv, functionName, 2, "symbol \"local\" or \"visible\"");
            core_set_eval_error(theEnv, TRUE);
            return(0L);
        }

        class_list = core_get_first_arg()->next_arg->next_arg;

        /* ===========================
         *  Check for "inherit" keyword
         *  Must be at least one class
         *  name following
         *  =========================== */
        if((class_list != NULL) ? (class_list->next_arg != NULL) : FALSE )
        {
            if((class_list->type != ATOM) ? FALSE :
               (strcmp(to_string(class_list->value), "inherit") == 0))
            {
                inheritFlag = TRUE;
                class_list = class_list->next_arg;
            }
        }
    }

    return((*saveFunction)(theEnv, fileFound, saveCode, class_list, inheritFlag));
}

/****************************************************
 *  NAME         : ProcessSaveClassList
 *  DESCRIPTION  : Evaluates a list of class name
 *              expressions and stores them in a
 *              data object list
 *  INPUTS       : 1) The name of the calling function
 *              2) The class expression list
 *              3) A flag indicating if only local
 *                 or all visible instances are
 *                 being saved
 *              4) A flag indicating if inheritance
 *                 relationships should be checked
 *                 between classes
 *  RETURNS      : The evaluated class pointer data
 *              objects - NULL on errors
 *  SIDE EFFECTS : Data objects allocated and
 *              classes validated
 *  NOTES        : None
 ****************************************************/
static core_data_object *ProcessSaveClassList(void *theEnv, char *functionName, core_expression_object *classExps, int saveCode, BOOLEAN inheritFlag)
{
    core_data_object *head = NULL, *prv, *newItem, tmp;
    DEFCLASS *theDefclass;
    struct module_definition *currentModule;
    int argIndex = inheritFlag ? 4 : 3;

    currentModule = ((struct module_definition *)get_current_module(theEnv));

    while( classExps != NULL )
    {
        if( core_eval_expression(theEnv, classExps, &tmp))
        {
            goto ProcessClassListError;
        }

        if( tmp.type != ATOM )
        {
            goto ProcessClassListError;
        }

        if( saveCode == LOCAL_SAVE )
        {
            theDefclass = LookupDefclassAnywhere(theEnv, currentModule, core_convert_data_to_string(tmp));
        }
        else
        {
            theDefclass = LookupDefclassInScope(theEnv, core_convert_data_to_string(tmp));
        }

        if( theDefclass == NULL )
        {
            goto ProcessClassListError;
        }
        else if( theDefclass->abstract && (inheritFlag == FALSE))
        {
            goto ProcessClassListError;
        }

        prv = newItem = head;

        while( newItem != NULL )
        {
            if( newItem->value == (void *)theDefclass )
            {
                goto ProcessClassListError;
            }
            else if( inheritFlag )
            {
                if( HasSuperclass((DEFCLASS *)newItem->value, theDefclass) ||
                    HasSuperclass(theDefclass, (DEFCLASS *)newItem->value))
                {
                    goto ProcessClassListError;
                }
            }

            prv = newItem;
            newItem = newItem->next;
        }

        newItem = core_mem_get_struct(theEnv, core_data);
        newItem->type = DEFCLASS_PTR;
        newItem->value = (void *)theDefclass;
        newItem->next = NULL;

        if( prv == NULL )
        {
            head = newItem;
        }
        else
        {
            prv->next = newItem;
        }

        argIndex++;
        classExps = classExps->next_arg;
    }

    return(head);

ProcessClassListError:

    if( inheritFlag )
    {
        report_explicit_type_error(theEnv, functionName, argIndex, "valid class name");
    }
    else
    {
        report_explicit_type_error(theEnv, functionName, argIndex, "valid concrete class name");
    }

    ReturnSaveClassList(theEnv, head);
    core_set_eval_error(theEnv, TRUE);
    return(NULL);
}

/****************************************************
 *  NAME         : ReturnSaveClassList
 *  DESCRIPTION  : Deallocates the class data object
 *              list created by ProcessSaveClassList
 *  INPUTS       : The class data object list
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Class data object returned
 *  NOTES        : None
 ****************************************************/
static void ReturnSaveClassList(void *theEnv, core_data_object *class_list)
{
    core_data_object *tmp;

    while( class_list != NULL )
    {
        tmp = class_list;
        class_list = class_list->next;
        core_mem_return_struct(theEnv, core_data, tmp);
    }
}

/***************************************************
 *  NAME         : SaveOrMarkInstances
 *  DESCRIPTION  : Iterates through all specified
 *              instances either marking is_needed
 *              atoms or writing instances in
 *              binary/text format
 *  INPUTS       : 1) NULL (for marking),
 *                 logical name (for text saves)
 *                 file pointer (for binary saves)
 *              2) A cope flag indicating LOCAL
 *                 or VISIBLE saves only
 *              3) A list of data objects
 *                 containing the names of classes
 *                 of instances to be saved
 *              4) A flag indicating whether to
 *                 include subclasses of arg #3
 *              5) A flag indicating if the
 *                 iteration can be interrupted
 *                 or not
 *              6) The access function to mark
 *                 or save an instance (can be NULL
 *                 if only counting instances)
 *  RETURNS      : The number of instances saved
 *  SIDE EFFECTS : Instances amrked or saved
 *  NOTES        : None
 ***************************************************/
static long SaveOrMarkInstances(void *theEnv, void *theOutput, int saveCode, core_data_object *class_list, BOOLEAN inheritFlag, BOOLEAN interruptOK, void (*saveInstanceFunc)(void *, void *, INSTANCE_TYPE *))
{
    struct module_definition *currentModule;
    int traversalID;
    core_data_object *tmp;
    INSTANCE_TYPE *ins;
    long instanceCount = 0L;

    currentModule = ((struct module_definition *)get_current_module(theEnv));

    if( class_list != NULL )
    {
        traversalID = GetTraversalID(theEnv);

        if( traversalID != -1 )
        {
            for( tmp = class_list ;
                 (!((tmp == NULL) || (core_get_evaluation_data(theEnv)->halt && interruptOK))) ;
                 tmp = tmp->next )
            {
                instanceCount += SaveOrMarkInstancesOfClass(theEnv, theOutput, currentModule, saveCode,
                                                            (DEFCLASS *)tmp->value, inheritFlag,
                                                            traversalID, saveInstanceFunc);
            }

            ReleaseTraversalID(theEnv);
        }
    }
    else
    {
        for( ins = (INSTANCE_TYPE *)GetNextInstanceInScope(theEnv, NULL) ;
             (ins != NULL) && (core_get_evaluation_data(theEnv)->halt != TRUE) ;
             ins = (INSTANCE_TYPE *)GetNextInstanceInScope(theEnv, (void *)ins))
        {
            if((saveCode == VISIBLE_SAVE) ? TRUE :
               (ins->cls->header.my_module->module_def == currentModule))
            {
                if( saveInstanceFunc != NULL )
                {
                    (*saveInstanceFunc)(theEnv, theOutput, ins);
                }

                instanceCount++;
            }
        }
    }

    return(instanceCount);
}

/***************************************************
 *  NAME         : SaveOrMarkInstancesOfClass
 *  DESCRIPTION  : Saves off the direct (and indirect)
 *              instance of the specified class
 *  INPUTS       : 1) The logical name of the output
 *                 (or file pointer for binary
 *                  output)
 *              2) The current module
 *              3) A flag indicating local
 *                 or visible saves
 *              4) The defclass
 *              5) A flag indicating whether to
 *                 save subclass instances or not
 *              6) A traversal id for marking
 *                 visited classes
 *              7) A pointer to the instance
 *                 manipulation function to call
 *                 (can be NULL for only counting
 *                  instances)
 *  RETURNS      : The number of instances saved
 *  SIDE EFFECTS : Appropriate instances saved
 *  NOTES        : None
 ***************************************************/
static long SaveOrMarkInstancesOfClass(void *theEnv, void *theOutput, struct module_definition *currentModule, int saveCode, DEFCLASS *theDefclass, BOOLEAN inheritFlag, int traversalID, void (*saveInstanceFunc)(void *, void *, INSTANCE_TYPE *))
{
    INSTANCE_TYPE *theInstance;
    DEFCLASS *subclass;
    long i;
    long instanceCount = 0L;

    if( TestTraversalID(theDefclass->traversalRecord, traversalID))
    {
        return(instanceCount);
    }

    SetTraversalID(theDefclass->traversalRecord, traversalID);

    if(((saveCode == LOCAL_SAVE) &&
        (theDefclass->header.my_module->module_def == currentModule)) ||
       ((saveCode == VISIBLE_SAVE) &&
        DefclassInScope(theEnv, theDefclass, currentModule)))
    {
        for( theInstance = (INSTANCE_TYPE *)
                           EnvGetNextInstanceInClass(theEnv, (void *)theDefclass, NULL) ;
             theInstance != NULL ;
             theInstance = (INSTANCE_TYPE *)
                           EnvGetNextInstanceInClass(theEnv, (void *)theDefclass, (void *)theInstance))
        {
            if( saveInstanceFunc != NULL )
            {
                (*saveInstanceFunc)(theEnv, theOutput, theInstance);
            }

            instanceCount++;
        }
    }

    if( inheritFlag )
    {
        for( i = 0 ; i < theDefclass->directSubclasses.classCount ; i++ )
        {
            subclass = theDefclass->directSubclasses.classArray[i];
            instanceCount += SaveOrMarkInstancesOfClass(theEnv, theOutput, currentModule, saveCode,
                                                        subclass, TRUE, traversalID,
                                                        saveInstanceFunc);
        }
    }

    return(instanceCount);
}

/***************************************************
 *  NAME         : SaveSingleInstanceText
 *  DESCRIPTION  : Writes given instance to file
 *  INPUTS       : 1) The logical name of the output
 *              2) The instance to save
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Instance written
 *  NOTES        : None
 ***************************************************/
static void SaveSingleInstanceText(void *theEnv, void *vLogicalName, INSTANCE_TYPE *theInstance)
{
    long i;
    INSTANCE_SLOT *sp;
    char *logical_name = (char *)vLogicalName;

    print_router(theEnv, logical_name, "([");
    print_router(theEnv, logical_name, to_string(theInstance->name));
    print_router(theEnv, logical_name, "] of ");
    print_router(theEnv, logical_name, to_string(theInstance->cls->header.name));

    for( i = 0 ; i < theInstance->cls->instanceSlotCount ; i++ )
    {
        sp = theInstance->slotAddresses[i];
        print_router(theEnv, logical_name, "\n   (");
        print_router(theEnv, logical_name, to_string(sp->desc->slotName->name));

        if( sp->type != LIST )
        {
            print_router(theEnv, logical_name, " ");
            core_print_atom(theEnv, logical_name, (int)sp->type, sp->value);
        }
        else if( GetInstanceSlotLength(sp) != 0 )
        {
            print_router(theEnv, logical_name, " ");
            print_list(theEnv, logical_name, (LIST_PTR)sp->value, 0,
                      (long)(GetInstanceSlotLength(sp) - 1), FALSE);
        }

        print_router(theEnv, logical_name, ")");
    }

    print_router(theEnv, logical_name, ")\n\n");
}

/**********************************************************************
 *  NAME         : LoadOrRestoreInstances
 *  DESCRIPTION  : Loads instances from named file
 *  INPUTS       : 1) The name of the input file
 *              2) An integer flag indicating whether or
 *                 not to use message-passing to create
 *                 the new instances and delete old versions
 *              3) An integer flag indicating if arg #1
 *                 is a file name or the name of a string router
 *  RETURNS      : The number of instances loaded/restored
 *  SIDE EFFECTS : Instances loaded from file
 *  NOTES        : None
 **********************************************************************/
static long LoadOrRestoreInstances(void *theEnv, char *file, int usemsgs, int isFileName)
{
    core_data_object temp;
    FILE *sfile = NULL, *svload = NULL;
    char *ilog;
    core_expression_object *top;
    int svoverride;
    long instanceCount = 0L;

    if( isFileName )
    {
        if((sfile = sysdep_open_file(theEnv, file, "r")) == NULL )
        {
            core_set_eval_error(theEnv, TRUE);
            return(-1L);
        }

        svload = get_fast_load(theEnv);
        ilog = (char *)sfile;
        set_fast_load(theEnv, sfile);
    }
    else
    {
        ilog = file;
    }

    top = core_generate_constant(theEnv, FCALL, (void *)core_lookup_function(theEnv, "make-instance"));
    core_get_token(theEnv, ilog, &DefclassData(theEnv)->ObjectParseToken);
    svoverride = InstanceData(theEnv)->MkInsMsgPass;
    InstanceData(theEnv)->MkInsMsgPass = usemsgs;

    while((core_get_type(DefclassData(theEnv)->ObjectParseToken) != STOP) && (core_get_evaluation_data(theEnv)->halt != TRUE))
    {
        if( core_get_type(DefclassData(theEnv)->ObjectParseToken) != LPAREN )
        {
            error_syntax(theEnv, "instance definition");
            core_mem_return_struct(theEnv, core_expression, top);

            if( isFileName )
            {
                sysdep_close_file(theEnv, sfile);
                set_fast_load(theEnv, svload);
            }

            core_set_eval_error(theEnv, TRUE);
            InstanceData(theEnv)->MkInsMsgPass = svoverride;
            return(instanceCount);
        }

        if( ParseSimpleInstance(theEnv, top, ilog) == NULL )
        {
            if( isFileName )
            {
                sysdep_close_file(theEnv, sfile);
                set_fast_load(theEnv, svload);
            }

            InstanceData(theEnv)->MkInsMsgPass = svoverride;
            core_set_eval_error(theEnv, TRUE);
            return(instanceCount);
        }

        core_increment_expression(theEnv, top);
        core_eval_expression(theEnv, top, &temp);
        core_decrement_expression(theEnv, top);

        if( !core_get_evaluation_data(theEnv)->eval_error )
        {
            instanceCount++;
        }

        core_return_expression(theEnv, top->args);
        top->args = NULL;
        core_get_token(theEnv, ilog, &DefclassData(theEnv)->ObjectParseToken);
    }

    core_mem_return_struct(theEnv, core_expression, top);

    if( isFileName )
    {
        sysdep_close_file(theEnv, sfile);
        set_fast_load(theEnv, svload);
    }

    InstanceData(theEnv)->MkInsMsgPass = svoverride;
    return(instanceCount);
}

/***************************************************
 *  NAME         : ProcessFileErrorMessage
 *  DESCRIPTION  : Prints an error message when a
 *              file containing text or binary
 *              instances cannot be processed.
 *  INPUTS       : The name of the input file and the
 *              function which opened it.
 *  RETURNS      : No value
 *  SIDE EFFECTS : None
 *  NOTES        : None
 ***************************************************/
static void ProcessFileErrorMessage(void *theEnv, char *functionName, char *fileName)
{
    error_print_id(theEnv, "INSFILE", 1, FALSE);
    print_router(theEnv, WERROR, "Function ");
    print_router(theEnv, WERROR, functionName);
    print_router(theEnv, WERROR, " could not completely process file ");
    print_router(theEnv, WERROR, fileName);
    print_router(theEnv, WERROR, ".\n");
}

#endif
