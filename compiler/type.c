#include "type.h"
#include "error.h"
#include "parser.h"

Type *infer_type(Node *node) {
  Type* result = NULL;

  switch(node->type) {
    
  }    
  
  return result;  
}  

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

void resolve_tcq(ParserContext *pc) {
  //  pc->tc_assign_queue;
  //  pc->tc_ident_queue;
  //  pc->tc_type_queue;

  unsigned err_cnt = 0;  
  
  while (pc->tc_type_queue->size != 0) {
    RawTypeTCQN *raw_type_tcqn = q_pop(pc->tc_type_queue);
    
    wprintf(L"Check type existance : %S\n", raw_type_tcqn->type_str);
    
    if (!check_type_exist(pc, raw_type_tcqn->type_str)) {
      err_cnt++;      
    }
  }

  while (pc->tc_ident_queue->size != 0) {
    IdentifierTCQN *ident_tcqn = q_pop(pc->tc_ident_queue);
    assert(ident_tcqn != NULL && ident_tcqn->ident_data_node != NULL);

    IdentDataNode *ident_data_node = ident_tcqn->ident_data_node;    
    IdentData *ident_data = ident_tcqn->ident_data_node->ident_data;

    assert(ident_data->type_str != NULL);

    Type *type = find_type(pc, ident_data->type_str);

    wprintf(L"Check type of identifier : %S | type : %S\n", ident_data->str,
            type->type_str);
    
    if (type == NULL) {
      err_cnt++;
      continue;
    }

    while (true) {
      ident_data_node = ident_data_node->attr;

      if (ident_data_node == NULL) { // attr search done.
	break;        
      }        
      
      ident_data = ident_data_node->ident_data;
      
      if ((type = get_type_of_attr(pc, type, ident_data)) == NULL) { // attr type not exist.
	err_cnt++;        
        break;
      }
    }    
  }
  
  if (err_cnt > 0){
    wprintf(L"Type error occured! : %d\n", err_cnt);
  }
}

Type* get_type_of_attr(ParserContext* pc, Type *target, IdentData* attr) {

  ClassData *cd = (ClassData *)target->data;
  assert(cd != NULL);
  
  switch (attr->attr_type) {
  case IT_Var:{
    VarData* var_data = NULL;
    if ((var_data = ht_find(cd->member_vars, attr->str)) != NULL) {
      assert(var_data->node->type == AST_VariableDeclaration);
      
      VarDeclAST *var_decl_ast = (VarDeclAST *)var_data->node->ast;
      return find_type(pc, var_decl_ast->var_type_tok->str);
    }

    return NULL;    
  }    

  case IT_Func:{
    FuncData* func_data = NULL;
    if ((func_data = ht_find(cd->member_funcs, attr->str)) != NULL) {
      assert(func_data->node->type == AST_FunctionDeclaration);

      FuncDeclAST *func_decl_ast = (FuncDeclAST *)func_data->node->ast;
      return find_type(pc, func_decl_ast->ret_type_tok->str);
    }

    return NULL;    
  }
    
  default:
    wprintf(L"Warning! there are another attrib type.\n");
    return NULL;    
  }    
}

bool check_type_exist(ParserContext* pc, const wchar_t *type) {
  return find_type(pc, type) != NULL || wcscmp(type, L"void") == 0;
}  
