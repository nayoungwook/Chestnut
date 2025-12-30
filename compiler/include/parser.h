#ifndef PARSER_H
#define PARSER_H

#include <token.h>
#include <type.h>
#include <util.h>

#define ACMOD_PUBLIC 1
#define ACMOD_PRIVATE 2
#define ACMOD_PROTECTED 3
#define ACMOD_DEFAULT 4

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

typedef struct _Node {
  ASTType type;
  void* ast;
  struct _Node* attribute;
} Node;

typedef struct {
  Token* num_tok;
  short byte;
} NumberLiteralAST;

typedef struct {
  Token* str_tok;
} StringLiteralAST;

typedef struct {
  Token* ident;
} IdentifierAST;

typedef struct {
  Token* bool_tok;
} BoolLiteralAST;

typedef struct {
  Token* var_name_tok;
  Token* var_type_tok;
  Node* decl;
  int ac_mod;
} VarDeclAST;

typedef struct {
  Node** var_decls;
  int var_count;
} VarDeclBundleAST;

typedef enum { OpNone, OpADD, OpSUB, OpMUL, OpDIV, OpEQUAL, OpNOTEQUAL, OpGREATER, OpLESS, OpEQUALGREATER, OpEQUALLESS, OpASSIGN, OpOR, OpAND } OperatorType;

typedef struct {
  Node* left, * right;
  OperatorType opType;
}BinExprAST;

typedef struct {
  Node* expr;
}UnaryExprAST;

typedef enum { StmtNone, StmtIf, StmtElseIf, StmtElse } IfStmtType;

typedef struct {
  IfStmtType stmt_type;
  Node* cond;
  Node* next_stmt;
  Node** body;
  unsigned body_size;
} IfStmtAST;

typedef struct {
  Token* func_name_tok;
  Token* ret_type_tok;
  Node* params; // variable declaration bundle.
  Node** body;
  unsigned body_size;
  int ac_mod;
} FuncDeclAST;

typedef struct {
  Token* func_name_tok;
  Node** params;
  int param_size;
} FuncCallAST;

typedef struct {
  Token* identifier;
} IdentIncreAST;

typedef struct {
  Token* identifier;
} IdentDecreAST;

typedef struct {
  Node* init;
  Node* cond;
  Node* step;
  Node** body;
  unsigned body_size;
} ForStmtAST;

typedef struct {
  Node* expr;
} ReturnAST;

typedef struct {
  Node* parameters;
  Node** body;
  unsigned body_size;
  int ac_mod;
} ConstructorAST;

typedef struct {
  Node* initializer;
  Node* constructor;

  Node** body;
  unsigned body_size;

  Token* name_tok;
  Token* parent_name_tok;

} ClassAST;

typedef struct {
  Token* name_tok;
  Node** parameters;
  int parameter_count;
} NewAST;

typedef struct {

}NullAST;

typedef struct {
  int element_count;
  Node** elements;
  Token* ele_type_tok;
} ArrayDeclAST;

typedef struct {
  Node** indexes;
  int access_count;
  Node* target_array;
}ArrayAccessAST;

typedef struct {
  Node* ast;
}NegAST;

typedef struct _FuncData{
  unsigned id;
  Node* node;
} FuncData;

typedef struct _VarData {
  unsigned id;
  Node* node;
}VarData;

typedef struct _ClassData {
  unsigned id;
  Node* node;
  HTable* member_vars;
  HTable* member_funcs;
  struct _ClassData* paren_class;
} ClassData;

typedef struct {
  TokenizerContext* tc;

  HTable* glob_var_smtb;
  HTable* glob_func_smtb;
  HTable* class_smtb;

  ClassData* current_class; // current parsing class.
  FuncData* current_func; // current parsing func.
} ParserContext;

ParserContext *gen_pc(TokenizerContext *tc);

// parse
Node *parse(ParserContext* pc, bool is_expr);

#endif
