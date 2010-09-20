/* Purpose: Query Functions for Objects                      */

/* =========================================
 *****************************************
 *            EXTERNAL DEFINITIONS
 *  =========================================
 ***************************************** */
#include "setup.h"

#if INSTANCE_SET_QUERIES

#include "core_arguments.h"
#include "classes_kernel.h"
#include "funcs_class.h"
#include "core_environment.h"
#include "core_memory.h"
#include "parser_expressions.h"
#include "funcs_instance.h"
#include "classes_instances_init.h"
#include "parser_class_query.h"
#include "funcs_function.h"
#include "router.h"
#include "core_gc.h"

#define __CLASSES_INSTANCES_QUERY_SOURCE__
#include "classes_instances_query.h"

/* =========================================
 *****************************************
 *   INTERNALLY VISIBLE FUNCTION HEADERS
 *  =========================================
 ***************************************** */

static void          PushQueryCore(void *);
static void          PopQueryCore(void *);
static QUERY_CORE *  FindQueryCore(void *, int);
static QUERY_CLASS * DetermineQueryClasses(void *, core_expression_object *, char *, unsigned *);
static QUERY_CLASS * FormChain(void *, char *, core_data_object *);
static void          DeleteQueryClasses(void *, QUERY_CLASS *);
static int           TestForFirstInChain(void *, QUERY_CLASS *, int);
static int           TestForFirstInstanceInClass(void *, struct module_definition *, int, DEFCLASS *, QUERY_CLASS *, int);
static void          TestEntireChain(void *, QUERY_CLASS *, int);
static void          TestEntireClass(void *, struct module_definition *, int, DEFCLASS *, QUERY_CLASS *, int);
static void          AddSolution(void *);
static void          PopQuerySoln(void *);

/****************************************************
 *  NAME         : SetupQuery
 *  DESCRIPTION  : Initializes instance query H/L
 *                functions and parsers
 *  INPUTS       : None
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Sets up kernel functions and parsers
 *  NOTES        : None
 ****************************************************/
globle void SetupQuery(void *theEnv)
{
    core_allocate_environment_data(theEnv, INSTANCE_QUERY_DATA, sizeof(struct instanceQueryData), NULL);

    InstanceQueryData(theEnv)->QUERY_DELIMETER_ATOM = (ATOM_HN *)store_atom(theEnv, QUERY_DELIMETER_STRING);
    inc_atom_count(InstanceQueryData(theEnv)->QUERY_DELIMETER_ATOM);

    core_define_function(theEnv, "(query-instance)", 'o',
                       PTR_FN GetQueryInstance, "GetQueryInstance", NULL);

    core_define_function(theEnv, "(query-instance-slot)", 'u',
                       PTR_FN GetQueryInstanceSlot, "GetQueryInstanceSlot", NULL);

    core_define_function(theEnv, "any-instancep", 'b', PTR_FN AnyInstances, "AnyInstances", NULL);
    core_add_function_parser(theEnv, "any-instancep", ParseQueryNoAction);

    core_define_function(theEnv, "find-instance", 'm',
                       PTR_FN QueryFindInstance, "QueryFindInstance", NULL);
    core_add_function_parser(theEnv, "find-instance", ParseQueryNoAction);

    core_define_function(theEnv, "find-all-instances", 'm',
                       PTR_FN QueryFindAllInstances, "QueryFindAllInstances", NULL);
    core_add_function_parser(theEnv, "find-all-instances", ParseQueryNoAction);

    core_define_function(theEnv, "do-for-instance", 'u',
                       PTR_FN QueryDoForInstance, "QueryDoForInstance", NULL);
    core_add_function_parser(theEnv, "do-for-instance", ParseQueryAction);

    core_define_function(theEnv, "do-for-all-instances", 'u',
                       PTR_FN QueryDoForAllInstances, "QueryDoForAllInstances", NULL);
    core_add_function_parser(theEnv, "do-for-all-instances", ParseQueryAction);

    core_define_function(theEnv, "delayed-do-for-all-instances", 'u',
                       PTR_FN DelayedQueryDoForAllInstances,
                       "DelayedQueryDoForAllInstances", NULL);
    core_add_function_parser(theEnv, "delayed-do-for-all-instances", ParseQueryAction);
}

/*************************************************************
 *  NAME         : GetQueryInstance
 *  DESCRIPTION  : Internal function for referring to instance
 *                 array on instance-queries
 *  INPUTS       : None
 *  RETURNS      : The name of the specified instance-set member
 *  SIDE EFFECTS : None
 *  NOTES        : H/L Syntax : ((query-instance) <index>)
 *************************************************************/
globle void *GetQueryInstance(void *theEnv)
{
    register QUERY_CORE *core;

    core = FindQueryCore(theEnv, to_int(core_get_pointer_value(core_get_first_arg())));
    return(GetFullInstanceName(theEnv, core->solns[to_int(core_get_pointer_value(core_get_first_arg()->next_arg))]));
}

/***************************************************************************
 * NAME         : GetQueryInstanceSlot
 * DESCRIPTION  : Internal function for referring to slots of instances in
 *                instance array on instance-queries
 * INPUTS       : The caller's result buffer
 * RETURNS      : Nothing useful
 * SIDE EFFECTS : Caller's result buffer set appropriately
 * NOTES        : H/L Syntax : ((query-instance-slot) <index> <slot-name>)
 **************************************************************************/
globle void GetQueryInstanceSlot(void *theEnv, core_data_object *result)
{
    INSTANCE_TYPE *ins;
    INSTANCE_SLOT *sp;
    core_data_object temp;
    QUERY_CORE *core;

    result->type = ATOM;
    result->value = get_false(theEnv);

    core = FindQueryCore(theEnv, to_int(core_get_pointer_value(core_get_first_arg())));
    ins = core->solns[to_int(core_get_pointer_value(core_get_first_arg()->next_arg))];
    core_eval_expression(theEnv, core_get_first_arg()->next_arg->next_arg, &temp);

    if( temp.type != ATOM )
    {
        report_explicit_type_error(theEnv, "get", 1, "symbol");
        core_set_eval_error(theEnv, TRUE);
        return;
    }

    sp = FindInstanceSlot(theEnv, ins, (ATOM_HN *)temp.value);

    if( sp == NULL )
    {
        error_method_duplication(theEnv, to_string(temp.value), "instance-set query");
        return;
    }

    result->type = (unsigned short)sp->type;
    result->value = sp->value;

    if( sp->type == LIST )
    {
        result->begin = 0;
        core_set_data_ptr_end(result, GetInstanceSlotLength(sp));
    }
}

/* =============================================================================
 *  =============================================================================
 *  Following are the instance query functions :
 *
 *  any-instancep         : Determines if any instances satisfy the query
 *  find-instance         : Finds first (set of) instance(s) which satisfies
 *                            the query and stores it in a list
 *  find-all-instances    : Finds all (sets of) instances which satisfy the
 *                            the query and stores them in a list
 *  do-for-instance       : Executes a given action for the first (set of)
 *                            instance(s) which satisfy the query
 *  do-for-all-instances  : Executes an action for all instances which satisfy
 *                            the query as they are found
 *  delayed-do-for-all-instances : Same as above - except that the list of instances
 *                            which satisfy the query is formed before any
 *                            actions are executed
 *
 *  Instance candidate search algorithm :
 *
 *  All permutations of first restriction class instances with other
 *    restriction class instances (Rightmost are varied first)
 *  All permutations of first restriction class's subclasses' instances with
 *    other restriction class instances.
 *  And  so on...
 *
 *  For any one class, instances are examined in the order they were defined
 *
 *  Example :
 *  (defclass a (is-a standard-user))
 *  (defclass b (is-a standard-user))
 *  (defclass c (is-a standard-user))
 *  (defclass d (is-a a b))
 *  (make-instance a1 of a)
 *  (make-instance a2 of a)
 *  (make-instance b1 of b)
 *  (make-instance b2 of b)
 *  (make-instance c1 of c)
 *  (make-instance c2 of c)
 *  (make-instance d1 of d)
 *  (make-instance d2 of d)
 *
 *  (any-instancep ((?a a b) (?b c)) <query>)
 *
 *  The permutations (?a ?b) would be examined in the following order :
 *
 *  (a1 c1),(a1 c2),(a2 c1),(a2 c2),(d1 c1),(d1 c2),(d2 c1),(d2 c2),
 *  (b1 c1),(b1 c2),(b2 c1),(b2 c2),(d1 c1),(d1 c2),(d2 c1),(d2 c2)
 *
 *  Notice the duplication because d is a subclass of both and a and b.
 *  =============================================================================
 *  ============================================================================= */

/******************************************************************************
 *  NAME         : AnyInstances
 *  DESCRIPTION  : Determines if there any existing instances which satisfy
 *                the query
 *  INPUTS       : None
 *  RETURNS      : TRUE if the query is satisfied, FALSE otherwise
 *  SIDE EFFECTS : The query class-expressions are evaluated once,
 *                and the query boolean-expression is evaluated
 *                zero or more times (depending on instance restrictions
 *                and how early the expression evaulates to TRUE - if at all).
 *  NOTES        : H/L Syntax : See ParseQueryNoAction()
 ******************************************************************************/
globle BOOLEAN AnyInstances(void *theEnv)
{
    QUERY_CLASS *qclasses;
    unsigned rcnt;
    int TestResult;

    qclasses = DetermineQueryClasses(theEnv, core_get_first_arg()->next_arg,
                                     "any-instancep", &rcnt);

    if( qclasses == NULL )
    {
        return(FALSE);
    }

    PushQueryCore(theEnv);
    InstanceQueryData(theEnv)->QueryCore = core_mem_get_struct(theEnv, query_core);
    InstanceQueryData(theEnv)->QueryCore->solns = (INSTANCE_TYPE **)core_mem_alloc_no_init(theEnv, (sizeof(INSTANCE_TYPE *) * rcnt));
    InstanceQueryData(theEnv)->QueryCore->query = core_get_first_arg();
    TestResult = TestForFirstInChain(theEnv, qclasses, 0);
    InstanceQueryData(theEnv)->AbortQuery = FALSE;
    core_mem_release(theEnv, (void *)InstanceQueryData(theEnv)->QueryCore->solns, (sizeof(INSTANCE_TYPE *) * rcnt));
    core_mem_return_struct(theEnv, query_core, InstanceQueryData(theEnv)->QueryCore);
    PopQueryCore(theEnv);
    DeleteQueryClasses(theEnv, qclasses);
    return(TestResult);
}

/******************************************************************************
 *  NAME         : QueryFindInstance
 *  DESCRIPTION  : Finds the first set of instances which satisfy the query and
 *                stores their names in the user's list variable
 *  INPUTS       : Caller's result buffer
 *  RETURNS      : TRUE if the query is satisfied, FALSE otherwise
 *  SIDE EFFECTS : The query class-expressions are evaluated once,
 *                and the query boolean-expression is evaluated
 *                zero or more times (depending on instance restrictions
 *                and how early the expression evaulates to TRUE - if at all).
 *  NOTES        : H/L Syntax : See ParseQueryNoAction()
 ******************************************************************************/
globle void QueryFindInstance(void *theEnv, core_data_object *result)
{
    QUERY_CLASS *qclasses;
    unsigned rcnt, i;

    result->type = LIST;
    result->begin = 0;
    result->end = -1;
    qclasses = DetermineQueryClasses(theEnv, core_get_first_arg()->next_arg,
                                     "find-instance", &rcnt);

    if( qclasses == NULL )
    {
        result->value = (void *)create_list(theEnv, 0L);
        return;
    }

    PushQueryCore(theEnv);
    InstanceQueryData(theEnv)->QueryCore = core_mem_get_struct(theEnv, query_core);
    InstanceQueryData(theEnv)->QueryCore->solns = (INSTANCE_TYPE **)
                                                  core_mem_alloc_no_init(theEnv, (sizeof(INSTANCE_TYPE *) * rcnt));
    InstanceQueryData(theEnv)->QueryCore->query = core_get_first_arg();

    if( TestForFirstInChain(theEnv, qclasses, 0) == TRUE )
    {
        result->value = (void *)create_list(theEnv, rcnt);
        core_set_data_ptr_end(result, rcnt);

        for( i = 1 ; i <= rcnt ; i++ )
        {
            set_list_node_type(result->value, i, INSTANCE_NAME);
            set_list_node_value(result->value, i, GetFullInstanceName(theEnv, InstanceQueryData(theEnv)->QueryCore->solns[i - 1]));
        }
    }
    else
    {
        result->value = (void *)create_list(theEnv, 0L);
    }

    InstanceQueryData(theEnv)->AbortQuery = FALSE;
    core_mem_release(theEnv, (void *)InstanceQueryData(theEnv)->QueryCore->solns, (sizeof(INSTANCE_TYPE *) * rcnt));
    core_mem_return_struct(theEnv, query_core, InstanceQueryData(theEnv)->QueryCore);
    PopQueryCore(theEnv);
    DeleteQueryClasses(theEnv, qclasses);
}

/******************************************************************************
 *  NAME         : QueryFindAllInstances
 *  DESCRIPTION  : Finds all sets of instances which satisfy the query and
 *                stores their names in the user's list variable
 *
 *              The sets are stored sequentially :
 *
 *                Number of sets = (Multi-field length) / (Set length)
 *
 *              The first set is if the first (set length) atoms of the
 *                list variable, and so on.
 *  INPUTS       : Caller's result buffer
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : The query class-expressions are evaluated once,
 *                and the query boolean-expression is evaluated
 *                once for every instance set.
 *  NOTES        : H/L Syntax : See ParseQueryNoAction()
 ******************************************************************************/
globle void QueryFindAllInstances(void *theEnv, core_data_object *result)
{
    QUERY_CLASS *qclasses;
    unsigned rcnt;
    register unsigned i, j;

    result->type = LIST;
    result->begin = 0;
    result->end = -1;
    qclasses = DetermineQueryClasses(theEnv, core_get_first_arg()->next_arg,
                                     "find-all-instances", &rcnt);

    if( qclasses == NULL )
    {
        result->value = (void *)create_list(theEnv, 0L);
        return;
    }

    PushQueryCore(theEnv);
    InstanceQueryData(theEnv)->QueryCore = core_mem_get_struct(theEnv, query_core);
    InstanceQueryData(theEnv)->QueryCore->solns = (INSTANCE_TYPE **)core_mem_alloc_no_init(theEnv, (sizeof(INSTANCE_TYPE *) * rcnt));
    InstanceQueryData(theEnv)->QueryCore->query = core_get_first_arg();
    InstanceQueryData(theEnv)->QueryCore->action = NULL;
    InstanceQueryData(theEnv)->QueryCore->soln_set = NULL;
    InstanceQueryData(theEnv)->QueryCore->soln_size = rcnt;
    InstanceQueryData(theEnv)->QueryCore->soln_cnt = 0;
    TestEntireChain(theEnv, qclasses, 0);
    InstanceQueryData(theEnv)->AbortQuery = FALSE;
    result->value = (void *)create_list(theEnv, InstanceQueryData(theEnv)->QueryCore->soln_cnt * rcnt);

    while( InstanceQueryData(theEnv)->QueryCore->soln_set != NULL )
    {
        for( i = 0, j = (unsigned)(result->end + 2) ; i < rcnt ; i++, j++ )
        {
            set_list_node_type(result->value, j, INSTANCE_NAME);
            set_list_node_value(result->value, j, GetFullInstanceName(theEnv, InstanceQueryData(theEnv)->QueryCore->soln_set->soln[i]));
        }

        result->end = (long)j - 2;
        PopQuerySoln(theEnv);
    }

    core_mem_release(theEnv, (void *)InstanceQueryData(theEnv)->QueryCore->solns, (sizeof(INSTANCE_TYPE *) * rcnt));
    core_mem_return_struct(theEnv, query_core, InstanceQueryData(theEnv)->QueryCore);
    PopQueryCore(theEnv);
    DeleteQueryClasses(theEnv, qclasses);
}

/******************************************************************************
 *  NAME         : QueryDoForInstance
 *  DESCRIPTION  : Finds the first set of instances which satisfy the query and
 *                executes a user-action with that set
 *  INPUTS       : None
 *  RETURNS      : Caller's result buffer
 *  SIDE EFFECTS : The query class-expressions are evaluated once,
 *                and the query boolean-expression is evaluated
 *                zero or more times (depending on instance restrictions
 *                and how early the expression evaulates to TRUE - if at all).
 *                Also the action expression is executed zero or once.
 *              Caller's result buffer holds result of user-action
 *  NOTES        : H/L Syntax : See ParseQueryAction()
 ******************************************************************************/
globle void QueryDoForInstance(void *theEnv, core_data_object *result)
{
    QUERY_CLASS *qclasses;
    unsigned rcnt;

    result->type = ATOM;
    result->value = get_false(theEnv);
    qclasses = DetermineQueryClasses(theEnv, core_get_first_arg()->next_arg->next_arg,
                                     "do-for-instance", &rcnt);

    if( qclasses == NULL )
    {
        return;
    }

    PushQueryCore(theEnv);
    InstanceQueryData(theEnv)->QueryCore = core_mem_get_struct(theEnv, query_core);
    InstanceQueryData(theEnv)->QueryCore->solns = (INSTANCE_TYPE **)core_mem_alloc_no_init(theEnv, (sizeof(INSTANCE_TYPE *) * rcnt));
    InstanceQueryData(theEnv)->QueryCore->query = core_get_first_arg();
    InstanceQueryData(theEnv)->QueryCore->action = core_get_first_arg()->next_arg;

    if( TestForFirstInChain(theEnv, qclasses, 0) == TRUE )
    {
        core_eval_expression(theEnv, InstanceQueryData(theEnv)->QueryCore->action, result);
    }

    InstanceQueryData(theEnv)->AbortQuery = FALSE;
    get_flow_control_data(theEnv)->break_flag = FALSE;
    core_mem_release(theEnv, (void *)InstanceQueryData(theEnv)->QueryCore->solns, (sizeof(INSTANCE_TYPE *) * rcnt));
    core_mem_return_struct(theEnv, query_core, InstanceQueryData(theEnv)->QueryCore);
    PopQueryCore(theEnv);
    DeleteQueryClasses(theEnv, qclasses);
}

/******************************************************************************
 *  NAME         : QueryDoForAllInstances
 *  DESCRIPTION  : Finds all sets of instances which satisfy the query and
 *                executes a user-function for each set as it is found
 *  INPUTS       : Caller's result buffer
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : The query class-expressions are evaluated once,
 *                and the query boolean-expression is evaluated
 *                once for every instance set.  Also, the action is
 *                executed for every instance set.
 *              Caller's result buffer holds result of last action executed.
 *  NOTES        : H/L Syntax : See ParseQueryAction()
 ******************************************************************************/
globle void QueryDoForAllInstances(void *theEnv, core_data_object *result)
{
    QUERY_CLASS *qclasses;
    unsigned rcnt;

    result->type = ATOM;
    result->value = get_false(theEnv);
    qclasses = DetermineQueryClasses(theEnv, core_get_first_arg()->next_arg->next_arg,
                                     "do-for-all-instances", &rcnt);

    if( qclasses == NULL )
    {
        return;
    }

    PushQueryCore(theEnv);
    InstanceQueryData(theEnv)->QueryCore = core_mem_get_struct(theEnv, query_core);
    InstanceQueryData(theEnv)->QueryCore->solns = (INSTANCE_TYPE **)core_mem_alloc_no_init(theEnv, (sizeof(INSTANCE_TYPE *) * rcnt));
    InstanceQueryData(theEnv)->QueryCore->query = core_get_first_arg();
    InstanceQueryData(theEnv)->QueryCore->action = core_get_first_arg()->next_arg;
    InstanceQueryData(theEnv)->QueryCore->result = result;
    core_value_increment(theEnv, InstanceQueryData(theEnv)->QueryCore->result);
    TestEntireChain(theEnv, qclasses, 0);
    core_value_decrement(theEnv, InstanceQueryData(theEnv)->QueryCore->result);
    core_pass_return_value(theEnv, InstanceQueryData(theEnv)->QueryCore->result);
    InstanceQueryData(theEnv)->AbortQuery = FALSE;
    get_flow_control_data(theEnv)->break_flag = FALSE;
    core_mem_release(theEnv, (void *)InstanceQueryData(theEnv)->QueryCore->solns, (sizeof(INSTANCE_TYPE *) * rcnt));
    core_mem_return_struct(theEnv, query_core, InstanceQueryData(theEnv)->QueryCore);
    PopQueryCore(theEnv);
    DeleteQueryClasses(theEnv, qclasses);
}

/******************************************************************************
 *  NAME         : DelayedQueryDoForAllInstances
 *  DESCRIPTION  : Finds all sets of instances which satisfy the query and
 *                and exceutes a user-action for each set
 *
 *              This function differs from QueryDoForAllInstances() in
 *                that it forms the complete list of query satisfactions
 *                BEFORE executing any actions.
 *  INPUTS       : Caller's result buffer
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : The query class-expressions are evaluated once,
 *                and the query boolean-expression is evaluated
 *                once for every instance set.  The action is executed
 *                for evry query satisfaction.
 *              Caller's result buffer holds result of last action executed.
 *  NOTES        : H/L Syntax : See ParseQueryNoAction()
 ******************************************************************************/
globle void DelayedQueryDoForAllInstances(void *theEnv, core_data_object *result)
{
    QUERY_CLASS *qclasses;
    unsigned rcnt;
    register unsigned i;

    result->type = ATOM;
    result->value = get_false(theEnv);
    qclasses = DetermineQueryClasses(theEnv, core_get_first_arg()->next_arg->next_arg,
                                     "delayed-do-for-all-instances", &rcnt);

    if( qclasses == NULL )
    {
        return;
    }

    PushQueryCore(theEnv);
    InstanceQueryData(theEnv)->QueryCore = core_mem_get_struct(theEnv, query_core);
    InstanceQueryData(theEnv)->QueryCore->solns = (INSTANCE_TYPE **)core_mem_alloc_no_init(theEnv, (sizeof(INSTANCE_TYPE *) * rcnt));
    InstanceQueryData(theEnv)->QueryCore->query = core_get_first_arg();
    InstanceQueryData(theEnv)->QueryCore->action = NULL;
    InstanceQueryData(theEnv)->QueryCore->soln_set = NULL;
    InstanceQueryData(theEnv)->QueryCore->soln_size = rcnt;
    InstanceQueryData(theEnv)->QueryCore->soln_cnt = 0;
    TestEntireChain(theEnv, qclasses, 0);
    InstanceQueryData(theEnv)->AbortQuery = FALSE;
    InstanceQueryData(theEnv)->QueryCore->action = core_get_first_arg()->next_arg;

    while( InstanceQueryData(theEnv)->QueryCore->soln_set != NULL )
    {
        for( i = 0 ; i < rcnt ; i++ )
        {
            InstanceQueryData(theEnv)->QueryCore->solns[i] = InstanceQueryData(theEnv)->QueryCore->soln_set->soln[i];
        }

        PopQuerySoln(theEnv);
        core_get_evaluation_data(theEnv)->eval_depth++;
        core_eval_expression(theEnv, InstanceQueryData(theEnv)->QueryCore->action, result);
        core_get_evaluation_data(theEnv)->eval_depth--;

        if( get_flow_control_data(theEnv)->return_flag == TRUE )
        {
            core_pass_return_value(theEnv, result);
        }

        core_gc_periodic_cleanup(theEnv, FALSE, TRUE);

        if( core_get_evaluation_data(theEnv)->halt || get_flow_control_data(theEnv)->break_flag || get_flow_control_data(theEnv)->return_flag )
        {
            while( InstanceQueryData(theEnv)->QueryCore->soln_set != NULL )
            {
                PopQuerySoln(theEnv);
            }

            break;
        }
    }

    get_flow_control_data(theEnv)->break_flag = FALSE;
    core_mem_release(theEnv, (void *)InstanceQueryData(theEnv)->QueryCore->solns, (sizeof(INSTANCE_TYPE *) * rcnt));
    core_mem_return_struct(theEnv, query_core, InstanceQueryData(theEnv)->QueryCore);
    PopQueryCore(theEnv);
    DeleteQueryClasses(theEnv, qclasses);
}

/* =========================================
 *****************************************
 *       INTERNALLY VISIBLE FUNCTIONS
 *  =========================================
 ***************************************** */

/*******************************************************
 *  NAME         : PushQueryCore
 *  DESCRIPTION  : Pushes the current QueryCore onto stack
 *  INPUTS       : None
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Allocates new stack node and changes
 *                QueryCoreStack
 *  NOTES        : None
 *******************************************************/
static void PushQueryCore(void *theEnv)
{
    QUERY_STACK *qptr;

    qptr = core_mem_get_struct(theEnv, query_stack);
    qptr->core = InstanceQueryData(theEnv)->QueryCore;
    qptr->nxt = InstanceQueryData(theEnv)->QueryCoreStack;
    InstanceQueryData(theEnv)->QueryCoreStack = qptr;
}

/******************************************************
 *  NAME         : PopQueryCore
 *  DESCRIPTION  : Pops top of QueryCore stack and
 *                restores QueryCore to this core
 *  INPUTS       : None
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Stack node deallocated, QueryCoreStack
 *                changed and QueryCore reset
 *  NOTES        : Assumes stack is not empty
 ******************************************************/
static void PopQueryCore(void *theEnv)
{
    QUERY_STACK *qptr;

    InstanceQueryData(theEnv)->QueryCore = InstanceQueryData(theEnv)->QueryCoreStack->core;
    qptr = InstanceQueryData(theEnv)->QueryCoreStack;
    InstanceQueryData(theEnv)->QueryCoreStack = InstanceQueryData(theEnv)->QueryCoreStack->nxt;
    core_mem_return_struct(theEnv, query_stack, qptr);
}

/***************************************************
 *  NAME         : FindQueryCore
 *  DESCRIPTION  : Looks up a QueryCore Stack Frame
 *                 Depth 0 is current frame
 *                 1 is next deepest, etc.
 *  INPUTS       : Depth
 *  RETURNS      : Address of query core stack frame
 *  SIDE EFFECTS : None
 *  NOTES        : None
 ***************************************************/
static QUERY_CORE *FindQueryCore(void *theEnv, int depth)
{
    QUERY_STACK *qptr;

    if( depth == 0 )
    {
        return(InstanceQueryData(theEnv)->QueryCore);
    }

    qptr = InstanceQueryData(theEnv)->QueryCoreStack;

    while( depth > 1 )
    {
        qptr = qptr->nxt;
        depth--;
    }

    return(qptr->core);
}

/**********************************************************
 *  NAME         : DetermineQueryClasses
 *  DESCRIPTION  : Builds a list of classes to be used in
 *                instance queries - uses parse form.
 *  INPUTS       : 1) The parse class expression chain
 *              2) The name of the function being executed
 *              3) Caller's buffer for restriction count
 *                 (# of separate lists)
 *  RETURNS      : The query list, or NULL on errors
 *  SIDE EFFECTS : Memory allocated for list
 *              Busy count incremented for all classes
 *  NOTES        : Each restriction is linked by nxt pointer,
 *                multiple classes in a restriction are
 *                linked by the chain pointer.
 *              Rcnt caller's buffer is set to reflect the
 *                total number of chains
 *              Assumes classExp is not NULL and that each
 *                restriction chain is terminated with
 *                the QUERY_DELIMITER_ATOM "(QDS)"
 **********************************************************/
static QUERY_CLASS *DetermineQueryClasses(void *theEnv, core_expression_object *classExp, char *func, unsigned *rcnt)
{
    QUERY_CLASS *clist = NULL, *cnxt = NULL, *cchain = NULL, *tmp;
    int new_list = FALSE;
    core_data_object temp;

    *rcnt = 0;

    while( classExp != NULL )
    {
        if( core_eval_expression(theEnv, classExp, &temp))
        {
            DeleteQueryClasses(theEnv, clist);
            return(NULL);
        }

        if((temp.type == ATOM) && (temp.value == (void *)InstanceQueryData(theEnv)->QUERY_DELIMETER_ATOM))
        {
            new_list = TRUE;
            (*rcnt)++;
        }
        else if((tmp = FormChain(theEnv, func, &temp)) != NULL )
        {
            if( clist == NULL )
            {
                clist = cnxt = cchain = tmp;
            }
            else if( new_list == TRUE )
            {
                new_list = FALSE;
                cnxt->nxt = tmp;
                cnxt = cchain = tmp;
            }
            else
            {
                cchain->chain = tmp;
            }

            while( cchain->chain != NULL )
            {
                cchain = cchain->chain;
            }
        }
        else
        {
            error_syntax(theEnv, "instance-set query class restrictions");
            DeleteQueryClasses(theEnv, clist);
            core_set_eval_error(theEnv, TRUE);
            return(NULL);
        }

        classExp = classExp->next_arg;
    }

    return(clist);
}

/*************************************************************
 *  NAME         : FormChain
 *  DESCRIPTION  : Builds a list of classes to be used in
 *                instance queries - uses parse form.
 *  INPUTS       : 1) Name of calling function for error msgs
 *              2) Data object - must be a symbol or a
 *                   list value containing all symbols
 *              The symbols must be names of existing classes
 *  RETURNS      : The query chain, or NULL on errors
 *  SIDE EFFECTS : Memory allocated for chain
 *              Busy count incremented for all classes
 *  NOTES        : None
 *************************************************************/
static QUERY_CLASS *FormChain(void *theEnv, char *func, core_data_object *val)
{
    DEFCLASS *cls;
    QUERY_CLASS *head, *bot, *tmp;
    register long i, end; /* 6.04 Bug Fix */
    char *className;
    struct module_definition *currentModule;

    currentModule = ((struct module_definition *)get_current_module(theEnv));

    if( val->type == DEFCLASS_PTR )
    {
        IncrementDefclassBusyCount(theEnv, (void *)val->value);
        head = core_mem_get_struct(theEnv, query_class);
        head->cls = (DEFCLASS *)val->value;

        if( DefclassInScope(theEnv, head->cls, currentModule))
        {
            head->module_def = currentModule;
        }
        else
        {
            head->module_def = head->cls->header.my_module->module_def;
        }

        head->chain = NULL;
        head->nxt = NULL;
        return(head);
    }

    if( val->type == ATOM )
    {
        /* ===============================================
         *  Allow instance-set query restrictions to have a
         *  module specifier as part of the class name,
         *  but search imported defclasses too if a
         *  module specifier is not given
         *  =============================================== */
        cls = LookupDefclassByMdlOrScope(theEnv, core_convert_data_ptr_to_string(val));

        if( cls == NULL )
        {
            ClassExistError(theEnv, func, core_convert_data_ptr_to_string(val));
            return(NULL);
        }

        IncrementDefclassBusyCount(theEnv, (void *)cls);
        head = core_mem_get_struct(theEnv, query_class);
        head->cls = cls;

        if( DefclassInScope(theEnv, head->cls, currentModule))
        {
            head->module_def = currentModule;
        }
        else
        {
            head->module_def = head->cls->header.my_module->module_def;
        }

        head->chain = NULL;
        head->nxt = NULL;
        return(head);
    }

    if( val->type == LIST )
    {
        head = bot = NULL;
        end = core_get_data_ptr_end(val);

        for( i = core_get_data_ptr_start(val) ; i <= end ; i++ )
        {
            if( get_list_node_type(val->value, i) == ATOM )
            {
                className = to_string(get_list_node_value(val->value, i));
                cls = LookupDefclassByMdlOrScope(theEnv, className);

                if( cls == NULL )
                {
                    ClassExistError(theEnv, func, className);
                    DeleteQueryClasses(theEnv, head);
                    return(NULL);
                }
            }
            else
            {
                DeleteQueryClasses(theEnv, head);
                return(NULL);
            }

            IncrementDefclassBusyCount(theEnv, (void *)cls);
            tmp = core_mem_get_struct(theEnv, query_class);
            tmp->cls = cls;

            if( DefclassInScope(theEnv, tmp->cls, currentModule))
            {
                tmp->module_def = currentModule;
            }
            else
            {
                tmp->module_def = tmp->cls->header.my_module->module_def;
            }

            tmp->chain = NULL;
            tmp->nxt = NULL;

            if( head == NULL )
            {
                head = tmp;
            }
            else
            {
                bot->chain = tmp;
            }

            bot = tmp;
        }

        return(head);
    }

    return(NULL);
}

/******************************************************
 *  NAME         : DeleteQueryClasses
 *  DESCRIPTION  : Deletes a query class-list
 *  INPUTS       : The query list address
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Nodes deallocated
 *              Busy count decremented for all classes
 *  NOTES        : None
 ******************************************************/
static void DeleteQueryClasses(void *theEnv, QUERY_CLASS *qlist)
{
    QUERY_CLASS *tmp;

    while( qlist != NULL )
    {
        while( qlist->chain != NULL )
        {
            tmp = qlist->chain;
            qlist->chain = qlist->chain->chain;
            DecrementDefclassBusyCount(theEnv, (void *)tmp->cls);
            core_mem_return_struct(theEnv, query_class, tmp);
        }

        tmp = qlist;
        qlist = qlist->nxt;
        DecrementDefclassBusyCount(theEnv, (void *)tmp->cls);
        core_mem_return_struct(theEnv, query_class, tmp);
    }
}

/************************************************************
 *  NAME         : TestForFirstInChain
 *  DESCRIPTION  : Processes all classes in a restriction chain
 *                until success or done
 *  INPUTS       : 1) The current chain
 *              2) The index of the chain restriction
 *                  (e.g. the 4th query-variable)
 *  RETURNS      : TRUE if query succeeds, FALSE otherwise
 *  SIDE EFFECTS : Sets current restriction class
 *              Instance variable values set
 *  NOTES        : None
 ************************************************************/
static int TestForFirstInChain(void *theEnv, QUERY_CLASS *qchain, int indx)
{
    QUERY_CLASS *qptr;
    int id;

    InstanceQueryData(theEnv)->AbortQuery = TRUE;

    for( qptr = qchain ; qptr != NULL ; qptr = qptr->chain )
    {
        InstanceQueryData(theEnv)->AbortQuery = FALSE;

        if((id = GetTraversalID(theEnv)) == -1 )
        {
            return(FALSE);
        }

        if( TestForFirstInstanceInClass(theEnv, qptr->module_def, id, qptr->cls, qchain, indx))
        {
            ReleaseTraversalID(theEnv);
            return(TRUE);
        }

        ReleaseTraversalID(theEnv);

        if((core_get_evaluation_data(theEnv)->halt == TRUE) || (InstanceQueryData(theEnv)->AbortQuery == TRUE))
        {
            return(FALSE);
        }
    }

    return(FALSE);
}

/*****************************************************************
 *  NAME         : TestForFirstInstanceInClass
 *  DESCRIPTION  : Processes all instances in a class and then
 *                all subclasses of a class until success or done
 *  INPUTS       : 1) The module for which classes tested must be
 *                 in scope
 *              2) Visitation traversal id
 *              3) The class
 *              4) The current class restriction chain
 *              5) The index of the current restriction
 *  RETURNS      : TRUE if query succeeds, FALSE otherwise
 *  SIDE EFFECTS : Instance variable values set
 *  NOTES        : None
 *****************************************************************/
static int TestForFirstInstanceInClass(void *theEnv, struct module_definition *module_def, int id, DEFCLASS *cls, QUERY_CLASS *qchain, int indx)
{
    long i;
    INSTANCE_TYPE *ins;
    core_data_object temp;

    if( TestTraversalID(cls->traversalRecord, id))
    {
        return(FALSE);
    }

    SetTraversalID(cls->traversalRecord, id);

    if( DefclassInScope(theEnv, cls, module_def) == FALSE )
    {
        return(FALSE);
    }

    ins = cls->instanceList;

    while( ins != NULL )
    {
        InstanceQueryData(theEnv)->QueryCore->solns[indx] = ins;

        if( qchain->nxt != NULL )
        {
            ins->busy++;

            if( TestForFirstInChain(theEnv, qchain->nxt, indx + 1) == TRUE )
            {
                ins->busy--;
                break;
            }

            ins->busy--;

            if((core_get_evaluation_data(theEnv)->halt == TRUE) || (InstanceQueryData(theEnv)->AbortQuery == TRUE))
            {
                break;
            }
        }
        else
        {
            ins->busy++;
            core_get_evaluation_data(theEnv)->eval_depth++;
            core_eval_expression(theEnv, InstanceQueryData(theEnv)->QueryCore->query, &temp);
            core_get_evaluation_data(theEnv)->eval_depth--;
            core_gc_periodic_cleanup(theEnv, FALSE, TRUE);
            ins->busy--;

            if( core_get_evaluation_data(theEnv)->halt == TRUE )
            {
                break;
            }

            if((temp.type != ATOM) ? TRUE :
               (temp.value != get_false(theEnv)))
            {
                break;
            }
        }

        ins = ins->nxtClass;

        while((ins != NULL) ? (ins->garbage == 1) : FALSE )
        {
            ins = ins->nxtClass;
        }
    }

    if( ins != NULL )
    {
        return(((core_get_evaluation_data(theEnv)->halt == TRUE) || (InstanceQueryData(theEnv)->AbortQuery == TRUE))
               ? FALSE : TRUE);
    }

    for( i = 0 ; i < cls->directSubclasses.classCount ; i++ )
    {
        if( TestForFirstInstanceInClass(theEnv, module_def, id, cls->directSubclasses.classArray[i],
                                        qchain, indx))
        {
            return(TRUE);
        }

        if((core_get_evaluation_data(theEnv)->halt == TRUE) || (InstanceQueryData(theEnv)->AbortQuery == TRUE))
        {
            return(FALSE);
        }
    }

    return(FALSE);
}

/************************************************************
 *  NAME         : TestEntireChain
 *  DESCRIPTION  : Processes all classes in a restriction chain
 *                until done
 *  INPUTS       : 1) The current chain
 *              2) The index of the chain restriction
 *                  (i.e. the 4th query-variable)
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Sets current restriction class
 *              Query instance variables set
 *              Solution sets stored in global list
 *  NOTES        : None
 ************************************************************/
static void TestEntireChain(void *theEnv, QUERY_CLASS *qchain, int indx)
{
    QUERY_CLASS *qptr;
    int id;

    InstanceQueryData(theEnv)->AbortQuery = TRUE;

    for( qptr = qchain ; qptr != NULL ; qptr = qptr->chain )
    {
        InstanceQueryData(theEnv)->AbortQuery = FALSE;

        if((id = GetTraversalID(theEnv)) == -1 )
        {
            return;
        }

        TestEntireClass(theEnv, qptr->module_def, id, qptr->cls, qchain, indx);
        ReleaseTraversalID(theEnv);

        if((core_get_evaluation_data(theEnv)->halt == TRUE) || (InstanceQueryData(theEnv)->AbortQuery == TRUE))
        {
            return;
        }
    }
}

/*****************************************************************
 *  NAME         : TestEntireClass
 *  DESCRIPTION  : Processes all instances in a class and then
 *                all subclasses of a class until done
 *  INPUTS       : 1) The module for which classes tested must be
 *                 in scope
 *              2) Visitation traversal id
 *              3) The class
 *              4) The current class restriction chain
 *              5) The index of the current restriction
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Instance variable values set
 *              Solution sets stored in global list
 *  NOTES        : None
 *****************************************************************/
static void TestEntireClass(void *theEnv, struct module_definition *module_def, int id, DEFCLASS *cls, QUERY_CLASS *qchain, int indx)
{
    long i;
    INSTANCE_TYPE *ins;
    core_data_object temp;

    if( TestTraversalID(cls->traversalRecord, id))
    {
        return;
    }

    SetTraversalID(cls->traversalRecord, id);

    if( DefclassInScope(theEnv, cls, module_def) == FALSE )
    {
        return;
    }

    ins = cls->instanceList;

    while( ins != NULL )
    {
        InstanceQueryData(theEnv)->QueryCore->solns[indx] = ins;

        if( qchain->nxt != NULL )
        {
            ins->busy++;
            TestEntireChain(theEnv, qchain->nxt, indx + 1);
            ins->busy--;

            if((core_get_evaluation_data(theEnv)->halt == TRUE) || (InstanceQueryData(theEnv)->AbortQuery == TRUE))
            {
                break;
            }
        }
        else
        {
            ins->busy++;
            core_get_evaluation_data(theEnv)->eval_depth++;
            core_eval_expression(theEnv, InstanceQueryData(theEnv)->QueryCore->query, &temp);
            core_get_evaluation_data(theEnv)->eval_depth--;
            core_gc_periodic_cleanup(theEnv, FALSE, TRUE);
            ins->busy--;

            if( core_get_evaluation_data(theEnv)->halt == TRUE )
            {
                break;
            }

            if((temp.type != ATOM) ? TRUE :
               (temp.value != get_false(theEnv)))
            {
                if( InstanceQueryData(theEnv)->QueryCore->action != NULL )
                {
                    ins->busy++;
                    core_get_evaluation_data(theEnv)->eval_depth++;
                    core_value_decrement(theEnv, InstanceQueryData(theEnv)->QueryCore->result);
                    core_eval_expression(theEnv, InstanceQueryData(theEnv)->QueryCore->action, InstanceQueryData(theEnv)->QueryCore->result);
                    core_value_increment(theEnv, InstanceQueryData(theEnv)->QueryCore->result);
                    core_get_evaluation_data(theEnv)->eval_depth--;
                    core_gc_periodic_cleanup(theEnv, FALSE, TRUE);
                    ins->busy--;

                    if( get_flow_control_data(theEnv)->break_flag || get_flow_control_data(theEnv)->return_flag )
                    {
                        InstanceQueryData(theEnv)->AbortQuery = TRUE;
                        break;
                    }

                    if( core_get_evaluation_data(theEnv)->halt == TRUE )
                    {
                        break;
                    }
                }
                else
                {
                    AddSolution(theEnv);
                }
            }
        }

        ins = ins->nxtClass;

        while((ins != NULL) ? (ins->garbage == 1) : FALSE )
        {
            ins = ins->nxtClass;
        }
    }

    if( ins != NULL )
    {
        return;
    }

    for( i = 0 ; i < cls->directSubclasses.classCount ; i++ )
    {
        TestEntireClass(theEnv, module_def, id, cls->directSubclasses.classArray[i], qchain, indx);

        if((core_get_evaluation_data(theEnv)->halt == TRUE) || (InstanceQueryData(theEnv)->AbortQuery == TRUE))
        {
            return;
        }
    }
}

/***************************************************************************
 *  NAME         : AddSolution
 *  DESCRIPTION  : Adds the current instance set to a global list of
 *                solutions
 *  INPUTS       : None
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Global list and count updated
 *  NOTES        : Solutions are stored as sequential arrays of INSTANCE_TYPE *
 ***************************************************************************/
static void AddSolution(void *theEnv)
{
    QUERY_SOLN *new_soln;
    register unsigned i;

    new_soln = (QUERY_SOLN *)core_mem_alloc_no_init(theEnv, (int)sizeof(QUERY_SOLN));
    new_soln->soln = (INSTANCE_TYPE **)
                     core_mem_alloc_no_init(theEnv, (sizeof(INSTANCE_TYPE *) * (InstanceQueryData(theEnv)->QueryCore->soln_size)));

    for( i = 0 ; i < InstanceQueryData(theEnv)->QueryCore->soln_size ; i++ )
    {
        new_soln->soln[i] = InstanceQueryData(theEnv)->QueryCore->solns[i];
    }

    new_soln->nxt = NULL;

    if( InstanceQueryData(theEnv)->QueryCore->soln_set == NULL )
    {
        InstanceQueryData(theEnv)->QueryCore->soln_set = new_soln;
    }
    else
    {
        InstanceQueryData(theEnv)->QueryCore->soln_bottom->nxt = new_soln;
    }

    InstanceQueryData(theEnv)->QueryCore->soln_bottom = new_soln;
    InstanceQueryData(theEnv)->QueryCore->soln_cnt++;
}

/***************************************************
 *  NAME         : PopQuerySoln
 *  DESCRIPTION  : Deallocates the topmost solution
 *                set for an instance-set query
 *  INPUTS       : None
 *  RETURNS      : Nothing useful
 *  SIDE EFFECTS : Solution set deallocated
 *  NOTES        : Assumes QueryCore->soln_set != 0
 ***************************************************/
static void PopQuerySoln(void *theEnv)
{
    InstanceQueryData(theEnv)->QueryCore->soln_bottom = InstanceQueryData(theEnv)->QueryCore->soln_set;
    InstanceQueryData(theEnv)->QueryCore->soln_set = InstanceQueryData(theEnv)->QueryCore->soln_set->nxt;
    core_mem_release(theEnv, (void *)InstanceQueryData(theEnv)->QueryCore->soln_bottom->soln,
       (sizeof(INSTANCE_TYPE *) * InstanceQueryData(theEnv)->QueryCore->soln_size));
    core_mem_release(theEnv, (void *)InstanceQueryData(theEnv)->QueryCore->soln_bottom, sizeof(QUERY_SOLN));
}

#endif
