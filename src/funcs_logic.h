#ifndef __FUNCS_LOGIC_H__
#define __FUNCS_LOGIC_H__

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef __FUNCS_LOGIC_SOURCE__
#define LOCALE
#else
#define LOCALE extern
#endif

LOCALE void    init_logic_functions(void *);
LOCALE BOOLEAN broccoli_not(void *);
LOCALE BOOLEAN broccoli_and(void *);
LOCALE BOOLEAN broccoli_or(void *);

#define FUNC_NAME_AND               "and"
#define FUNC_CNSTR_AND              "0*"
#define FUNC_NAME_OR                "or"
#define FUNC_CNSTR_OR               "0*"
#define FUNC_NAME_NOT               "not"
#define FUNC_CNSTR_NOT              "11"


#endif
