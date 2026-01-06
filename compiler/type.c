#include "type.h"

Type *infer_type(Node *node) {
  Type* result = NULL;

  switch(node->type) {
  default:
    return NULL;    
  }    
  
  return result;  
}  

Type *gen_type(const wchar_t *type_str, void *data) {
  Type *type = (Type *)S_malloc(sizeof(Type));

  type->type_str = wcsdup(type_str);
  type->data = data;  

  return type;
}

TypeCheckContext *gen_tcc() {
  TypeCheckContext *tcc =
      (TypeCheckContext *)S_malloc(sizeof(TypeCheckContext));

  tcc->tc_type_queue = gen_queue();
  tcc->tc_ident_queue = gen_queue();  
  tcc->tc_assign_queue = gen_queue();  
  
  return tcc;
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

static unsigned resolve_raw_type_tcq(ParserContext *pc, TypeCheckContext *tcc) {
  unsigned err_cnt = 0;
  
  while (tcc->tc_type_queue->size != 0) {
    RawTypeTCQN *raw_type_tcqn = q_pop(tcc->tc_type_queue);

#ifdef DEBUG    
    wprintf(L"Check type existance : %S\n", raw_type_tcqn->type_str);
#endif
    
    if (!check_type_exist(pc, raw_type_tcqn->type_str)) {
      err_cnt++;      
    }
  }

  return err_cnt;  
}

static unsigned resolve_attr_tcq(ParserContext *pc, Type *type, IdentDataNode *ident_data_node) {
  unsigned err_cnt = 0;
  IdentData *ident_data = ident_data_node->ident_data;
  
  while (true) {
    ident_data_node = ident_data_node->attr;

    if (ident_data_node == NULL) { // attr search done.
      break;
    }
    
    wprintf(L"Check attr of %S -> %S\n", type->type_str, ident_data_node->ident_data->str); 
      
    ident_data = ident_data_node->ident_data;
      
    if ((type = get_type_of_attr(pc, type, ident_data)) == NULL) { // attr type not exist.
      err_cnt++;        
      break;
    }
  }

  return err_cnt;    
}

static unsigned resolve_identifier_tcq(ParserContext *pc, TypeCheckContext *tcc) {
  unsigned err_cnt = 0;
  
  while (tcc->tc_ident_queue->size != 0) {
    IdentifierTCQN *ident_tcqn = q_pop(tcc->tc_ident_queue);
    assert(ident_tcqn != NULL && ident_tcqn->ident_data_node != NULL);

    IdentDataNode *ident_data_node = ident_tcqn->ident_data_node;    
    IdentData *ident_data = ident_data_node->ident_data;

    assert(ident_data->type_str != NULL);

    Type *type = find_type(pc, ident_data->type_str);

    wprintf(L"Check type of identifier : %S | type : %S\n", ident_data->str,
            type->type_str);
    
    if (type == NULL) {
      err_cnt++;
      continue;
    }

    err_cnt += resolve_attr_tcq(pc, type, ident_data_node);
  }
  
  return err_cnt;
}

static unsigned resolve_assign_tcq(ParserContext *pc, TypeCheckContext *tcc) {
  unsigned err_cnt = 0;

  while (tcc->tc_assign_queue->size != 0) {
    AssignTCQN *assign_tcqn = q_pop(tcc->tc_assign_queue);

    assert(assign_tcqn != NULL);
    assert(assign_tcqn->left_node != NULL && assign_tcqn->right_node != NULL);
    assert(assign_tcqn->tok != NULL);

    Type *left_type = infer_type(assign_tcqn->left_node);
    Type *right_type = infer_type(assign_tcqn->right_node);
    
    // TODO : check assignable
  }    

  return err_cnt;  
}  

void resolve_tcq(ParserContext *pc, TypeCheckContext *tcc) {
  //  pc->tc_assign_queue;
  //  pc->tc_ident_queue;
  //  pc->tc_type_queue;

  unsigned err_cnt = 0;

  err_cnt += resolve_raw_type_tcq(pc, tcc);
  err_cnt += resolve_identifier_tcq(pc, tcc);
  err_cnt += resolve_assign_tcq(pc, tcc);
  
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
