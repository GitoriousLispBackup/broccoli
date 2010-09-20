/* Purpose: Contains the code for file commands including
 *   batch, dribble-on, dribble-off, save, load, bsave, and
 *   bload.                                                  */

#ifndef _H_filecom

#define _H_filecom

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef __FUNCS_IO_BASIC_SOURCE__
#define LOCALE
#else
#define LOCALE extern
#endif

LOCALE void    init_io_functions(void *);
LOCALE int     io_get_ch_from_batch(void *, char *, int);
LOCALE int     io_open_batch(void *, char *, int);
LOCALE int     io_pop_batch(void *);
LOCALE BOOLEAN io_is_batching(void *);
LOCALE void    io_close_all_batches(void *);
LOCALE int     broccoli_run(void *);
LOCALE int     broccoli_run_silent(void *, char *);
LOCALE int     broccoli_import(void *);
LOCALE void    init_io_all_functions(void *);
LOCALE void    broccoli_print(void *);

#if IO_FUNCTIONS
LOCALE BOOLEAN SetFullCRLF(void *, BOOLEAN);
LOCALE void ReadFunction(void *, core_data_object_ptr);
LOCALE int OpenFunction(void *);
LOCALE int CloseFunction(void *);
LOCALE int GetCharFunction(void *);
LOCALE void ReadlineFunction(void *, core_data_object_ptr);
LOCALE void                          * FormatFunction(void *);
LOCALE int                             RemoveFunction(void *);
LOCALE int                             RenameFunction(void *);
LOCALE void SetLocaleFunction(void *, core_data_object_ptr);
LOCALE void ReadNumberFunction(void *, core_data_object_ptr);
#endif

#endif
