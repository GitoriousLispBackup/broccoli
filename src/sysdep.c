/* Purpose: Isolation of system dependent routines.          */

#define _ARCH_SOURCE_

#include "setup.h"

#include <stdio.h>
#define _STDIO_INCLUDED_
#include <string.h>

#include <stdlib.h>
#include <time.h>
#include <stdarg.h>

#if   VAX_VMS
#include timeb
#include <descrip.h>
#include <ssdef.h>
#include <stsdef.h>
#include signal
extern int LIB $ SPAWN();
#endif

#if MAC_MCW || MAC_XCD
#include <Carbon/Carbon.h>
#define kTwoPower32 (4294967296.0)      /* 2^32 */
#endif

#if MAC_MCW || MAC_XCD
#include <strings.h>
#endif

#if MAC_MCW || WIN_MCW || MAC_XCD
#include <unistd.h>
#endif

#if WIN_MVC
#include <sys\types.h>
#include <sys\timeb.h>
#include <io.h>
#include <fcntl.h>
#include <limits.h>
#include <process.h>
#include <signal.h>
#endif

#if WIN_BTC
#include <io.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#endif

#if WIN_MCW
#include <io.h>
#include <limits.h>
#endif

#if   UNIX_7 || WIN_GCC
#include <sys/types.h>
#include <sys/timeb.h>
#include <signal.h>
#endif

#if   UNIX_V || LINUX || DARWIN
#include <sys/types.h>
#include <sys/time.h>
#include <sys/times.h>
#include <unistd.h>
#include <signal.h>
#endif

#include "core_arguments.h"
#include "funcs_math_basic.h"
#include "core_command_prompt.h"
#include "constraints_kernel.h"
#include "core_constructs.h"
#include "parser_constructs.h"
#include "core_environment.h"
#include "funcs_io_basic.h"
#include "core_memory.h"
#include "funcs_misc.h"
#include "type_list.h"
#include "funcs_list.h"
#include "core_functions_util.h"
#include "funcs_predicate.h"
#include "funcs_comparison.h"
#include "funcs_logic.h"
#include "funcs_profiling.h"
#include "funcs_flow_control.h"
#include "router.h"
#include "funcs_sorting.h"
#include "funcs_string.h"
#include "core_gc.h"
#include "core_watch.h"

#include "sysdep.h"


#if DEFGENERIC_CONSTRUCT
#include "generics_kernel.h"
#endif

#if DEFFUNCTION_CONSTRUCT
#include "funcs_function.h"
#endif

#if OBJECT_SYSTEM
#include "classes_init.h"
#endif

#if META_SYSTEM
#include "funcs_meta.h"
#endif

#include "modules_init.h"

#if DEVELOPER
#include "extensions_developer.h"
#endif

/**************
 * DEFINITIONS
 ***************/

#define NO_SWITCH         0
#define BATCH_SWITCH      1
#define BATCH_STAR_SWITCH 2
#define LOAD_SWITCH       3

/*******************
 * ENVIRONMENT DATA
 ********************/

#define SYSTEM_DEPENDENT_DATA_INDEX 58

struct sysdep_data
{
    void (*fn_redraw)(void *);
    void (*fn_pause)(void *);
    void (*fn_continue)(void *, int);

#if WIN_BTC || WIN_MVC
    int BinaryFileHandle;
#endif
#if (!WIN_BTC) && (!WIN_MVC)
    FILE *binary_file_ptr;
#endif
    int      (*fn_before_open)(void *);
    int      (*fn_after_open)(void *);
    jmp_buf *jump_buffer;
};

#define get_sysdep_data(env) ((struct sysdep_data *)core_get_environment_data(env, SYSTEM_DEPENDENT_DATA_INDEX))

/***************************************
 * GLOBAL EXTERNAL FUNCTION DEFINITIONS
 ****************************************/

extern void ext_function_hook(void);
extern void ext_environment_aware_function_hook(void *);

/**************************************
 * LOCAL FUNCTION PROTOTYPES
 ***************************************/

static void _init_sysdep_data(void *);
static void _system_function_definitions(void *);
static void _init_keywords(void *);
static void _init_non_portables(void *);
#if   (VAX_VMS || UNIX_V || LINUX || DARWIN || UNIX_7 || WIN_GCC || WIN_BTC || WIN_MVC)
static void _catch_ctrl_c(int);
#endif

/*******************************************************
 * InitializeSystemDependentData: Allocates environment
 *    data for system dependent routines.
 ********************************************************/
static void _init_sysdep_data(void *env)
{
    core_allocate_environment_data(env, SYSTEM_DEPENDENT_DATA_INDEX, sizeof(struct sysdep_data), NULL);
}

/****************************************************
 * init_system: Performs initialization
 *   of the KB environment.
 *****************************************************/
void init_system(void *venvironment, struct atom_hash_node **symbolTable, struct float_hash_node **floatTable, struct integer_hash_node **integerTable, struct bitmap_hash_node **bitmapTable, struct external_address_hash_node **externalAddressTable)
{
    struct core_environment *environment = (struct core_environment *)venvironment;

    /*================================================
     * Don't allow the initialization to occur twice.
     *================================================*/

    if( environment->initialized )
    {
        return;
    }

    /*================================
     * Initialize the memory manager.
     *================================*/

    core_mem_init_memory(environment);

    /*===================================================
     * Initialize environment data for various features.
     *===================================================*/

    core_init_command_prompt(environment);
    core_init_constructs(environment);
    core_init_evaluation_data(environment);
    core_init_function_data(environment);
    init_list_data(environment);
    core_init_pp_buffer(environment);
    core_init_utility_data(environment);
    core_init_scanner_data(environment);
    _init_sysdep_data(environment);
    ext_init_data(environment);
    core_init_gc_data(environment);
#if DEBUGGING_FUNCTIONS
    InitializeWatchData(environment);
#endif

    /*===============================================
     * Initialize the hash tables for atomic values.
     *===============================================*/

    init_symbol_tables(environment, symbolTable, floatTable, integerTable, bitmapTable, externalAddressTable);

    /*=========================================
     * Initialize file and string I/O routers.
     *=========================================*/

    init_routers(environment);

    /*=========================================================
     * Initialize some system dependent features such as time.
     *=========================================================*/

    _init_non_portables(environment);

    /*=============================================
     * Register system and user defined functions.
     *=============================================*/

    _system_function_definitions(environment);
    ext_function_hook();
    ext_environment_aware_function_hook(environment);

    /*====================================
     * Initialize the constraint manager.
     *====================================*/

    init_constraints(environment);

    /*==========================================
     * Initialize the expression hash table and
     * pointers to specific functions.
     *==========================================*/

    core_init_expression_data(environment);

    /*===================================
     * Initialize the construct manager.
     *===================================*/

    core_init_constructs_manager(environment);

    /*=====================================
     * Initialize the module_definition construct.
     *=====================================*/

    init_module_data(environment);

    /*=====================================================
     * Initialize the defgeneric and defmethod constructs.
     *=====================================================*/

#if DEFGENERIC_CONSTRUCT
    SetupGenericFunctions(environment);
#endif

    /*=======================================
     * Initialize the deffunction construct.
     *=======================================*/

#if DEFFUNCTION_CONSTRUCT
    init_functions(environment);
#endif

    /*=============================
     * Initialize COOL constructs.
     *=============================*/

#if OBJECT_SYSTEM
    SetupObjectSystem(environment);
#endif

    /*=====================================
     * Initialize the module_definition construct.
     *=====================================*/

    init_modules(environment);

    /*======================================================
     * Register commands and functions for development use.
     *======================================================*/

#if DEVELOPER
    DeveloperCommands(environment);
#endif

    /*=========================================
     * Install the special function primitives
     * used by procedural code in constructs.
     *=========================================*/

    core_install_function_primitive(environment);

    /*==============================================
     * Install keywords in the symbol table so that
     * they are available for command completion.
     *==============================================*/
    _init_keywords(environment);

    /*========================
     * Issue a clear command.
     *========================*/

    core_clear(environment);

    /*=============================
     * Initialization is complete.
     *=============================*/

    environment->initialized = TRUE;
}

/************************************************
 * sysdep_route_stdin: Processes the -f, -f2, and -l
 *   options available on machines which support
 *   argc and arv command line options.
 *************************************************/
int sysdep_route_stdin(void *env, int argc, char *argv[])
{
    int i;
    int theSwitch = NO_SWITCH;
    int cl = TRUE;

    /*======================================
     * If there aren't enough arguments for
     * the -f argument, then return.
     *======================================*/

    if( argc < 3 )
    {
        return(cl);
    }

    /*=====================================
     * If argv was not passed then return.
     *=====================================*/

    if( argv == NULL )
    {
        return(cl);
    }

    /*=============================================
     * Process each of the command line arguments.
     *=============================================*/

    for( i = 1 ; i < argc ; i++ )
    {
        if( strcmp(argv[i], "-f") == 0 )
        {
            theSwitch = BATCH_SWITCH;
            cl = FALSE;
        }

        else if( strcmp(argv[i], "-f2") == 0 )
        {
            theSwitch = BATCH_STAR_SWITCH;
        }
        else if( strcmp(argv[i], "-l") == 0 )
        {
            theSwitch = LOAD_SWITCH;
        }
        else if( theSwitch == NO_SWITCH )
        {
            error_print_id(env, "ARCH", 2, FALSE);
            print_router(env, WERROR, "Invalid option\n");
        }

        if( i > (argc - 1))
        {
            error_print_id(env, "ARCH", 1, FALSE);
            print_router(env, WERROR, "No file found for ");

            switch( theSwitch )
            {
            case BATCH_SWITCH:
                print_router(env, WERROR, "-f");
                break;

            case BATCH_STAR_SWITCH:
                print_router(env, WERROR, "-f2");
                break;

            case LOAD_SWITCH:
                print_router(env, WERROR, "-l");
            }

            print_router(env, WERROR, " option\n");
            return(cl);
        }

        switch( theSwitch )
        {
        case BATCH_SWITCH:
            io_open_batch(env, argv[++i], TRUE);
            break;

        case BATCH_STAR_SWITCH:
            broccoli_run_silent(env, argv[++i]);
            break;

        case LOAD_SWITCH:
            load_parse(env, argv[++i]);
            break;
        }
    }

    return(cl);
}

/*************************************************
 * SystemFunctionDefinitions: Sets up definitions
 *   of system defined functions.
 **************************************************/
static void _system_function_definitions(void *env)
{
    func_init_flow_control(env);
    init_misc_functions(env);

    init_io_all_functions(env);

    init_predicate_functions(env);
    init_logic_functions(env);
    func_init_comparisons(env);
    init_basic_math_functions(env);
    init_io_functions(env);
    init_sort_functions(env);

#if META_SYSTEM
    init_meta_functions(env);
#endif

#if DEBUGGING_FUNCTIONS
    WatchFunctionDefinitions(env);
#endif

#if LIST_FUNCTIONS
    init_list_functions(env);
#endif

#if STRING_FUNCTIONS
    StringFunctionDefinitions(env);
#endif

#if EXTENDED_MATH_FUNCTIONS
    ExtendedMathFunctionDefinitions(env);
#endif


#if PROFILING_FUNCTIONS
    ConstructProfilingFunctionDefinitions(env);
#endif
}

/********************************************************
 * sysdep_time: A function to return a floating point number
 *   which indicates the present time. Used internally
 *   for timing rule firings and debugging.
 *********************************************************/
double sysdep_time()
{
    return((double)clock() / (double)CLOCKS_PER_SEC);
}

/****************************************************
 * sysdep_system: Generic routine for passing a string
 *   representing a command to the operating system.
 *****************************************************/
void sysdep_system(void *env)
{
#if OFF
    char *commandBuffer = NULL;
    size_t bufferPosition = 0;
    size_t bufferMaximum = 0;
    int numa, i;
    core_data_object tempValue;
    char *theString;

    /*===========================================
     * Check for the corret number of arguments.
     *===========================================*/

    if((numa = core_check_arg_count(env, "system", AT_LEAST, 1)) == -1 )
    {
        return;
    }

    /*============================================================
     * Concatenate the arguments together to form a single string
     * containing the command to be sent to the operating system.
     *============================================================*/

    for( i = 1 ; i <= numa; i++ )
    {
        core_get_arg_at(env, i, &tempValue);

        if((core_get_type(tempValue) != STRING) &&
           (core_get_type(tempValue) != ATOM))
        {
            core_set_halt_eval(env, TRUE);
            core_set_eval_error(env, TRUE);
            report_implicit_type_error(env, "system", i);
            return;
        }

        theString = core_convert_data_to_string(tempValue);

        commandBuffer = core_gc_append_to_string(env, theString, commandBuffer, &bufferPosition, &bufferMaximum);
    }

    if( commandBuffer == NULL )
    {
        return;
    }

    /*=======================================
     * Execute the operating system command.
     *=======================================*/

#if VAX_VMS

    if( get_sysdep_data(env)->fn_pause != NULL )
    {
        (*get_sysdep_data(env)->fn_pause)(env);
    }

    VMSSystem(commandBuffer);
    putchar('\n');

    if( get_sysdep_data(env)->fn_continue != NULL )
    {
        (*get_sysdep_data(env)->fn_continue)(env, 1);
    }

    if( get_sysdep_data(env)->fn_redraw != NULL )
    {
        (*get_sysdep_data(env)->fn_redraw)(env);
    }

#endif

#if   UNIX_7 || UNIX_V || LINUX || DARWIN || WIN_MVC || WIN_BTC || WIN_MCW || WIN_GCC || MAC_XCD

    if( get_sysdep_data(env)->fn_pause != NULL )
    {
        (*get_sysdep_data(env)->fn_pause)(env);
    }

    system(commandBuffer);

    if( get_sysdep_data(env)->fn_continue != NULL )
    {
        (*get_sysdep_data(env)->fn_continue)(env, 1);
    }

    if( get_sysdep_data(env)->fn_redraw != NULL )
    {
        (*get_sysdep_data(env)->fn_redraw)(env);
    }

#else

#if !VAX_VMS
    print_router(env, WDIALOG,
                 "System function not fully defined for this system.\n");
#endif

#endif

    /*==================================================
     * Return the string buffer containing the command.
     *==================================================*/

    core_mem_release(env, commandBuffer, bufferMaximum);

    return;

#endif
}

#if   VAX_VMS

/************************************************
 * VMSSystem: Implements system command for VMS.
 *************************************************/
void VMSSystem(char *cmd)
{
    long status, complcode;

    struct dsc $ descriptor_s cmd_desc;

    cmd_desc.dsc $ w_length = strlen(cmd);
    cmd_desc.dsc $ a_pointer = cmd;
    cmd_desc.dsc $ b_class = DSC $ K_CLASS_S;
    cmd_desc.dsc $ b_dtype = DSC $ K_DTYPE_T;

    status = LIB $ SPAWN(&cmd_desc, 0, 0, 0, 0, 0, &complcode, 0, 0, 0);
}

#endif

/**********************************************************
 * InitializeNonportableFeatures: Initializes non-portable
 *   features. Currently, the only non-portable feature
 *   requiring initialization is the interrupt handler
 *   which allows execution to be halted.
 ***********************************************************/
#if WIN_BTC
#pragma argsused
#endif
static void _init_non_portables(void *env)
{
#if MAC_MCW || WIN_MCW || MAC_XCD
#pragma unused(env)
#endif

#if VAX_VMS || UNIX_V || LINUX || DARWIN || UNIX_7 || WIN_GCC || WIN_BTC || WIN_MVC
    signal(SIGINT, _catch_ctrl_c);
#endif

    /*
     #if WIN_BTC
     * get_sysdep_data(env)->OldCtrlC = getvect(0x23);
     * get_sysdep_data(env)->OldBreak = getvect(0x1b);
     * setvect(0x23,CatchCtrlC);
     * setvect(0x1b,CatchCtrlC);
     * atexit(RestoreInterruptVectors);
     #endif
     */

    /*
     #if WIN_MVC
     * get_sysdep_data(env)->OldCtrlC = _dos_getvect(0x23);
     * get_sysdep_data(env)->OldBreak = _dos_getvect(0x1b);
     * _dos_setvect(0x23,CatchCtrlC);
     * _dos_setvect(0x1b,CatchCtrlC);
     * atexit(RestoreInterruptVectors);
     #endif
     */
}

/************************************************************
 * Functions For Handling Control C Interrupt: The following
 *   functions handle interrupt processing for several
 *   machines. For the Macintosh control-c is not handle,
 *   but a function is provided to call periodically which
 *   calls SystemTask (allowing periodic tasks to be handled
 *   by the operating system).
 *************************************************************/


#if   VAX_VMS || UNIX_V || LINUX || DARWIN || UNIX_7 || WIN_GCC || WIN_BTC || WIN_MVC || DARWIN

/*********************************************
 * CatchCtrlC: VMS and UNIX specific function
 *   to allow control-c interrupts.
 **********************************************/
#if WIN_BTC
#pragma argsused
#endif
static void _catch_ctrl_c(int sgnl)
{
    signal(SIGINT, _catch_ctrl_c);
}

#endif

/*************************************
 * sysdep_exit:  A generic exit function.
 **************************************/
void sysdep_exit(void *env, int num)
{
    if( get_sysdep_data(env)->jump_buffer != NULL )
    {
        longjmp(*get_sysdep_data(env)->jump_buffer, 1);
    }

    exit(num);
}

/*************************************
 * sysdep_set_jump_buffer:
 **************************************/
void sysdep_set_jump_buffer(void *env, jmp_buf *theJmpBuffer)
{
    get_sysdep_data(env)->jump_buffer = theJmpBuffer;
}

/*****************************************
 * sysdep_strcpy: Generic sysdep_strcpy function.
 ******************************************/
char *sysdep_strcpy(char *dest, const char *src)
{
    return(strcpy(dest, src));
}

/*******************************************
 * sysdep_strncpy: Generic sysdep_strncpy function.
 ********************************************/
char *sysdep_strncpy(char *dest, const char *src, size_t n)
{
    return(strncpy(dest, src, n));
}

/*****************************************
 * sysdep_strcat: Generic sysdep_strcat function.
 ******************************************/
char *sysdep_strcat(char *dest, const char *src)
{
    return(strcat(dest, src));
}

/*******************************************
 * sysdep_strncat: Generic sysdep_strncat function.
 ********************************************/
char *sysdep_strncat(char *dest, const char *src, size_t n)
{
    return(strncat(dest, src, n));
}

/****************************************
 * sysdep_sprintf: Generic sprintf function.
 *****************************************/
int sysdep_sprintf(char *buffer, const char *restrictStr, ...)
{
    va_list args;
    int rv;

    va_start(args, restrictStr);

    rv = vsprintf(buffer, restrictStr, args);

    va_end(args);

    return(rv);
}

/*****************************************************
 * sysdep_rand: Generic random number generator function.
 ******************************************************/
int sysdep_rand()
{
    return(rand());
}

/*********************************************************************
 * sysdep_seed_rand: Generic function for seeding the random number generator.
 **********************************************************************/
void sysdep_seed_rand(int seed)
{
    srand((unsigned)seed);
}

/********************************************
 * sysdep_pwd: Generic function for returning
 *   the current directory.
 *********************************************/
#if WIN_BTC
#pragma argsused
#endif
char *sysdep_pwd(char *buffer, int buflength)
{
#if OFF
#if MAC_MCW || WIN_MCW || MAC_XCD
    return(getcwd(buffer, buflength));

#endif

    if( buffer != NULL )
    {
        buffer[0] = 0;
    }

#endif
    return(buffer);
}

/***************************************************
 * sysdep_remove_file: Generic function for removing a file.
 ****************************************************/
int sysdep_remove_file(char *fileName)
{
    if( remove(fileName))
    {
        return(FALSE);
    }

    return(TRUE);
}

/***************************************************
 * sysdep_rename_file: Generic function for renaming a file.
 ****************************************************/
int sysdep_rename_file(char *oldFileName, char *newFileName)
{
    if( rename(oldFileName, newFileName))
    {
        return(FALSE);
    }

    return(TRUE);
}

/*************************************
 * sysdep_set_before_open_fn: Sets the
 *  value of fn_before_open.
 **************************************/
int(*sysdep_set_before_open_fn(void *env,
                               int (*func)(void *))) (void *)
{
    int (*tempFunction)(void *);

    tempFunction = get_sysdep_data(env)->fn_before_open;
    get_sysdep_data(env)->fn_before_open = func;
    return(tempFunction);
}

/************************************
 * sysdep_set_after_open_fn: Sets the
 *  value of fn_after_open.
 *************************************/
int(*sysdep_set_after_open_fn(void *env, int (*func)(void *))) (void *)
{
    int (*tempFunction)(void *);

    tempFunction = get_sysdep_data(env)->fn_after_open;
    get_sysdep_data(env)->fn_after_open = func;
    return(tempFunction);
}

/********************************************
 * sysdep_open_file: Trap routine for opening a file.
 *********************************************/
FILE *sysdep_open_file(void *env, char *fileName, char *accessType)
{
    FILE *theFile;

    if( get_sysdep_data(env)->fn_before_open != NULL )
    {
        (*get_sysdep_data(env)->fn_before_open)(env);
    }

#if WIN_MVC
#if _MSC_VER >= 1400
    fopen_s(&theFile, fileName, accessType);
#else
    theFile = fopen(fileName, accessType);
#endif
#else
    theFile = fopen(fileName, accessType);
#endif

    if( get_sysdep_data(env)->fn_after_open != NULL )
    {
        (*get_sysdep_data(env)->fn_after_open)(env);
    }

    return(theFile);
}

/*********************************************
 * sysdep_close_file: Trap routine for closing a file.
 **********************************************/
int sysdep_close_file(void *env, FILE *theFile)
{
    int rv;

    if( get_sysdep_data(env)->fn_before_open != NULL )
    {
        (*get_sysdep_data(env)->fn_before_open)(env);
    }

    rv = fclose(theFile);

    if( get_sysdep_data(env)->fn_after_open != NULL )
    {
        (*get_sysdep_data(env)->fn_after_open)(env);
    }

    return(rv);
}

/***********************************************************
 * sysdep_open_r_binary: Generic and machine specific code for
 *   opening a file for binary access. Only one file may be
 *   open at a time when using this function since the file
 *   pointer is stored in a global variable.
 ************************************************************/
int sysdep_open_r_binary(void *env, char *funcName, char *fileName)
{
#if OFF

    if( get_sysdep_data(env)->fn_before_open != NULL )
    {
        (*get_sysdep_data(env)->fn_before_open)(env);
    }

#if WIN_BTC || WIN_MVC

#if WIN_MVC
    get_sysdep_data(env)->BinaryFileHandle = _open(fileName, O_RDONLY | O_BINARY);
#else
    get_sysdep_data(env)->BinaryFileHandle = open(fileName, O_RDONLY | O_BINARY);
#endif

    if( get_sysdep_data(env)->BinaryFileHandle == -1 )
    {
        if( get_sysdep_data(env)->fn_after_open != NULL )
        {
            (*get_sysdep_data(env)->fn_after_open)(env);
        }

        report_file_open_error(env, funcName, fileName);
        return(FALSE);
    }

#endif

#if (!WIN_BTC) && (!WIN_MVC)

    if((get_sysdep_data(env)->binary_file_ptr = fopen(fileName, "rb")) == NULL )
    {
        if( get_sysdep_data(env)->fn_after_open != NULL )
        {
            (*get_sysdep_data(env)->fn_after_open)(env);
        }

        report_file_open_error(env, funcName, fileName);
        return(FALSE);
    }

#endif

    if( get_sysdep_data(env)->fn_after_open != NULL )
    {
        (*get_sysdep_data(env)->fn_after_open)(env);
    }

#endif
    return(TRUE);
}

/**********************************************
 * sysdep_read_r_binary: Generic and machine specific
 *   code for reading from a file.
 ***********************************************/
void sysdep_read_r_binary(void *env, void *dataPtr, size_t size)
{
#if OFF
#if WIN_MVC
    char *tempPtr;

    tempPtr = (char *)dataPtr;

    while( size > INT_MAX )
    {
        _read(get_sysdep_data(env)->BinaryFileHandle, tempPtr, INT_MAX);
        size -= INT_MAX;
        tempPtr = tempPtr + INT_MAX;
    }

    if( size > 0 )
    {
        _read(get_sysdep_data(env)->BinaryFileHandle, tempPtr, (unsigned int)size);
    }

#endif

#if WIN_BTC
    char *tempPtr;

    tempPtr = (char *)dataPtr;

    while( size > INT_MAX )
    {
        read(get_sysdep_data(env)->BinaryFileHandle, tempPtr, INT_MAX);
        size -= INT_MAX;
        tempPtr = tempPtr + INT_MAX;
    }

    if( size > 0 )
    {
        read(get_sysdep_data(env)->BinaryFileHandle, tempPtr, (STD_SIZE)size);
    }

#endif

#if (!WIN_BTC) && (!WIN_MVC)
    fread(dataPtr, size, 1, get_sysdep_data(env)->binary_file_ptr);
#endif
#endif
}

/**************************************************
 * sysdep_seek_r_binary:  Generic and machine specific
 *   code for seeking a position in a file.
 ***************************************************/
void sysdep_seek_r_binary(void *env, long offset)
{
#if OFF
#if WIN_BTC
    lseek(get_sysdep_data(env)->BinaryFileHandle, offset, SEEK_CUR);
#endif

#if WIN_MVC
    _lseek(get_sysdep_data(env)->BinaryFileHandle, offset, SEEK_CUR);
#endif

#if (!WIN_BTC) && (!WIN_MVC)
    fseek(get_sysdep_data(env)->binary_file_ptr, offset, SEEK_CUR);
#endif
#endif
}

/**************************************************
 * sysdep_seek_set_r_binary:  Generic and machine specific
 *   code for seeking a position in a file.
 ***************************************************/
void sysdep_seek_set_r_binary(void *env, long offset)
{
#if OFF
#if WIN_BTC
    lseek(get_sysdep_data(env)->BinaryFileHandle, offset, SEEK_SET);
#endif

#if WIN_MVC
    _lseek(get_sysdep_data(env)->BinaryFileHandle, offset, SEEK_SET);
#endif

#if (!WIN_BTC) && (!WIN_MVC)
    fseek(get_sysdep_data(env)->binary_file_ptr, offset, SEEK_SET);
#endif
#endif
}

/***********************************************
 * sysdep_tell_r_binary:  Generic and machine specific
 *   code for telling a position in a file.
 ************************************************/
void sysdep_tell_r_binary(void *env, long *offset)
{
#if OFF
#if WIN_BTC
    *offset = lseek(get_sysdep_data(env)->BinaryFileHandle, 0, SEEK_CUR);
#endif

#if WIN_MVC
    *offset = _lseek(get_sysdep_data(env)->BinaryFileHandle, 0, SEEK_CUR);
#endif

#if (!WIN_BTC) && (!WIN_MVC)
    *offset = ftell(get_sysdep_data(env)->binary_file_ptr);
#endif
#endif
}

/***************************************
 * sysdep_close_r_binary:  Generic and machine
 *   specific code for closing a file.
 ****************************************/
void sysdep_close_r_binary(void *env)
{
#if OFF

    if( get_sysdep_data(env)->fn_before_open != NULL )
    {
        (*get_sysdep_data(env)->fn_before_open)(env);
    }

#if WIN_BTC
    close(get_sysdep_data(env)->BinaryFileHandle);
#endif

#if WIN_MVC
    _close(get_sysdep_data(env)->BinaryFileHandle);
#endif

#if (!WIN_BTC) && (!WIN_MVC)
    fclose(get_sysdep_data(env)->binary_file_ptr);
#endif

    if( get_sysdep_data(env)->fn_after_open != NULL )
    {
        (*get_sysdep_data(env)->fn_after_open)(env);
    }

#endif
}

/**********************************************
 * sysdep_write_to_file: Generic routine for writing to a
 *   file. No machine specific code as of yet.
 ***********************************************/
void sysdep_write_to_file(void *dataPtr, size_t size, FILE *fp)
{
#if OFF

    if( size == 0 )
    {
        return;
    }

#if UNIX_7
    fwrite(dataPtr, size, 1, fp);
#else
    fwrite(dataPtr, size, 1, fp);
#endif
#endif
}

/********************************************
 * InitializeKeywords: Adds key words to the
 *   symbol table so that they are available
 *   for command completion.
 *********************************************/
static void _init_keywords(void *env)
{
}

#if WIN_BTC

/********************************************
 * strtoll: Convert string to long long int.
 *    Note supported by Turbo C++ 2006.
 *********************************************/
__int64 _RTLENTRY _EXPFUNC strtoll(const char *str, char**endptr, int base)
// convert string to long long int
{
    if( endptr != NULL )
    {
        *endptr = (char*)str + (base == 10 ? strspn(str, "0123456789") : 0);
    }

    return(_atoi64(str));
}

/******************************************
 * llabs: absolute value of long long int.
 *    Note supported by Turbo C++ 2006.
 *******************************************/
__int64 _RTLENTRY _EXPFUNC llabs(__int64 val)
{
    if( val >= 0 )
    {
        return(val);
    }
    else
    {
        return(-val);
    }
}

#endif
