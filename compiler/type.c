#include "type.h"
#include "error.h"

Type *gen_type(const wchar_t *type_str, void *data) {
  Type *type = (Type *)S_malloc(sizeof(Type));

  type->type_str = wcsdup(type_str);
  type->data = data;  

  return type;
}

IdentifierTCQN *gen_ident_tcqn(ParserContext *pc,
                               IdentDataNode *ident_data_node) {
  IdentifierTCQN *result = (IdentifierTCQN *)S_malloc(sizeof(IdentifierTCQN));

  result->ident_data_node = ident_data_node;
  result->tok = peek(pc->tc);
  
  return result;  
}

RawTypeTCQN *gen_rawtype_tcqn(ParserContext *pc, wchar_t *type_str) {
  RawTypeTCQN *result = (RawTypeTCQN *)S_malloc(sizeof(RawTypeTCQN));

  result->type_str = type_str;
  result->tok = peek(pc->tc);

  return result;  
}

AssignTCQN *gen_assign_tcqn(ParserContext *pc, Node *left_node,
                            Node *right_node) {
  AssignTCQN *result = (AssignTCQN*) S_malloc(sizeof(AssignTCQN));
  result->right_node = right_node;
  result->left_node = left_node;
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
