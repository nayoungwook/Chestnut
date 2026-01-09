#ifndef IR_H
#define IR_H

#include <parser.h>

#define BYTE_CHUNK 1024 * 4

#define META_BEGIN 0x01
#define META_END 0x02

typedef unsigned char byte;

typedef struct {
  byte *bytes;  
  Node *node;
  unsigned byte_cnt, byte_size; // byte counter, byte size
} IRContext;

IRContext *gen_irc();

void init_irc(IRContext *irc, Node *node);
void emit_byte(IRContext *irc, byte _b);

byte *gen_metadata(ParserContext *pc);

void print_bytes(IRContext *irc);

#endif
