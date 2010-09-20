/* Purpose: Provides routines useful for browsing various
 *   data structures. The functions are provided for
 *   development use.                                        */

#define __EXTENSIONS_DEVELOPER_SOURCE__

#include <stdio.h>
#define _STDIO_INCLUDED_

#include "setup.h"

#include "core_arguments.h"
#include "core_environment.h"
#include "core_functions.h"
#if OBJECT_SYSTEM
#include "classes_instances_kernel.h"
#endif
#include "modules_query.h"
#include "router.h"
#include "core_gc.h"

#if OBJECT_SYSTEM
#include "funcs_instance.h"
#endif

#include "extensions_developer.h"

#if DEVELOPER

/*************************************************
 * DeveloperCommands: Sets up developer commands.
 **************************************************/
void DeveloperCommands(void *env)
{
    core_define_function(env, "primitives-info", 'v', PTR_FN PrimitiveTablesInfo, "PrimitiveTablesInfo", "00");
    core_define_function(env, "primitives-usage", 'v', PTR_FN PrimitiveTablesUsage, "PrimitiveTablesUsage", "00");
    core_define_function(env, "enable-gc-heuristics", 'v', PTR_FN EnableGCHeuristics, "EnableGCHeuristics", "00");
    core_define_function(env, "disable-gc-heuristics", 'v', PTR_FN DisableGCHeuristics, "DisableGCHeuristics", "00");

#if OBJECT_SYSTEM
    core_define_function(env, "instance-table-usage", 'v', PTR_FN InstanceTableUsage, "InstanceTableUsage", "00");
#endif
}

/*****************************************************
 * EnableGCHeuristics:
 ******************************************************/
void EnableGCHeuristics(void *env)
{
    core_check_arg_count(env, "enable-gc-heuristics", EXACTLY, 0);
    core_gc_set_is_using_gc_heuristics(env, TRUE);
}

/*****************************************************
 * DisableGCHeuristics:
 ******************************************************/
void DisableGCHeuristics(void *env)
{
    core_check_arg_count(env, "disable-gc-heuristics", EXACTLY, 0);
    core_gc_set_is_using_gc_heuristics(env, FALSE);
}

/*****************************************************
 * PrimitiveTablesInfo: Prints information about the
 *   symbol, float, integer, and bitmap tables.
 ******************************************************/
void PrimitiveTablesInfo(void *env)
{
    unsigned long i;
    ATOM_HN **symbolArray, *symbolPtr;
    FLOAT_HN **floatArray, *floatPtr;
    INTEGER_HN **integerArray, *integerPtr;
    BITMAP_HN **bitMapArray, *bitMapPtr;
    unsigned long int symbolCount = 0, integerCount = 0;
    unsigned long int floatCount = 0, bitMapCount = 0;

    core_check_arg_count(env, "primitives-info", EXACTLY, 0);

    /*====================================
     * Count entries in the symbol table.
     *====================================*/

    symbolArray = get_symbol_table(env);

    for( i = 0; i < ATOM_HASH_SZ; i++ )
    {
        for( symbolPtr = symbolArray[i]; symbolPtr != NULL; symbolPtr = symbolPtr->next )
        {
            symbolCount++;
        }
    }

    /*====================================
     * Count entries in the integer table.
     *====================================*/

    integerArray = get_integer_table(env);

    for( i = 0; i < INTEGER_HASH_SZ; i++ )
    {
        for( integerPtr = integerArray[i]; integerPtr != NULL; integerPtr = integerPtr->next )
        {
            integerCount++;
        }
    }

    /*====================================
     * Count entries in the float table.
     *====================================*/

    floatArray = get_float_table(env);

    for( i = 0; i < FLOAT_HASH_SZ; i++ )
    {
        for( floatPtr = floatArray[i]; floatPtr != NULL; floatPtr = floatPtr->next )
        {
            floatCount++;
        }
    }

    /*====================================
     * Count entries in the bitmap table.
     *====================================*/

    bitMapArray = get_bitmap_table(env);

    for( i = 0; i < BITMAP_HASH_SZ; i++ )
    {
        for( bitMapPtr = bitMapArray[i]; bitMapPtr != NULL; bitMapPtr = bitMapPtr->next )
        {
            bitMapCount++;
        }
    }

    /*========================
     * Print the information.
     *========================*/

    print_router(env, WDISPLAY, "Symbols: ");
    core_print_long(env, WDISPLAY, (long long)symbolCount);
    print_router(env, WDISPLAY, "\n");
    print_router(env, WDISPLAY, "Integers: ");
    core_print_long(env, WDISPLAY, (long long)integerCount);
    print_router(env, WDISPLAY, "\n");
    print_router(env, WDISPLAY, "Floats: ");
    core_print_long(env, WDISPLAY, (long long)floatCount);
    print_router(env, WDISPLAY, "\n");
    print_router(env, WDISPLAY, "BitMaps: ");
    core_print_long(env, WDISPLAY, (long long)bitMapCount);
    print_router(env, WDISPLAY, "\n");

    /*
     * print_router(env,WDISPLAY,"Ephemerals: ");
     * core_print_long(env,WDISPLAY,(long long) EphemeralSymbolCount());
     * print_router(env,WDISPLAY,"\n");
     */
}

#define COUNT_SIZE 21

/*****************************************************
 * PrimitiveTablesUsage: Prints information about the
 *   symbol, float, integer, and bitmap tables.
 ******************************************************/
void PrimitiveTablesUsage(void *env)
{
    unsigned long i;
    int symbolCounts[COUNT_SIZE], floatCounts[COUNT_SIZE];
    ATOM_HN **symbolArray, *symbolPtr;
    FLOAT_HN **floatArray, *floatPtr;
    unsigned long int symbolCount, totalSymbolCount = 0;
    unsigned long int floatCount, totalFloatCount = 0;

    core_check_arg_count(env, "primitives-usage", EXACTLY, 0);

    for( i = 0; i < 21; i++ )
    {
        symbolCounts[i] = 0;
        floatCounts[i] = 0;
    }

    /*====================================
     * Count entries in the symbol table.
     *====================================*/

    symbolArray = get_symbol_table(env);

    for( i = 0; i < ATOM_HASH_SZ; i++ )
    {
        symbolCount = 0;

        for( symbolPtr = symbolArray[i]; symbolPtr != NULL; symbolPtr = symbolPtr->next )
        {
            symbolCount++;
            totalSymbolCount++;
        }

        if( symbolCount < (COUNT_SIZE - 1))
        {
            symbolCounts[symbolCount]++;
        }
        else
        {
            symbolCounts[COUNT_SIZE - 1]++;
        }
    }

    /*===================================
     * Count entries in the float table.
     *===================================*/

    floatArray = get_float_table(env);

    for( i = 0; i < FLOAT_HASH_SZ; i++ )
    {
        floatCount = 0;

        for( floatPtr = floatArray[i]; floatPtr != NULL; floatPtr = floatPtr->next )
        {
            floatCount++;
            totalFloatCount++;
        }

        if( floatCount < (COUNT_SIZE - 1))
        {
            floatCounts[floatCount]++;
        }
        else
        {
            floatCounts[COUNT_SIZE - 1]++;
        }
    }


    /*========================
     * Print the information.
     *========================*/

    print_router(env, WDISPLAY, "Total Symbols: ");
    core_print_long(env, WDISPLAY, (long long)totalSymbolCount);
    print_router(env, WDISPLAY, "\n");

    for( i = 0; i < COUNT_SIZE; i++ )
    {
        core_print_long(env, WDISPLAY, (long long)i);
        print_router(env, WDISPLAY, " ");
        core_print_long(env, WDISPLAY, (long long)symbolCounts[i]);
        print_router(env, WDISPLAY, "\n");
    }

    print_router(env, WDISPLAY, "\nTotal Floats: ");
    core_print_long(env, WDISPLAY, (long long)totalFloatCount);
    print_router(env, WDISPLAY, "\n");

    for( i = 0; i < COUNT_SIZE; i++ )
    {
        core_print_long(env, WDISPLAY, (long long)i);
        print_router(env, WDISPLAY, " ");
        core_print_long(env, WDISPLAY, (long long)floatCounts[i]);
        print_router(env, WDISPLAY, "\n");
    }
}

#if OBJECT_SYSTEM

/*****************************************************
 * InstanceTableUsage: Prints information about the
 *   instances in the instance hash table.
 ******************************************************/
void InstanceTableUsage(void *env)
{
    unsigned long i;
    int instanceCounts[COUNT_SIZE];
    INSTANCE_TYPE *ins;
    unsigned long int instanceCount, totalInstanceCount = 0;

    core_check_arg_count(env, "instance-table-usage", EXACTLY, 0);

    for( i = 0; i < COUNT_SIZE; i++ )
    {
        instanceCounts[i] = 0;
    }

    /*======================================
     * Count entries in the instance table.
     *======================================*/

    for( i = 0; i < INSTANCE_TABLE_HASH_SIZE; i++ )
    {
        instanceCount = 0;

        for( ins = InstanceData(env)->InstanceTable[i]; ins != NULL; ins = ins->nxtHash )
        {
            instanceCount++;
            totalInstanceCount++;
        }

        if( instanceCount < (COUNT_SIZE - 1))
        {
            instanceCounts[instanceCount]++;
        }
        else
        {
            instanceCounts[COUNT_SIZE - 1]++;
        }
    }

    /*========================
     * Print the information.
     *========================*/

    print_router(env, WDISPLAY, "Total Instances: ");
    core_print_long(env, WDISPLAY, (long long)totalInstanceCount);
    print_router(env, WDISPLAY, "\n");

    for( i = 0; i < COUNT_SIZE; i++ )
    {
        core_print_long(env, WDISPLAY, (long long)i);
        print_router(env, WDISPLAY, " ");
        core_print_long(env, WDISPLAY, (long long)instanceCounts[i]);
        print_router(env, WDISPLAY, "\n");
    }
}

#endif

#endif
