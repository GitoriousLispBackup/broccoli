/* Purpose: Manages the atomic data value hash tables for
 *   storing symbols, integers, floats, and bit maps.
 *   Contains routines for adding entries, examining the
 *   hash tables, and performing garbage collection to
 *   remove entries no longer in use.                        */

#ifndef __TYPE_ATOM_H__
#define __TYPE_ATOM_H__

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef __TYPE_ATOM_SOURCE__
#define LOCALE
#else
#define LOCALE extern
#endif

#include <stdlib.h>

#ifndef ATOM_HASH_SZ
#define ATOM_HASH_SZ       63559L
#endif

#ifndef FLOAT_HASH_SZ
#define FLOAT_HASH_SZ         8191
#endif

#ifndef INTEGER_HASH_SZ
#define INTEGER_HASH_SZ       8191
#endif

#ifndef BITMAP_HASH_SZ
#define BITMAP_HASH_SZ        8191
#endif

#ifndef EXTERNAL_ADDRESS_HASH_SZ
#define EXTERNAL_ADDRESS_HASH_SZ        128
#endif

/***********************************************************
 * atom_hash_node STRUCTURE:
 ************************************************************/
struct atom_hash_node
{
    struct atom_hash_node *next;
    long                   count;
    int                    depth;
    unsigned int           is_permanent :
    1;
    unsigned int is_ephemeral :
    1;
    unsigned int is_needed :
    1;
    unsigned int bucket :
    29;
    char *contents;
};

/***********************************************************
 * float_hash_node STRUCTURE:
 ************************************************************/
struct float_hash_node
{
    struct float_hash_node *next;
    long                    count;
    int                     depth;
    unsigned int            is_permanent :
    1;
    unsigned int is_ephemeral :
    1;
    unsigned int is_needed :
    1;
    unsigned int bucket :
    29;
    double contents;
};

/***********************************************************
 * integer_hash_node STRUCTURE:
 ************************************************************/
struct integer_hash_node
{
    struct integer_hash_node *next;
    long                      count;
    int                       depth;
    unsigned int              is_permanent :
    1;
    unsigned int is_ephemeral :
    1;
    unsigned int is_needed :
    1;
    unsigned int bucket :
    29;
    long long contents;
};

/***********************************************************
 * bitmap_hash_node STRUCTURE:
 ************************************************************/
struct bitmap_hash_node
{
    struct bitmap_hash_node *next;
    long                     count;
    int                      depth;
    unsigned int             is_permanent :
    1;
    unsigned int is_ephemeral :
    1;
    unsigned int is_needed :
    1;
    unsigned int bucket :
    29;
    char *         contents;
    unsigned short size;
};

/***********************************************************
 * external_address_hash_node STRUCTURE:
 ************************************************************/
struct external_address_hash_node
{
    struct external_address_hash_node *next;
    long                               count;
    int                                depth;
    unsigned int                       is_permanent :
    1;
    unsigned int is_ephemeral :
    1;
    unsigned int is_needed :
    1;
    unsigned int bucket :
    29;
    void *         address;
    unsigned short type;
};

/***********************************************************
 * generic_hash_node STRUCTURE:
 ************************************************************/
struct generic_hash_node
{
    struct generic_hash_node *next;
    long                      count;
    int                       depth;
    unsigned int              is_permanent :
    1;
    unsigned int is_ephemeral :
    1;
    unsigned int is_needed :
    1;
    unsigned int bucket :
    29;
};

typedef struct atom_hash_node             ATOM_HN;
typedef struct float_hash_node            FLOAT_HN;
typedef struct integer_hash_node          INTEGER_HN;
typedef struct bitmap_hash_node           BITMAP_HN;
typedef struct external_address_hash_node EXTERNAL_ADDRESS_HN;
typedef struct generic_hash_node          GENERIC_HN;

/*********************************************************
 * EPHEMERON STRUCTURE: Data structure used to keep track
 *   of is_ephemeral symbols, floats, and integers.
 *
 *   its_value: Contains a pointer to the storage
 *   structure for the symbol, float, or integer which is
 *   is_ephemeral.
 *
 *   next: Contains a pointer to the next is_ephemeral item
 *   in a list of is_ephemeral items.
 **********************************************************/
struct ephemeron
{
    GENERIC_HN *      its_value;
    struct ephemeron *next;
};

/***********************************************************
 * atom_match STRUCTURE:
 ************************************************************/
struct atom_match
{
    struct atom_hash_node *match;
    struct atom_match     *next;
};

#define to_string(target)           (((struct atom_hash_node *)(target))->contents)
#define to_double(target)           (((struct float_hash_node *)(target))->contents)
#define to_long(target)             (((struct integer_hash_node *)(target))->contents)
#define to_int(target)              ((int)(((struct integer_hash_node *)(target))->contents))
#define to_bitmap(target)           ((void *)((struct bitmap_hash_node *)(target))->contents)
#define to_external_address(target) ((void *)((struct external_address_hash_node *)(target))->address)

#define inc_atom_count(val)             (((ATOM_HN *)val)->count++)
#define inc_float_count(val)            (((FLOAT_HN *)val)->count++)
#define inc_integer_count(val)          (((INTEGER_HN *)val)->count++)
#define inc_bitmap_count(val)           (((BITMAP_HN *)val)->count++)
#define inc_external_address_count(val) (((EXTERNAL_ADDRESS_HN *)val)->count++)

/*==================
 * ENVIRONMENT DATA
 *==================*/

#define ATOM_DATA_INDEX 49

struct atom_data
{
    void *                true_atom;
    void *                false_atom;
    void *                positive_inf_atom;
    void *                negative_inf_atom;
    void *                zero_atom;
    ATOM_HN **            symbol_table;
    FLOAT_HN **           float_table;
    INTEGER_HN **         integer_table;
    BITMAP_HN **          bitmap_table;
    EXTERNAL_ADDRESS_HN **external_address_table;
    struct ephemeron *    atom_ephemerons;
    struct ephemeron *    float_ephemerons;
    struct ephemeron *    integer_ephemerons;
    struct ephemeron *    bitmap_ephemerons;
    struct ephemeron *    external_address_ephemerons;
};

#define get_atom_data(env)     ((struct atom_data *)core_get_environment_data(env, ATOM_DATA_INDEX))

#define get_false(env) get_atom_data(env)->false_atom
#define get_true(env)  get_atom_data(env)->true_atom

LOCALE void                               init_symbol_tables(void *, struct atom_hash_node **, struct float_hash_node **, struct integer_hash_node **, struct bitmap_hash_node **, struct external_address_hash_node **);
LOCALE void                          *    store_atom(void *, char *);
LOCALE ATOM_HN                     *      lookup_atom(void *, char *);
LOCALE void                          *    store_double(void *, double);
LOCALE void                          *    store_long(void *, long long);
LOCALE void                          *    store_bitmap(void *, void *, unsigned);
LOCALE void                          *    store_external_address(void *, void *, unsigned);
LOCALE INTEGER_HN                    *    lookup_long(void *, long long);
LOCALE unsigned long                      hash_atom(char *, unsigned long);
LOCALE unsigned long                      hash_float(double, unsigned long);
LOCALE unsigned long                      hash_integer(long long, unsigned long);
LOCALE unsigned long                      hash_bitmap(char *, unsigned long, unsigned);
LOCALE unsigned long                      hash_external_address(void *, unsigned long);
LOCALE void                               dec_atom_count(void *, struct atom_hash_node *);
LOCALE void                               dec_float_count(void *, struct float_hash_node *);
LOCALE void                               dec_integer_count(void *, struct integer_hash_node *);
LOCALE void                               dec_bitmap_count(void *, struct bitmap_hash_node *);
LOCALE void                               dec_external_address_count(void *, struct external_address_hash_node *);
LOCALE void                               remove_ephemeral_atoms(void *);
LOCALE struct atom_hash_node        **    get_symbol_table(void *);
LOCALE void                               set_symbol_table(void *, struct atom_hash_node **);
LOCALE struct float_hash_node          ** get_float_table(void *);
LOCALE void                               set_float_table(void *, struct float_hash_node **);
LOCALE struct integer_hash_node       **  get_integer_table(void *);
LOCALE void                               set_integer_table(void *, struct integer_hash_node **);
LOCALE struct bitmap_hash_node        **  get_bitmap_table(void *);
LOCALE void                               set_bitmap_table(void *, struct bitmap_hash_node **);
LOCALE struct external_address_hash_node
**                                    get_external_address_table(void *);
LOCALE void                           set_external_address_table(void *, struct external_address_hash_node **);
LOCALE void                           reset_special_atoms(void *);
LOCALE struct atom_match            * find_atom_matches(void *, char *, unsigned *, size_t *);
LOCALE void                           return_atom_matches(void *, struct atom_match *);
LOCALE ATOM_HN                     *next_atom_match(void *, char *, size_t, ATOM_HN *, int, size_t *);
LOCALE void init_bitmap(void *, unsigned);

#endif
