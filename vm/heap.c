#include <heap.h>
#include <vm.h>

#include <memory.h>

#define HEAP_META_SIZE 4 * 2

#define DEBUG

/*
    HEAP_META = [SIZE_OF_MEMORY][OBJECT_ID]
    [HEAP_META][DATA] [HEAP_META][DATA]
*/

#ifdef DEBUG
static void debug_print_heap(struct VM *vm) {
     int i;

     printf("\nheap view\n");
     for (i = 0; i < vm->heap_object_count; i++) {
          void *heap = vm->heap_mapper[i];

          if(heap != NULL){
              printf("[ %d byte | id : %d ]", *(int*) (heap), *(int*)(heap + 4));
          } else {
               printf("[ FREE ]");
          }
     }
     printf("\n");
}
#endif

void vm_free(struct VM *vm, unsigned heap_mapper_index) {
  unsigned *hmi = (unsigned *)S_malloc(sizeof(unsigned));
  *hmi = heap_mapper_index;
  q_push(vm->heap_index_queue, hmi);

  vm->heap_mapper[heap_mapper_index] = NULL;
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

  uint64_t header = 0;
  header |= ((uint64_t)size << 32);
  header |= object_id;

  memcpy(vm->heap_alloc_loc, &header, sizeof(uint64_t));
  memset((uint8_t *)vm->heap_alloc_loc + HEAP_META_SIZE, 0, size);
  vm->heap_alloc_loc = (uint8_t *)vm->heap_alloc_loc + size + HEAP_META_SIZE;

#ifdef DEBUG
    debug_print_heap(vm);
#endif
  
  return heap_mapper_index;
}
