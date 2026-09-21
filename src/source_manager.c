#include <source_manager.h>
#include <util.h>

struct Sources *gen_sources(){
    struct Sources *sources = (struct Sources *) S_malloc(sizeof(struct Sources));

    sources->capacity = 1;
    sources->count = 0;
    sources->paths = (const char **) S_malloc(sizeof(const char *));

    return sources;
}

void add_source(struct Sources *sources, const char *path){
    if(sources->capacity < sources->count + 1) {
        sources->capacity *= 2;

        sources->paths = (const char **) S_realloc(sources->paths, sizeof(const char *) * sources->capacity);
    }
    
    sources->paths[sources->count] = path;
    sources->count++;
}
