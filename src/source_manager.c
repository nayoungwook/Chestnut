#include <source_manager.h>
#include <util.h>
#include <string.h>

struct Sources *gen_sources(){
    struct Sources *sources = (struct Sources *) S_malloc(sizeof(struct Sources));

    sources->capacity = 1;
    sources->count = 0;
    sources->paths = (const char **) S_malloc(sizeof(const char *));

    return sources;
}

void add_source(struct Sources *sources, const char *path){
    char *copy = S_malloc(strlen(path) + 1);
    strcpy(copy, path);

    if(sources->capacity < sources->count + 1) {
        sources->capacity *= 2;

        sources->paths = (const char **) S_realloc(sources->paths, sizeof(const char *) * sources->capacity);
    }
    
    sources->paths[sources->count] = copy;
    sources->count++;
}

void free_sources(struct Sources *sources) {
    unsigned i;

    for (i = 0; i < sources->count; i++)
        free((void *)sources->paths[i]);
    free(sources->paths);
    free(sources);
}
