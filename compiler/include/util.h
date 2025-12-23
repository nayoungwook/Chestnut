#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#define bool short
#define true 1
#define false 0

#define HTABLE_BUFF 512

#ifdef __linux__
#include <errno.h>
void unix_error(char *msg);
#endif

void *S_malloc(size_t size);
void *S_realloc(void *ptr, size_t size);

struct Node {
  struct Node *next;
  const wchar_t* key;  
  void* ptr;
};

unsigned get_hash(const wchar_t *key);

struct HTable {
  struct Node *bucket[HTABLE_BUFF];
  unsigned size;
  unsigned capacity;
};

struct HTable *gen_htable();
void HT_insert(struct HTable* target_table, const wchar_t* key, void* ptr);
struct Node* HT_find(struct HTable* target_table, const wchar_t* key);

#endif
