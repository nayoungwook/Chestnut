#include <type.h>
#include <parser.h>
#include <error.h>

#define DEBUG

static unsigned resolve_raw_type_tcq(struct ParserContext *pc,
                                     struct TypeCheckContext *tcc);
static unsigned resolve_identifier_tcq(struct ParserContext *pc,
                                       struct TypeCheckContext *tcc);
static unsigned resolve_identifier_tcq(struct ParserContext *pc,
                                       struct TypeCheckContext *tcc);
static unsigned resolve_assign_tcq(struct ParserContext *pc,
                                   struct TypeCheckContext *tcc);

struct Type *infer_type(struct ParserContext *pc, struct Node *node) {  
  struct Type* result = NULL;
  const char *ident = "";
  
  assert(node->ast != NULL);

  switch (node->type) {
  case AST_FunctionCall: {
    struct FuncCallAST *func_call_ast = (struct FuncCallAST *)node->ast;    
    struct IdentDataNode *ident_data_node =
      (func_call_ast)->ident_data_node;

    result = get_type_of_ident_data_node(pc, ident_data_node);
    ident = ident_data_node->ident_data->str;
    
    break;
  }

  case AST_Identifier: {
    struct IdentDataNode *ident_data_node =
        ((struct IdentifierAST *)node->ast)->ident_data_node;
    
    result = get_type_of_ident_data_node(pc, ident_data_node);
    ident = ident_data_node->ident_data->str;
    
    break;    
  }    
    
  default:
    break;
  }

  if (result == NULL) {
    char err_buf[512];
    sprintf(err_buf, "Failed to infer type of identifier : %s", ident);
    printf("%s\n", err_buf);
  }    
  
  return result;  
}

struct PrimitiveType *gen_primitive_type(const char *type_str, unsigned nbyte,
                                  unsigned rank, bool is_signed) {
  struct PrimitiveType *result = (struct PrimitiveType *)S_malloc(sizeof(struct PrimitiveType));

  result->type_str = type_str;
  result->nbyte = nbyte;
  result->rank = rank;
  result->is_signed = is_signed;

  return result;  
}

struct Type *gen_type(const char *type_str, void *data) {
  struct Type *type = (struct Type *)S_malloc(sizeof(struct Type));  

  type->type_str = type_str;
  type->data = data;  

  return type;
}

struct TypeCheckContext *gen_tcc() {
  struct TypeCheckContext *tcc =
      (struct TypeCheckContext *)S_malloc(sizeof(struct TypeCheckContext));  

  tcc->tc_type_queue = gen_queue();
  tcc->tc_ident_queue = gen_queue();  
  tcc->tc_assign_queue = gen_queue();  
  
  return tcc;
}  

struct IdentifierTCQN *gen_ident_tcqn(struct ParserContext *pc,
				      struct IdentDataNode *ident_data_node) {
  struct IdentifierTCQN *result =
      (struct IdentifierTCQN *)S_malloc(sizeof(struct IdentifierTCQN)); 

  result->ident_data_node = ident_data_node;
  result->tok = peek(pc->tc);
  
  return result;  
}

struct RawTypeTCQN *gen_rawtype_tcqn(struct ParserContext *pc,
                                     const char *type_str) {  
  struct RawTypeTCQN *result =
      (struct RawTypeTCQN *)S_malloc(sizeof(struct RawTypeTCQN));  

  result->type_str = type_str;
  result->tok = peek(pc->tc);

  return result;  
}

struct AssignTCQN *gen_assign_tcqn(struct ParserContext *pc,
                                   struct Node *left_node,
                                   struct Node *right_node) {  
  struct AssignTCQN *result =
      (struct AssignTCQN *)S_malloc(sizeof(struct AssignTCQN));  
  result->right_node = right_node;
  result->left_node = left_node;
  result->tok = peek(pc->tc);
  
  return result;  
}

static unsigned resolve_raw_type_tcq(struct ParserContext *pc,
                                     struct TypeCheckContext *tcc) {  
  unsigned err_cnt = 0;

  while (tcc->tc_type_queue->size != 0) {
    struct RawTypeTCQN *raw_type_tcqn = q_pop(tcc->tc_type_queue);    

#ifdef DEBUG    
    printf("Check type existance : %s\n", raw_type_tcqn->type_str);
#endif
    
    if (!check_type_existance(pc, raw_type_tcqn->type_str)) {
      err_cnt++;      
    }
  }

  return err_cnt;  
}

static void resolve_attr_tcq(struct ParserContext *pc, struct Type *type,
                             struct IdentDataNode *ident_data_node) {  
  struct IdentData *ident_data = ident_data_node->ident_data;
  struct IdentDataNode *first_ident_data_node = ident_data_node;
  
  while (true) {
    ident_data_node = ident_data_node->attr;

    if (ident_data_node == NULL) { // attr search done.
      break;
    }

#ifdef DEBUG
    printf("Check attr of %s -> %s\n", type->type_str,
            ident_data_node->ident_data->str);
#endif    
      
    ident_data = ident_data_node->ident_data;

#ifdef DEBUG
    struct Type *type_cache = type;
#endif
    
    if ((type = get_type_of_attr(pc, type, ident_data)) ==
        NULL) { // attr type not exist.
#ifdef DEBUG
      printf("Type %s does not contains %s\n", type_cache->type_str,
             ident_data->str);
      exit(0);
#endif

      break;
    }

    // this will update type of ident data.    
    ident_data->type = type;
  }

  first_ident_data_node->type_checked = true;
}

static unsigned resolve_identifier_tcq(struct ParserContext *pc,
                                       struct TypeCheckContext *tcc) {  
  unsigned err_cnt = 0;
  
  while (tcc->tc_ident_queue->size != 0) {
    struct IdentifierTCQN *ident_tcqn = q_pop(tcc->tc_ident_queue);
    assert(ident_tcqn != NULL && ident_tcqn->ident_data_node != NULL);

    struct IdentDataNode *ident_data_node = ident_tcqn->ident_data_node;    

    struct Type *type = get_type_of_ident_data_node(pc, ident_data_node);

#ifdef DEBUG
    struct IdentData *ident_data = ident_data_node->ident_data;
    printf("Check type of identifier : %s | type : %s\n", ident_data->str,
            type->type_str);
#endif    
    
    if (type == NULL) {
      err_cnt++;
      continue;
    }

  }
  
  return err_cnt;
}

static unsigned resolve_assign_tcq(struct ParserContext *pc,
                                   struct TypeCheckContext *tcc) {  
  unsigned err_cnt = 0;

  while (tcc->tc_assign_queue->size != 0) {
    struct AssignTCQN *assign_tcqn = q_pop(tcc->tc_assign_queue);

    assert(assign_tcqn != NULL);
    assert(assign_tcqn->left_node != NULL && assign_tcqn->right_node != NULL);
    assert(assign_tcqn->tok != NULL);

    struct Type *left_type = infer_type(pc, assign_tcqn->left_node);
    struct Type *right_type = infer_type(pc, assign_tcqn->right_node);    

#ifdef DEBUG
    printf("Check If types are assignable. %s : %s\n", left_type->type_str, right_type->type_str);
#endif

  }    

  return err_cnt;  
}

struct Type *find_type(struct ParserContext *pc, const char *type_str) {  
  struct Type *result = ht_find(pc->primitive_type_smtb, type_str);

  if (result == NULL) {
    result = ht_find(pc->class_type_smtb, type_str);
  }

  return result;
}

void resolve_tcq(struct ParserContext *pc, struct TypeCheckContext *tcc) {  
  //  pc->tc_assign_queue;
  //  pc->tc_ident_queue;
  //  pc->tc_type_queue;

  unsigned err_cnt = 0;

  err_cnt += resolve_raw_type_tcq(pc, tcc);
  err_cnt += resolve_identifier_tcq(pc, tcc);
  err_cnt += resolve_assign_tcq(pc, tcc);
  
  if (err_cnt > 0){
    printf("Type error occured! : %d\n", err_cnt);
  }
}

struct Type *get_type_of_attr(struct ParserContext *pc, struct Type *target,
                              struct IdentData *attr) {
  struct ClassData *cd = (struct ClassData *)target->data;
  assert(cd != NULL);
  
  switch (attr->attr_type) {
  case IT_Var:{
    struct VarData *vd = NULL;

    if ((vd = ht_find(cd->member_vars, attr->str)) != NULL) {
      return find_type(pc, vd->type);
    }

    return NULL;    
  }    

  case IT_Func:{
    struct FuncData *fd = NULL;
    
    if ((fd = ht_find(cd->member_funcs, attr->str)) != NULL) {
      return find_type(pc, fd->return_type);
    }

    return NULL;    
  }
    
  default:
    printf("Warning! there are another attrib type.\n");
    return NULL;    
  }    
}

struct Type *
get_type_of_ident_data_node(struct ParserContext *pc,
                            struct IdentDataNode *ident_data_node) {  
  struct IdentData *ident_data = ident_data_node->ident_data;

  assert(ident_data != NULL);

  const char* str = ident_data->type_str;
  
  struct Type *result = find_type(pc, str);

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

bool check_type_existance(struct ParserContext *pc, const char *type) {  
  return find_type(pc, type) != NULL || strcmp(type, "void") == 0;
}  
