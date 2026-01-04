#ifndef TYPE_H
#define TYPE_H

#include "parser.h"

#include <stdbool.h>
#include <wchar.h>

typedef struct {
  const wchar_t *type_str;
  void* data;  
} Type;

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

// for type checker.
typedef enum {
  TCK_CheckTypeExist,
  TCK_CheckAssignable,  
} TypeCheckKind;  

typedef struct {
  TypeCheckKind kind;
  void *node;
  Token* tok;
} TypeCheckQueueNode;

TypeCheckQueueNode* gen_tcqnode(ParserContext* pc, TypeCheckKind kind, void* node);

bool check_attribute(ParserContext* pc, const wchar_t *target, IdentData* attr);
bool check_type_exists(ParserContext* pc, const wchar_t* type);

#endif
