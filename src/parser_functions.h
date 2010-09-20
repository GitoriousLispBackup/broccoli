#ifndef __PARSER_FUNCTIONS_H__
#define __PARSER_FUNCTIONS_H__

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef __PARSER_FUNCTIONS_SOURCE__
#define LOCALE
#else
#define LOCALE extern
#endif

LOCALE BOOLEAN parse_function(void *, char *);

#ifndef __PARSER_FUNCTIONS_SOURCE__
#endif

#endif
