#include "compile_sources.h"
#include "run_bytecode.h"

#include <util.h>
#include <string.h>

static bool has_extension(const char *path, const char *expected) {
    const char *extension = strrchr(path, '.');
    return extension != NULL && strcmp(extension, expected) == 0;
}

int main(int argc, char *argv[]) {
    const char **sources;
    unsigned source_count = 0;
    int i;
    bool compiled;

    if (argc < 2)
        return 1;

    sources = S_malloc(sizeof(*sources) * (size_t)(argc - 1));
    for (i = 1; i < argc; i++) {
        if (has_extension(argv[i], ".cn")) {
            sources[source_count++] = argv[i];
        } else if (!has_extension(argv[i], ".cb")) {
            fprintf(stderr, "Unsupported file extension: %s (expected .cn or .cb)\n", argv[i]);
            free(sources);
            return 1;
        }
    }
    compiled = source_count == 0 || compile_sources(sources, source_count);
    free(sources);
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
