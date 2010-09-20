#ifndef __FUNCS_COMPARISON_H__
#define __FUNCS_COMPARISON_H__

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef __FUNCS_COMPARISON_SOURCE__
#define LOCALE
#else
#define LOCALE extern
#endif

LOCALE void    func_init_comparisons(void *);
LOCALE BOOLEAN broccoli_equal(void *);
LOCALE BOOLEAN broccoli_not_equal(void *);
LOCALE BOOLEAN broccoli_less_than_or_equal(void *);
LOCALE BOOLEAN broccoli_greater_than_or_equal(void *);
LOCALE BOOLEAN broccoli_less_than(void *);
LOCALE BOOLEAN broccoli_greater_than(void *);


#define FUNC_NAME_EQUAL       "="
#define FUNC_CNSTR_EQUAL      "1*"
#define FUNC_NAME_NOT_EQUAL   "/="
#define FUNC_CNSTR_NOT_EQUAL  "1*"

#endif
