#include "type.h"
#include "error.h"

Type *gen_type(const wchar_t *type_str, void *data) {
  Type *type = (Type *)S_malloc(sizeof(Type));

  type->type_str = wcsdup(type_str);
  type->data = data;  

  return type;
}

Type *find_type(ParserContext* pc, const wchar_t *str) {
  Type *result = ht_find(pc->primitive_type_smtb, str);

  if (result == NULL) {
    result = ht_find(pc->class_type_smtb, str);
  }

  return result;  
}  

TypeCheckQueueNode *gen_tcqnode(ParserContext* pc, TypeCheckKind kind, void *node) {
  TypeCheckQueueNode *result =
      (TypeCheckQueueNode *)S_malloc(sizeof(TypeCheckQueueNode));

  result->kind = kind;
  result->node = node;
  result->tok = peek(pc->tc);

  return result;  
}

bool check_attribute(ParserContext* pc, const wchar_t *target, IdentData* attr) {
  Type *c_type = ht_find(pc->class_type_smtb, target);
  
  if (c_type == NULL) {
    panic(L"Failed to find class type", pc->tc);
  }
  
  ClassData* cd = (ClassData*) c_type->data;
  assert(cd != NULL);
  
  bool result = false;

  switch (attr->attr_type) {
  case IT_Var:
    if (ht_find(cd->member_vars, attr->str) != NULL) {
      result = true;
    }      
    break;

  case IT_Func:
    if (ht_find(cd->member_funcs, attr->str) != NULL) {
      result = true;
    }
    break;

  default:
    wprintf(L"Warning! there are another attrib type.\n");
    break;    
  }    
  
  return result;
}

bool check_type_exists(ParserContext* pc, const wchar_t *type) {
  return false;
}  
