#ifndef TYPE_H
#define TYPE_H

#include "token.h"
#include "parser.h"

#include <stdbool.h>
#include <wchar.h>

typedef struct {
  const wchar_t *type_str;
  void* data;  
} Type;

Type *infer_type(Node *node);
Type *gen_type(const wchar_t *type_str, void *data);
Type *find_type(ParserContext *pc, const wchar_t *str);

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

IdentifierTCQN *gen_ident_tcqn(ParserContext *pc,
                               IdentDataNode *ident_data_node);
RawTypeTCQN *gen_rawtype_tcqn(ParserContext *pc,
			     wchar_t* type);
AssignTCQN *gen_assign_tcqn(ParserContext *pc, Node *left_node,
                            Node *right_node);

void resolve_tcq(ParserContext *pc, TypeCheckContext *tcc);

Type *get_type_of_attr(ParserContext *pc, Type *target, IdentData *attr);
bool check_type_exist(ParserContext* pc, const wchar_t* type);

#endif
