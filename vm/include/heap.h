#ifndef HEAP_H
#define HEAP_H

struct VM;

unsigned vm_malloc(struct VM *vm, unsigned size, int object_id);
void vm_free(struct VM *vm, unsigned heap_mapper_index);

#endif
