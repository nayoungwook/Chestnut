#include <vm.h>
#include <code_data.h>
#include <util.h>

struct VM *gen_vm() {
	struct VM *vm = {
		0,
	};
	
	vm->class_data_capacity = 1;
	vm->class_data = (struct ClassData **) S_malloc(sizeof(struct ClassData*));
	
	return vm;
}

void add_class_data(struct VM *vm) {
	vm->class_data_count++;

	if (vm->class_data_count >= vm->class_data_capacity) {
		vm->class_data_capacity *= 2;
		vm->class_data =
			(struct ClassData **) S_realloc(vm->class_data, sizeof(struct ClassData*) * vm->class_data_capacity);
	}

	struct ClassData *class_data = (struct ClassData *) S_malloc(sizeof(struct ClassData));
	vm->class_data[vm->class_data_count - 1] = class_data;
}
