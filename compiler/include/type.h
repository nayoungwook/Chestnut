#ifndef TYPE_H
#define TYPE_H

#include <token.h>

#include <stdbool.h>
#include <stdint.h>
#include <wchar.h>

struct Type {
	const char *type_str;
	struct ClassData *data;

	uint32_t nbyte;
	unsigned rank;
	bool is_signed;
};

struct ParserContext;
struct Node;

struct Type *gen_primitive_type(const char *type_str, unsigned nbyte,
	unsigned rank, bool is_signed);
struct Type *gen_class_type(const char *type_str, void *data);

struct Type *infer_type(struct ParserContext *pc, struct Node *node);

bool check_type_existance(struct ParserContext *pc, const char *type);

struct Type *find_type(struct ParserContext *pc, const char *type_str);

#endif
