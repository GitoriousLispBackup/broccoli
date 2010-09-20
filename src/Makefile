OBJS = main.o sysdep.o \
	\
	core_command_prompt.o core_arguments.o core_constructs.o core_constructs_query.o \
	core_environment.o core_evaluation.o core_expressions.o core_expressions_operators.o \
	core_functions.o core_memory.o core_pretty_print.o core_functions_util.o core_utilities.o \
	core_scanner.o core_gc.o core_watch.o \
	\
	funcs_io_basic.o funcs_math_basic.o funcs_meta.o funcs_misc.o funcs_sorting.o \
	funcs_predicate.o funcs_flow_control.o funcs_logic.o funcs_comparison.o \
	funcs_function.o \
	funcs_list.o funcs_string.o \
	\
	parser_constructs.o parser_constraints.o parser_expressions.o \
	parser_functions.o \
	parser_flow_control.o \
	parser_modules.o \
	\
	modules_kernel.o modules_init.o modules_query.o \
	\
	functions_kernel.o \
	\
	constraints_kernel.o constraints_query.o constraints_operators.o \
	\
 	extensions.o extensions_developer.o extensions_data.o \
 	\
 	router.o router_file.o router_string.o \
 	\
 	type_symbol.o type_list.o \

.c.o :
	gcc -c -Os -Wall  -Wundef -Wpointer-arith -Wshadow -Wcast-qual \
	    -Wcast-align -Winline -Wmissing-declarations -Wredundant-decls \
	    -Wmissing-prototypes -Wnested-externs \
	    -Wstrict-prototypes -Waggregate-return -Wno-implicit $<

broccoli : $(OBJS)
	gcc -o broccoli $(OBJS) -lm
	. _test.sh

# Dependencies generated using "gcc -MM *.c"

constraints_kernel.o: constraints_kernel.c setup.h core_environment.h \
  type_symbol.h extensions.h core_evaluation.h constant.h \
  core_expressions.h core_expressions_operators.h parser_expressions.h \
  core_functions.h extensions_data.h core_scanner.h core_pretty_print.h \
  core_arguments.h modules_init.h parser_modules.h core_gc.h \
  core_constructs.h core_memory.h type_list.h router.h core_utilities.h \
  constraints_kernel.h
constraints_operators.o: constraints_operators.c setup.h \
  core_environment.h type_symbol.h extensions.h core_evaluation.h \
  constant.h core_expressions.h core_expressions_operators.h \
  parser_expressions.h core_functions.h extensions_data.h core_scanner.h \
  core_pretty_print.h core_memory.h router.h core_utilities.h \
  modules_init.h parser_modules.h core_gc.h core_constructs.h type_list.h \
  constraints_kernel.h constraints_query.h constraints_operators.h
constraints_query.o: constraints_query.c setup.h core_environment.h \
  type_symbol.h extensions.h core_evaluation.h constant.h \
  core_expressions.h core_expressions_operators.h parser_expressions.h \
  core_functions.h extensions_data.h core_scanner.h core_pretty_print.h \
  router.h core_utilities.h modules_init.h parser_modules.h core_gc.h \
  core_constructs.h type_list.h constraints_operators.h \
  constraints_kernel.h constraints_query.h
core_arguments.o: core_arguments.c setup.h core_environment.h \
  type_symbol.h extensions.h core_evaluation.h constant.h \
  core_expressions.h core_expressions_operators.h parser_expressions.h \
  core_functions.h extensions_data.h core_scanner.h core_pretty_print.h \
  router.h core_utilities.h modules_init.h parser_modules.h core_gc.h \
  core_constructs.h constraints_query.h constraints_kernel.h sysdep.h \
  core_arguments.h
core_command_prompt.o: core_command_prompt.c setup.h core_environment.h \
  type_symbol.h extensions.h core_evaluation.h constant.h \
  core_expressions.h core_expressions_operators.h parser_expressions.h \
  core_functions.h extensions_data.h core_scanner.h core_pretty_print.h \
  core_arguments.h modules_init.h parser_modules.h core_gc.h \
  core_constructs.h parser_constructs.h funcs_io_basic.h core_memory.h \
  funcs_flow_control.h parser_flow_control.h constraints_kernel.h \
  router.h core_utilities.h router_string.h sysdep.h \
  core_command_prompt.h
core_constructs.o: core_constructs.c setup.h core_environment.h \
  type_symbol.h extensions.h core_evaluation.h constant.h \
  core_expressions.h core_expressions_operators.h parser_expressions.h \
  core_functions.h extensions_data.h core_scanner.h core_pretty_print.h \
  core_memory.h router.h core_utilities.h modules_init.h parser_modules.h \
  core_gc.h core_constructs.h core_watch.h funcs_flow_control.h \
  parser_flow_control.h constraints_kernel.h core_arguments.h type_list.h \
  sysdep.h core_command_prompt.h
core_constructs_query.o: core_constructs_query.c setup.h \
  core_environment.h type_symbol.h extensions.h core_evaluation.h \
  constant.h core_expressions.h core_expressions_operators.h \
  parser_expressions.h core_functions.h extensions_data.h core_scanner.h \
  core_pretty_print.h core_memory.h modules_init.h parser_modules.h \
  core_gc.h core_constructs.h core_arguments.h type_list.h \
  modules_query.h router.h core_utilities.h core_command_prompt.h \
  sysdep.h parser_constructs.h core_constructs_query.h
core_environment.o: core_environment.c setup.h core_environment.h \
  type_symbol.h extensions.h core_evaluation.h constant.h \
  core_expressions.h core_expressions_operators.h parser_expressions.h \
  core_functions.h extensions_data.h core_scanner.h core_pretty_print.h \
  core_memory.h core_utilities.h modules_init.h parser_modules.h \
  core_gc.h core_constructs.h router.h sysdep.h
core_evaluation.o: core_evaluation.c setup.h core_environment.h \
  type_symbol.h extensions.h core_evaluation.h constant.h \
  core_expressions.h core_expressions_operators.h parser_expressions.h \
  core_functions.h extensions_data.h core_scanner.h core_pretty_print.h \
  core_arguments.h modules_init.h parser_modules.h core_gc.h \
  core_constructs.h core_command_prompt.h core_memory.h router.h \
  core_utilities.h funcs_flow_control.h type_list.h funcs_profiling.h \
  sysdep.h funcs_function.h core_constructs_query.h
core_expressions.o: core_expressions.c setup.h core_environment.h \
  type_symbol.h extensions.h core_evaluation.h constant.h \
  core_expressions.h core_expressions_operators.h parser_expressions.h \
  core_functions.h extensions_data.h core_scanner.h core_pretty_print.h \
  core_memory.h router.h core_utilities.h modules_init.h parser_modules.h \
  core_gc.h core_constructs.h funcs_comparison.h funcs_logic.h
core_expressions_operators.o: core_expressions_operators.c setup.h \
  core_environment.h type_symbol.h extensions.h core_evaluation.h \
  constant.h core_expressions.h core_expressions_operators.h \
  parser_expressions.h core_functions.h extensions_data.h core_scanner.h \
  core_pretty_print.h core_memory.h router.h core_utilities.h \
  modules_init.h parser_modules.h core_gc.h core_constructs.h \
  constraints_query.h constraints_kernel.h constraints_operators.h
core_functions.o: core_functions.c setup.h core_environment.h \
  type_symbol.h extensions.h core_evaluation.h constant.h \
  core_expressions.h core_expressions_operators.h parser_expressions.h \
  core_functions.h extensions_data.h core_scanner.h core_pretty_print.h \
  router.h core_utilities.h modules_init.h parser_modules.h core_gc.h \
  core_constructs.h core_memory.h
core_functions_util.o: core_functions_util.c setup.h core_environment.h \
  type_symbol.h extensions.h core_evaluation.h constant.h \
  core_expressions.h core_expressions_operators.h parser_expressions.h \
  core_functions.h extensions_data.h core_scanner.h core_pretty_print.h \
  core_memory.h type_list.h parser_flow_control.h constraints_kernel.h \
  router.h core_utilities.h modules_init.h parser_modules.h core_gc.h \
  core_constructs.h core_functions_util.h
core_gc.o: core_gc.c setup.h core_environment.h type_symbol.h \
  extensions.h core_evaluation.h constant.h core_expressions.h \
  core_expressions_operators.h parser_expressions.h core_functions.h \
  extensions_data.h core_scanner.h core_pretty_print.h core_memory.h \
  type_list.h core_utilities.h modules_init.h parser_modules.h core_gc.h \
  core_constructs.h sysdep.h
core_memory.o: core_memory.c setup.h core_environment.h type_symbol.h \
  extensions.h core_evaluation.h constant.h core_expressions.h \
  core_expressions_operators.h parser_expressions.h core_functions.h \
  extensions_data.h core_scanner.h core_pretty_print.h core_memory.h \
  router.h core_utilities.h modules_init.h parser_modules.h core_gc.h \
  core_constructs.h
core_pretty_print.o: core_pretty_print.c setup.h core_environment.h \
  type_symbol.h extensions.h core_evaluation.h constant.h \
  core_expressions.h core_expressions_operators.h parser_expressions.h \
  core_functions.h extensions_data.h core_scanner.h core_pretty_print.h \
  core_memory.h sysdep.h core_gc.h
core_scanner.o: core_scanner.c setup.h core_environment.h type_symbol.h \
  extensions.h core_evaluation.h constant.h core_expressions.h \
  core_expressions_operators.h parser_expressions.h core_functions.h \
  extensions_data.h core_scanner.h core_pretty_print.h router.h \
  core_utilities.h modules_init.h parser_modules.h core_gc.h \
  core_constructs.h core_memory.h sysdep.h
core_utilities.o: core_utilities.c setup.h core_environment.h \
  type_symbol.h extensions.h core_evaluation.h constant.h \
  core_expressions.h core_expressions_operators.h parser_expressions.h \
  core_functions.h extensions_data.h core_scanner.h core_pretty_print.h \
  core_gc.h core_arguments.h modules_init.h parser_modules.h \
  core_constructs.h router.h core_utilities.h funcs_list.h core_memory.h \
  sysdep.h
core_watch.o: core_watch.c setup.h core_environment.h type_symbol.h \
  extensions.h core_evaluation.h constant.h core_expressions.h \
  core_expressions_operators.h parser_expressions.h core_functions.h \
  extensions_data.h core_scanner.h core_pretty_print.h
extensions.o: extensions.c broccoli.h setup.h core_environment.h \
  type_symbol.h extensions.h core_evaluation.h constant.h \
  core_expressions.h core_expressions_operators.h parser_expressions.h \
  core_functions.h extensions_data.h core_scanner.h core_pretty_print.h \
  core_arguments.h modules_init.h parser_modules.h core_gc.h \
  core_constructs.h core_memory.h parser_constructs.h funcs_io_basic.h \
  funcs_string.h core_command_prompt.h router.h core_utilities.h \
  router_file.h router_string.h sysdep.h funcs_math_basic.h core_watch.h \
  modules_kernel.h funcs_function.h core_constructs_query.h
extensions_data.o: extensions_data.c setup.h core_environment.h \
  type_symbol.h extensions.h core_evaluation.h constant.h \
  core_expressions.h core_expressions_operators.h parser_expressions.h \
  core_functions.h extensions_data.h core_scanner.h core_pretty_print.h
extensions_developer.o: extensions_developer.c setup.h core_environment.h \
  type_symbol.h extensions.h core_evaluation.h constant.h \
  core_expressions.h core_expressions_operators.h parser_expressions.h \
  core_functions.h extensions_data.h core_scanner.h core_pretty_print.h \
  core_arguments.h modules_init.h parser_modules.h core_gc.h \
  core_constructs.h modules_query.h router.h core_utilities.h \
  extensions_developer.h
funcs_comparison.o: funcs_comparison.c setup.h core_environment.h \
  type_symbol.h extensions.h core_evaluation.h constant.h \
  core_expressions.h core_expressions_operators.h parser_expressions.h \
  core_functions.h extensions_data.h core_scanner.h core_pretty_print.h \
  core_arguments.h modules_init.h parser_modules.h core_gc.h \
  core_constructs.h type_list.h router.h core_utilities.h \
  funcs_comparison.h
funcs_flow_control.o: funcs_flow_control.c setup.h core_environment.h \
  type_symbol.h extensions.h core_evaluation.h constant.h \
  core_expressions.h core_expressions_operators.h parser_expressions.h \
  core_functions.h extensions_data.h core_scanner.h core_pretty_print.h \
  core_arguments.h modules_init.h parser_modules.h core_gc.h \
  core_constructs.h constraints_kernel.h constraints_query.h \
  constraints_operators.h core_memory.h type_list.h parser_flow_control.h \
  router.h core_utilities.h funcs_flow_control.h
funcs_function.o: funcs_function.c setup.h core_environment.h \
  type_symbol.h extensions.h core_evaluation.h constant.h \
  core_expressions.h core_expressions_operators.h parser_expressions.h \
  core_functions.h extensions_data.h core_scanner.h core_pretty_print.h \
  core_constructs.h modules_init.h parser_modules.h core_gc.h \
  parser_constructs.h parser_functions.h functions_kernel.h \
  funcs_function.h core_constructs_query.h core_arguments.h core_memory.h \
  router.h core_utilities.h
funcs_io_basic.o: funcs_io_basic.c setup.h core_environment.h \
  type_symbol.h extensions.h core_evaluation.h constant.h \
  core_expressions.h core_expressions_operators.h parser_expressions.h \
  core_functions.h extensions_data.h core_scanner.h core_pretty_print.h \
  core_arguments.h modules_init.h parser_modules.h core_gc.h \
  core_constructs.h core_command_prompt.h parser_constructs.h \
  core_memory.h funcs_flow_control.h router.h core_utilities.h \
  router_string.h sysdep.h router_file.h funcs_io_basic.h
funcs_list.o: funcs_list.c setup.h core_environment.h type_symbol.h \
  extensions.h core_evaluation.h constant.h core_expressions.h \
  core_expressions_operators.h parser_expressions.h core_functions.h \
  extensions_data.h core_scanner.h core_pretty_print.h core_arguments.h \
  modules_init.h parser_modules.h core_gc.h core_constructs.h \
  core_memory.h type_list.h funcs_list.h parser_flow_control.h \
  constraints_kernel.h funcs_flow_control.h router.h core_utilities.h
funcs_logic.o: funcs_logic.c setup.h core_environment.h type_symbol.h \
  extensions.h core_evaluation.h constant.h core_expressions.h \
  core_expressions_operators.h parser_expressions.h core_functions.h \
  extensions_data.h core_scanner.h core_pretty_print.h core_arguments.h \
  modules_init.h parser_modules.h core_gc.h core_constructs.h type_list.h \
  router.h core_utilities.h funcs_logic.h
funcs_math_basic.o: funcs_math_basic.c setup.h core_environment.h \
  type_symbol.h extensions.h core_evaluation.h constant.h \
  core_expressions.h core_expressions_operators.h parser_expressions.h \
  core_functions.h extensions_data.h core_scanner.h core_pretty_print.h \
  core_arguments.h modules_init.h parser_modules.h core_gc.h \
  core_constructs.h router.h core_utilities.h funcs_math_basic.h
funcs_meta.o: funcs_meta.c setup.h core_environment.h type_symbol.h \
  extensions.h core_evaluation.h constant.h core_expressions.h \
  core_expressions_operators.h parser_expressions.h core_functions.h \
  extensions_data.h core_scanner.h core_pretty_print.h core_arguments.h \
  modules_init.h parser_modules.h core_gc.h core_constructs.h \
  core_memory.h type_list.h router.h core_utilities.h sysdep.h \
  router_string.h parser_flow_control.h constraints_kernel.h \
  core_command_prompt.h funcs_function.h core_constructs_query.h \
  funcs_meta.h
funcs_misc.o: funcs_misc.c setup.h core_environment.h type_symbol.h \
  extensions.h core_evaluation.h constant.h core_expressions.h \
  core_expressions_operators.h parser_expressions.h core_functions.h \
  extensions_data.h core_scanner.h core_pretty_print.h core_arguments.h \
  modules_init.h parser_modules.h core_gc.h core_constructs.h \
  core_memory.h type_list.h router.h core_utilities.h sysdep.h \
  funcs_function.h core_constructs_query.h funcs_misc.h
funcs_predicate.o: funcs_predicate.c setup.h core_environment.h \
  type_symbol.h extensions.h core_evaluation.h constant.h \
  core_expressions.h core_expressions_operators.h parser_expressions.h \
  core_functions.h extensions_data.h core_scanner.h core_pretty_print.h \
  core_arguments.h modules_init.h parser_modules.h core_gc.h \
  core_constructs.h type_list.h router.h core_utilities.h \
  funcs_predicate.h
funcs_profiling.o: funcs_profiling.c setup.h core_environment.h \
  type_symbol.h extensions.h core_evaluation.h constant.h \
  core_expressions.h core_expressions_operators.h parser_expressions.h \
  core_functions.h extensions_data.h core_scanner.h core_pretty_print.h
funcs_sorting.o: funcs_sorting.c setup.h core_environment.h type_symbol.h \
  extensions.h core_evaluation.h constant.h core_expressions.h \
  core_expressions_operators.h parser_expressions.h core_functions.h \
  extensions_data.h core_scanner.h core_pretty_print.h core_arguments.h \
  modules_init.h parser_modules.h core_gc.h core_constructs.h \
  funcs_function.h core_constructs_query.h core_memory.h type_list.h \
  sysdep.h funcs_sorting.h
funcs_string.o: funcs_string.c setup.h core_environment.h type_symbol.h \
  extensions.h core_evaluation.h constant.h core_expressions.h \
  core_expressions_operators.h parser_expressions.h core_functions.h \
  extensions_data.h core_scanner.h core_pretty_print.h
functions_kernel.o: functions_kernel.c setup.h core_environment.h \
  type_symbol.h extensions.h core_evaluation.h constant.h \
  core_expressions.h core_expressions_operators.h parser_expressions.h \
  core_functions.h extensions_data.h core_scanner.h core_pretty_print.h \
  core_constructs.h modules_init.h parser_modules.h core_gc.h \
  funcs_function.h core_constructs_query.h core_functions_util.h \
  funcs_profiling.h router.h core_utilities.h core_watch.h \
  funcs_flow_control.h functions_kernel.h
main.o: main.c setup.h core_environment.h type_symbol.h extensions.h \
  core_evaluation.h constant.h core_expressions.h \
  core_expressions_operators.h parser_expressions.h core_functions.h \
  extensions_data.h core_scanner.h core_pretty_print.h broccoli.h \
  core_arguments.h modules_init.h parser_modules.h core_gc.h \
  core_constructs.h core_memory.h parser_constructs.h funcs_io_basic.h \
  funcs_string.h core_command_prompt.h router.h core_utilities.h \
  router_file.h router_string.h sysdep.h funcs_math_basic.h core_watch.h \
  modules_kernel.h funcs_function.h core_constructs_query.h
modules_init.o: modules_init.c setup.h core_environment.h type_symbol.h \
  extensions.h core_evaluation.h constant.h core_expressions.h \
  core_expressions_operators.h parser_expressions.h core_functions.h \
  extensions_data.h core_scanner.h core_pretty_print.h core_memory.h \
  router.h core_utilities.h modules_init.h parser_modules.h core_gc.h \
  core_constructs.h core_arguments.h modules_kernel.h
modules_kernel.o: modules_kernel.c setup.h core_environment.h \
  type_symbol.h extensions.h core_evaluation.h constant.h \
  core_expressions.h core_expressions_operators.h parser_expressions.h \
  core_functions.h extensions_data.h core_scanner.h core_pretty_print.h \
  core_constructs.h modules_init.h parser_modules.h core_gc.h \
  core_utilities.h router.h core_arguments.h modules_kernel.h
modules_query.o: modules_query.c setup.h core_environment.h type_symbol.h \
  extensions.h core_evaluation.h constant.h core_expressions.h \
  core_expressions_operators.h parser_expressions.h core_functions.h \
  extensions_data.h core_scanner.h core_pretty_print.h core_memory.h \
  router.h core_utilities.h modules_init.h parser_modules.h core_gc.h \
  core_constructs.h sysdep.h modules_query.h
parser_constraints.o: parser_constraints.c setup.h core_environment.h \
  type_symbol.h extensions.h core_evaluation.h constant.h \
  core_expressions.h core_expressions_operators.h parser_expressions.h \
  core_functions.h extensions_data.h core_scanner.h core_pretty_print.h \
  core_memory.h router.h core_utilities.h modules_init.h parser_modules.h \
  core_gc.h core_constructs.h constraints_operators.h \
  constraints_kernel.h constraints_query.h sysdep.h parser_constraints.h
parser_constructs.o: parser_constructs.c setup.h core_environment.h \
  type_symbol.h extensions.h core_evaluation.h constant.h \
  core_expressions.h core_expressions_operators.h parser_expressions.h \
  core_functions.h extensions_data.h core_scanner.h core_pretty_print.h \
  router.h core_utilities.h modules_init.h parser_modules.h core_gc.h \
  core_constructs.h core_watch.h parser_flow_control.h \
  constraints_kernel.h modules_query.h sysdep.h parser_constructs.h
parser_expressions.o: parser_expressions.c setup.h core_environment.h \
  type_symbol.h extensions.h core_evaluation.h constant.h \
  core_expressions.h core_expressions_operators.h parser_expressions.h \
  core_functions.h extensions_data.h core_scanner.h core_pretty_print.h \
  router.h core_utilities.h modules_init.h parser_modules.h core_gc.h \
  core_constructs.h router_string.h core_memory.h core_arguments.h \
  constraints_query.h constraints_kernel.h modules_query.h \
  funcs_flow_control.h funcs_function.h core_constructs_query.h
parser_flow_control.o: parser_flow_control.c setup.h core_environment.h \
  type_symbol.h extensions.h core_evaluation.h constant.h \
  core_expressions.h core_expressions_operators.h parser_expressions.h \
  core_functions.h extensions_data.h core_scanner.h core_pretty_print.h \
  core_arguments.h modules_init.h parser_modules.h core_gc.h \
  core_constructs.h constraints_kernel.h constraints_query.h \
  constraints_operators.h core_memory.h modules_query.h type_list.h \
  router.h core_utilities.h parser_flow_control.h
parser_functions.o: parser_functions.c setup.h core_environment.h \
  type_symbol.h extensions.h core_evaluation.h constant.h \
  core_expressions.h core_expressions_operators.h parser_expressions.h \
  core_functions.h extensions_data.h core_scanner.h core_pretty_print.h \
  parser_constraints.h constraints_kernel.h core_constructs.h \
  modules_init.h parser_modules.h core_gc.h funcs_function.h \
  core_constructs_query.h core_memory.h core_functions_util.h router.h \
  core_utilities.h parser_functions.h
parser_modules.o: parser_modules.c setup.h core_environment.h \
  type_symbol.h extensions.h core_evaluation.h constant.h \
  core_expressions.h core_expressions_operators.h parser_expressions.h \
  core_functions.h extensions_data.h core_scanner.h core_pretty_print.h
router.o: router.c setup.h core_environment.h type_symbol.h extensions.h \
  core_evaluation.h constant.h core_expressions.h \
  core_expressions_operators.h parser_expressions.h core_functions.h \
  extensions_data.h core_scanner.h core_pretty_print.h core_arguments.h \
  modules_init.h parser_modules.h core_gc.h core_constructs.h \
  router_file.h core_memory.h router_string.h sysdep.h router.h \
  core_utilities.h
router_file.o: router_file.c setup.h core_environment.h type_symbol.h \
  extensions.h core_evaluation.h constant.h core_expressions.h \
  core_expressions_operators.h parser_expressions.h core_functions.h \
  extensions_data.h core_scanner.h core_pretty_print.h core_memory.h \
  router.h core_utilities.h modules_init.h parser_modules.h core_gc.h \
  core_constructs.h sysdep.h router_file.h
router_string.o: router_string.c setup.h core_environment.h type_symbol.h \
  extensions.h core_evaluation.h constant.h core_expressions.h \
  core_expressions_operators.h parser_expressions.h core_functions.h \
  extensions_data.h core_scanner.h core_pretty_print.h core_memory.h \
  router.h core_utilities.h modules_init.h parser_modules.h core_gc.h \
  core_constructs.h sysdep.h router_string.h
sysdep.o: sysdep.c setup.h core_environment.h type_symbol.h extensions.h \
  core_evaluation.h constant.h core_expressions.h \
  core_expressions_operators.h parser_expressions.h core_functions.h \
  extensions_data.h core_scanner.h core_pretty_print.h core_arguments.h \
  modules_init.h parser_modules.h core_gc.h core_constructs.h \
  funcs_math_basic.h core_command_prompt.h constraints_kernel.h \
  parser_constructs.h funcs_io_basic.h core_memory.h funcs_misc.h \
  type_list.h funcs_list.h core_functions_util.h funcs_predicate.h \
  funcs_comparison.h funcs_logic.h funcs_profiling.h funcs_flow_control.h \
  router.h core_utilities.h funcs_sorting.h funcs_string.h core_watch.h \
  sysdep.h funcs_function.h core_constructs_query.h funcs_meta.h
type_list.o: type_list.c setup.h core_environment.h type_symbol.h \
  extensions.h core_evaluation.h constant.h core_expressions.h \
  core_expressions_operators.h parser_expressions.h core_functions.h \
  extensions_data.h core_scanner.h core_pretty_print.h core_memory.h \
  router.h core_utilities.h modules_init.h parser_modules.h core_gc.h \
  core_constructs.h router_string.h type_list.h
type_symbol.o: type_symbol.c setup.h core_environment.h type_symbol.h \
  extensions.h core_evaluation.h constant.h core_expressions.h \
  core_expressions_operators.h parser_expressions.h core_functions.h \
  extensions_data.h core_scanner.h core_pretty_print.h core_memory.h \
  router.h core_utilities.h modules_init.h parser_modules.h core_gc.h \
  core_constructs.h core_arguments.h sysdep.h
