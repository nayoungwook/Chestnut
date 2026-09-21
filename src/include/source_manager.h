#ifndef SOURCE_MANAGER_H
#define SOURCE_MANAGER_H

struct Sources {
    const char **paths;
    unsigned count, capacity;
};

struct Sources *gen_sources();
void add_source(struct Sources *sources, const char *path);

#endif
