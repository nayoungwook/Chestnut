#include <util.h>
#include <token.h>

#ifdef __linux__
void unix_error(char *msg){
  fprintf(stderr, "%s: %s\n", msg, strerror(errno));
}
#endif

unsigned get_hash(const wchar_t *key) {
  unsigned hash = 5381;
  wchar_t ch;

  while ((ch = *key) != L'\0') {
    hash += ((ch << 5) + ch) % HTABLE_SIZE;
    key++;    
  }

  return hash;
}

struct HTable *gen_htable() {
  struct HTable *res = (struct HTable *)S_malloc(sizeof(struct HTable));
  
  res->size = 0;
  
  return res;      
}

void HT_insert(struct HTable *target_table, const wchar_t* key, void *ptr) {
  struct Node *node = (struct Node *)S_malloc(sizeof(struct Node));
  unsigned hash = get_hash(key);
  
  node->ptr = ptr;
  node->key = key;

  struct Node *tnode = target_table->bucket[hash];

  if (!tnode) {
    target_table->bucket[hash] = node;
  } else {
    tnode->next = node;
  }

  target_table->size++;
}

struct Node *HT_find(struct HTable *target_table, const wchar_t *key) {
  unsigned hash = get_hash(key);
  struct Node *tnode = target_table->bucket[hash];

  while (tnode) {
    if (wcscmp(tnode->key, key) == 0) {
      break;
    }
    tnode = tnode->next;
  }
  
  return tnode;  
}  

void* S_malloc(size_t size){
  void* res = malloc(size);

  if(!res){
    unix_error("memory allocation failed.");
    exit(1);
  }
  
  return res;
}
