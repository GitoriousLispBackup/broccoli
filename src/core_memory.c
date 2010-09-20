/* Purpose: Memory allocation routines.                      */

#define _MEMORY_SOURCE_

#include <stdio.h>
#define _STDIO_INCLUDED_

#include "setup.h"

#include "constant.h"
#include "core_environment.h"
#include "core_memory.h"
#include "router.h"
#include "core_gc.h"

#include <stdlib.h>

#if WIN_BTC
#include <alloc.h>
#endif
#if WIN_MVC
#include <malloc.h>
#endif

#define STRICT_ALIGN_SIZE sizeof(double)

#define _special_malloc(sz) malloc((STD_SIZE)sz)
#define _special_free(ptr)  free(ptr)

/**************************************
 * LOCAL FUNCTION PROTOTYPES
 ***************************************/

#if BLOCK_MEMORY
static int InitializeBlockMemory(void *, unsigned int);
static int AllocateBlock(void *, struct core_mem_block *, unsigned int);
static void AllocateChunk(void *, struct core_mem_block *, struct core_mem_chunk *, size_t);
#endif

/*******************************************
 * core_mem_init_memory: Sets up memory tables.
 ********************************************/
void core_mem_init_memory(void *env)
{
    int i;

    core_allocate_environment_data(env, MEMORY_DATA_INDEX, sizeof(struct core_mem_memory), NULL);

    core_mem_get_memory_data(env)->fn_out_of_memory = core_mem_fn_out_of_memory;

    core_mem_get_memory_data(env)->memory_table = (struct core_mem_ptr **)malloc((STD_SIZE)(sizeof(struct core_mem_ptr *) *MEM_TABLE_SIZE));

    if( core_mem_get_memory_data(env)->memory_table == NULL )
    {
        error_print_id(env, "MEMORY", 1, TRUE);
        print_router(env, WERROR, "Out of memory.\n");
        exit_router(env, EXIT_FAILURE);
    }

    for( i = 0; i < MEM_TABLE_SIZE; i++ )
    {
        core_mem_get_memory_data(env)->memory_table[i] = NULL;
    }
}

/**************************************************
 * core_mem_alloc: A generic memory allocation function.
 ***************************************************/
void *core_mem_alloc(void *env, size_t size)
{
    char *memPtr;

#if   BLOCK_MEMORY
    memPtr = (char *)RequestChunk(env, size);

    if( memPtr == NULL )
    {
        core_mem_release_count(env, (long)((size * 5 > 4096) ? size * 5 : 4096), FALSE);
        memPtr = (char *)RequestChunk(env, size);

        if( memPtr == NULL )
        {
            core_mem_release_count(env, -1L, TRUE);
            memPtr = (char *)RequestChunk(env, size);

            while( memPtr == NULL )
            {
                if((*core_mem_get_memory_data(env)->fn_out_of_memory)(env, (unsigned long)size))
                {
                    return(NULL);
                }

                memPtr = (char *)RequestChunk(env, size);
            }
        }
    }

#else
    memPtr = (char *)malloc(size);

    if( memPtr == NULL )
    {
        core_mem_release_count(env, (long)((size * 5 > 4096) ? size * 5 : 4096), FALSE);
        memPtr = (char *)malloc(size);

        if( memPtr == NULL )
        {
            core_mem_release_count(env, -1L, TRUE);
            memPtr = (char *)malloc(size);

            while( memPtr == NULL )
            {
                if((*core_mem_get_memory_data(env)->fn_out_of_memory)(env, size))
                {
                    return(NULL);
                }

                memPtr = (char *)malloc(size);
            }
        }
    }

#endif

    core_mem_get_memory_data(env)->amount += (long)size;
    core_mem_get_memory_data(env)->calls++;

    return((void *)memPtr);
}

/**********************************************
 * core_mem_fn_out_of_memory: Function called
 *   when the KB runs out of memory.
 ***********************************************/
#if WIN_BTC
#pragma argsused
#endif
int core_mem_fn_out_of_memory(void *env, size_t size)
{
#if MAC_MCW || WIN_MCW || MAC_XCD
#pragma unused(size)
#endif

    error_print_id(env, "MEMORY", 1, TRUE);
    print_router(env, WERROR, "Out of memory.\n");
    exit_router(env, EXIT_FAILURE);
    return(TRUE);
}

/***************************************************
 * core_mem_free: A generic memory deallocation function.
 ****************************************************/
int core_mem_free(void *env, void *waste, size_t size)
{
#if BLOCK_MEMORY

    if( ReturnChunk(env, waste, size) == FALSE )
    {
        error_print_id(env, "MEMORY", 2, TRUE);
        print_router(env, WERROR, "Release error in genfree.\n");
        return(-1);
    }

#else
    free(waste);
#endif

    core_mem_get_memory_data(env)->amount -= (long)size;
    core_mem_get_memory_data(env)->calls--;

    return(0);
}

/*****************************************************
 * core_mem_realloc: Simple (i.e. dumb) version of realloc.
 ******************************************************/
void *core_mem_realloc(void *env, void *oldaddr, size_t oldsz, size_t newsz)
{
    char *newaddr;
    unsigned i;
    size_t limit;

    newaddr = ((newsz != 0) ? (char *)core_mem_alloc_no_init(env, newsz) : NULL);

    if( oldaddr != NULL )
    {
        limit = (oldsz < newsz) ? oldsz : newsz;

        for( i = 0 ; i < limit ; i++ )
        {
            newaddr[i] = ((char *)oldaddr)[i];
        }

        for(; i < newsz; i++ )
        {
            newaddr[i] = '\0';
        }

        core_mem_release(env, (void *)oldaddr, oldsz);
    }

    return((void *)newaddr);
}

/**********************************
 * core_mem_release_count: C access routine
 *   for the release-mem command.
 ***********************************/
long int core_mem_release_count(void *env, long int maximum, int printMessage)
{
    struct core_mem_ptr *tmpPtr, *memPtr;
    int i;
    long int returns = 0;
    long int amount = 0;

    if( printMessage == TRUE )
    {
        print_router(env, WDIALOG, "\n*** DEALLOCATING MEMORY ***\n");
    }

    for( i = (MEM_TABLE_SIZE - 1) ; i >= (int)sizeof(char *) ; i-- )
    {
        core_gc_yield_time(env);
        memPtr = core_mem_get_memory_data(env)->memory_table[i];

        while( memPtr != NULL )
        {
            tmpPtr = memPtr->next;
            core_mem_free(env, (void *)memPtr, (unsigned)i);
            memPtr = tmpPtr;
            amount += i;
            returns++;

            if((returns % 100) == 0 )
            {
                core_gc_yield_time(env);
            }
        }

        core_mem_get_memory_data(env)->memory_table[i] = NULL;

        if((amount > maximum) && (maximum > 0))
        {
            if( printMessage == TRUE )
            {
                print_router(env, WDIALOG, "*** MEMORY  DEALLOCATED ***\n");
            }

            return(amount);
        }
    }

    if( printMessage == TRUE )
    {
        print_router(env, WDIALOG, "*** MEMORY  DEALLOCATED ***\n");
    }

    return(amount);
}

/****************************************************
 * core_mem_alloc_and_init: Allocates memory and sets all bytes to zero.
 *****************************************************/
void *core_mem_alloc_and_init(void *env, size_t size)
{
    struct core_mem_ptr *memPtr;
    char *tmpPtr;
    size_t i;

    if( size < (long)sizeof(char *))
    {
        size = sizeof(char *);
    }

    if( size >= MEM_TABLE_SIZE )
    {
        tmpPtr = (char *)core_mem_alloc(env, (unsigned)size);

        for( i = 0 ; i < size ; i++ )
        {
            tmpPtr[i] = '\0';
        }

        return((void *)tmpPtr);
    }

    memPtr = (struct core_mem_ptr *)core_mem_get_memory_data(env)->memory_table[size];

    if( memPtr == NULL )
    {
        tmpPtr = (char *)core_mem_alloc(env, (unsigned)size);

        for( i = 0 ; i < size ; i++ )
        {
            tmpPtr[i] = '\0';
        }

        return((void *)tmpPtr);
    }

    core_mem_get_memory_data(env)->memory_table[size] = memPtr->next;

    tmpPtr = (char *)memPtr;

    for( i = 0 ; i < size ; i++ )
    {
        tmpPtr[i] = '\0';
    }

    return((void *)tmpPtr);
}

/****************************************************
 * core_mem_alloc_no_init: Allocates memory and does not initialize it.
 *****************************************************/
void *core_mem_alloc_no_init(void *env, size_t size)
{
    struct core_mem_ptr *memPtr;

    if( size < sizeof(char *))
    {
        size = sizeof(char *);
    }

    if( size >= MEM_TABLE_SIZE )
    {
        return(core_mem_alloc(env, (unsigned)size));
    }

    memPtr = (struct core_mem_ptr *)core_mem_get_memory_data(env)->memory_table[size];

    if( memPtr == NULL )
    {
        return(core_mem_alloc(env, (unsigned)size));
    }

    core_mem_get_memory_data(env)->memory_table[size] = memPtr->next;

    return((void *)memPtr);
}

/****************************************************
 * core_mem_alloc_large_no_init: Allocates memory and does not initialize it.
 *****************************************************/
void *core_mem_alloc_large_no_init(void *env, size_t size)
{
    struct core_mem_ptr *memPtr;

    if( size < (long)sizeof(char *))
    {
        size = sizeof(char *);
    }

    if( size >= MEM_TABLE_SIZE )
    {
        return(core_mem_alloc(env, size));
    }

    memPtr = (struct core_mem_ptr *)core_mem_get_memory_data(env)->memory_table[(int)size];

    if( memPtr == NULL )
    {
        return(core_mem_alloc(env, size));
    }

    core_mem_get_memory_data(env)->memory_table[(int)size] = memPtr->next;

    return((void *)memPtr);
}

/***************************************
 * core_mem_release: Returns a block of memory to the
 *   maintained pool of free memory.
 ****************************************/
int core_mem_release(void *env, void *str, size_t size)
{
    struct core_mem_ptr *memPtr;

    if( size == 0 )
    {
        error_system(env, "MEMORY", 1);
        exit_router(env, EXIT_FAILURE);
    }

    if( size < sizeof(char *))
    {
        size = sizeof(char *);
    }

    if( size >= MEM_TABLE_SIZE )
    {
        return(core_mem_free(env, (void *)str, (unsigned)size));
    }

    memPtr = (struct core_mem_ptr *)str;
    memPtr->next = core_mem_get_memory_data(env)->memory_table[size];
    core_mem_get_memory_data(env)->memory_table[size] = memPtr;
    return(1);
}

/*******************************************
 * core_mem_release_sized: Returns a block of memory to the
 *   maintained pool of free memory that's
 *   size is indicated with a long integer.
 ********************************************/
int core_mem_release_sized(void *env, void *str, size_t size)
{
    struct core_mem_ptr *memPtr;

    if( size == 0 )
    {
        error_system(env, "MEMORY", 1);
        exit_router(env, EXIT_FAILURE);
    }

    if( size < (long)sizeof(char *))
    {
        size = sizeof(char *);
    }

    if( size >= MEM_TABLE_SIZE )
    {
        return(core_mem_free(env, (void *)str, (unsigned long)size));
    }

    memPtr = (struct core_mem_ptr *)str;
    memPtr->next = core_mem_get_memory_data(env)->memory_table[(int)size];
    core_mem_get_memory_data(env)->memory_table[(int)size] = memPtr;
    return(1);
}

/**************************************************
 * core_mem_get_pool_sz: Returns number of bytes in free pool.
 ***************************************************/
unsigned long core_mem_get_pool_sz(void *env)
{
    register int i;
    struct core_mem_ptr *memPtr;
    unsigned long cnt = 0;

    for( i = sizeof(char *) ; i < MEM_TABLE_SIZE ; i++ )
    {
        memPtr = core_mem_get_memory_data(env)->memory_table[i];

        while( memPtr != NULL )
        {
            cnt += (unsigned long)i;
            memPtr = memPtr->next;
        }
    }

    return(cnt);
}

/*************************
 * core_mem_memcopy:
 **************************/
void core_mem_memcopy(char *dst, char *src, unsigned long size)
{
    unsigned long i;

    for( i = 0L ; i < size ; i++ )
    {
        dst[i] = src[i];
    }
}

/*************************
 * BLOCK MEMORY FUNCTIONS
 **************************/

#if BLOCK_MEMORY

/**************************************************
 * InitializeBlockMemory: Initializes block memory
 *   management and allocates the first block.
 ***************************************************/
static int InitializeBlockMemory(void *env, unsigned int requestSize)
{
    struct core_mem_chunk *chunkPtr;
    unsigned int initialBlockSize, usableBlockSize;

    /*===========================================
     * The block memory routines depend upon the
     * size of a character being 1 byte.
     *===========================================*/

    if( sizeof(char) != 1 )
    {
        fprintf(stdout, "Size of character data is not 1\n");
        fprintf(stdout, "Memory allocation functions may not work\n");
        return(0);
    }

    core_mem_get_memory_data(env)->ChunkInfoSize = sizeof(struct core_mem_chunk);
    core_mem_get_memory_data(env)->ChunkInfoSize = (int)((((core_mem_get_memory_data(env)->ChunkInfoSize - 1) / STRICT_ALIGN_SIZE) + 1) * STRICT_ALIGN_SIZE);

    core_mem_get_memory_data(env)->BlockInfoSize = sizeof(struct core_mem_block);
    core_mem_get_memory_data(env)->BlockInfoSize = (int)((((core_mem_get_memory_data(env)->BlockInfoSize - 1) / STRICT_ALIGN_SIZE) + 1) * STRICT_ALIGN_SIZE);

    initialBlockSize = (INITBLOCKSIZE > requestSize ? INITBLOCKSIZE : requestSize);
    initialBlockSize += core_mem_get_memory_data(env)->ChunkInfoSize * 2 + core_mem_get_memory_data(env)->BlockInfoSize;
    initialBlockSize = (((initialBlockSize - 1) / STRICT_ALIGN_SIZE) + 1) * STRICT_ALIGN_SIZE;

    usableBlockSize = initialBlockSize - (2 * core_mem_get_memory_data(env)->ChunkInfoSize) - core_mem_get_memory_data(env)->BlockInfoSize;

    /* make sure we get a buffer big enough to be usable */
    if((requestSize < INITBLOCKSIZE) &&
       (usableBlockSize <= requestSize + core_mem_get_memory_data(env)->ChunkInfoSize))
    {
        initialBlockSize = requestSize + core_mem_get_memory_data(env)->ChunkInfoSize * 2 + core_mem_get_memory_data(env)->BlockInfoSize;
        initialBlockSize = (((initialBlockSize - 1) / STRICT_ALIGN_SIZE) + 1) * STRICT_ALIGN_SIZE;
        usableBlockSize = initialBlockSize - (2 * core_mem_get_memory_data(env)->ChunkInfoSize) - core_mem_get_memory_data(env)->BlockInfoSize;
    }

    core_mem_get_memory_data(env)->TopMemoryBlock = (struct core_mem_block *)malloc((STD_SIZE)initialBlockSize);

    if( core_mem_get_memory_data(env)->TopMemoryBlock == NULL )
    {
        fprintf(stdout, "Unable to allocate initial memory pool\n");
        return(0);
    }

    core_mem_get_memory_data(env)->TopMemoryBlock->next = NULL;
    core_mem_get_memory_data(env)->TopMemoryBlock->previous = NULL;
    core_mem_get_memory_data(env)->TopMemoryBlock->next_free = (struct core_mem_chunk *)(((char *)core_mem_get_memory_data(env)->TopMemoryBlock) + core_mem_get_memory_data(env)->BlockInfoSize);
    core_mem_get_memory_data(env)->TopMemoryBlock->size = (long)usableBlockSize;

    chunkPtr = (struct core_mem_chunk *)(((char *)core_mem_get_memory_data(env)->TopMemoryBlock) + core_mem_get_memory_data(env)->BlockInfoSize + core_mem_get_memory_data(env)->ChunkInfoSize + usableBlockSize);
    chunkPtr->next_free = NULL;
    chunkPtr->last_free = NULL;
    chunkPtr->previous = core_mem_get_memory_data(env)->TopMemoryBlock->next_free;
    chunkPtr->size = 0;

    core_mem_get_memory_data(env)->TopMemoryBlock->next_free->next_free = NULL;
    core_mem_get_memory_data(env)->TopMemoryBlock->next_free->last_free = NULL;
    core_mem_get_memory_data(env)->TopMemoryBlock->next_free->previous = NULL;
    core_mem_get_memory_data(env)->TopMemoryBlock->next_free->size = (long)usableBlockSize;

    core_mem_get_memory_data(env)->BlockMemoryInitialized = TRUE;
    return(1);
}

/**************************************************************************
 * AllocateBlock: Adds a new block of memory to the list of memory blocks.
 ***************************************************************************/
static int AllocateBlock(void *env, struct core_mem_block *blockPtr, unsigned int requestSize)
{
    unsigned int blockSize, usableBlockSize;
    struct core_mem_block *newBlock;
    struct core_mem_chunk *newTopChunk;

    /*============================================================
     * Determine the size of the block that needs to be allocated
     * to satisfy the request. Normally, a default block size is
     * used, but if the requested size is larger than the default
     * size, then the requested size is used for the block size.
     *============================================================*/

    blockSize = (BLOCKSIZE > requestSize ? BLOCKSIZE : requestSize);
    blockSize += core_mem_get_memory_data(env)->BlockInfoSize + core_mem_get_memory_data(env)->ChunkInfoSize * 2;
    blockSize = (((blockSize - 1) / STRICT_ALIGN_SIZE) + 1) * STRICT_ALIGN_SIZE;

    usableBlockSize = blockSize - core_mem_get_memory_data(env)->BlockInfoSize - (2 * core_mem_get_memory_data(env)->ChunkInfoSize);

    /*=========================
     * Allocate the new block.
     *=========================*/

    newBlock = (struct core_mem_block *)malloc((STD_SIZE)blockSize);

    if( newBlock == NULL )
    {
        return(0);
    }

    /*======================================
     * Initialize the block data structure.
     *======================================*/

    newBlock->next = NULL;
    newBlock->previous = blockPtr;
    newBlock->next_free = (struct core_mem_chunk *)(((char *)newBlock) + core_mem_get_memory_data(env)->BlockInfoSize);
    newBlock->size = (long)usableBlockSize;
    blockPtr->next = newBlock;

    newTopChunk = (struct core_mem_chunk *)(((char *)newBlock) + core_mem_get_memory_data(env)->BlockInfoSize + core_mem_get_memory_data(env)->ChunkInfoSize + usableBlockSize);
    newTopChunk->next_free = NULL;
    newTopChunk->last_free = NULL;
    newTopChunk->size = 0;
    newTopChunk->previous = newBlock->next_free;

    newBlock->next_free->next_free = NULL;
    newBlock->next_free->last_free = NULL;
    newBlock->next_free->previous = NULL;
    newBlock->next_free->size = (long)usableBlockSize;

    return(1);
}

/******************************************************
 * RequestChunk: Allocates memory by returning a chunk
 *   of memory from a larger block of memory.
 *******************************************************/
void *RequestChunk(void *env, size_t requestSize)
{
    struct core_mem_chunk *chunkPtr;
    struct core_mem_block *blockPtr;

    /*==================================================
     * Allocate initial memory pool block if it has not
     * already been allocated.
     *==================================================*/

    if( core_mem_get_memory_data(env)->BlockMemoryInitialized == FALSE )
    {
        if( InitializeBlockMemory(env, requestSize) == 0 )
        {
            return(NULL);
        }
    }

    /*====================================================
     * Make sure that the amount of memory requested will
     * fall on a boundary of strictest alignment
     *====================================================*/

    requestSize = (((requestSize - 1) / STRICT_ALIGN_SIZE) + 1) * STRICT_ALIGN_SIZE;

    /*=====================================================
     * Search through the list of free memory for a block
     * of the appropriate size.  If a block is found, then
     * allocate and return a pointer to it.
     *=====================================================*/

    blockPtr = core_mem_get_memory_data(env)->TopMemoryBlock;

    while( blockPtr != NULL )
    {
        chunkPtr = blockPtr->next_free;

        while( chunkPtr != NULL )
        {
            if((chunkPtr->size == requestSize) ||
               (chunkPtr->size > (requestSize + core_mem_get_memory_data(env)->ChunkInfoSize)))
            {
                AllocateChunk(env, blockPtr, chunkPtr, requestSize);

                return((void *)(((char *)chunkPtr) + core_mem_get_memory_data(env)->ChunkInfoSize));
            }

            chunkPtr = chunkPtr->next_free;
        }

        if( blockPtr->next == NULL )
        {
            if( AllocateBlock(env, blockPtr, requestSize) == 0 )  /* get another block */
            {
                return(NULL);
            }
        }

        blockPtr = blockPtr->next;
    }

    error_system(env, "MEMORY", 2);
    exit_router(env, EXIT_FAILURE);
    return(NULL); /* Unreachable, but prevents warning. */
}

/*******************************************
 * AllocateChunk: Allocates a chunk from an
 *   existing chunk in a block of memory.
 ********************************************/
static void AllocateChunk(void *env, struct core_mem_block *parentBlock, struct core_mem_chunk *chunkPtr, size_t requestSize)
{
    struct core_mem_chunk *splitChunk, *nextChunk;

    /*=============================================================
     * If the size of the memory chunk is an exact match for the
     * requested amount of memory, then the chunk can be allocated
     * without splitting it.
     *=============================================================*/

    if( requestSize == chunkPtr->size )
    {
        chunkPtr->size = -(long int)requestSize;

        if( chunkPtr->last_free == NULL )
        {
            if( chunkPtr->next_free != NULL )
            {
                parentBlock->next_free = chunkPtr->next_free;
            }
            else
            {
                parentBlock->next_free = NULL;
            }
        }
        else
        {
            chunkPtr->last_free->next_free = chunkPtr->next_free;
        }

        if( chunkPtr->next_free != NULL )
        {
            chunkPtr->next_free->last_free = chunkPtr->last_free;
        }

        chunkPtr->last_free = NULL;
        chunkPtr->next_free = NULL;
        return;
    }

    /*===========================================================
     * If the size of the memory chunk is larger than the memory
     * request, then split the chunk into two pieces.
     *===========================================================*/

    nextChunk = (struct core_mem_chunk *)
                (((char *)chunkPtr) + core_mem_get_memory_data(env)->ChunkInfoSize + chunkPtr->size);

    splitChunk = (struct core_mem_chunk *)
                 (((char *)chunkPtr) + (core_mem_get_memory_data(env)->ChunkInfoSize + requestSize));

    splitChunk->size = (long)(chunkPtr->size - (requestSize + core_mem_get_memory_data(env)->ChunkInfoSize));
    splitChunk->previous = chunkPtr;

    splitChunk->next_free = chunkPtr->next_free;
    splitChunk->last_free = chunkPtr->last_free;

    nextChunk->previous = splitChunk;

    if( splitChunk->last_free == NULL )
    {
        parentBlock->next_free = splitChunk;
    }
    else
    {
        splitChunk->last_free->next_free = splitChunk;
    }

    if( splitChunk->next_free != NULL )
    {
        splitChunk->next_free->last_free = splitChunk;
    }

    chunkPtr->size = -(long int)requestSize;
    chunkPtr->last_free = NULL;
    chunkPtr->next_free = NULL;

    return;
}

/**********************************************************
 * ReturnChunk: Frees memory allocated using RequestChunk.
 ***********************************************************/
int ReturnChunk(void *env, void *memPtr, size_t size)
{
    struct core_mem_chunk *chunkPtr, *lastChunk, *nextChunk, *topChunk;
    struct core_mem_block *blockPtr;

    /*=====================================================
     * Determine if the expected size of the chunk matches
     * the size stored in the chunk's information record.
     *=====================================================*/

    size = (((size - 1) / STRICT_ALIGN_SIZE) + 1) * STRICT_ALIGN_SIZE;

    chunkPtr = (struct core_mem_chunk *)(((char *)memPtr) - core_mem_get_memory_data(env)->ChunkInfoSize);

    if( chunkPtr == NULL )
    {
        return(FALSE);
    }

    if( chunkPtr->size >= 0 )
    {
        return(FALSE);
    }

    if( chunkPtr->size != -(long int)size )
    {
        return(FALSE);
    }

    chunkPtr->size = -chunkPtr->size;

    /*=============================================
     * Determine in which block the chunk resides.
     *=============================================*/

    topChunk = chunkPtr;

    while( topChunk->previous != NULL )
    {
        topChunk = topChunk->previous;
    }

    blockPtr = (struct core_mem_block *)(((char *)topChunk) - core_mem_get_memory_data(env)->BlockInfoSize);

    /*===========================================
     * Determine the chunks physically preceding
     * and following the returned chunk.
     *===========================================*/

    lastChunk = chunkPtr->previous;
    nextChunk = (struct core_mem_chunk *)(((char *)memPtr) + size);

    /*=========================================================
     * Add the chunk to the list of free chunks for the block.
     *=========================================================*/

    if( blockPtr->next_free != NULL )
    {
        blockPtr->next_free->last_free = chunkPtr;
    }

    chunkPtr->next_free = blockPtr->next_free;
    chunkPtr->last_free = NULL;

    blockPtr->next_free = chunkPtr;

    /*=====================================================
     * Combine this chunk with previous chunk if possible.
     *=====================================================*/

    if( lastChunk != NULL )
    {
        if( lastChunk->size > 0 )
        {
            lastChunk->size += (core_mem_get_memory_data(env)->ChunkInfoSize + chunkPtr->size);

            if( nextChunk != NULL )
            {
                nextChunk->previous = lastChunk;
            }
            else
            {
                return(FALSE);
            }

            if( lastChunk->last_free != NULL )
            {
                lastChunk->last_free->next_free = lastChunk->next_free;
            }

            if( lastChunk->next_free != NULL )
            {
                lastChunk->next_free->last_free = lastChunk->last_free;
            }

            lastChunk->next_free = chunkPtr->next_free;

            if( chunkPtr->next_free != NULL )
            {
                chunkPtr->next_free->last_free = lastChunk;
            }

            lastChunk->last_free = NULL;

            blockPtr->next_free = lastChunk;
            chunkPtr->last_free = NULL;
            chunkPtr->next_free = NULL;
            chunkPtr = lastChunk;
        }
    }

    /*=====================================================
     * Combine this chunk with the next chunk if possible.
     *=====================================================*/

    if( nextChunk == NULL )
    {
        return(FALSE);
    }

    if( chunkPtr == NULL )
    {
        return(FALSE);
    }

    if( nextChunk->size > 0 )
    {
        chunkPtr->size += (core_mem_get_memory_data(env)->ChunkInfoSize + nextChunk->size);

        topChunk = (struct core_mem_chunk *)(((char *)nextChunk) + nextChunk->size + core_mem_get_memory_data(env)->ChunkInfoSize);

        if( topChunk != NULL )
        {
            topChunk->previous = chunkPtr;
        }
        else
        {
            return(FALSE);
        }

        if( nextChunk->last_free != NULL )
        {
            nextChunk->last_free->next_free = nextChunk->next_free;
        }

        if( nextChunk->next_free != NULL )
        {
            nextChunk->next_free->last_free = nextChunk->last_free;
        }
    }

    /*===========================================
     * Free the buffer if we can, but don't free
     * the first buffer if it's the only one.
     *===========================================*/

    if((chunkPtr->previous == NULL) &&
       (chunkPtr->size == blockPtr->size))
    {
        if( blockPtr->previous != NULL )
        {
            blockPtr->previous->next = blockPtr->next;

            if( blockPtr->next != NULL )
            {
                blockPtr->next->previous = blockPtr->previous;
            }

            free((char *)blockPtr);
        }
        else
        {
            if( blockPtr->next != NULL )
            {
                blockPtr->next->previous = NULL;
                core_mem_get_memory_data(env)->TopMemoryBlock = blockPtr->next;
                free((char *)blockPtr);
            }
        }
    }

    return(TRUE);
}

/**********************************************
 * ReturnAllBlocks: Frees all allocated blocks
 *   back to the operating system.
 ***********************************************/
void ReturnAllBlocks(void *env)
{
    struct core_mem_block *theBlock, *next;

    /*======================================
     * Free up int based memory allocation.
     *======================================*/

    theBlock = core_mem_get_memory_data(env)->TopMemoryBlock;

    while( theBlock != NULL )
    {
        next = theBlock->next;
        free((char *)theBlock);
        theBlock = next;
    }

    core_mem_get_memory_data(env)->TopMemoryBlock = NULL;
}

#endif
