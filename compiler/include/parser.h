#ifndef PARSER_H
#define PARSER_H

#include <token.h>

struct CNODE {
  struct CNODE* next,* prev;
  void* ptr;
};

typedef struct {
  
} ParserContext;

#endif
