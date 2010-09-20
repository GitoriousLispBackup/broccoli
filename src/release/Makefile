OBJS = main.o sysdep.o \
	\
	core_command_prompt.o core_arguments.o core_constructs.o core_constructs_query.o \
	core_environment.o core_evaluation.o core_expressions.o core_expressions_operators.o \
	core_functions.o core_memory.o core_pretty_print.o core_functions_util.o \
	core_utilities.o core_scanner.o core_gc.o core_watch.o \
	\
	funcs_io_basic.o funcs_math_basic.o funcs_meta.o funcs_misc.o funcs_sorting.o \
	funcs_predicate.o funcs_flow_control.o funcs_logic.o funcs_comparison.o \
	funcs_class.o funcs_function.o funcs_generics.o funcs_instance.o funcs_method.o \
	funcs_list.o funcs_string.o \
	\
	parser_class.o parser_class_members.o parser_constructs.o parser_constraints.o \
	parser_functions.o parser_generics.o parser_generics_implicit.o parser_class_inheritance.o \
	parser_class_instances.o parser_expressions.o parser_class_methods.o parser_flow_control.o \
	parser_modules.o parser_class_query.o \
	\
	modules_kernel.o modules_init.o modules_query.o \
	\
	functions_kernel.o generics_kernel.o generics_call.o \
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

vodka : $(OBJS)
	gcc -o vodka $(OBJS) -lm 

# Dependencies generated using "gcc -MM *.c"
