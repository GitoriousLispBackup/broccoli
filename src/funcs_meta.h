#ifndef __FUNCS_META_H__
#define __FUNCS_META_H__

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef _META_SOURCE_
#define LOCALE
#else
#define LOCALE extern
#endif

LOCALE void init_meta_functions(void *);
LOCALE void broccoli_call(void *, core_data_object *);
LOCALE void broccoli_help(void *, core_data_object *);
LOCALE void broccoli_eval(void *, core_data_object_ptr);

#endif
