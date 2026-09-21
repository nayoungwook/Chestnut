#include "compile_sources.h"
#include <source_manager.h>
#include "run_bytecode.h"

#include <util.h>
#include <string.h>

static bool has_extension(const char *path, const char *expected) {
    const char *extension = strrchr(path, '.');
    return extension != NULL && strcmp(extension, expected) == 0;
}

int main(int argc, char *argv[]) {
    struct HTable *source_table = gen_htable();
    struct Sources *sources = gen_sources();

    int i;
    bool compiled;

    if (argc < 2)
        return 1;
    
    for (i = 1; i < argc; i++) {
        if (has_extension(argv[i], ".cn")) {
            if(ht_find(source_table, argv[i]) == NULL){
                add_source(sources, argv[i]);
                ht_insert(source_table, argv[i], argv[i]);
            }
        } else if (!has_extension(argv[i], ".cb")) {
            fprintf(stderr, "Unsupported file extension: %s (expected .cn or .cb)\n", argv[i]);
            free(sources);
            return 1;
        }
    }

    handle_preprocessor(source_table, sources);

    compiled = compile_sources(sources);
    
    if (!compiled)
        return 1;

    const char **bytecodes = S_malloc(sizeof(*bytecodes) * (size_t)(argc - 1));
    unsigned bytecode_count = 0;
    
    for (i = 1; i < argc; i++)
        if (has_extension(argv[i], ".cb"))
            bytecodes[bytecode_count++] = argv[i];
    
    bool executed = bytecode_count == 0 || run_bytecodes(bytecodes, bytecode_count);
    free(bytecodes);

    return executed ? 0 : 1;
}
