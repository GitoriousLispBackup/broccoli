/* Purpose: Contains the code for sorting functions.         */

#define _SORTFUN_SOURCE_

#include "setup.h"

#include "core_arguments.h"
#include "funcs_function.h"
#include "core_environment.h"
#include "core_evaluation.h"
#include "core_functions.h"
#include "core_memory.h"
#include "type_list.h"
#include "sysdep.h"

#include "funcs_sorting.h"

#if OFF
static void _merge_sort(void *, unsigned long, core_data_object *, int(*) (void *, core_data_object *, core_data_object *));

#define SORTFUN_DATA 7

struct sort_function_data
{
    struct core_expression *comparator;
};

#define get_sort_function_data(env) ((struct sort_function_data *)core_get_environment_data(env, SORTFUN_DATA))
#endif

/**************************************
 * LOCAL FUNCTION PROTOTYPES
 ***************************************/
#if OFF
static void DoMergeSort(void *, core_data_object *, core_data_object *, unsigned long,
                        unsigned long, unsigned long, unsigned long,
                        int(*) (void *, core_data_object *, core_data_object *));
static int  DefaultCompareSwapFunction(void *, core_data_object *, core_data_object *);
static void DeallocateSortFunctionData(void *);
#endif

/***************************************
 * init_sort_functions: Initializes
 *   the sorting functions.
 ****************************************/
void init_sort_functions(void *env)
{
#if OFF
    core_allocate_environment_data(env, SORTFUN_DATA, sizeof(struct sort_function_data), DeallocateSortFunctionData);

    core_define_function(env, "sort", 'u', PTR_FN broccoli_sort, "SortFunction", "1**w");
#endif
}

#if OFF

/******************************************************
 * DeallocateSortFunctionData: Deallocates environment
 *    data for the sort function.
 *******************************************************/
static void DeallocateSortFunctionData(void *env)
{
    core_return_expression(env, get_sort_function_data(env)->comparator);
}

/*************************************
 * DefaultCompareSwapFunction:
 **************************************/
static int DefaultCompareSwapFunction(void *env, core_data_object *item1, core_data_object *item2)
{
    core_data_object ret;

    get_sort_function_data(env)->comparator->args = core_generate_constant(env, item1->type, item1->value);
    get_sort_function_data(env)->comparator->args->next_arg = core_generate_constant(env, item2->type, item2->value);
    core_increment_expression(env, get_sort_function_data(env)->comparator);
    core_eval_expression(env, get_sort_function_data(env)->comparator, &ret);
    core_decrement_expression(env, get_sort_function_data(env)->comparator);
    core_return_expression(env, get_sort_function_data(env)->comparator->args);
    get_sort_function_data(env)->comparator->args = NULL;

    if((core_get_type(ret) == ATOM) &&
       (core_get_value(ret) == get_false(env)))
    {
        return(FALSE);
    }

    return(TRUE);
}

/*************************************
 * broccoli_sort: H/L access routine
 *   for the rest$ function.
 **************************************/
void broccoli_sort(void *env, core_data_object_ptr ret)
{
    long argumentCount, i, j, k = 0;
    core_data_object *args, *args2;
    core_data_object theArg;
    struct list *list, *tempList;
    char *functionName;
    struct core_expression *functionReference;
    int argumentSize = 0;
    struct core_function_definition *fptr;

#if DEFFUNCTION_CONSTRUCT
    FUNCTION_DEFINITION *dptr;
#endif

    /*==================================
     * Set up the default return value.
     *==================================*/

    core_set_pointer_type(ret, ATOM);
    core_set_pointer_value(ret, get_false(env));

    /*=============================================
     * The function expects at least one argument.
     *=============================================*/

    if((argumentCount = core_check_arg_count(env, "sort", AT_LEAST, 1)) == -1 )
    {
        return;
    }

    /*=============================================
     * Verify that the comparison function exists.
     *=============================================*/

    if( core_check_arg_type(env, "sort", 1, ATOM, &theArg) == FALSE )
    {
        return;
    }

    functionName = core_convert_data_to_string(theArg);
    functionReference = core_convert_expression_to_function(env, functionName);

    if( functionReference == NULL )
    {
        report_explicit_type_error(env, "sort", 1, "function name, deffunction name, or defgeneric name");
        return;
    }

    /*======================================
     * For an external function, verify the
     * correct number of arguments.
     *======================================*/

    if( functionReference->type == FCALL )
    {
        fptr = (struct core_function_definition *)functionReference->value;

        if((core_get_min_args(fptr) > 2) ||
           (core_get_max_args(fptr) == 0) ||
           (core_get_max_args(fptr) == 1))
        {
            report_explicit_type_error(env, "sort", 1, "function name expecting two arguments");
            core_return_expression(env, functionReference);
            return;
        }
    }

    /*=======================================
     * For a deffunction, verify the correct
     * number of arguments.
     *=======================================*/

#if DEFFUNCTION_CONSTRUCT

    if( functionReference->type == PCALL )
    {
        dptr = (FUNCTION_DEFINITION *)functionReference->value;

        if((dptr->min_args > 2) ||
           (dptr->max_args == 0) ||
           (dptr->max_args == 1))
        {
            report_explicit_type_error(env, "sort", 1, "deffunction name expecting two arguments");
            core_return_expression(env, functionReference);
            return;
        }
    }

#endif

    /*=====================================
     * If there are no items to be sorted,
     * then return an empty list.
     *=====================================*/

    if( argumentCount == 1 )
    {
        core_create_error_list(env, ret);
        core_return_expression(env, functionReference);
        return;
    }

    /*=====================================
     * Retrieve the arguments to be sorted
     * and determine how many there are.
     *=====================================*/

    args = (core_data_object *)core_mem_alloc(env, (argumentCount - 1) * sizeof(core_data_object));

    for( i = 2; i <= argumentCount; i++ )
    {
        core_get_arg_at(env, i, &args[i - 2]);

        if( core_get_type(args[i - 2]) == LIST )
        {
            argumentSize += core_get_data_ptr_length(&args[i - 2]);
        }
        else
        {
            argumentSize++;
        }
    }

    if( argumentSize == 0 )
    {
        core_mem_free(env, args, (argumentCount - 1) * sizeof(core_data_object)); /* Bug Fix */
        core_create_error_list(env, ret);
        core_return_expression(env, functionReference);
        return;
    }

    /*====================================
     * Pack all of the items to be sorted
     * into a data object array.
     *====================================*/

    args2 = (core_data_object *)core_mem_alloc(env, argumentSize * sizeof(core_data_object));

    for( i = 2; i <= argumentCount; i++ )
    {
        if( core_get_type(args[i - 2]) == LIST )
        {
            tempList = (struct list *)core_get_value(args[i - 2]);

            for( j = core_get_data_start(args[i - 2]); j <= core_get_data_end(args[i - 2]); j++, k++ )
            {
                core_set_type(args2[k], get_list_node_type(tempList, j));
                core_set_value(args2[k], get_list_node_value(tempList, j));
            }
        }
        else
        {
            core_set_type(args2[k], core_get_type(args[i - 2]));
            core_set_value(args2[k], core_get_value(args[i - 2]));
            k++;
        }
    }

    core_mem_free(env, args, (argumentCount - 1) * sizeof(core_data_object));

    functionReference->next_arg = get_sort_function_data(env)->comparator;
    get_sort_function_data(env)->comparator = functionReference;

    for( i = 0; i < argumentSize; i++ )
    {
        core_value_increment(env, &args2[i]);
    }

    _merge_sort(env, (unsigned long)argumentSize, args2, DefaultCompareSwapFunction);

    for( i = 0; i < argumentSize; i++ )
    {
        core_value_decrement(env, &args2[i]);
    }

    get_sort_function_data(env)->comparator = get_sort_function_data(env)->comparator->next_arg;
    functionReference->next_arg = NULL;
    core_return_expression(env, functionReference);

    list = (struct list *)create_list(env, (unsigned long)argumentSize);

    for( i = 0; i < argumentSize; i++ )
    {
        set_list_node_type(list, i + 1, core_get_type(args2[i]));
        set_list_node_value(list, i + 1, core_get_value(args2[i]));
    }

    core_mem_free(env, args2, argumentSize * sizeof(core_data_object));

    core_set_pointer_type(ret, LIST);
    core_set_data_ptr_start(ret, 1);
    core_set_data_ptr_end(ret, argumentSize);
    core_set_pointer_value(ret, (void *)list);
}

/******************************************
 * merge_sort: Sorts a list of fields
 *   according to user specified criteria.
 *******************************************/
void _merge_sort(void *env, unsigned long listSize, core_data_object *list, int (*swapFunction)(void *, core_data_object *, core_data_object  *))
{
    core_data_object *tempList;
    unsigned long middle;

    if( listSize <= 1 )
    {
        return;
    }

    /*==============================
     * Create the temporary storage
     * is_needed for the merge sort.
     *==============================*/

    tempList = (core_data_object *)core_mem_alloc(env, listSize * sizeof(core_data_object));

    /*=====================================
     * Call the merge sort driver routine.
     *=====================================*/

    middle = (listSize + 1) / 2;
    DoMergeSort(env, list, tempList, 0, middle - 1, middle, listSize - 1, swapFunction);

    /*==================================
     * Deallocate the temporary storage
     * is_needed by the merge sort.
     *==================================*/

    core_mem_free(env, tempList, listSize * sizeof(core_data_object));
}

/*****************************************************
 * DoMergeSort: Driver routine for performing a merge
 *   sort on an array of core_data_object structures.
 ******************************************************/
static void DoMergeSort(void *env, core_data_object *list, core_data_object *tempList, unsigned long s1, unsigned long e1, unsigned long s2, unsigned long e2, int (*swapFunction)(void *, core_data_object *, core_data_object *))
{
    core_data_object temp;
    unsigned long middle, size;
    unsigned long c1, c2, mergePoint;

    /* Sort the two subareas before merging them. */

    if( s1 == e1 )
    {     /* List doesn't need to be merged. */
    }
    else if((s1 + 1) == e1 )
    {
        if((*swapFunction)(env, &list[s1], &list[e1]))
        {
            core_copy_data(&temp, &list[s1]);
            core_copy_data(&list[s1], &list[e1]);
            core_copy_data(&list[e1], &temp);
        }
    }
    else
    {
        size = ((e1 - s1) + 1);
        middle = s1 + ((size + 1) / 2);
        DoMergeSort(env, list, tempList, s1, middle - 1, middle, e1, swapFunction);
    }

    if( s2 == e2 )
    {     /* List doesn't need to be merged. */
    }
    else if((s2 + 1) == e2 )
    {
        if((*swapFunction)(env, &list[s2], &list[e2]))
        {
            core_copy_data(&temp, &list[s2]);
            core_copy_data(&list[s2], &list[e2]);
            core_copy_data(&list[e2], &temp);
        }
    }
    else
    {
        size = ((e2 - s2) + 1);
        middle = s2 + ((size + 1) / 2);
        DoMergeSort(env, list, tempList, s2, middle - 1, middle, e2, swapFunction);
    }

    /*======================
     * Merge the two areas.
     *======================*/

    mergePoint = s1;
    c1 = s1;
    c2 = s2;

    while( mergePoint <= e2 )
    {
        if( c1 > e1 )
        {
            core_copy_data(&tempList[mergePoint], &list[c2]);
            c2++;
            mergePoint++;
        }
        else if( c2 > e2 )
        {
            core_copy_data(&tempList[mergePoint], &list[c1]);
            c1++;
            mergePoint++;
        }
        else if((*swapFunction)(env, &list[c1], &list[c2]))
        {
            core_copy_data(&tempList[mergePoint], &list[c2]);
            c2++;
            mergePoint++;
        }
        else
        {
            core_copy_data(&tempList[mergePoint], &list[c1]);
            c1++;
            mergePoint++;
        }
    }

    /*=======================================
     * Copy them back to the original array.
     *=======================================*/

    for( c1 = s1; c1 <= e2; c1++ )
    {
        core_copy_data(&list[c1], &tempList[c1]);
    }
}

#endif
