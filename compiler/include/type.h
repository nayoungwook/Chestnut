#ifndef TYPE_H
#define TYPE_H

#include <token.h>

#include <stdbool.h>
#include <stdint.h>
#include <wchar.h>

struct Type {
	const char *type_str;
	void *data;

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

// for @TypeCheckAttrib
enum IdentType {
	IT_None,  
	IT_Var,
	IT_Func,
};

struct IdentData {
	const char *str;
	const char *type_str;
	struct Type *type;
	enum IdentType attr_type;
};

// [ First Node ( type name ) ] -> [ Attr (attribute name) ] -> [Attr (attribute name) ] -> ....
struct IdentDataNode {
	struct IdentData *ident_data;
	struct IdentDataNode *attr;
	bool type_checked;
};

//================= for type checker. =================

// identifier type check node
struct IdentifierTCQN {
	struct IdentDataNode *ident_data_node;
	struct Token *tok;
};

// raw type existance check node
struct RawTypeTCQN {
	const char *type_str;
	struct Token *tok;
};

// assign type check node
struct AssignTCQN {
	struct Node *left_node;
	struct Node *right_node;
	struct Token *tok;
};

struct TypeCheckContext {
	struct Queue *tc_ident_queue; // queue for type checking of identifier.
	// stores IdentDataNode (like a, b, foo, bar ..)

	struct Queue *tc_type_queue; // queue for type checking of type existance..

	struct Queue *tc_assign_queue; // queue for type checking of assign.
};

struct TypeCheckContext *gen_tcc();

struct IdentifierTCQN *gen_ident_tcqn(struct ParserContext *pc,
									  struct IdentDataNode *ident_data_node);
struct RawTypeTCQN *gen_rawtype_tcqn(struct ParserContext *pc,
									 const char* type);
struct AssignTCQN *gen_assign_tcqn(struct ParserContext *pc, struct Node *left_node,
								   struct Node *right_node);

void resolve_tcq(struct ParserContext *pc, struct TypeCheckContext *tcc);

struct Type *get_type_of_attr(struct ParserContext *pc, struct Type *target, struct IdentData *attr);
bool check_type_existance(struct ParserContext *pc, const char *type);

struct Type *get_type_of_ident_data_node(struct ParserContext *pc,
										 struct IdentDataNode *ident_data_node);
struct Type *find_type(struct ParserContext *pc, const char *type_str);

typedef struct {
  
	struct Queue *tc_ident_queue; // queue for type checking of identifier.
	// stores IdentDataNode (like a, b, foo, bar ..)
	struct Queue *tc_type_queue; // queue for type checking of raw Type.
	// stores wstring (like class, int, float ..).
	struct Queue *tc_assign_queue; // queue for type checking of assign.
  
} TypeCheckContext;

#endif
