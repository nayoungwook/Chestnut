#ifndef IR_H
#define IR_H

#include "util.h"
#include <parser.h>
#include <type.h>

#define BYTE_CHUNK 1024

// --------------------------
// ** MetaData **
// --------------------------

#define META_BEGIN 0x01 // begining of metadata section
#define META_END 0x02   // end of metadata section

#define META_TERM 0x03

// META_CLASS class_id(uint) class_str(char*) [contents] META_TERM
#define META_CLASS 0x04

// META_FUNC func_id(uint) func_str(char*) return_type(char*)
#define META_FUNC 0x05

// META_VAR var_id(uint) var_str(char*)
#define META_VAR 0x06

// META_CONSTRUCTOR func_id(uint) func_str(char*)
#define META_CONSTRUCTOR 0X07

#define CODE_BEGIN 0x51 // begining of code section
#define CODE_END 0x52

#define CODE_TERM 0x53

#define CODE_CLASS 0x54 // CODE_CLASS class_id(uint)

#define CODE_FUNC 0x55 // CODE_FUNC func_id(uint)

#define RODATA_BEGIN 0x11
#define RODATA_END 0x12

#define RODATA_STR 0x13 // RODATA_STR id(uint) str(char*)

// --------------------------
// ** Operations **
// --------------------------

#define OP_SP_PUSH 0x56 // sp_push amount(uint)
#define OP_SP_POP 0x57  // sp_pop amount(uint)

#define OP_SP_LOAD 0x58 // sp_load offset(uint) size(uint)
#define OP_SP_SAVE 0x59 // sp_save offset(uint) size(uint)
#define OP_SP_INCRE 0x5a // sp_incre offset(uint) size(uint)
#define OP_SP_DECRE 0x5b // sp_decre offset(uint) size(uint)

#define OP_SYSCALL 0x5c // syscall id(uint) arg_cnt(uint)
#define OP_CALL 0x5d    // call id(uint) arg_cnt(uint)
#define OP_CALL_ATTR 0x5e // call_attr id(uint) arg_cnt(uint)
#define OP_CALL_CLASS 0x5f // call_class id(uint) arg_cnt(uint)
#define OP_CALL_GLOBAL 0x61 // call_global id(uint) arg_cnt(uint)

#define OP_LOAD_STR 0x60 // load_str id(uint)

#define OP_RET 0x70 // ret
#define OP_RET_VAL 0x71 // ret with value. store return value from top of the stack into ret register 

#define OP_GOTO 0x73 // goto label(uint)
#define OP_LABEL 0x74 // label(uint)
#define OP_JE 0x75   // je label(uint)
#define OP_JNE 0x76  // jne label(uint)

// --------------------------
// ** Binary expression **
// --------------------------

#define OP_EXPR_OP 0x72

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

// --------------------------
// ** Load and push data **
// --------------------------

#define OP_LDC_I4 0x81 // load const integer 4byte
#define OP_LDC_F4 0x82 // load const float 4byte
#define OP_LDC_F8 0x83 // load const float 8byte

#define OP_PUSH_NULL 0x84 // push null
#define OP_LOAD_CLASS 0x85 // load_class offset(uint) size(uint) # load from current class and push into stack.
#define OP_INCRE_CLASS 0x8a // incre_global offset(uint) size(uint) # increase from current class
#define OP_DECRE_CLASS 0x8b // decre_global offset(uint) size(uint) # decrease from current class

#define OP_LOAD_GLOBAL 0x8c // load_global offset(uint) sizeof(uint) # load from global and push into stack.
#define OP_INCRE_GLOBAL 0x8d // incre_global offset(uint) sizeof(uint) # increase from global
#define OP_DECRE_GLOBAL 0x8e // decre_global offset(uint) sizeof(uint) # decrease from global

#define OP_LOAD_ATTR 0x8f // load_attr offset(uint) size(uint) # load from data from top of the stack.

#define OP_INCRE_ATTR 0x90 // incre_attr offset(uint) size(uint) # increase from data from top of the stack.
#define OP_DECRE_ATTR 0x91 // decre_attr offset(uint) size(uint) # decrease from data from top of the stack.

typedef unsigned char byte;

struct RODATA_Str {
        const char *str;
        unsigned id;
};

struct IRContext {
        byte *bytes;
        struct Node *node;
        unsigned byte_cnt, byte_size; // byte counter, byte size
	unsigned label_id;
	
        struct HTable *str_rodata;

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
