#ifndef PARSER_H
#define PARSER_H

#include <token.h>
#include <type.h>

typedef enum {
  AST_NumberLiteral = 0,
  AST_Identifier = 1,
  AST_VariableDeclaration = 2,
  AST_VariableDeclarationBundle = 3,
  AST_BinExpr = 4,
  AST_IfStatement = 5,
  AST_UnaryExpr = 6,
  AST_FunctionDeclaration = 7,
  AST_FunctionCall = 8,
  AST_StringLiteral = 9,
  AST_ForStatement = 10,
  AST_IdentIncrease = 11,
  AST_IdentDecrease = 12,
  AST_Return = 13,
  AST_Class = 14,
  AST_Constructor = 15,
  AST_New = 16,
  AST_Null = 17,
  AST_ArrayDeclaration = 18,
  AST_ArrayAccess = 19,
  AST_BoolLiteral = 20,
  AST_Negative = 21,
}ASTType;

typedef struct {
  ASTType TYPE;
  Token* num_tok;
  short byte;
} NumberLiteralAST;

typedef struct {
  ASTType TYPE;
  Token* str_tok;
} StringLiteralAST;

typedef struct {
  ASTType TYPE;
  Token* ident;
  void* attribute;
} IdentifierAST;

typedef struct {
  ASTType TYPE;
  Token* bool_tok;
} BoolLiteralAST;

typedef struct {
  ASTType TYPE;
  Token* var_name_tok;
  Token* var_type_tok;
  void* decl;
  int ac_mod;
} VarDeclAST;

typedef struct {
  ASTType TYPE;
  VarDeclAST** var_decls;
  int var_count;
} VarDeclBundleAST;

typedef enum { OpNone, OpADD, OpSUB, OpMUL, OpDIV, OpEQUAL, OpNOTEQUAL, OpGREATER, OpLESS, OpEQUALGREATER, OpEQUALLESS, OpASSIGN, OpOR, OpAND } OperatorType;

typedef struct {
  ASTType TYPE;
  void* left, * right;
  OperatorType opType;
}BinExprAST;

typedef struct {
  ASTType TYPE;
  void* expr;
}UnaryExprAST;

typedef enum { StmtIf, StmtElseIf, StmtElse } IfStmtType;

typedef struct {
  ASTType TYPE;
  IfStmtType if_type;
  void* cond;
  void* next_stmt;
  void** body;
  int body_count;
} IfStmtAST;

typedef struct {
  ASTType TYPE;
  Token* func_name_tok;
  Token* ret_type_tok;
  VarDeclBundleAST* parameters;
  void** body;
  int body_count;
  int ac_mod;
} FuncDeclAST;

typedef struct {
  ASTType TYPE;
  Token* func_name_tok;
  void** parameters;
  int parameter_count;
  void* attribute;
} FuncCallAST;

typedef struct {
  ASTType TYPE;
  Token* identifier;
} IdentIncreAST;

typedef struct {
  ASTType TYPE;
  Token* identifier;
} IdentDecreAST;

typedef struct {
  ASTType TYPE;
  void* init;
  void* condition;
  void* step;
  void** body;
  int body_count;
} ForStmtAST;

typedef struct {
  ASTType TYPE;
  void* expression;
} ReturnAST;

typedef struct {
  ASTType TYPE;
  VarDeclBundleAST* parameters;
  void** body;
  int body_count;
  int ac_mod;
} ConstructorAST;

typedef struct {
  ASTType TYPE;

  void* initializer;
  ConstructorAST* constructor;

  VarDeclBundleAST** member_variables;
  FuncDeclAST** member_functions;

  int member_variable_bundle_count;
  int member_function_count;

  Token* class_name_token;
  Token* parent_class_name_token;

} ClassAST;

typedef struct {
  ASTType TYPE;
  Token* class_name_token;
  void** parameters;
  int parameter_count;
} NewAST;

typedef struct {
  ASTType TYPE;
}NullAST;

typedef struct {
  ASTType TYPE;
  int element_count;
  void** elements;
  Token* ele_type_tok;
} ArrayDeclAST;

typedef struct {
  ASTType TYPE;
  void** indexes;
  int access_count;
  IdentifierAST* target_array;
  void* attribute;
}ArrayAccessAST;

typedef struct {
  ASTType TYPE;
  void* ast;
}NegAST;

typedef struct {
  TokenizerContext* tc;
} ParserContext;
ParserContext *gen_pc(TokenizerContext *tc);

// parse
void *parse(ParserContext* pc);

#endif
