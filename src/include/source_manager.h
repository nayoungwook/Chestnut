#ifndef SOURCE_MANAGER_H
#define SOURCE_MANAGER_H

struct Sources {
    const char **paths;
    unsigned count, capacity;
};

struct Sources *gen_sources();
/* Store an owned copy so tokenizer-owned import paths can be released. */
void add_source(struct Sources *sources, const char *path);
void free_sources(struct Sources *sources);

#endif
