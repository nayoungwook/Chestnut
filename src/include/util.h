#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#ifdef __linux__

#include <errno.h>
void unix_error(char *msg);

#endif

void* S_malloc(size_t size);

#endif
