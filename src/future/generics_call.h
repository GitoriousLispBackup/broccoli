#ifndef __GENERICS_CALL_H__
#define __GENERICS_CALL_H__

#if DEFGENERIC_CONSTRUCT

#include "funcs_generics.h"
#ifndef __EXPRESSIONS_H__
#include "core_expressions.h"
#endif
#ifndef __CORE_EVALUATION_H__
#include "core_evaluation.h"
#endif

#ifdef LOCALE
#undef LOCALE
#endif

#ifdef __GENERICS_CALL_SOURCE__
#define LOCALE
#else
#define LOCALE extern
#endif

LOCALE void    GenericDispatch(void *, DEFGENERIC *, DEFMETHOD *, DEFMETHOD *, core_expression_object *, core_data_object *);
LOCALE void    UnboundMethodErr(void *);
LOCALE BOOLEAN IsMethodApplicable(void *, DEFMETHOD *);

LOCALE int  NextMethodP(void *);
LOCALE void CallNextMethod(void *, core_data_object *);
LOCALE void CallSpecificMethod(void *, core_data_object *);
LOCALE void OverrideNextMethod(void *, core_data_object *);

LOCALE void GetGenericCurrentArgument(void *, core_data_object *);

#ifndef __GENERICS_CALL_SOURCE__
#endif

#endif

#endif
