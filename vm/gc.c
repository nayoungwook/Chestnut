#include <gc.h>

#include <vm.h>
#include <heap.h>

void gc(struct VM *vm){
    printf("DO A FUCKING GC...\n");
    
    pack_heap(vm);

    int i;
    for(i=0; i<vm->heap_object_count; i++){
        if(vm->ref_count[i] == 0){
            vm_free(vm, i);
        }
    }

    pack_heap(vm);
}
