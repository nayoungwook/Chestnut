#ifndef COMPILE_SOURCES_H
#define COMPILE_SOURCES_H

#include <stdbool.h>

#define MAX_PATH_LENGTH 100

struct HTable;
struct Sources;

bool handle_preprocessor(struct HTable *source_table, struct Sources *sources);
bool compile_sources(struct Sources *sources);

#endif
