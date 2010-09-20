#ifndef _H_sortfun

#define _H_sortfun

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef _SORTFUN_SOURCE_
#define LOCALE
#else
#define LOCALE extern
#endif

LOCALE void init_sort_functions(void *);
LOCALE void broccoli_sort(void *, core_data_object *);

#endif
