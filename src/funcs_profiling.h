#ifndef __FUNCS_PROFILING_H__
#define __FUNCS_PROFILING_H__

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef __FUNCS_PROFILING_SOURCE__
#define LOCALE
#else
#define LOCALE extern
#endif

#include "extensions_data.h"
#if PROFILING_FUNCTIONS
struct constructProfileInfo
{
    struct ext_data ext_datum;
    long            numberOfEntries;
    unsigned int    childCall :
    1;
    double startTime;
    double totalSelfTime;
    double totalWithChildrenTime;
};

struct profileFrameInfo
{
    unsigned int parentCall :
    1;
    unsigned int profileOnExit :
    1;
    double                       parentStartTime;
    struct constructProfileInfo *oldProfileFrame;
};

#define PROFLFUN_DATA 15

struct profileFunctionData
{
    double                       ProfileStartTime;
    double                       ProfileEndTime;
    double                       ProfileTotalTime;
    int                          LastProfileInfo;
    double                       PercentThreshold;
    struct ext_data_record       ProfileDataInfo;
    unsigned char                ProfileDataID;
    int                          ProfileUserFunctions;
    int                          ProfileConstructs;
    struct constructProfileInfo *ActiveProfileFrame;
    char *                       OutputString;
};

#define ProfileFunctionData(env) ((struct profileFunctionData *)core_get_environment_data(env, PROFLFUN_DATA))

LOCALE void ConstructProfilingFunctionDefinitions(void *);
LOCALE void ProfileCommand(void *);
LOCALE void ProfileInfoCommand(void *);
LOCALE void StartProfile(void *,
                         struct profileFrameInfo *,
                         struct ext_data **,
                         BOOLEAN);
LOCALE void EndProfile(void *, struct profileFrameInfo *);
LOCALE void ProfileResetCommand(void *);
LOCALE void ResetProfileInfo(struct constructProfileInfo *);

LOCALE double                          SetProfilePercentThresholdCommand(void *);
LOCALE double                          SetProfilePercentThreshold(void *, double);
LOCALE double                          GetProfilePercentThresholdCommand(void *);
LOCALE double                          GetProfilePercentThreshold(void *);
LOCALE BOOLEAN                         Profile(void *, char *);
LOCALE void                            DeleteProfileData(void *, void *);
LOCALE void                          * CreateProfileData(void *);
LOCALE char                          * SetProfileOutputString(void *, char *);

#endif
#endif
