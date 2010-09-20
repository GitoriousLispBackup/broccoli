#ifndef _H_miscfun

#define _H_miscfun

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef _META_SOURCE_
#define LOCALE
#else
#define LOCALE extern
#endif

LOCALE void   init_misc_functions(void *);
LOCALE void   broccoli_expand(void *, core_data_object *);
LOCALE double broccoli_bench(void *);

#endif
