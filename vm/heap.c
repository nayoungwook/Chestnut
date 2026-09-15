#include <heap.h>
#include <vm.h>

#include <memory.h>
#include <inttypes.h>

#define DEBUG

/*
    HEAP_META = [SIZE_OF_MEMORY][OBJECT_ID]
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

static void debug_print_heap_view(const struct VM *vm) {
  const uint8_t *block = (const uint8_t *)vm->heap;
  const uint8_t *heap_end = (const uint8_t *)vm->heap_alloc_loc;

  printf("\nheap view\n");
  while (block < heap_end) {
    uint64_t header;
    uint32_t object_id;
    uint32_t size;
    size_t remaining = (size_t)(heap_end - block);

    if (remaining < HEAP_META_SIZE) {
      printf("[ CORRUPT | %zu byte ]", remaining);
      break;
    }

    memcpy(&header, block, sizeof(header));
    object_id = (uint32_t)header;
    size = (uint32_t)(header >> 32);

    if ((size_t)size > remaining - HEAP_META_SIZE) {
      printf("[ CORRUPT | size : %" PRIu32 " byte ]", size);
      break;
    }

    if (is_live_heap_block(vm, block))
      printf("[ id : %" PRIu32 " | %" PRIu32 " byte ]", object_id, size);
    else
      printf("[ FREE | %" PRIu32 " byte ]", size);

    block += HEAP_META_SIZE + size;
  }
  printf("\n");
}
#endif

void vm_free(struct VM *vm, unsigned heap_mapper_index) {
  if (heap_mapper_index == 0 || heap_mapper_index >= vm->heap_index ||
      vm->heap_mapper[heap_mapper_index] == NULL)
    return;

  unsigned *hmi = (unsigned *)S_malloc(sizeof(unsigned));
  *hmi = heap_mapper_index;
  q_push(vm->heap_index_queue, hmi);

  vm->heap_mapper[heap_mapper_index] = NULL;

#ifdef DEBUG
  debug_print_heap_view(vm);
#endif
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
  header |= (uint32_t) object_id;

  memcpy(vm->heap_alloc_loc, &header, sizeof(uint64_t));
  memset((uint8_t *)vm->heap_alloc_loc + HEAP_META_SIZE, 0, size);
  vm->heap_alloc_loc = (uint8_t *)vm->heap_alloc_loc + size + HEAP_META_SIZE;

#ifdef DEBUG
  debug_print_heap_view(vm);
#endif
  
  return heap_mapper_index;
}
