#ifndef ERROR_H
#define ERROR_H

#include <token.h>

#include <wchar.h>

void panic(char *msg, struct TokenizerContext *tc);

#endif
