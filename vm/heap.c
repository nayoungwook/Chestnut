#define DEBUG

#include <heap.h>
#include <vm.h>

#include <assert.h>
#include <memory.h>
#include <inttypes.h>

/*
  HEAP_META = [ SIZE_OF_MEMORY 4bytes ][ HEAP_MAPPER_ID 3bytes ][OBJECT_ID 1bytes]
  [HEAP_META][DATA] [HEAP_META][DATA]
*/

#ifdef DEBUG
static bool is_live_heap_block(const struct VM *vm, const void *block) {
    unsigned i;

    for (i = 1; i < vm->heap_index; i++) {
        if (vm->heap_mapper[i] == block)
            return true;
    }
    
    return false;
}

void debug_print_heap_view(const struct VM *vm) {
    const uint8_t *block = (const uint8_t *)vm->heap;
    const uint8_t *heap_end = (const uint8_t *)vm->heap_alloc_loc;

    printf("heap view\n");
    while (block < heap_end) {
        uint64_t header;
        int object_id;
        unsigned heap_mapper_id;
        uint32_t size;
        size_t remaining = (size_t)(heap_end - block);

        if (remaining < HEAP_META_SIZE) {
            printf("[ CORRUPT | %zu byte ]", remaining);
            break;
        }

        memcpy(&header, block, sizeof(header));
        object_id = (char) header;
        size = (uint32_t) (header >> 32);
        heap_mapper_id = (uint32_t) ((header >> 8) & 0x00FFFFFF);

        if ((size_t)size > remaining - HEAP_META_SIZE) {
            printf("[ CORRUPT | size : %d byte ] ", size);
            break;
        }

        if (is_live_heap_block(vm, block))
            printf("[ id : %d | %d byte | heap mapper id : %d ] ", object_id, size, heap_mapper_id);
        else
            printf("[ FREE | %d byte ] ", size);

        block += HEAP_META_SIZE + size;
    }
    printf("\n");
}
#endif

void pack_heap(struct VM *vm) {
#ifdef DEBUG
    printf("\nPacking a fucking heap...\n");
#endif
    
    const uint8_t *block = (const uint8_t *) vm->heap;
    uint8_t *packer_ptr = (uint8_t *) vm->heap;
    const uint8_t *heap_end = (const uint8_t *) vm->heap_alloc_loc;
    unsigned object_count = 0;
    
    while(block < heap_end){
        uint64_t header;
        memcpy(&header, block, sizeof(header));
        
        unsigned size = (uint32_t) (header >> 32);
        unsigned heap_mapper_id = (uint32_t) ((header >> 8) & 0x00FFFFFF);
        
        if(is_live_heap_block(vm, block)){
            object_count++;
            if(packer_ptr != block){ // if in the same position, we don't have to copy.
                memmove(packer_ptr, block, HEAP_META_SIZE + size);
            }

            vm->heap_mapper[heap_mapper_id] = packer_ptr;
            packer_ptr += HEAP_META_SIZE + size;
        }
        
        block += HEAP_META_SIZE + size;
    }

    vm->heap_alloc_loc = packer_ptr;
    vm->heap_object_count = object_count;
    printf("object count : %d\n", object_count);
    
#ifdef DEBUG
    debug_print_heap_view(vm);
#endif
}

void vm_free(struct VM *vm, unsigned heap_mapper_index) {
    if (heap_mapper_index == 0 || heap_mapper_index >= vm->heap_index ||
        vm->heap_mapper[heap_mapper_index] == NULL)
        return;

    unsigned *hmi = (unsigned *)S_malloc(sizeof(unsigned));
    *hmi = heap_mapper_index;
    q_push(vm->heap_index_queue, hmi);

    vm->heap_mapper[heap_mapper_index] = NULL;
    vm->ref_count[heap_mapper_index]--;
}

void vm_replace_heap_block(struct VM *vm, unsigned target_heap_mapper_index,
                           unsigned replacement_heap_mapper_index) {
    void *replacement;
    uint64_t header;

    replacement = vm->heap_mapper[replacement_heap_mapper_index];
    memcpy(&header, replacement, sizeof(header));
    header &= ~((uint64_t)0x00FFFFFFu << 8);
    header |= ((uint64_t)(target_heap_mapper_index & 0x00FFFFFFu) << 8);
    memcpy(replacement, &header, sizeof(header));

    vm_free(vm, replacement_heap_mapper_index);
    vm->heap_mapper[target_heap_mapper_index] = replacement;
    
    vm->ref_count[target_heap_mapper_index]++;
    vm->ref_count[replacement_heap_mapper_index]--;
}

unsigned vm_malloc(struct VM *vm, unsigned size, int object_id) {

    unsigned heap_mapper_index = 0;
    vm->heap_object_count++;

    if (vm->heap_index_queue->size == 0) {
        if (vm->heap_index >= HEAP_MAX_OBJECT_COUNT)
            return 0;
    
        heap_mapper_index = vm->heap_index;
        vm->heap_index++;
    } else {
        unsigned *hmi = (unsigned *)q_pop(vm->heap_index_queue);
        heap_mapper_index = *hmi;
        free(hmi);
    }

    vm->heap_mapper[heap_mapper_index] = vm->heap_alloc_loc;
    vm->ref_count[heap_mapper_index]++;
    
    uint64_t header = 0;
    header |= ((uint64_t)(heap_mapper_index & 0x00FFFFFFu) << 8);
    header |= (uint8_t) object_id;
    header |= ((uint64_t)size << 32);

    memcpy(vm->heap_alloc_loc, &header, sizeof(uint64_t));
    memset((uint8_t *)vm->heap_alloc_loc + HEAP_META_SIZE, 0, size);
    vm->heap_alloc_loc = (uint8_t *)vm->heap_alloc_loc + size + HEAP_META_SIZE;
  
    return heap_mapper_index;
}
