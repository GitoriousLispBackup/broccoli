/* Purpose: Routines for attaching user data to constructs,
 *   facts, instances, user functions, etc.                  */

#ifndef __EXTENSIONS_DATA_H__
#define __EXTENSIONS_DATA_H__

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef __EXTENSIONS_DATA_SOURCE__
#define LOCALE
#else
#define LOCALE extern
#endif

struct ext_data
{
    unsigned char    id;
    struct ext_data *next;
};

typedef struct ext_data   EXT_DATA;
typedef struct ext_data * EXT_DATA_PTR;

struct ext_data_record
{
    unsigned char id;
    void *        (*fn_create)(void *);
    void          (*fn_delete)(void *, void *);
};

typedef struct ext_data_record   EXT_DATA_RECORD;
typedef struct ext_data_record * EXT_DATA_RECORD_PTR;

#define MAXIMUM_USER_DATA_RECORDS 20

#define USER_DATA_DATA_INDEX 56

struct ext_data_datum
{
    struct ext_data_record *records[MAXIMUM_USER_DATA_RECORDS];
    unsigned char           count;
};

#define ext_get_data(env) ((struct ext_data_datum *)core_get_environment_data(env, USER_DATA_DATA_INDEX))

LOCALE void                            ext_init_data(void *);
LOCALE unsigned char                   ext_install_record(void *, struct ext_data_record *);
LOCALE struct ext_data               * ext_get_data_item(void *, unsigned char, struct ext_data **);
LOCALE struct ext_data               * ext_validate_data(unsigned char, struct ext_data *);
LOCALE void                            ext_clear_data(void *, struct ext_data *);
LOCALE struct ext_data               * ext_delete_data_item(void *, unsigned char, struct ext_data *);

#endif
