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

#define MAX_CLASS_COUNT 512
#define MAX_FUNC_COUNT 512

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
        short byte;
        bool is_integer;
};

struct StringLiteralAST {
        struct Token *str_tok;
};

struct IdentifierAST {
        struct Token *ident;
};

struct BoolLiteralAST {
        struct Token *bool_tok;
};

struct VarDeclAST {
        struct Token *var_name_tok;
        struct Token *var_type_tok;
        struct Node *decl;
        int ac_mod;
};

struct VarDeclBundleAST {
        struct Node **var_decls;
        int var_count;
};

enum OperatorType {
        OpNone,
        OpADD,
        OpSUB,
        OpMUL,
        OpDIV,
        OpEQUAL,
        OpNOTEQUAL,
        OpGREATER,
        OpLESS,
        OpEQUALGREATER,
        OpEQUALLESS,
        OpASSIGN,
        OpOR,
        OpAND
};

struct BinExprAST {
        struct Node *left, *right;
        enum OperatorType opType;
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
        unsigned body_size;
};

struct FuncDeclAST {
        struct Token *func_name_tok;
        struct Token *ret_type_tok;
        struct Node *params; // variable declaration bundle.
        struct Node **body;
        struct FuncData *func_data;

        struct VarData **declared_vars;
        unsigned declared_var_count;

        unsigned body_size;
        int ac_mod;
};

struct FuncCallAST {
        struct Token *func_name_tok;
        struct Node **params;
        struct FuncData *func_data;
        int param_size;
};

struct IdentIncreAST {
        struct Token *identifier;
};

struct IdentDecreAST {
        struct Token *identifier;
};

struct ForStmtAST {
        struct Node *init;
        struct Node *cond;
        struct Node *step;
        struct Node **body;
        unsigned body_size;
};

struct ReturnAST {
        struct Node *expr;
};

struct ConstructorAST {
        struct Node *parameters;
        struct Node **body;
        unsigned body_size;
        int ac_mod;
};

struct ClassAST {
        struct Node *initializer;
        struct Node *constructor;

        struct Node **body;
        unsigned body_size;

        struct Token *name_tok;
        struct Token *parent_name_tok;
};

struct NewAST {
        struct Token *name_tok;
        struct Node **parameters;
        int parameter_count;
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

struct FuncData {
        unsigned id;
        const char *func_name;
        const char *return_type;

        const char **arg_types;
        unsigned arg_count;

        bool varargs;
        bool is_syscall;
};

struct VarData {
        unsigned id;
        unsigned offset; // offset data from stack pointer or class
        const char *var_name;
        const char *type;
};

struct ClassData {
        unsigned id;
        const char *class_name, *parent_name;
        struct HTable *member_vars;
        struct HTable *member_funcs;
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

        struct HTable *syscall_smtb;

        unsigned class_data_count;
        struct ClassData *class_data[MAX_CLASS_COUNT];

        unsigned func_data_count;
        struct FuncData *func_data[MAX_FUNC_COUNT];

        struct Scope *current_scope;

        struct ClassData *current_class; // current parsing class.
        struct FuncData *current_func;   // current parsing func.

        struct VarData **declared_local_vars;
        unsigned declared_local_var_count, declared_local_var_capacity;

        struct Node **nodes;
        unsigned node_size, node_capacity;
};

struct ParserContext *gen_pc();

void compile_file(struct ParserContext *pc, struct TokenizerContext *tc);

// parse
struct Node *parse(struct ParserContext *pc, bool is_expr);
void parse_structure(struct ParserContext *pc);

void debug_view_data(struct ParserContext *pc);

#endif
