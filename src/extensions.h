#include "core_evaluation.h"

struct ext_class_info
{
    struct core_data_entity base;
    void                    (*decrementBasisCount)(void *, void *);
    void                    (*incrementBasisCount)(void *, void *);
    void                    (*matchFunction)(void *, void *);
    BOOLEAN                 (*synchronized)(void *, void *);
};

typedef struct ext_class_info   PTRN_ENTITY_RECORD;
typedef struct ext_class_info * PTRN_ENTITY_RECORD_PTR;

struct patternEntity
{
    struct ext_class_info *theInfo;
    void *                 dependents;
    unsigned               busyCount;
    unsigned long long     timeTag;
};

typedef struct patternEntity   PATTERN_ENTITY;
typedef struct patternEntity * PATTERN_ENTITY_PTR;
