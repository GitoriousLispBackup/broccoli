/* Purpose: Manages the atomic data value hash tables for
 *   storing symbols, integers, floats, and bit maps.
 *   Contains routines for adding entries, examining the
 *   hash tables, and performing garbage collection to
 *   remove entries no longer in use.                        */

#define __TYPE_ATOM_SOURCE__

#include <stdio.h>
#define _STDIO_INCLUDED_
#include <stdlib.h>
#include <string.h>

#include "setup.h"

#include "constant.h"
#include "core_environment.h"
#include "core_memory.h"
#include "router.h"
#include "core_gc.h"
#include "core_arguments.h"
#include "sysdep.h"

#include "type_symbol.h"

/**************
 * DEFINITIONS
 ***************/

#define FALSE_STRING             "nil"
#define TRUE_STRING              "t"
#define POSITIVE_INFINITY_STRING "+oo"
#define NEGATIVE_INFINITY_STRING "-oo"

#define AVERAGE_STRING_SIZE      10
#define AVERAGE_BITMAP_SIZE      sizeof(long)

/**************************************
 * LOCAL FUNCTION PROTOTYPES
 ***************************************/

static void                     _remove_hash_node(void *, GENERIC_HN *, GENERIC_HN **, int, int);
static void                     _add_ephemeron(void *, GENERIC_HN *, struct ephemeron **, int, int);
static void                     _remove_ephemeron(void *, struct ephemeron **, GENERIC_HN **, int, int, int);
static char                   * _is_string_in_string(char *, char *);
static size_t                   _common_prefix_length(char *, char *);
static void                     _delete_symbol_tables(void *);

/******************************************************
 * init_symbol_tables: Initializes the symbol_table,
 *   integer_table, and float_table. It also initializes
 *   the TrueSymbol and FalseSymbol.
 *******************************************************/
#if WIN_BTC
#pragma argsused
#endif
void init_symbol_tables(void *env, struct atom_hash_node **symbolTable, struct float_hash_node **floatTable, struct integer_hash_node **integerTable, struct bitmap_hash_node **bitmapTable, struct external_address_hash_node **externalAddressTable)
{
#if MAC_MCW || WIN_MCW || MAC_XCD
#pragma unused(symbolTable)
#pragma unused(floatTable)
#pragma unused(integerTable)
#pragma unused(bitmapTable)
#pragma unused(externalAddressTable)
#endif
    unsigned long i;

    core_allocate_environment_data(env, ATOM_DATA_INDEX, sizeof(struct atom_data), _delete_symbol_tables);

    /*=========================
     * Create the hash tables.
     *=========================*/

    get_atom_data(env)->symbol_table = (ATOM_HN **)
                                       core_mem_alloc_large_no_init(env, sizeof(ATOM_HN *) * ATOM_HASH_SZ);

    get_atom_data(env)->float_table = (FLOAT_HN **)
                                      core_mem_alloc_no_init(env, (int)sizeof(FLOAT_HN *) * FLOAT_HASH_SZ);

    get_atom_data(env)->integer_table = (INTEGER_HN **)
                                        core_mem_alloc_no_init(env, (int)sizeof(INTEGER_HN *) * INTEGER_HASH_SZ);

    get_atom_data(env)->bitmap_table = (BITMAP_HN **)
                                       core_mem_alloc_no_init(env, (int)sizeof(BITMAP_HN *) * BITMAP_HASH_SZ);

    get_atom_data(env)->external_address_table = (EXTERNAL_ADDRESS_HN **)
                                                 core_mem_alloc_no_init(env, (int)sizeof(EXTERNAL_ADDRESS_HN *) * EXTERNAL_ADDRESS_HASH_SZ);

    /*===================================================
     * Initialize all of the hash table entries to NULL.
     *===================================================*/

    for( i = 0; i < ATOM_HASH_SZ; i++ )
    {
        get_atom_data(env)->symbol_table[i] = NULL;
    }

    for( i = 0; i < FLOAT_HASH_SZ; i++ )
    {
        get_atom_data(env)->float_table[i] = NULL;
    }

    for( i = 0; i < INTEGER_HASH_SZ; i++ )
    {
        get_atom_data(env)->integer_table[i] = NULL;
    }

    for( i = 0; i < BITMAP_HASH_SZ; i++ )
    {
        get_atom_data(env)->bitmap_table[i] = NULL;
    }

    for( i = 0; i < EXTERNAL_ADDRESS_HASH_SZ; i++ )
    {
        get_atom_data(env)->external_address_table[i] = NULL;
    }

    /*========================
     * Predefine some values.
     *========================*/

    get_atom_data(env)->true_atom = store_atom(env, TRUE_STRING);
    inc_atom_count(get_atom_data(env)->true_atom);
    get_atom_data(env)->false_atom = store_atom(env, FALSE_STRING);
    inc_atom_count(get_atom_data(env)->false_atom);
    get_atom_data(env)->positive_inf_atom = store_atom(env, POSITIVE_INFINITY_STRING);
    inc_atom_count(get_atom_data(env)->positive_inf_atom);
    get_atom_data(env)->negative_inf_atom = store_atom(env, NEGATIVE_INFINITY_STRING);
    inc_atom_count(get_atom_data(env)->negative_inf_atom);
    get_atom_data(env)->zero_atom = store_long(env, 0LL);
    inc_integer_count(get_atom_data(env)->zero_atom);
}

/************************************************
 * DeallocateSymbolData: Deallocates environment
 *    data for symbols.
 *************************************************/
static void _delete_symbol_tables(void *env)
{
    int i;
    ATOM_HN *shPtr, *nextSHPtr;
    INTEGER_HN *ihPtr, *nextIHPtr;
    FLOAT_HN *fhPtr, *nextFHPtr;
    BITMAP_HN *bmhPtr, *nextBMHPtr;
    EXTERNAL_ADDRESS_HN *eahPtr, *nextEAHPtr;
    struct ephemeron *edPtr, *nextEDPtr;

    if((get_atom_data(env)->symbol_table == NULL) ||
       (get_atom_data(env)->float_table == NULL) ||
       (get_atom_data(env)->integer_table == NULL) ||
       (get_atom_data(env)->bitmap_table == NULL) ||
       (get_atom_data(env)->external_address_table == NULL))
    {
        return;
    }

    for( i = 0; i < ATOM_HASH_SZ; i++ )
    {
        shPtr = get_atom_data(env)->symbol_table[i];

        while( shPtr != NULL )
        {
            nextSHPtr = shPtr->next;

            if( !shPtr->is_permanent )
            {
                core_mem_release(env, shPtr->contents, strlen(shPtr->contents) + 1);
                core_mem_return_struct(env, atom_hash_node, shPtr);
            }

            shPtr = nextSHPtr;
        }
    }

    for( i = 0; i < FLOAT_HASH_SZ; i++ )
    {
        fhPtr = get_atom_data(env)->float_table[i];

        while( fhPtr != NULL )
        {
            nextFHPtr = fhPtr->next;

            if( !fhPtr->is_permanent )
            {
                core_mem_return_struct(env, float_hash_node, fhPtr);
            }

            fhPtr = nextFHPtr;
        }
    }

    for( i = 0; i < INTEGER_HASH_SZ; i++ )
    {
        ihPtr = get_atom_data(env)->integer_table[i];

        while( ihPtr != NULL )
        {
            nextIHPtr = ihPtr->next;

            if( !ihPtr->is_permanent )
            {
                core_mem_return_struct(env, integer_hash_node, ihPtr);
            }

            ihPtr = nextIHPtr;
        }
    }

    for( i = 0; i < BITMAP_HASH_SZ; i++ )
    {
        bmhPtr = get_atom_data(env)->bitmap_table[i];

        while( bmhPtr != NULL )
        {
            nextBMHPtr = bmhPtr->next;

            if( !bmhPtr->is_permanent )
            {
                core_mem_release(env, bmhPtr->contents, bmhPtr->size);
                core_mem_return_struct(env, bitmap_hash_node, bmhPtr);
            }

            bmhPtr = nextBMHPtr;
        }
    }

    for( i = 0; i < EXTERNAL_ADDRESS_HASH_SZ; i++ )
    {
        eahPtr = get_atom_data(env)->external_address_table[i];

        while( eahPtr != NULL )
        {
            nextEAHPtr = eahPtr->next;

            if( !eahPtr->is_permanent )
            {
                core_mem_return_struct(env, external_address_hash_node, eahPtr);
            }

            eahPtr = nextEAHPtr;
        }
    }

    /*=========================================
     * Remove the is_ephemeral symbol structures.
     *=========================================*/

    edPtr = get_atom_data(env)->atom_ephemerons;

    while( edPtr != NULL )
    {
        nextEDPtr = edPtr->next;
        core_mem_return_struct(env, ephemeron, edPtr);
        edPtr = nextEDPtr;
    }

    edPtr = get_atom_data(env)->float_ephemerons;

    while( edPtr != NULL )
    {
        nextEDPtr = edPtr->next;
        core_mem_return_struct(env, ephemeron, edPtr);
        edPtr = nextEDPtr;
    }

    edPtr = get_atom_data(env)->integer_ephemerons;

    while( edPtr != NULL )
    {
        nextEDPtr = edPtr->next;
        core_mem_return_struct(env, ephemeron, edPtr);
        edPtr = nextEDPtr;
    }

    edPtr = get_atom_data(env)->bitmap_ephemerons;

    while( edPtr != NULL )
    {
        nextEDPtr = edPtr->next;
        core_mem_return_struct(env, ephemeron, edPtr);
        edPtr = nextEDPtr;
    }

    edPtr = get_atom_data(env)->external_address_ephemerons;

    while( edPtr != NULL )
    {
        nextEDPtr = edPtr->next;
        core_mem_return_struct(env, ephemeron, edPtr);
        edPtr = nextEDPtr;
    }

    /*================================
     * Remove the symbol hash tables.
     *================================*/

    core_mem_release_sized(env, get_atom_data(env)->symbol_table, sizeof(ATOM_HN *) * ATOM_HASH_SZ);

    core_mem_free(env, get_atom_data(env)->float_table, (int)sizeof(FLOAT_HN *) * FLOAT_HASH_SZ);

    core_mem_free(env, get_atom_data(env)->integer_table, (int)sizeof(INTEGER_HN *) * INTEGER_HASH_SZ);

    core_mem_free(env, get_atom_data(env)->bitmap_table, (int)sizeof(BITMAP_HN *) * BITMAP_HASH_SZ);

    core_mem_free(env, get_atom_data(env)->external_address_table, (int)sizeof(EXTERNAL_ADDRESS_HN *) * EXTERNAL_ADDRESS_HASH_SZ);
}

/********************************************************************
 * store_atom: Searches for the string in the symbol table. If the
 *   string is already in the symbol table, then the address of the
 *   string's location in the symbol table is returned. Otherwise,
 *   the string is added to the symbol table and then the address
 *   of the string's location in the symbol table is returned.
 *********************************************************************/
void *store_atom(void *env, char *str)
{
    unsigned long tally;
    size_t length;
    ATOM_HN *past = NULL, *peek;

    /*====================================
     * Get the hash value for the string.
     *====================================*/

    if( str == NULL )
    {
        error_system(env, "ATOM", 1);
        exit_router(env, EXIT_FAILURE);
    }

    tally = hash_atom(str, ATOM_HASH_SZ);
    peek = get_atom_data(env)->symbol_table[tally];

    /*==================================================
     * Search for the string in the list of entries for
     * this symbol table location.  If the string is
     * found, then return the address of the string.
     *==================================================*/

    while( peek != NULL )
    {
        if( strcmp(str, peek->contents) == 0 )
        {
            return((void *)peek);
        }

        past = peek;
        peek = peek->next;
    }

    /*==================================================
     * Add the string at the end of the list of entries
     * for this symbol table location.
     *==================================================*/

    peek = core_mem_get_struct(env, atom_hash_node);

    if( past == NULL )
    {
        get_atom_data(env)->symbol_table[tally] = peek;
    }
    else
    {
        past->next = peek;
    }

    length = strlen(str) + 1;
    peek->contents = (char *)core_mem_alloc_no_init(env, length);
    peek->next = NULL;
    peek->bucket = tally;
    peek->count = 0;
    peek->is_permanent = FALSE;
    sysdep_strcpy(peek->contents, str);

    /*================================================
     * Add the string to the list of is_ephemeral items.
     *================================================*/

    _add_ephemeron(env, (GENERIC_HN *)peek, &get_atom_data(env)->atom_ephemerons,
                   sizeof(ATOM_HN), AVERAGE_STRING_SIZE);
    peek->depth = core_get_evaluation_data(env)->eval_depth;

    /*===================================
     * Return the address of the symbol.
     *===================================*/

    return((void *)peek);
}

/****************************************************************
 * lookup_atom: Searches for the string in the symbol table and
 *   returns a pointer to it if found, otherwise returns NULL.
 *****************************************************************/
ATOM_HN *lookup_atom(void *env, char *str)
{
    unsigned long tally;
    ATOM_HN *peek;

    tally = hash_atom(str, ATOM_HASH_SZ);

    for( peek = get_atom_data(env)->symbol_table[tally];
         peek != NULL;
         peek = peek->next )
    {
        if( strcmp(str, peek->contents) == 0 )
        {
            return(peek);
        }
    }

    return(NULL);
}

/******************************************************************
 * store_double: Searches for the double in the hash table. If the
 *   double is already in the hash table, then the address of the
 *   double is returned. Otherwise, the double is hashed into the
 *   table and the address of the double is also returned.
 *******************************************************************/
void *store_double(void *env, double number)
{
    unsigned long tally;
    FLOAT_HN *past = NULL, *peek;

    /*====================================
     * Get the hash value for the double.
     *====================================*/

    tally = hash_float(number, FLOAT_HASH_SZ);
    peek = get_atom_data(env)->float_table[tally];

    /*==================================================
     * Search for the double in the list of entries for
     * this hash location.  If the double is found,
     * then return the address of the double.
     *==================================================*/

    while( peek != NULL )
    {
        if( number == peek->contents )
        {
            return((void *)peek);
        }

        past = peek;
        peek = peek->next;
    }

    /*=================================================
     * Add the float at the end of the list of entries
     * for this hash location.
     *=================================================*/

    peek = core_mem_get_struct(env, float_hash_node);

    if( past == NULL )
    {
        get_atom_data(env)->float_table[tally] = peek;
    }
    else
    {
        past->next = peek;
    }

    peek->contents = number;
    peek->next = NULL;
    peek->bucket = tally;
    peek->count = 0;
    peek->is_permanent = FALSE;

    /*===============================================
     * Add the float to the list of is_ephemeral items.
     *===============================================*/

    _add_ephemeron(env, (GENERIC_HN *)peek, &get_atom_data(env)->float_ephemerons,
                   sizeof(FLOAT_HN), 0);
    peek->depth = core_get_evaluation_data(env)->eval_depth;

    /*==================================
     * Return the address of the float.
     *==================================*/

    return((void *)peek);
}

/**************************************************************
 * store_long: Searches for the long in the hash table. If the
 *   long is already in the hash table, then the address of
 *   the long is returned. Otherwise, the long is hashed into
 *   the table and the address of the long is also returned.
 ***************************************************************/
void *store_long(void *env, long long number)
{
    unsigned long tally;
    INTEGER_HN *past = NULL, *peek;

    /*==================================
     * Get the hash value for the long.
     *==================================*/

    tally = hash_integer(number, INTEGER_HASH_SZ);
    peek = get_atom_data(env)->integer_table[tally];

    /*================================================
     * Search for the long in the list of entries for
     * this hash location. If the long is found, then
     * return the address of the long.
     *================================================*/

    while( peek != NULL )
    {
        if( number == peek->contents )
        {
            return((void *)peek);
        }

        past = peek;
        peek = peek->next;
    }

    /*================================================
     * Add the long at the end of the list of entries
     * for this hash location.
     *================================================*/

    peek = core_mem_get_struct(env, integer_hash_node);

    if( past == NULL )
    {
        get_atom_data(env)->integer_table[tally] = peek;
    }
    else
    {
        past->next = peek;
    }

    peek->contents = number;
    peek->next = NULL;
    peek->bucket = tally;
    peek->count = 0;
    peek->is_permanent = FALSE;

    /*=================================================
     * Add the integer to the list of is_ephemeral items.
     *=================================================*/

    _add_ephemeron(env, (GENERIC_HN *)peek, &get_atom_data(env)->integer_ephemerons,
                   sizeof(INTEGER_HN), 0);
    peek->depth = core_get_evaluation_data(env)->eval_depth;

    /*====================================
     * Return the address of the integer.
     *====================================*/

    return((void *)peek);
}

/****************************************************************
 * lookup_long: Searches for the integer in the integer table and
 *   returns a pointer to it if found, otherwise returns NULL.
 *****************************************************************/
INTEGER_HN *lookup_long(void *env, long long theLong)
{
    unsigned long tally;
    INTEGER_HN *peek;

    tally = hash_integer(theLong, INTEGER_HASH_SZ);

    for( peek = get_atom_data(env)->integer_table[tally];
         peek != NULL;
         peek = peek->next )
    {
        if( peek->contents == theLong )
        {
            return(peek);
        }
    }

    return(NULL);
}

/******************************************************************
 * store_bitmap: Searches for the bitmap in the hash table. If the
 *   bitmap is already in the hash table, then the address of the
 *   bitmap is returned. Otherwise, the bitmap is hashed into the
 *   table and the address of the bitmap is also returned.
 *******************************************************************/
void *store_bitmap(void *env, void *vTheBitMap, unsigned size)
{
    char *theBitMap = (char *)vTheBitMap;
    unsigned long tally;
    unsigned i;
    BITMAP_HN *past = NULL, *peek;

    /*====================================
     * Get the hash value for the bitmap.
     *====================================*/

    if( theBitMap == NULL )
    {
        error_system(env, "ATOM", 2);
        exit_router(env, EXIT_FAILURE);
    }

    tally = hash_bitmap(theBitMap, BITMAP_HASH_SZ, size);
    peek = get_atom_data(env)->bitmap_table[tally];

    /*==================================================
     * Search for the bitmap in the list of entries for
     * this hash table location.  If the bitmap is
     * found, then return the address of the bitmap.
     *==================================================*/

    while( peek != NULL )
    {
        if( peek->size == (unsigned short)size )
        {
            for( i = 0; i < size ; i++ )
            {
                if( peek->contents[i] != theBitMap[i] )
                {
                    break;
                }
            }

            if( i == size )
            {
                return((void *)peek);
            }
        }

        past = peek;
        peek = peek->next;
    }

    /*==================================================
     * Add the bitmap at the end of the list of entries
     * for this hash table location.  Return the
     *==================================================*/

    peek = core_mem_get_struct(env, bitmap_hash_node);

    if( past == NULL )
    {
        get_atom_data(env)->bitmap_table[tally] = peek;
    }
    else
    {
        past->next = peek;
    }

    peek->contents = (char *)core_mem_alloc_no_init(env, size);
    peek->next = NULL;
    peek->bucket = tally;
    peek->count = 0;
    peek->is_permanent = FALSE;
    peek->size = (unsigned short)size;

    for( i = 0; i < size ; i++ )
    {
        peek->contents[i] = theBitMap[i];
    }

    /*================================================
     * Add the bitmap to the list of is_ephemeral items.
     *================================================*/

    _add_ephemeron(env, (GENERIC_HN *)peek, &get_atom_data(env)->bitmap_ephemerons,
                   sizeof(BITMAP_HN), sizeof(long));
    peek->depth = core_get_evaluation_data(env)->eval_depth;

    /*===================================
     * Return the address of the bitmap.
     *===================================*/

    return((void *)peek);
}

/******************************************************************
 * store_external_address: Searches for the external address in the
 *   hash table. If the external address is already in the hash
 *   table, then the address of the external address is returned.
 *   Otherwise, the external address is hashed into the table and
 *   the address of the external address is also returned.
 *******************************************************************/
void *store_external_address(void *env, void *theExternalAddress, unsigned theType)
{
    unsigned long tally;
    EXTERNAL_ADDRESS_HN *past = NULL, *peek;

    /*====================================
     * Get the hash value for the bitmap.
     *====================================*/

    tally = hash_external_address(theExternalAddress, EXTERNAL_ADDRESS_HASH_SZ);

    peek = get_atom_data(env)->external_address_table[tally];

    /*=============================================================
     * Search for the external address in the list of entries for
     * this hash table location.  If the external addressis found,
     * then return the address of the external address.
     *=============================================================*/

    while( peek != NULL )
    {
        if((peek->type == (unsigned short)theType) &&
           (peek->address == theExternalAddress))
        {
            return((void *)peek);
        }

        past = peek;
        peek = peek->next;
    }

    /*=================================================
     * Add the external address at the end of the list
     * of entries for this hash table location.
     *=================================================*/

    peek = core_mem_get_struct(env, external_address_hash_node);

    if( past == NULL )
    {
        get_atom_data(env)->external_address_table[tally] = peek;
    }
    else
    {
        past->next = peek;
    }

    peek->address = theExternalAddress;
    peek->type = (unsigned short)theType;
    peek->next = NULL;
    peek->bucket = tally;
    peek->count = 0;
    peek->is_permanent = FALSE;

    /*================================================
     * Add the bitmap to the list of is_ephemeral items.
     *================================================*/

    _add_ephemeron(env, (GENERIC_HN *)peek, &get_atom_data(env)->external_address_ephemerons,
                   sizeof(EXTERNAL_ADDRESS_HN), sizeof(long));
    peek->depth = core_get_evaluation_data(env)->eval_depth;

    /*=============================================
     * Return the address of the external address.
     *=============================================*/

    return((void *)peek);
}

/**************************************************
 * hash_atom: Computes a hash value for a symbol.
 ***************************************************/
unsigned long hash_atom(char *word, unsigned long range)
{
    register int i;
    unsigned long tally = 0;

    for( i = 0; word[i]; i++ )
    {
        tally = tally * 127 + word[i];
    }

    if( range == 0 )
    {
        return(tally);
    }

    return(tally % range);
}

/************************************************
 * hash_float: Computes a hash value for a float.
 *************************************************/
unsigned long hash_float(double number, unsigned long range)
{
    unsigned long tally = 0;
    char *word;
    unsigned i;

    word = (char *)&number;

    for( i = 0; i < sizeof(double); i++ )
    {
        tally = tally * 127 + word[i];
    }

    if( range == 0 )
    {
        return(tally);
    }

    return(tally % range);
}

/*****************************************************
 * hash_integer: Computes a hash value for an integer.
 ******************************************************/
unsigned long hash_integer(long long number, unsigned long range)
{
    unsigned long tally;

#if WIN_MVC

    if( number < 0 )
    {
        number = -number;
    }

    tally = (((unsigned)number) % range);
#else
    tally = (((unsigned)llabs(number)) % range);
#endif

    if( range == 0 )
    {
        return(tally);
    }

    return(tally);
}

/***************************************
 * hash_external_address: Computes a hash
 *   value for an external address.
 ****************************************/
unsigned long hash_external_address(void *theExternalAddress, unsigned long range)
{
    unsigned long tally;

    union
    {
        void *   vv;
        unsigned uv;
    } fis;

    fis.vv = theExternalAddress;
    tally = (fis.uv / 256);

    if( range == 0 )
    {
        return(tally);
    }

    return(tally % range);
}

/**************************************************
 * hash_bitmap: Computes a hash value for a bitmap.
 ***************************************************/
unsigned long hash_bitmap(char *word, unsigned long range, unsigned length)
{
    register unsigned k, j, i;
    unsigned long tally;
    unsigned longLength;
    unsigned long count = 0L, tmpLong;
    char *tmpPtr;

    tmpPtr = (char *)&tmpLong;

    /*================================================================
     * Add up the first part of the word as unsigned long int values.
     *================================================================ */

    longLength = length / sizeof(unsigned long);

    for( i = 0, j = 0 ; i < longLength; i++ )
    {
        for( k = 0 ; k < sizeof(unsigned long) ; k++, j++ )
        {
            tmpPtr[k] = word[j];
        }

        count += tmpLong;
    }

    /*============================================
     * Add the remaining characters to the count.
     *============================================*/

    for(; j < length; j++ )
    {
        count += (unsigned long)word[j];
    }

    /*========================
     * Return the hash value.
     *========================*/

    if( range == 0 )
    {
        return(count);
    }

    tally = (count % range);

    return(tally);
}

/****************************************************
 * dec_atom_count: Decrements the count value
 *   for a symbol_table entry. Adds the symbol to the
 *   atom_ephemerons if the count becomes zero.
 *****************************************************/
void dec_atom_count(void *env, ATOM_HN *val)
{
    if( val->count < 0 )
    {
        error_system(env, "ATOM", 3);
        exit_router(env, EXIT_FAILURE);
    }

    if( val->count == 0 )
    {
        error_system(env, "ATOM", 4);
        exit_router(env, EXIT_FAILURE);
    }

    val->count--;

    if( val->count != 0 )
    {
        return;
    }

    if( val->is_ephemeral == FALSE )
    {
        _add_ephemeron(env, (GENERIC_HN *)val, &get_atom_data(env)->atom_ephemerons,
                       sizeof(ATOM_HN), AVERAGE_STRING_SIZE);
    }

    return;
}

/**************************************************
 * dec_float_count: Decrements the count value
 *   for a float_table entry. Adds the float to the
 *   float_ephemerons if the count becomes zero.
 ***************************************************/
void dec_float_count(void *env, FLOAT_HN *val)
{
    if( val->count <= 0 )
    {
        error_system(env, "ATOM", 5);
        exit_router(env, EXIT_FAILURE);
    }

    val->count--;

    if( val->count != 0 )
    {
        return;
    }

    if( val->is_ephemeral == FALSE )
    {
        _add_ephemeron(env, (GENERIC_HN *)val, &get_atom_data(env)->float_ephemerons,
                       sizeof(FLOAT_HN), 0);
    }

    return;
}

/********************************************************
 * dec_integer_count: Decrements the count value for
 *   an integer_table entry. Adds the integer to the
 *   integer_ephemerons if the count becomes zero.
 *********************************************************/
void dec_integer_count(void *env, INTEGER_HN *val)
{
    if( val->count <= 0 )
    {
        error_system(env, "ATOM", 6);
        exit_router(env, EXIT_FAILURE);
    }

    val->count--;

    if( val->count != 0 )
    {
        return;
    }

    if( val->is_ephemeral == FALSE )
    {
        _add_ephemeron(env, (GENERIC_HN *)val, &get_atom_data(env)->integer_ephemerons,
                       sizeof(INTEGER_HN), 0);
    }

    return;
}

/****************************************************
 * dec_bitmap_count: Decrements the count value
 *   for a BitmapTable entry. Adds the bitmap to the
 *   bitmap_ephemerons if the count becomes zero.
 *****************************************************/
void dec_bitmap_count(void *env, BITMAP_HN *val)
{
    if( val->count < 0 )
    {
        error_system(env, "ATOM", 7);
        exit_router(env, EXIT_FAILURE);
    }

    if( val->count == 0 )
    {
        error_system(env, "ATOM", 8);
        exit_router(env, EXIT_FAILURE);
    }

    val->count--;

    if( val->count != 0 )
    {
        return;
    }

    if( val->is_ephemeral == FALSE )
    {
        _add_ephemeron(env, (GENERIC_HN *)val, &get_atom_data(env)->bitmap_ephemerons,
                       sizeof(BITMAP_HN), sizeof(long));
    }

    return;
}

/************************************************************
 * dec_external_address_count: Decrements the count value
 *   for an ExternAddressTable entry. Adds the bitmap to the
 *   external_address_ephemerons if the count becomes zero.
 *************************************************************/
void dec_external_address_count(void *env, EXTERNAL_ADDRESS_HN *val)
{
    if( val->count < 0 )
    {
        error_system(env, "ATOM", 9);
        exit_router(env, EXIT_FAILURE);
    }

    if( val->count == 0 )
    {
        error_system(env, "ATOM", 10);
        exit_router(env, EXIT_FAILURE);
    }

    val->count--;

    if( val->count != 0 )
    {
        return;
    }

    if( val->is_ephemeral == FALSE )
    {
        _add_ephemeron(env, (GENERIC_HN *)val, &get_atom_data(env)->external_address_ephemerons,
                       sizeof(EXTERNAL_ADDRESS_HN), sizeof(long));
    }

    return;
}

/***********************************************
 * RemoveHashNode: Removes a hash node from the
 *   symbol_table, float_table, integer_table,
 *   bitmap_table, or external_address_table.
 ************************************************/
static void _remove_hash_node(void *env, GENERIC_HN *val, GENERIC_HN **theTable, int size, int type)
{
    GENERIC_HN *previousNode, *currentNode;
    struct external_address_hash_node *theAddress;

    /*=============================================
     * Find the entry in the specified hash table.
     *=============================================*/

    previousNode = NULL;
    currentNode = theTable[val->bucket];

    while( currentNode != val )
    {
        previousNode = currentNode;
        currentNode = currentNode->next;

        if( currentNode == NULL )
        {
            error_system(env, "ATOM", 11);
            exit_router(env, EXIT_FAILURE);
        }
    }

    /*===========================================
     * Remove the entry from the list of entries
     * stored in the hash table bucket.
     *===========================================*/

    if( previousNode == NULL )
    {
        theTable[val->bucket] = val->next;
    }
    else
    {
        previousNode->next = currentNode->next;
    }

    /*=================================================
     * Symbol and bit map nodes have additional memory
     * use to store the character or bitmap string.
     *=================================================*/

    if( type == ATOM )
    {
        core_mem_release(env, ((ATOM_HN *)val)->contents,
                         strlen(((ATOM_HN *)val)->contents) + 1);
    }
    else if( type == BITMAPARRAY )
    {
        core_mem_release(env, ((BITMAP_HN *)val)->contents,
                         ((BITMAP_HN *)val)->size);
    }
    else if( type == EXTERNAL_ADDRESS )
    {
        theAddress = (struct external_address_hash_node *)val;

        if((core_get_evaluation_data(env)->ext_address_types[theAddress->type] != NULL) &&
           (core_get_evaluation_data(env)->ext_address_types[theAddress->type]->discard_fn != NULL))
        {
            (*core_get_evaluation_data(env)->ext_address_types[theAddress->type]->discard_fn)(env, theAddress->address);
        }
    }

    /*===========================
     * Return the table entry to
     * the pool of free memory.
     *===========================*/

    core_mem_return_fixed_struct(env, size, val);
}

/**********************************************************
 * AddEphemeralHashNode: Adds a symbol, integer, float, or
 *   bit map table entry to the list of is_ephemeral atomic
 *   values. These entries have a zero count indicating
 *   that no structure is using the data value.
 ***********************************************************/
static void _add_ephemeron(void *env, GENERIC_HN *theHashNode, struct ephemeron **theEphemeralList, int hashNodeSize, int averageContentsSize)
{
    struct ephemeron *temp;

    /*===========================================
     * If the count isn't zero then this routine
     * should never have been called.
     *===========================================*/

    if( theHashNode->count != 0 )
    {
        error_system(env, "ATOM", 12);
        exit_router(env, EXIT_FAILURE);
    }

    /*=====================================
     * Mark the atomic value as is_ephemeral.
     *=====================================*/

    theHashNode->is_ephemeral = TRUE;

    /*=============================
     * Add the atomic value to the
     * list of is_ephemeral values.
     *=============================*/

    temp = core_mem_get_struct(env, ephemeron);
    temp->its_value = theHashNode;
    temp->next = *theEphemeralList;
    *theEphemeralList = temp;

    /*=========================================================
     * Increment the is_ephemeral count and size variables. These
     * variables are used by the garbage collection routines
     * to determine when garbage collection should occur.
     *=========================================================*/

    core_get_gc_data(env)->generational_item_count++;
    core_get_gc_data(env)->generational_item_sz += sizeof(struct ephemeron) + hashNodeSize +
                                                   averageContentsSize;
}

/**************************************************
 * remove_ephemeral_atoms: Causes the removal of all
 *   is_ephemeral symbols, integers, floats, and bit
 *   maps that still have a count value of zero,
 *   from their respective storage tables.
 ***************************************************/
void remove_ephemeral_atoms(void *env)
{
    _remove_ephemeron(env, &get_atom_data(env)->atom_ephemerons, (GENERIC_HN **)get_atom_data(env)->symbol_table,
                      sizeof(ATOM_HN), ATOM, AVERAGE_STRING_SIZE);
    _remove_ephemeron(env, &get_atom_data(env)->float_ephemerons, (GENERIC_HN **)get_atom_data(env)->float_table,
                      sizeof(FLOAT_HN), FLOAT, 0);
    _remove_ephemeron(env, &get_atom_data(env)->integer_ephemerons, (GENERIC_HN **)get_atom_data(env)->integer_table,
                      sizeof(INTEGER_HN), INTEGER, 0);
    _remove_ephemeron(env, &get_atom_data(env)->bitmap_ephemerons, (GENERIC_HN **)get_atom_data(env)->bitmap_table,
                      sizeof(BITMAP_HN), BITMAPARRAY, AVERAGE_BITMAP_SIZE);
    _remove_ephemeron(env, &get_atom_data(env)->external_address_ephemerons, (GENERIC_HN **)get_atom_data(env)->external_address_table,
                      sizeof(EXTERNAL_ADDRESS_HN), EXTERNAL_ADDRESS, 0);
}

/***************************************************************
 * RemoveEphemeralHashNodes: Removes symbols from the is_ephemeral
 *   symbol list that have a count of zero and were placed on
 *   the list at a higher level than the current evaluation
 *   depth. Since symbols are ordered in the list in descending
 *   order, the removal process can end when a depth is reached
 *   less than the current evaluation depth. Because is_ephemeral
 *   symbols can be "pulled" up through an evaluation depth,
 *   this routine needs to check through both the previous and
 *   current evaluation depth.
 ****************************************************************/
static void _remove_ephemeron(void *env, struct ephemeron **theEphemeralList, GENERIC_HN **theTable, int hashNodeSize, int hashNodeType, int averageContentsSize)
{
    struct ephemeron *edPtr, *lastPtr = NULL, *nextPtr;

    edPtr = *theEphemeralList;

    while( edPtr != NULL )
    {
        /*======================================================
         * Check through previous and current evaluation depth
         * because these symbols can be interspersed, otherwise
         * symbols are stored in descending evaluation depth.
         *======================================================*/

        nextPtr = edPtr->next;

        /*==================================================
         * Remove any symbols that have a count of zero and
         * were added to the is_ephemeral list at a higher
         * evaluation depth.
         *==================================================*/

        if((edPtr->its_value->count == 0) &&
           (edPtr->its_value->depth > core_get_evaluation_data(env)->eval_depth))
        {
            _remove_hash_node(env, edPtr->its_value, theTable, hashNodeSize, hashNodeType);
            core_mem_return_struct(env, ephemeron, edPtr);

            if( lastPtr == NULL )
            {
                *theEphemeralList = nextPtr;
            }
            else
            {
                lastPtr->next = nextPtr;
            }

            core_get_gc_data(env)->generational_item_count--;
            core_get_gc_data(env)->generational_item_sz -= sizeof(struct ephemeron) + hashNodeSize +
                                                           averageContentsSize;
        }

        /*=======================================
         * Remove is_ephemeral status of any symbol
         * with a count greater than zero.
         *=======================================*/

        else if( edPtr->its_value->count > 0 )
        {
            edPtr->its_value->is_ephemeral = FALSE;

            core_mem_return_struct(env, ephemeron, edPtr);

            if( lastPtr == NULL )
            {
                *theEphemeralList = nextPtr;
            }
            else
            {
                lastPtr->next = nextPtr;
            }

            core_get_gc_data(env)->generational_item_count--;
            core_get_gc_data(env)->generational_item_sz -= sizeof(struct ephemeron) + hashNodeSize +
                                                           averageContentsSize;
        }

        /*==================================================
         * Otherwise keep the symbol in the is_ephemeral list.
         *==================================================*/

        else
        {
            lastPtr = edPtr;
        }

        edPtr = nextPtr;
    }
}

/********************************************************
 * get_symbol_table: Returns a pointer to the symbol_table.
 *********************************************************/
ATOM_HN **get_symbol_table(void *env)
{
    return(get_atom_data(env)->symbol_table);
}

/*****************************************************
 * set_symbol_table: Sets the value of the symbol_table.
 ******************************************************/
void set_symbol_table(void *env, ATOM_HN **value)
{
    get_atom_data(env)->symbol_table = value;
}

/******************************************************
 * get_float_table: Returns a pointer to the float_table.
 *******************************************************/
FLOAT_HN **get_float_table(void *env)
{
    return(get_atom_data(env)->float_table);
}

/***************************************************
 * set_float_table: Sets the value of the float_table.
 ****************************************************/
void set_float_table(void *env, FLOAT_HN **value)
{
    get_atom_data(env)->float_table = value;
}

/**********************************************************
 * get_integer_table: Returns a pointer to the integer_table.
 ***********************************************************/
INTEGER_HN **get_integer_table(void *env)
{
    return(get_atom_data(env)->integer_table);
}

/*******************************************************
 * set_integer_table: Sets the value of the integer_table.
 ********************************************************/
void set_integer_table(void *env, INTEGER_HN **value)
{
    get_atom_data(env)->integer_table = value;
}

/********************************************************
 * get_bitmap_table: Returns a pointer to the bitmap_table.
 *********************************************************/
BITMAP_HN **get_bitmap_table(void *env)
{
    return(get_atom_data(env)->bitmap_table);
}

/*****************************************************
 * set_bitmap_table: Sets the value of the bitmap_table.
 ******************************************************/
void set_bitmap_table(void *env, BITMAP_HN **value)
{
    get_atom_data(env)->bitmap_table = value;
}

/**************************************************************************
 * get_external_address_table: Returns a pointer to the external_address_table.
 ***************************************************************************/
EXTERNAL_ADDRESS_HN **get_external_address_table(void *env)
{
    return(get_atom_data(env)->external_address_table);
}

/***********************************************************************
 * set_external_address_table: Sets the value of the external_address_table.
 ************************************************************************/
void set_external_address_table(void *env, EXTERNAL_ADDRESS_HN **value)
{
    get_atom_data(env)->external_address_table = value;
}

/*****************************************************
 * reset_special_atoms: Resets the values of the
 *   TrueSymbol, FalseSymbol, zero_atom, positive_inf_atom,
 *   and negative_inf_atom symbols.
 ******************************************************/
void reset_special_atoms(void *env)
{
    get_atom_data(env)->true_atom = (void *)lookup_atom(env, TRUE_STRING);
    get_atom_data(env)->false_atom = (void *)lookup_atom(env, FALSE_STRING);
    get_atom_data(env)->positive_inf_atom = (void *)lookup_atom(env, POSITIVE_INFINITY_STRING);
    get_atom_data(env)->negative_inf_atom = (void *)lookup_atom(env, NEGATIVE_INFINITY_STRING);
    get_atom_data(env)->zero_atom = (void *)lookup_long(env, 0L);
}

/**********************************************************
 * find_atom_matches: Finds all symbols in the symbol_table
 *   which begin with a specified symbol. This function is
 *   used to implement the command completion feature
 *   found in some of the machine specific interfaces.
 ***********************************************************/
struct atom_match *find_atom_matches(void *env, char *searchString, unsigned *numberOfMatches, size_t *commonPrefixLength)
{
    struct atom_match *reply = NULL, *temp;
    struct atom_hash_node *hashPtr = NULL;
    size_t searchLength;

    searchLength = strlen(searchString);
    *numberOfMatches = 0;

    while((hashPtr = next_atom_match(env, searchString, searchLength, hashPtr,
                                     FALSE, commonPrefixLength)) != NULL )
    {
        *numberOfMatches = *numberOfMatches + 1;
        temp = core_mem_get_struct(env, atom_match);
        temp->match = hashPtr;
        temp->next = reply;
        reply = temp;
    }

    return(reply);
}

/********************************************************
 * return_atom_matches: Returns a set of symbol matches.
 *********************************************************/
void return_atom_matches(void *env, struct atom_match *listOfMatches)
{
    struct atom_match *temp;

    while( listOfMatches != NULL )
    {
        temp = listOfMatches->next;
        core_mem_return_struct(env, atom_match, listOfMatches);
        listOfMatches = temp;
    }
}

/**************************************************************
 * init_bitmap: Initializes the values of a bitmap to zero.
 ***************************************************************/
void init_bitmap(void *vTheBitMap, unsigned length)
{
    char *theBitMap = (char *)vTheBitMap;
    unsigned i;

    for( i = 0; i < length; i++ )
    {
        theBitMap[i] = '\0';
    }
}

/****************************************************************
 * next_atom_match: Finds the next symbol in the symbol_table
 *   which begins with a specified symbol. This function is used
 *   to implement the command completion feature found in some
 *   of the machine specific interfaces.
 *****************************************************************/
ATOM_HN *next_atom_match(void *env, char *searchString, size_t searchLength, ATOM_HN *prevSymbol, int anywhere, size_t *commonPrefixLength)
{
    register unsigned long i;
    ATOM_HN *hashPtr;
    int flag = TRUE;
    size_t prefixLength;

    /*==========================================
     * If we're looking anywhere in the string,
     * then there's no common prefix length.
     *==========================================*/

    if( anywhere && (commonPrefixLength != NULL))
    {
        *commonPrefixLength = 0;
    }

    /*========================================================
     * If we're starting the search from the beginning of the
     * symbol table, the previous symbol argument is NULL.
     *========================================================*/

    if( prevSymbol == NULL )
    {
        i = 0;
        hashPtr = get_atom_data(env)->symbol_table[0];
    }

    /*==========================================
     * Otherwise start the search at the symbol
     * after the last symbol found.
     *==========================================*/

    else
    {
        i = prevSymbol->bucket;
        hashPtr = prevSymbol->next;
    }

    /*==============================================
     * Search through all the symbol table buckets.
     *==============================================*/

    while( flag )
    {
        /*===================================
         * Search through all of the entries
         * in the bucket being examined.
         *===================================*/

        for(; hashPtr != NULL; hashPtr = hashPtr->next )
        {
            /*================================================
             * Skip symbols that being with ( since these are
             * typically symbols for internal use. Also skip
             * any symbols that are marked is_ephemeral since
             * these aren't in use.
             *================================================*/

            if((hashPtr->contents[0] == '(') ||
               (hashPtr->is_ephemeral))
            {
                continue;
            }

            /*==================================================
             * Two types of matching can be performed: the type
             * comparing just to the beginning of the string
             * and the type which looks for the substring
             * anywhere within the string being examined.
             *==================================================*/

            if( !anywhere )
            {
                /*=============================================
                 * Determine the common prefix length between
                 * the previously found match (if available or
                 * the search string if not) and the symbol
                 * table entry.
                 *=============================================*/

                if( prevSymbol != NULL )
                {
                    prefixLength = _common_prefix_length(prevSymbol->contents, hashPtr->contents);
                }
                else
                {
                    prefixLength = _common_prefix_length(searchString, hashPtr->contents);
                }

                /*===================================================
                 * If the prefix length is greater than or equal to
                 * the length of the search string, then we've found
                 * a match. If this is the first match, the common
                 * prefix length is set to the length of the first
                 * match, otherwise the common prefix length is the
                 * smallest prefix length found among all matches.
                 *===================================================*/

                if( prefixLength >= searchLength )
                {
                    if( commonPrefixLength != NULL )
                    {
                        if( prevSymbol == NULL )
                        {
                            *commonPrefixLength = strlen(hashPtr->contents);
                        }
                        else if( prefixLength < *commonPrefixLength )
                        {
                            *commonPrefixLength = prefixLength;
                        }
                    }

                    return(hashPtr);
                }
            }
            else
            {
                if( _is_string_in_string(hashPtr->contents, searchString) != NULL )
                {
                    return(hashPtr);
                }
            }
        }

        /*=================================================
         * Move on to the next bucket in the symbol table.
         *=================================================*/

        if( ++i >= ATOM_HASH_SZ )
        {
            flag = FALSE;
        }
        else
        {
            hashPtr = get_atom_data(env)->symbol_table[i];
        }
    }

    /*=====================================
     * There are no more matching symbols.
     *=====================================*/

    return(NULL);
}

/*********************************************
 * StringWithinString: Determines if a string
 *   is contained within another string.
 **********************************************/
static char *_is_string_in_string(char *cs, char *ct)
{
    register unsigned i, j, k;

    for( i = 0 ; cs[i] != '\0' ; i++ )
    {
        for( j = i, k = 0 ; ct[k] != '\0' && cs[j] == ct[k] ; j++, k++ )
        {
            ;
        }

        if((ct[k] == '\0') && (k != 0))
        {
            return(cs + i);
        }
    }

    return(NULL);
}

/***********************************************
 * CommonPrefixLength: Determines the length of
 *    the maximumcommon prefix of two strings
 ************************************************/
static size_t _common_prefix_length(char *cs, char *ct)
{
    register unsigned i;

    for( i = 0 ; (cs[i] != '\0') && (ct[i] != '\0') ; i++ )
    {
        if( cs[i] != ct[i] )
        {
            break;
        }
    }

    return(i);
}
