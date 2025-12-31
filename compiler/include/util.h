#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <stdbool.h>

#define HTABLE_BUFF 509

#ifdef __linux__
#include <errno.h>
void unix_error(char *msg);
#endif

void *S_malloc(size_t size);
void *S_realloc(void *ptr, size_t size);

// node for data structure.
typedef struct _DNode {
  struct _DNode *next;
  void* ptr;
  const wchar_t* key;  
} DataNode;

typedef struct {
  DataNode *bucket[HTABLE_BUFF];
  unsigned size;
  unsigned capacity;
} HTable;

HTable *gen_htable();
void ht_insert(HTable* target_table, const wchar_t* key, void* ptr);
void* ht_find(HTable* target_table, const wchar_t* key);

typedef struct {
  DataNode *tail;
  unsigned size;
} Queue;

Queue *gen_queue();
void q_push(Queue *target_queue, void* ptr);
void* q_pop(Queue *target_queue);

#endif
