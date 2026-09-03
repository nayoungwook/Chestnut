#include <heap.h>
#include <vm.h>

#include <memory.h>

#define HEAP_META_SIZE 4 * 2

/*
    HEAP_META = [SIZE_OF_MEMORY][OBJECT_ID]
    [HEAP_META][DATA] [HEAP_META][DATA]
*/

void vm_free(struct VM* vm, unsigned heap_mapper_index) {
    unsigned* hmi = (unsigned*)S_malloc(sizeof(unsigned));
    *hmi = heap_mapper_index;
    q_push(vm->heap_index_queue, hmi);
}

unsigned vm_malloc(struct VM* vm, unsigned size, unsigned object_id) {
    unsigned heap_mapper_index = 0;

    if (vm->heap_index_queue->size == 0) {
        heap_mapper_index = vm->heap_index;
        vm->heap_index++;
    }
    else {
        unsigned* hmi = (unsigned*)q_pop(vm->heap_index_queue);
        heap_mapper_index = *hmi;
        free(hmi);
    }

    if (vm->heap_index >= HEAP_MAX_OBJECT_COUNT) {
        // occur error.
    }

    vm->heap_mapper[heap_mapper_index] = vm->heap_alloc_loc;

    uint64_t header = 0;
    header |= ((uint64_t)size << 32);
    header |= object_id;

    memcpy(vm->heap_alloc_loc, &header, sizeof(uint64_t));
    memset((uint8_t*)vm->heap_alloc_loc + HEAP_META_SIZE, 0, size);
    vm->heap_alloc_loc =
        (uint8_t*)vm->heap_alloc_loc + size + HEAP_META_SIZE;

    return heap_mapper_index;
}
