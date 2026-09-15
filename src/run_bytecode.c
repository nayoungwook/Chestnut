#include "run_bytecode.h"

#include <ir_read.h>
#include <vm.h>

#include <limits.h>

static bool load_bytecode(const char *path, struct IRContext *irc) {
    FILE *file = fopen(path, "rb");
    long size;
    bool loaded = false;

    if (file == NULL) {
        perror(path);
        return false;
    }
    if (fseek(file, 0, SEEK_END) != 0)
        goto done;
    size = ftell(file);
    if (size <= 0 || (unsigned long)size > UINT_MAX || fseek(file, 0, SEEK_SET) != 0)
        goto done;

    irc->bytes = S_malloc((size_t)size);
    irc->byte_cnt = (unsigned)size;
    loaded = fread(irc->bytes, 1, (size_t)size, file) == (size_t)size;

done:
    fclose(file);
    if (!loaded)
        fprintf(stderr, "Failed to read bytecode: %s\n", path);
    return loaded;
}

bool run_bytecodes(const char **paths, unsigned count) {
    struct IRReader **readers = S_malloc(sizeof(*readers) * count);
    struct VM *vm = gen_vm();
    struct VMFunctionData *entry;
    unsigned loaded = 0;
    unsigned i;
    bool success = false;

    vm->main_func_id = 0;
    for (i = 0; i < count; i++) {
        struct IRContext *irc = gen_irc();
        if (!load_bytecode(paths[i], irc)) {
            free_irc(irc);
            goto done;
        }
        readers[loaded++] = gen_ir_reader(irc);
    }
    if (!read_ir_files(vm, readers, count))
        goto done;
    entry = vm_find_function_data(vm, NULL, vm->main_func_id);
    if (entry == NULL) {
        fprintf(stderr, "Bytecode has no main function.\n");
        goto done;
    }
    vm_exec_function(vm, entry, (unsigned)-1);
    success = true;

done:
    free_vm(vm);
    for (i = 0; i < loaded; i++) {
        free_irc(readers[i]->irc);
        free_ir_reader(readers[i]);
    }
    free(readers);
    return success;
}

bool run_bytecode(const char *path) {
    return run_bytecodes(&path, 1);
}
