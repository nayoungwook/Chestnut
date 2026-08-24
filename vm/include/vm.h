#ifndef VM_H
#define VM_H

struct ClassData;

struct VM {
	struct ClassData **class_data;
	unsigned class_data_count, class_data_capacity;
};

struct VM *gen_vm();
void add_class_data(struct VM *vm);

#endif
