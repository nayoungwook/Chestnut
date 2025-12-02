#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#ifdef __linux__

#include <errno.h>
void unix_error(char *msg);

#endif

#define bool short
#define true 1
#define false 0

void* S_malloc(size_t size);

#endif
