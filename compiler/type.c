#include "type.h"
#include "parser.h"

static unsigned resolve_raw_type_tcq(ParserContext *pc, TypeCheckContext *tcc);
static unsigned resolve_identifier_tcq(ParserContext *pc,
                                       TypeCheckContext *tcc);
static unsigned resolve_identifier_tcq(ParserContext *pc, TypeCheckContext *tcc);
static unsigned resolve_assign_tcq(ParserContext *pc, TypeCheckContext *tcc);

Type *infer_type(ParserContext *pc, Node *node) {
  Type* result = NULL;

  assert(node->ast != NULL);
  
  switch (node->type) {
  case AST_FunctionCall:{
    IdentDataNode *ident_data_node =
      ((FuncCallAST *)node->ast)->ident_data_node;

    result = get_type_of_ident_data_node(pc, ident_data_node);
    
    break;
  }

  case AST_Identifier: {
    IdentDataNode *ident_data_node =
        ((IdentifierAST *)node->ast)->ident_data_node;
    
    result = get_type_of_ident_data_node(pc, ident_data_node);
    
    break;    
  }    
    
  default:
    break;
  }

  assert(result != NULL);  
  
  return result;  
}

PrimitiveType *gen_primitive_type(const wchar_t *type_str, unsigned nbyte,
                                  unsigned rank, bool is_signed) {
  PrimitiveType *result = (PrimitiveType *)S_malloc(sizeof(PrimitiveType));

  result->type_str = type_str;
  result->nbyte = nbyte;
  result->rank = rank;
  result->is_signed = is_signed;

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

    if(wcscmp(raw_type_tcqn->type_str, L"void")) continue; // pass 'void' type.
    
#ifdef DEBUG    
    wprintf(L"Check type existance : %S\n", raw_type_tcqn->type_str);
#endif
    
    if (!check_type_exist(pc, raw_type_tcqn->type_str)) {
      err_cnt++;      
    }
  }

  return err_cnt;  
}

static void resolve_attr_tcq(ParserContext *pc, Type *type, IdentDataNode *ident_data_node) {
  IdentData *ident_data = ident_data_node->ident_data;
  IdentDataNode *first_ident_data_node = ident_data_node;
  
  while (true) {
    ident_data_node = ident_data_node->attr;

    if (ident_data_node == NULL) { // attr search done.
      break;
    }

#ifdef DEBUG
    wprintf(L"Check attr of %S -> %S\n", type->type_str,
            ident_data_node->ident_data->str);
#endif    
      
    ident_data = ident_data_node->ident_data;

#ifdef DEBUG
    Type *type_cache = type;
#endif
    
    if ((type = get_type_of_attr(pc, type, ident_data)) ==
        NULL) { // attr type not exist.
#ifdef DEBUG      
      wprintf(L"Type %S does not contains %S\n", type_cache->type_str, ident_data->str);
#endif

      break;
    }

    // this will update type of ident data.    
    ident_data->type = type;
  }

  first_ident_data_node->type_checked = true;
}

static unsigned resolve_identifier_tcq(ParserContext *pc, TypeCheckContext *tcc) {
  unsigned err_cnt = 0;
  
  while (tcc->tc_ident_queue->size != 0) {
    IdentifierTCQN *ident_tcqn = q_pop(tcc->tc_ident_queue);
    assert(ident_tcqn != NULL && ident_tcqn->ident_data_node != NULL);

    IdentDataNode *ident_data_node = ident_tcqn->ident_data_node;    

    Type *type = get_type_of_ident_data_node(pc, ident_data_node);

#ifdef DEBUG
    IdentData *ident_data = ident_data_node->ident_data;
    wprintf(L"Check type of identifier : %S | type : %S\n", ident_data->str,
            type->type_str);
#endif    
    
    if (type == NULL) {
      err_cnt++;
      continue;
    }

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

    Type *left_type = infer_type(pc, assign_tcqn->left_node);
    Type *right_type = infer_type(pc, assign_tcqn->right_node);

#ifdef DEBUG
    wprintf(L"Check If types are assignable. %S : %S\n", left_type->type_str, right_type->type_str);
#endif

  }    

  return err_cnt;  
}

Type *find_type(ParserContext *pc, const wchar_t *type_str) {
  Type *result = ht_find(pc->primitive_type_smtb, type_str);

  if (result == NULL) {
    result = ht_find(pc->class_type_smtb, type_str);
  }

  return result;
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

Type *get_type_of_attr(ParserContext *pc, Type *target, IdentData *attr) {
  
  ClassData *cd = (ClassData *)target->data;
  assert(cd != NULL);
  
  switch (attr->attr_type) {
  case IT_Var:{
    VarData *vd = NULL;

    if ((vd = ht_find(cd->member_vars, attr->str)) != NULL) {
      return find_type(pc, vd->type);
    }

    return NULL;    
  }    

  case IT_Func:{
    FuncData *fd = NULL;
    
    if ((fd = ht_find(cd->member_funcs, attr->str)) != NULL) {
      return find_type(pc, fd->return_type);
    }

    return NULL;    
  }
    
  default:
    wprintf(L"Warning! there are another attrib type.\n");
    return NULL;    
  }    
}

Type *get_type_of_ident_data_node(ParserContext *pc,
				  IdentDataNode *ident_data_node) {
  IdentData *ident_data = ident_data_node->ident_data;

  assert(ident_data != NULL);

  const wchar_t* str = ident_data->type_str;
  
  Type *result = find_type(pc, str);

  // if all types all not checked, we have to resolve attr data.  
  if(!ident_data_node->type_checked)
    resolve_attr_tcq(pc, result, ident_data_node);

  while (ident_data_node->attr != NULL) {
    ident_data_node = ident_data_node->attr;      
    ident_data = ident_data_node->ident_data;
    result = ident_data->type;
  }
  
  return result;
}

bool check_type_exist(ParserContext* pc, const wchar_t *type) {
  return find_type(pc, type) != NULL || wcscmp(type, L"void") == 0;
}  
