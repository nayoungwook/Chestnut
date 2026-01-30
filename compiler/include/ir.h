#ifndef IR_H
#define IR_H

#include <parser.h>

#define BYTE_CHUNK 1024

#define META_BEGIN 0x01 // begining of metadata section
#define META_END 0x02   // end of metadata section

#define META_TERM 0x03

#define META_CLASS 0x04 // META_CLASS class_id(uint) class_str(char*) [contents] META_TERM
#define META_FUNC 0x05  // META_FUNC func_id(uint) func_str(char*) return_type(char*)
#define META_VAR 0x06   // META_VAR var_id(uint) var_str(char*)

#define CODE_BEGIN 0x51 // begining of code section
#define CODE_END 0x52

#define CODE_TERM 0x53

#define CODE_CLASS 0x54

#define OP_NUMBER_LITERAL 0x55

typedef unsigned char byte;

struct IRContext {
	byte *bytes;
	struct Node *node;
	unsigned byte_cnt, byte_size; // byte counter, byte size
};

struct IRContext *gen_irc();

void init_irc(struct IRContext *irc, struct Node *node);
void emit_byte(struct IRContext *irc, byte _b);

void print_bytes(struct IRContext *irc);
void gen_ir(struct IRContext *irc, struct ParserContext *pc);

const byte *get_bytes(struct IRContext *irc);

#endif
