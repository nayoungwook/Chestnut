#ifndef VM_H
#define VM_H

#include <stdbool.h>

struct VMClassData;
struct VMFunctionData;

struct VM {
	struct VMClassData **class_data;
	unsigned class_data_count, class_data_capacity;

	struct VMFunctionData **function_data;
	unsigned function_data_count, function_data_capacity;

	unsigned main_func_id;
};

struct VM *gen_vm();
struct VMClassData *vm_add_class_data(struct VM *vm, unsigned id,
				 const char *name, unsigned parent_id,
				 unsigned size);
struct VMFunctionData *vm_add_function_data(struct VM *vm,
				       struct VMClassData *owner,
				       unsigned id, const char *name,
				       const char *return_type,
				       const char **argument_types,
				       unsigned argument_count,
				       bool is_constructor);
struct VMClassData *vm_find_class_data(const struct VM *vm, unsigned id);
struct VMFunctionData *vm_find_function_data(const struct VM *vm,
					struct VMClassData *owner,
					unsigned id);
void vm_set_function_code(struct VMFunctionData *function_data,
		       const unsigned char *code, unsigned code_size);

void vm_exec_function(const struct VM* vm, struct VMFunctionData *function_data);

#endif
