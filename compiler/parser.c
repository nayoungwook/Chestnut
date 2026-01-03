#include "token.h"
#include "type.h"
#include "parser.h"
#include "error.h"

static Node *parse_term(ParserContext* pc);
static Node *parse_simple_expression(ParserContext* pc);
static Node *parse_unary_expression(ParserContext *pc);
static Node *parse_compare_expression(ParserContext* pc);
static Node *parse_expression(ParserContext* pc);

ParserContext *gen_pc() {
  ParserContext *pc = (ParserContext *)S_malloc(sizeof(ParserContext));

  pc->tc = NULL;

  pc->current_scope = NULL;
  pc->current_class = NULL;
  pc->current_func = NULL;
  
  pc->glob_func_smtb = gen_htable();
  pc->glob_var_smtb = gen_htable();
  pc->class_smtb = gen_htable();
  pc->typecheck_queue = gen_queue();

  return pc;
}

void compile_file(ParserContext* pc, TokenizerContext *tc) {
  void *ast = NULL;
  pc->tc = tc;  

  while ((ast = parse(pc, false)) != NULL) {
  }

  pc->tc = NULL;  
  free(tc);  
}  

static Node* pack(ASTType type, void* ptr){
  Node* result = (Node*) S_malloc(sizeof(Node));
  
  result->ast = ptr;
  result->type = type;
  
  return result;
}

static Node* gen_func_call_node(Token* first, ParserContext* pc, bool is_expr){
  FuncCallAST* func_call = (FuncCallAST*) S_malloc(sizeof(FuncCallAST));
  TokenizerContext* tc = pc->tc;

  unsigned capacity = 1, size = 0;
  Node** params = (Node**) S_malloc(sizeof(Node*) * capacity);
  
  consume(tc, TokLParen);

  while(peek(tc)->type != TokRParen){
    Node* expr = parse_expression(pc);

    if(size + 1 >= capacity) {
      capacity *= 2;
      params = (Node**) S_realloc(params, sizeof(Node*) * capacity);
    }

    params[size++] = expr;
    
    if(peek(tc)->type == TokComma){
      consume(tc, TokComma);
    }    
  }

  consume(tc, TokRParen);

  func_call->func_name_tok = first;
  func_call->params = params;
  func_call->param_size = size;

  if(!is_expr){
    consume(tc, TokSemiColon);
  }
  
  return pack(AST_FunctionCall, func_call);
}

/*
  @AttribNode -> this identifier node will be attribute of "attr_of" node.
  [first identifier] ... [attr_of] -> [attr_of] -> [gen_ident_node]
  We have to check attribute and type validation after the parsing.
*/

static IdentData *gen_ident_data(const wchar_t *ident, IdentType attr_type) {
  IdentData *ident_data = (IdentData *)S_malloc(sizeof(IdentData));

  ident_data->attr_type = attr_type;
  ident_data->str = ident;

  return ident_data;  
}

static IdentNode *gen_attrib_node(const wchar_t* ident, IdentType ident_type, IdentNode* attr_of) {
  IdentNode *ident_node = (IdentNode *)S_malloc(sizeof(IdentNode));
  ident_node->ident_data = gen_ident_data(ident, ident_type);

  if (attr_of != NULL) {
    // This identifier will be "attr_node" of "attr_of"      
    attr_of->attr = ident_node;
  }

  return ident_node;
}

void free_ident_node(IdentNode* ident_node) {
  free(ident_node->ident_data);
  free(ident_node);
}

static VarData *find_var_data(ParserContext *pc, const wchar_t *var_name) {
  VarData *result = NULL;

  if (pc->current_scope != NULL) { // first find in local
    Scope *scope_searcher = pc->current_scope;
    while (scope_searcher != NULL) {
      result = (VarData *)ht_find(scope_searcher->local_var_smtb, var_name);

      if (result != NULL)
        break;

      scope_searcher = scope_searcher->prev_scope;
    }      
  }

  if (result == NULL && pc->current_class != NULL) { // and find in class
    result = (VarData*) ht_find(pc->current_class->member_vars, var_name);
  }

  if (result == NULL) {
    result = ht_find(pc->glob_var_smtb, var_name);    
  }    
  
  return result;
}

static FuncData *find_func_data(ParserContext* pc, const wchar_t *func_name) {
  ClassData *current_class = pc->current_class;
  FuncData *result = NULL;
  
  if (current_class != NULL) { // find in class.
    result = ht_find(current_class->member_funcs, func_name);
  }

  if (result == NULL) { // find in glob
    result = ht_find(pc->glob_func_smtb, func_name);
  }

  return result;  
}

static wchar_t *get_type_of_identifier(ParserContext *pc, IdentType ident_type,
                                       const wchar_t *str) {
  wchar_t* result = L"";
  switch (ident_type) {
  case IT_Var: {
    VarData *var = find_var_data(pc, str);
    if (var == NULL) {
      panic(L"Failed to find variable.", pc->tc);
    }
    assert(var->node->type == AST_VariableDeclaration);
      
    VarDeclAST* var_decl_ast = (VarDeclAST*) var->node;
    result = wcsdup(var_decl_ast->var_type_tok->str);
      
    break;
  }

  case IT_Func: {
    FuncData* func = find_func_data(pc, str);
    if (func == NULL) {
      panic(L"Failed to find function.", pc->tc);
    }

    assert(func->node->type == AST_FunctionDeclaration);

    FuncDeclAST* func_decl_ast = (FuncDeclAST*) func->node;
    result = wcsdup(func_decl_ast->ret_type_tok->str);
      
    break;
  }
      
  default:
    break;
  }

  return result;
}  

static Node *gen_ident_node(Token* first, ParserContext *pc, IdentNode* attr_of, bool is_expr) {
  TokenizerContext* tc = pc->tc;
  Token *nt = peek(tc);

  IdentNode *ident_node = NULL;
  IdentType attr_type = IT_None;
  
  Node *result = NULL;

  if (nt->type == TokLParen) {
    attr_type = IT_Func;    
    result = gen_func_call_node(first, pc, is_expr);
  }else{
    attr_type = IT_Var;
    
    IdentifierAST *ident_ast = (IdentifierAST *)S_malloc(sizeof(IdentifierAST));
    ident_ast->ident = first;
    
    result = pack(AST_Identifier, ident_ast);
  }

  assert(result != NULL);
  assert(attr_type != IT_None);

  wchar_t *attr_node_str = wcsdup(first->str);

  ident_node = gen_attrib_node(attr_node_str, attr_type, attr_of);
  
  if ((nt = peek(tc))->type == TokDot) {
    consume(tc, TokDot);

    Node *attr = gen_ident_node(pull(tc), pc, ident_node, is_expr);
    result->attribute = attr;
  }
  
  // If it is first node, we have to check type of identifier.
  if (attr_of == NULL && ident_node != NULL) {
    attr_node_str = get_type_of_identifier(pc, attr_type, attr_node_str);    
  }
  
  // This is first identifier we put typechek on queue.
  if (attr_of == NULL && ident_node != NULL) {
    q_push(pc->typecheck_queue, gen_tcqnode(TCK_CheckTypeExist, ident_node));
  }
    
  return result;  
}

static Node *gen_func_param_node(ParserContext *pc) {
  TokenizerContext *tc = pc->tc;
  VarDeclBundleAST *result = (VarDeclBundleAST*) S_malloc(sizeof(VarDeclBundleAST));
  result->var_count = 0;
  result->var_decls = (Node**) S_malloc(sizeof(Node*));
  
  unsigned param_size = 0, capacity = 1;
  
  // ( a : int, b : int )
  consume(tc, TokLParen);

  while (peek(tc)->type != TokRParen) {
    Token *name_tok = pull(tc);

    consume(tc, TokColon); // :
    
    Token *type_tok = pull(tc);
    VarDeclAST* var_decl = (VarDeclAST*)S_malloc(sizeof(VarDeclAST));
    var_decl->decl = NULL;
    var_decl->var_name_tok = name_tok;
    var_decl->var_type_tok = type_tok;
    
    if (param_size + 1 >= capacity) {
      capacity *= 2;
      result->var_decls = S_realloc(result->var_decls, sizeof(VarDeclBundleAST*) * capacity);
    }

    result->var_decls[param_size++] = pack(AST_VariableDeclaration, var_decl);

    Token* nt = peek(tc);
    if(nt->type == TokRParen) break;
    if(nt->type == TokComma) pull(tc);
  }
  
  result->var_count = param_size;

  consume(tc, TokRParen);

  return pack(AST_VariableDeclarationBundle, result);
}

static Scope *gen_scope(Scope* prev_scope) {
  Scope *result = (Scope *)S_malloc(sizeof(Scope));

  result->local_var_smtb = gen_htable();
  result->prev_scope = prev_scope;
  
  return result;
}

static void open_scope(ParserContext *pc) {
  Scope *scope = gen_scope(pc->current_scope);

  pc->current_scope = scope;
}

static void close_scope(ParserContext *pc) {
  assert(pc->current_scope != NULL);

  Scope *prev_scope = pc->current_scope->prev_scope;
  
  free_htable(pc->current_scope->local_var_smtb);
  free(pc->current_scope);
  
  pc->current_scope = prev_scope;
}  

static Node **gen_body(ParserContext *pc, unsigned *body_size) {
  TokenizerContext *tc = pc->tc;

  consume(tc, TokLBracket);
  open_scope(pc);
  
  unsigned size = 0, capacity = 1;
  Node **result = (Node **)S_malloc(sizeof(Node *) * capacity);
  
  while (peek(tc)->type != TokRBracket) {
    void *element = parse(pc, false);
    assert(element != NULL);

    if (size + 1 >= capacity) {
      capacity *= 2;
      result = (Node**) S_realloc(result, sizeof(Node*) * capacity);
    }

    result[size++] = element;    
  }

  consume(tc, TokRBracket);
  close_scope(pc);
  
  *body_size = size;
  
  return result;  
}  

static FuncData* register_func_data(Node* node, ParserContext* pc){
  FuncDeclAST* func_decl = node->ast;

  FuncData* data = (FuncData*) S_malloc(sizeof(FuncData));
  data->node = node;    
  
  if(pc->current_class){ // register in class member.
    ClassData *current_class = pc->current_class;
    
    data->id = current_class->member_funcs->size + 1;
    
    ht_insert(current_class->member_funcs, func_decl->func_name_tok->str, data);

    return data;
  } else { // register in global.
    data->id = pc->glob_func_smtb->size + 1;
      
    ht_insert(pc->glob_func_smtb, func_decl->func_name_tok->str, data);
    pc->func_data[pc->func_data_cnt++] = data;
    
    return data;
  }

  free(data);
  return NULL;
}

static Node* gen_func_decl_node(Token *first, ParserContext *pc) {
  FuncDeclAST *func_decl = (FuncDeclAST *)S_malloc(sizeof(FuncDeclAST));
  
  TokenizerContext *tc = pc->tc;

  Token *func_name_tok = pull(tc);
  
  Node* params = gen_func_param_node(pc);

  consume(tc, TokColon);

  Token* ret_type_tok = pull(tc);

  func_decl->func_name_tok = func_name_tok;
  func_decl->ret_type_tok = ret_type_tok;
  func_decl->params = params;

  // first register function data.
  Node* result =  pack(AST_FunctionDeclaration, func_decl);
  FuncData* fd = register_func_data(result, pc);
  assert(fd != NULL);

  // and parse body.
  pc->current_func = fd;
  unsigned body_size = 0;
  func_decl->body = gen_body(pc, &body_size);
  pc->current_func = NULL;
  
  return result;
}  

static VarData* register_var_data(Node* node, ParserContext* pc){
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
  
  VarData* data = (VarData*) S_malloc(sizeof(VarData));
  data->node = node;

  HTable* target_smtb = NULL;

  if (member) {
    target_smtb = pc->current_class->member_vars;
  }

  if (glob) {
    target_smtb = pc->glob_var_smtb;    
  }

  if (local) {
    target_smtb = pc->current_scope->local_var_smtb;
  }

  assert(target_smtb != NULL);  

  data->id = target_smtb->size + 1;
  ht_insert(target_smtb, var_decl->var_name_tok->str, data);

  return data;
}

static Node* gen_var_decl_node(Token* first, ParserContext* pc, bool is_expr){
  TokenizerContext *tc = pc->tc;
  VarDeclBundleAST* result = (VarDeclBundleAST*) S_malloc(sizeof(VarDeclBundleAST));
  result->var_decls = (Node**) S_malloc(sizeof(Node*));
  
  // var a: int = 0, b : float;

  bool comp = false;
  unsigned var_count = 0, capacity = 1;
  
  while(!comp){
    Token* var_name_tok = pull(tc);

    consume(tc, TokColon);

    Token* var_type_tok = pull(tc);

    Token* cont_tok = peek(tc);

    void* decl = NULL;

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
      panic(L"Unexpected token in variable declaration.", tc);
    }
    
    VarDeclAST* var_decl = (VarDeclAST*) S_malloc(sizeof(VarDeclAST));
    var_decl->var_name_tok = var_name_tok;
    var_decl->var_type_tok = var_type_tok;
    var_decl->decl = decl;
    var_decl->ac_mod = ACMOD_DEFAULT;

    if(var_count + 1 >= capacity){
      capacity *= 2;
      result->var_decls = (Node**) S_realloc(result->var_decls, sizeof(Node*) * capacity);
    }

    Node* node = pack(AST_VariableDeclaration, var_decl);
    result->var_decls[var_count++] = node;

    register_var_data(node, pc);
  }

  return pack(AST_VariableDeclarationBundle, result);
}

static Node *gen_if_stmt_node(Token* first, ParserContext* pc);
static IfStmtType get_if_stmt_type(Token* first, ParserContext* pc);
static Node* gen_next_if_stmt_node(IfStmtType stmt_type, ParserContext* pc);

static IfStmtType get_if_stmt_type(Token* first, ParserContext* pc){
  TokenizerContext* tc = pc->tc;
  Token* nt = peek(tc);
  IfStmtType stmt_type = StmtNone;
    
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

static Node* gen_next_if_stmt_node(IfStmtType stmt_type, ParserContext* pc){
  TokenizerContext* tc = pc->tc;
  Token* nt = peek(tc);
  Node* next_stmt = NULL;
  
  if(nt->type == TokElse){
    consume(tc, TokElse);
    next_stmt = gen_if_stmt_node(nt, pc);
  }

  if(stmt_type == StmtElse && next_stmt != NULL){
    panic(L"Wrong statement. statement after else statement.", tc);
  }

  return next_stmt;
}

static Node *gen_if_stmt_node(Token* first, ParserContext* pc){
  IfStmtAST* result = (IfStmtAST*) S_malloc(sizeof(IfStmtAST));

  TokenizerContext* tc = pc->tc;
  IfStmtType stmt_type = get_if_stmt_type(first, pc);

  if(stmt_type == StmtElseIf){
    consume(tc, TokIf);
  }
  if(stmt_type == StmtNone){
    panic(L"Wrong Statement type", tc);
  }
  
  void* cond = NULL;
  if(stmt_type == StmtElseIf || stmt_type == StmtIf){
    consume(tc, TokLParen);
    cond = parse_expression(pc);
    consume(tc, TokRParen);
  }

  unsigned body_size = 0;
  Node** body = gen_body(pc, &body_size);

  result->cond = cond;
  result->body = body;
  result->body_size = body_size;
  result->stmt_type = stmt_type;

  // gen next if stmt
  result->next_stmt = gen_next_if_stmt_node(stmt_type, pc);
  
  return pack(AST_IfStatement, result);
}

static Node* gen_for_stmt_node(Token* first, ParserContext* pc){
  ForStmtAST* result = (ForStmtAST*) S_malloc(sizeof(ForStmtAST));

  TokenizerContext* tc = pc->tc;

  consume(tc, TokLParen);
  
  Node* init = parse_expression(pc);
  consume(tc, TokSemiColon);
  
  Node* cond = parse_expression(pc);
  consume(tc, TokSemiColon);

  Node* step = parse_expression(pc);

  consume(tc, TokRParen);

  unsigned body_size = 0;
  Node** body = gen_body(pc, &body_size);

  result->init = init;
  result->cond = cond;
  result->step = step;
  result->body = body;
  result->body_size = body_size;

  return pack(AST_ForStatement, result);
}

static Node* gen_ret_node(Token* first, ParserContext* pc){
  Node* expr = parse_expression(pc);
  ReturnAST* ret_ast = (ReturnAST*) S_malloc(sizeof(ReturnAST));

  ret_ast->expr = expr;

  return pack(AST_Return, ret_ast);
}

static ClassData* register_class_data(Node* node, ParserContext* pc){
  ClassAST* class_ast = node->ast;

  ClassData* data = (ClassData*) S_malloc(sizeof(ClassData));
  data->id = pc->class_smtb->size + 1;
  data->node = node;

  data->member_funcs = gen_htable();
  data->member_vars = gen_htable();
  
  ht_insert(pc->class_smtb, class_ast->name_tok->str, data);
  pc->class_data[pc->class_data_cnt++]= data;
  
  return data;
}

static Node* gen_class_decl_node(Token* first, ParserContext* pc){
  TokenizerContext* tc = pc->tc;
  ClassAST* class_ast = (ClassAST*) S_malloc(sizeof(ClassAST));

  Token* name_tok = pull(tc);
  class_ast->name_tok = name_tok;
  class_ast->parent_name_tok = NULL;
  
  if(peek(tc)->type == TokExtends){
    consume(tc, TokExtends);

    Token* parent_name_tok = pull(tc);
    class_ast->parent_name_tok = parent_name_tok;
  }
  
  // first register class data.
  Node* result = pack(AST_Class, class_ast);
  ClassData* cd = register_class_data(result, pc);
  assert(cd != NULL);
  
  pc->current_class = cd;
  // and parse body
  unsigned body_size;
  class_ast->body = gen_body(pc, &body_size);
  pc->current_class = NULL;
  
  return result;
}

Node *parse(ParserContext *pc, bool is_expr) {
  TokenizerContext *tc = pc->tc;
  assert(tc != NULL);

  Token *first = pull(tc);
  
  switch (first->type) {

  case TokNumberLiteral:{
    NumberLiteralAST* num = (NumberLiteralAST *)S_malloc(sizeof(NumberLiteralAST));
    num->num_tok = first;
    
    return pack(AST_NumberLiteral, num);
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
    Node* expr = parse_expression(pc);

    consume(tc, TokRParen);

    return expr;
  }

  case TokEOF:{
    return NULL;
  }
    
  default:
    panic(L"Unexpected Token type\n", tc);
    
  }    

  return NULL;
}  

static Node *parse_term(ParserContext* pc) {
  Node* node = parse(pc, true);
  TokenizerContext* tc = pc->tc;
  
  while (peek(tc) && (peek(tc)->type == TokMul || peek(tc)->type == TokDiv)) {
    TokenType op = pull(tc)->type;
    void* right = parse(pc, true);

    OperatorType op_type = (op == TokMul) ? OpMUL : OpDIV;

    BinExprAST* bin_expr = (BinExprAST*)S_malloc(sizeof(BinExprAST));

    bin_expr->left = node;
    bin_expr->right = right;
    bin_expr->opType = op_type;

    node = pack(AST_BinExpr, bin_expr);
  }
  return node;
}

static Node *parse_simple_expression(ParserContext* pc) {
  Node* node = parse_term(pc);
  TokenizerContext* tc = pc->tc;
  
  while (peek(tc) && (peek(tc)->type == TokAdd || peek(tc)->type == TokSub)) {
    TokenType op = pull(tc)->type;
    void* right = parse_term(pc);

    OperatorType op_type = (op == TokAdd) ? OpADD : OpSUB;

    BinExprAST* bin_expr = (BinExprAST*)S_malloc(sizeof(BinExprAST));

    bin_expr->left = node;
    bin_expr->right = right;
    bin_expr->opType = op_type;

    node = pack(AST_BinExpr, bin_expr);
  }
  return node;
}

static Node *parse_unary_expression(ParserContext *pc) {
  TokenizerContext* tc = pc->tc; 

  if (peek(tc) && peek(tc)->type == TokNot) {
    pull(tc); // Consume '!'
    UnaryExprAST* unary_expr = (UnaryExprAST*)S_malloc(sizeof(UnaryExprAST));

    unary_expr->expr = parse_unary_expression(pc);
    return pack(AST_UnaryExpr, unary_expr);
  }
  
  return parse_simple_expression(pc);
}

static Node *parse_compare_expression(ParserContext* pc) {
  void* node = parse_unary_expression(pc);
  TokenizerContext* tc = pc->tc;

  while (peek(tc) &&
         (peek(tc)->type == TokEqual || peek(tc)->type == TokNotEqual ||
          peek(tc)->type == TokGreater || peek(tc)->type == TokLesser ||
          peek(tc)->type == TokEqualGreater ||
          peek(tc)->type == TokEqualLesser)) {
    
    Token* operator_token = pull(tc);
    TokenType op = operator_token->type;
    Node* right = parse_unary_expression(pc);

    OperatorType op_type = OpNone;
    switch (op) {
    case TokEqual: op_type = OpEQUAL; break;
    case TokNotEqual: op_type = OpNOTEQUAL; break;
    case TokGreater: op_type = OpGREATER; break;
    case TokLesser: op_type = OpLESS; break;
    case TokEqualGreater: op_type = OpEQUALGREATER; break;
    case TokEqualLesser: op_type = OpEQUALLESS; break;
    default:
      panic(L"Unknown operator type.", tc);
    }

    BinExprAST* bin_expr = (BinExprAST*)S_malloc(sizeof(BinExprAST));

    bin_expr->left = node;
    bin_expr->right = right;

    bin_expr->opType = op_type;

    node = pack(AST_BinExpr, bin_expr);
  }
  return node;
}

static Node *parse_expression(ParserContext* pc) {
  Node* node = parse_compare_expression(pc);
  TokenizerContext* tc = pc->tc;

  while (peek(tc) && (peek(tc)->type == TokOr || peek(tc)->type == TokAnd)) {
    TokenType op = pull(tc)->type;
    Node* right = parse_compare_expression(pc);

    OperatorType op_type = OpNone;
    switch (op) {
    case TokOr: op_type = OpOR; break;
    case TokAnd: op_type = OpAND; break;
    default:
      panic(L"Unknown operator type.", tc);
    }

    BinExprAST* bin_expr = (BinExprAST*)S_malloc(sizeof(BinExprAST));

    bin_expr->left = node;
    bin_expr->right = right;
    bin_expr->opType = op_type;

    node = pack(AST_BinExpr, bin_expr);
  }

  return node;
}

