#include "token.h"
#include <parser.h>
#include <error.h>

ParserContext *gen_pc(TokenizerContext *tc) {
  ParserContext *pc = (ParserContext *)S_malloc(sizeof(ParserContext));

  pc->tc = tc;  
  
  return pc;
}

static IdentifierAST *gen_ident_ast(Token* first, ParserContext *pc) {
  IdentifierAST *ident = (IdentifierAST *)S_malloc(sizeof(IdentifierAST));

  ident->TYPE = AST_Identifier;
  ident->ident = first;
  ident->attribute = NULL;
  
  return ident;  
}

static VarDeclBundleAST *gen_func_params(ParserContext *pc) {
  TokenizerContext *tc = pc->tc;
  VarDeclBundleAST *result = (VarDeclBundleAST*) S_malloc(sizeof(VarDeclBundleAST));
  result->TYPE = AST_VariableDeclarationBundle;
  result->var_count = 0;
  
  unsigned param_size = 0, capacity = 1;
  
  // ( a : int, b : int )
  consume(tc, TokLParen);

  while (peek(tc)->type != TokRParen) {
    Token *name_tok = pull(tc);

    consume(tc, TokColon); // :
    
    Token *type_tok = pull(tc);
    VarDeclAST* var_decl = (VarDeclAST*)S_malloc(sizeof(VarDeclAST));
    var_decl->decl = NULL;
    var_decl->TYPE = AST_VariableDeclaration;
    var_decl->var_name_tok = name_tok;
    var_decl->var_type_tok = type_tok;
    
    if (param_size + 1 >= capacity) {
      capacity *= 2;
      result->var_decls = S_realloc(result->var_decls, sizeof(VarDeclBundleAST) * capacity);
    }

    result->var_decls[param_size++] = var_decl;
  }
  
  result->var_count = param_size;

  consume(tc, TokLParen);

  return result;
}

static void **gen_body(ParserContext *pc, unsigned *body_size) {
  TokenizerContext *tc = pc->tc;

  consume(tc, TokLSquareBracket);

  void **result = (void **)S_malloc(sizeof(void *));
  unsigned size = 0, capacity = 1;
  
  while (peek(tc)->type != TokRSquareBracket) {
    void *element = parse(pc);

    if (size + 1 >= capacity) {
      capacity *= 2;
      result = (void**) S_realloc(result, sizeof(void*) * capacity);
    }

    result[size++] = element;    
  }

  *body_size = size;

  return result;  
}  

static FuncDeclAST* gen_func_ast(Token *first, ParserContext *pc) {
  FuncDeclAST *func_decl = (FuncDeclAST *)S_malloc(sizeof(FuncDeclAST));
  func_decl->TYPE = AST_FunctionDeclaration;

  TokenizerContext *tc = pc->tc;

  Token *func_name_tok = pull(tc);
  
  VarDeclBundleAST* params = gen_func_params(pc);

  consume(tc, TokColon);

  Token* ret_type_tok = pull(tc);

  func_decl->func_name_tok = func_name_tok;
  func_decl->ret_type_tok = ret_type_tok;
  func_decl->parameters = params;

  unsigned body_size = 0;
  func_decl->body = gen_body(pc, &body_size);
  
  // func name(): void {
  
  return func_decl;
}  

void *parse(ParserContext *pc) {
  TokenizerContext *tc = pc->tc;
  Token *first = peek(tc);
  
  switch (first->type) {

  case TokNumberLiteral:{
    NumberLiteralAST* num = (NumberLiteralAST *)S_malloc(sizeof(NumberLiteralAST));
    num->TYPE = AST_NumberLiteral;
    num->num_tok = first;
    
    return num;
  }

  case TokFunc: {
    return gen_func_ast(first, pc);
  }    
    
  case TokIdent: {
    return gen_ident_ast(first, pc);
  }    
    
  default:
    panic(L"Unexpected Token type\n", tc);    
  }    

  return NULL;
}  

static void *parse_term(ParserContext* pc) {
  void* node = parse(pc);
  TokenizerContext* tc = pc->tc;
  
  while (peek(tc) && (peek(tc)->type == TokMul || peek(tc)->type == TokDiv)) {
    TokenType op = pull(tc)->type;
    void* right = parse(pc);

    OperatorType op_type = (op == TokMul) ? OpMUL : OpDIV;

    BinExprAST* bin_expr = (BinExprAST*)S_malloc(sizeof(BinExprAST));

    bin_expr->TYPE = AST_BinExpr;
    bin_expr->left = node;
    bin_expr->right = right;
    bin_expr->opType = op_type;

    node = bin_expr;
  }
  return node;
}

static void *parse_simple_expression(ParserContext* pc) {
  void* node = parse_term(pc);
  TokenizerContext* tc = pc->tc;
  
  while (peek(tc) && (peek(tc)->type == TokAdd || peek(tc)->type == TokSub)) {
    TokenType op = pull(tc)->type;
    void* right = parse_term(pc);

    OperatorType op_type = (op == TokAdd) ? OpADD : OpSUB;

    BinExprAST* bin_expr = (BinExprAST*)S_malloc(sizeof(BinExprAST));

    bin_expr->TYPE = AST_BinExpr;
    bin_expr->left = node;
    bin_expr->right = right;
    bin_expr->opType = op_type;

    node = bin_expr;
  }
  return node;
}

static void *parse_unary_expression(ParserContext *pc) {
  TokenizerContext* tc = pc->tc; 

  if (peek(tc) && peek(tc)->type == TokNot) {
    pull(tc); // Consume '!'
    UnaryExprAST* unary_expr = (UnaryExprAST*)S_malloc(sizeof(UnaryExprAST));

    unary_expr->TYPE = AST_UnaryExpr;
    unary_expr->expr = parse_unary_expression(pc);
    return unary_expr;
  }
  
  return parse_simple_expression(pc);
}

static void *parse_compare_expression(ParserContext* pc) {
  void* node = parse_unary_expression(pc);
  TokenizerContext* tc = pc->tc;

  while (peek(tc) &&
         (peek(tc)->type == TokEqual || peek(tc)->type == TokNotEqual ||
          peek(tc)->type == TokGreater || peek(tc)->type == TokLesser ||
          peek(tc)->type == TokEqualGreater ||
          peek(tc)->type == TokEqualLesser)) {
    
    Token* operator_token = pull(tc);
    TokenType op = operator_token->type;
    void* right = parse_unary_expression(pc);

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

    bin_expr->TYPE = AST_BinExpr;
    bin_expr->left = node;
    bin_expr->right = right;

    bin_expr->opType = op_type;

    node = bin_expr;
  }
  return node;
}

static void *parse_expression(ParserContext* pc) {
  void* node = parse_compare_expression(pc);
  TokenizerContext* tc = pc->tc;

  while (peek(tc) && (peek(tc)->type == TokOr || peek(tc)->type == TokAnd)) {
    TokenType op = pull(tc)->type;
    void* right = parse_compare_expression(pc);

    OperatorType op_type = OpNone;
    switch (op) {
    case TokOr: op_type = OpOR; break;
    case TokAnd: op_type = OpAND; break;
    default:
      panic(L"Unknown operator type.", tc);
    }

    BinExprAST* bin_expr = (BinExprAST*)S_malloc(sizeof(BinExprAST));

    bin_expr->TYPE = AST_BinExpr;
    bin_expr->left = node;
    bin_expr->right = right;
    bin_expr->opType = op_type;

    node = bin_expr;
  }

  return node;
}

