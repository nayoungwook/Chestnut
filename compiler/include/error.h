#ifndef ERROR_H
#define ERROR_H

#include <wchar.h>

#include "token.h"

void panic(char *msg, struct TokenizerContext* tc);

#endif
