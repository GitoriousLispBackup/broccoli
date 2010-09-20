#ifndef _H_setup
#define _H_setup

/***************************************************************
 * -------------------- COMPILER FLAGS ------------------------
 ****************************************************************/

/********************************************************************
 * Flag denoting the environment in which the executable is to run.
 * Only one of these flags should be turned on (set to 1) at a time.
 *********************************************************************/

#ifndef UNIX_V
#define UNIX_V  0   /* UNIX System V, 4.2bsd, or HP Unix, presumably with gcc */
#endif

#ifndef UNIX_7
#define UNIX_7  0   /* UNIX System III Version 7 or Sun Unix, presumably with gcc */
#endif

#ifndef LINUX
#define LINUX   0   /* Untested, presumably with gcc */
#endif

#ifndef DARWIN
#define DARWIN  1   /* Darwin Mac OS 10.5, presumably with gcc or Xcode 3.0 with Console */
#endif

#ifndef MAC_XCD
#define MAC_XCD 0   /* MacOS 10.5, with Xcode 3.0 and Cocoa GUI */
#endif

#ifndef MAC_MCW
#define MAC_MCW 0   /* MacOS 10.5, with CodeWarrior 9.6 */
#endif

#ifndef WIN_MVC
#define WIN_MVC 0   /* Windows XP, with VC++ 2008 Express */
#endif

#ifndef WIN_BTC
#define WIN_BTC 0   /* Windows XP, with Borland Turbo C++ 2006 */
#endif

#ifndef WIN_MCW
#define WIN_MCW 0   /* Windows XP, with CodeWarrior 9.4 */
#endif

#ifndef WIN_GCC
#define WIN_GCC 0   /* Windows XP, with DJGPP 3.21 */
#endif

/* The following are unsupported: */

#ifndef VAX_VMS
#define VAX_VMS 0   /* VAX VMS */
#endif

/* Use GENERIC if nothing else is used. */

#ifndef GENERIC
#if (!UNIX_V) && (!LINUX) && (!UNIX_7) && \
    (!MAC_XCD) && (!MAC_MCW) && (!DARWIN) && \
    (!WIN_MVC) && (!WIN_BTC) && (!WIN_MCW) && (!WIN_GCC) && \
    (!VAX_VMS)
#define GENERIC 1   /* Generic (any machine)                   */
#else
#define GENERIC 0   /* Generic (any machine)                   */
#endif
#endif

#if WIN_MVC || WIN_BTC || WIN_MCW
#define IBM 1
#else
#define IBM 0
#endif

/**********************************************
 * Some definitions for use with declarations.
 ***********************************************/

#define VOID     void
#define VOID_ARG void
#define STD_SIZE size_t

#define BOOLEAN int

/***********************************************
 * DEFMODULE_CONSTRUCT:  Determines whether the
 *   module_definition construct is included.
 ************************************************/

#ifndef DEFMODULE_CONSTRUCT
#define DEFMODULE_CONSTRUCT 0
#endif


#ifndef METAOBJECT_PROTOCOL
#define METAOBJECT_PROTOCOL 0
#endif


/*********************************************
 * DEFFUNCTION_CONSTRUCT:  Determines whether
 *   deffunction construct is included.
 **********************************************/

#ifndef DEFFUNCTION_CONSTRUCT
#define DEFFUNCTION_CONSTRUCT 1
#endif

/********************************************
 * DEFGENERIC_CONSTRUCT:  Determines whether
 *   generic functions  are included.
 *********************************************/

#ifndef DEFGENERIC_CONSTRUCT
#define DEFGENERIC_CONSTRUCT 0
#endif

/****************************************************************
 * OBJECT_SYSTEM:  Determines whether object system is included.
 *   The LIST_FUNCTIONS flag should also be on if you want
 *   to be able to manipulate list slots.
 *****************************************************************/

#ifndef OBJECT_SYSTEM
#define OBJECT_SYSTEM 0
#endif

/****************************************************************
 * DEFINSTANCES_CONSTRUCT: Determines whether the definstances
 *   construct is enabled.
 *****************************************************************/

#ifndef DEFINSTANCES_CONSTRUCT
#define DEFINSTANCES_CONSTRUCT      0
#endif

#if !OBJECT_SYSTEM
#undef DEFINSTANCES_CONSTRUCT
#define DEFINSTANCES_CONSTRUCT      0
#endif

/*******************************************************************
 * INSTANCE_SET_QUERIES: Determines if instance-set query functions
 *  such as any-instancep and do-for-all-instances are included.
 ********************************************************************/

#ifndef INSTANCE_SET_QUERIES
#define INSTANCE_SET_QUERIES 0
#endif

#if !OBJECT_SYSTEM
#undef INSTANCE_SET_QUERIES
#define INSTANCE_SET_QUERIES        0
#endif

/***************************************************************
 * EXTENDED MATH PACKAGE FLAG: If this is on, then the extended
 * math package functions will be available for use, (normal
 * default). If this flag is off, then the extended math
 * functions will not be available, and the 30K or so of space
 * they require will be free. Usually a concern only on PC type
 * machines.
 ****************************************************************/

#ifndef EXTENDED_MATH_FUNCTIONS
#define EXTENDED_MATH_FUNCTIONS 0
#endif

/***************************************************************
 * TEXT PROCESSING : Turn on this flag for support of the
 * hierarchical lookup system.
 ****************************************************************/


#ifndef META_SYSTEM
#define META_SYSTEM 1
#endif

/***************************************************************
 * HELP: To implement the help facility, set the flag below and
 * specify the path and name of the help data file your system.
 ****************************************************************/

#ifndef HELP_FUNCTIONS
#define HELP_FUNCTIONS 0
#endif

#if HELP_FUNCTIONS
#define HELP_DEFAULT "broccoli.hlp"
#endif

/***********************************************
 * IO_FUNCTIONS: Includes printout, read, open,
 *   close, format, and readline functions.
 ************************************************/

#ifndef IO_FUNCTIONS
#define IO_FUNCTIONS 0
#endif

/***********************************************
 * STRING_FUNCTIONS: Includes string functions:
 *   str-length, str-compare, upcase, lowcase,
 *   sub-string, str-index, and eval.
 ************************************************/

#ifndef STRING_FUNCTIONS
#define STRING_FUNCTIONS 0
#endif

/********************************************
 * LIST_FUNCTIONS: Includes list
 *   functions:  mv-subseq, mv-delete,
 *   mv-append, str-explode, str-implode.
 *********************************************/

#ifndef LIST_FUNCTIONS
#define LIST_FUNCTIONS 1
#endif

/***************************************************
 * DEBUGGING_FUNCTIONS: Includes functions
 ****************************************************/

#ifndef DEBUGGING_FUNCTIONS
#define DEBUGGING_FUNCTIONS 0
#endif

/**************************************************
 * PROFILING_FUNCTIONS: Enables code for profiling
 *   constructs and user functions.
 ***************************************************/

#ifndef PROFILING_FUNCTIONS
#define PROFILING_FUNCTIONS 0
#endif

/***********************************************************************
 * BLOCK_MEMORY: Causes memory to be allocated in large blocks.
 *   INITBUFFERSIZE and BLOCKSIZE should both be set to less than the
 *   maximum size of a signed integer.
 ************************************************************************/

#ifndef BLOCK_MEMORY
#define BLOCK_MEMORY 0
#endif

#if BLOCK_MEMORY

#define INITBLOCKSIZE 32000
#define BLOCKSIZE     32000

#endif

/*******************************************
 * DEVELOPER: Enables code for debugging a
 *   development version of the executable.
 ********************************************/

#ifndef DEVELOPER
#define DEVELOPER 0
#endif

#if DEVELOPER
#include <assert.h>
#define Bogus(x) assert(!(x))
#else
#define Bogus(x)
#endif

/**************************
 * Environment Definitions
 ***************************/

#include "core_environment.h"


/************************************************
 * Any user defined global setup information can
 * be included in the file extensions.h which is
 * an empty file in the baseline version.
 *************************************************/
#include "extensions.h"

#endif  /* _H_setup */
