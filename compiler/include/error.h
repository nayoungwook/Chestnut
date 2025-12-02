#ifndef ERROR_H
#define ERROR_H

#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

#include <token.h>

void panic(wchar_t *msg, TokenizerContext* tc);

#endif
