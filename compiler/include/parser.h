#ifndef PARSER_H
#define PARSER_H

#include <token.h>

struct CNode {
  struct CNode* next;
  void* ptr;
};


typedef struct {
  
} ParserContext;

#endif
