#ifndef TYPE_H
#define TYPE_H

#include "parser.h"

#include <stdbool.h>
#include <wchar.h>

// for @TypeCheckAttrib
typedef enum {
  IT_None,  
  IT_Var,
  IT_Func,
} IdentType;

typedef struct {
  const wchar_t *str;
  IdentType attr_type;
} IdentData;

// [ First Node ( type name ) ] -> [ Attr (attribute name) ] -> [Attr (attribute name) ] -> ....
typedef struct _IdentNode {
  IdentData* ident_data;
  struct _IdentNode* attr;
} IdentNode;

// for type checker.
typedef enum {
  TCK_CheckTypeExist,
} TypeCheckKind;  

typedef struct {
  TypeCheckKind kind;
  void* node;  
} TypeCheckQueueNode;

TypeCheckQueueNode* gen_tcqnode(TypeCheckKind kind, void* node);

bool check_attribute(ParserContext* pc, const wchar_t *target, IdentData* attr);
bool check_type_exists(ParserContext* pc, const wchar_t* type);

#endif
