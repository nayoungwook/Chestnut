#ifndef VM_H
#define VM_H

#include <stdbool.h>
#include <stdint.h>

struct VMClassData;
struct VMFunctionData;
struct IRReader;

enum VMOPType {
	OPRND_NULL = 0,
	OPRND_String = 1,
	OPRND_INT32 = 2,
	OPRND_FLOAT32 = 3,
	OPRND_FLOAT64 = 4,
};

struct VMOperand {
	enum VMOPType op_type;
	int64_t val;
};

struct VMStack {
	unsigned size;
	struct VMOperand stack[1024 * 256]; // 256 KB
	unsigned index;
};

void vm_stack_push(struct VMStack *vm_stack, struct VMOperand val);
struct VMOperand vm_stack_pop(struct VMStack *vm_stack);

struct VM {
	struct VMClassData **class_data;
	unsigned class_data_count, class_data_capacity;

	struct VMFunctionData **function_data;
	unsigned function_data_count, function_data_capacity;

	struct IRReader *ir_reader;

	unsigned main_func_id;

	void *heap, *stack;
	void *stack_pointer;
	char *stack_pointer_type;

	struct VMStack *vm_stack;
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

void vm_exec_function(struct VM *vm, struct VMFunctionData *function_data);

#endif
