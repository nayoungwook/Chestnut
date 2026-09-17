#ifndef HEAP_H
#define HEAP_H

struct VM;

#define HEAP_META_SIZE 8

unsigned vm_malloc(struct VM *vm, unsigned size, int object_id);
void vm_free(struct VM *vm, unsigned heap_mapper_index);

#ifdef DEBUG
void debug_print_heap_view(const struct VM *vm);
#endif
    
#endif
