#include "type.h"
#include "error.h"

TypeCheckQueueNode *gen_tcqnode(TypeCheckKind kind, void *node) {
  TypeCheckQueueNode *result =
      (TypeCheckQueueNode *)S_malloc(sizeof(TypeCheckQueueNode));

  result->kind = kind;
  result->node = node;

  return result;  
}

bool check_attribute(ParserContext* pc, const wchar_t *target, IdentData* attr) {
  ClassData* cd = ht_find(pc->class_smtb, target);
  bool result = false;
  
  if (cd == NULL) {
    panic(L"Failed to find class", pc->tc);
  }

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
