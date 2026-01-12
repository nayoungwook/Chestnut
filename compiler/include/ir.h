#ifndef IR_H
#define IR_H

#include <parser.h>

#define BYTE_CHUNK 1024

#define META_BEGIN 0x01 // begining of metadata section
#define META_END 0x02   // end of metadata section

#define META_TERM 0x03

#define META_CLASS 0x04 // META_CLASS class_id(uint) class_str(whcar_t*) [contents] META_TERM
#define META_FUNC 0x05  // META_FUNC func_id(uint) func_str(wchar_t*) return_type(wchar_t*)
#define META_VAR 0x06   // META_VAR var_id(uint) var_str(wchar_t*)

typedef unsigned char byte;

typedef struct {
  byte *bytes;  
  Node *node;
  unsigned byte_cnt, byte_size; // byte counter, byte size
} IRContext;

IRContext *gen_irc();

void init_irc(IRContext *irc, Node *node);
void emit_byte(IRContext *irc, byte _b);

void gen_metadata(IRContext *irc, ParserContext *pc);

void print_bytes(IRContext *irc);

#endif
