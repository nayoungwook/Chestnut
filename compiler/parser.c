#include <parser.h>
#include <error.h>

ParserContext *gen_pc(TokenizerContext *tc) {
  ParserContext *pc = (ParserContext *)S_malloc(sizeof(ParserContext *));

  pc->tc = tc;  
  
  return pc;
}

void *parse(ParserContext *pc) {
  
  return NULL;
}  

void* parse_term(ParserContext* pc) {
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

void* parse_simple_expression(ParserContext* pc) {
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

void *parse_unary_expression(ParserContext *pc) {
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

void* parse_compare_expression(ParserContext* pc) {
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

void* parse_expression(ParserContext* pc) {
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

