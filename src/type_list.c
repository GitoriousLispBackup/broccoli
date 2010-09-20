#define __TYPE_LIST_SOURCE__

#include <stdio.h>
#define _STDIO_INCLUDED_

#include "setup.h"

#include "constant.h"
#include "core_memory.h"
#include "core_environment.h"
#include "core_evaluation.h"
#include "core_scanner.h"
#include "router.h"
#include "router_string.h"
#include "core_gc.h"
#if OBJECT_SYSTEM
#include "classes.h"
#endif

#include "type_list.h"

/**************************************
 * LOCAL FUNCTION PROTOTYPES
 ***************************************/

static void DeallocateListData(void *);

/**************************************************
 * init_list_data: Allocates environment
 *    data for list values.
 ***************************************************/
void init_list_data(void *env)
{
    core_allocate_environment_data(env, LIST_DATA_INDEX, sizeof(struct list_data), DeallocateListData);
}

/****************************************************
 * DeallocateListData: Deallocates environment
 *    data for list values.
 *****************************************************/
static void DeallocateListData(void *env)
{
    struct list *tmpPtr, *nextPtr;

    tmpPtr = get_list_data(env)->all_lists;

    while( tmpPtr != NULL )
    {
        nextPtr = tmpPtr->next;
        release_list(env, tmpPtr);
        tmpPtr = nextPtr;
    }
}

/**********************************************************
 * create_sized_list:
 ***********************************************************/
void *create_sized_list(void *env, long size)
{
    struct list *list_segment;
    long newSize = size;

    if( size <= 0 )
    {
        newSize = 1;
    }

    list_segment = core_mem_get_dynamic_struct(env, list, (long)sizeof(struct node) * (newSize - 1L));

    list_segment->length = size;
    list_segment->depth = (short)core_get_evaluation_data(env)->eval_depth;
    list_segment->busy_count = 0;
    list_segment->next = NULL;

    return((void *)list_segment);
}

/****************************************************************
 * release_list:
 *****************************************************************/
void release_list(void *env, struct list *list_segment)
{
    unsigned long newSize;

    if( list_segment == NULL )
    {
        return;
    }

    if( list_segment->length == 0 )
    {
        newSize = 1;
    }
    else
    {
        newSize = list_segment->length;
    }

    core_mem_release_dynamic_struct(env, list, sizeof(struct node) * (newSize - 1), list_segment);
}

/*****************************
 * install_list:
 ******************************/
void install_list(void *env, struct list *list_segment)
{
    unsigned long length, i;
    struct node *theFields;

    if( list_segment == NULL )
    {
        return;
    }

    length = list_segment->length;

    list_segment->busy_count++;
    theFields = list_segment->cell;

    for( i = 0 ; i < length ; i++ )
    {
        core_install_data(env, theFields[i].type, theFields[i].value);
    }
}

/*****************************
 * uninstall_list:
 ******************************/
void uninstall_list(void *env, struct list *list_segment)
{
    unsigned long length, i;
    struct node *theFields;

    if( list_segment == NULL )
    {
        return;
    }

    length = list_segment->length;
    list_segment->busy_count--;
    theFields = list_segment->cell;

    for( i = 0 ; i < length ; i++ )
    {
        core_decrement_atom(env, theFields[i].type, theFields[i].value);
    }
}

/******************************************************
 * convert_str_to_list:  Returns a list structure
 *    that represents the string sent as the argument.
 *******************************************************/
struct list *convert_str_to_list(void *env, char *theString)
{
#if OFF
    struct token theToken;
    struct list *list_segment;
    struct node *theFields;
    unsigned long numberOfFields = 0;
    struct core_expression *topAtom = NULL, *lastAtom = NULL, *theAtom;

    /*====================================================
     * Open the string as an input source and read in the
     * list of values to be stored in the list.
     *====================================================*/

    open_string_source(env, "list-str", theString, 0);

    core_get_token(env, "list-str", &theToken);

    while( theToken.type != STOP )
    {
        if((theToken.type == ATOM) || (theToken.type == STRING) ||
           (theToken.type == FLOAT) || (theToken.type == INTEGER) ||
           (theToken.type == INSTANCE_NAME))
        {
            theAtom = core_generate_constant(env, theToken.type, theToken.value);
        }
        else
        {
            theAtom = core_generate_constant(env, STRING, store_atom(env, theToken.pp));
        }

        numberOfFields++;

        if( topAtom == NULL )
        {
            topAtom = theAtom;
        }
        else
        {
            lastAtom->next_arg = theAtom;
        }

        lastAtom = theAtom;
        core_get_token(env, "list-str", &theToken);
    }

    close_string_source(env, "list-str");

    /*====================================================================
     * Create a list of the appropriate size for the values parsed.
     *====================================================================*/

    list_segment = (struct list *)create_list(env, numberOfFields);
    theFields = list_segment->cell;

    /*====================================
     * Copy the values to the list.
     *====================================*/

    theAtom = topAtom;
    numberOfFields = 0;

    while( theAtom != NULL )
    {
        theFields[numberOfFields].type = theAtom->type;
        theFields[numberOfFields].value = theAtom->value;
        numberOfFields++;
        theAtom = theAtom->next_arg;
    }

    /*===========================
     * Return the parsed values.
     *===========================*/

    core_return_expression(env, topAtom);

    /*============================
     * Return the new list.
     *============================*/
    return(list_segment);

#endif
    return(NULL);
}

/*************************************************************
 * create_list: Creates a list of the specified
 *   size and adds it to the list of segments.
 **************************************************************/
void *create_list(void *env, long size)
{
    struct list *list_segment;
    long newSize;

    if( size <= 0 )
    {
        newSize = 1;
    }
    else
    {
        newSize = size;
    }

    list_segment = core_mem_get_dynamic_struct(env, list, (long)sizeof(struct node) * (newSize - 1L));

    list_segment->length = size;
    list_segment->depth = (short)core_get_evaluation_data(env)->eval_depth;
    list_segment->busy_count = 0;
    list_segment->next = NULL;

    list_segment->next = get_list_data(env)->all_lists;
    get_list_data(env)->all_lists = list_segment;

    core_get_gc_data(env)->generational_item_count++;
    core_get_gc_data(env)->generational_item_sz += sizeof(struct list) + (sizeof(struct node) * newSize);

    return((void *)list_segment);
}

/********************************************************************
 * convert_data_object_to_list:
 *********************************************************************/
void *convert_data_object_to_list(void *env, core_data_object *val)
{
    struct list *dst, *src;

    if( val->type != LIST )
    {
        return(NULL);
    }

    dst = (struct list *)create_sized_list(env, (unsigned long)core_get_data_ptr_length(val));

    src = (struct list *)val->value;
    core_mem_copy_memory(struct node, dst->length, &(dst->cell[0]), &(src->cell[core_get_data_ptr_start(val) - 1]));

    return((void *)dst);
}

/**********************************************************
 * track_list:
 ***********************************************************/
void track_list(void *env, struct list *list_segment)
{
    list_segment->depth = (short)core_get_evaluation_data(env)->eval_depth;
    list_segment->next = get_list_data(env)->all_lists;
    get_list_data(env)->all_lists = list_segment;

    core_get_gc_data(env)->generational_item_count++;
    core_get_gc_data(env)->generational_item_sz += sizeof(struct list) + (sizeof(struct node) * list_segment->length);
}

/**********************************************************
 * flush_lists:
 ***********************************************************/
void flush_lists(void *env)
{
    struct list *list_segment, *nextPtr, *lastPtr = NULL;
    unsigned long newSize;

    list_segment = get_list_data(env)->all_lists;

    while( list_segment != NULL )
    {
        nextPtr = list_segment->next;

        if((list_segment->depth > core_get_evaluation_data(env)->eval_depth) && (list_segment->busy_count == 0))
        {
            core_get_gc_data(env)->generational_item_count--;
            core_get_gc_data(env)->generational_item_sz -= sizeof(struct list) +
                                                           (sizeof(struct node) * list_segment->length);

            if( list_segment->length == 0 )
            {
                newSize = 1;
            }
            else
            {
                newSize = list_segment->length;
            }

            core_mem_release_dynamic_struct(env, list, sizeof(struct node) * (newSize - 1), list_segment);

            if( lastPtr == NULL )
            {
                get_list_data(env)->all_lists = nextPtr;
            }
            else
            {
                lastPtr->next = nextPtr;
            }
        }
        else
        {
            lastPtr = list_segment;
        }

        list_segment = nextPtr;
    }
}

/********************************************************************
 * clone_list: Allocates a new segment and copies results from
 *                  old value to new - NOT put on all_lists!!
 *********************************************************************/
void clone_list(void *env, core_data_object_ptr dst, core_data_object_ptr src)
{
    dst->type = LIST;
    dst->begin = 0;
    dst->end = src->end - src->begin;
    dst->value = (void *)create_sized_list(env, (unsigned long)dst->end + 1);
    core_mem_copy_memory(struct node, dst->end + 1, &((struct list *)dst->value)->cell[0],
                         &((struct list *)src->value)->cell[src->begin]);
}

/********************************************************************
 * copy_list:
 *********************************************************************/
void *copy_list(void *env, struct list *src)
{
    struct list *dst;

    dst = (struct list *)create_sized_list(env, src->length);
    core_mem_copy_memory(struct node, src->length, &(dst->cell[0]), &(src->cell[0]));
    return((void *)dst);
}

/*********************************************************
 * print_list: Prints out a list
 **********************************************************/
void print_list(void *env, char *fileid, struct list *segment, long begin, long end, int printParens)
{
    struct node *list;
    int i;

    list = segment->cell;

    if( printParens )
    {
        print_router(env, fileid, "(");
    }

    i = begin;

    while( i <= end )
    {
        if( list[i].type == LIST )
        {
            struct list*pass = (struct list *)list[i].value;
            print_list(env, fileid, pass, 0, pass->length - 1, 1);
        }
        else
        {
            core_print_atom(env, fileid, list[i].type, list[i].value);
        }

        i++;

        if( i <= end )
        {
            print_router(env, fileid, " ");
        }
    }

    if( printParens )
    {
        print_router(env, fileid, ")");
    }
}

/****************************************************
 * add_to_list:  Append function for segments.
 *****************************************************/
void add_to_list(void *env, core_data_object *ret, core_expression_object *expptr, int garbageSegment)
{
    core_data_object val_ptr;
    core_data_object *val_arr;
    struct list *list;
    struct list *orig_ptr;
    long start, end, i, j, k, argCount;
    unsigned long seg_size;

    argCount = core_count_args(expptr);

    /*=========================================
     * If no arguments are given return a NULL
     * list of length zero.
     *=========================================*/

    if( argCount == 0 )
    {
        core_set_pointer_type(ret, LIST);
        core_set_data_ptr_start(ret, 1);
        core_set_data_ptr_end(ret, 0);

        if( garbageSegment )
        {
            list = (struct list *)create_list(env, 0L);
        }
        else
        {
            list = (struct list *)create_sized_list(env, 0L);
        }

        core_set_pointer_value(ret, (void *)list);
        return;
    }

    else
    {
        /*========================================
         * Get a new segment with length equal to
         * the total length of all the arguments.
         *========================================*/

        val_arr = (core_data_object *)core_mem_alloc_large_no_init(env, (long)sizeof(core_data_object) * argCount);
        seg_size = 0;

        for( i = 1 ; i <= argCount ; i++, expptr = expptr->next_arg )
        {
            core_eval_expression(env, expptr, &val_ptr);

            if( core_get_evaluation_data(env)->eval_error )
            {
                core_set_pointer_type(ret, LIST);
                core_set_data_ptr_start(ret, 1);
                core_set_data_ptr_end(ret, 0);

                if( garbageSegment )
                {
                    list = (struct list *)create_list(env, 0L);
                }
                else
                {
                    list = (struct list *)create_sized_list(env, 0L);
                }

                core_set_pointer_value(ret, (void *)list);
                core_mem_release_sized(env, val_arr, (long)sizeof(core_data_object) * argCount);
                return;
            }

            core_set_pointer_type(val_arr + i - 1, core_get_type(val_ptr));

            if( core_get_type(val_ptr) == LIST )
            {
                core_set_pointer_value(val_arr + i - 1, core_get_value(val_ptr));
                start = end = -1;
            }
            else if( core_get_type(val_ptr) == RVOID )
            {
                core_set_pointer_value(val_arr + i - 1, core_get_value(val_ptr));
                start = 1;
                end = 0;
            }
            else
            {
                core_set_pointer_value(val_arr + i - 1, core_get_value(val_ptr));
                start = end = -1;
            }

            seg_size += (unsigned long)(end - start + 1);
            core_set_data_ptr_start(val_arr + i - 1, start);
            core_set_data_ptr_end(val_arr + i - 1, end);
        }

        if( garbageSegment )
        {
            list = (struct list *)create_list(env, seg_size);
        }
        else
        {
            list = (struct list *)create_sized_list(env, seg_size);
        }

        /*========================================
         * Copy each argument into new segment.
         *========================================*/

        for( k = 0, j = 1; k < argCount; k++ )
        {
            if( core_get_pointer_type(val_arr + k) == LIST )
            {
                orig_ptr = (struct list *)core_get_pointer_value(val_arr + k);
                set_list_node_type(list, j, LIST);
                set_list_node_value(list, j, orig_ptr);
                j++;

/*
 *            start = core_get_data_ptr_start(val_arr + k);
 *            end = core_get_data_ptr_end(val_arr + k);
 *            orig_ptr = (struct list *)core_get_pointer_value(val_arr + k);
 *
 *            for( i = start; i < end + 1; i++, j++ )
 *            {
 *                set_list_node_type(list, j, (get_list_node_type(orig_ptr, i)));
 *                set_list_node_value(list, j, (get_list_node_value(orig_ptr, i)));
 *            }
 */
            }
            else if( core_get_pointer_type(val_arr + k) != LIST )
            {
                set_list_node_type(list, j, (short)(core_get_pointer_type(val_arr + k)));
                set_list_node_value(list, j, (core_get_pointer_value(val_arr + k)));
                j++;
            }
        }

        /*=========================
         * Return the new segment.
         *=========================*/

        core_set_pointer_type(ret, LIST);
        core_set_data_ptr_start(ret, 1);
        core_set_data_ptr_end(ret, (long)seg_size);
        core_set_pointer_value(ret, (void *)list);
        core_mem_release_sized(env, val_arr, (long)sizeof(core_data_object) * argCount);
        return;
    }
}

/************************************************************
 * are_list_segments_equal: determines if two segments are equal.
 *************************************************************/
BOOLEAN are_list_segments_equal(core_data_object_ptr dobj1, core_data_object_ptr dobj2)
{
    long extent1, extent2; /* 6.04 Bug Fix */
    NODE_PTR e1, e2;

    extent1 = core_get_data_ptr_length(dobj1);
    extent2 = core_get_data_ptr_length(dobj2);

    if( extent1 != extent2 )
    {
        return(FALSE);
    }

    e1 = (NODE_PTR)get_list_ptr(core_get_pointer_value(dobj1), core_get_data_ptr_start(dobj1));
    e2 = (NODE_PTR)get_list_ptr(core_get_pointer_value(dobj2), core_get_data_ptr_start(dobj2));

    while( extent1 != 0 )
    {
        if( e1->type != e2->type )
        {
            return(FALSE);
        }

        if( e1->value != e2->value )
        {
            return(FALSE);
        }

        extent1--;

        if( extent1 > 0 )
        {
            e1++;
            e2++;
        }
    }

    return(TRUE);
}

/*****************************************************************
 * are_lists_equal: Determines if two lists are identical.
 ******************************************************************/
int are_lists_equal(struct list *segment1, struct list *segment2)
{
    struct node *elem1;
    struct node *elem2;
    long length, i = 0;

    length = segment1->length;

    if( length != segment2->length )
    {
        return(FALSE);
    }

    elem1 = segment1->cell;
    elem2 = segment2->cell;

    /*==================================================
     * Compare each field of both facts until the facts
     * match completely or the facts mismatch.
     *==================================================*/

    while( i < length )
    {
        if( elem1[i].type != elem2[i].type )
        {
            return(FALSE);
        }

        if( elem1[i].type == LIST )
        {
            if( are_lists_equal((struct list *)elem1[i].value,
                                (struct list *)elem2[i].value) == FALSE )
            {
                return(FALSE);
            }
        }
        else if( elem1[i].value != elem2[i].value )
        {
            return(FALSE);
        }

        i++;
    }

    return(TRUE);
}

/***********************************************************
 * hash_list: Returns the hash value for a list.
 ************************************************************/
unsigned long hash_list(struct list *list_segment, unsigned long theRange)
{
    unsigned long length, i;
    unsigned long tvalue;
    unsigned long count;
    struct node *fieldPtr;

    union
    {
        double        fv;
        void *        vv;
        unsigned long liv;
    } fis;

    /*================================================
     * Initialize variables for computing hash value.
     *================================================*/

    count = 0;
    length = list_segment->length;
    fieldPtr = list_segment->cell;

    /*====================================================
     * Loop through each value in the list, compute
     * its hash value, and add it to the running total.
     *====================================================*/

    for( i = 0;
         i < length;
         i++ )
    {
        switch( fieldPtr[i].type )
        {
        case LIST:
            count += hash_list((struct list *)fieldPtr[i].value, theRange);
            break;

        case FLOAT:
            fis.fv = to_double(fieldPtr[i].value);
            count += (fis.liv * (i + 29))  +
                     (unsigned long)to_double(fieldPtr[i].value);
            break;

        case INTEGER:
            count += (((unsigned long)to_long(fieldPtr[i].value)) * (i + 29)) +
                     ((unsigned long)to_long(fieldPtr[i].value));
            break;

#if OBJECT_SYSTEM
        case INSTANCE_ADDRESS:
            fis.liv = 0;
            fis.vv = fieldPtr[i].value;
            count += (unsigned long)(fis.liv * (i + 29));
            break;
#endif
        case EXTERNAL_ADDRESS:
            fis.liv = 0;
            fis.vv = to_external_address(fieldPtr[i].value);
            count += (unsigned long)(fis.liv * (i + 29));
            break;

        case ATOM:
        case STRING:
#if OBJECT_SYSTEM
        case INSTANCE_NAME:
#endif
            tvalue = (unsigned long)hash_atom(to_string(fieldPtr[i].value), theRange);
            count += (unsigned long)(tvalue * (i + 29));
            break;
        }
    }

    /*========================
     * Return the hash value.
     *========================*/

    return(count);
}

/*********************
 * get_all_lists:
 **********************/
struct list *get_all_lists(void *env)
{
    return(get_list_data(env)->all_lists);
}

/**************************************
 * implode_list: C access routine
 *   for the implode$ function.
 ***************************************/
void *implode_list(void *env, core_data_object *value)
{
    size_t strsize = 0;
    long i, j;
    char *tmp_str;
    char *ret_str;
    void *rv;
    struct list *list;
    core_data_object tempDO;

    /*===================================================
     * Determine the size of the string to be allocated.
     *===================================================*/

    list = (struct list *)core_get_pointer_value(value);

    for( i = core_get_data_ptr_start(value) ; i <= core_get_data_ptr_end(value) ; i++ )
    {
        if( get_list_node_type(list, i) == FLOAT )
        {
            tmp_str = core_convert_float_to_string(env, to_double(get_list_node_value(list, i)));
            strsize += strlen(tmp_str) + 1;
        }
        else if( get_list_node_type(list, i) == INTEGER )
        {
            tmp_str = core_conver_long_to_string(env, to_long(get_list_node_value(list, i)));
            strsize += strlen(tmp_str) + 1;
        }
        else if( get_list_node_type(list, i) == STRING )
        {
            strsize += strlen(to_string(get_list_node_value(list, i))) + 3;
            tmp_str = to_string(get_list_node_value(list, i));

            while( *tmp_str )
            {
                if( *tmp_str == '"' )
                {
                    strsize++;
                }
                else if( *tmp_str == '\\' ) /* GDR 111599 #835 */
                {
                    strsize++;    /* GDR 111599 #835 */
                }

                tmp_str++;
            }
        }

#if OBJECT_SYSTEM
        else if( get_list_node_type(list, i) == INSTANCE_NAME )
        {
            strsize += strlen(to_string(get_list_node_value(list, i))) + 3;
        }
        else if( get_list_node_type(list, i) == INSTANCE_ADDRESS )
        {
            strsize += strlen(to_string(((INSTANCE_TYPE *)
                                         get_list_node_value(list, i))->name)) + 3;
        }
#endif

        else
        {
            core_set_type(tempDO, get_list_node_type(list, i));
            core_set_value(tempDO, get_list_node_value(list, i));
            strsize += strlen(core_convert_data_object_to_string(env, &tempDO)) + 1;
        }
    }

    /*=============================================
     * Allocate the string and copy all components
     * of the LIST variable to it.
     *=============================================*/

    if( strsize == 0 )
    {
        return(store_atom(env, ""));
    }

    ret_str = (char *)core_mem_alloc_no_init(env, strsize);

    for( j = 0, i = core_get_data_ptr_start(value); i <= core_get_data_ptr_end(value) ; i++ )
    {
        /*============================
         * Convert numbers to strings
         *============================*/

        if( get_list_node_type(list, i) == FLOAT )
        {
            tmp_str = core_convert_float_to_string(env, to_double(get_list_node_value(list, i)));

            while( *tmp_str )
            {
                *(ret_str + j) = *tmp_str;
                j++, tmp_str++;
            }
        }
        else if( get_list_node_type(list, i) == INTEGER )
        {
            tmp_str = core_conver_long_to_string(env, to_long(get_list_node_value(list, i)));

            while( *tmp_str )
            {
                *(ret_str + j) = *tmp_str;
                j++, tmp_str++;
            }
        }

        /*=======================================
         * Enclose strings in quotes and preceed
         * imbedded quotes with a backslash
         *=======================================*/

        else if( get_list_node_type(list, i) == STRING )
        {
            tmp_str = to_string(get_list_node_value(list, i));
            *(ret_str + j) = '"';
            j++;

            while( *tmp_str )
            {
                if( *tmp_str == '"' )
                {
                    *(ret_str + j) = '\\';
                    j++;
                }
                else if( *tmp_str == '\\' ) /* GDR 111599 #835 */
                {                        /* GDR 111599 #835 */
                    *(ret_str + j) = '\\';    /* GDR 111599 #835 */
                    j++;                    /* GDR 111599 #835 */
                }                        /* GDR 111599 #835 */

                *(ret_str + j) = *tmp_str;
                j++, tmp_str++;
            }

            *(ret_str + j) = '"';
            j++;
        }

#if OBJECT_SYSTEM
        else if( get_list_node_type(list, i) == INSTANCE_NAME )
        {
            tmp_str = to_string(get_list_node_value(list, i));
            *(ret_str + j++) = '[';

            while( *tmp_str )
            {
                *(ret_str + j) = *tmp_str;
                j++, tmp_str++;
            }

            *(ret_str + j++) = ']';
        }
        else if( get_list_node_type(list, i) == INSTANCE_ADDRESS )
        {
            tmp_str = to_string(((INSTANCE_TYPE *)get_list_node_value(list, i))->name);
            *(ret_str + j++) = '[';

            while( *tmp_str )
            {
                *(ret_str + j) = *tmp_str;
                j++, tmp_str++;
            }

            *(ret_str + j++) = ']';
        }
#endif
        else
        {
            core_set_type(tempDO, get_list_node_type(list, i));
            core_set_value(tempDO, get_list_node_value(list, i));
            tmp_str = core_convert_data_object_to_string(env, &tempDO);

            while( *tmp_str )
            {
                *(ret_str + j) = *tmp_str;
                j++, tmp_str++;
            }
        }
        *(ret_str + j) = ' ';
        j++;
    }

    *(ret_str + j - 1) = '\0';

    /*====================
     * Return the string.
     *====================*/

    rv = store_atom(env, ret_str);
    core_mem_release(env, ret_str, strsize);
    return(rv);
}

void concatenate_lists(void *env, core_data_object *ret, core_expression_object *expptr, int garbageSegment)
{
    core_data_object val_ptr;
    core_data_object *val_arr;
    struct list *list;
    struct list *orig_ptr;
    long start, end, i, j, k, argCount;
    unsigned long seg_size;

    argCount = core_count_args(expptr);

    /*=========================================
     * If no arguments are given return a NULL
     * list of length zero.
     *=========================================*/

    if( argCount == 0 )
    {
        core_set_pointer_type(ret, LIST);
        core_set_data_ptr_start(ret, 1);
        core_set_data_ptr_end(ret, 0);

        if( garbageSegment )
        {
            list = (struct list *)create_list(env, 0L);
        }
        else
        {
            list = (struct list *)create_sized_list(env, 0L);
        }

        core_set_pointer_value(ret, (void *)list);
        return;
    }

    else
    {
        /*========================================
         * Get a new segment with length equal to
         * the total length of all the arguments.
         *========================================*/

        val_arr = (core_data_object *)core_mem_alloc_large_no_init(env, (long)sizeof(core_data_object) * argCount);
        seg_size = 0;

        for( i = 1 ; i <= argCount ; i++, expptr = expptr->next_arg )
        {
            core_eval_expression(env, expptr, &val_ptr);

            if( core_get_evaluation_data(env)->eval_error )
            {
                core_set_pointer_type(ret, LIST);
                core_set_data_ptr_start(ret, 1);
                core_set_data_ptr_end(ret, 0);

                if( garbageSegment )
                {
                    list = (struct list *)create_list(env, 0L);
                }
                else
                {
                    list = (struct list *)create_sized_list(env, 0L);
                }

                core_set_pointer_value(ret, (void *)list);
                core_mem_release_sized(env, val_arr, (long)sizeof(core_data_object) * argCount);
                return;
            }

            core_set_pointer_type(val_arr + i - 1, core_get_type(val_ptr));

            if( core_get_type(val_ptr) == LIST )
            {
                core_set_pointer_value(val_arr + i - 1, core_get_pointer_value(&val_ptr));
                start = core_get_data_start(val_ptr);
                end = core_get_data_end(val_ptr);
            }
            else if( core_get_type(val_ptr) == RVOID )
            {
                core_set_pointer_value(val_arr + i - 1, core_get_value(val_ptr));
                start = 1;
                end = 0;
            }
            else
            {
                core_set_pointer_value(val_arr + i - 1, core_get_value(val_ptr));
                start = end = -1;
            }

            seg_size += (unsigned long)(end - start + 1);
            core_set_data_ptr_start(val_arr + i - 1, start);
            core_set_data_ptr_end(val_arr + i - 1, end);
        }

        if( garbageSegment )
        {
            list = (struct list *)create_list(env, seg_size);
        }
        else
        {
            list = (struct list *)create_sized_list(env, seg_size);
        }

        /*========================================
         * Copy each argument into new segment.
         *========================================*/

        for( k = 0, j = 1; k < argCount; k++ )
        {
            if( core_get_pointer_type(val_arr + k) == LIST )
            {
                start = core_get_data_ptr_start(val_arr + k);
                end = core_get_data_ptr_end(val_arr + k);
                orig_ptr = (struct list *)core_get_pointer_value(val_arr + k);

                for( i = start; i < end + 1; i++, j++ )
                {
                    set_list_node_type(list, j, (get_list_node_type(orig_ptr, i)));
                    set_list_node_value(list, j, (get_list_node_value(orig_ptr, i)));
                }
            }
            else if( core_get_pointer_type(val_arr + k) != LIST )
            {
                set_list_node_type(list, j, (short)(core_get_pointer_type(val_arr + k)));
                set_list_node_value(list, j, (core_get_pointer_value(val_arr + k)));
                j++;
            }
        }

        /*=========================
         * Return the new segment.
         *=========================*/

        core_set_pointer_type(ret, LIST);
        core_set_data_ptr_start(ret, 1);
        core_set_data_ptr_end(ret, (long)seg_size);
        core_set_pointer_value(ret, (void *)list);
        core_mem_release_sized(env, val_arr, (long)sizeof(core_data_object) * argCount);
        return;
    }
}
