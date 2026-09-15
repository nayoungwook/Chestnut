#ifndef TYPE_H
#define TYPE_H

#include <token.h>

#include <stdbool.h>
#include <stdint.h>

struct NumericData {
    unsigned rank;
    bool is_signed, is_integer;
};
struct NumericData *gen_numeric_data(unsigned rank, bool is_signed,
                                     bool is_integer);

union TypeData {
    struct ClassData *class_data;
    struct NumericData *numeric_data;
    struct Type *element_type;
};

enum TypeKind { TK_Numeric, TK_Class, TK_Primitive, TK_Null, TK_Array };

struct Type {
    const char *type_str;
    union TypeData data;
    enum TypeKind type_kind;

    uint32_t nbyte;
};

struct ParserContext;
struct Node;

struct Type *gen_primitive_type(const char *type_str, unsigned nbyte);
struct Type *gen_numeric_type(const char *type_str, unsigned nbyte,
                              struct NumericData *numeric_data);
struct Type *gen_class_type(const char *type_str);
struct Type *gen_array_type(const char *type_str, struct Type *element_type);
struct Type *gen_null_type();

struct Type *infer_type(struct ParserContext *pc, struct Node *node);

bool check_type_existance(struct ParserContext *pc, const char *type);

unsigned get_size_of_type(struct ParserContext *pc, struct Type *type);
struct Type *find_type(struct ParserContext *pc, const char *type_str);
bool is_castable(struct Type *from, struct Type *to);

#endif
