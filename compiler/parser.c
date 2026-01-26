#include <parser.h>
#include <error.h>

static struct Node *parse_term(struct ParserContext *pc);
static struct Node *parse_simple_expression(struct ParserContext *pc);
static struct Node *parse_unary_expression(struct ParserContext *pc);
static struct Node *parse_compare_expression(struct ParserContext* pc);
static struct Node *parse_expression(struct ParserContext *pc);

static void init_primitive(struct ParserContext *pc) {
  // char < int < uint < float < double
  ht_insert(pc->primitive_type_smtb, "int",
            gen_primitive_type("int", 4, 2, true));

  ht_insert(pc->primitive_type_smtb, "char",
            gen_primitive_type("char", 2, 1, false));

  ht_insert(pc->primitive_type_smtb, "float",
            gen_primitive_type("float", 4, 4, true));

  ht_insert(pc->primitive_type_smtb, "double",
            gen_primitive_type("double", 8, 5, true));
  
  ht_insert(pc->primitive_type_smtb, "bool",
            gen_primitive_type("bool", 1, -1, false));

  ht_insert(pc->primitive_type_smtb, "void",
            gen_primitive_type("void", 0, -1, false));
}  

struct ParserContext *gen_pc() {
  struct ParserContext *pc = (struct ParserContext *)S_malloc(sizeof(struct ParserContext));

  pc->tc = NULL;

  pc->current_scope = NULL;
  pc->current_class = NULL;
  pc->current_func = NULL;
  
  pc->glob_func_smtb = gen_htable();
  pc->glob_var_smtb = gen_htable();
  
  pc->class_type_smtb = gen_htable();
  pc->primitive_type_smtb = gen_htable();

  pc->class_data_cnt = 0;
  pc->func_data_cnt = 0;

  pc->nodes = NULL;
  pc->node_size = 0;
  pc->node_capacity = 1;

  init_primitive(pc);  
  
  return pc;
}

void compile_file(struct ParserContext *pc, struct TokenizerContext *tc,
                  struct TypeCheckContext *tcc) {  
  struct Node *node = NULL;

  pc->tc = tc;
  pc->tcc = tcc;

  while ((node = parse(pc, false)) != NULL) {
    if (pc->node_size + 1 >= pc->node_capacity) {
      pc->node_capacity *= 2;
      pc->nodes = (struct Node **)S_realloc(pc->nodes, sizeof(struct Node *) *
                                                           pc->node_capacity);
    }

    pc->nodes[pc->node_size++] = node;    
  }

  pc->tc = NULL;  
  free(tc);  
}  

static struct Node* pack(enum ASTType type, void* ptr){
  struct Node* result = (struct Node*) S_malloc(sizeof(struct Node));
  
  result->ast = ptr;
  result->type = type;
  
  return result;
}

static void parse_func_call_params(struct ParserContext *pc, struct FuncCallAST* func_call) {
  struct TokenizerContext *tc = pc->tc;

  unsigned capacity = 1, size = 0;
  struct Node** params = (struct Node**) S_malloc(sizeof(struct Node*) * capacity);
  
  consume(tc, TokLParen);

  while(peek(tc)->type != TokRParen){
    struct Node* expr = parse_expression(pc);

    if(size + 1 >= capacity) {
      capacity *= 2;
      params = (struct Node**) S_realloc(params, sizeof(struct Node*) * capacity);
    }

    params[size++] = expr;
    
    if(peek(tc)->type == TokComma){
      consume(tc, TokComma);
    }
  }
  
  func_call->params = params;
  func_call->param_size = size;

  consume(tc, TokRParen);
}  

static struct Node* gen_func_call_node(struct Token* first, struct ParserContext* pc, bool is_expr){
  struct FuncCallAST* func_call = (struct FuncCallAST*) S_malloc(sizeof(struct FuncCallAST));
  struct TokenizerContext* tc = pc->tc;

  parse_func_call_params(pc, func_call);  
  
  func_call->func_name_tok = first;

  if(!is_expr){
    consume(tc, TokSemiColon);
  }
  
  return pack(AST_FunctionCall, func_call);
}

/*
  AttribNode -> this identifier node will be attribute of "attr_of" node.
  [first identifier] ... [attr_of] -> [attr_of] -> [gen_ident_node]
  We have to check attribute and type validation after the parsing.
*/
static struct IdentData *gen_ident_data(const char *ident, enum IdentType attr_type) {
  struct IdentData *ident_data = (struct IdentData *)S_malloc(sizeof(struct IdentData));

  ident_data->type_str = "";
  ident_data->type = NULL;
  ident_data->attr_type = attr_type;
  ident_data->str = ident;

  return ident_data;  
}

static struct IdentDataNode *
gen_ident_data_node(const char *ident, enum IdentType ident_type,
                    struct IdentDataNode *attr_of) {
  struct IdentDataNode *ident_data_node = (struct IdentDataNode *)S_malloc(sizeof(struct IdentDataNode));
  ident_data_node->ident_data = gen_ident_data(ident, ident_type);
  ident_data_node->attr = NULL;
  ident_data_node->type_checked = false;  
  
  if (attr_of != NULL) {
    // This identifier will be "attr_node" of "attr_of"
    attr_of->attr = ident_data_node;
  }

  return ident_data_node;
}

void free_ident_node(struct IdentDataNode* ident_data_node) {
  free(ident_data_node->ident_data);
  free(ident_data_node);
}

static struct VarData *find_var_data(struct ParserContext *pc, const char *var_name) {
  struct VarData *result = NULL;

  if (pc->current_scope != NULL) { // first find in local
    struct Scope *scope_searcher = pc->current_scope;
    while (scope_searcher != NULL) {
      result = (struct VarData *)ht_find(scope_searcher->local_var_smtb, var_name);

      if (result != NULL)
        break;

      scope_searcher = scope_searcher->prev_scope;
    }      
  }

  if (result == NULL && pc->current_class != NULL) { // and find in class
    result = (struct VarData*) ht_find(pc->current_class->member_vars, var_name);
  }

  if (result == NULL) {
    result = ht_find(pc->glob_var_smtb, var_name);    
  }    
  
  return result;
}

static struct FuncData *find_func_data(struct ParserContext* pc, const char *func_name) {
  struct ClassData *current_class = pc->current_class;
  struct FuncData *result = NULL;
  
  if (current_class != NULL) { // find in class.
    result = ht_find(current_class->member_funcs, func_name);
  }

  if (result == NULL) { // find in glob
    result = ht_find(pc->glob_func_smtb, func_name);
  }

  return result;  
} 

static const char *get_type_of_identifier(struct ParserContext *pc, enum IdentType ident_type,
                                    const char *str) {
  const char *result = NULL;
  
  switch (ident_type) {
  case IT_Var: {
    struct VarData *var = find_var_data(pc, str);
    if (var == NULL) {
      panic("Failed to find variable.", pc->tc);
    }

    result = var->type;
    break;
  }

  case IT_Func: {
    struct FuncData *func = find_func_data(pc, str);
    
    if (func == NULL) {
      panic("Failed to find function.", pc->tc);
    }

    result = func->return_type;
    break;
  }
      
  default:
    break;
  }

  return result;
}

static void get_first_node_type(struct ParserContext *pc, const char *ident_str,
                                enum IdentType ident_type,
                                struct IdentDataNode *ident_data_node,
                                struct IdentDataNode *attr_of) {
  assert(ident_data_node != NULL);
  
  // If it is first node, we have to check type of identifier.
  // This is first identifier we put typechek on queue.
  if (attr_of == NULL) {
    const char* type_str = get_type_of_identifier(pc, ident_type, ident_str);
    struct TypeCheckContext *tcc = pc->tcc;

    assert(tcc != NULL);    
    
    ident_data_node->ident_data->type_str = type_str;
    
    q_push(tcc->tc_ident_queue, gen_ident_tcqn(pc, ident_data_node));
  }
}

static struct Node *gen_ident_node(struct Token *first, struct ParserContext *pc, struct IdentDataNode *attr_of,
                            bool is_expr);

static void parse_attribute(struct ParserContext *pc, struct Node* result, struct IdentDataNode* ident_node, bool is_expr) {
  struct TokenizerContext* tc = pc->tc;
  struct Token *nt = NULL;
  
  if ((nt = peek(tc))->type == TokDot) {
    consume(tc, TokDot);

    struct Node *attr = gen_ident_node(pull(tc), pc, ident_node, is_expr);
    result->attr = attr;
  }
}

// return binary expr ast node if it is assign expression.
static struct Node *check_assign(struct ParserContext *pc, struct IdentDataNode *ident_node,
				 struct IdentDataNode *attr_of, struct Node *result) {
  // if it is not first node, return
  if (attr_of != NULL) {
    return result;
  }
  
  struct TokenizerContext* tc = pc->tc;
  struct Token *nt = peek(tc);

  if (nt->type != TokAssign) {
    return result;
  }
  
  consume(tc, TokAssign);

  struct Node* expr = parse_expression(pc);
  consume(tc, TokSemiColon);

  struct TypeCheckContext *tcc = pc->tcc;

  assert(tcc != NULL);  
  
  struct BinExprAST *bin_expr_ast = (struct BinExprAST *)S_malloc(sizeof(struct BinExprAST));

  bin_expr_ast->left = result;
  bin_expr_ast->opType = OpASSIGN;
  bin_expr_ast->right = expr;

  q_push(tcc->tc_assign_queue, gen_assign_tcqn(pc, bin_expr_ast->left, bin_expr_ast->right));  
  
  return pack(AST_BinExpr, bin_expr_ast);
}

static struct Node *gen_ident_node(struct Token *first,
                                   struct ParserContext *pc,
                                   struct IdentDataNode *attr_of,
                                   bool is_expr) {
  struct TokenizerContext* tc = pc->tc;
  struct Token *nt = peek(tc);

  const char *ident_str = first->str;
  struct IdentDataNode *ident_data_node = NULL;
  enum IdentType ident_type = IT_None;
  
  struct Node *result = NULL;

  if (nt->type == TokLParen) {
    ident_type = IT_Func;    
    result = gen_func_call_node(first, pc, is_expr);
  }else{
    ident_type = IT_Var;
    
    struct IdentifierAST *ident_ast = (struct IdentifierAST *)S_malloc(sizeof(struct IdentifierAST));
    ident_ast->ident = first;
    
    result = pack(AST_Identifier, ident_ast);
  }

  assert(result != NULL);
  assert(ident_type != IT_None);

  ident_data_node = gen_ident_data_node(ident_str, ident_type, attr_of);

  parse_attribute(pc, result, ident_data_node, is_expr);
  
  get_first_node_type(pc, ident_str, ident_type, ident_data_node, attr_of);

  switch (result->type) {
  case AST_Identifier:
    ((struct IdentifierAST*) result->ast)->ident_data_node = ident_data_node;
    break;

  case AST_FunctionCall:
    ((struct FuncCallAST*) result->ast)->ident_data_node = ident_data_node;
    break;

  default:
    panic("Unexpected identifier type. check parser.c", tc);
    break;    
  }    

  result = check_assign(pc, ident_data_node, attr_of, result);
  
  return result;  
}

static struct Node *gen_func_param_node(struct ParserContext *pc) {
  struct TokenizerContext *tc = pc->tc;
  struct VarDeclBundleAST *result =
      (struct VarDeclBundleAST *)S_malloc(sizeof(struct VarDeclBundleAST));
  struct TypeCheckContext *tcc = pc->tcc;
  
  assert(tcc != NULL);
  
  result->var_count = 0;
  result->var_decls = (struct Node**) S_malloc(sizeof(struct Node*));
  
  unsigned param_size = 0, capacity = 1;
  
  // ( a : int, b : int )
  consume(tc, TokLParen);

  while (peek(tc)->type != TokRParen) {
    struct Token *name_tok = pull(tc);

    consume(tc, TokColon); // :

    struct Token *type_tok = pull(tc);
    VarDeclAST* var_decl = (VarDeclAST*)S_malloc(sizeof(VarDeclAST));
    var_decl->decl = NULL;
    var_decl->var_name_tok = name_tok;
    var_decl->var_type_tok = type_tok;

    q_push(tcc->tc_type_queue, gen_rawtype_tcqn(pc, type_tok->str));
    
    if (param_size + 1 >= capacity) {
      capacity *= 2;
      result->var_decls = S_realloc(result->var_decls, sizeof(struct VarDeclBundleAST*) * capacity);
    }

    result->var_decls[param_size++] = pack(AST_VariableDeclaration, var_decl);

    struct Token* nt = peek(tc);
    if(nt->type == TokRParen) break;
    if(nt->type == TokComma) pull(tc);
  }
  
  result->var_count = param_size;

  consume(tc, TokRParen);

  return pack(AST_VariableDeclarationBundle, result);
}

static struct Scope *gen_scope(struct Scope* prev_scope) {
  struct Scope *result = (struct Scope *)S_malloc(sizeof(struct Scope));

  result->local_var_smtb = gen_htable();
  result->prev_scope = prev_scope;
  
  return result;
}

static void open_scope(struct ParserContext *pc) {
  struct Scope *scope = gen_scope(pc->current_scope);

  pc->current_scope = scope;
}

static void close_scope(struct ParserContext *pc) {
  assert(pc->current_scope != NULL);

  struct Scope *prev_scope = pc->current_scope->prev_scope;
  
  free_htable(pc->current_scope->local_var_smtb);
  free(pc->current_scope);
  
  pc->current_scope = prev_scope;
}

static struct Node **gen_body(struct ParserContext *pc, unsigned *body_size) {
  struct TokenizerContext *tc = pc->tc;

  consume(tc, TokLBracket);
  open_scope(pc);
  
  unsigned size = 0, capacity = 1;
  struct Node **result = (struct Node **)S_malloc(sizeof(struct Node *) * capacity);

  while (peek(tc)->type != TokRBracket) {
    void *element = parse(pc, false);
    assert(element != NULL);

    if (size + 1 >= capacity) {
      capacity *= 2;
      result = (struct Node**) S_realloc(result, sizeof(struct Node*) * capacity);
    }

    result[size++] = element;
  }

  consume(tc, TokRBracket);
  close_scope(pc);

  *body_size = size;
  
  return result;  
}  

static struct FuncData* register_func_data(struct Node* node, struct ParserContext* pc){
  struct FuncDeclAST* func_decl = node->ast;
  
  struct FuncData* data = (struct FuncData*) S_malloc(sizeof(struct FuncData));
  data->return_type = func_decl->ret_type_tok->str;
  data->func_name = func_decl->func_name_tok->str;

  if (pc->current_class) {
    // register in class member.    
    struct ClassData *current_class = pc->current_class;
    
    data->id = current_class->member_funcs->size + 1;
    
    ht_insert(current_class->member_funcs, func_decl->func_name_tok->str, data);

    return data;
  }
  else{
    // register in global.
    data->id = pc->glob_func_smtb->size + 1;
      
    ht_insert(pc->glob_func_smtb, func_decl->func_name_tok->str, data);
    pc->func_data[pc->func_data_cnt++] = data;

    return data;
  }  
}

static struct Node* gen_func_decl_node(struct Token *first, struct ParserContext *pc) {
  struct FuncDeclAST *func_decl = (struct FuncDeclAST *)S_malloc(sizeof(struct FuncDeclAST));
  
  struct TokenizerContext *tc = pc->tc;

  //reset local var declaration.
  pc->declared_local_var_count = 0;
  pc->declared_local_var_capacity = 1;
  pc->declared_local_vars =
      (struct VarData **)S_malloc(sizeof(struct VarData *));
  
  struct Token *func_name_tok = pull(tc);
  
  struct Node* params = gen_func_param_node(pc);

  consume(tc, TokColon);

  struct Token* ret_type_tok = pull(tc);
  struct TypeCheckContext *tcc = pc->tcc;
  
  q_push(tcc->tc_type_queue, gen_rawtype_tcqn(pc, ret_type_tok->str));    
  
  func_decl->func_name_tok = func_name_tok;
  func_decl->ret_type_tok = ret_type_tok;
  func_decl->params = params;

  // first register function data.
  struct Node* result =  pack(AST_FunctionDeclaration, func_decl);
  struct FuncData* fd = register_func_data(result, pc);
  assert(fd != NULL);
  func_decl->func_data = fd;  
  
  // and parse body.
  pc->current_func = fd;
  unsigned body_size = 0;
  func_decl->body = gen_body(pc, &body_size);
  pc->current_func = NULL;

  // register local var data to calcaulte stack size after.  
  fd->declared_var_count = pc->declared_local_var_count;
  fd->declared_vars = pc->declared_local_vars;
  
  return result;
}  

static struct VarData* register_var_data(struct Node* node, struct ParserContext* pc){
  VarDeclAST* var_decl = node->ast;

  assert(var_decl != NULL);  
  
  bool in_class = pc->current_class != NULL;
  bool in_func = pc->current_func != NULL;

  // register in member variables.
  bool member = !in_func && in_class;

  // register in global.  
  bool glob = !in_func && !in_class;

  // register in local.
  bool local = in_func;
  
  struct VarData* data = (struct VarData*) S_malloc(sizeof(struct VarData));
  data->type = var_decl->var_type_tok->str;
  data->var_name = var_decl->var_name_tok->str;

  struct HTable *target_smtb = NULL;
  
  if (member) {
    target_smtb = pc->current_class->member_vars;
  }

  if (glob) {
    target_smtb = pc->glob_var_smtb;    
  }

  if (local) {
    // register in pc->declared_local_vars to calcaulte total size of stack
    if (pc->declared_local_var_capacity >= pc->declared_local_var_count + 1) {
      pc->declared_local_var_capacity *= 2;
      pc->declared_local_vars = (struct VarData **)S_realloc(
          pc->declared_local_vars,
          sizeof(struct VarData **) * pc->declared_local_var_capacity);
    }

    pc->declared_local_vars[pc->declared_local_var_count++] = data;    
    
    target_smtb = pc->current_scope->local_var_smtb;
  }

  assert(target_smtb != NULL);  

  data->id = target_smtb->size + 1;
  ht_insert(target_smtb, var_decl->var_name_tok->str, data);

  return data;
}

static struct Node *gen_var_decl_node(struct Token *first,
                                      struct ParserContext *pc, bool is_expr) {
  struct TokenizerContext *tc = pc->tc;
  struct VarDeclBundleAST *result =
      (struct VarDeclBundleAST *)S_malloc(sizeof(struct VarDeclBundleAST));  
  struct TypeCheckContext *tcc = pc->tcc;
  
  result->var_decls = (struct Node**) S_malloc(sizeof(struct Node*));
  
  // var a: int = 0, b : float;

  bool comp = false;
  unsigned var_count = 0, capacity = 1;
  
  while(!comp){
    struct Token *var_name_tok = pull(tc);    

    consume(tc, TokColon);

    struct Token *var_type_tok = pull(tc);
    q_push(tcc->tc_type_queue, gen_rawtype_tcqn(pc, var_type_tok->str));

    struct Token *cont_tok = peek(tc);    

    void *decl = NULL;    

    if(cont_tok->type == TokAssign){
      consume(tc, TokAssign);
      decl = parse_expression(pc);
      cont_tok = peek(tc);
    }
    
    switch(cont_tok->type){
    case TokComma:
      consume(tc, TokComma);
      break;
    case TokSemiColon:
      comp = true;
      if(!is_expr) // if it is statement, consume semicolon
	consume(tc, TokSemiColon);
      break;
    default:
      panic("Unexpected token in variable declaration.", tc);
    }

    VarDeclAST *var_decl = (VarDeclAST *)S_malloc(sizeof(VarDeclAST));    
    var_decl->var_name_tok = var_name_tok;
    var_decl->var_type_tok = var_type_tok;
    var_decl->decl = decl;
    var_decl->ac_mod = ACMOD_DEFAULT;

    if(var_count + 1 >= capacity){
      capacity *= 2;
      result->var_decls = (struct Node **)S_realloc(
          result->var_decls, sizeof(struct Node *) * capacity);      
    }

    struct Node *node = pack(AST_VariableDeclaration, var_decl);    
    result->var_decls[var_count++] = node;

    register_var_data(node, pc);
  }

  return pack(AST_VariableDeclarationBundle, result);
}

static struct Node *gen_if_stmt_node(struct Token* first, struct ParserContext* pc);
static enum IfStmtType get_if_stmt_type(struct Token* first, struct ParserContext* pc);
static struct Node* gen_next_if_stmt_node(enum IfStmtType stmt_type, struct  ParserContext* pc);

static enum IfStmtType get_if_stmt_type(struct Token *first,
                                        struct ParserContext *pc) { 
  struct TokenizerContext *tc = pc->tc;
  struct Token *nt = peek(tc);  
  enum IfStmtType stmt_type = StmtNone;
    
  if(first->type == TokIf){
    stmt_type = StmtIf;
  }

  if(first->type == TokElse){
    stmt_type = StmtElse;

    if(nt->type == TokIf){
      stmt_type = StmtElseIf;
    }
  }
  
  return stmt_type;
}

static struct Node *gen_next_if_stmt_node(enum IfStmtType stmt_type,
                                          struct ParserContext *pc) {  
  struct TokenizerContext *tc = pc->tc;  
  struct Token *nt = peek(tc);
  struct Node *next_stmt = NULL;
  
  
  if(nt->type == TokElse){
    consume(tc, TokElse);
    next_stmt = gen_if_stmt_node(nt, pc);
  }

  if(stmt_type == StmtElse && next_stmt != NULL){
    panic("Wrong statement. statement after else statement.", tc);
  }

  return next_stmt;
}

static struct Node *gen_if_stmt_node(struct Token *first,
                                     struct ParserContext *pc) {
  struct IfStmtAST *result =
      (struct IfStmtAST *)S_malloc(sizeof(struct IfStmtAST));  

  struct TokenizerContext *tc = pc->tc;
  enum IfStmtType stmt_type = get_if_stmt_type(first, pc);  

  if(stmt_type == StmtElseIf){
    consume(tc, TokIf);
  }
  if(stmt_type == StmtNone){
    panic("Wrong Statement type", tc);
  }
  
  void* cond = NULL;
  if(stmt_type == StmtElseIf || stmt_type == StmtIf){
    consume(tc, TokLParen);
    cond = parse_expression(pc);
    consume(tc, TokRParen);
  }

  unsigned body_size = 0;
  struct Node **body = gen_body(pc, &body_size);  

  result->cond = cond;
  result->body = body;
  result->body_size = body_size;
  result->stmt_type = stmt_type;

  // gen next if stmt
  result->next_stmt = gen_next_if_stmt_node(stmt_type, pc);
  
  return pack(AST_IfStatement, result);
}

static struct Node *gen_for_stmt_node(struct Token *first,
                                      struct ParserContext *pc) {  
  struct ForStmtAST *result =
      (struct ForStmtAST *)S_malloc(sizeof(struct ForStmtAST));
 
  struct TokenizerContext* tc = pc->tc;

  consume(tc, TokLParen);
  
  struct Node* init = parse_expression(pc);
  consume(tc, TokSemiColon);
  
  struct Node* cond = parse_expression(pc);
  consume(tc, TokSemiColon);

  struct Node *step = parse_expression(pc);
  

  consume(tc, TokRParen);

  unsigned body_size = 0;
  struct Node **body = gen_body(pc, &body_size);  

  result->init = init;
  result->cond = cond;
  result->step = step;
  result->body = body;
  result->body_size = body_size;

  return pack(AST_ForStatement, result);
}

static struct Node *gen_ret_node(struct Token *first,
                                 struct ParserContext *pc) {
  struct Node *expr = parse_expression(pc);  
  struct ReturnAST *ret_ast =
      (struct ReturnAST *)S_malloc(sizeof(struct ReturnAST));  

  ret_ast->expr = expr;

  return pack(AST_Return, ret_ast);
}

static struct ClassData *register_class_data(struct Node *node,
                                             struct ParserContext *pc) {
  struct ClassAST *class_ast = node->ast;  

  struct ClassData *data =
      (struct ClassData *)S_malloc(sizeof(struct ClassData));  
  data->id = pc->class_type_smtb->size + 1;
  data->class_name = class_ast->name_tok->str;

  if (class_ast->parent_name_tok == NULL) {
    data->parent_name = "";
  } else {
    data->parent_name = class_ast->parent_name_tok->str;
  }    
  
  data->member_funcs = gen_htable();
  data->member_vars = gen_htable();

  ht_insert(pc->class_type_smtb, class_ast->name_tok->str,
            gen_class_type(class_ast->name_tok->str, data));
  
  pc->class_data[pc->class_data_cnt++] = data;  

  return data;
}

static struct Node *gen_class_decl_node(struct Token *first,
                                        struct ParserContext *pc) {
  struct TokenizerContext *tc = pc->tc;  
  struct ClassAST *class_ast =
      (struct ClassAST *)S_malloc(sizeof(struct ClassAST));  

  struct Token *name_tok = pull(tc);  
  class_ast->name_tok = name_tok;
  class_ast->parent_name_tok = NULL;
  
  if(peek(tc)->type == TokExtends){
    consume(tc, TokExtends);

    struct Token *parent_name_tok = pull(tc);    
    class_ast->parent_name_tok = parent_name_tok;
  }
  
  // first register class data.
  struct Node *result = pack(AST_Class, class_ast);  
  struct ClassData *cd = register_class_data(result, pc);
  assert(cd != NULL);
  
  pc->current_class = cd;
  // and parse body
  unsigned body_size;
  class_ast->body = gen_body(pc, &body_size);
  pc->current_class = NULL;
  
  return result;
}

struct Node *parse(struct ParserContext *pc, bool is_expr) {  
  struct TokenizerContext *tc = pc->tc;
  assert(tc != NULL);

  struct Token *first = pull(tc);

  switch (first->type) {

  case TokNumberLiteral:{
    struct NumberLiteralAST *num =
        (struct NumberLiteralAST *)S_malloc(sizeof(struct NumberLiteralAST));    
    num->num_tok = first;
    
    return pack(AST_NumberLiteral, num);
  }

  case TokNull: {
    struct NullAST *null = (struct NullAST *)S_malloc(sizeof(struct NullAST *));
    return pack(AST_Null, null);
  }    
    
  case TokIf: {
    return gen_if_stmt_node(first, pc);
  }

  case TokFor: {
    return gen_for_stmt_node(first, pc);
  }
    
  case TokFunc: {
    return gen_func_decl_node(first, pc);
  }    

  case TokVar: {
    return gen_var_decl_node(first, pc, is_expr);
  }

  case TokClass: {
    return gen_class_decl_node(first, pc);
  }
    
  case TokIdent: {
    return gen_ident_node(first, pc, false, is_expr);
  }

  case TokReturn : {
    return gen_ret_node(first, pc);
  }

  case TokLParen: {
    struct Node* expr = parse_expression(pc);

    consume(tc, TokRParen);

    return expr;
  }

  case TokEOF:{
    return NULL;
  }
    
  default:
    panic("Unexpected Token type\n", tc);
    
  }    

  return NULL;
}

static struct Node *parse_term(struct ParserContext *pc) {
  struct Node *node = parse(pc, true);
  struct TokenizerContext *tc = pc->tc;  
  
  while (peek(tc) && (peek(tc)->type == TokMul || peek(tc)->type == TokDiv)) {
    enum TokenType op = pull(tc)->type;
    void *right = parse(pc, true);    

    enum OperatorType op_type = (op == TokMul) ? OpMUL : OpDIV;

    struct BinExprAST *bin_expr =
        (struct BinExprAST *)S_malloc(sizeof(struct BinExprAST));    

    bin_expr->left = node;
    bin_expr->right = right;
    bin_expr->opType = op_type;

    node = pack(AST_BinExpr, bin_expr);
  }
  return node;
}

static struct Node *parse_simple_expression(struct ParserContext *pc) {
  struct Node *node = parse_term(pc);  
  struct TokenizerContext *tc = pc->tc; 
  
  while (peek(tc) && (peek(tc)->type == TokAdd || peek(tc)->type == TokSub)) {
    enum TokenType op = pull(tc)->type;
    void *right = parse_term(pc);

    enum OperatorType op_type = (op == TokAdd) ? OpADD : OpSUB;

    struct BinExprAST* bin_expr = (struct BinExprAST*)S_malloc(sizeof(struct BinExprAST));

    bin_expr->left = node;
    bin_expr->right = right;
    bin_expr->opType = op_type;

    node = pack(AST_BinExpr, bin_expr);
  }
  return node;
}

static struct Node *parse_unary_expression(struct ParserContext *pc) {
  struct TokenizerContext* tc = pc->tc; 

  if (peek(tc) && peek(tc)->type == TokNot) {
    pull(tc); // Consume '!'
    struct UnaryExprAST* unary_expr = (struct UnaryExprAST*)S_malloc(sizeof(struct UnaryExprAST));

    unary_expr->expr = parse_unary_expression(pc);
    return pack(AST_UnaryExpr, unary_expr);
  }
  
  return parse_simple_expression(pc);
}

static struct Node *parse_compare_expression(struct ParserContext* pc) {
  void* node = parse_unary_expression(pc);
  struct TokenizerContext* tc = pc->tc;

  while (peek(tc) &&
         (peek(tc)->type == TokEqual || peek(tc)->type == TokNotEqual ||
          peek(tc)->type == TokGreater || peek(tc)->type == TokLesser ||
          peek(tc)->type == TokEqualGreater ||
          peek(tc)->type == TokEqualLesser)) {
    
    struct Token* operator_token = pull(tc);
    enum TokenType op = operator_token->type;
    struct Node* right = parse_unary_expression(pc);

    enum OperatorType op_type = OpNone;
    switch (op) {
    case TokEqual: op_type = OpEQUAL; break;
    case TokNotEqual: op_type = OpNOTEQUAL; break;
    case TokGreater: op_type = OpGREATER; break;
    case TokLesser: op_type = OpLESS; break;
    case TokEqualGreater: op_type = OpEQUALGREATER; break;
    case TokEqualLesser: op_type = OpEQUALLESS; break;
    default:
      panic("Unknown operator type.", tc);
    }

    struct BinExprAST* bin_expr = (struct BinExprAST*)S_malloc(sizeof(struct BinExprAST));

    bin_expr->left = node;
    bin_expr->right = right;

    bin_expr->opType = op_type;

    node = pack(AST_BinExpr, bin_expr);
  }
  return node;
}

static struct Node *parse_expression(struct ParserContext* pc) {
  struct Node* node = parse_compare_expression(pc);
  struct TokenizerContext* tc = pc->tc;

  while (peek(tc) && (peek(tc)->type == TokOr || peek(tc)->type == TokAnd)) {
    enum TokenType op = pull(tc)->type;
    struct Node* right = parse_compare_expression(pc);

    enum OperatorType op_type = OpNone;
    switch (op) {
    case TokOr: op_type = OpOR; break;
    case TokAnd: op_type = OpAND; break;
    default:
      panic("Unknown operator type.", tc);
    }

    struct BinExprAST* bin_expr = (struct BinExprAST*)S_malloc(sizeof(struct BinExprAST));

    bin_expr->left = node;
    bin_expr->right = right;
    bin_expr->opType = op_type;

    node = pack(AST_BinExpr, bin_expr);
  }

  return node;
}

