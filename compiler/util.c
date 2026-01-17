#include "util.h"

#include <assert.h>
#include <token.h>
#include <stdio.h>

#ifdef __unix__
void unix_error(char *msg){
  fprintf(stderr, "%s: %s\n", msg, strerror(errno));
}
#endif

static char *read_file_unix(const char *path) {
  FILE *fp = fopen(path, "rb");

  char err_buf[512];

  if (strlen(path) >= 512) {
    unix_error("File path is too long.");
  }    
  
  sprintf(err_buf, "Failed to open file : %s", path);

  if (!fp) {
    unix_error(err_buf);
  }

  fseek(fp, 0, SEEK_END);
  long size = ftell(fp);
  rewind(fp);
  
  char *bytes = S_malloc(size + 1);
  fread(bytes, 1, size, fp);
  bytes[size] = '\0';
  fclose(fp);

  /*
  mbstate_t st = {0};
  const char *p = bytes;

  size_t wlen = mbsrtowcs(NULL, &p, 0, &st);
  if (wlen == (size_t)-1) {
    unix_error("Invalid UTF-8 sequence");
  }

  wchar_t *wbuf = S_malloc((wlen + 1) * sizeof(wchar_t));

  st = (mbstate_t){0};
  p = bytes;
  mbsrtowcs(wbuf, &p, wlen, &st);
  wbuf[wlen] = L'\0';
  */
  
  return bytes;
}

char *read_file(char *path) {
  char *result = NULL;  
#if defined(__unix__)
  result = read_file_unix(path);
#endif

  assert(result != NULL);

  return result;  
}  

static unsigned get_hash(const char *key) {
  unsigned hash = 5381;
  char ch;

  while ((ch = *key) != L'\0') {
    hash += ((ch << 5) + ch) % HTABLE_BUFF;
    key++;    
  }

  return hash % HTABLE_BUFF;
}

struct HTable *gen_htable() {
  struct HTable *res = (struct HTable *)S_malloc(sizeof(struct HTable));

  res->size = 0;
  res->capacity = HTABLE_BUFF;
  
  return res;      
}

void free_htable(struct HTable *target_table) {
  int i;
  for (i = 0; i < HTABLE_BUFF; i++) {
    struct DataNode* node = target_table->bucket[i];
    while (node != NULL) {
      struct DataNode* next_node = node->next;      
      free(node);
      node = next_node;      
    }      
  }    
}  

void ht_insert(struct HTable *target_table, const char* key, void *ptr) {
  struct DataNode *node = (struct DataNode *)S_malloc(sizeof(struct DataNode));  
  unsigned hash = get_hash(key);
  
  node->ptr = ptr;
  node->key = key;
  node->next = NULL;

  struct DataNode *tnode = target_table->bucket[hash];  

  if (!tnode) {
    target_table->bucket[hash] = node;
  } else {
    tnode->next = node;
  }

  target_table->size++;
}

void *ht_find(struct HTable *target_table, const char *key) {  
  unsigned hash = get_hash(key);
  struct DataNode *tnode = target_table->bucket[hash];  

  while (tnode) {
    if (strcmp(tnode->key, key) == 0) {
      break;
    }
    tnode = tnode->next;
  }

  if (tnode == NULL) {
    return NULL;
  }    
  
  return tnode->ptr;
}  

struct Queue *gen_queue(){
  struct Queue *result = (struct Queue *)S_malloc(sizeof(struct Queue));  

  result->size = 0;
  result->tail = NULL;
  
  return result;
}

void q_push(struct Queue *target_queue, void *ptr) {
  struct DataNode *node = (struct DataNode *)S_malloc(sizeof(struct DataNode));  
  node->ptr = ptr;

  assert(target_queue != NULL);
  
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

void *q_pop(struct Queue *target_queue) {  
  if(target_queue->size == 0)
    return NULL;

  assert(target_queue->tail != NULL);

  struct DataNode *result = target_queue->tail->next;  

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
