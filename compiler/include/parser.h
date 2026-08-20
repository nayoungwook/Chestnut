#ifndef PARSER_H
#define PARSER_H

#include <token.h>
#include <type.h>
#include <util.h>

#include <assert.h>

#define ACMOD_PUBLIC 1
#define ACMOD_PRIVATE 2
#define ACMOD_PROTECTED 3
#define ACMOD_DEFAULT 4

#define MAX_CLASS_COUNT 1024
#define MAX_FUNC_COUNT 1024

/*
  AST_NumberLiteral = 
  AST_Identifier = 
  AST_VariableDeclaration = 
  AST_VariableDeclarationBundle = 
  AST_BinExpr = 
  AST_IfStatement = 
  AST_UnaryExpr = 
  AST_FunctionDeclaration = 
  AST_FunctionCall = 
  AST_StringLiteral = Done
  AST_ForStatement = 
  AST_IdentIncrease = 
  AST_IdentDecrease = 
  AST_Return = 
  AST_Class = 
  AST_Constructor = 
  AST_New = 
  AST_Null = Done
  AST_ArrayDeclaration = 
  AST_ArrayAccess = 
  AST_BoolLiteral = 
  AST_Negative = 
*/


enum ASTType {
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
};

struct Node {
        enum ASTType type;
        void *ast;
        struct Node *attr;
};

struct NumberLiteralAST {
        struct Token *num_tok;
	struct Type *type;
};

struct StringLiteralAST {
        struct Token *str_tok;
};

struct IdentifierAST {
        struct Token *ident;
	bool is_attr;

	// semantics
	struct VarData *var_data;
};

struct BoolLiteralAST {
        struct Token *bool_tok;
};

struct VarDeclAST {
        struct Token *var_name_tok;
        struct Token *var_type_tok;
        struct Node *decl;
        int ac_mod;

	// semantics
	struct VarData *var_data;
};

struct VarDeclBundleAST {
        struct Node **var_decls;
        int var_count;
};

enum OperatorType {
        OpNone = 0,
        OpADD = 1,
        OpSUB = 2,
        OpMUL = 3,
        OpDIV = 4,
        OpEQUAL = 5,
        OpNOTEQUAL = 6,
        OpGREATER = 7,
        OpLESS = 8,
        OpEQUALGREATER = 9,
        OpEQUALLESS = 10,
        OpASSIGN = 11,
        OpOR = 12,
        OpAND = 13
};

struct BinExprAST {
        struct Node *left, *right;
        enum OperatorType op_type;
};

struct UnaryExprAST {
        struct Node *expr;
};

enum IfStmtType { StmtNone, StmtIf, StmtElseIf, StmtElse };

struct IfStmtAST {
        enum IfStmtType stmt_type;
        struct Node *cond;
        struct Node *next_stmt;
        struct Node **body;
        unsigned body_count;
};

struct FuncDeclAST {
        struct Token *func_name_tok;
	struct Token *ret_type_tok;
        struct Node *params; // variable declaration bundle.
        struct Node **body;

        unsigned body_count;
        int ac_mod;

	// semantics
        struct FuncData *func_data;
        struct VarData **declared_vars;
        unsigned declared_var_count;
};

struct ParamData {
	struct Node **params;
        int param_count;
};

struct FuncCallAST {
        struct Token *func_name_tok;
        struct Node **params;
        int param_count;
	bool is_attr;

	// semantics
        struct FuncData *func_data;
};

struct IdentIncreAST {
	struct Node *ident_node;
	bool is_attr;
};

struct IdentDecreAST {
        struct Node *ident_node;
	bool is_attr;
};

struct ForStmtAST {
        struct Node *init;
        struct Node *cond;
        struct Node *step;
        struct Node **body;
        unsigned body_count;
};

struct ReturnAST {
        struct Node *expr;
};

struct ConstructorAST {
        struct Node *params;
        struct Node **body;
	struct ClassData *class_data;

	struct Token *func_name;
	
	unsigned body_count;
        int ac_mod;

	// semantics
	struct FuncData *func_data;
	struct VarData **declared_vars;
        unsigned declared_var_count;
};

struct ClassAST {
        struct Node *initializer;
        struct Node *constructor;

        struct Node **body;
        unsigned body_count;
	
        struct Token *name_tok;
        struct Token *parent_name_tok;

	// semantics
	struct ClassData *class_data;
};

struct NewAST {
        struct Token *name_tok;
        struct Node **params;
        int param_count;

	// semantics
	struct ClassData *class_data;
};

struct NullAST {
        struct Token *null_tok;
};

struct ArrayDeclAST {
        int element_count;
        struct Node **elements;
        struct Token *ele_type_tok;
};

struct ArrayAccessAST {
        struct Node **indexes;
        struct Node *target_array;
        int access_count;
};

struct NegAST {
        struct Node *ast;
};

enum ScopeData {
	ScopeNone, ScopeLocal, ScopeGlobal, ScopeClass, ScopeSyscall, ScopeArray
};

struct FuncData {
        unsigned id;
        const char *func_name;
        struct Type *return_type;

        struct Type **arg_types;
        unsigned arg_count;

	bool is_constructor;
	
        bool varargs;

	enum ScopeData scope_data;
};

struct VarData {
        unsigned id;
        unsigned offset; // offset data from stack pointer or class
	struct Type *type;
        const char *var_name;

	enum ScopeData scope_data;
};

struct ClassData {
        unsigned id;

	struct Type *class_type, *parent_type;

        struct HTable *member_vars;
        struct HTable *member_funcs;

	struct FuncData *constructor;
	unsigned instance_size;
	unsigned instance_align;
	unsigned inherited_var_count;
	unsigned inherited_func_count;
	int registration_state;
	struct ClassAST *ast;
};

struct Scope {
        struct HTable *local_var_smtb;
        struct Scope *prev_scope;
};

struct ParserContext {
        struct TokenizerContext *tc;

        struct Queue *first_pass_queue;
        struct Queue *second_pass_queue;

        struct HTable *glob_var_smtb;  // VarData will be stored.
        struct HTable *glob_func_smtb; // FuncData will be stored.

        struct HTable *class_type_smtb;     // Type will be stored.
        struct HTable *primitive_type_smtb; // Type will be stored.

	struct Type **numeric_type_array;
	unsigned numeric_type_count, numeric_type_capacity;
		
        struct HTable *syscall_smtb;

        unsigned class_data_count;
        struct ClassData *class_data[MAX_CLASS_COUNT];

        unsigned func_data_count;
        struct FuncData *func_data[MAX_FUNC_COUNT];

        struct Scope *current_scope;

        struct VarData **declared_local_vars;
        unsigned declared_local_var_count, declared_local_var_capacity;

        struct ClassData *current_class; // current parsing class.
        struct FuncData *current_func;   // current parsing func.

        struct Node **nodes;
        unsigned node_count, node_capacity;
};

struct ParserContext *gen_pc();

void compile_file(struct ParserContext *pc, struct TokenizerContext *tc);

// parse
struct Node *parse_stmt(struct ParserContext *pc);
struct Node *parse_expr_node(struct ParserContext *pc);

void parse_structure(struct ParserContext *pc);
void debug_view_data(struct ParserContext *pc);

#endif
