#include <vm.h>
#include <code_data.h>
#include <util.h>

#include <string.h>

static const char *copy_string(const char *value) {
	size_t size = strlen(value) + 1;
	char *copy = (char *)S_malloc(size);
	memcpy(copy, value, size);
	return copy;
}

struct VM *gen_vm() {
	struct VM *vm = (struct VM *)S_malloc(sizeof(struct VM));

	vm->class_data_count = 0;
	vm->class_data_capacity = 1;
	vm->class_data =
		(struct VMClassData **)S_malloc(sizeof(struct VMClassData *));

	vm->function_data_count = 0;
	vm->function_data_capacity = 1;
	vm->function_data =
		(struct VMFunctionData **)S_malloc(sizeof(struct VMFunctionData *));

	return vm;
}

struct VMClassData *vm_add_class_data(struct VM *vm, unsigned id,
				 const char *name, unsigned parent_id,
				 unsigned size) {
	struct VMClassData *class_data;

	if (vm->class_data_count == vm->class_data_capacity) {
		vm->class_data_capacity *= 2;
		vm->class_data = (struct VMClassData **)S_realloc(
			vm->class_data,
			sizeof(struct VMClassData *) * vm->class_data_capacity);
	}

	class_data =
		(struct VMClassData *)S_malloc(sizeof(struct VMClassData));
	class_data->id = id;
	class_data->name = copy_string(name);
	class_data->parent_id = parent_id;
	class_data->size = size;
	class_data->function_data_count = 0;
	class_data->function_data_capacity = 1;
	class_data->function_data = (FunctionData **)S_malloc(
		sizeof(FunctionData *));

	vm->class_data[vm->class_data_count++] = class_data;
	return class_data;
}

struct VMFunctionData *vm_add_function_data(struct VM *vm,
				       struct VMClassData *owner,
				       unsigned id, const char *name,
				       const char *return_type,
				       const char **argument_types,
				       unsigned argument_count,
				       bool is_constructor) {
	struct VMFunctionData *function_data;
	unsigned i;

	function_data =
		(struct VMFunctionData *)S_malloc(sizeof(struct VMFunctionData));
	function_data->id = id;
	function_data->name = copy_string(name);
	function_data->return_type = copy_string(return_type);
	function_data->argument_count = argument_count;
	function_data->is_constructor = is_constructor;
	function_data->code = NULL;
	function_data->code_size = 0;

	function_data->argument_types = argument_count == 0 ? NULL :
		(const char **)S_malloc(sizeof(char *) * argument_count);
	for (i = 0; i < argument_count; i++)
		function_data->argument_types[i] = copy_string(argument_types[i]);

	if (owner == NULL) {
		if (vm->function_data_count == vm->function_data_capacity) {
			vm->function_data_capacity *= 2;
			vm->function_data = (struct VMFunctionData **)S_realloc(
				vm->function_data, sizeof(struct VMFunctionData *) *
				vm->function_data_capacity);
		}
		vm->function_data[vm->function_data_count++] = function_data;
	} else {
		if (owner->function_data_count ==
		    owner->function_data_capacity) {
			owner->function_data_capacity *= 2;
			owner->function_data = (FunctionData **)S_realloc(
				owner->function_data, sizeof(FunctionData *) *
				owner->function_data_capacity);
		}
		owner->function_data[owner->function_data_count++] =
			function_data;
	}
	return function_data;
}

struct VMClassData *vm_find_class_data(const struct VM *vm, unsigned id) {
	unsigned i;

	for (i = 0; i < vm->class_data_count; i++)
		if (vm->class_data[i]->id == id)
			return vm->class_data[i];
	return NULL;
}

struct VMFunctionData *vm_find_function_data(const struct VM *vm,
					struct VMClassData *owner,
					unsigned id) {
	struct VMFunctionData **items = owner == NULL ? vm->function_data :
		owner->function_data;
	unsigned count = owner == NULL ? vm->function_data_count :
		owner->function_data_count;
	unsigned i;

	for (i = 0; i < count; i++)
		if (items[i]->id == id)
			return items[i];
	return NULL;
}

void vm_set_function_code(struct VMFunctionData *function_data,
		       const unsigned char *code, unsigned code_size) {
	function_data->code_size = code_size;
	function_data->code = code_size == 0 ? NULL :
		(unsigned char *)S_malloc(code_size);
	if (code_size != 0)
		memcpy(function_data->code, code, code_size);
}
