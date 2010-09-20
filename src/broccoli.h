#ifndef _H_API
#define _H_API

#include "setup.h"
#ifndef __ARGUMENTS_H__
#include "core_arguments.h"
#endif
#include "constant.h"
#include "core_memory.h"
#include "parser_constructs.h"
#include "funcs_io_basic.h"
#include "funcs_string.h"
#include "core_environment.h"
#include "core_command_prompt.h"
#ifndef __TYPE_ATOM_H__
#include "type_symbol.h"
#endif

#include "router.h"
#include "router_file.h"
#include "router_string.h"

#include "sysdep.h"
#include "funcs_math_basic.h"
#ifndef __EXPRESSIONS_H__
#include "core_expressions.h"
#endif
#include "parser_expressions.h"
#ifndef __CORE_EVALUATION_H__
#include "core_evaluation.h"
#endif
#ifndef __CORE_CONSTRUCTS_H__
#include "core_constructs.h"
#endif
#include "core_gc.h"
#include "core_watch.h"
#include "modules_kernel.h"

#if DEFFUNCTION_CONSTRUCT
#include "funcs_function.h"
#endif

#if DEFGENERIC_CONSTRUCT
#include "generics_kernel.h"
#include "funcs_generics.h"
#endif

#if OBJECT_SYSTEM
#include "classes_kernel.h"
#include "classexm.h"
#include "classinf.h"
#include "classini.h"
#include "defins.h"
#include "inscom.h"
#include "insfile.h"
#include "funcs_instance.h"
#include "msgcom.h"
#include "msgpass.h"
#endif


#endif
