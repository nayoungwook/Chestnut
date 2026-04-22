#ifndef SEMANTICS_H
#define SEMANTICS_H

#include <stdbool.h>

struct ParserContext;
struct Type;
struct Node;

struct FuncData *find_func_data(struct ParserContext *pc, const char *func_name);
struct ClassData *find_class_data(struct ParserContext *pc, struct Type *type);

struct FuncData *gen_func_data(const char *func_name, struct Type *ret_type, unsigned id, bool varargs);

struct FuncData *register_constructor_data(const char *func_name, struct ParserContext *pc);
struct FuncData *register_func_data(const char *func_name, struct Type *ret_type, struct ParserContext *pc);
struct ClassData *register_class_data(const char *class_name, struct ParserContext *pc);
struct VarData *register_local_var_data(const char *name, struct Type *type, struct ParserContext *pc);
struct VarData *register_non_local_var_data(const char *name, struct Type *type, struct ParserContext *pc);

void check_semantics(struct ParserContext *pc, struct Node *node);

#endif
