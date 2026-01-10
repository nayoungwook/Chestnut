#ifndef TYPE_H
#define TYPE_H

#include "token.h"
#include "parser.h"

#include <stdbool.h>
#include <stdint.h>
#include <wchar.h>

typedef struct {
  const wchar_t *type_str;
  void* data;  
} Type;

typedef struct {
  const wchar_t *type_str;
  uint32_t nbyte;
  unsigned rank;
  bool is_signed;
} PrimitiveType;

PrimitiveType *gen_primitive_type(const wchar_t *type_str, unsigned nbyte,
                                  unsigned rank, bool is_signed);

Type *infer_type(ParserContext *pc, Node *node);
Type *gen_type(const wchar_t *type_str, void *data);

// for @TypeCheckAttrib
typedef enum {
  IT_None,  
  IT_Var,
  IT_Func,
} IdentType;

typedef struct {
  const wchar_t *str;
  const wchar_t* type_str;  
  Type* type;  
  IdentType attr_type;
} IdentData;

// [ First Node ( type name ) ] -> [ Attr (attribute name) ] -> [Attr (attribute name) ] -> ....
typedef struct _IdentDataNode {
  IdentData* ident_data;
  struct _IdentDataNode *attr;
  bool type_checked;
} IdentDataNode;

//================= for type checker. =================

// identifier type check node
typedef struct {
  IdentDataNode *ident_data_node;
  Token* tok;
} IdentifierTCQN;

// raw type existance check node
typedef struct {
  wchar_t *type_str;
  Token* tok;  
} RawTypeTCQN;

// assign type check node
typedef struct {
  Node *left_node;
  Node *right_node;
  Token *tok;
} AssignTCQN;

typedef struct _TypeCheckContext {
  
  Queue *tc_ident_queue; // queue for type checking of identifier.
                         // stores IdentDataNode (like a, b, foo, bar ..)

  Queue *tc_type_queue; // queue for type checking of raw Type.
                        // stores wstring (like class, int, float ..).

  Queue *tc_assign_queue; // queue for type checking of assign.

} TypeCheckContext;

TypeCheckContext *gen_tcc();

IdentifierTCQN *gen_ident_tcqn(ParserContext *pc,
                               IdentDataNode *ident_data_node);
RawTypeTCQN *gen_rawtype_tcqn(ParserContext *pc,
			     wchar_t* type);
AssignTCQN *gen_assign_tcqn(ParserContext *pc, Node *left_node,
                            Node *right_node);

void resolve_tcq(ParserContext *pc, TypeCheckContext *tcc);

Type *get_type_of_attr(ParserContext *pc, Type *target, IdentData *attr);
bool check_type_exist(ParserContext *pc, const wchar_t *type);

Type *get_type_of_ident_data_node(ParserContext *pc,
				  IdentDataNode *ident_data_node);
Type *find_type(ParserContext *pc, const wchar_t *type_str);

#endif
