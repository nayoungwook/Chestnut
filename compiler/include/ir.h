#ifndef IR_H
#define IR_H

#include <parser.h>
#include <type.h>

#define BYTE_CHUNK 1024

#define META_BEGIN 0x01 // begining of metadata section
#define META_END 0x02   // end of metadata section

#define META_TERM 0x03

// META_CLASS class_id(uint) class_str(char*) [contents] META_TERM
#define META_CLASS 0x04

// META_FUNC func_id(uint) func_str(char*) return_type(char*)
#define META_FUNC 0x05

// META_VAR var_id(uint) var_str(char*)
#define META_VAR 0x06

#define CODE_BEGIN 0x51 // begining of code section
#define CODE_END 0x52

#define CODE_TERM 0x53

#define CODE_CLASS 0x54 // CODE_CLASS class_id(uint)

#define CODE_FUNC 0x55 // CODE_FUNC func_id(uint)

#define RODATA_BEGIN 0x11
#define RODATA_END 0x12

#define RODATA_STR 0x13 // RODATA_STR id(uint) str(char*)

// ops

#define OP_SP_PUSH 0x56 // sp_push amount(uint)
#define OP_SP_POP 0x57  // sp_pop amount(uint)

#define OP_SP_LOAD 0x58 // sp_load offset(uint) size(uint)
#define OP_SP_SAVE 0x59 // sp_save offset(uint) size(uint)

#define OP_SYSCALL 0x5a // syscall id(uint) arg_cnt(uint)
#define OP_CALL 0x5b    // call id(uint) arg_cnt(uint)

#define OP_LOAD_STR 0x60 // load_str id(uint)

#define OP_RET 0x70 // ret

// binary expression operation.
#define OP_EXPR_OP 0x71

#define OP_ADD 0x01
#define OP_SUB 0x02
#define OP_MUL 0x03
#define OP_DIV 0x04
#define OP_EQUAL 0x05
#define OP_NOTEQUAL 0x06
#define OP_GREATER 0x07
#define OP_LESS 0x08
#define OP_EQUALGREATER 0x09
#define OP_EQUALLESS 0x0a
#define OP_ASSIGN 0x0b
#define OP_OR 0x0c
#define OP_AND 0x0d

#define OP_LDC_I4 0x81 // load const integer 4byte
#define OP_LDC_F4 0x82 // load const float 4byte
#define OP_LDC_F8 0x83 // load const float 8byte

#define OP_PUSH_NULL 0x84 // push null

typedef unsigned char byte;

struct RODATA_Str {
        const char *str;
        unsigned id;
};

struct IRContext {
        byte *bytes;
        struct Node *node;
        unsigned byte_cnt, byte_size; // byte counter, byte size

        struct Queue *str_rodata;

	struct ClassData *current_class;
	struct FuncData *current_func;
};

struct IRContext *gen_irc();

void init_irc(struct IRContext *irc, struct Node *node);
void emit_byte(struct IRContext *irc, byte _b);

void print_bytes(struct IRContext *irc);
void gen_ir(struct IRContext *irc, struct ParserContext *pc);

const byte *get_bytes(struct IRContext *irc);

#endif
