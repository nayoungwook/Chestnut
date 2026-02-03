#ifndef UTIL_H
#define UTIL_H

#include <memory.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define HTABLE_BUFF 509

char *read_file(char *path);
void write_file(const char *path, const size_t len, const char *data);

void *S_malloc(size_t size);
void *S_realloc(void *ptr, size_t size);

// node for data structure.
struct DataNode {
    struct DataNode *next;
    void *ptr;
    const char *key;
};

struct HTable {
    struct DataNode *bucket[HTABLE_BUFF];
    unsigned size;
    unsigned capacity;
};

struct HTable *gen_htable();
void free_htable(struct HTable *target_table);

void ht_insert(struct HTable *target_table, const char *key, void *ptr);
void *ht_find(struct HTable *target_table, const char *key);

struct Queue {
    struct DataNode *tail;
    unsigned size;
};

struct Queue *gen_queue();
void q_push(struct Queue *target_queue, void *ptr);
void *q_pop(struct Queue *target_queue);

#endif
