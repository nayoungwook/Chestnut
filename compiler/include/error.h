#ifndef ERROR_H
#define ERROR_H

#include <wchar.h>

#include "token.h"

void panic(wchar_t *msg, TokenizerContext* tc);

#endif
