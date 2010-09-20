/* Purpose: Memory allocation routines.                      */

#ifndef __CORE_MEMORY_H__

#include <string.h>

#define __CORE_MEMORY_H__

struct core_mem_chunk;
struct core_mem_block;
struct core_mem_ptr;

#define MEM_TABLE_SIZE 500

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef _MEMORY_SOURCE_
#define LOCALE
#else
#define LOCALE extern
#endif

struct core_mem_chunk
{
    struct core_mem_chunk *previous;
    struct core_mem_chunk *next_free;
    struct core_mem_chunk *last_free;
    long int               size;
};

struct core_mem_block
{
    struct core_mem_block *next;
    struct core_mem_block *previous;
    struct core_mem_chunk *next_free;
    long int               size;
};

struct core_mem_ptr
{
    struct core_mem_ptr *next;
};

#define core_mem_get_struct(env, type) \
    ((core_mem_get_memory_data(env)->memory_table[sizeof(struct type)] == NULL) ? \
     ((struct type *)core_mem_alloc(env, sizeof(struct type))) : \
     ((core_mem_get_memory_data(env)->temp_memory = core_mem_get_memory_data(env)->memory_table[sizeof(struct type)]), \
                                                    core_mem_get_memory_data(env)->memory_table[sizeof(struct type)] = core_mem_get_memory_data(env)->temp_memory->next, \
                                                    ((struct type *)core_mem_get_memory_data(env)->temp_memory)))

#define core_mem_return_struct(env, type, struct_ptr) \
    (core_mem_get_memory_data(env)->temp_memory = (struct core_mem_ptr *)struct_ptr, \
     core_mem_get_memory_data(env)->temp_memory->next = core_mem_get_memory_data(env)->memory_table[sizeof(struct type)], \
     core_mem_get_memory_data(env)->memory_table[sizeof(struct type)] = core_mem_get_memory_data(env)->temp_memory)

#define core_mem_return_fixed_struct(env, size, struct_ptr) \
    (core_mem_get_memory_data(env)->temp_memory = (struct core_mem_ptr *)struct_ptr, \
     core_mem_get_memory_data(env)->temp_memory->next = core_mem_get_memory_data(env)->memory_table[size], \
     core_mem_get_memory_data(env)->memory_table[size] = core_mem_get_memory_data(env)->temp_memory)

#define core_mem_get_dynamic_struct(env, type, vsize) \
    ((((sizeof(struct type) + vsize) <  MEM_TABLE_SIZE) ? \
      (core_mem_get_memory_data(env)->memory_table[sizeof(struct type) + vsize] == NULL) : 1) ? \
     ((struct type *)core_mem_alloc(env, (sizeof(struct type) + vsize))) : \
     ((core_mem_get_memory_data(env)->temp_memory = core_mem_get_memory_data(env)->memory_table[sizeof(struct type) + vsize]), \
                                                    core_mem_get_memory_data(env)->memory_table[sizeof(struct type) + vsize] = core_mem_get_memory_data(env)->temp_memory->next, \
                                                    ((struct type *)core_mem_get_memory_data(env)->temp_memory)))

#define core_mem_release_dynamic_struct(env, type, vsize, struct_ptr) \
    (core_mem_get_memory_data(env)->temp_sz = sizeof(struct type) + vsize, \
     ((core_mem_get_memory_data(env)->temp_sz < MEM_TABLE_SIZE) ? \
      (core_mem_get_memory_data(env)->temp_memory = (struct core_mem_ptr *)struct_ptr, \
       core_mem_get_memory_data(env)->temp_memory->next = core_mem_get_memory_data(env)->memory_table[core_mem_get_memory_data(env)->temp_sz], \
       core_mem_get_memory_data(env)->memory_table[core_mem_get_memory_data(env)->temp_sz] =  core_mem_get_memory_data(env)->temp_memory) : \
                                                                                             (core_mem_free(env, (void *)struct_ptr, core_mem_get_memory_data(env)->temp_sz), (struct core_mem_ptr *)struct_ptr)))

#define core_mem_get_memory(env, size) \
    (((size <  MEM_TABLE_SIZE) ? \
      (core_mem_get_memory_data(env)->memory_table[size] == NULL) : 1) ? \
     ((struct type *)core_mem_alloc(env, (size_t)(size))) : \
     ((core_mem_get_memory_data(env)->temp_memory = core_mem_get_memory_data(env)->memory_table[size]), \
                                                    core_mem_get_memory_data(env)->memory_table[size] = core_mem_get_memory_data(env)->temp_memory->next, \
                                                    ((struct type *)core_mem_get_memory_data(env)->temp_memory)))

#define core_mem_return_memory(env, size, ptr) \
    (core_mem_get_memory_data(env)->temp_sz = size, \
     ((core_mem_get_memory_data(env)->temp_sz < MEM_TABLE_SIZE) ? \
      (core_mem_get_memory_data(env)->temp_memory = (struct core_mem_ptr *)ptr, \
       core_mem_get_memory_data(env)->temp_memory->next = core_mem_get_memory_data(env)->memory_table[core_mem_get_memory_data(env)->temp_sz], \
       core_mem_get_memory_data(env)->memory_table[core_mem_get_memory_data(env)->temp_sz] =  core_mem_get_memory_data(env)->temp_memory) : \
                                                                                             (core_mem_free(env, (void *)ptr, core_mem_get_memory_data(env)->temp_sz), (struct core_mem_ptr *)ptr)))

#define core_mem_copy_memory(type, cnt, dst, src) memcpy((void *)(dst), (void *)(src), sizeof(type) * (size_t)(cnt))

#define MEMORY_DATA_INDEX 59

struct core_mem_memory
{
    long int amount;
    long int calls;
    BOOLEAN  conservation;
    int      (*fn_out_of_memory)(void *, size_t);
#if BLOCK_MEMORY
    struct core_mem_block *TopMemoryBlock;
    int                    BlockInfoSize;
    int                    ChunkInfoSize;
    int                    BlockMemoryInitialized;
#endif
    struct core_mem_ptr * temp_memory;
    struct core_mem_ptr **memory_table;
    size_t                temp_sz;
};

#define core_mem_get_memory_data(env) ((struct core_mem_memory *)core_get_environment_data(env, MEMORY_DATA_INDEX))

LOCALE void core_mem_init_memory(void *);
LOCALE void *core_mem_alloc(void *, size_t);
LOCALE int core_mem_fn_out_of_memory(void *, size_t);
LOCALE int core_mem_free(void *, void *, size_t);
LOCALE void *core_mem_realloc(void *, void *, size_t, size_t);
LOCALE long core_mem_release_count(void *, long, int);
LOCALE void *core_mem_alloc_and_init(void *, size_t);
LOCALE void *core_mem_alloc_no_init(void *, size_t);
LOCALE void *core_mem_alloc_large_no_init(void *, size_t);
LOCALE int core_mem_release(void *, void *, size_t);
LOCALE int core_mem_release_sized(void *, void *, size_t);
LOCALE unsigned long core_mem_get_pool_sz(void *);
LOCALE void          core_mem_memcopy(char *, char *, unsigned long);

#endif
