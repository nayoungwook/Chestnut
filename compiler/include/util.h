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

typedef struct _HNode {
  struct _HNode *next;
  const wchar_t* key;  
  void* ptr;
} HNode;

typedef struct {
  HNode *bucket[HTABLE_BUFF];
  unsigned size;
  unsigned capacity;
} HTable;

HTable *gen_htable();
void ht_insert(HTable* target_table, const wchar_t* key, void* ptr);
void* ht_find(HTable* target_table, const wchar_t* key);

#endif
