#ifndef RUN_BYTECODE_H
#define RUN_BYTECODE_H

#include <stdbool.h>

bool run_bytecode(const char *path);
bool run_bytecodes(const char **paths, unsigned count);

#endif
