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

// META_CLASS class_id(uint) class_str(char*) parent_id(uint) size(uint)
//            [contents] META_TERM
#define META_CLASS 0x04

// META_FUNC func_id(uint) func_str(char*) return_type(char*)
//           arg_count(uint) arg_types(char* ...) stack_size(uint)
#define META_FUNC 0x05

// META_VAR var_id(uint) var_str(char*) type(char*) offset(uint) size(uint)
#define META_VAR 0x06

// META_CONSTRUCTOR func_id(uint) func_str(char*)
//                  arg_count(uint) arg_types(char* ...) stack_size(uint)
#define META_CONSTRUCTOR 0X07

#define CODE_BEGIN 0x51 // begining of code section
#define CODE_END 0x52

#define CODE_TERM 0x53

#define CODE_CLASS 0x54 // CODE_CLASS class_id(uint)

#define CODE_FUNC 0x55 // CODE_FUNC func_id(uint) code_size(uint) code(bytes)

#define CODE_INITIALIZER 0x50 // CODE_INITIALIZER code_size(uint) code(bytes)

#define RODATA_BEGIN 0x11
#define RODATA_END 0x12

#define RODATA_STR 0x13 // RODATA_STR id(uint) str(char*)

// --------------------------
// ** Operations **
// --------------------------

#define IR_INSTRUCTION_LIST(X)                                                 \
    X(OP_SP_PUSH, 0x56, "sp_push")                                            \
    X(OP_SP_POP, 0x57, "sp_pop")                                              \
    X(OP_SP_LOAD, 0x58, "sp_load")                                            \
    X(OP_SP_SAVE, 0x59, "sp_save")                                            \
    X(OP_SP_INCRE, 0x5a, "sp_incre")                                          \
    X(OP_SP_DECRE, 0x5b, "sp_decre")                                          \
    X(OP_SYSCALL, 0x5c, "syscall")                                            \
    X(OP_CALL, 0x5d, "call")                                                  \
    X(OP_CALL_ATTR, 0x5e, "call_attr")                                        \
    X(OP_CALL_CLASS, 0x5f, "call_class")                                      \
    X(OP_LOAD_STR, 0x60, "load_str")                                          \
    X(OP_CALL_GLOBAL, 0x61, "call_global")                                    \
    X(OP_CALL_SUPER, 0x62, "call_super")                                      \
    X(OP_RET, 0x70, "ret")                                                    \
    X(OP_EXPR_OP, 0x72, "expr_op")                                            \
    X(OP_GOTO, 0x73, "goto")                                                  \
    X(OP_LABEL, 0x74, "label")                                                \
    X(OP_JE, 0x75, "je")                                                      \
    X(OP_JNE, 0x76, "jne")                                                    \
    X(OP_NEG, 0x77, "neg")                                                    \
    X(OP_LDC_I4, 0x81, "ldc_i4")                                              \
    X(OP_LDC_F4, 0x82, "ldc_f4")                                              \
    X(OP_LDC_F8, 0x83, "ldc_f8")                                              \
    X(OP_PUSH_NULL, 0x84, "push_null")                                        \
    X(OP_LOAD_CLASS, 0x85, "load_class")                                      \
    X(OP_INCRE_CLASS, 0x8a, "incre_class")                                    \
    X(OP_DECRE_CLASS, 0x8b, "decre_class")                                    \
    X(OP_LOAD_GLOBAL, 0x8c, "load_global")                                    \
    X(OP_INCRE_GLOBAL, 0x8d, "incre_global")                                  \
    X(OP_DECRE_GLOBAL, 0x8e, "decre_global")                                  \
    X(OP_LOAD_ATTR, 0x8f, "load_attr")                                        \
    X(OP_INCRE_ATTR, 0x90, "incre_attr")                                      \
    X(OP_DECRE_ATTR, 0x91, "decre_attr")                                      \
    X(OP_SAVE_CLASS, 0x92, "save_class")                                      \
    X(OP_SAVE_GLOBAL, 0x93, "save_global")                                    \
    X(OP_SAVE_ATTR, 0x94, "save_attr")                                        \
    X(OP_NEW_OBJECT, 0x95, "new_object")                                      \
    X(OP_NEW_ARRAY, 0x96, "new_array")                                        \
    X(OP_ARRAY_LOAD, 0x97, "array_load")                                      \
    X(OP_ARRAY_SAVE, 0x98, "array_save")                                      \
    X(OP_ARRAY_LENGTH, 0x99, "array_length")                                  \
    X(OP_ARRAY_PUSH, 0x9a, "array_push")                                      \
    X(OP_ARRAY_REMOVE, 0x9b, "array_remove")

#define IR_EXPRESSION_OP_LIST(X)                                               \
    X(OP_ADD, 0x01, "add")                                                    \
    X(OP_SUB, 0x02, "sub")                                                    \
    X(OP_MUL, 0x03, "mul")                                                    \
    X(OP_DIV, 0x04, "div")                                                    \
    X(OP_EQUAL, 0x05, "equal")                                                \
    X(OP_NOTEQUAL, 0x06, "notequal")                                          \
    X(OP_GREATER, 0x07, "greater")                                            \
    X(OP_LESS, 0x08, "less")                                                  \
    X(OP_EQUALGREATER, 0x09, "eqgreater")                                    \
    X(OP_EQUALLESS, 0x0a, "eqless")                                           \
    X(OP_ASSIGN, 0x0b, "assign")                                              \
    X(OP_OR, 0x0c, "or")                                                      \
    X(OP_AND, 0x0d, "and")

#define DEF_INSTRUCTION(OP_NAME, HEX, OP_STR)                                  \
    enum { OP_NAME = HEX };                                                    \
    static const char OP_NAME##_STR[] = OP_STR;

IR_INSTRUCTION_LIST(DEF_INSTRUCTION)
IR_EXPRESSION_OP_LIST(DEF_INSTRUCTION)

#define DEF_INSTRUCTION_STR(OP_NAME, HEX, OP_STR) [OP_NAME] = OP_NAME##_STR,

static const char *const IR_INSTRUCTION_STRS[256] = {
    IR_INSTRUCTION_LIST(DEF_INSTRUCTION_STR)
};

static const char *const IR_EXPRESSION_OP_STRS[256] = {
    IR_EXPRESSION_OP_LIST(DEF_INSTRUCTION_STR)
};

#define GET_INSTRUCTION_STR(OP_CODE)                                           \
    (IR_INSTRUCTION_STRS[(unsigned char)(OP_CODE)])

#define GET_EXPRESSION_OP_STR(OP_CODE)                                         \
    (IR_EXPRESSION_OP_STRS[(unsigned char)(OP_CODE)])

#undef DEF_INSTRUCTION_STR

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
    unsigned current_stack_size;
};

struct IRContext *gen_irc();

void init_irc(struct IRContext *irc, struct Node *node);
void emit_byte(struct IRContext *irc, byte _b);

void print_bytes(struct IRContext *irc);
void gen_ir(struct IRContext *irc, struct ParserContext *pc);

const byte *get_bytes(struct IRContext *irc);

#endif
