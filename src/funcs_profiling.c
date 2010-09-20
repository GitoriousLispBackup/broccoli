/* Purpose: Contains the code for profiling the amount of
 *   time spent in constructs and user defined functions.    */

#define __FUNCS_PROFILING_SOURCE__

#include "setup.h"

#if PROFILING_FUNCTIONS

#include "core_arguments.h"
#include "classes_kernel.h"
#include "funcs_function.h"
#include "core_environment.h"
#include "core_functions.h"
#include "generics_kernel.h"
#include "funcs_generics.h"
#include "core_memory.h"
#include "classes_methods_kernel.h"
#include "router.h"
#include "sysdep.h"

#include "funcs_profiling.h"

#include <string.h>

#define NO_PROFILE      0
#define USER_FUNCTIONS  1
#define CONSTRUCTS_CODE 2

#define OUTPUT_STRING "%-40s %7ld %15.6f  %8.2f%%  %15.6f  %8.2f%%\n"

/**************************************
 * LOCAL FUNCTION PROTOTYPES
 ***************************************/

static BOOLEAN OutputProfileInfo(void *, char *, struct constructProfileInfo *, char *, char *, char *, char **);
static void    OutputUserFunctionsInfo(void *);
static void    OutputConstructsCodeInfo(void *);
static void    ProfileClearFunction(void *);

/*****************************************************
 * ConstructProfilingFunctionDefinitions: Initializes
 *   the construct profiling functions.
 ******************************************************/
void ConstructProfilingFunctionDefinitions(void *env)
{
    struct ext_data_record profileDataInfo =
    {
        0, CreateProfileData, DeleteProfileData
    };

    core_allocate_environment_data(env, PROFLFUN_DATA, sizeof(struct profileFunctionData), NULL);

    memcpy(&ProfileFunctionData(env)->ProfileDataInfo, &profileDataInfo, sizeof(struct ext_data_record));

    ProfileFunctionData(env)->LastProfileInfo = NO_PROFILE;
    ProfileFunctionData(env)->PercentThreshold = 0.0;
    ProfileFunctionData(env)->OutputString = OUTPUT_STRING;

    core_define_function(env, "profile", 'v', PTR_FN ProfileCommand, "ProfileCommand", "11w");
    core_define_function(env, "profile-info", 'v', PTR_FN ProfileInfoCommand, "ProfileInfoCommand", "01w");
    core_define_function(env, "profile-reset", 'v', PTR_FN ProfileResetCommand, "ProfileResetCommand", "00");

    core_define_function(env, "set-profile-percent-threshold", 'd',
                         PTR_FN SetProfilePercentThresholdCommand,
                         "SetProfilePercentThresholdCommand", "11n");
    core_define_function(env, "get-profile-percent-threshold", 'd',
                         PTR_FN GetProfilePercentThresholdCommand,
                         "GetProfilePercentThresholdCommand", "00");

    ProfileFunctionData(env)->ProfileDataID = ext_install_record(env, &ProfileFunctionData(env)->ProfileDataInfo);

    core_add_clear_fn(env, "profile", ProfileClearFunction, 0);
}

/*********************************
 * CreateProfileData: Allocates a
 *   profile user data structure.
 **********************************/
void *CreateProfileData(void *env)
{
    struct constructProfileInfo *theInfo;

    theInfo = (struct constructProfileInfo *)
              core_mem_alloc(env, sizeof(struct constructProfileInfo));

    theInfo->numberOfEntries = 0;
    theInfo->childCall = FALSE;
    theInfo->startTime = 0.0;
    theInfo->totalSelfTime = 0.0;
    theInfo->totalWithChildrenTime = 0.0;

    return(theInfo);
}

/*************************************
 * DeleteProfileData:
 **************************************/
void DeleteProfileData(void *env, void *data)
{
    core_mem_free(env, data, sizeof(struct constructProfileInfo));
}

/*************************************
 * ProfileCommand: H/L access routine
 *   for the profile command.
 **************************************/
void ProfileCommand(void *env)
{
    char *argument;
    core_data_object val;

    if( core_check_arg_count(env, "profile", EXACTLY, 1) == -1 )
    {
        return;
    }

    if( core_check_arg_type(env, "profile", 1, ATOM, &val) == FALSE )
    {
        return;
    }

    argument = core_convert_data_to_string(val);

    if( !Profile(env, argument))
    {
        report_explicit_type_error(env, "profile", 1, "symbol with value constructs, user-functions, or off");
        return;
    }

    return;
}

/*****************************
 * Profile: C access routine
 *   for the profile command.
 ******************************/
BOOLEAN Profile(void *env, char *argument)
{
    /*======================================================
     * If the argument is the symbol "user-functions", then
     * user-defined functions should be profiled. If the
     * argument is the symbol "constructs", then
     * deffunctions, generic functions, message-handlers,
     * and rule RHS actions are profiled.
     *======================================================*/

    if( strcmp(argument, "user-functions") == 0 )
    {
        ProfileFunctionData(env)->ProfileStartTime = sysdep_time();
        ProfileFunctionData(env)->ProfileUserFunctions = TRUE;
        ProfileFunctionData(env)->ProfileConstructs = FALSE;
        ProfileFunctionData(env)->LastProfileInfo = USER_FUNCTIONS;
    }

    else if( strcmp(argument, "constructs") == 0 )
    {
        ProfileFunctionData(env)->ProfileStartTime = sysdep_time();
        ProfileFunctionData(env)->ProfileUserFunctions = FALSE;
        ProfileFunctionData(env)->ProfileConstructs = TRUE;
        ProfileFunctionData(env)->LastProfileInfo = CONSTRUCTS_CODE;
    }

    /*======================================================
     * Otherwise, if the argument is the symbol "off", then
     * don't profile constructs and user-defined functions.
     *======================================================*/

    else if( strcmp(argument, "off") == 0 )
    {
        ProfileFunctionData(env)->ProfileEndTime = sysdep_time();
        ProfileFunctionData(env)->ProfileTotalTime += (ProfileFunctionData(env)->ProfileEndTime - ProfileFunctionData(env)->ProfileStartTime);
        ProfileFunctionData(env)->ProfileUserFunctions = FALSE;
        ProfileFunctionData(env)->ProfileConstructs = FALSE;
    }

    /*=====================================================
     * Otherwise, generate an error since the only allowed
     * arguments are "on" or "off."
     *=====================================================*/

    else
    {
        return(FALSE);
    }

    return(TRUE);
}

/*****************************************
 * ProfileInfoCommand: H/L access routine
 *   for the profile-info command.
 ******************************************/
void ProfileInfoCommand(void *env)
{
    int argCount;
    core_data_object val;
    char buffer[512];

    /*===================================
     * The profile-info command expects
     * at most a single symbol argument.
     *===================================*/

    if((argCount = core_check_arg_count(env, "profile", NO_MORE_THAN, 1)) == -1 )
    {
        return;
    }

    /*===========================================
     * The first profile-info argument indicates
     * the field on which sorting is performed.
     *===========================================*/

    if( argCount == 1 )
    {
        if( core_check_arg_type(env, "profile", 1, ATOM, &val) == FALSE )
        {
            return;
        }
    }

    /*==================================
     * If code is still being profiled,
     * update the profile end time.
     *==================================*/

    if( ProfileFunctionData(env)->ProfileUserFunctions || ProfileFunctionData(env)->ProfileConstructs )
    {
        ProfileFunctionData(env)->ProfileEndTime = sysdep_time();
        ProfileFunctionData(env)->ProfileTotalTime += (ProfileFunctionData(env)->ProfileEndTime - ProfileFunctionData(env)->ProfileStartTime);
    }

    /*==================================
     * Print the profiling information.
     *==================================*/

    if( ProfileFunctionData(env)->LastProfileInfo != NO_PROFILE )
    {
        sysdep_sprintf(buffer, "Profile elapsed time = %g seconds\n",
                       ProfileFunctionData(env)->ProfileTotalTime);
        print_router(env, WDISPLAY, buffer);

        if( ProfileFunctionData(env)->LastProfileInfo == USER_FUNCTIONS )
        {
            print_router(env, WDISPLAY, "Function Name                            ");
        }
        else if( ProfileFunctionData(env)->LastProfileInfo == CONSTRUCTS_CODE )
        {
            print_router(env, WDISPLAY, "Construct Name                           ");
        }

        print_router(env, WDISPLAY, "Entries         Time           %          Time+Kids     %+Kids\n");

        if( ProfileFunctionData(env)->LastProfileInfo == USER_FUNCTIONS )
        {
            print_router(env, WDISPLAY, "-------------                            ");
        }
        else if( ProfileFunctionData(env)->LastProfileInfo == CONSTRUCTS_CODE )
        {
            print_router(env, WDISPLAY, "--------------                           ");
        }

        print_router(env, WDISPLAY, "-------        ------        -----        ---------     ------\n");
    }

    if( ProfileFunctionData(env)->LastProfileInfo == USER_FUNCTIONS )
    {
        OutputUserFunctionsInfo(env);
    }

    if( ProfileFunctionData(env)->LastProfileInfo == CONSTRUCTS_CODE )
    {
        OutputConstructsCodeInfo(env);
    }
}

/*********************************************
 * StartProfile: Initiates bookkeeping is_needed
 *   to profile a construct or function.
 **********************************************/
void StartProfile(void *env, struct profileFrameInfo *theFrame, struct ext_data **list, BOOLEAN checkFlag)
{
    double startTime, addTime;
    struct constructProfileInfo *profileInfo;

    if( !checkFlag )
    {
        theFrame->profileOnExit = FALSE;
        return;
    }

    profileInfo = (struct constructProfileInfo *)ext_get_data_item(env, ProfileFunctionData(env)->ProfileDataID, list);

    theFrame->profileOnExit = TRUE;
    theFrame->parentCall = FALSE;

    startTime = sysdep_time();
    theFrame->oldProfileFrame = ProfileFunctionData(env)->ActiveProfileFrame;

    if( ProfileFunctionData(env)->ActiveProfileFrame != NULL )
    {
        addTime = startTime - ProfileFunctionData(env)->ActiveProfileFrame->startTime;
        ProfileFunctionData(env)->ActiveProfileFrame->totalSelfTime += addTime;
    }

    ProfileFunctionData(env)->ActiveProfileFrame = profileInfo;

    ProfileFunctionData(env)->ActiveProfileFrame->numberOfEntries++;
    ProfileFunctionData(env)->ActiveProfileFrame->startTime = startTime;

    if( !ProfileFunctionData(env)->ActiveProfileFrame->childCall )
    {
        theFrame->parentCall = TRUE;
        theFrame->parentStartTime = startTime;
        ProfileFunctionData(env)->ActiveProfileFrame->childCall = TRUE;
    }
}

/******************************************
 * EndProfile: Finishes bookkeeping is_needed
 *   to profile a construct or function.
 *******************************************/
void EndProfile(void *env, struct profileFrameInfo *theFrame)
{
    double endTime, addTime;

    if( !theFrame->profileOnExit )
    {
        return;
    }

    endTime = sysdep_time();

    if( theFrame->parentCall )
    {
        addTime = endTime - theFrame->parentStartTime;
        ProfileFunctionData(env)->ActiveProfileFrame->totalWithChildrenTime += addTime;
        ProfileFunctionData(env)->ActiveProfileFrame->childCall = FALSE;
    }

    ProfileFunctionData(env)->ActiveProfileFrame->totalSelfTime += (endTime - ProfileFunctionData(env)->ActiveProfileFrame->startTime);

    if( theFrame->oldProfileFrame != NULL )
    {
        theFrame->oldProfileFrame->startTime = endTime;
    }

    ProfileFunctionData(env)->ActiveProfileFrame = theFrame->oldProfileFrame;
}

/*****************************************
 * OutputProfileInfo: Prints out a single
 *   line of profile information.
 ******************************************/
static BOOLEAN OutputProfileInfo(void *env, char *itemName, struct constructProfileInfo *profileInfo, char *printPrefixBefore, char *printPrefix, char *printPrefixAfter, char **banner)
{
    double percent = 0.0, percentWithKids = 0.0;
    char buffer[512];

    if( profileInfo == NULL )
    {
        return(FALSE);
    }

    if( profileInfo->numberOfEntries == 0 )
    {
        return(FALSE);
    }

    if( ProfileFunctionData(env)->ProfileTotalTime != 0.0 )
    {
        percent = (profileInfo->totalSelfTime * 100.0) / ProfileFunctionData(env)->ProfileTotalTime;

        if( percent < 0.005 )
        {
            percent = 0.0;
        }

        percentWithKids = (profileInfo->totalWithChildrenTime * 100.0) / ProfileFunctionData(env)->ProfileTotalTime;

        if( percentWithKids < 0.005 )
        {
            percentWithKids = 0.0;
        }
    }

    if( percent < ProfileFunctionData(env)->PercentThreshold )
    {
        return(FALSE);
    }

    if((banner != NULL) && (*banner != NULL))
    {
        print_router(env, WDISPLAY, *banner);
        *banner = NULL;
    }

    if( printPrefixBefore != NULL )
    {
        print_router(env, WDISPLAY, printPrefixBefore);
    }

    if( printPrefix != NULL )
    {
        print_router(env, WDISPLAY, printPrefix);
    }

    if( printPrefixAfter != NULL )
    {
        print_router(env, WDISPLAY, printPrefixAfter);
    }

    if( strlen(itemName) >= 40 )
    {
        print_router(env, WDISPLAY, itemName);
        print_router(env, WDISPLAY, "\n");
        itemName = "";
    }

    sysdep_sprintf(buffer, ProfileFunctionData(env)->OutputString,
                   itemName,
                   (long)profileInfo->numberOfEntries,

                   (double)profileInfo->totalSelfTime,
                   (double)percent,

                   (double)profileInfo->totalWithChildrenTime,
                   (double)percentWithKids);
    print_router(env, WDISPLAY, buffer);

    return(TRUE);
}

/******************************************
 * ProfileResetCommand: H/L access routine
 *   for the profile-reset command.
 *******************************************/
void ProfileResetCommand(void *env)
{
    struct core_function_definition *func;
    int i;

#if DEFFUNCTION_CONSTRUCT
    FUNCTION_DEFINITION *theDeffunction;
#endif
#if DEFGENERIC_CONSTRUCT
    DEFGENERIC *theDefgeneric;
    unsigned int methodIndex;
    DEFMETHOD *theMethod;
#endif
#if OBJECT_SYSTEM
    DEFCLASS *theDefclass;
    HANDLER *theHandler;
    unsigned handlerIndex;
#endif

    ProfileFunctionData(env)->ProfileStartTime = 0.0;
    ProfileFunctionData(env)->ProfileEndTime = 0.0;
    ProfileFunctionData(env)->ProfileTotalTime = 0.0;
    ProfileFunctionData(env)->LastProfileInfo = NO_PROFILE;

    for( func = core_get_function_list(env);
         func != NULL;
         func = func->next )
    {
        ResetProfileInfo((struct constructProfileInfo *)
                         ext_validate_data(ProfileFunctionData(env)->ProfileDataID, func->ext_data));
    }

    for( i = 0; i < MAXIMUM_PRIMITIVES; i++ )
    {
        if( core_get_evaluation_data(env)->primitives[i] != NULL )
        {
            ResetProfileInfo((struct constructProfileInfo *)
                             ext_validate_data(ProfileFunctionData(env)->ProfileDataID, core_get_evaluation_data(env)->primitives[i]->ext_data));
        }
    }

#if DEFFUNCTION_CONSTRUCT

    for( theDeffunction = (FUNCTION_DEFINITION *)get_next_function(env, NULL);
         theDeffunction != NULL;
         theDeffunction = (FUNCTION_DEFINITION *)get_next_function(env, theDeffunction))
    {
        ResetProfileInfo((struct constructProfileInfo *)
                         ext_validate_data(ProfileFunctionData(env)->ProfileDataID, theDeffunction->header.ext_data));
    }

#endif


#if DEFGENERIC_CONSTRUCT

    for( theDefgeneric = (DEFGENERIC *)EnvGetNextDefgeneric(env, NULL);
         theDefgeneric != NULL;
         theDefgeneric = (DEFGENERIC *)EnvGetNextDefgeneric(env, theDefgeneric))
    {
        ResetProfileInfo((struct constructProfileInfo *)
                         ext_validate_data(ProfileFunctionData(env)->ProfileDataID, theDefgeneric->header.ext_data));

        for( methodIndex = EnvGetNextDefmethod(env, theDefgeneric, 0);
             methodIndex != 0;
             methodIndex = EnvGetNextDefmethod(env, theDefgeneric, methodIndex))
        {
            theMethod = GetDefmethodPointer(theDefgeneric, methodIndex);
            ResetProfileInfo((struct constructProfileInfo *)
                             ext_validate_data(ProfileFunctionData(env)->ProfileDataID, theMethod->ext_data));
        }
    }

#endif

#if OBJECT_SYSTEM

    for( theDefclass = (DEFCLASS *)EnvGetNextDefclass(env, NULL);
         theDefclass != NULL;
         theDefclass = (DEFCLASS *)EnvGetNextDefclass(env, theDefclass))
    {
        ResetProfileInfo((struct constructProfileInfo *)
                         ext_validate_data(ProfileFunctionData(env)->ProfileDataID, theDefclass->header.ext_data));

        for( handlerIndex = EnvGetNextDefmessageHandler(env, theDefclass, 0);
             handlerIndex != 0;
             handlerIndex = EnvGetNextDefmessageHandler(env, theDefclass, handlerIndex))
        {
            theHandler = GetDefmessageHandlerPointer(theDefclass, handlerIndex);
            ResetProfileInfo((struct constructProfileInfo *)
                             ext_validate_data(ProfileFunctionData(env)->ProfileDataID, theHandler->ext_data));
        }
    }

#endif
}

/************************************************
 * ResetProfileInfo: Sets the initial values for
 *   a constructProfileInfo data structure.
 *************************************************/
void ResetProfileInfo(struct constructProfileInfo *profileInfo)
{
    if( profileInfo == NULL )
    {
        return;
    }

    profileInfo->numberOfEntries = 0;
    profileInfo->childCall = FALSE;
    profileInfo->startTime = 0.0;
    profileInfo->totalSelfTime = 0.0;
    profileInfo->totalWithChildrenTime = 0.0;
}

/************************************************
 * OutputUserFunctionsInfo:
 *************************************************/
static void OutputUserFunctionsInfo(void *env)
{
    struct core_function_definition *func;
    int i;

    for( func = core_get_function_list(env);
         func != NULL;
         func = func->next )
    {
        OutputProfileInfo(env, to_string(func->function_handle),
                          (struct constructProfileInfo *)
                          ext_validate_data(ProfileFunctionData(env)->ProfileDataID,
                                            func->ext_data),
                          NULL, NULL, NULL, NULL);
    }

    for( i = 0; i < MAXIMUM_PRIMITIVES; i++ )
    {
        if( core_get_evaluation_data(env)->primitives[i] != NULL )
        {
            OutputProfileInfo(env, core_get_evaluation_data(env)->primitives[i]->name,
                              (struct constructProfileInfo *)
                              ext_validate_data(ProfileFunctionData(env)->ProfileDataID,
                                                core_get_evaluation_data(env)->primitives[i]->ext_data),
                              NULL, NULL, NULL, NULL);
        }
    }
}

/************************************************
 * OutputConstructsCodeInfo:
 *************************************************/
#if WIN_BTC && (!DEFFUNCTION_CONSTRUCT) && (!DEFGENERIC_CONSTRUCT) && (!OBJECT_SYSTEM)
#pragma argsused
#endif
static void OutputConstructsCodeInfo(void *env)
{
#if (!DEFFUNCTION_CONSTRUCT) && (!DEFGENERIC_CONSTRUCT) && (!OBJECT_SYSTEM)
#pragma unused(env)
#endif
#if DEFFUNCTION_CONSTRUCT
    FUNCTION_DEFINITION *theDeffunction;
#endif
#if DEFGENERIC_CONSTRUCT
    DEFGENERIC *theDefgeneric;
    DEFMETHOD *theMethod;
    unsigned methodIndex;
    char methodBuffer[512];
#endif
#if OBJECT_SYSTEM
    DEFCLASS *theDefclass;
    HANDLER *theHandler;
    unsigned handlerIndex;
#endif
#if DEFGENERIC_CONSTRUCT || OBJECT_SYSTEM
    char *prefix, *prefixBefore, *prefixAfter;
#endif
    char *banner;

    banner = "\n*** Deffunctions ***\n\n";

#if DEFFUNCTION_CONSTRUCT

    for( theDeffunction = (FUNCTION_DEFINITION *)get_next_function(env, NULL);
         theDeffunction != NULL;
         theDeffunction = (FUNCTION_DEFINITION *)get_next_function(env, theDeffunction))
    {
        OutputProfileInfo(env, get_function_name(env, theDeffunction),
                          (struct constructProfileInfo *)
                          ext_validate_data(ProfileFunctionData(env)->ProfileDataID, theDeffunction->header.ext_data),
                          NULL, NULL, NULL, &banner);
    }

#endif

    banner = "\n*** Defgenerics ***\n";
#if DEFGENERIC_CONSTRUCT

    for( theDefgeneric = (DEFGENERIC *)EnvGetNextDefgeneric(env, NULL);
         theDefgeneric != NULL;
         theDefgeneric = (DEFGENERIC *)EnvGetNextDefgeneric(env, theDefgeneric))
    {
        prefixBefore = "\n";
        prefix = EnvGetDefgenericName(env, theDefgeneric);
        prefixAfter = "\n";

        for( methodIndex = EnvGetNextDefmethod(env, theDefgeneric, 0);
             methodIndex != 0;
             methodIndex = EnvGetNextDefmethod(env, theDefgeneric, methodIndex))
        {
            theMethod = GetDefmethodPointer(theDefgeneric, methodIndex);

            EnvGetDefmethodDescription(env, methodBuffer, 510, theDefgeneric, methodIndex);

            if( OutputProfileInfo(env, methodBuffer,
                                  (struct constructProfileInfo *)
                                  ext_validate_data(ProfileFunctionData(env)->ProfileDataID, theMethod->ext_data),
                                  prefixBefore, prefix, prefixAfter, &banner))
            {
                prefixBefore = NULL;
                prefix = NULL;
                prefixAfter = NULL;
            }
        }
    }

#endif

    banner = "\n*** Defclasses ***\n";
#if OBJECT_SYSTEM

    for( theDefclass = (DEFCLASS *)EnvGetNextDefclass(env, NULL);
         theDefclass != NULL;
         theDefclass = (DEFCLASS *)EnvGetNextDefclass(env, theDefclass))
    {
        prefixAfter = "\n";
        prefix = EnvGetDefclassName(env, theDefclass);
        prefixBefore = "\n";

        for( handlerIndex = EnvGetNextDefmessageHandler(env, theDefclass, 0);
             handlerIndex != 0;
             handlerIndex = EnvGetNextDefmessageHandler(env, theDefclass, handlerIndex))
        {
            theHandler = GetDefmessageHandlerPointer(theDefclass, handlerIndex);

            if( OutputProfileInfo(env, EnvGetDefmessageHandlerName(env, theDefclass, handlerIndex),
                                  (struct constructProfileInfo *)
                                  ext_validate_data(ProfileFunctionData(env)->ProfileDataID,
                                                    theHandler->ext_data),
                                  prefixBefore, prefix, prefixAfter, &banner))
            {
                prefixBefore = NULL;
                prefix = NULL;
                prefixAfter = NULL;
            }
        }
    }

#endif

    banner = "\n*** Defrules ***\n\n";
}

/********************************************************
 * SetProfilePercentThresholdCommand: H/L access routine
 *   for the set-profile-percent-threshold command.
 *********************************************************/
double SetProfilePercentThresholdCommand(void *env)
{
    core_data_object val;
    double newThreshold;

    if( core_check_arg_count(env, "set-profile-percent-threshold", EXACTLY, 1) == -1 )
    {
        return(ProfileFunctionData(env)->PercentThreshold);
    }

    if( core_check_arg_type(env, "set-profile-percent-threshold", 1, INTEGER_OR_FLOAT, &val) == FALSE )
    {
        return(ProfileFunctionData(env)->PercentThreshold);
    }

    if( core_get_type(val) == INTEGER )
    {
        newThreshold = (double)core_convert_data_to_long(val);
    }
    else
    {
        newThreshold = (double)core_convert_data_to_double(val);
    }

    if((newThreshold < 0.0) || (newThreshold > 100.0))
    {
        report_explicit_type_error(env, "set-profile-percent-threshold", 1,
                                   "number in the range 0 to 100");
        return(-1.0);
    }

    return(SetProfilePercentThreshold(env, newThreshold));
}

/***************************************************
 * SetProfilePercentThreshold: C access routine for
 *   the set-profile-percent-threshold command.
 ****************************************************/
double SetProfilePercentThreshold(void *env, double value)
{
    double oldPercentThreshhold;

    if((value < 0.0) || (value > 100.0))
    {
        return(-1.0);
    }

    oldPercentThreshhold = ProfileFunctionData(env)->PercentThreshold;

    ProfileFunctionData(env)->PercentThreshold = value;

    return(oldPercentThreshhold);
}

/********************************************************
 * GetProfilePercentThresholdCommand: H/L access routine
 *   for the get-profile-percent-threshold command.
 *********************************************************/
double GetProfilePercentThresholdCommand(void *env)
{
    core_check_arg_count(env, "get-profile-percent-threshold", EXACTLY, 0);

    return(ProfileFunctionData(env)->PercentThreshold);
}

/***************************************************
 * GetProfilePercentThreshold: C access routine for
 *   the get-profile-percent-threshold command.
 ****************************************************/
double GetProfilePercentThreshold(void *env)
{
    return(ProfileFunctionData(env)->PercentThreshold);
}

/*********************************************************
 * SetProfileOutputString: Sets the output string global.
 **********************************************************/
char *SetProfileOutputString(void *env, char *value)
{
    char *oldOutputString;

    if( value == NULL )
    {
        return(ProfileFunctionData(env)->OutputString);
    }

    oldOutputString = ProfileFunctionData(env)->OutputString;

    ProfileFunctionData(env)->OutputString = value;

    return(oldOutputString);
}

/*****************************************************************
 * ProfileClearFunction: Profiling clear routine for use with the
 *   clear command. Removes user data attached to user functions.
 ******************************************************************/
static void ProfileClearFunction(void *env)
{
    struct core_function_definition *func;
    int i;

    for( func = core_get_function_list(env);
         func != NULL;
         func = func->next )
    {
        func->ext_data =
            ext_delete_data_item(env, ProfileFunctionData(env)->ProfileDataID, func->ext_data);
    }

    for( i = 0; i < MAXIMUM_PRIMITIVES; i++ )
    {
        if( core_get_evaluation_data(env)->primitives[i] != NULL )
        {
            core_get_evaluation_data(env)->primitives[i]->ext_data =
                ext_delete_data_item(env, ProfileFunctionData(env)->ProfileDataID, core_get_evaluation_data(env)->primitives[i]->ext_data);
        }
    }
}

#endif /* PROFILING_FUNCTIONS */
