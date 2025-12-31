#include <util.h>
#include <token.h>

#ifdef __linux__
void unix_error(char *msg){
  fprintf(stderr, "%s: %s\n", msg, strerror(errno));
}
#endif

static unsigned get_hash(const wchar_t *key) {
  unsigned hash = 5381;
  wchar_t ch;

  while ((ch = *key) != L'\0') {
    hash += ((ch << 5) + ch) % HTABLE_BUFF;
    key++;    
  }

  return hash % HTABLE_BUFF;
}

HTable *gen_htable() {
  HTable *res = (HTable *)S_malloc(sizeof(HTable));

  res->size = 0;
  res->capacity = HTABLE_BUFF;
  
  return res;      
}

void ht_insert(HTable *target_table, const wchar_t* key, void *ptr) {
  DataNode *node = (DataNode *)S_malloc(sizeof(DataNode));
  unsigned hash = get_hash(key);
  
  node->ptr = ptr;
  node->key = key;

  DataNode *tnode = target_table->bucket[hash];

  if (!tnode) {
    target_table->bucket[hash] = node;
  } else {
    tnode->next = node;
  }

  target_table->size++;
}

void *ht_find(HTable *target_table, const wchar_t *key) {
  unsigned hash = get_hash(key);
  DataNode *tnode = target_table->bucket[hash];

  while (tnode) {
    if (wcscmp(tnode->key, key) == 0) {
      break;
    }
    tnode = tnode->next;
  }
  
  return tnode->ptr;
}  

Queue *gen_queue(){
  Queue* result = (Queue*) S_malloc(sizeof(Queue));

  result->size = 0;
  result->tail = NULL;
  
  return result;
}

void q_push(Queue *target_queue, void* ptr){
  DataNode *node = (DataNode*) S_malloc(sizeof(DataNode));
  node->ptr = ptr;
  
  if(target_queue->tail == NULL){
    target_queue->tail = node;
    node->next = target_queue->tail;
  } else {
    node->next = target_queue->tail->next;
    target_queue->tail->next = node;
    target_queue->tail = node;
  }
  
  target_queue->size++;
}

void* q_pop(Queue *target_queue){
  if(target_queue->size == 0)
    return NULL;
  
  DataNode* result = target_queue->tail->next;

  if(target_queue->size == 1){
    target_queue->tail = NULL;
  }else{
    target_queue->tail->next = result->next;
  }
  
  target_queue->size--;
  result->next = NULL;

  return result->ptr;
}

void* S_malloc(size_t size){
  void* res = malloc(size);

  if(!res){
    unix_error("memory allocation failed.");
    exit(1);
  }
  
  return res;
}

void* S_realloc(void* ptr, size_t size){
  void* res = realloc(ptr, size);

  if(!res){
    unix_error("memory allocation failed.");
    exit(1);
  }
  
  return res;
}
